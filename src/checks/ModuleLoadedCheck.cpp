#include "ModuleLoadedCheck.h"

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang::tidy {

void ModuleLoadedCheck::registerMatchers(MatchFinder *Finder) {
  // Bind the translation unit so we trigger exactly once per file.
  Finder->addMatcher(translationUnitDecl().bind("tu"), this);
}

void ModuleLoadedCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *TU = Result.Nodes.getNodeAs<TranslationUnitDecl>("tu");
  if (!TU || !Result.SourceManager)
    return;

  diag(TU->getBeginLoc(),
       "TLS thesis module loaded (sanity check). Custom checks are available.");
}

} // namespace clang::tidy

