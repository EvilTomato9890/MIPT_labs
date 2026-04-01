#include "vector.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"


#define VECTOR_RETURN_IF_ERROR(error_) \
    do {                               \
        vector_error_t err_ = (error_); \
        if (err_ != VEC_ERR_OK) {      \
            return err_;               \
        }                              \
    } while (0)

static const size_t INITIAL_CAPACITY  = 32;

static const float  GROWTH_FACTOR     = 2.0f;   
static const float  SHRINK_FACTOR     = 0.5f;   
static const float  MIN_LOAD_FACTOR   = 0.25f;  

//================================================================================

static inline unsigned char* vector_ptr(const vector_t* vector, size_t index) {
    return (unsigned char*)vector->data + index * vector->elem_size;
}

static inline const unsigned char* vector_ptr_const(const vector_t* vector, size_t index) {
    return (const unsigned char*)vector->data + index * vector->elem_size;
}

size_t vector_required_bytes(size_t capacity, size_t elem_size) {
    if (elem_size == 0) return 0;
    return capacity * elem_size;
}

//================================================================================

static vector_error_t vector_realloc(vector_t* vector, size_t new_capacity) {
    HARD_ASSERT(vector != NULL, "vector is NULL");

    if (vector->is_static) return VEC_ERR_FULL;

    if (new_capacity < INITIAL_CAPACITY) new_capacity = INITIAL_CAPACITY;
    if (new_capacity == vector->capacity)   return VEC_ERR_OK;

    const size_t old_bytes = vector->capacity * vector->elem_size;
    const size_t new_bytes = new_capacity  * vector->elem_size;

    void* new_data = realloc(vector->data, new_bytes);
    if (new_data == NULL) return VEC_ERR_MEM_ALLOC;

    if (new_bytes > old_bytes) {
        memset((unsigned char*)new_data + old_bytes, 0, new_bytes - old_bytes);
    }

    vector->data     = new_data;
    vector->capacity = new_capacity;

    if (vector->size > vector->capacity) vector->size = vector->capacity;

    return VEC_ERR_OK;
}

static vector_error_t normalize_for_grow(vector_t* vector, size_t need_elems) {
    HARD_ASSERT(vector != NULL, "vector is NULL");

    if (need_elems <= vector->capacity) return VEC_ERR_OK;
    if (vector->is_static)              return VEC_ERR_FULL;

    size_t new_capacity = vector->capacity;
    if (new_capacity < INITIAL_CAPACITY) new_capacity = INITIAL_CAPACITY;

    while (new_capacity < need_elems) {
        size_t next = (size_t)((double)new_capacity * (double)GROWTH_FACTOR);
        if (next <= new_capacity) return VEC_ERR_BAD_ARG; 
        new_capacity = next;
    }

    return vector_realloc(vector, new_capacity);
}


static vector_error_t normalize_for_shrink(vector_t* vector) {
    HARD_ASSERT(vector != NULL, "vector is NULL");

    if (vector->is_static)                    return VEC_ERR_OK;
    if (vector->capacity <= INITIAL_CAPACITY) return VEC_ERR_OK;

    size_t new_capacity = vector->capacity;

    while (new_capacity > INITIAL_CAPACITY) {
        const double load = (new_capacity == 0) ? 1.0 : (double)vector->size / (double)new_capacity;

        if (load >= MIN_LOAD_FACTOR) break;
        size_t candidate = (size_t)((double)new_capacity * (double)SHRINK_FACTOR);
        if (candidate < INITIAL_CAPACITY) candidate = INITIAL_CAPACITY;
        if (candidate < vector->size)     candidate = vector->size;
        if (candidate >= new_capacity) break; 
        new_capacity = candidate;
    }

    if (new_capacity == vector->capacity) return VEC_ERR_OK;
    return vector_realloc(vector, new_capacity);
}


