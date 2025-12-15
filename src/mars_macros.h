#ifndef MARS_MACROS_H
#define MARS_MACROS_H 1

import mars;

#define TRY(proc) do {\
    mars::Error<mars::noreturn> procResult = proc;\
    if(!procResult.okay()) return procResult;\
} while(false)
#endif