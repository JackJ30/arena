#include "arena.h"

// using libc malloc backend

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


// TODO: add debug statistic collection mode for arena
// Should collect things like:
// - How many times new_region was called
// - How many times existing region was skipped
// - How many times allocation exceeded ARENA_REGION_DEFAULT_CAPACITY

// Regions

// TODO: instead of accepting specific capacity new_region() should accept the size of the object we want to fit into the region
// It should be up to new_region() to decide the actual capacity to allocate
ArenaRegion *new_region(size_t capacity) {
    size_t size = sizeof(ArenaRegion) + capacity;
    // TODO: it would be nice if we could guarantee that the regions are allocated by ARENA_BACKEND_LIBC_MALLOC are page aligned
    ArenaRegion *r = (ArenaRegion*)malloc(size);
    assert(r); // TODO: since ARENA_ASSERT is disableable go through all the places where we use it to check for failed memory allocation and return with NULL there.
    r->next = NULL;
    r->count = 0;
    r->capacity = capacity;
    return r;
}

void free_region(ArenaRegion *r) {
    free(r);
}

// Arena

void arena_create(Arena *a, size_t default_region_size) {
	assert(a->first == NULL && a->current == NULL);

	// create first region and set region size
	a->current = new_region(default_region_size);
	a->first = a->current;
	a->default_region_size = default_region_size;
}

void arena_destroy(Arena *a) {
	assert(a->first != NULL && a->current != NULL);

    ArenaRegion *r = a->first;
    while (r) {
        ArenaRegion *r0 = r;
        r = r->next;
        free_region(r0);
    }
    a->first = NULL;
    a->current = NULL;
}

size_t get_aligned_offset(ArenaRegion* region, size_t align) {
	uintptr_t base = (uintptr_t)region->data;
	uintptr_t ptr  = base + region->count;
	uintptr_t aligned = (ptr + (align - 1)) & ~(uintptr_t)(align - 1);
	return aligned - base;
}

void *arena_alloc(Arena *a, size_t size, size_t align) {
	assert(a->first != NULL && a->current != NULL);
	assert((align & (align - 1)) == 0); // alignment must be power of 2

	while (true) {
		uintptr_t base = (uintptr_t)a->current->data;
		uintptr_t ptr  = base + a->current->count;
		uintptr_t aligned = (ptr + (align - 1)) & ~(uintptr_t)(align - 1);
		size_t aligned_offset = aligned - base;

		// fits in this region
		if (aligned_offset + size <= a->current->capacity) {
			// allocate it
            a->current->count = aligned_offset + size;
            return (void*)aligned;
		}

		// move or allocate new region if at the end
        if (a->current->next) {
            a->current = a->current->next;
        } else {
            size_t cap = a->default_region_size;
            if (cap < align + size) cap = align + size;
            a->current->next = new_region(cap);
            a->current = a->current->next;
        }
	}
}

void *arena_alloc_zero(Arena *a, size_t size, size_t align) {
	assert(a->first != NULL && a->current != NULL);

	void* ret = arena_alloc(a, size, align);
	memset(ret, 0, size);
	return ret;
}

void *arena_realloc(Arena *a, void *oldptr, size_t oldsz, size_t newsz, size_t align) {
	assert(a->first != NULL && a->current != NULL);

    if (newsz <= oldsz) return oldptr;
    void *newptr = arena_alloc(a, newsz, align);
    char *newptr_char = (char*)newptr;
    char *oldptr_char = (char*)oldptr;
    for (size_t i = 0; i < oldsz; ++i) {
        newptr_char[i] = oldptr_char[i];
    }
    return newptr;

	// realloc improvement will have to confirm alignment
}


void arena_reset(Arena *a) {
	assert(a->first != NULL && a->current != NULL);

    for (ArenaRegion *r = a->first; r != NULL; r = r->next) {
        r->count = 0;
    }

    a->current = a->first;
}

void arena_trim(Arena *a) {
	assert(a->first != NULL && a->current != NULL);

	if (!a->current) return;
    ArenaRegion *r = a->current->next;
    while (r) {
        ArenaRegion *r0 = r;
        r = r->next;
        free_region(r0);
    }
    a->current->next = NULL;
}

char *arena_strdup(Arena *a, const char *cstr) {
	assert(a->first != NULL && a->current != NULL);

    size_t n = strlen(cstr);
    char *dup = (char*)arena_alloc(a, n + 1, _Alignof(char));
    memcpy(dup, cstr, n);
    dup[n] = '\0';
    return dup;
}

char *arena_vsprintf(Arena *a, const char *format, va_list args) {
	assert(a->first != NULL && a->current != NULL);

    va_list args_copy;
    va_copy(args_copy, args);
    int n = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    assert(n >= 0);
    char *result = (char*)arena_alloc(a, n + 1, _Alignof(char));
    vsnprintf(result, n + 1, format, args);

    return result;
}

char *arena_sprintf(Arena *a, const char *format, ...) {
	assert(a->first != NULL && a->current != NULL);

    va_list args;
    va_start(args, format);
    char *result = arena_vsprintf(a, format, args);
    va_end(args);

    return result;
}

ArenaMark arena_mark(Arena *a) {
	assert(a->first != NULL && a->current != NULL);

    ArenaMark m;
	m.arena = a;
	m.region = a->current;
	m.count  = a->current->count;

    return m;
}

void arena_rewind(ArenaMark m) {
	assert(m.arena != NULL && m.region != NULL && m.arena->first != NULL && m.arena->current != NULL);

    m.region->count = m.count;
    for (ArenaRegion *r = m.region->next; r != NULL; r = r->next) {
        r->count = 0;
    }

    m.arena->current = m.region;
}

// scratch
Arena scratch_pool[2] = {0};
ArenaMark get_scratch_arena(Arena** conflicting, int num_conflicting) {
    for (int i = 0; i < sizeof(scratch_pool) / sizeof(scratch_pool[0]); i++) {
        bool is_conflicting = false;
        for (int z = 0; z < num_conflicting; z++) {
            if (&scratch_pool[i] == conflicting[z]) {
                is_conflicting = true;
            }
        }

        if (!is_conflicting) {
            return arena_mark(&scratch_pool[i]);
        }
    }

	assert(false); // should be unreachable unless something is wrong
    return (ArenaMark){NULL, NULL, -1};
}

void arena_debug_stats(Arena *a, size_t *allocated, size_t* used, size_t* wasted) {
	assert(a->first != NULL && a->current != NULL);

	if (allocated) *allocated = 0;
	if (used) *used = 0;
	if (wasted) *wasted = 0;

	bool reached_current = false;
    ArenaRegion *r = a->first;
    while (r) {
		if (r == a->current) reached_current = true;

		if (allocated) *allocated += r->capacity;
		if (used) *used += r->count;
		if (wasted && !reached_current) *wasted += r->capacity - r->count;

        r = r->next;
    }
}

void arena_debug_print(Arena *a) {
	assert(a->first != NULL && a->current != NULL);

	size_t allocated, used, wasted;
	arena_debug_stats(a, &allocated, &used, &wasted);
	printf("=== Arena %p debug info: [%zu/%zu bytes] (%zu bytes wasted) ===\n", a, used, allocated, wasted);

    ArenaRegion *r = a->first;
	size_t cur = 0;
    while (r) {
		char* selected = "";
		if (r == a->current) {
			selected = " <---";
		}
		printf("Region %zu: [%zu/%zu bytes]%s\n", cur, r->count, r->capacity, selected);
        r = r->next;
		++cur;
    }
}
