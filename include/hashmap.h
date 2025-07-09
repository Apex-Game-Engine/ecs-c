#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct HashmapT* hashmap_t;
typedef void* hashmap_key_ptr;
typedef void* hashmap_value_ptr;
typedef uint64_t (*hashmap_hash_func)(hashmap_key_ptr);
typedef bool (*hashmap_keyeq_func)(hashmap_key_ptr, hashmap_key_ptr);

hashmap_t hashmap_create(uint32_t key_size, uint32_t value_size, hashmap_hash_func hash_func, hashmap_keyeq_func keyeq_func, uint32_t init_count, hashmap_key_ptr tombstone);

void hashmap_destroy(hashmap_t hashmap);

void hashmap_resize(hashmap_t* phashmap, uint32_t new_count);

hashmap_value_ptr hashmap_emplace(hashmap_t hashmap, hashmap_key_ptr pkey);

bool hashmap_insert(hashmap_t hashmap, hashmap_key_ptr pkey, hashmap_value_ptr pvalue);

bool hashmap_erase(hashmap_t hashmap, hashmap_key_ptr key, hashmap_value_ptr value);

hashmap_value_ptr hashmap_find(hashmap_t hashmap, hashmap_key_ptr pkey);

uint32_t hashmap_iternext(hashmap_t hashmap, uint32_t index, hashmap_key_ptr* ppkey, hashmap_value_ptr* ppvalue);
