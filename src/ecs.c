#include "ecs.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#if ECS_DEBUG
#	define ecs_debugf(fmt, ...) (printf("[ecs] (%s:%d) :: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__))
#else
#	define ecs_debugf(fmt, ...)
#endif

struct ecs_storage_t {
	uint32_t slot_size;
	uint32_t slot_count;
	uint32_t next_slot;
	id_t     id_count;
	void*    slots;
	id_t*    ids;
};

typedef struct ecs_slot_t {
	id_t     id;
	char     data[];
} ecs_slot_t;

void memswp(void* src, void* dst, size_t sz) {
	char tmp[sz];
	memcpy(tmp, src, sz);
	memcpy(src, dst, sz);
	memcpy(dst, tmp, sz);
}

void storage_create(ecs_storage_t* pool, uint32_t slot_size, uint32_t slot_count, id_t id_count) {
	pool->slot_size = slot_size;
	pool->slot_count = slot_count;
	pool->next_slot = 0;
	pool->id_count = id_count;
	pool->slots = calloc(pool->slot_count, pool->slot_size);
	pool->ids = (id_t*)malloc(pool->id_count * sizeof(id_t));
	memset(pool->ids, -1, pool->id_count * sizeof(id_t));
	ecs_debugf(" slot_size: %u | slot_count: %u | id_count: %lu", pool->slot_size, pool->slot_count, pool->id_count);
}

bool storage_resize_ids(ecs_storage_t* pool, id_t id_count) {
	if (id_count <= pool->id_count) {
		return false;
	}
	id_t* new_ids = (id_t*)malloc(id_count * sizeof(id_t));
	memcpy(new_ids, pool->ids, pool->id_count * sizeof(id_t));
	memset(new_ids + pool->id_count, -1, (id_count - pool->id_count) * sizeof(id_t));
	free(pool->ids);
	pool->ids = new_ids;
	pool->id_count = id_count;
	return true;
}

bool storage_resize_slots(ecs_storage_t* pool, uint32_t count) {
	if (count <= pool->slot_count) {
		return false;
	}
	void* new_slots = calloc(count, pool->slot_size);
	memcpy(new_slots, pool->slots, pool->slot_count * pool->slot_size);
	free(pool->slots);
	pool->slots = new_slots;
	pool->slot_count = count;
	return true;
}

void* storage_slot_byindex(ecs_storage_t* pool, uint32_t index) {
	return ((char*)pool->slots) + pool->slot_size * index;
}

void* storage_slot_byid(ecs_storage_t* pool, id_t id) {
	if (id >= pool->id_count) {
		return NULL;
	}
	uint32_t index = pool->ids[id];
	if (index != (uint32_t)(-1)) {
		return storage_slot_byindex(pool, index);
	}
	return NULL;
}

bool storage_emplace(ecs_storage_t* pool, id_t id, void** pslot) {
	void* slot = storage_slot_byid(pool, id);
	if (slot) {
		*pslot = slot;
		return false;
	}

	if (pool->next_slot < pool->slot_count) {
		uint32_t index = pool->next_slot++;
		pool->ids[id] = index;
		*pslot = storage_slot_byindex(pool, index);
		return true;
	}

	return false;
}

bool storage_delete(ecs_storage_t* pool, id_t id) {
	if (id >= pool->id_count) {
		return false;
	}
	uint32_t index = pool->ids[id];
	if (index == (uint32_t)(-1) || index >= pool->next_slot) {
		return false;
	}
	if (index < pool->next_slot - 1) {
		// memswp(storage_slot_byindex(pool, index), storage_slot_byindex(pool, pool->next_slot - 1), pool->slot_size);
		memcpy(storage_slot_byindex(pool, index), storage_slot_byindex(pool, pool->next_slot - 1), pool->slot_size);
	}
	pool->next_slot--;
	return true;
}

/*    ECS Registry    */
static uint64_t id_hash_func(hashmap_key_ptr pkey) {
	return *(id_t*)pkey;
}

static bool id_keyeq_func(hashmap_key_ptr key1, hashmap_key_ptr key2) {
	return *(id_t*)key1 == *(id_t*)key2;
}

void ecs_init(ecs_registry_t* reg) {
	reg->next_id = 0;
	reg->storage_map = hashmap_create(sizeof(id_t), sizeof(ecs_storage_t), id_hash_func, id_keyeq_func, 200, &(id_t) { -1 });
}

void ecs_cleanup(ecs_registry_t* reg) {
	hashmap_destroy(reg->storage_map);
}

bool ecs_register_component(ecs_registry_t* reg, uint32_t component_size, id_t component_id, uint32_t init_count) {
	ecs_storage_t* storage = hashmap_emplace(reg->storage_map, &component_id);
	if (storage) {
		storage_create(storage, sizeof(id_t) + component_size, init_count, (reg->next_id + 1) * 2);
		return true;
	}
	return false;
}

void* ecs_component_storage(ecs_registry_t* reg, id_t component_id) {
	ecs_storage_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return NULL;
	}
	return storage_slot_byindex(storage, 0);
}

uint32_t ecs_component_size(ecs_registry_t* reg, id_t component_id) {
	ecs_storage_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return 0;
	}
	return storage->slot_size;
}

id_t ecs_new_entity(ecs_registry_t* reg) {
	id_t id = reg->next_id++;
	ecs_debugf("New entity: %lu", id);
	return id;
}

void* ecs_add_component(ecs_registry_t* reg, id_t entity_id, id_t component_id) {
	ecs_storage_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return NULL;
	}
	ecs_slot_t* slot;
	bool res = storage_emplace(storage, entity_id, (void**)&slot);
	if (!res) {
		return NULL;
	}
	slot->id = entity_id;
	return slot->data;
}

bool ecs_has_component(ecs_registry_t* reg, id_t entity_id, id_t component_id) {
	ecs_storage_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return false;
	}
	ecs_slot_t* slot = (ecs_slot_t*)storage_slot_byid(storage, entity_id);
	if (!slot) {
		return false;
	}
	return true;
}

void* ecs_get_component(ecs_registry_t* reg, id_t entity_id, id_t component_id) {
	ecs_storage_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return NULL;
	}
	ecs_slot_t* slot = (ecs_slot_t*)storage_slot_byid(storage, entity_id);
	if (!slot) {
		return NULL;
	}
	return slot->data;
}

void ecs_remove_component(ecs_registry_t* reg, id_t entity_id, id_t component_id) {
	ecs_storage_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return;
	}
	storage_delete(storage, entity_id);
}

void ecs_iter_component(ecs_registry_t* reg, id_t component_id, pfn_ecs_iter_component_callback callback) {
	ecs_storage_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return;
	}
	for (uint32_t i = 0; i < storage->next_slot; i++) {
		ecs_slot_t* slot = (ecs_slot_t*)storage_slot_byindex(storage, i);
		callback(reg, slot->id, slot->data);
	}
}

