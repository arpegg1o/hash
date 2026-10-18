
#include "cuckoo_hash_internal.h"

CuckooHashEntry* cuckoo_hash_entry_create(void* key, void* value) {
    CuckooHashEntry* entry = malloc(sizeof(CuckooHashEntry));
    if (!entry) {
        return NULL;
    }

    entry->key = key;
    entry->value = value;

    return entry;
}

void cuckoo_hash_entry_destroy(const CuckooHash* table, CuckooHashEntry* entry) {
    if (entry != NULL) {
        if (entry->key != NULL) {
            table->config.key_destroy(entry->key);
        }

        if (entry->value != NULL) {
            table->config.value_destroy(entry->value);
        }
    }

    free(entry);
}

void* cuckoo_hash_entry_key(const CuckooHashEntry *entry) {
    if (entry == NULL) {
        return NULL;
    }

    return entry->key;
}

void * cuckoo_hash_entry_value(const CuckooHashEntry *entry) {
    if (entry == NULL) {
        return NULL;
    }

    return entry->value;
}

/* Creation */

CuckooHash* cuckoo_hash_create(const CuckooHashConfig config) {
    CuckooHash* table = malloc(sizeof(CuckooHash));
    if (!table) {
        return NULL;
    }

    table->table1 = calloc(CUCKOO_HASH_INITIAL_CAPACITY, sizeof(CuckooHashEntry*));
    if (!table->table1) {
        free(table);
        return NULL;
    }

    table->table2 = calloc(CUCKOO_HASH_INITIAL_CAPACITY, sizeof(CuckooHashEntry*));
    if (!table->table2) {
        free(table->table1);
        free(table);
        return NULL;
    }

    table->capacity = CUCKOO_HASH_INITIAL_CAPACITY;
    table->size = 0;

    table->config = config;

    cuckoo_hash_generate_seeds(table);

    return table;
}

void cuckoo_hash_destroy(CuckooHash* table) {
    if (table != NULL) {
        CuckooHashIterator iterator = cuckoo_hash_iterator_create(table);
        while (cuckoo_hash_iterator_has_next(&iterator)) {
            cuckoo_hash_entry_destroy(table, cuckoo_hash_iterator_next(&iterator));
        }

        free(table->table1);
        free(table->table2);
        free(table);
    }
}

/* Modification */

bool cuckoo_hash_insert(CuckooHash* table, const void* key, const void* value) {
    CuckooHashEntry* entry = cuckoo_hash_entry_create(key, value);
    printf("Created Entry and ready to insert\n");
    return cuckoo_hash_insert_entry(table, entry);
}

bool cuckoo_hash_insert_entry(CuckooHash* table, CuckooHashEntry* entry) {
    if (cuckoo_hash_contains(table, entry->key)) {
        printf("Hash already contains KEY %p.\n", entry->key);
        return false;
    }

    printf("new Entry detected\n");

    while ((double)(table->size + 1) / (2 * table->capacity) > CUCKOO_HASH_MAX_LOAD_FACTOR) {
        printf("MAX LOAD FACTOR reached while inserting KEY %p.\n", entry->key);
        if (!cuckoo_hash_resize(table)) {
            return false;
        }
    }

    while (cuckoo_hash_has_cycle(table, entry)) {
        printf("Cycle detected while inserting KEY %p.\n", entry->key);
        if (!cuckoo_hash_rehash(table)) {
            return false;
        }
    }

    CuckooHashEntry* temp;

    while (true) {
        printf("Inserting KEY %p into index %d at table 1...\n", entry->key, table->config.hash(entry->key, table->seed1) % table->capacity);
        temp = table->table1[table->config.hash(entry->key, table->seed1) % table->capacity];
        table->table1[table->config.hash(entry->key, table->seed1) % table->capacity] = entry;
        entry = temp;
        if (entry == NULL) {
            table->size++;
            return true;
        }


        printf("Inserting KEY %p into index %d at table 2...\n", entry->key, table->config.hash(entry->key, table->seed2) % table->capacity);
        temp = table->table2[table->config.hash(entry->key, table->seed2) % table->capacity];
        table->table2[table->config.hash(entry->key, table->seed2) % table->capacity] = entry;
        entry = temp;
        if (entry == NULL) {
            table->size++;
            return true;
        }
    }
}

