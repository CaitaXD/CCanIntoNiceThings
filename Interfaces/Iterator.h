#ifndef RANDOM_ACCESS_ITERATOR_H
#define RANDOM_ACCESS_ITERATOR_H

#include <stdint.h>
#include "Allocator.h"

#ifndef stack_box
#  define stack_box(X) ((typeof(X)[1]){X})
#endif

#ifndef container_of
#   define container_of(ptr__, type__, member__)((type__*) ((intptr_t)(ptr__) - offsetof(type__, member__)))
#endif

#ifndef fieldref
#   define fieldref(expr__, field__) &stack_box((expr__))->field__
#endif

#ifndef iterator
#   define iterator(val__) fieldref((val__), iterator)
#endif

#ifndef CONCAT_
#   define CONCAT_(a,b) a##b
#endif

#ifndef CONCAT
#   define CONCAT(a,b) CONCAT_(a,b)
#endif

#ifndef LINE_IDENT
#   define LINE_IDENT(name) CONCAT(name, __LINE__)
#endif

#ifndef scoped_expression
#define scoped_expression(var__, ...) \
    for (bool LINE_IDENT(__once_guard) = 1; LINE_IDENT(__once_guard); LINE_IDENT(__once_guard) = false) \
    for (var__;LINE_IDENT(__once_guard);LINE_IDENT(__once_guard) = false, ({__VA_ARGS__;}))
#endif

#ifndef IT_API
#   define IT_API static inline
#endif

struct iterator_interface {
    size_t size;
    size_t element_size;

    void * (*advance)(
        const struct iterator_interface *it,
        size_t element_size,
        size_t index
    );

    struct iterator_interface * (*clone)(
        const struct iterator_interface *it,
        const struct allocator_t *allocator
    );
};

IT_API void *it_advance(const struct iterator_interface *it, size_t element_size, size_t index);

IT_API struct iterator_interface *it_clone(const struct iterator_interface *it,
                                           const struct allocator_t *allocator);

IT_API ssize_t (it_index_of)(const struct iterator_interface *it, const void *element);

IT_API ssize_t (it_last_index_of)(const struct iterator_interface *it, const void *element);

IT_API ssize_t (it_find_index)(const struct iterator_interface *it, bool (*eq)(const void *a, const void *b),
                               const void *element);

IT_API ssize_t (it_find_last_index)(const struct iterator_interface *it, bool (*eq)(const void *a, const void *b),
                                    const void *element);

#define it_index_of(it__, element__) (it_index_of)(iterator(it__), stack_box(element__))
#define it_last_index_of(it__, element__) (it_last_index_of)(iterator(it__), stack_box(element__))
#define it_find_index(it__, eq__, element__) (it_find_index)(iterator(it__), eq__, stack_box(element__))
#define it_find_last_index(it__, eq__, element__) (it_find_last_index)(iterator(it__), eq__, stack_box(element__))

// Expects a struct with a field called iterator of type struct iterator_interface and a field called current of any type
#define foreach(var__, it__)\
scoped_expression(typeof(it__) __it = (it__)) \
scoped_expression(struct iterator_interface* __rait = iterator(__it)) \
scoped_expression(uint8_t __vla[__rait->size]) \
scoped_expression(struct arena_allocator_t __a = arena_allocator_init(__vla, sizeof (__vla))) \
scoped_expression(struct iterator_interface* __clone = it_clone(__rait, &__a.allocator)) \
scoped_expression(size_t __element_size = __rait->element_size) \
for (\
    typeof(__it.current) *var__ = it_advance(__clone, __element_size, 0);\
    var__;\
    var__ = it_advance(__clone, __element_size, 1)\
)

void *it_advance(const struct iterator_interface *it, const size_t element_size, const size_t index) {
    return it->advance(it, element_size, index);
}

struct iterator_interface *it_clone(const struct iterator_interface *it,
                                    const struct allocator_t *allocator) {
    if (it->clone != NULL) return it->clone(it, allocator);

    void *data = alloc(allocator, it->size);
    assert(data != NULL && "Allocation failed, Buy more RAM");
    memcpy(data, it, it->size);
    return data;
}

ssize_t (it_index_of)(const struct iterator_interface *it, const void *element) {
    uint8_t vla[it->size];
    const struct arena_allocator_t a = arena_allocator_init(vla, sizeof(vla));
    const struct iterator_interface *clone = it_clone(it, &a.allocator);

    ssize_t ret = -1;
    ssize_t index = -1;
    const void *current = it_advance(clone, clone->element_size, 0);
    while (current != NULL) {
        index += 1;
        if (memcmp(element, current, clone->element_size) == 0) {
            ret = index;
            break;
        }
        current = it_advance(clone, clone->element_size, 1);
    }
    return ret;
}

ssize_t (it_last_index_of)(const struct iterator_interface *it, const void *element) {
    uint8_t vla[it->size];
    const struct arena_allocator_t a = arena_allocator_init(vla, sizeof(vla));
    const struct iterator_interface *clone = it_clone(it, &a.allocator);

    ssize_t ret = -1, index = -1;
    const void *current = it_advance(clone, clone->element_size, 0);
    while (current != NULL) {
        index += 1;
        if (memcmp(element, current, clone->element_size) == 0) {
            ret = index;
        }
        current = it_advance(it, clone->element_size, 1);
    }
    return ret;
}

ssize_t (it_find_index)(const struct iterator_interface *it, bool (*eq)(const void *a, const void *b),
                        const void *element) {
    uint8_t vla[it->size];
    const struct arena_allocator_t a = arena_allocator_init(vla, sizeof(vla));
    const struct iterator_interface *clone = it_clone(it, &a.allocator);

    ssize_t ret = -1;
    ssize_t index = -1;
    const void *current = it_advance(clone, clone->element_size, 0);
    while (current != NULL) {
        index += 1;
        if (eq(element, current)) {
            ret = index;
            break;
        }
        current = it_advance(clone, clone->element_size, 1);
    }
    return ret;
}

ssize_t (it_find_last_index)(const struct iterator_interface *it, bool (*eq)(const void *a, const void *b),
                             const void *element) {
    ssize_t ret = -1, index = -1;
    const void *current = it_advance(it, it->element_size, 0);
    while (current != NULL) {
        index += 1;
        if (eq(element, current)) {
            ret = index;
        }
        current = it_advance(it, it->element_size, 1);
    }
    return ret;
}

#endif //RANDOM_ACCESS_ITERATOR_H
