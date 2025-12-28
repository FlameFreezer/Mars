#ifndef MARS_MACROS_H
#define MARS_MACROS_H 1

#define TRY(proc) if(mars::Error<mars::noreturn> procResult = proc; !procResult) return procResult

#endif