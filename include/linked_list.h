#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdio.h>
typedef int data_t;

static const size_t poison_index = (size_t)(-1);
static const data_t poison_data  = -1;

enum list_error_t {
    //TODO
    LINKED_LIST_SUCCESS         = 0,
    LINKED_LIST_NULL            = 1,
    LINKED_LIST_MEMORY_ERROR    = 2,
    LINKED_LIST_NULL_DATA       = 3,
    LINKED_LIST_INVALID_INDEX   = 4,
    LINKED_LIST_NULL_PARAMETER  = 5,
    LINKED_LIST_DUMP_ERROR      = 6,
    LINKED_LIST_INVALID_SIZE    = 7,
    LINKED_LIST_INVALID_NODE_NEXT = 8,
    LINKED_LIST_INVALID_NODE_PREV = 9,
    LINKED_LIST_LOOP_ERROR        = 10,
    LINKED_LIST_INVALID_NODE_CONNECTION = 11,
};

struct list_node_t {
    data_t data;
    size_t prev;
    size_t next;
};

struct linked_list_t {
    list_node_t *array;
    size_t       capacity;
    size_t       free;
    size_t       dumps_number;
    size_t       size;

    #ifndef LINKED_LIST_OFF_DUMP
        FILE *general_dump_file;
    #endif
};
#ifdef LINKED_LIST_OFF_DUMP
    #define LINKED_LIST_DUMP(...)
#else
    #define LINKED_LIST_DUMP(__list)         \
        linked_list_dump(&(__list),          \
                         __FILE__,           \
                         __LINE__,           \
                         __PRETTY_FUNCTION__,\
                         #__list);

    list_error_t linked_list_dump(linked_list_t *list,
                                  const char    *caller_file,
                                  size_t         caller_line,
                                  const char    *caller_function,
                                  const char    *list_variable_name);
#endif

list_error_t linked_list_ctor          (linked_list_t *list,
                                        size_t         capacity);
list_error_t linked_list_insert_after  (linked_list_t *list,
                                        size_t         real_index,
                                        data_t         data);
list_error_t linked_list_insert_before (linked_list_t *list,
                                        size_t         real_index,
                                        data_t         data);
list_error_t linked_list_remove        (linked_list_t *list,
                                        size_t         real_index,
                                        data_t        *output);
list_error_t linked_list_dtor          (linked_list_t *list);

#endif
