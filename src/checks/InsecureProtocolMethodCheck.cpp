#include "InsecureProtocolMethodCheck.h"

#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/ADT/StringRef.h"

using namespace clang::ast_matchers;

namespace clang::tidy {

void InsecureProtocolMethodCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      callExpr(
          callee(functionDecl(hasAnyName(
              "SSLv2_method",
              "SSLv3_method",
              "TLSv1_method",
              "TLSv1_1_method",
              "SSLv2_client_method",
              "SSLv2_server_method",
              "SSLv3_client_method",
              "SSLv3_server_method",
              "TLSv1_client_method",
              "TLSv1_server_method",
              "TLSv1_1_client_method",
              "TLSv1_1_server_method"
          )).bind("callee"))
      ).bind("protoCall"),
      this);
}


void InsecureProtocolMethodCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Call = Result.Nodes.getNodeAs<CallExpr>("protoCall");
  const auto *Callee = Result.Nodes.getNodeAs<FunctionDecl>("callee");
  if (!Call || !Callee)
    return;

  llvm::StringRef Name = Callee->getName();

  // Recommend the most appropriate modern API based on naming.
  llvm::StringRef Replacement = "TLS_method";
  if (Name.contains("client"))
    Replacement = "TLS_client_method";
  else if (Name.contains("server"))
    Replacement = "TLS_server_method";

  diag(Call->getBeginLoc(),
       "obsolete TLS/SSL protocol-specific method '%0' used; prefer '%1()' "
       "to negotiate the highest supported TLS version")
      << Name << Replacement;
}

} // namespace clang::tidy

