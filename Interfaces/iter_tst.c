#include <stdio.h>
#include <stdlib.h>
#define ARRAY_IMPLEMENTATION
#define ALLOCATOR_IMPLEMENTATION
#define RANDOM_ACCESS_ITERATOR_IMPLEMENTATION
#include <defer.h>

#include "Array.h"
#include "Iterator.h"
#include "ArrayInterfaces.h"
#include "Allocator.h"

struct kvp {
    char *key;
    int value;
};


bool kvp_equals(const void *a, const void *b) {
    const struct kvp *const a_ = a;
    const struct kvp *const b_ = b;
    return strcmp(a_->key, b_->key) == 0;
}

int main(void) {
    auto const array = array(struct kvp,
                             {"One", 1},
                             {"Two", 2},
                             {"Three", 3},
                             {"Four", 4},
                             {"Five", 5},
                             {"Six", 6},
                             {"Seven", 7},
                             {"Eight", 8},
                             {"Nine", 9},
                             {"Ten", 10});

    auto const index = it_find_index(array_begin(array, struct kvp), kvp_equals,
                                     (struct kvp) { .key = "Five" });

    auto const item = array_data(struct kvp, array)[index];

    printf("%s: %d\n", item.key, item.value);

    return 0;
}
