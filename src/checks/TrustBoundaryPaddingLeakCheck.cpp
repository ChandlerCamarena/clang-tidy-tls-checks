//===-- TrustBoundaryPaddingLeakCheck.cpp ---------------------------------===//
//
// Thesis prototype: security-misc-padding-boundary-leak
//
// Detects record types transferred by value across trust-boundary-annotated
// functions when the type contains ABI-introduced padding bytes that may
// remain uninitialized (thesis Chapters 6–7).
//
//===----------------------------------------------------------------------===//

#include "TrustBoundaryPaddingLeakCheck.h"

#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>  // getenv
#include <fstream>
#include <mutex>
#include <string>

using namespace clang::ast_matchers;

namespace clang::tidy {

// ---------------------------------------------------------------------------
// Event log — CSV rows written for every extracted EB event (hit or miss).
// Populates the raw data for Tables 8.1 and 8.2 in the thesis.
// Format: boundary_fn,event_kind,type_name,type_size,has_padding,pad_bytes,
//         init_class,evidence_level,diag_emitted
// ---------------------------------------------------------------------------
static std::mutex LogMutex;

void TrustBoundaryPaddingLeakCheck::logEvent(StringRef BoundaryFn,
                                              StringRef EventKind,
                                              StringRef TypeName,
                                              bool HasPadding,
                                              uint64_t PadBytes,
                                              InitClass IC,
                                              bool DiagEmitted) {
  const char *LogPath = std::getenv("PADDING_LEAK_LOG");
  if (!LogPath) return;

  std::string ICStr;
  switch (IC) {
  case InitClass::WholeObject: ICStr = "whole_object"; break;
  case InitClass::FieldWise:   ICStr = "field_wise";   break;
  case InitClass::Unknown:     ICStr = "unknown";      break;
  }

  std::string Level = "E0";
  if (HasPadding && IC == InitClass::WholeObject) Level = "E2_suppressed";
  else if (HasPadding && IC == InitClass::FieldWise) Level = "E3";
  else if (HasPadding && IC == InitClass::Unknown)   Level = "E2";

  std::lock_guard<std::mutex> Guard(LogMutex);
  std::ofstream Out(LogPath, std::ios::app);
  if (!Out.is_open()) return;

  Out << BoundaryFn.str() << ","
      << EventKind.str()  << ","
      << TypeName.str()   << ","
      << (HasPadding ? "true" : "false") << ","
      << PadBytes         << ","
      << ICStr            << ","
      << Level            << ","
      << (DiagEmitted ? "true" : "false") << "\n";
}

// ---------------------------------------------------------------------------
// Annotation check — does FD carry annotate("trust_boundary")?
// ---------------------------------------------------------------------------
bool TrustBoundaryPaddingLeakCheck::isTrustBoundary(const FunctionDecl *FD) {
  for (const Attr *A : FD->attrs()) {
    if (const auto *Ann = dyn_cast<AnnotateAttr>(A)) {
      if (Ann->getAnnotation() == "trust_boundary")
        return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// Padding computation (thesis §6.3)
//
// HasPadding(T) is true iff there exist byte positions in [0, sizeof(T)) that
// are not covered by the value representation of any declared field.
// Gaps before/between fields and tail padding both qualify.
//
// Bit-fields are conservatively skipped (they pack differently and are unusual
// in the ABI-critical structs we care about).
// ---------------------------------------------------------------------------
bool TrustBoundaryPaddingLeakCheck::computeHasPadding(const RecordDecl *RD,
                                                       ASTContext &Ctx) {
  if (!RD || !RD->isCompleteDefinition())
    return false;

  const ASTRecordLayout &Layout = Ctx.getASTRecordLayout(RD);
  uint64_t TotalBytes = Layout.getSize().getQuantity();

  uint64_t CoveredEnd = 0; // highest byte covered by a field so far

  for (const FieldDecl *FD : RD->fields()) {
    if (FD->isBitField()) continue; // skip bitfields

    // getFieldOffset returns in *bits*; convert to bytes.
    uint64_t OffsetBytes = Layout.getFieldOffset(FD->getFieldIndex()) / 8;
    uint64_t FieldBytes  = Ctx.getTypeSizeInChars(FD->getType()).getQuantity();

    // Gap before this field → padding exists.
    if (OffsetBytes > CoveredEnd)
      return true;

    uint64_t End = OffsetBytes + FieldBytes;
    if (End > CoveredEnd)
      CoveredEnd = End;
  }

  // Tail padding: covered range doesn't reach end of struct.
  return CoveredEnd < TotalBytes;
}

uint64_t TrustBoundaryPaddingLeakCheck::paddingBytes(const RecordDecl *RD,
                                                      ASTContext &Ctx) {
  if (!RD || !RD->isCompleteDefinition()) return 0;

  const ASTRecordLayout &Layout = Ctx.getASTRecordLayout(RD);
  uint64_t TotalBytes = Layout.getSize().getQuantity();
  uint64_t CoveredEnd = 0;
  uint64_t PadCount   = 0;

  // Collect all covered intervals, then measure gaps.
  struct Interval { uint64_t Start, End; };
  llvm::SmallVector<Interval, 16> Covered;

  for (const FieldDecl *FD : RD->fields()) {
    if (FD->isBitField()) continue;
    uint64_t Off  = Layout.getFieldOffset(FD->getFieldIndex()) / 8;
    uint64_t Size = Ctx.getTypeSizeInChars(FD->getType()).getQuantity();
    Covered.push_back({Off, Off + Size});
  }

  // Sort by start offset.
  llvm::sort(Covered, [](const Interval &A, const Interval &B) {
    return A.Start < B.Start;
  });

  CoveredEnd = 0;
  for (const auto &I : Covered) {
    if (I.Start > CoveredEnd)
      PadCount += I.Start - CoveredEnd;
    if (I.End > CoveredEnd)
      CoveredEnd = I.End;
  }
  PadCount += TotalBytes - CoveredEnd; // tail padding
  return PadCount;
}

// ---------------------------------------------------------------------------
// Initialization classification (thesis §6.4)
//
// Conservative structural heuristic — no dataflow, translation-unit scope.
//
// WholeObject: the VarDecl has a value-initializer that covers the entire
//   object representation (e.g., T v = {0}, T v = {}, T v; memset => TODO).
// FieldWise:   the variable has some initializer but not whole-object.
// Unknown:     no reliable evidence (e.g., non-DeclRefExpr, no init visible).
//
// Note: memset detection would require walking the parent function's body and
// doing dominance reasoning. As documented in §6.4, this is treated as a
// staged extension; the current implementation is intentionally conservative.
// ---------------------------------------------------------------------------
TrustBoundaryPaddingLeakCheck::InitClass
TrustBoundaryPaddingLeakCheck::classifyInit(const Expr *E, ASTContext &Ctx) {
  if (!E) return InitClass::Unknown;

  // Strip implicit casts, parens, and address-of so we reach the base operand.
  const Expr *Base = E->IgnoreParenImpCasts();

  // Materialised temporaries (C++ only): T{} or T{0} as a function argument.
  if (const auto *MT = dyn_cast<MaterializeTemporaryExpr>(Base))
    Base = MT->getSubExpr()->IgnoreParenImpCasts();

  // C++ constructor expression: T() or T{} or T(0, ...)
  if (const auto *CE = dyn_cast<CXXConstructExpr>(Base)) {
    // Default constructor with no arguments = value initialisation → whole.
    if (CE->getNumArgs() == 0)
      return InitClass::WholeObject; // CK_Complete removed in Clang 21
    return InitClass::FieldWise; // conservative
  }

  // Compound literal: (struct T){0} or (struct T){}
  if (const auto *CL = dyn_cast<CompoundLiteralExpr>(Base)) {
    if (const auto *ILE =
            dyn_cast<InitListExpr>(CL->getInitializer()->IgnoreParenImpCasts())) {
      if (ILE->getNumInits() == 0) return InitClass::WholeObject; // {}
      // {0} — first initialiser is the integer 0, rest zero-extended by spec.
      if (ILE->getNumInits() >= 1) {
        if (const auto *IL = dyn_cast<IntegerLiteral>(
                ILE->getInit(0)->IgnoreParenImpCasts())) {
          if (IL->getValue() == 0) return InitClass::WholeObject;
        }
      }
    }
    return InitClass::FieldWise;
  }

  // DeclRefExpr — look at the variable's own declaration and initialiser.
  if (const auto *DRE = dyn_cast<DeclRefExpr>(Base)) {
    const auto *VD = dyn_cast<VarDecl>(DRE->getDecl());
    if (!VD) return InitClass::Unknown;

    const Expr *Init = VD->getInit();
    if (!Init) {
      // No initialiser at all: automatic variable with no init = Unknown.
      return InitClass::Unknown;
    }

    // Initialiser is an InitListExpr: T v = {} or T v = {0}
    if (const auto *ILE = dyn_cast<InitListExpr>(Init->IgnoreParenImpCasts())) {
      if (ILE->getNumInits() == 0) return InitClass::WholeObject;
      if (ILE->getNumInits() >= 1) {
        if (const auto *IL = dyn_cast<IntegerLiteral>(
                ILE->getInit(0)->IgnoreParenImpCasts())) {
          if (IL->getValue() == 0) return InitClass::WholeObject;
        }
      }
      // Non-zero init list → field-wise at best.
      return InitClass::FieldWise;
    }

    // Initialiser is present but not an init-list (e.g., copy from another
    // variable, function return, etc.) — conservative.
    return InitClass::FieldWise;
  }

  return InitClass::Unknown;
}

// ---------------------------------------------------------------------------
// Diagnostic emission
// ---------------------------------------------------------------------------
void TrustBoundaryPaddingLeakCheck::emitDiagnostic(SourceLocation Loc,
                                                    const RecordDecl *RD,
                                                    StringRef BoundaryFnName,
                                                    StringRef EventKind,
                                                    InitClass IC,
                                                    uint64_t PadBytes,
                                                    ASTContext &Ctx) {
  bool HasPad = computeHasPadding(RD, Ctx);
  bool ShouldReport = HasPad && (IC != InitClass::WholeObject);

  // Log every EB event regardless of whether we report.
  logEvent(BoundaryFnName, EventKind, RD->getNameAsString(),
           HasPad, PadBytes, IC, ShouldReport);

  if (!ShouldReport) return;

  // Determine evidence level (thesis §5.6.2).
  // E3: padding + visible field-wise-only init (high confidence)
  // E2: padding + init state unknown under translation-unit visibility
  StringRef Level = (IC == InitClass::FieldWise) ? "E3" : "E2";

  uint64_t TotalBytes =
      Ctx.getASTRecordLayout(RD).getSize().getQuantity();

  diag(Loc,
       "[%0] boundary transfer of '%1' (%2 bytes, ~%3 padding bytes) by value "
       "at trust boundary '%4': the ABI copies the full object representation "
       "including padding bytes that may contain indeterminate or stale data. "
       "Remediation: (a) whole-object zero-init before assignment "
       "(e.g., `%1 v = {0};`), or (b) serialise only semantic fields into an "
       "explicit boundary buffer.")
      << Level
      << RD->getNameAsString()
      << TotalBytes
      << PadBytes
      << BoundaryFnName;
}

// ---------------------------------------------------------------------------
// AST matcher registration
//
// Two matchers:
//   1. callExpr to any annotated function  → call-by-value events
//   2. returnStmt inside any annotated fn  → return-by-value events
//
// Annotation filtering (trust_boundary vs other annotations) is done in
// check() to avoid writing a custom AST matcher.
// ---------------------------------------------------------------------------
void TrustBoundaryPaddingLeakCheck::registerMatchers(MatchFinder *Finder) {
  // --- Matcher 1: call-by-value at boundary ---
  // Match calls whose callee has any AnnotateAttr; filter in check().
  Finder->addMatcher(
      callExpr(callee(functionDecl(hasAttr(attr::Annotate)).bind("callee")))
          .bind("callExpr"),
      this);

  // --- Matcher 2: return-by-value from boundary function ---
  Finder->addMatcher(
      returnStmt(
          hasReturnValue(expr().bind("retVal")),
          hasAncestor(functionDecl(hasAttr(attr::Annotate)).bind("fnDecl")))
          .bind("retStmt"),
      this);
}

// ---------------------------------------------------------------------------
// Match result dispatch
// ---------------------------------------------------------------------------
void TrustBoundaryPaddingLeakCheck::check(
    const MatchFinder::MatchResult &Result) {
  ASTContext &Ctx = *Result.Context;

  // -----------------------------------------------------------------------
  // Case 1: call-by-value event
  // -----------------------------------------------------------------------
  if (const auto *Call = Result.Nodes.getNodeAs<CallExpr>("callExpr")) {
    const auto *Callee =
        Result.Nodes.getNodeAs<FunctionDecl>("callee");
    if (!Callee || !isTrustBoundary(Callee)) return;

    StringRef FnName = Callee->getName();
    unsigned NumParams = Callee->getNumParams();

    for (unsigned I = 0; I < Call->getNumArgs() && I < NumParams; ++I) {
      const ParmVarDecl *Param = Callee->getParamDecl(I);
      QualType ParamTy = Param->getType().getCanonicalType();

      // Must be a plain record type (by value, not pointer or reference).
      if (!ParamTy->isRecordType()) continue;

      const RecordType *RT = ParamTy->getAs<RecordType>();
      if (!RT) continue;
      const RecordDecl *RD = RT->getDecl();
      if (!RD->isCompleteDefinition()) continue;

      const Expr *Arg = Call->getArg(I);
      InitClass IC  = classifyInit(Arg, Ctx);
      uint64_t Pad  = paddingBytes(RD, Ctx);

      emitDiagnostic(Arg->getBeginLoc(), RD, FnName, "call", IC, Pad, Ctx);
    }
    return;
  }

  // -----------------------------------------------------------------------
  // Case 2: return-by-value event
  // -----------------------------------------------------------------------
  if (const auto *Ret = Result.Nodes.getNodeAs<ReturnStmt>("retStmt")) {
    const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("fnDecl");
    if (!FD || !isTrustBoundary(FD)) return;

    QualType RetTy = FD->getReturnType().getCanonicalType();
    if (!RetTy->isRecordType()) return;

    const RecordType *RT = RetTy->getAs<RecordType>();
    if (!RT) return;
    const RecordDecl *RD = RT->getDecl();
    if (!RD->isCompleteDefinition()) return;

    const auto *RetVal = Result.Nodes.getNodeAs<Expr>("retVal");
    InitClass IC  = classifyInit(RetVal, Ctx);
    uint64_t Pad  = paddingBytes(RD, Ctx);

    emitDiagnostic(Ret->getBeginLoc(), RD, FD->getName(), "ret", IC, Pad, Ctx);
  }
}

} // namespace clang::tidy
