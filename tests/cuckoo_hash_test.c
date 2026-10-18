
#include "../include/cuckoo_hash.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    unsigned long long id;
    char* name;
} Person;


/* =========================================================
   Test helpers
   ========================================================= */

static Person* person_create(unsigned long long id, const char* name) {
    Person* person = malloc(sizeof(Person));
    assert(person != NULL);

    person->id = id;

    person->name = malloc(strlen(name) + 1);
    assert(person->name != NULL);

    strcpy(person->name, name);

    return person;
}

static int* int_create(int value) {
    int* result = malloc(sizeof(int));
    assert(result != NULL);

    *result = value;

    return result;
}


/* =========================================================
   Hash functions
   ========================================================= */

static size_t person_hash(const void* key, uint64_t seed) {
    const Person* person = key;

    uint64_t x = person->id + seed;

    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x ^= x >> 31;

    return (size_t)x;
}


/*
 * Deliberately terrible hash function.
 *
 * Every key gets exactly the same hash value.
 * This is useful for testing collision handling.
 */
static size_t collision_hash(const void* key, uint64_t seed) {
    (void)key;
    (void)seed;

    return 0;
}


/* =========================================================
   Equality
   ========================================================= */

static bool person_equal(const void* a, const void* b) {
    if (a == NULL || b == NULL) {
        return a == b;
    }

    return ((const Person*)a)->id == ((const Person*)b)->id;
}


/* =========================================================
   Destructors
   ========================================================= */

static void person_destroy(void* key) {
    if (key != NULL) {
        Person* person = key;

        free(person->name);
        free(person);
    }
}

static void int_destroy(void* value) {
    free(value);
}


/* =========================================================
   Configurations
   ========================================================= */

static CuckooHashConfig normal_config(void) {
    return (CuckooHashConfig){
        .hash = person_hash,
        .equal = person_equal,
        .key_destroy = person_destroy,
        .value_destroy = int_destroy
    };
}

static CuckooHashConfig collision_config(void) {
    return (CuckooHashConfig){
        .hash = collision_hash,
        .equal = person_equal,
        .key_destroy = person_destroy,
        .value_destroy = int_destroy
    };
}


/* =========================================================
   Tests
   ========================================================= */

static void test_create_destroy(void) {
    printf("test_create_destroy... ");

    CuckooHash* table = cuckoo_hash_create(normal_config());

    assert(table != NULL);
    assert(cuckoo_hash_size(table) == 0);
    assert(cuckoo_hash_is_empty(table));
    assert(cuckoo_hash_load_factor(table) == 0.0);
    assert(cuckoo_hash_capacity(table) > 0);

    cuckoo_hash_destroy(table);

    printf("PASS\n");
}


static void test_insert_and_get(void) {
    printf("test_insert_and_get...\n");

    CuckooHash* table = cuckoo_hash_create(normal_config());
    assert(table != NULL);
    printf("Created hash table\n");

    Person* key = person_create(1, "Alice");
    int* value = int_create(42);

    assert(cuckoo_hash_insert(table, key, value));
    printf("Inserted KEY\n");

    assert(cuckoo_hash_size(table) == 1);
    assert(!cuckoo_hash_is_empty(table));

    printf("Key: %p\n", key);
    assert(cuckoo_hash_contains(table, key));

    int* result = cuckoo_hash_get(table, key);

    assert(result != NULL);
    assert(*result == 42);

    cuckoo_hash_destroy(table);

    printf("PASS\n");
}


static void test_missing_key(void) {
    printf("test_missing_key... ");

    CuckooHash* table = cuckoo_hash_create(normal_config());
    assert(table != NULL);

    Person* stored_key = person_create(1, "Alice");
    int* value = int_create(42);

    assert(cuckoo_hash_insert(table, stored_key, value));

    Person* missing_key = person_create(999, "Nobody");

    assert(!cuckoo_hash_contains(table, missing_key));
    assert(cuckoo_hash_get(table, missing_key) == NULL);

    /*
     * This key was never inserted, so the hash table
     * does not own it.
     */
    person_destroy(missing_key);

    cuckoo_hash_destroy(table);

    printf("PASS\n");
}


