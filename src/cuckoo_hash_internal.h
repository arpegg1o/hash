#ifndef CUCKOO_HASH_INTERNAL_H
#define CUCKOO_HASH_INTERNAL_H

#include "../include/cuckoo_hash.h"

#include <stdio.h>
#include <stdlib.h>

#define CUCKOO_HASH_INITIAL_CAPACITY 16
#define CUCKOO_HASH_RESIZE_FACTOR 2
#define CUCKOO_HASH_MAX_LOAD_FACTOR 0.50

struct CuckooHashEntry {
    void* key;
    void* value;
};

struct CuckooHash {
    CuckooHashEntry** table1;
    CuckooHashEntry** table2;

    size_t capacity;
    size_t size;

    CuckooHashConfig config;

    uint64_t seed1;
    uint64_t seed2;
};


CuckooHashEntry* cuckoo_hash_entry_create(
    void* key,
    void* value
);

void cuckoo_hash_entry_destroy(
    const CuckooHash* table,
    CuckooHashEntry* entry
);

/* Modification */

bool cuckoo_hash_insert_entry(
    CuckooHash* table,
    CuckooHashEntry* entry
);

bool cuckoo_hash_has_cycle(
    const CuckooHash* table,
    const CuckooHashEntry* entry
);

bool cuckoo_hash_rehash(
    CuckooHash *table
);

bool cuckoo_hash_resize(
    CuckooHash *table
);

/* Lookup */

CuckooHashEntry* cuckoo_hash_find(
    const CuckooHash* table,
    const void* key
);


/* Capacity */

CuckooHashConfig cuckoo_hash_config(
    const CuckooHash* table
);

void cuckoo_hash_generate_seeds(
    CuckooHash* table
);

/* Iterator */

void cuckoo_hash_iterator_advance(
    CuckooHashIterator* iterator
);

#endif
