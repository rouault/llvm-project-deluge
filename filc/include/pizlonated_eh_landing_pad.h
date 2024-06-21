/*
 * Copyright (c) 2024 Epic Games, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY EPIC GAMES, INC. ``AS IS AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL EPIC GAMES, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef PIZLONATED_EH_LANDING_PAD_H
#define PIZLONATED_EH_LANDING_PAD_H

#ifdef __cplusplus
extern "C" {
#endif

#if 0
} /* Tell emacs what's up. */
#endif

#ifdef __cplusplus
typedef bool pizlonated_eh_bool;
#else
typedef _Bool pizlonated_eh_bool;
#endif

struct pizlonated_eh_filter;
struct pizlonated_eh_landing_pad;
typedef struct pizlonated_eh_filter pizlonated_eh_filter;
typedef struct pizlonated_eh_landing_pad pizlonated_eh_landing_pad;

/* The pizlonated_eh_landing_pad is returned as the language-specific data when using the Fil-C++
   compiler. Currently, the format of that data is private to the Fil-C compiler and runtime, except
   for the parts that are exposed as static inline functions.
   
   Concretely, when you're in a personality function and you call _Unwind_GetLanguageSpecificData,
   you get a const pizlonated_eh_landing_pad*. */

unsigned pizlonated_eh_landing_pad_get_num_actions(const pizlonated_eh_landing_pad* pad);

int pizlonated_eh_landing_pad_get_encoded_action(const pizlonated_eh_landing_pad* pad,
                                                 unsigned action_index);

static inline pizlonated_eh_bool pizlonated_eh_encoded_action_is_cleanup(int encoded_action)
{
    return !encoded_action;
}

static inline pizlonated_eh_bool pizlonated_eh_encoded_action_is_catch(int encoded_action)
{
    return encoded_action > 0;
}

static inline pizlonated_eh_bool pizlonated_eh_encoded_action_is_filter(int encoded_action)
{
    return encoded_action < 0;
}

static inline pizlonated_eh_bool pizlonated_eh_landing_pad_action_is_cleanup(
    const pizlonated_eh_landing_pad* pad,
    unsigned action_index)
{
    return pizlonated_eh_encoded_action_is_cleanup(
        pizlonated_eh_landing_pad_get_encoded_action(pad, action_index));
}

static inline pizlonated_eh_bool pizlonated_eh_landing_pad_action_is_catch(
    const pizlonated_eh_landing_pad* pad,
    unsigned action_index)
{
    return pizlonated_eh_encoded_action_is_catch(
        pizlonated_eh_landing_pad_get_encoded_action(pad, action_index));
}

static inline pizlonated_eh_bool pizlonated_eh_landing_pad_action_is_filter(
    const pizlonated_eh_landing_pad* pad, unsigned action_index)
{
    return pizlonated_eh_encoded_action_is_filter(
        pizlonated_eh_landing_pad_get_encoded_action(pad, action_index));
}

const void* pizlonated_eh_landing_pad_get_caught_type_by_encoded_action(
    const pizlonated_eh_landing_pad* pad,
    int encoded_action);

const pizlonated_eh_filter* pizlonated_eh_landing_pad_get_filter_by_encoded_action(
    const pizlonated_eh_landing_pad* pad,
    int encoded_action);

static inline const void* pizlonated_eh_landing_pad_get_caught_type_by_action_index(
    const pizlonated_eh_landing_pad* pad,
    unsigned action_index)
{
    return pizlonated_eh_landing_pad_get_caught_type_by_encoded_action(
        pad, pizlonated_eh_landing_pad_get_encoded_action(pad, action_index));
}

static inline const pizlonated_eh_filter* pizlonated_eh_landing_pad_get_filter_by_action_index(
    pizlonated_eh_landing_pad* pad,
    unsigned action_index)
{
    return pizlonated_eh_landing_pad_get_filter_by_encoded_action(
        pad, pizlonated_eh_landing_pad_get_encoded_action(pad, action_index));
}

unsigned pizlonated_eh_filter_get_num_types(const pizlonated_eh_filter* filter);

const void* pizlonated_eh_filter_get_type(const pizlonated_eh_filter* filter, unsigned index);

#ifdef __cplusplus
}
#endif

#endif /* PIZLONATED_EH_LANDING_PAD_H */

