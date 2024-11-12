#include <stdlib.h>
#include <string.h>

#include "custom_assert.h"
#include "colors.h"
#include "linked_list.h"

/*=======================================================================================*/

#ifndef LINKED_LIST_OFF_DUMP
    static const char   *general_log_file_name     = "log/list.html";
    static const char   *log_folder                = "log";
    static const char   *dump_dot_file_folder      = "dot";
    static const char   *dump_png_file_folder      = "img";
    static const size_t  max_file_name_length      = 64;
    static const size_t  max_system_command_length = 256;
    static const char   *zero_color                = "#bfecff";
    static const char   *head_color                = "#cdc1ff";
    static const char   *tail_color                = "#ffccea";
    static const char   *elem_color                = "#fff6e3";
    static const char   *free_color                = "#c1cfa1";

    /*=======================================================================================*/

    static const char  *linked_list_element_color          (linked_list_t *list,
                                                            size_t         node);
    static list_error_t linked_list_create_png_dump        (linked_list_t *list,
                                                            const char    *png_filename,
                                                            const char    *dot_filename,
                                                            const char    *caller_file,
                                                            size_t         caller_line,
                                                            const char    *caller_function,
                                                            const char    *list_variable_name);
    static list_error_t linked_list_create_dot_dump        (linked_list_t *list,
                                                            const char    *filename);
    static list_error_t linked_list_write_node_connections (linked_list_t *list,
                                                            FILE          *dot_file);
    static list_error_t linked_list_initialize_dump        (linked_list_t *list);

    static list_error_t linked_list_write_nodes            (linked_list_t *list,
                                                            FILE          *dot_file);
#endif

/*=======================================================================================*/

#ifndef LINKED_LIST_OFF_VERIFICATION
    #define LINKED_LIST_VERIFY(__list)  {                         \
        list_error_t __error_code = linked_list_verify((__list)); \
        if(__error_code != LINKED_LIST_SUCCESS) {                 \
            return __error_code;                                  \
        }                                                         \
    }

    /*=======================================================================================*/

    static list_error_t linked_list_verify(linked_list_t *list);
#else
    #define LINKED_LIST_VERIFY(...)
#endif

/*=======================================================================================*/

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

/*=======================================================================================*/

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


    if((error_code = linked_list_initialize_dump(list)) != LINKED_LIST_SUCCESS) {
        return error_code;
    }

    LINKED_LIST_VERIFY(list);
    return LINKED_LIST_SUCCESS;
}

/*=======================================================================================*/

list_error_t linked_list_insert_after(linked_list_t *list, size_t real_index, data_t data) {
    LINKED_LIST_VERIFY(list);
    C_ASSERT(real_index < list->capacity + 1, return LINKED_LIST_INVALID_INDEX);

    return linked_list_insert_between(list, real_index, list->array[real_index].next, data);
}

/*=======================================================================================*/

list_error_t linked_list_insert_before(linked_list_t *list, size_t real_index, data_t data) {
    LINKED_LIST_VERIFY(list);
    C_ASSERT(real_index < list->capacity + 1, return LINKED_LIST_INVALID_INDEX);

    return linked_list_insert_between(list, list->array[real_index].prev, real_index, data);
}

/*=======================================================================================*/

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

/*=======================================================================================*/

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

/*=======================================================================================*/