//================================================================================
//                      Р С™Р С•Р Р…РЎРѓРЎвЂљРЎР‚РЎС“Р С”РЎвЂљР С•РЎР‚РЎвЂ№ / Р вЂќР ВµРЎРѓРЎвЂљРЎР‚РЎС“Р С”РЎвЂљР С•РЎР‚РЎвЂ№ / Р С™Р С•Р С—Р С‘РЎР‚Р С•Р Р†Р В°Р В»РЎРЉР В·Р С‘Р С”Р С‘
//================================================================================

vector_error_t vector_init(vector_t* vector, size_t capacity, size_t elem_size) {
    HARD_ASSERT(vector != NULL, "vector is NULL");
    HARD_ASSERT(elem_size > 0, "elem_size must be > 0");

    if (capacity < INITIAL_CAPACITY) capacity = INITIAL_CAPACITY;

    void* data = calloc(capacity, elem_size);
    if (data == NULL) return VEC_ERR_MEM_ALLOC;

    vector->data      = data;
    vector->size      = 0;
    vector->capacity  = capacity;
    vector->elem_size = elem_size;
    vector->is_static = false;

    return VEC_ERR_OK;
}

vector_error_t vector_static_init(vector_t* vector, void* data, size_t capacity, size_t elem_size) {
    HARD_ASSERT(vector != NULL, "vector is NULL");
    HARD_ASSERT(data != NULL, "data is NULL");
    HARD_ASSERT(elem_size > 0, "elem_size must be > 0");

    if (capacity == 0) return VEC_ERR_BAD_ARG;

    memset(data, 0, capacity * elem_size);

    vector->data      = data;
    vector->size      = 0;
    vector->capacity  = capacity;
    vector->elem_size = elem_size;
    vector->is_static = true;

    return VEC_ERR_OK;
}

vector_error_t vector_destroy(vector_t* vector) {
    HARD_ASSERT(vector != NULL, "vector is NULL");

    if (!vector->is_static && vector->data != NULL) free(vector->data);
    memset(vector, 0, sizeof(*vector));

    return VEC_ERR_OK;
}

//================================================================================
//                              Р вЂР В°Р В·Р С•Р Р†РЎвЂ№Р Вµ РЎвЂћРЎС“Р Р…Р С”РЎвЂ Р С‘Р С‘
//================================================================================

size_t vector_size(const vector_t* vector) {
    HARD_ASSERT(vector != NULL, "vector is NULL");
    return vector->size;
}

size_t vector_capacity(const vector_t* vector) {
    HARD_ASSERT(vector != NULL, "vector is NULL");
    return vector->capacity;
}

void* vector_get(const vector_t* vector, size_t index) {
    HARD_ASSERT(vector != NULL, "vector is NULL");
    if (index >= vector->size) return NULL; //TODO - Р вЂ”Р В°Р С”Р С‘Р Р…РЎС“РЎвЂљРЎРЉ Р Р† godbolt
    return (void*)vector_ptr(vector, index);
}

const void* vector_get_const(const vector_t* vector, size_t index) {
    HARD_ASSERT(vector != NULL, "vector is NULL");
    if (index >= vector->size) return NULL;
    return (const void*)vector_ptr_const(vector, index);
}

void* vector_put(const vector_t* vector, size_t index, void* value) {
    HARD_ASSERT(vector != NULL, "vector is NULL");
    HARD_ASSERT(value  != NULL, "Values is nullptr");
    if (index >= vector->size) return NULL;
    void* target_ptr = (void*)vector_ptr(vector, index);
    memcpy(target_ptr, value, vector->elem_size); 
    return target_ptr;
}
void vector_clear(vector_t* vector) {
    HARD_ASSERT(vector != NULL, "vector is NULL");

    vector->size = 0;
    normalize_for_shrink(vector);
}

//================================================================================
//                           Р С›РЎРѓР Р…Р С•Р Р†Р Р…РЎвЂ№Р Вµ РЎвЂћРЎС“Р Р…Р С”РЎвЂ Р С‘Р С‘
//================================================================================

