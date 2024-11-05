#include <stdlib.h>
#include <string.h>

#include "custom_assert.h"
#include "colors.h"
#include "linked_list.h"

#ifndef LINKED_LIST_OFF_DUMP
    static const char   *general_log_file_name     = "log/list.html";
    static const char   *log_folder                = "log/";
    static const char   *dump_dot_file_folder      = "dot/";
    static const char   *dump_png_file_folder      = "img/";
    static const size_t  max_file_name_length      = 64;
    static const size_t  max_system_command_length = 256;
#endif

#ifndef LINKED_LIST_OFF_VERIFICATION
    #define LINKED_LIST_VERIFY(__list)  {                         \
        list_error_t __error_code = linked_list_verify((__list)); \
        if(__error_code != LINKED_LIST_SUCCESS) {                 \
            return __error_code;                                  \
        }                                                         \
    }

    static list_error_t linked_list_verify(linked_list_t *list);
#else
    #define LINKED_LIST_VERIFY(...)
#endif

static list_error_t linked_list_find_free      (linked_list_t *list,
                                                size_t        *output);
static list_error_t linked_list_insert_between (linked_list_t *list,
                                                size_t         prev_node,
                                                size_t         next_node,
                                                data_t         data);
static size_t       linked_list_get_head       (linked_list_t *list);
static size_t       linked_list_get_tail       (linked_list_t *list);
static list_error_t list_reallocate_memory     (linked_list_t *list);
static list_error_t linked_list_set_default    (linked_list_t *list,
                                                size_t         start,
                                                size_t         end);

list_error_t linked_list_ctor(linked_list_t *list, size_t capacity) {
    C_ASSERT(list != NULL, return LINKED_LIST_NULL);

    list_error_t error_code = LINKED_LIST_SUCCESS;

    list->array = (list_node_t *)calloc(capacity + 1, sizeof(list_node_t));
    if(list->array == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating data memory.\r\n");
        return LINKED_LIST_MEMORY_ERROR;
    }

    list->array->next = 0;
    list->array->prev = 0;
    list->array->data = 211;
    list->free        = 1;
    list->capacity    = capacity;
    list->size        = 0;

    if((error_code = linked_list_set_default(list,
                                             1,
                                             capacity + 1)) != LINKED_LIST_SUCCESS) {
        return error_code;
    }

    list->general_dump_file = fopen(general_log_file_name, "wb");
    if(list->general_dump_file == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening log file.\r\n");
        return LINKED_LIST_DUMP_ERROR;
    }

    fputs("<pre>\r\n", list->general_dump_file);

    LINKED_LIST_VERIFY(list);
    return LINKED_LIST_SUCCESS;
}

list_error_t linked_list_insert_after(linked_list_t *list, size_t real_index, data_t data) {
    LINKED_LIST_VERIFY(list);
    C_ASSERT(real_index  <  list->capacity + 1, return LINKED_LIST_INVALID_INDEX);

    return linked_list_insert_between(list, real_index, list->array[real_index].next, data);
}

list_error_t linked_list_insert_before(linked_list_t *list, size_t real_index, data_t data) {
    LINKED_LIST_VERIFY(list);
    C_ASSERT(real_index  <  list->capacity + 1, return LINKED_LIST_INVALID_INDEX);

    return linked_list_insert_between(list, list->array[real_index].prev, real_index, data);
}

list_error_t linked_list_remove(linked_list_t *list, size_t real_index, data_t *output) {
    LINKED_LIST_VERIFY(list);
    C_ASSERT(real_index  != 0                 , return LINKED_LIST_INVALID_INDEX );
    C_ASSERT(real_index  <  list->capacity + 1, return LINKED_LIST_INVALID_INDEX );
    C_ASSERT(output      != NULL              , return LINKED_LIST_NULL_PARAMETER);

    *output                    = list->array[real_index].data;

    list_node_t *removing_node = list->array + real_index;
    list_node_t *prev_node     = list->array + removing_node->prev;
    list_node_t *next_node     = list->array + removing_node->next;

    prev_node->next            = removing_node->next;
    next_node->prev            = removing_node->prev;

    removing_node->next        = list->free;
    removing_node->prev        = poison_index;
    removing_node->data        = poison_data;

    list->free                 = real_index;
    list->size--;

    LINKED_LIST_VERIFY(list);
    return LINKED_LIST_SUCCESS;
}

