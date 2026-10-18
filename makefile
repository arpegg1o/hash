CC ?= clang

CFLAGS ?= -Wall -Wextra -Wpedantic -std=c11 -g
CPPFLAGS ?= -Iinclude

TARGET := test
SOURCES := src/cuckoo_hash.c tests/cuckoo_hash_test.c
OBJECTS := $(SOURCES:.c=.o)
DEPENDS := $(OBJECTS:.o=.d)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

src/%.o: src/%.c src/cuckoo_hash_internal.h include/cuckoo_hash.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

tests/%.o: tests/%.c include/cuckoo_hash.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

leak-check: $(TARGET)
	leaks --atExit -- ./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPENDS)

-include $(DEPENDS)

.PHONY: run leak-check clean
