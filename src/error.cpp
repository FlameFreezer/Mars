#include "error.h"

Error<noreturn> success() noexcept {
    return Error<noreturn>();
}
