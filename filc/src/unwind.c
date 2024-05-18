#include <unwind.h>
#include <stdfil.h>

_Unwind_Reason_Code
    _Unwind_RaiseException(_Unwind_Exception *exception_object)
{
    (void)exception_object;
    zerror("Not implemented.");
    return _URC_NO_REASON;
}

void _Unwind_Resume(_Unwind_Exception *exception_object)
{
    (void)exception_object;
    zerror("Not implemented.");
}

void _Unwind_DeleteException(_Unwind_Exception *exception_object)
{
    (void)exception_object;
    zerror("Not implemented.");
}

unsigned long _Unwind_GetGR(struct _Unwind_Context *context, int index)
{
    (void)context;
    (void)index;
    zerror("Not implemented.");
    return 0;
}

void _Unwind_SetGR(struct _Unwind_Context *context, int index,
                   unsigned long new_value)
{
    (void)context;
    (void)index;
    (void)new_value;
    zerror("Not implemented.");
}

unsigned long _Unwind_GetIP(struct _Unwind_Context *context)
{
    (void)context;
    zerror("Not implemented.");
    return 0;
}

void _Unwind_SetIP(struct _Unwind_Context *context, unsigned long new_value)
{
    (void)context;
    (void)new_value;
    zerror("Not implemented.");
}

unsigned long _Unwind_GetRegionStart(struct _Unwind_Context *context)
{
    (void)context;
    zerror("Not implemented.");
    return 0;
}

unsigned long _Unwind_GetLanguageSpecificData(struct _Unwind_Context *context)
{
    (void)context;
    zerror("Not implemented.");
    return 0;
}

_Unwind_Reason_Code _Unwind_ForcedUnwind(_Unwind_Exception *exception_object,
                                         _Unwind_Stop_Fn stop, void *stop_parameter)
{
    (void)exception_object;
    (void)stop;
    (void)stop_parameter;
    zerror("Not implemented.");
    return _URC_NO_REASON;
}

_Unwind_Reason_Code _Unwind_Backtrace(_Unwind_Trace_Fn fn, void *arg)
{
    (void)fn;
    (void)arg;
    zerror("Not implemented.");
    return _URC_NO_REASON;
}
