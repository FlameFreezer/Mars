#define TRY(proc) do {\
    procResult = proc;\
    if(!procResult.okay()) return procResult;\
} while(0)

#define INIT_TRY(proc) do {\
    procResult = proc;\
    if(!procResult.okay()) {\
        initFail = true;\
        return procResult;\
    } \
} while(0)