bool cuckoo_hash_remove(CuckooHash* table, const void* key) {
    CuckooHashEntry* entry;
    size_t index;

    index = table->config.hash(key, table->seed1) % table->capacity;
    entry = table->table1[index];
    if (entry != NULL) {
        if (table->config.equal(entry->key, key)) {
            cuckoo_hash_entry_destroy(table, entry);
            table->table1[index] = NULL;
            table->size--;

            return true;
        }
    }


    index = table->config.hash(key, table->seed2) % table->capacity;
    entry = table->table2[index];
    if (entry != NULL) {
        if (table->config.equal(entry->key, key)) {
            cuckoo_hash_entry_destroy(table, entry);
            table->table2[index] = NULL;
            table->size--;

            return true;
        }
    }


    return false;
}

bool cuckoo_hash_has_cycle(const CuckooHash* table, const CuckooHashEntry* entry) {
    const CuckooHashEntry* tortoise_entry = entry;
    int tortoise_table_index = 0;

    const CuckooHashEntry* hare_entry = entry;
    int hare_table_index = 0;

    while (true) {
        switch(tortoise_table_index) {
            case 0:
                tortoise_entry = table->table1[table->config.hash(tortoise_entry->key, table->seed1) % table->capacity];
                break;
            case 1:
                tortoise_entry = table->table2[table->config.hash(tortoise_entry->key, table->seed2) % table->capacity];
        }

        if (tortoise_entry == NULL) {
            return false;
        }

        tortoise_table_index = 1 - tortoise_table_index;


        for (size_t i = 0; i < 2; i++) {
            switch(hare_table_index) {
                case 0:
                    hare_entry = table->table1[table->config.hash(hare_entry->key, table->seed1) % table->capacity];
                    break;
                case 1:
                    hare_entry = table->table2[table->config.hash(hare_entry->key, table->seed2) % table->capacity];
            }

            if (hare_entry == NULL) {
                return false;
            }

            hare_table_index = 1 - hare_table_index;
        }

        if (hare_entry == tortoise_entry && hare_table_index == tortoise_table_index) {
            return true;
        }
    }
}

bool cuckoo_hash_rehash(CuckooHash *table) {
    printf("Trying to rehash...\n");
    CuckooHash* temp = malloc(sizeof(CuckooHash));
    temp->config = table->config;
    temp->table1 = calloc(table->capacity, sizeof(CuckooHashEntry*));
    if (temp->table1 == NULL) {
        return false;
    }

    temp->table2 = calloc(table->capacity, sizeof(CuckooHashEntry*));
    if (temp->table2 == NULL) {
        free(temp->table1);
        return false;
    }

    temp->capacity = table->capacity;
    temp->size = 0;
    cuckoo_hash_generate_seeds(temp);
    printf("seed1: %llu\n", temp->seed1);

    CuckooHashIterator iterator = cuckoo_hash_iterator_create(table);
    printf("Created iterator\n");
    while (cuckoo_hash_iterator_has_next(&iterator)) {
        printf("Has next\n");
        if (!cuckoo_hash_insert_entry(temp, cuckoo_hash_iterator_next(&iterator))) {
            free(temp->table1);
            free(temp->table2);
            return false;
        }
    }


    free(table->table1);
    free(table->table2);

    table->table1 = temp->table1;
    table->table2 = temp->table2;

    table->seed1 = temp->seed1;
    table->seed2 = temp->seed2;

    free(temp);

    return true;
}

bool cuckoo_hash_resize(CuckooHash *table) {
    printf("Trying to resize...\n");
    CuckooHash* temp = malloc(sizeof(CuckooHash));
    temp->config = table->config;
    temp->table1 = calloc(table->capacity * CUCKOO_HASH_RESIZE_FACTOR, sizeof(CuckooHashEntry*));
    if (temp->table1 == NULL) {
        return false;
    }

    temp->table2 = calloc(table->capacity * CUCKOO_HASH_RESIZE_FACTOR, sizeof(CuckooHashEntry*));
    if (temp->table2 == NULL) {
        free(temp->table1);
        return false;
    }

    temp->capacity = table->capacity * CUCKOO_HASH_RESIZE_FACTOR;
    temp->size = 0;
    cuckoo_hash_generate_seeds(temp);
    printf("seed1: %d\n", temp->seed1);

    CuckooHashIterator iterator = cuckoo_hash_iterator_create(table);
    printf("Created iterator\n");
    while (cuckoo_hash_iterator_has_next(&iterator)) {
        printf("Has next\n");
        if (!cuckoo_hash_insert_entry(temp, cuckoo_hash_iterator_next(&iterator))) {
            free(temp->table1);
            free(temp->table2);
            return false;
        }
    }


    free(table->table1);
    free(table->table2);

    table->table1 = temp->table1;
    table->table2 = temp->table2;

    table->seed1 = temp->seed1;
    table->seed2 = temp->seed2;

    table->capacity = temp->capacity;

    free(temp);

    return true;
}

