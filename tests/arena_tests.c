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
	arena_destroy(&a);
}

void test_regions(); // test designed to exploit region boundaries
void test_mark(); // test designed to exploit mark
void test_test_scratch(); // test designed to exploit scratch
void test_strings();

int main() {
	printf("1 ===== Running general tests...\n");
	test_general();
}
