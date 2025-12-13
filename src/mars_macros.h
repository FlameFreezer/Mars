#define TRY(proc) do {\
    procResult = proc;\
    if(!procResult.okay()) return procResult;\
} while(false)