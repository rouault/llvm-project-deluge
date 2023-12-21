#include "deluge_runtime.h"
#include "deluge_internal.h"
#include "pas_utils.h"

void deluge_validate_type(deluge_type* type)
{
    PAS_ASSERT(type->size);
    PAS_ASSERT(type->alignment);
    PAS_ASSERT(!(type->alignment & ~(type->alignment - 1)));
    PAS_ASSERT(!(type->size % type->alignment));
    if (type->trailing_array) {
        PAS_ASSERT(!type->trailing_array->trailing_array);
        deluge_validate_type(type->trailing_array);
    }
}

bool deluge_type_is_equal(deluge_type* a, deluge_type* b)
{
    size_t index;
    if (a == b)
        return true;
    if (a->size != b->size)
        return false;
    if (a->alignment != b->alignment)
        return false;
    if (a->trailing_array) {
        if (!b->trailing_array)
            return false;
        return deluge_type_is_equal(a->trailing_array, b->trailing_array);
    } else if (b->trailing_array)
        return false;
    for (index = deluge_type_num_words(a); index--;) {
        if (a->word_types[index] != b->word_types[index])
            return false;
    }
    return true;
}

unsigned deluge_type_hash(deluge_type* type)
{
    unsigned result;
    size_t index;
    result = type->size + 3 * type->alignment;
    if (type->trailing_array)
        result += 7 * deluge_type_hash(type->trailing_array);
    for (index = deluge_type_num_words(type); index--;) {
        result *= 11;
        result += type->word_types[index];
    }
    return result;
}

