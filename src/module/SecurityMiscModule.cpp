#include "../checks/TrustBoundaryPaddingLeakCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"

namespace clang::tidy {

class SecurityMiscModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &Factories) override {
    Factories.registerCheck<TrustBoundaryPaddingLeakCheck>(
        "security-misc-padding-boundary-leak");
  }
};

static ClangTidyModuleRegistry::Add<SecurityMiscModule>
    X("security-misc-module",
      "Security misconfiguration checks (thesis prototype).");

volatile int SecurityMiscModuleAnchorSource = 0;

} // namespace clang::tidy
