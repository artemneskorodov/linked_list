#include <stdio.h>

#include "linked_list.h"

int main(void) {
    linked_list_t list = {};

    printf("ctor            | %d\n", linked_list_ctor(&list, 4));
    printf("insert after 0  | %d\n", linked_list_insert_after(&list, 0, 10)); //TODO zero can be used only one time
    printf("insert after 1  | %d\n", linked_list_insert_after(&list, 1, 20));
    printf("insert after 1  | %d\n", linked_list_insert_after(&list, 1, 30));
    LINKED_LIST_DUMP(list);

    data_t data = 0;
    printf("remove 3        | %d\n", linked_list_remove(&list, 3, &data));
    LINKED_LIST_DUMP(list);
    printf("insert after 2  | %d\n", linked_list_insert_after(&list, 2, 40));
    LINKED_LIST_DUMP(list);
    printf("insert after 2  | %d\n", linked_list_insert_after(&list, 2, 50));
    LINKED_LIST_DUMP(list);
    printf("insert before 1 | %d\n", linked_list_insert_before(&list, 1, 222));

    LINKED_LIST_DUMP(list);

    printf("dtor            | %d\n", linked_list_dtor(&list));

}
