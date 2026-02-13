#include "DisableVerifyCheck.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang::tidy {

void DisableVerifyCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      callExpr(
          callee(functionDecl(hasAnyName("SSL_CTX_set_verify", "SSL_set_verify"))),
          hasArgument(1, expr().bind("modeExpr"))
      ).bind("verifyCall"),
      this);
}

void DisableVerifyCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Call = Result.Nodes.getNodeAs<CallExpr>("verifyCall");
  const auto *Mode = Result.Nodes.getNodeAs<Expr>("modeExpr");
  if (!Call || !Mode || !Result.Context)
    return;

  // Evaluate mode as an integer constant, if possible.
  Expr::EvalResult Eval;
  if (!Mode->EvaluateAsInt(Eval, *Result.Context))
    return;

  // SSL_VERIFY_NONE is effectively 0. If mode is 0, verification is disabled.
  const llvm::APSInt &V = Eval.Val.getInt();
  if (V != 0)
    return;

  diag(Call->getBeginLoc(),
       "certificate verification disabled (verify mode is 0 / SSL_VERIFY_NONE); "
       "this makes TLS connections vulnerable to MITM attacks")
      << FixItHint::CreateReplacement(Mode->getSourceRange(), "SSL_VERIFY_PEER");
}

} // namespace clang::tidy

