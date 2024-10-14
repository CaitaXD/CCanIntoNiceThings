#ifndef ARRAYITERATOR_H
#define ARRAYITERATOR_H

#include "Iterator.h"
#include "Array.h"

struct array_iterator_t {
    struct iterator_interface iterator;
    void *current;
    size_t length;
};

struct array_split_iterator_t {
    struct iterator_interface iterator;
    struct span_t current;
    size_t length;
    size_t segment_length;
    size_t element_size;
    uint8_t *data;
};

extern struct iterator_interface array_iterator;

ARRAY_API struct array_iterator_t (array_begin)(const struct array_t *array, size_t element_size);

ARRAY_API struct array_split_iterator_t (array_split)(const struct array_t *array, size_t element_size,
                                                      size_t chunk_size);

ARRAY_API struct array_split_iterator_t (span_split)(struct span_t span, size_t element_size, size_t chunk_size);


#define array_begin(array__, type) (array_begin)((array__), sizeof(type))

static void *array_iterator_advance(struct iterator_interface *it, const size_t element_size, const size_t index) {
    struct array_iterator_t *iterator = (void *) container_of(it, struct array_iterator_t, iterator);
    const intptr_t being_offset = (intptr_t) iterator->current;
    const intptr_t end_offset = (intptr_t) iterator->current + iterator->length * element_size;
    const ptrdiff_t diff = being_offset + index * element_size - end_offset;
    if (diff >= 0) {
        return NULL;
    }
    iterator->current = iterator->current + index * element_size;
    iterator->length = iterator->length - index;
    return iterator->current;
}

struct array_iterator_t (array_begin)(const struct array_t *array, const size_t element_size) {
    return (struct array_iterator_t){
        .iterator = {
            .size = sizeof(struct array_iterator_t),
            .element_size = element_size,
            .advance = (void *) array_iterator_advance,
            .clone = NULL,
        },
        .length = array->length,
        .current = (uint8_t *) array->data
    };
}

static void *array_split_iterator_advance(struct iterator_interface *it, const size_t element_size,
                                          const size_t index) {
    (void) element_size;

    struct array_split_iterator_t *iterator = (void *) container_of(it, struct array_split_iterator_t, iterator);
    if ((ssize_t) iterator->length < iterator->segment_length) {
        return NULL;
    }

    iterator->length = iterator->length - index * iterator->segment_length;
    iterator->current = (struct span_t){
        .length = iterator->segment_length < iterator->length ? iterator->segment_length : iterator->length,
        .data = iterator->data
    };
    iterator->data = iterator->data + iterator->element_size * iterator->segment_length;
    return &iterator->current;
}

struct array_split_iterator_t (array_split)(const struct array_t *array, const size_t element_size,
                                            const size_t chunk_size) {
    const struct array_split_iterator_t iterator = {
        .iterator = {
            .size = sizeof(struct array_split_iterator_t),
            .advance = (void *) array_split_iterator_advance,
            .clone = NULL,
        },
        .length = array->length,
        .segment_length = chunk_size,
        .element_size = element_size,
        .data = (uint8_t *) array->data,
        .current = {},
    };
    return iterator;
}

struct array_split_iterator_t (span_split)(const struct span_t span,
                                           const size_t element_size, const size_t chunk_size) {
    const struct array_split_iterator_t iterator = {
        .iterator = {
            .size = sizeof(struct array_split_iterator_t),
            .advance = (void *) array_split_iterator_advance,
            .clone = NULL,
        },
        .length = span.length,
        .segment_length = chunk_size,
        .element_size = element_size,
        .data = (uint8_t *) span.data,
        .current = {},
    };
    return iterator;
}

#endif //ARRAYITERATOR_H
