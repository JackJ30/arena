#include "arena.h"

#include <stdio.h>

void test_general() {
	// test at many different arena block sizes
	Arena a = {0};
	arena_create(&a, 4000000);

	char* str = arena_strdup(&a, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur nisi.");
	char* str2 = arena_strdup(&a, "n");
	ArenaMark mark = arena_mark(&a);
	str2 = arena_strdup(&a, "nothing");
	str2 = arena_strdup(&a, "nothina askjdasknjdansd askdjaslnkajsdlnfkajdsfasd fsakdjlbflkdsajbfasdf asdfasdf");
	str2 = arena_strdup(&a, "nothina askjdasknjdansd askdjaslnkajsdlnfkajdsfasd fsakdjlbflkdsajbfasdf asdfasdf");
	printf("%s\n", str);
	printf("%s\n", str2);
	arena_debug_print(&a);
	arena_rewind(mark);
	arena_debug_print(&a);
	arena_alloc_array(&a, int, 20);
	arena_debug_print(&a);
	arena_temp_scratch(scratch, NULL, 0) {
		for (int i = 0; i < 100; ++i) {
			str2 = arena_strdup(scratch, "nothina askjdasknjdansd askdjaslnkajsdlnfkajdsfasd fsakdjlbflkdsajbfasdf asdfasdf");
		}
		arena_debug_print(scratch);
	}
	arena_temp_scratch(scratch, NULL, 0) {
		arena_debug_print(scratch);
	}
	arena_debug_print(&a);

	arena_destroy(&a);
	deinit_scratch();

	arena_create(&a, 500);
	ArenaMark m = arena_mark(&a);
	arena_alloc(&a, 1000, 1);
	arena_debug_print(&a);
	arena_rewind(m);
	arena_alloc(&a, 50, 1); // SHOULD use region #0 again
	arena_debug_print(&a);
}

void test_regions(); // test designed to exploit region boundaries
void test_mark(); // test designed to exploit mark
void test_test_scratch(); // test designed to exploit scratch
void test_strings();
// maybe realloc tests
// maybe multithreading tests

int main() {
	printf("1 ===== Running general tests...\n");
	test_general();
}
