#include "../checks/ModuleLoadedCheck.h"

#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"

namespace clang::tidy {

class TlsChecksModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &Factories) override {
    Factories.registerCheck<ModuleLoadedCheck>("tls-module-loaded");
  }
};

static ClangTidyModuleRegistry::Add<TlsChecksModule>
    X("tls-module", "Custom TLS/OpenSSL checks (thesis module).");

// This anchor is used to force the linker to link in the generated object file
// and thus register the module.
volatile int TlsChecksModuleAnchorSource = 0;

} // namespace clang::tidy

