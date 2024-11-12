#include <stdio.h>

#include "linked_list.h"

int main(void) {
    linked_list_t list = {};

    printf("ctor            | %d\n", linked_list_ctor(&list, 4));
    LINKED_LIST_DUMP(list, "1");
    printf("insert after 0  | %d\n", linked_list_insert_after(&list, 0, 10));
    LINKED_LIST_DUMP(list, "2");
    printf("insert after 1  | %d\n", linked_list_insert_after(&list, 1, 20));
    printf("insert after 1  | %d\n", linked_list_insert_after(&list, 1, 30));
    LINKED_LIST_DUMP(list, "3");

    data_t data = 0;
    printf("remove 3        | %d\n", linked_list_remove(&list, 3, &data));
    LINKED_LIST_DUMP(list, "3");
    printf("insert after 2  | %d\n", linked_list_insert_after(&list, 2, 40));
    LINKED_LIST_DUMP(list, "4");
    printf("insert after 2  | %d\n", linked_list_insert_after(&list, 2, 50));
    LINKED_LIST_DUMP(list, "aaaaa");

    printf("insert before 1 | %d\n", linked_list_insert_before(&list, 1, 222));

    LINKED_LIST_DUMP(list, "5");

    list.array[4].prev = 5;

    LINKED_LIST_DUMP(list, "6");

    printf("dtor            | %d\n", linked_list_dtor(&list));

}