vector_error_t vector_push_back(vector_t* vector, const void* elem) {
    HARD_ASSERT(vector != NULL, "vector is NULL");
    HARD_ASSERT(elem   != NULL, "elem is NULL");

    vector_error_t err = normalize_for_grow(vector, vector->size + 1);
    VECTOR_RETURN_IF_ERROR(err);

    memcpy(vector_ptr(vector, vector->size), elem, vector->elem_size);
    vector->size++;
    return VEC_ERR_OK;
}

vector_error_t vector_pop_back(vector_t* vector, void* elem_out) {
    HARD_ASSERT(vector != NULL, "vector is NULL");

    if (vector->size == 0) return VEC_ERR_NOT_FOUND;
    const size_t last = vector->size - 1;
    if (elem_out != NULL) 
        memcpy(elem_out, vector_ptr(vector, last), vector->elem_size);

    vector->size--;
    vector_error_t err = normalize_for_shrink(vector);
    VECTOR_RETURN_IF_ERROR(err);

    return VEC_ERR_OK;
}

vector_error_t vector_insert(vector_t* vector, size_t index, const void* elem) {
    HARD_ASSERT(vector != NULL, "vector is NULL");
    HARD_ASSERT(elem   != NULL, "elem is NULL");

    if (index > vector->size) return VEC_ERR_BAD_ARG;

    vector_error_t err = normalize_for_grow(vector, vector->size + 1);
    VECTOR_RETURN_IF_ERROR(err);

    if (index != vector->size) {
        void*  dst   = vector_ptr(vector, index + 1);
        void*  src   = vector_ptr(vector, index);
        size_t bytes = (vector->size - index) * vector->elem_size;
        memmove(dst, src, bytes);
    }
    memcpy(vector_ptr(vector, index), elem, vector->elem_size);
    vector->size++;

    return VEC_ERR_OK;
}

vector_error_t vector_erase(vector_t* vector, size_t index, void* elem_out) {
    HARD_ASSERT(vector != NULL, "vector is NULL");

    if (index >= vector->size) return VEC_ERR_NOT_FOUND;

    if (elem_out != NULL) 
        memcpy(elem_out, vector_ptr(vector, index), vector->elem_size);
    
    if (index + 1 != vector->size) {
        void*  dst   = vector_ptr(vector, index);
        void*  src   = vector_ptr(vector, index + 1);
        size_t bytes = (vector->size - index - 1) * vector->elem_size;
        memmove(dst, src, bytes);
    }

    vector->size--;

    vector_error_t err = normalize_for_shrink(vector);
    VECTOR_RETURN_IF_ERROR(err);

    return VEC_ERR_OK;
}



size_t vector_lower_bound(const vector_t* vector, 
                          size_t left_border, size_t right_border,
                          const void* value, compare_func_t compare_func) {
    HARD_ASSERT(vector       != NULL, "vector is NULL");
    HARD_ASSERT(value        != NULL, "value is NULL");
    HARD_ASSERT(compare_func != NULL, "compare_func is NULL");

    while (left_border < right_border) {
        size_t middle = left_border + (right_border - left_border) / 2;

        if (compare_func(vector_get(vector, middle), (void*)value) < 0) {
            left_border = middle + 1;
        } else {
            right_border = middle;
        }
    }

    return left_border;
}

size_t vector_upper_bound(const vector_t* vector, 
                          size_t left_border, size_t right_border, 
                          const void* value, compare_func_t compare_func) {
    HARD_ASSERT(vector       != NULL, "vector is NULL");
    HARD_ASSERT(value        != NULL, "value is NULL");
    HARD_ASSERT(compare_func != NULL, "compare_func is NULL");

    while (left_border < right_border) {
        size_t middle = left_border + (right_border - left_border) / 2;

        if (compare_func(vector_get(vector, middle), (void*)value) <= 0) {
            left_border = middle + 1;
        } else {
            right_border = middle;
        }
    }

    return left_border;
}

