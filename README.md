# Cuckoo Hash

A generic cuckoo hash table implementation in C11. The table stores opaque
keys and values, uses two candidate positions per entry, and automatically
resizes or rehashes when insertion becomes difficult.

## Features

- Generic `void *` keys and values
- User-supplied hash and equality functions
- User-supplied key and value destructors
- Lookup, insertion, removal, and duplicate-key detection
- Automatic growth and cycle recovery through resizing and rehashing
- Capacity and load-factor inspection
- Iteration over all stored entries
- No dependency on a project-specific key or value type

## Repository layout

```text
include/cuckoo_hash.h       Public API
src/cuckoo_hash.c           Implementation
src/cuckoo_hash_internal.h  Internal data structures and helpers
tests/cuckoo_hash_test.c    Test suite and usage examples
makefile                    Build and test targets
```

## Requirements

- A C compiler with C11 support
- `make`

The implementation uses `arc4random()` to generate hash seeds. On platforms
where this function is not provided by the system C library, a compatible
implementation or portability adjustment may be required.

## Build and test

Build the test executable:

```sh
make
```

Run the test suite:

```sh
make run
```

Clean generated objects, dependency files, and the test executable:

```sh
make clean
```

The default compiler is `clang`, but it can be overridden:

```sh
make CC=gcc
```

Additional compiler, preprocessor, linker, and library flags can be supplied
through `CFLAGS`, `CPPFLAGS`, `LDFLAGS`, and `LDLIBS`.

## Usage

Include the public header and provide callbacks for hashing, equality, and
destruction:

```c
#include "cuckoo_hash.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static size_t hash_key(const void *key, uint64_t seed) {
    const unsigned int value = *(const unsigned int *)key;
    return (size_t)value + (size_t)seed;
}

static bool equal_keys(const void *a, const void *b) {
    return *(const unsigned int *)a == *(const unsigned int *)b;
}

static void destroy_key(void *key) {
    free(key);
}

static void destroy_value(void *value) {
    free(value);
}

int main(void) {
    CuckooHashConfig config = {
        .hash = hash_key,
        .equal = equal_keys,
        .key_destroy = destroy_key,
        .value_destroy = destroy_value,
    };

    CuckooHash *table = cuckoo_hash_create(config);
    if (table == NULL) {
        return 1;
    }

    unsigned int *key = malloc(sizeof(*key));
    int *value = malloc(sizeof(*value));
    if (key == NULL || value == NULL) {
        free(key);
        free(value);
        cuckoo_hash_destroy(table);
        return 1;
    }

    *key = 42;
    *value = 123;

    const bool inserted = cuckoo_hash_insert(table, key, value);
    if (!inserted) {
        /* The caller retains ownership when insertion fails. */
        destroy_key(key);
        destroy_value(value);
    }

    if (inserted) {
        int *stored_value = cuckoo_hash_get(table, key);
        if (stored_value != NULL) {
            /* Use *stored_value here. */
        }
    }

    cuckoo_hash_destroy(table);
    return 0;
}
```

Compile an application against the implementation and public header, for
example:

```sh
cc -std=c11 -Wall -Wextra -Iinclude \
    src/cuckoo_hash.c app.c -o app
```

## Ownership and behavior

- On a successful insert, the table owns the key and value and releases them
  using `key_destroy` and `value_destroy` when the entry is removed or the
  table is destroyed.
- If insertion fails, including because an equivalent key already exists, the
  caller retains ownership of the key and value passed to that insert.
- A failed insertion of an equivalent key does not replace the existing value.
- A missing lookup returns `NULL`; removing a missing key returns `false`.
- Iterators expose `CuckooHashEntry` objects. Use
  `cuckoo_hash_entry_key()` and `cuckoo_hash_entry_value()` to access their
  contents.
- Do not destroy or mutate the table while an iterator is in use. Rehashing,
  resizing, insertion, or removal can invalidate an iterator.
- The public API does not provide synchronization primitives. Coordinate
  concurrent access externally if a table is shared between threads.

## Public API

The declarations in [`include/cuckoo_hash.h`](include/cuckoo_hash.h) cover:

- **Lifecycle:** `cuckoo_hash_create`, `cuckoo_hash_destroy`
- **Modification:** `cuckoo_hash_insert`, `cuckoo_hash_remove`
- **Lookup:** `cuckoo_hash_contains`, `cuckoo_hash_get`
- **Capacity:** `cuckoo_hash_reserve`, `cuckoo_hash_size`,
  `cuckoo_hash_capacity`, `cuckoo_hash_load_factor`,
  `cuckoo_hash_is_empty`
- **Iteration:** `cuckoo_hash_iterator_create`,
  `cuckoo_hash_iterator_has_next`, `cuckoo_hash_iterator_next`
- **Entry access:** `cuckoo_hash_entry_key`, `cuckoo_hash_entry_value`

## License

No license file is currently included in the repository.
