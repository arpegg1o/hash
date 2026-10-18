#ifndef CUCKOO_HASH_H
#define CUCKOO_HASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct CuckooHash CuckooHash;
typedef struct CuckooHashEntry CuckooHashEntry;

typedef struct {
    const CuckooHash* table;
    size_t table_index;
    size_t index;
} CuckooHashIterator;

typedef size_t (*CuckooHashFunction)(
    const void *key,
    uint64_t seed
);

typedef bool (*CuckooHashEqualFunction)(
    const void *a,
    const void *b
);

typedef void* (*CuckooHashCopyFunction)(
    const void *source
);

typedef void (*CuckooHashDestroyFunction)(
    void *data
);

typedef struct {
    CuckooHashFunction hash;
    CuckooHashEqualFunction equal;

    CuckooHashDestroyFunction key_destroy;
    CuckooHashDestroyFunction value_destroy;
} CuckooHashConfig;

void *cuckoo_hash_entry_key(
    const CuckooHashEntry *entry
);

void *cuckoo_hash_entry_value(
    const CuckooHashEntry *entry
);

/* Creation */

CuckooHash* cuckoo_hash_create(
    const CuckooHashConfig config
);

void cuckoo_hash_destroy(
    CuckooHash* table
);

/* Modification */

bool cuckoo_hash_insert(
    CuckooHash* table,
    const void* key,
    const void* value
);

bool cuckoo_hash_remove(
    CuckooHash* table,
    const void* key
);


/* Lookup */

bool cuckoo_hash_contains(
    const CuckooHash* table,
    const void* key
);

void* cuckoo_hash_get(
    const CuckooHash* table,
    const void* key
);

/* Capacity */

bool cuckoo_hash_reserve(
    CuckooHash* table,
    size_t capacity
);

size_t cuckoo_hash_size(
    const CuckooHash* table
);

size_t cuckoo_hash_capacity(
    const CuckooHash* table
);

double cuckoo_hash_load_factor(
    const CuckooHash* table
);

bool cuckoo_hash_is_empty(
    const CuckooHash* table
);

/* Iterator */

CuckooHashIterator cuckoo_hash_iterator_create(
    const CuckooHash* table
);

bool cuckoo_hash_iterator_has_next(
    const CuckooHashIterator *iterator
);

CuckooHashEntry *cuckoo_hash_iterator_next(
    CuckooHashIterator *iterator
);

#endif