//================================================================================
//                           Р РЋР С•РЎР‚РЎвЂљР С‘РЎР‚Р С•Р Р†Р С”Р С‘
//================================================================================

vector_error_t vector_swap(vector_t* vector, size_t idx1, size_t idx2) {
    HARD_ASSERT(vector != NULL, "Vector is nullptr");
    if (idx1 >= vector->size || idx2 >= vector->size) return VEC_ERR_NOT_FOUND; 
    if (idx1 == idx2) return VEC_ERR_OK;

    unsigned char* elem_ptr1 = (unsigned char*)vector_get(vector, idx1);
    unsigned char* elem_ptr2 = (unsigned char*)vector_get(vector, idx2);

    size_t i = vector->elem_size;
#define DOSHIT(type)                  \
    while (i >= sizeof(type)) {       \
        type tmp          = *(type*)elem_ptr1;      \
        *(type*)elem_ptr1 = *(type*)elem_ptr2;  \
        *(type*)elem_ptr2 = tmp;           \
        elem_ptr1 += sizeof(type);         \
        elem_ptr2 += sizeof(type);         \
        i          = i - sizeof(type);         \
    }

    DOSHIT(uint64_t)
    DOSHIT(uint32_t)
    DOSHIT(uint16_t)
    DOSHIT(uint8_t)
#undef DOSHIT
    return VEC_ERR_OK;
}

//Р В Р В°Р В±Р С•РЎвЂљР В°Р ВµРЎвЂљ Р Р…Р В° Р С•РЎвЂљРЎР‚Р ВµР В·Р С”Р В°РЎвЂ¦ ,Р Р…Р Вµ Р Р…Р В° Р С—Р С•Р В»РЎС“Р С‘Р Р…РЎвЂљР ВµРЎР‚Р Р†Р В°Р В»Р В°РЎвЂ¦
static vector_error_t vector_qsort_impl(vector_t* vector, compare_func_t compare_func, //TODO - Р РЋР Т‘Р ВµР В»Р В°РЎвЂљРЎРЉ Р Р…Р С•РЎР‚Р С Р С•Р В±РЎР‚Р В°Р В±Р С•РЎвЂљР С”РЎС“ Р С•РЎв‚¬Р С‘Р В±Р С•Р С”
                                 size_t left_border, size_t right_border) {
    HARD_ASSERT(vector       != NULL, "Vector is nullptr");
    HARD_ASSERT(compare_func != NULL, "Compare_func is nullptr");

    if(left_border >= right_border) return VEC_ERR_OK;

    vector_error_t error = VEC_ERR_OK;
    void* pivot = calloc(1, vector->elem_size);
    if (pivot == NULL) return VEC_ERR_MEM_ALLOC;


    size_t pivot_index = left_border + (right_border - left_border) / 2;
    memcpy(pivot, vector_get(vector, pivot_index), vector->elem_size);

    size_t left_iter  = left_border;
    size_t right_iter = right_border;

     while (left_iter <= right_iter) {
        while (left_iter <= right_border &&
               compare_func(vector_get(vector, left_iter), pivot) < 0) {
            left_iter++;
        }

        while (right_iter > left_border &&
               compare_func(vector_get(vector, right_iter), pivot) > 0) {
            right_iter--;
        }

        if (left_iter <= right_iter) {
            error = vector_swap(vector, left_iter, right_iter);
            if (error != VEC_ERR_OK) {
                free(pivot);
                return error;
            }

            left_iter++;

            if (right_iter == 0) break;
            right_iter--;
        }
    }

    free(pivot);

    if (left_border < right_iter) {
        error = vector_qsort_impl(vector, compare_func, left_border, right_iter);
        if (error != VEC_ERR_OK) return error;
    }

    if (left_iter < right_border) {
        error = vector_qsort_impl(vector, compare_func, left_iter, right_border);
        if (error != VEC_ERR_OK) return error;
    }
    return VEC_ERR_OK;
}

