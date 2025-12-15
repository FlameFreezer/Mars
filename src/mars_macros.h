#ifndef MARS_MACROS_H
#define MARS_MACROS_H 1

import mars;

namespace mars::internal {
    static Error<noreturn> procResult;
}

#define TRY(proc) do {\
    mars::internal::procResult = proc;\
    if(!mars::internal::procResult.okay()) return mars::internal::procResult;\
} while(false)
#endif