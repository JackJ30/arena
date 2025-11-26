#ifndef _ARENA_H_
#define _ARENA_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef struct ArenaRegion ArenaRegion;
struct ArenaRegion {
    ArenaRegion *next;
	uint8_t* ptr;
	uint8_t* end;
    uint8_t data[];
};

typedef struct {
    ArenaRegion *first, *current;
	size_t default_region_size;
} Arena;

typedef struct  {
	Arena* arena;
    ArenaRegion *region;
	uint8_t* ptr;
} ArenaMark;

// base functions
void arena_create(Arena *a, size_t default_region_size);
void arena_destroy(Arena *a);

// allocations
void *arena_alloc(Arena *a, size_t size, size_t align);
void *arena_alloc_zero(Arena *a, size_t size, size_t align);
void *arena_realloc(Arena *a, void *oldptr, size_t oldsz, size_t newsz, size_t align);
void arena_reset(Arena *a);
void arena_trim(Arena *a);

// alloc macros
#define arena_alloc_array(arena, type, count) (type *)arena_alloc((arena), sizeof(type)*(count), _Alignof(type))
#define arena_alloc_array_zero(arena, type, count) (type *)arena_alloc_zero((arena), sizeof(type)*(count), _Alignof(type))
#define arena_alloc_struct(arena, type) arena_alloc_array((arena), (type), 1)
#define arena_alloc_struct_zero(arena, type) arena_alloc_array_zero((arena), (type), 1)

// strings helpers
char *arena_strdup(Arena *a, const char *cstr);
char *arena_sprintf(Arena *a, const char *format, ...) __attribute__((format(printf, 2, 3)));
char *arena_vsprintf(Arena *a, const char *format, va_list args);

// mark system
ArenaMark arena_mark(Arena *a);
void arena_rewind(ArenaMark m);

// debug
void arena_debug_stats(Arena *a, size_t *allocated, size_t* used, size_t* wasted);
void arena_debug_print(Arena *a);

// scratch system
ArenaMark get_scratch_arena(Arena** conflicting, int num_conflicting);

#endif // _ARENA_H_