vector_error_t vector_qsort(vector_t* vector, compare_func_t compare_func) {
    HARD_ASSERT(vector       != NULL, "Vector is nullptr");
    HARD_ASSERT(compare_func != NULL, "Compare_func is nullptr");

    if (vector->size <= 1) return VEC_ERR_OK;

    return vector_qsort_impl(vector, compare_func, 0, vector->size - 1);
}


static vector_error_t vector_merge_ranges(vector_t* vector, compare_func_t compare_func,
                                          unsigned char* buffer,
                                          size_t left_border, size_t middle_border, size_t right_border) {
    HARD_ASSERT(vector       != NULL, "vector is NULL");
    HARD_ASSERT(compare_func != NULL, "compare_func is NULL");
    HARD_ASSERT(buffer       != NULL, "buffer is NULL");

    size_t left_iter  = left_border;
    size_t right_iter = middle_border;
    size_t write_iter = left_border;

    while (left_iter < middle_border && right_iter < right_border) {
        void* left_elem_ptr  = vector_ptr(vector, left_iter);
        void* right_elem_ptr = vector_ptr(vector, right_iter);

        if (compare_func(left_elem_ptr, right_elem_ptr) <= 0) {
            memcpy(buffer + write_iter * vector->elem_size, left_elem_ptr, vector->elem_size);
            left_iter++;
        } else {
            memcpy(buffer + write_iter * vector->elem_size, right_elem_ptr, vector->elem_size);
            right_iter++;
        }

        write_iter++;
    }

    while (left_iter < middle_border) {
        memcpy(buffer + write_iter * vector->elem_size,
               vector_ptr(vector, left_iter),
               vector->elem_size);
        left_iter++;
        write_iter++;
    }

    while (right_iter < right_border) {
        memcpy(buffer + write_iter * vector->elem_size,
               vector_ptr(vector, right_iter),
               vector->elem_size);
        right_iter++;
        write_iter++;
    }

    memcpy(vector_ptr(vector, left_border),
           buffer + left_border * vector->elem_size,
           (right_border - left_border) * vector->elem_size);

    return VEC_ERR_OK;
}

static vector_error_t vector_msort_impl(vector_t* vector, compare_func_t compare_func,
                                        unsigned char* buffer,
                                        size_t left_border, size_t right_border) {
    HARD_ASSERT(vector       != NULL, "vector is NULL");
    HARD_ASSERT(compare_func != NULL, "compare_func is NULL");
    HARD_ASSERT(buffer       != NULL, "buffer is NULL");

    if (right_border - left_border <= 1) return VEC_ERR_OK;

    size_t middle_border = left_border + (right_border - left_border) / 2;

    vector_error_t error = vector_msort_impl(vector, compare_func, buffer,
                                             left_border, middle_border);
    VECTOR_RETURN_IF_ERROR(error);

    error = vector_msort_impl(vector, compare_func, buffer,
                              middle_border, right_border);
    VECTOR_RETURN_IF_ERROR(error);

    error = vector_merge_ranges(vector, compare_func, buffer,
                                left_border, middle_border, right_border);
    VECTOR_RETURN_IF_ERROR(error);

    return VEC_ERR_OK;
}

vector_error_t vector_msort(vector_t* vector, compare_func_t compare_func) {
    HARD_ASSERT(vector       != NULL, "vector is NULL");
    HARD_ASSERT(compare_func != NULL, "compare_func is NULL");

    if (vector->size <= 1) return VEC_ERR_OK;

    unsigned char* buffer = (unsigned char*)calloc(vector->size, vector->elem_size);
    if (buffer == NULL) return VEC_ERR_MEM_ALLOC;

    vector_error_t error = vector_msort_impl(vector, compare_func, buffer, 0, vector->size);

    free(buffer);
    return error;
}

