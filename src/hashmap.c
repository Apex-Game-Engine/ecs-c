#include "hashmap.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define _malloc(nbytes) 	(printf("[malloc] %8zu bytes\n", nbytes), calloc(nbytes, 1))
#define _free(ptr) 			(printf("[free]\n"), free(ptr))

#if HASHMAP_DEBUG
#	define hashmap_debugf(fmt, ...)    (printf("[hashmap] (%s:%d) :: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__))
#else
#	define hashmap_debugf(fmt, ...)
#endif

typedef struct HashmapT {
	uint32_t key_size;
	uint32_t value_size;
	uint32_t bucket_count;
	uint32_t count;
	hashmap_hash_func hash_func;
	hashmap_keyeq_func keyeq_func;
	char buckets[];
} HashmapT;

static bool getkv(hashmap_t hashmap, uint32_t index, hashmap_key_ptr* pkey, hashmap_value_ptr* pvalue) {
	char* kvpair = ((char*)hashmap->buckets) + hashmap->key_size + ((hashmap->key_size + hashmap->value_size) * index);
	*pkey = (void*)kvpair;
	*pvalue = (void*)(kvpair + hashmap->key_size);
	return true;
}

// https://stackoverflow.com/a/28563801
static bool memvcmp(void *memory, unsigned char val, unsigned int size) {
    unsigned char *mm = (unsigned char*)memory;
    return (*mm == val) && memcmp(mm, mm + 1, size - 1) == 0;
}

static bool iskeyempty(hashmap_t hashmap, hashmap_key_ptr pkey) {
	return memvcmp(pkey, 0, hashmap->key_size);
}

static bool iskeytombstone(hashmap_t hashmap, hashmap_key_ptr pkey) {
	return memcmp(pkey, hashmap->buckets, hashmap->key_size) == 0;
}

hashmap_t hashmap_create(uint32_t key_size, uint32_t value_size, hashmap_hash_func hash_func, hashmap_keyeq_func keyeq_func, uint32_t init_count, hashmap_key_ptr tombstone) {
	hashmap_t hashmap = (hashmap_t)_malloc(sizeof(HashmapT) + key_size + (key_size + value_size) * init_count);
	hashmap->key_size = key_size;
	hashmap->value_size = value_size;
	hashmap->bucket_count = init_count;
	hashmap->count = 0;
	hashmap->hash_func = hash_func;
	hashmap->keyeq_func = keyeq_func;
	memcpy(hashmap->buckets, tombstone, key_size);
	return hashmap;
}

void hashmap_destroy(hashmap_t hashmap) {
	_free(hashmap);
}

void hashmap_resize(hashmap_t* phashmap, uint32_t new_count) {
	hashmap_t old_hashmap = *phashmap;
	hashmap_t new_hashmap;
	if (new_count <= old_hashmap->bucket_count) {
		return;
	}
	new_hashmap = hashmap_create(
		old_hashmap->key_size,
		old_hashmap->value_size,
		old_hashmap->hash_func,
		old_hashmap->keyeq_func,
		new_count,
		old_hashmap->buckets
	);

	for (uint32_t index = 0; index < old_hashmap->bucket_count; index++) {
		hashmap_key_ptr pkey; hashmap_value_ptr pvalue;
		getkv(old_hashmap, index, &pkey, &pvalue);
		if (iskeyempty(old_hashmap, pkey) || iskeytombstone(old_hashmap, pkey)) {
			continue;
		}
		hashmap_insert(new_hashmap, pkey, pvalue);
	}
}

bool hashmap_insert(hashmap_t hashmap, hashmap_key_ptr key, hashmap_value_ptr value) {
	hashmap_value_ptr pvalue = hashmap_emplace(hashmap, key);
	if (pvalue) {
		memcpy(pvalue, value, hashmap->value_size);
	}
	return pvalue != NULL;
}

hashmap_value_ptr hashmap_emplace(hashmap_t hashmap, hashmap_key_ptr key) {
	uint64_t hash = hashmap->hash_func(key);
	uint32_t index = hash % hashmap->bucket_count;
	uint32_t ideal_index = index;
	hashmap_key_ptr pkey; hashmap_value_ptr pvalue;
	getkv(hashmap, index, &pkey, &pvalue);
	hashmap_debugf("try insert @ %u", index);
	while (!iskeyempty(hashmap, pkey) && !iskeytombstone(hashmap, pkey)) {
		if (hashmap->keyeq_func(pkey, key)) {
			hashmap_debugf("found existing @ %u", index);
			return NULL;
		}
		index = (index + 1) % hashmap->bucket_count;
		hashmap_debugf("try insert @ %u", index);
		if (index == ideal_index) {
			return NULL;
		}
		getkv(hashmap, index, &pkey, &pvalue);
	}
	hashmap_debugf("inserted @ %u", index);
	memcpy(pkey, key, hashmap->key_size);
	return pvalue;
}

void* hashmap_find(hashmap_t hashmap, hashmap_key_ptr key) {
	uint64_t hash = hashmap->hash_func(key);
	uint32_t index = hash % hashmap->bucket_count;
	uint32_t ideal_index = index;
	hashmap_key_ptr pkey; hashmap_value_ptr pvalue;
	getkv(hashmap, index, &pkey, &pvalue);
	while (!iskeyempty(hashmap, pkey)) {
		hashmap_debugf("check key @ %u", index);
		if (iskeytombstone(hashmap, pkey)) {
			continue;
		}
		if (hashmap->keyeq_func(pkey, key)) {
			hashmap_debugf("found key @ %u", index);
			return pvalue;
		}
		index = (index + 1) % hashmap->bucket_count;
		if (index == ideal_index) {
			return NULL;
		}
		getkv(hashmap, index, &pkey, &pvalue);
	}
	return NULL;
}

bool hashmap_erase(hashmap_t hashmap, hashmap_key_ptr key, hashmap_value_ptr value) {
	uint64_t hash = hashmap->hash_func(key);
	uint32_t index = hash % hashmap->bucket_count;
	uint32_t ideal_index = index;
	hashmap_key_ptr pkey; hashmap_value_ptr pvalue;
	getkv(hashmap, index, &pkey, &pvalue);
	while (!iskeyempty(hashmap, pkey)) {
		if (iskeytombstone(hashmap, pkey)) {
			continue;
		}
		if (hashmap->keyeq_func(pkey, key)) {
			break;
		}
		index = (index + 1) % hashmap->bucket_count;
		if (index == ideal_index) {
			return false;
		}
		getkv(hashmap, index, &pkey, &pvalue);
	}
	memcpy(pkey, hashmap->buckets, hashmap->key_size);
	if (value) {
		memcpy(value, pvalue, hashmap->value_size);
	}
	return true;
}


uint32_t hashmap_iternext(hashmap_t hashmap, uint32_t index, hashmap_key_ptr* ppkey, hashmap_value_ptr* ppvalue) {
	hashmap_key_ptr pkey; hashmap_value_ptr pvalue;
	getkv(hashmap, index, &pkey, &pvalue);
	while (iskeyempty(hashmap, pkey) || iskeytombstone(hashmap, pkey)) {
		index = index + 1;
		if (index == hashmap->bucket_count) {
			return -1;
		}
		getkv(hashmap, index, &pkey, &pvalue);
	}
	*ppkey = pkey; *ppvalue = pvalue;
	return index + 1;
}
