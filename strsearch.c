/*
 * String search implementation via hashing.
 * Copyright (C) 2015 Anton Belyy.
 *
 * Database file is mmap-ed into address space, allowing OS to efficiently manage memory, e.g. by loading
 * "hot" parts of database into memory when needed, and also making it possible to modify database on the fly.
 *
 * Hash table is represented as `htable` array of size N (N = number of lines in db), where each element either points
 * to the end of a single-linked list or is equal to zero. Lists are stored in pre-allocated array `clist` of size N.
 *
 * This implementation is very memory-efficient and cache-friendly, requiring only 12N + O(1) bytes of "real" memory
 * (which can be smaller than the size of database!), sacrificing however for request processing speed,
 * which is O(req_size) in most cases, but may be up to O(db_size) due to hash collision.
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
    uint32_t ptr;
};

struct collision_list {
    uint32_t fpos;
    uint32_t len;
    uint32_t prev;
};

struct compr_collision_list {
    uint32_t fpos;
    uint32_t len;
};

typedef struct htable_entry htable_entry_t;
typedef struct collision_list collision_list_t;
typedef struct compr_collision_list compr_collision_list_t;

char * dict;
htable_entry_t * htable;
collision_list_t * clist;
compr_collision_list_t * cclist;
size_t num_of_bytes, num_of_lines;
size_t num_of_buckets;
int clist_size = 1;

static uint32_t hasher(char * s, size_t len) {
    uint32_t hash = 5381;
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + (s[i] - 32);
    }
    return hash % num_of_buckets;
}

static void htable_insert(size_t fpos, size_t len) {
    uint32_t hash = hasher(dict + fpos, len);
    clist[clist_size].fpos = fpos;
    clist[clist_size].len = len;
    clist[clist_size].prev = htable[hash].ptr;
    htable[hash].ptr = clist_size;
    ++clist_size;
}

static int htable_lookup(char * s, size_t len) {
    uint32_t hash = hasher(s, len);
    uint32_t ptr = htable[hash].ptr;
    uint32_t nextptr = htable[hash + 1].ptr;
    for (; ptr < nextptr; ptr++) {
        if (len == cclist[ptr].len) {
            if (!memcmp(s, dict + cclist[ptr].fpos, len)) {
                // Totally present.
                return 1;
            }
        }
    }
    // That was just a collision...
    return 0;
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
    htable = calloc(num_of_buckets + 1, sizeof(htable_entry_t));
    // We enumerate elements of clist from 1 so that htable[hash].ptr == 0 would mean that no values with hash `hash` are present.
    clist = malloc((num_of_lines + 1) * sizeof(collision_list_t));

    size_t prev_start = 0;
    for (size_t i = 0; i < num_of_bytes; i++) {
        if (dict[i] == '\n') {
            // Add line to htable and clist.
            htable_insert(prev_start, i + 1 - prev_start);
            prev_start = i + 1;
        }
    }

    // Compress collision list and update ptrs in htable accordingly.
    htable[num_of_buckets].ptr = num_of_lines;
    cclist = malloc((num_of_lines + 1) * sizeof(compr_collision_list_t));
    size_t clines = 0;
    for (size_t i = 0; i < num_of_buckets; i++) {
        uint32_t ptr = htable[i].ptr;
        htable[i].ptr = clines;
        while (ptr != 0) {
            cclist[clines].fpos = clist[ptr].fpos;
            cclist[clines].len = clist[ptr].len;
            ptr = clist[ptr].prev;
            clines++;
        }
    }
    free(clist);

    // Ready to accept requests.
    char * req_buf = malloc(MAX_REQUEST_SIZE);

    while (!feof(stdin)) {
        // Read request.
        fgets(req_buf, MAX_REQUEST_SIZE, stdin);
        size_t len = strlen(req_buf);
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
    free(cclist);
    munmap(dict, num_of_bytes);
    close(dictfd);

    return 0;
}