static uint64_t vector_lsd_make_key_signed(const void* elem_ptr, size_t elem_size) {
    HARD_ASSERT(elem_ptr != NULL, "elem_ptr is NULL");

    switch (elem_size) {
        case 1: {
            uint8_t value = 0;
            memcpy(&value, elem_ptr, sizeof(value));
            return (uint64_t)(value ^ 0x80u);
        }

        case 2: {
            uint16_t value = 0;
            memcpy(&value, elem_ptr, sizeof(value));
            return (uint64_t)(value ^ 0x8000u);
        }

        case 4: {
            uint32_t value = 0;
            memcpy(&value, elem_ptr, sizeof(value));
            return (uint64_t)(value ^ 0x80000000u);
        }

        case 8: {
            uint64_t value = 0;
            memcpy(&value, elem_ptr, sizeof(value));
            return value ^ 0x8000000000000000ull;
        }

        default:
            return 0;
    }
}

vector_error_t vector_lsd_sort(vector_t* vector) {
    HARD_ASSERT(vector != NULL, "vector is NULL");

    if (vector->size <= 1) return VEC_ERR_OK;

    if (!(vector->elem_size == 1 ||
          vector->elem_size == 2 ||
          vector->elem_size == 4 ||
          vector->elem_size == 8)) {
        return VEC_ERR_BAD_ARG;
    }

    vector_t buffer = {0};
    vector_error_t error = vector_init(&buffer, vector->size, vector->elem_size);
    VECTOR_RETURN_IF_ERROR(error);

    unsigned char* from_data = (unsigned char*)vector->data;
    unsigned char* to_data   = (unsigned char*)buffer.data;

    for (size_t byte_index = 0; byte_index < vector->elem_size; byte_index++) {
        size_t count_arr[256]    = {0};
        size_t position_arr[256] = {0};

        for (size_t i = 0; i < vector->size; i++) {
            const void*  elem_ptr     = from_data + i * vector->elem_size;
            uint64_t     key_value    = vector_lsd_make_key_signed(elem_ptr, vector->elem_size);
            unsigned int current_byte = (unsigned int)((key_value >> (byte_index * 8)) & 0xFFu);
            count_arr[current_byte]++;
        }

        for (size_t i = 1; i < 256; i++) {
            position_arr[i] = position_arr[i - 1] + count_arr[i - 1];
        }

        for (size_t i = 0; i < vector->size; i++) {
            const void*  elem_ptr     = from_data + i * vector->elem_size;
            uint64_t     key_value    = vector_lsd_make_key_signed(elem_ptr, vector->elem_size);
            unsigned int current_byte = (unsigned int)((key_value >> (byte_index * 8)) & 0xFFu);

            memcpy(to_data + position_arr[current_byte] * vector->elem_size,
                   elem_ptr,
                   vector->elem_size);

            position_arr[current_byte]++;
        }

        unsigned char* temp_ptr = from_data;
        from_data = to_data;
        to_data   = temp_ptr;
    }

    if (from_data != (unsigned char*)vector->data) {
        memcpy(vector->data, from_data, vector->size * vector->elem_size);
    }

    vector_destroy(&buffer);
    return VEC_ERR_OK;
}

// ----------------------------------------------------------------------------
//                               Р вЂ™Р В»Р В°Р Т‘, Р С‘Р Т‘Р С‘ Р Р…Р В°РЎвЂћР С‘Р С– (Р В·Р В°РЎвЂЎР ВµР С РЎвЂљР В°Р С”Р С•Р Вµ Р В·Р В°Р Т‘Р В°Р Р†Р В°РЎвЂљРЎРЉ (Р С›Р Р…Р С• Р Р†Р С•Р С•Р В±РЎвЂ°Р Вµ Р С–Р Т‘Р Вµ-РЎвЂљР С• Р С‘РЎРѓР С—Р С•Р В»РЎРЉР В·РЎС“Р ВµРЎвЂљРЎРѓРЎРЏ? Р Р‡ Р В¶Р Вµ Р Р…Р Вµ Р В·РЎР‚РЎРЏ Р ВµР С–Р С• Р Р…Р С•РЎР‚Р СР В°Р В»РЎРЉР Р…Р С• Р С—Р С‘РЎРѓР В°Р В»??))
// ----------------------------------------------------------------------------

