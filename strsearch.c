/*
 * String search implementation via hashing.
 * Copyright (C) 2015 Anton Belyy.
 *
 * Database file is mmap-ed into address space, allowing OS to efficiently manage memory,e.g. by loading
 * "hot" parts of database into memory when needed, and also making it possible to modify database on the fly.
 *
 * Hash table is represented as `htable` array of size N (N = number of lines in db), where each element either points
 * to the beginning of a single-linked list or is equal to zero. Lists are stored in pre-allocated array `clist` of size N.
 * To support O(1) insertion we also store pointer to the end of a single-linked list.
 *
 * This implementation is very memory-efficient, requiring only 16N + O(1) bytes of "real" memory (which can be
 * smaller than the size of database!), sacrificing however for request processing speed, which is O(req_size) in most
 * cases, but may be up to O(db_size) due to hash collision.
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>

// 2 additional bytes for '\n' and '\0'.
#define MAX_REQUEST_SIZE (128 * 1024 * 1024 + 2)

struct htable_entry {
    uint32_t first;
    uint32_t last;
};

struct collision_list {
    uint32_t fpos;
    uint32_t next;
};

typedef struct htable_entry htable_entry_t;
typedef struct collision_list collision_list_t;

char * dict;
htable_entry_t * htable;
collision_list_t * clist;
size_t num_of_bytes, num_of_lines;
size_t num_of_buckets;
int clist_size = 1;

uint32_t hasher(char * s, int len) {
    uint32_t hash = 5381;
    for (int i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + (s[i] - 32);
    }
    return hash % num_of_buckets;
}

void htable_insert(int fpos, size_t len) {
    uint32_t hash = hasher(dict + fpos, len);
    clist[clist_size].fpos = fpos;
    if (htable[hash].first == 0) {
        // No collisions with previous lines.
        htable[hash].first = clist_size;
    } else {
        // Collision detected.
        clist[htable[hash].last].next = clist_size;
    }
    htable[hash].last = clist_size;
    ++clist_size;
}

int htable_lookup(char * s, size_t len) {
    uint32_t hash = hasher(s, len);
    uint32_t pos = htable[hash].first;
    if (pos == 0) {
        // Definitely not present.
        return 0;
    } else {
        // Maybe present, maybe not. Traverse clist to find out.
        do {
            size_t fpos = clist[pos].fpos;
            if (!memcmp(s, dict + fpos, len)) {
                // Totally present.
                return 1;
            }
            pos = clist[pos].next;
        } while (pos != 0);
        // That was just a collision...
        return 0;
    }
}

int main(int argc, char ** argv) {
    // Check args.
    if (argc != 2) {
        printf("usage: %s /path/to/db\n", argv[0]);
        return 2;
    }

    // Map dictionary file directly to memory for easier access from program.
    int dictfd;
    if ((dictfd = open(argv[1], O_RDONLY)) < 0) {
        perror("open");
        return 1;
    }

    struct stat st;
    fstat(dictfd, &st);
    num_of_bytes = st.st_size;
    if ((dict = mmap(0, num_of_bytes, PROT_READ, MAP_PRIVATE, dictfd, 0)) == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Count number of lines in file.
    for (size_t i = 0; i < num_of_bytes; i++) {
        if (dict[i] == '\n') {
            num_of_lines++;
        }
    }
    num_of_lines++;

    // Initialize htable and clist.
    num_of_buckets = num_of_lines;
    htable = calloc(num_of_buckets, sizeof(htable_entry_t));
    // We enumerate elements of clist from 1 so that htable[hash].first == 0 would mean that no values with hash `hash` are present.
    clist = calloc(num_of_lines + 1, sizeof(collision_list_t));

    int prev_start = 0;
    for (size_t i = 0; i < num_of_bytes; i++) {
        if (dict[i] == '\n') {
            // Add line to htable and clist.
            htable_insert(prev_start, i + 1 - prev_start);
            prev_start = i + 1;
        }
    }

    // Ready to accept requests.
    char * req_buf = malloc(MAX_REQUEST_SIZE);

    while (!feof(stdin)) {
        // Read request.
        fgets(req_buf, MAX_REQUEST_SIZE, stdin);
        int len = strlen(req_buf);
        // Exit on "exit" command.
        if (!strncmp(req_buf, "exit\n", len)) {
            break;
        }
        // Process request.
        printf(htable_lookup(req_buf, len) ? "YES\n" : "NO\n");
    }

    // Release resources.
    free(req_buf);
    free(htable);
    free(clist);
    munmap(dict, num_of_bytes);
    close(dictfd);

    return 0;
}
