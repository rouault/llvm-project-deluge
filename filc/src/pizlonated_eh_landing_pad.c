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

#include "pizlonated_eh_landing_pad.h"

#include <stdfil.h>

struct pizlonated_eh_filter;
struct pizlonated_eh_landing_pad;
struct pizloanted_eh_type_table;
typedef struct pizlonated_eh_filter pizlonated_eh_filter;
typedef struct pizlonated_eh_landing_pad pizlonated_eh_landing_pad;
typedef struct pizlonated_eh_type_table pizlonated_eh_type_table;

/* The exception handling data for a particular landing pad. This is returned as the language-specific
   data by _Unwind_GetLanguageSpecificData. */
struct pizlonated_eh_landing_pad {
    const pizlonated_eh_type_table* type_table;
    unsigned num_actions;

    /* The meaning of actions:
       
       !action        Cleanup, like for running destructors.
       
       action > 0     1-based index into type_table->types_and_filters, indicating a catch type. That
                      type might be NULL, indicating a catch-all, like `catch (...)`.
       
       action < 0     Negative 1-based index to the end of type_table->types_and_filters, indicating a
                      filter. This is used to implement `throw(T)` specifiers in C++. */
    int actions[];
};

struct pizlonated_eh_filter {
    unsigned num_types;
    const void* types[];
};

/* This is a per-module type table; multiple landing pads will point to the same one. */
struct pizlonated_eh_type_table {
    unsigned num_types;

    /* The first num_types entries are type pointers for catch rules. The next num_filters entries are
       type filter pointers for filter rules. */
    const void* types_and_filters[];
};

unsigned pizlonated_eh_landing_pad_get_num_actions(const pizlonated_eh_landing_pad* pad)
{
    return pad->num_actions;
}

int pizlonated_eh_landing_pad_get_encoded_action(const pizlonated_eh_landing_pad* pad,
                                                 unsigned action_index)
{
    return pad->actions[action_index];
}

const void* pizlonated_eh_landing_pad_get_caught_type_by_encoded_action(
    const pizlonated_eh_landing_pad* pad,
    int encoded_action)
{
    ZASSERT(pizlonated_eh_encoded_action_is_catch(encoded_action));
    return pad->type_table->types_and_filters[encoded_action - 1];
}

const pizlonated_eh_filter* pizlonated_eh_landing_pad_get_filter_by_encoded_action(
    const pizlonated_eh_landing_pad* pad,
    int encoded_action)
{
    ZASSERT(pizlonated_eh_encoded_action_is_filter(encoded_action));
    return (const pizlonated_eh_filter*)pad->type_table->types_and_filters[
        -encoded_action - 1 + pad->type_table->num_types];
}

unsigned pizlonated_eh_filter_get_num_types(const pizlonated_eh_filter* filter)
{
    return filter->num_types;
}

const void* pizlonated_eh_filter_get_type(const pizlonated_eh_filter* filter, unsigned index)
{
    return filter->types[index];
}