static vector_error_t vector_insertion_sort_range_fq(vector_t* vector, compare_func_t compare_func,
                                                     size_t left_border, size_t right_border) {
    HARD_ASSERT(vector       != NULL, "vector is NULL");
    HARD_ASSERT(compare_func != NULL, "compare_func is NULL");

    for (size_t i = left_border + 1; i < right_border; i++) {
        size_t current_index = i;

        while (current_index > left_border &&
               compare_func(vector_ptr(vector, current_index - 1),
                            vector_ptr(vector, current_index)) > 0) {
            vector_error_t error = vector_swap(vector, current_index - 1, current_index);
            VECTOR_RETURN_IF_ERROR(error);
            current_index--;
        }
    }

    return VEC_ERR_OK;
}

static vector_error_t vector_partition_three_way_fq(vector_t* vector, compare_func_t compare_func,
                                                    void* pivot_value,
                                                    size_t left_border, size_t right_border,
                                                    size_t* less_end, size_t* greater_begin) {
    HARD_ASSERT(vector        != NULL, "vector is NULL");
    HARD_ASSERT(compare_func  != NULL, "compare_func is NULL");
    HARD_ASSERT(pivot_value   != NULL, "pivot_value is NULL");
    HARD_ASSERT(less_end      != NULL, "less_end is NULL");
    HARD_ASSERT(greater_begin != NULL, "greater_begin is NULL");

    size_t less_iter    = left_border;
    size_t scan_iter    = left_border;
    size_t greater_iter = right_border;

    while (scan_iter < greater_iter) {
        int compare_result = compare_func(vector_ptr(vector, scan_iter), pivot_value);

        if (compare_result < 0) {
            vector_error_t error = vector_swap(vector, less_iter, scan_iter);
            VECTOR_RETURN_IF_ERROR(error);

            less_iter++;
            scan_iter++;
        } else if (compare_result > 0) {
            greater_iter--;

            vector_error_t error = vector_swap(vector, scan_iter, greater_iter);
            VECTOR_RETURN_IF_ERROR(error);
        } else {
            scan_iter++;
        }
    }

    *less_end = less_iter;
    *greater_begin = greater_iter;

    return VEC_ERR_OK;
}