static void test_multiple_entries(void) {
    printf("test_multiple_entries... ");

    CuckooHash* table = cuckoo_hash_create(normal_config());
    assert(table != NULL);

    enum {
        COUNT = 100
    };

    Person* keys[COUNT];

    for (size_t i = 0; i < COUNT; i++) {
        keys[i] = person_create(i, "Person");

        int* value = int_create((int)(i * 10));

        assert(cuckoo_hash_insert(table, keys[i], value));
    }

    assert(cuckoo_hash_size(table) == COUNT);
    assert(!cuckoo_hash_is_empty(table));

    /*
     * Verify every element after the table has
     * presumably performed several resizes/rehashes.
     */
    for (size_t i = 0; i < COUNT; i++) {
        assert(cuckoo_hash_contains(table, keys[i]));

        int* value = cuckoo_hash_get(table, keys[i]);

        assert(value != NULL);
        assert(*value == (int)(i * 10));
    }

    cuckoo_hash_destroy(table);

    printf("PASS\n");
}


static void test_collisions(void) {
    printf("test_collisions... ");

    CuckooHash* table = cuckoo_hash_create(normal_config());
    assert(table != NULL);

    enum {
        COUNT = 20
    };

    Person* keys[COUNT];

    for (size_t i = 0; i < COUNT; i++) {
        keys[i] = person_create(i, "Collision");

        int* value = int_create((int)i);

        assert(cuckoo_hash_insert(table, keys[i], value));
    }

    assert(cuckoo_hash_size(table) == COUNT);

    /*
     * Every key has the exact same two candidate positions.
     *
     * This strongly tests:
     * - collision handling
     * - eviction
     * - cycle detection
     * - rehashing
     */
    for (size_t i = 0; i < COUNT; i++) {
        assert(cuckoo_hash_contains(table, keys[i]));

        int* value = cuckoo_hash_get(table, keys[i]);

        assert(value != NULL);
        assert(*value == (int)i);
    }

    cuckoo_hash_destroy(table);

    printf("PASS\n");
}


static void test_iterator(void) {
    printf("test_iterator... ");

    CuckooHash* table = cuckoo_hash_create(normal_config());
    assert(table != NULL);

    enum {
        COUNT = 100
    };

    for (size_t i = 0; i < COUNT; i++) {
        Person* key = person_create(i, "Iterator");
        int* value = int_create((int)i);

        assert(cuckoo_hash_insert(table, key, value));
    }

    bool seen[COUNT] = { false };

    CuckooHashIterator iterator =
        cuckoo_hash_iterator_create(table);

    size_t count = 0;

    while (cuckoo_hash_iterator_has_next(&iterator)) {
        CuckooHashEntry* entry =
            cuckoo_hash_iterator_next(&iterator);

        assert(entry != NULL);

        Person* person =
            cuckoo_hash_entry_key(entry);

        int* value =
            cuckoo_hash_entry_value(entry);

        assert(person != NULL);
        assert(value != NULL);

        assert(person->id < COUNT);
        assert(*value == (int)person->id);

        /*
         * Every element should occur exactly once.
         */
        assert(!seen[person->id]);

        seen[person->id] = true;
        count++;
    }

    assert(count == COUNT);

    for (size_t i = 0; i < COUNT; i++) {
        assert(seen[i]);
    }

    /*
     * Calling next() after exhaustion should return NULL.
     */
    assert(cuckoo_hash_iterator_next(&iterator) == NULL);

    cuckoo_hash_destroy(table);

    printf("PASS\n");
}


