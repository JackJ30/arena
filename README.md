# Growable Arena/Stack Allocator for C
This is an implementation of the arena allocator described in [Ryan Fleury's article](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator).
- It is dynamically growable (using a linked list of blocks).
- Use this library by copying `arena.h` and `arena.c` into your source tree. You can also grab any of the data structure implementations that you want.
- It was originally based on [Tsoding's single header arena](https://github.com/tsoding/arena). 
- It depends on libc and uses malloc to allocate regions.

You must explicitly initialize the arena with a size. You should pick an amount which your program will never reach, but this arena will still work if you exceed it.

## Todo
 - goofy ahh scope based cleanup macros for temp/scratch 
 - better tests
 - asan poisoning
 - data structures (dynamic array, pointer stable dynamic array (non continous), hash trie)
 - debug allocation tracking
 
 Possible improvement - thread safe scratch sytem that automatically returns scratch arenas depending if they are "in use" or not (could potentially solve the conflicting arena problem and not force each thread to initialize scratch)
