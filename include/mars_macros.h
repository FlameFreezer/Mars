#pragma once

#define TRY(proc) \
if(auto procResult = proc; !procResult) return procResult

#define TRY_ASSIGN(name, proc, errType) \
if(auto procResult = proc; !procResult) return procResult.moveError<errType>();\
else name = procResult

#define TRY_INIT(type, name, proc, errType) \
type name;\
if(auto procResult = proc; !procResult) return procResult.moveError<errType>();\
else name = procResult