static vector_error_t vector_select_kth_index_impl_fq(vector_t* vector, compare_func_t compare_func,
                                                      void*  pivot_buffer,
                                                      size_t left_border, size_t right_border,
                                                      size_t kth_index,
                                                      size_t* result_index) {
    HARD_ASSERT(vector       != NULL, "vector is NULL");
    HARD_ASSERT(compare_func != NULL, "compare_func is NULL");
    HARD_ASSERT(pivot_buffer != NULL, "pivot_buffer is NULL");
    HARD_ASSERT(result_index != NULL, "result_index is NULL");

    while (true) {
        size_t range_size = right_border - left_border;

        if (range_size <= 5) {
            vector_error_t error = vector_insertion_sort_range_fq(vector, compare_func,
                                                                  left_border, right_border);
            VECTOR_RETURN_IF_ERROR(error);

            *result_index = left_border + kth_index;
            return VEC_ERR_OK;
        }

        size_t medians_count = 0;

        for (size_t group_left = left_border; group_left < right_border; group_left += 5) {
            size_t group_right = group_left + 5;
            if (group_right > right_border) group_right = right_border;

            vector_error_t error = vector_insertion_sort_range_fq(vector, compare_func,
                                                                  group_left, group_right);
            VECTOR_RETURN_IF_ERROR(error);

            size_t group_median_index = group_left + (group_right - group_left) / 2;
            error = vector_swap(vector, left_border + medians_count, group_median_index);
            VECTOR_RETURN_IF_ERROR(error);

            medians_count++;
        }

        size_t median_of_medians_index = 0;
        vector_error_t error = vector_select_kth_index_impl_fq(vector, compare_func, pivot_buffer,
                                                               left_border, left_border + medians_count,
                                                               medians_count / 2,
                                                               &median_of_medians_index);
        VECTOR_RETURN_IF_ERROR(error);

        memcpy(pivot_buffer, vector_ptr(vector, median_of_medians_index), vector->elem_size);

        size_t less_end = 0;
        size_t greater_begin = 0;

        error = vector_partition_three_way_fq(vector, compare_func, pivot_buffer,
                                              left_border, right_border,
                                              &less_end, &greater_begin);
        VECTOR_RETURN_IF_ERROR(error);

        size_t left_size = less_end - left_border;
        size_t middle_size = greater_begin - less_end;

        if (kth_index < left_size) {
            right_border = less_end;
        } else if (kth_index < left_size + middle_size) {
            *result_index = less_end;
            return VEC_ERR_OK;
        } else {
            kth_index -= left_size + middle_size;
            left_border = greater_begin;
        }
    }
}

static vector_error_t vector_qsort_impl_fq(vector_t* vector, compare_func_t compare_func,
                                        void* pivot_buffer,
                                        size_t left_border, size_t right_border) {
    HARD_ASSERT(vector       != NULL, "vector is NULL");
    HARD_ASSERT(compare_func != NULL, "compare_func is NULL");
    HARD_ASSERT(pivot_buffer != NULL, "pivot_buffer is NULL");

    while (right_border - left_border > 1) {
        if (right_border - left_border <= 16) {
            return vector_insertion_sort_range_fq(vector, compare_func, left_border, right_border);
        }

        size_t pivot_index  = 0;
        size_t middle_order = (right_border - left_border) / 2;

        vector_error_t error = vector_select_kth_index_impl_fq(vector, compare_func, pivot_buffer,
                                                               left_border, right_border,
                                                               middle_order,
                                                               &pivot_index);
        VECTOR_RETURN_IF_ERROR(error);

        memcpy(pivot_buffer, vector_ptr(vector, pivot_index), vector->elem_size);

        size_t less_end      = 0;
        size_t greater_begin = 0;

        error = vector_partition_three_way_fq(vector, compare_func, pivot_buffer,
                                              left_border, right_border,
                                              &less_end, &greater_begin);
        VECTOR_RETURN_IF_ERROR(error);

        size_t left_size  = less_end - left_border;
        size_t right_size = right_border - greater_begin;

        if (left_size < right_size) {
            error = vector_qsort_impl_fq(vector, compare_func, pivot_buffer,
                                         left_border, less_end);
            VECTOR_RETURN_IF_ERROR(error);

            left_border = greater_begin;
        } else {
            error = vector_qsort_impl_fq(vector, compare_func, pivot_buffer,
                                         greater_begin, right_border);
            VECTOR_RETURN_IF_ERROR(error);

            right_border = less_end;
        }
    }

    return VEC_ERR_OK;
}

vector_error_t vector_qsort_fq(vector_t* vector, compare_func_t compare_func) {
    HARD_ASSERT(vector != NULL, "vector is NULL");
    HARD_ASSERT(compare_func != NULL, "compare_func is NULL");

    if (vector->size <= 1) return VEC_ERR_OK;

    void* pivot_buffer = calloc(1, vector->elem_size);
    if (pivot_buffer == NULL) return VEC_ERR_MEM_ALLOC;

    vector_error_t error = vector_qsort_impl_fq(vector, compare_func, pivot_buffer,
                                             0, vector->size);

    free(pivot_buffer);
    return error;
}
