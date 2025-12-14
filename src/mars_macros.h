#ifndef MARS_MACROS_H
#define MARS_MACROS_H 1

#define TRY(proc) do {\
    procResult = proc;\
    if(!procResult.okay()) return procResult;\
} while(false)
#endif