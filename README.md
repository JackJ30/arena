# Growable Arena/Stack Allocator for C
This is an implementation of the arena allocator described in [Ryan Fleury's article](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator).
- It is dynamically growable (using a linked list of blocks).
- Use this library by copying `arena.h` and `arena.c` into your source tree. You can also grab any of the data structure implementations that you want.
- It was originally based on [Tsoding's single header arena](https://github.com/tsoding/arena). 
- It depends on libc and uses malloc to allocate regions.

You must explicitly initialize the arena with a size. You should pick an amount which your program will never reach, but this arena will still work if you exceed it.

## Todo
 - better tests
 - asan poisoning
 - look into improving scratch system to not need manual conflict managing (also shared-thread scratch pool that can be deinitialized at once)
 - data structures (dynamic array, pointer stable dynamic array (non continous), hash trie)
 - debug allocation tracking (allocations, padding, waste) also track stats like (how many times new_region, how many times region skipped, how many times allocation was larger than default size)
 
 maybe
 - out of memory handling or error/null
 - re add other memory backends, remove libc dependency (maybe virtual memory, but best improvement is page aligned regions)