#ifndef LINKED_LIST_OFF_DUMP
    list_error_t linked_list_dump(linked_list_t *list,
                                  const char    *caller_file,
                                  size_t         caller_line,
                                  const char    *caller_function,
                                  const char    *list_variable_name) {
        C_ASSERT(list               != NULL, return LINKED_LIST_NULL_PARAMETER);
        C_ASSERT(caller_file        != NULL, return LINKED_LIST_NULL_PARAMETER);
        C_ASSERT(caller_line        != NULL, return LINKED_LIST_NULL_PARAMETER);
        C_ASSERT(list_variable_name != NULL, return LINKED_LIST_NULL_PARAMETER);

        char dot_filename[max_file_name_length] = {};
        char img_filename[max_file_name_length] = {};

        sprintf(dot_filename,
                "%s/%s/dump%llu.dot",
                log_folder,
                dump_dot_file_folder,
                list->dumps_number);
        sprintf(img_filename,
                "%s/dump%llu.svg",
                dump_png_file_folder,
                list->dumps_number);

        list_error_t error_code = LINKED_LIST_SUCCESS;
        if((error_code = linked_list_create_dot_dump(list, dot_filename)) != LINKED_LIST_SUCCESS) {
            return error_code;
        }

        if((error_code = linked_list_create_png_dump(list,
                                                     img_filename,
                                                     dot_filename,
                                                     caller_file,
                                                     caller_line,
                                                     caller_function,
                                                     list_variable_name)) != LINKED_LIST_SUCCESS) {
            return error_code;
        }
        list->dumps_number++;
        return LINKED_LIST_SUCCESS;
    }

    /*=======================================================================================*/

    list_error_t linked_list_create_png_dump(linked_list_t *list,
                                             const char    *img_filename,
                                             const char    *dot_filename,
                                             const char    *caller_file,
                                             size_t         caller_line,
                                             const char    *caller_function,
                                             const char    *list_variable_name) {
        char dot_system_command[max_system_command_length] = {};
        sprintf(dot_system_command,
                "dot %s -Tsvg -o %s/%s",
                dot_filename,
                log_folder,
                img_filename);
        system(dot_system_command);

        fprintf(list->general_dump_file,
                "<h1>==========================================================</h1>\r\n"
                "<h1>Linked list dump %llu</h1>\r\n"
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
                img_filename);
        fflush(list->general_dump_file);
        return LINKED_LIST_SUCCESS;
    }

    /*=======================================================================================*/

    list_error_t linked_list_create_dot_dump(linked_list_t *list,
                                             const char    *filename) {
        FILE *dot_file = fopen(filename, "wb");
        if(dot_file == NULL) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Error while opening dump file.\r\n");
            return LINKED_LIST_DUMP_ERROR;
        }

        fputs("digraph {\r\n"
              "node[shape = Mrecord, style = filled];\r\n"
              "splines=ortho;\r\n"
              "rankdir = LR;\r\n", dot_file);

        linked_list_write_nodes(list, dot_file);

        linked_list_write_node_connections(list, dot_file);

        fprintf(dot_file, "FREE[fillcolor = \"#d8e2dc\"]");
        if(list->free < list->capacity + 1) {
            fprintf(dot_file, "FREE -> node%llx[color = \"#9d8189\", constraint = false];\r\n", list->free);
        }

        for(size_t free = list->free; free < list->capacity; free = list->array[free].next) {
            fprintf(dot_file, "node%llx -> node%llx[color = \"#9d8189\", constraint = false];\r\n",
            free, list->array[free].next);
        }

        fputs("}\n", dot_file);
        fclose(dot_file);
        return LINKED_LIST_SUCCESS;
    }

    /*=======================================================================================*/

    list_error_t linked_list_write_nodes(linked_list_t *list, FILE *dot_file) {
        for(size_t node = 0; node < list->capacity + 1; node++) {
            const char *color = linked_list_element_color(list, node);
            char prev[16] = {};
            if(list->array[node].prev == poison_index) {
                sprintf(prev, "\\n(POISON)");
            }
            else {
                sprintf(prev, "%llx", list->array[node].prev);
            }

            fprintf(dot_file,
                    "node%llx[label = \"%llu | {prev = %s} | {next = %llx} | {data = %d  }\", "
                    "fillcolor = \"%s\", "
                    "width = 1.5, height = 2];\n",
                    node,
                    node,
                    prev,
                    list->array[node].next,
                    list->array[node].data,
                    color);
        }
        return LINKED_LIST_SUCCESS;
    }

    /*=======================================================================================*/

    list_error_t linked_list_write_node_connections(linked_list_t *list,
                                                    FILE          *dot_file) {
        for(size_t edge = 0; edge < list->capacity ; edge++) {
            fprintf(dot_file, "node%llx -> node%llx[style = invis];\r\n",
            edge, edge + 1);
        }


        for(size_t node = 0, counter = 0; node != 0 || counter == 0; counter++, node = list->array[node].next) {
            if(list->array[list->array[node].next].prev == node) {
                fprintf(dot_file, "node%llx -> node%llx[color = \"#06d6a0\", constraint = false];\r\n",
                        node, list->array[node].next);
            }
            else {
                fprintf(dot_file, "node%llx -> node%llx[color = \"#ff006e\", constraint = false];\r\n",
                        node, list->array[node].next);
                fprintf(dot_file, "node%llx -> node%llx[color = \"#ff006e\", constraint = false];\r\n",
                        list->array[list->array[node].next].prev, list->array[node].next);
            }
        }

        return LINKED_LIST_SUCCESS;
    }

    const char *linked_list_element_color(linked_list_t *list, size_t node) {
        if(node == 0) {
            return zero_color;
        }
        else if(node == linked_list_get_head(list)) {
            return head_color;
        }
        else if(node == linked_list_get_tail(list)) {
            return tail_color;
        }
        else if(list->array[node].prev != poison_index) {
            return elem_color;
        }
        return free_color;
    }

    /*=======================================================================================*/

    list_error_t linked_list_initialize_dump(linked_list_t *list) {

        char command[max_system_command_length] = {};
        sprintf(command, "md %s", log_folder);
        system(command);

        sprintf(command, "md %s\\%s", log_folder, dump_dot_file_folder);
        system(command);

        sprintf(command, "md %s\\%s", log_folder, dump_png_file_folder);
        system(command);

        list->general_dump_file = fopen(general_log_file_name, "wb");
        if(list->general_dump_file == NULL) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Error while opening log file.\r\n");
            return LINKED_LIST_DUMP_ERROR;
        }

        fputs("<pre>\r\n", list->general_dump_file);

        fprintf(list->general_dump_file,
                "<pre>\r\n"
                "<h3 style = \"background: %s;\">zero element color</h3>"
                "<h3 style = \"background: %s;\">head element color</h3>"
                "<h3 style = \"background: %s;\">tail element color</h3>"
                "<h3 style = \"background: %s;\">node element color</h3>"
                "<h3 style = \"background: %s;\">free element color</h3>\n",
                zero_color, head_color, tail_color, elem_color, free_color);

        return LINKED_LIST_SUCCESS;
    }
#endif

/*=======================================================================================*/

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

/*=======================================================================================*/

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

/*=======================================================================================*/

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

/*=======================================================================================*/

size_t linked_list_get_head(linked_list_t *list) {
    return list->array->next;
}

/*=======================================================================================*/

size_t linked_list_get_tail(linked_list_t *list) {
    return list->array->prev;
}

/*=======================================================================================*/

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

/*=======================================================================================*/

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
