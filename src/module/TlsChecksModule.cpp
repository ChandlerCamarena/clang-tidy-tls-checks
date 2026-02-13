#include "../checks/DisableVerifyCheck.h"

#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"

namespace clang::tidy {

class TlsChecksModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &Factories) override {
    Factories.registerCheck<DisableVerifyCheck>(
        "tls-cert-verify-disabled");
  }
};

static ClangTidyModuleRegistry::Add<TlsChecksModule>
    X("tls-module", "Custom TLS/OpenSSL checks (thesis module).");

volatile int TlsChecksModuleAnchorSource = 0;

} // namespace clang::tidy