/* Capacity */

size_t cuckoo_hash_size(const CuckooHash* table) {
    return table->size;
}

size_t cuckoo_hash_capacity(const CuckooHash* table) {
    return table->capacity;
}

double cuckoo_hash_load_factor(const CuckooHash* table) {
    return (double)(table->size) / (2 * table->capacity);
}

bool cuckoo_hash_is_empty(const CuckooHash* table) {
    return table->size == 0;
}

CuckooHashConfig cuckoo_hash_config(const CuckooHash* table) {
    return table->config;
}

/* Lookup */

bool cuckoo_hash_contains(const CuckooHash* table, const void* key) {
    return cuckoo_hash_find(table, key) != NULL;
}

void* cuckoo_hash_get(const CuckooHash* table, const void* key) {
    CuckooHashEntry* entry = cuckoo_hash_find(table, key);
    if (entry == NULL) {
        return NULL;
    }

    return entry->value;
}

CuckooHashEntry* cuckoo_hash_find(const CuckooHash* table, const void* key) {
    CuckooHashEntry* entry;

    if (key == NULL) {
        return NULL;
    }

    if (table == NULL) {
        printf("table is null\n");
    }

    if (table->table1 == NULL) {
        printf("table->table1 is null\n");
    }

    if (table->table2 == NULL) {
        printf("table->table2 is null\n");
    }

    printf("table: %p\n", table);
    printf(" table->table1: %p\n",  table->table1);
    printf("key: %p\n", key);
    printf("seed1: %llu\n", table->seed1);
    printf("capacity: %lu\n", table->capacity);

    entry = table->table1[table->config.hash(key, table->seed1) % table->capacity];
    printf("Searching for KEY %p in index %d in table 1...(which is entry %p)\n", key,  table->config.hash(key, table->seed1) % table->capacity, entry);
    if (entry != NULL) {
        if (table->config.equal(entry->key, key)) {
            return entry;
        }
    }


    entry = table->table2[table->config.hash(key, table->seed2) % table->capacity];
    printf("Searching for KEY %p in index %d in table 2...\n", key , table->config.hash(key, table->seed2) % table->capacity);
    if (entry != NULL) {
        if (table->config.equal(entry->key, key)) {
            return entry;
        }
    }

    return NULL;
}

void cuckoo_hash_generate_seeds(CuckooHash* table) {
    table->seed1 = ((uint64_t)arc4random() << 32) | arc4random();
    table->seed2 = ((uint64_t)arc4random() << 32) | arc4random();
}

/* Iterator */

CuckooHashIterator cuckoo_hash_iterator_create(const CuckooHash* table) {

    CuckooHashIterator iterator = {
        .table = table,
        .table_index = 0,
        .index = 0,
    };

    cuckoo_hash_iterator_advance(&iterator);

    return iterator;
}

bool cuckoo_hash_iterator_has_next(const CuckooHashIterator *iterator) {
    return iterator->table_index <= 1;
}

CuckooHashEntry *cuckoo_hash_iterator_next(CuckooHashIterator *iterator) {
    if (!cuckoo_hash_iterator_has_next(iterator)) {
        return NULL;
    }

    CuckooHashEntry* entry;
    if (iterator->table_index == 0) entry = iterator->table->table1[iterator->index];
    if (iterator->table_index == 1) entry = iterator->table->table2[iterator->index];

    iterator->index++;
    cuckoo_hash_iterator_advance(iterator);

    return entry;
}

void cuckoo_hash_iterator_advance(CuckooHashIterator* iterator) {
    while (iterator->table_index <= 1) {
        while (iterator->index < iterator->table->capacity) {

            if (iterator->table_index == 0) {
                if (iterator->table->table1[iterator->index] != NULL) return;
            }

            if (iterator->table_index == 1) {
                if (iterator->table->table2[iterator->index] != NULL) return;
            }

            iterator->index++;
        }

        iterator->table_index++;
        iterator->index = 0;
    }
}
