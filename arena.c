#include "arena.h"

// using libc malloc backend

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

ArenaRegion *new_region(size_t capacity) {
    size_t size = sizeof(ArenaRegion) + capacity;
    ArenaRegion *r = (ArenaRegion*)malloc(size);
    assert(r);
    r->next = NULL;
    r->ptr = r->data;
    r->end = r->data + capacity;
    return r;
}

void free_region(ArenaRegion *r) {
    free(r);
}

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

void *arena_alloc(Arena *a, size_t size, size_t align) {
	assert(a->first != NULL && a->current != NULL);
	assert((align & (align - 1)) == 0); // alignment must be power of 2

	while (true) {

		// if fits in this region (with alignment)
		uintptr_t aligned = ((uintptr_t)a->current->ptr + (align - 1)) & ~(uintptr_t)(align - 1);
		uint8_t* next = (uint8_t*)(aligned + size);
		if (next <= a->current->end) {
			// allocate it
            a->current->ptr = next;
            return (void*)aligned;
		}

		// move or allocate new region if at the end
        if (a->current->next) {
            a->current = a->current->next;
        } else {
			// allocate a new region big enough to hold the allocation (with alignment)
            size_t cap = a->default_region_size;
            if (cap < align + size - 1) cap = align + size - 1;
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

	// find the region that oldptr is in
	ArenaRegion *r = a->first;
	while (r != NULL) {
		if ((uint8_t*)oldptr >= r->data && (uint8_t*)oldptr < r->end) {
			// found the region
			assert((uint8_t*)oldptr + oldsz <= r->ptr);
			break;
		}
		r = r->next;
	}
	assert(r != NULL); // oldptr must be in a region

	if (((uintptr_t)oldptr % align) == 0) {
		// the alignment on the old pointer is still okay, so we can attempt to re-use it

		// check if we are at the end of the stack
		if ((uint8_t*)oldptr + oldsz == r->ptr) {
			// check if we are shrinking or not
			if (newsz <= oldsz) {
				r->ptr = (uint8_t*)oldptr + newsz;
				return oldptr;
			} else if ((uint8_t*)oldptr + newsz <= r->end) {
				// grow stack if it fits in region
				r->ptr = (uint8_t*)oldptr + newsz;
                return oldptr;
			}
		} else if (newsz <= oldsz) {
			// if we aren't at the end of the stack and we are shrinking, do nothing
			return oldptr;
		}
	}

	// if all else, reallocate and copy
	void *newptr = arena_alloc(a, newsz, align);
	memcpy(newptr, oldptr, oldsz < newsz ? oldsz : newsz);
	return newptr;
}


void arena_reset(Arena *a) {
	assert(a->first != NULL && a->current != NULL);

    for (ArenaRegion *r = a->first; r != NULL; r = r->next) {
        r->ptr = r->data;
    }

    a->current = a->first;
}

void arena_trim(Arena *a) {
	assert(a->first != NULL && a->current != NULL);

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
	m.arena  = a;
	m.region = a->current;
	m.ptr    = a->current->ptr;

    return m;
}

void arena_rewind(ArenaMark m) {
	assert(m.arena != NULL && m.region != NULL && m.arena->first != NULL && m.arena->current != NULL);

    m.region->ptr = m.ptr;
    for (ArenaRegion *r = m.region->next; r != NULL; r = r->next) {
        r->ptr = r->data;
    }

    m.arena->current = m.region;
}

// scratch
#define NUM_SCRATCH_ARENAS 2
_Thread_local
Arena scratch_pool[NUM_SCRATCH_ARENAS] = {0};
_Thread_local
bool scratch_initialized = false;

void init_scratch(size_t region_size) {
	for (size_t i = 0; i < NUM_SCRATCH_ARENAS; ++i) {
		arena_create(&scratch_pool[i], region_size);
	}
	scratch_initialized = true;
}

void deinit_scratch() {
	for (size_t i = 0; i < NUM_SCRATCH_ARENAS; ++i) {
		arena_destroy(&scratch_pool[i]);
	}
	scratch_initialized = false;
}

ArenaMark get_scratch_arena(Arena** conflicting, size_t num_conflicting) {
	if (!scratch_initialized) {
		init_scratch(DEFAULT_SCRATCH_REGION_SIZE);
	}

    for (int i = 0; i < NUM_SCRATCH_ARENAS; i++) {
        bool is_conflicting = false;
        for (size_t z = 0; z < num_conflicting; z++) {
            if (&scratch_pool[i] == conflicting[z]) {
                is_conflicting = true;
            }
        }

        if (!is_conflicting) {
            return arena_mark(&scratch_pool[i]);
        }
    }

	assert(false); // should be unreachable unless something is wrong
    return (ArenaMark){NULL, NULL, NULL};
}

// debug
void arena_debug_stats(Arena *a, size_t *allocated, size_t* used, size_t* wasted) {
	assert(a->first != NULL && a->current != NULL);

	if (allocated) *allocated = 0;
	if (used) *used = 0;
	if (wasted) *wasted = 0;

	bool reached_current = false;
    ArenaRegion *r = a->first;
    while (r) {
		if (r == a->current) reached_current = true;

		if (allocated) *allocated += r->end - r->data;
		if (used) *used += r->ptr - r->data;
		if (wasted && !reached_current) *wasted += r->end - r->ptr;

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
		printf("Region %zu: [%zu/%zu bytes]%s\n", cur, r->ptr - r->data, r->end - r->data, selected);
        r = r->next;
		++cur;
    }
}
