/*
 * Fil-C unwind API.
 *
 * This is currently all stubbed out and is made to look like Itanium ABI, sicne that's simpler.
 *
 * Includes code copied from LLVM's libunwind, so this is licensed under the Apache License v2.0
 * with LLVM Exceptions.
 */

#ifndef FILC_UNWIND_H
#define FILC_UNWIND_H

#ifdef __cplusplus
extern "C" {
#endif

#if 0
} /* Tell emacs what's up. */
#endif

typedef enum {
    _URC_NO_REASON = 0,
    _URC_OK = 0,
    _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
    _URC_FATAL_PHASE2_ERROR = 2,
    _URC_FATAL_PHASE1_ERROR = 3,
    _URC_NORMAL_STOP = 4,
    _URC_END_OF_STACK = 5,
    _URC_HANDLER_FOUND = 6,
    _URC_INSTALL_CONTEXT = 7,
    _URC_CONTINUE_UNWIND = 8,
} _Unwind_Reason_Code;

typedef enum {
    _UA_SEARCH_PHASE = 1,
    _UA_CLEANUP_PHASE = 2,
    _UA_HANDLER_FRAME = 4,
    _UA_FORCE_UNWIND = 8,
    _UA_END_OF_STACK = 16 // gcc extension to C++ ABI
} _Unwind_Action;

struct _Unwind_Context;   // opaque
struct _Unwind_Exception; // forward declaration
typedef struct _Unwind_Exception _Unwind_Exception;
typedef unsigned long long _Unwind_Exception_Class;

struct _Unwind_Exception {
    _Unwind_Exception_Class exception_class;
    void (*exception_cleanup)(_Unwind_Reason_Code reason,
                              _Unwind_Exception *exc);
};

typedef _Unwind_Reason_Code (*_Unwind_Personality_Fn)(
    int version, _Unwind_Action actions, _Unwind_Exception_Class exceptionClass,
    _Unwind_Exception *exceptionObject, struct _Unwind_Context *context);

//
// The following are the base functions documented by the C++ ABI
//
_Unwind_Reason_Code
    _Unwind_RaiseException(_Unwind_Exception *exception_object);
void _Unwind_Resume(_Unwind_Exception *exception_object); /* Not needed. */
void _Unwind_DeleteException(_Unwind_Exception *exception_object); /* Not needed. */

void* _Unwind_GetGR(struct _Unwind_Context *context, int index);
void _Unwind_SetGR(struct _Unwind_Context *context, int index,
                   void* new_value);
unsigned long _Unwind_GetIP(struct _Unwind_Context *context); /* Not needed. */
void _Unwind_SetIP(struct _Unwind_Context *, unsigned long new_value); /* Not needed. */

typedef _Unwind_Reason_Code (*_Unwind_Stop_Fn)
    (int version,
     _Unwind_Action actions,
     _Unwind_Exception_Class exceptionClass,
     _Unwind_Exception* exceptionObject,
     struct _Unwind_Context* context,
     void* stop_parameter);

unsigned long _Unwind_GetRegionStart(struct _Unwind_Context *context); /* Probably not needed, or
                                                                          could return 0. */
const void* _Unwind_GetLanguageSpecificData(struct _Unwind_Context *context);
_Unwind_Reason_Code _Unwind_ForcedUnwind(_Unwind_Exception *exception_object,
                                         _Unwind_Stop_Fn stop, void *stop_parameter); /* Kinda needed,
                                                                                         but we won't
                                                                                         support it
                                                                                         yet. */

//
// The following are semi-supported extensions to the C++ ABI
//
// Fil-C just supports the ones that libcxxabi wants.
//

// _Unwind_Backtrace() is a gcc extension that walks the stack and calls the
// _Unwind_Trace_Fn once per frame until it reaches the bottom of the stack
// or the _Unwind_Trace_Fn function returns something other than _URC_NO_REASON.
typedef _Unwind_Reason_Code (*_Unwind_Trace_Fn)(struct _Unwind_Context *,
                                                void *);
_Unwind_Reason_Code _Unwind_Backtrace(_Unwind_Trace_Fn, void *);

#ifdef __cplusplus
}
#endif

#endif /* FILC_UNWIND_H */