list_error_t linked_list_dtor(linked_list_t *list) {
    C_ASSERT(list != NULL, return LINKED_LIST_NULL);

    fclose(list->general_dump_file);
    free(list->array);

    if(memset(list, 0, sizeof(linked_list_t)) != list) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while setting list structure memory to zeros.\r\n");
        return LINKED_LIST_MEMORY_ERROR;
    }

    return LINKED_LIST_SUCCESS;
}

#ifndef LINKED_LIST_OFF_DUMP
    list_error_t linked_list_dump(linked_list_t *list,
                                  const char    *caller_file,
                                  size_t         caller_line,
                                  const char    *caller_function,
                                  const char    *list_variable_name) {
        C_ASSERT(caller_file        != NULL, return LINKED_LIST_NULL_PARAMETER);
        C_ASSERT(caller_line        != NULL, return LINKED_LIST_NULL_PARAMETER);
        C_ASSERT(list_variable_name != NULL, return LINKED_LIST_NULL_PARAMETER);

        char dot_filename[max_file_name_length] = {};
        char png_filename[max_file_name_length] = {};

        sprintf(dot_filename,
                "%s%sdump%llu.dot",
                log_folder,
                dump_dot_file_folder,
                list->dumps_number);
        sprintf(png_filename,
                "%sdump%llu.png",
                dump_png_file_folder,
                list->dumps_number);

        FILE *dot_file = fopen(dot_filename, "wb");
        if(dot_file == NULL) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Error while opening dump file.\r\n");
            return LINKED_LIST_DUMP_ERROR;
        }

        fputs("digraph {\r\n"
              "node[shape = Mrecord, style = filled, fillcolor = \"#C1CFA1\"];\r\n"
              "edge[arrowhead = empty, color = \"#FFBD73\", style = bold];\r\n"
              "rankdir = LR;\r\n", dot_file);

        size_t head_index = linked_list_get_head(list);
        size_t tail_index = linked_list_get_tail(list);

        for(size_t node = 0; node < list->capacity + 1; node++) {
            fprintf(dot_file, "node%llx[label = \"%llu | {prev = %llx} | {next = %llx} | {data = %d}\"",
                    node,
                    node,
                    list->array[node].prev == poison_index ? 0 : list->array[node].prev,
                    list->array[node].next,
                    list->array[node].data);

            if(node == 0) {
                fputs(", fillcolor = \"#BFECFF\"", dot_file);
            }
            else if(node == head_index) {
                fputs(", fillcolor = \"#CDC1FF\"", dot_file);
            }
            else if(node == tail_index) {
                fputs(", fillcolor = \"#FFCCEA\"", dot_file);
            }
            else if(list->array[node].prev != poison_index) {
                fputs(", fillcolor = \"#FFF6E3\"", dot_file);
            }

            fputs("];\r\n", dot_file);
        }

        for(size_t edge = 0; edge + 1 < list->capacity + 1; edge++) {
            fprintf(dot_file, "node%llx -> node%llx[style = invis, weight = 1000.0]\r\n",
            edge, edge + 1);
        }

        for(size_t node = 0; list->array[node].next != 0; node = list->array[node].next) {
            fprintf(dot_file, "node%llx -> node%llx\r\n",
                    node, list->array[node].next);
        }

        fputs("}", dot_file);
        fclose(dot_file);

        char dot_system_command[max_system_command_length] = {};
        sprintf(dot_system_command,
                "dot %s -Tpng -o %s%s",
                dot_filename,
                log_folder,
                png_filename);
        system(dot_system_command);

        fprintf(list->general_dump_file,
                "<h1>Linked list dump %llu</h1>"
                "Called from '%s' in %s:%llu\r\n"
                "%s [0x%p]\r\n"
                "\tcapacity = %llu\r\n"
                "\tfree     = %llu\r\n"
                "\tdata [0x%p]\r\n"
                "<img src = \"%s\">\r\n",
                list->dumps_number + 1,
                caller_function,
                caller_file,
                caller_line,
                list_variable_name,
                list,
                list->capacity,
                list->free,
                list->array,
                png_filename);
        fflush(list->general_dump_file);

        list->dumps_number++;
        return LINKED_LIST_SUCCESS;
    }
