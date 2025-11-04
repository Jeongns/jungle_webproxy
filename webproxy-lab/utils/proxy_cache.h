#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_URI_LEN 1024
#define HASH_SIZE 211

typedef struct _cache_block {
    char uri[MAX_URI_LEN];
    char* data;
    int datasize;
    struct _cache_block* hash_next;
    struct _cache_block *next, *prev;
} cache_block;



cache_block* get_cache(const char* key);
void insert_cache(const char* key, const char* value);
void remove_cache(cache_block* cache);
static int string_hash(const char* s);
static cache_block* insert_hash(const char* key, const char* value);
static void remove_hash(const char* key);
static void iter_hash(const char* key, cache_block** prev_out, cache_block** cur_out);
static void lru_detach(cache_block* node);
static void lru_insert_front(cache_block* node);
static void lru_move_to_front(cache_block* node);