static void test_remove(void) {
    printf("test_remove... ");

    CuckooHash* table = cuckoo_hash_create(normal_config());
    assert(table != NULL);

    enum {
        COUNT = 50
    };

    Person* keys[COUNT];

    for (size_t i = 0; i < COUNT; i++) {
        keys[i] = person_create(i, "Remove");

        int* value = int_create((int)i);

        assert(cuckoo_hash_insert(table, keys[i], value));
    }

    assert(cuckoo_hash_size(table) == COUNT);

    /*
     * Remove every other element.
     */
    for (size_t i = 0; i < COUNT; i += 2) {
        assert(cuckoo_hash_remove(table, keys[i]));

        assert(!cuckoo_hash_contains(table, keys[i]));
        assert(cuckoo_hash_get(table, keys[i]) == NULL);
    }

    assert(cuckoo_hash_size(table) == COUNT / 2);

    /*
     * The remaining elements must still work.
     */
    for (size_t i = 1; i < COUNT; i += 2) {
        assert(cuckoo_hash_contains(table, keys[i]));

        int* value = cuckoo_hash_get(table, keys[i]);

        assert(value != NULL);
        assert(*value == (int)i);
    }

    /*
     * Removing an already removed key should fail.
     */
    for (size_t i = 0; i < COUNT; i += 2) {
        assert(!cuckoo_hash_remove(table, keys[i]));
    }

    cuckoo_hash_destroy(table);

    printf("PASS\n");
}


static void test_remove_all(void) {
    printf("test_remove_all... ");

    CuckooHash* table = cuckoo_hash_create(normal_config());
    assert(table != NULL);

    enum {
        COUNT = 30
    };

    Person* keys[COUNT];

    for (size_t i = 0; i < COUNT; i++) {
        keys[i] = person_create(i, "Remove All");

        int* value = int_create((int)i);

        assert(cuckoo_hash_insert(table, keys[i], value));
    }

    for (size_t i = 0; i < COUNT; i++) {
        assert(cuckoo_hash_remove(table, keys[i]));
    }

    assert(cuckoo_hash_size(table) == 0);
    assert(cuckoo_hash_is_empty(table));

    CuckooHashIterator iterator =
        cuckoo_hash_iterator_create(table);

    assert(!cuckoo_hash_iterator_has_next(&iterator));

    /*
     * The table should still be usable after becoming empty.
     */
    Person* key = person_create(1234, "After Empty");
    int* value = int_create(999);

    assert(cuckoo_hash_insert(table, key, value));

    assert(cuckoo_hash_size(table) == 1);
    assert(*((int*)cuckoo_hash_get(table, key)) == 999);

    cuckoo_hash_destroy(table);

    printf("PASS\n");
}



static void test_duplicate_key(void) {
    printf("test_duplicate_key... ");

    CuckooHash* table = cuckoo_hash_create(normal_config());
    assert(table != NULL);

    Person* key1 = person_create(1, "Alice");
    int* value1 = int_create(100);

    assert(cuckoo_hash_insert(table, key1, value1));
    assert(cuckoo_hash_size(table) == 1);

    /*
     * Different Person object, but logically equal key.
     */
    Person* key2 = person_create(1, "Different Object");
    int* value2 = int_create(200);

    /*
     * Assuming insert() rejects duplicate keys.
     */
    assert(!cuckoo_hash_insert(table, key2, value2));

    /*
     * Since insertion failed, ownership of key2/value2
     * remains with the caller.
     */
    person_destroy(key2);
    int_destroy(value2);

    assert(cuckoo_hash_size(table) == 1);

    int* stored_value = cuckoo_hash_get(table, key1);

    assert(stored_value != NULL);
    assert(*stored_value == 100);

    cuckoo_hash_destroy(table);

    printf("PASS\n");
}


/* =========================================================
   Main
   ========================================================= */

int main(void) {
    printf("Running Cuckoo Hash Table tests...\n\n");

    test_create_destroy();
    test_insert_and_get();
    test_missing_key();
    test_multiple_entries();
    test_collisions();
    test_iterator();
    test_remove();
    test_remove_all();
    test_duplicate_key();

    printf("\nAll tests passed!\n");

    return EXIT_SUCCESS;
}
