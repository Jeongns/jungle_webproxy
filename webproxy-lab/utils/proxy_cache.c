#include "proxy_cache.h"

#include <stdlib.h>
#include <string.h>

int use_cache_size = 0;

cache_block *lru_head = NULL, *lru_tail = NULL;
cache_block* hash_list[HASH_SIZE] = {0};

cache_block* get_cache(const char* key) {
    cache_block* cur = NULL;
    iter_hash(key, NULL, &cur);
    if (cur) lru_move_to_front(cur);
    return cur;
}

void insert_cache(const char* key, const char* value) {
    if (strlen(key) > MAX_URI_LEN || strlen(value) > MAX_OBJECT_SIZE) return;
    use_cache_size += strlen(value);

    while (use_cache_size > MAX_CACHE_SIZE) {
        use_cache_size -= lru_tail->datasize;
        remove_cache(lru_tail);
    }

    cache_block* cache = insert_hash(key, value);

    lru_insert_front(cache);
}

void remove_cache(cache_block* cache) {
    lru_detach(cache);
    remove_hash(cache->uri);
}

static int string_hash(const char* s) {
    unsigned long h = 0;
    while (*s) {
        h = 31 * h + (unsigned char)(*s);
        s++;
    }
    return h % HASH_SIZE;
}

static cache_block* insert_hash(const char* key, const char* value) {
    int hash = string_hash(key);
    cache_block *prev, *cur;
    iter_hash(key, &prev, &cur);
    if (cur == NULL) cur = calloc(1, sizeof(cache_block));
    if (cur->data != NULL) free(cur->data);
    if (prev)
        prev->next = cur;
    else
        hash_list[hash] = cur;

    cur->data = malloc(strlen(value) + 1);
    cur->datasize = strlen(value);
    strcpy(cur->uri, key);
    strcpy(cur->data, value);
    return cur;
}

static void remove_hash(const char* key) {
    cache_block *prev, *cur;
    iter_hash(key, &prev, &cur);
    prev->next = cur->next;
    free(cur->data);
    free(cur);
}

static void iter_hash(const char* key, cache_block** prev_out, cache_block** cur_out) {
    int hash = string_hash(key);
    cache_block* prev = NULL;
    cache_block* cur = hash_list[hash];
    while (cur != NULL) {
        if (!strcmp(cur->uri, key)) break;
        prev = cur;
        cur = cur->hash_next;
    }
    if (prev_out) *prev_out = prev;
    if (cur_out) *cur_out = cur;
}

static void lru_detach(cache_block* node) {
    if (!node) return;

    if (node->prev)
        node->prev->next = node->next;
    else
        lru_head = node->next;

    if (node->next)
        node->next->prev = node->prev;
    else
        lru_tail = node->prev;

    node->prev = node->next = NULL;
}

static void lru_insert_front(cache_block* node) {
    node->prev = NULL;
    node->next = lru_head;
    if (lru_head)
        lru_head->prev = node;
    else
        lru_tail = node;
    lru_head = node;
}

static void lru_move_to_front(cache_block* node) {
    if (node == lru_head) return;
    lru_detach(node);
    lru_insert_front(node);
}