#endif

list_error_t linked_list_find_free(linked_list_t *list, size_t *output) {
    LINKED_LIST_VERIFY(list);
    C_ASSERT(output      != NULL, return LINKED_LIST_NULL_PARAMETER);

    list_error_t error_code = LINKED_LIST_SUCCESS;

    if(list->size == list->capacity) {
        if((error_code = list_reallocate_memory(list)) != LINKED_LIST_SUCCESS) {
            return error_code;
        }
    }

    *output = list->free;
    list->free = list->array[list->free].next;

    LINKED_LIST_VERIFY(list);
    return LINKED_LIST_SUCCESS;
}

list_error_t linked_list_insert_between(linked_list_t *list,
                                        size_t         prev_node,
                                        size_t         next_node,
                                        data_t         data) {
    LINKED_LIST_VERIFY(list);

    size_t insertion_index = 0;
    list_error_t error_code = LINKED_LIST_SUCCESS;
    if((error_code = linked_list_find_free(list, &insertion_index)) != LINKED_LIST_SUCCESS) {
        return error_code;
    }

    list_node_t *insertion_node = list->array + insertion_index;

    insertion_node->data        = data;
    insertion_node->prev        = prev_node;
    insertion_node->next        = next_node;

    list->array[prev_node].next = insertion_index;
    list->array[next_node].prev = insertion_index;

    list->size++;

    LINKED_LIST_VERIFY(list);
    return LINKED_LIST_SUCCESS;
}

list_error_t list_reallocate_memory(linked_list_t *list) {
    LINKED_LIST_VERIFY(list);
    list_error_t error_code = LINKED_LIST_SUCCESS;

    list->array = (list_node_t *)realloc(list->array, (list->capacity * 2 + 1) * sizeof(list_node_t));
    if(list->array == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reallocating list memory.\r\n");
        return LINKED_LIST_MEMORY_ERROR;
    }

    if(memset(list->array + list->capacity + 1, 0,
              list->capacity * sizeof(list_node_t)) !=
       list->array + list->capacity + 1) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while setting allocated memory to zeros.\r\n");
        return LINKED_LIST_MEMORY_ERROR;
    }

    if((error_code = linked_list_set_default(list,
                                             list->capacity + 1,
                                             list->capacity * 2 + 1)) != LINKED_LIST_SUCCESS) {
        return error_code;
    }

    list->capacity *= 2;

    LINKED_LIST_VERIFY(list);
    return LINKED_LIST_SUCCESS;
}

size_t linked_list_get_head(linked_list_t *list) {
    LINKED_LIST_VERIFY(list);

    return list->array->next;
}

size_t linked_list_get_tail(linked_list_t *list) {
    LINKED_LIST_VERIFY(list);

    return list->array->prev;
}

list_error_t linked_list_set_default(linked_list_t *list,
                                     size_t         start,
                                     size_t         end) {
    LINKED_LIST_VERIFY(list);

    for(size_t node = start; node < end; node++) {
        list->array[node].data = poison_data;
        list->array[node].next = node + 1;
        list->array[node].prev = poison_index;
    }

    LINKED_LIST_VERIFY(list);
    return LINKED_LIST_SUCCESS;
}

#ifndef LINKED_LIST_OFF_VERIFICATION
    list_error_t linked_list_verify(linked_list_t *list) {
        if(list == NULL) {
            return LINKED_LIST_NULL;
        }
        if(list->array == NULL) {
            return LINKED_LIST_NULL_DATA;
        }
        if(list->size > list->capacity) {
            return LINKED_LIST_INVALID_SIZE;
        }

        for(size_t node = 0, counter = 0; list->array[node].next != 0; node++, counter++) {
            if(list->array[node].next > list->capacity) {
                return LINKED_LIST_INVALID_NODE_NEXT;
            }
            if(list->array[node].prev > list->capacity) {
                return LINKED_LIST_INVALID_NODE_PREV;
            }
            if(list->array[list->array[node].prev].next != node ||
               list->array[list->array[node].next].prev != node) {
                return LINKED_LIST_INVALID_NODE_CONNECTION;
            }
            if(counter > list->size) {
                return LINKED_LIST_LOOP_ERROR;
            }
        }
        return LINKED_LIST_SUCCESS;
    }
#endif
