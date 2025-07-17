#include "ecs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#if ECS_DEBUG
#	define ecs_debugf(fmt, ...) DEBUG_CHANNEL(ecs, fmt, ##__VA_ARGS__)
#else
#	define ecs_debugf(fmt, ...)
#endif

/********************************************************************
 * Utility Functions
 *******************************************************************/
void memzero(void* mem, size_t size) {
	memset(mem, 0, size);
}

void memswp(void* dst, void* src, size_t size) {
	char tmp[size];
	memcpy(tmp, src, size);
	memcpy(src, dst, size);
	memcpy(dst, tmp, size);
}

/********************************************************************
 * Sparse Set Implementation
 *******************************************************************/

struct ecs_ss_t {
	uint32_t 	sparse_size; 	// reserved count in sparse array
	uint32_t 	dense_size;		// reserved count in dense array
	uint32_t 	slot_size;		// byte size of each slot
	uint32_t 	slot_count; 	// no. of slots in use
	uint32_t* 	sparse;			// sparse array of indices
	ecs_id_t* 	dense_ids;		// dense array of ids
	void* 		dense_slots;	// dense array of slots
};

void ecs_ss_create_ex(ecs_ss_t* ss, uint32_t slot_size, uint32_t sparse_size, uint32_t dense_size) {
	ss->sparse_size = sparse_size;
	ss->dense_size = dense_size;
	ss->slot_size = slot_size;
	ss->slot_count = 0;
	ss->sparse = malloc(sizeof(uint32_t) * sparse_size);
	ss->dense_ids = malloc(sizeof(ecs_id_t) * dense_size);
	ss->dense_slots = malloc(slot_size * dense_size);
	memset(ss->sparse, -1, sizeof(uint32_t) * sparse_size);
}

ecs_ss_t* ecs_ss_create(uint32_t slot_size, uint32_t sparse_size, uint32_t dense_size) {
	ecs_ss_t* ss = malloc(sizeof(ecs_ss_t));
	ecs_ss_create_ex(ss, slot_size, sparse_size, dense_size);
	return ss;
}

void ecs_ss_destroy(ecs_ss_t* ss) {
	free(ss->sparse);
	free(ss->dense_ids);
	free(ss->dense_slots);
	free(ss);
}

bool ecs_ss_has(ecs_ss_t* ss, ecs_id_t entity_id) {
	if (entity_id.id >= ss->sparse_size) {
		return false;
	}
	uint32_t index;
	index = ss->sparse[entity_id.id];
	return ECS_NULL != index;
}

#define ecs_ss_slotbyidx(ss, idx) (((uint8_t*)((ss)->dense_slots)) + ((ss)->slot_size * (idx)))

ecs_ss_slot_t ecs_ss_get(ecs_ss_t* ss, ecs_id_t entity_id) {
	ecs_ss_slot_t slot = { NULL };
	if (entity_id.id >= ss->sparse_size) {
		ecs_debugf("out of bounds");
		return slot;
	}
	uint32_t index;
	index = ss->sparse[entity_id.id];
	if (ECS_NULL == index) {
		ecs_debugf("not found");
		return slot;
	}
	assert(ss->dense_ids[index].id == entity_id.id);
	slot.data = ecs_ss_slotbyidx(ss, index);
	return slot;
}

int ecs_ss_emplace(ecs_ss_t* ss, ecs_id_t entity_id, ecs_ss_slot_t* pslot) {
	if (entity_id.id >= ss->sparse_size) {
		return ECS_INVALID_ARG;
	}
	uint32_t index;
	index = ss->sparse[entity_id.id];
	if (ECS_NULL != index) {
		assert(ss->dense_ids[index].id == entity_id.id);
		pslot->data = ecs_ss_slotbyidx(ss, index);
		return ECS_FOUND;
	}
	if (ss->slot_count >= ss->dense_size) {
		return ECS_OUT_OF_MEMORY;
	}
	index = ss->slot_count++;
	ss->sparse[entity_id.id] = index;
	ss->dense_ids[index] = entity_id;
	pslot->data = ecs_ss_slotbyidx(ss, index);
	return ECS_OK;
}

int ecs_ss_insert(ecs_ss_t* ss, ecs_id_t entity_id, ecs_ss_slot_t slot) {
	ecs_ss_slot_t dst_slot;
	ecs_ss_emplace(ss, entity_id, &dst_slot);
	memcpy(dst_slot.data, slot.data, ss->slot_size);
	return ECS_OK;
}

int ecs_ss_erase(ecs_ss_t* ss, ecs_id_t entity_id) {
	int res = ecs_ss_pop(ss, entity_id, (ecs_ss_slot_t) { NULL });
	return res;
}

int ecs_ss_pop(ecs_ss_t* ss, ecs_id_t entity_id, ecs_ss_slot_t slot) {
	if (entity_id.id >= ss->sparse_size) {
		return ECS_INVALID_ARG;
	}
	uint32_t index;
	index = ss->sparse[entity_id.id];
	if (ECS_NULL == index) {
		return ECS_NOT_FOUND;
	}
	ss->sparse[entity_id.id] = ECS_NULL;
	uint32_t lastindex = --ss->slot_count;
	ecs_id_t lastid = ss->dense_ids[lastindex];
	ss->dense_ids[index] = lastid;
	ss->sparse[lastid.id] = index;
	if (slot.data) memcpy(slot.data, ecs_ss_slotbyidx(ss, index), ss->slot_size);
	memcpy(ecs_ss_slotbyidx(ss, index), ecs_ss_slotbyidx(ss, lastindex), ss->slot_size);
	return ECS_OK;
}

ecs_id_t ecs_ss_getid(ecs_ss_t* ss, uint32_t idx)
{
	return ss->dense_ids[idx];
}

ecs_ss_slot_t ecs_ss_getslot(ecs_ss_t* ss, uint32_t idx)
{
	return (ecs_ss_slot_t) { ecs_ss_slotbyidx(ss, idx) };
}

/********************************************************************
 * ECS Registry Implementation
 *******************************************************************/
struct ecs_registry_t {
	hashmap_t storage_map;
	ecs_id_t      next_id;
};

static uint64_t id_hash_func(hashmap_key_ptr pkey) {
	return ((ecs_id_t*)pkey)->id;
}

static bool id_keyeq_func(hashmap_key_ptr key1, hashmap_key_ptr key2) {
	return ((ecs_id_t*)key1)->id == ((ecs_id_t*)key2)->id;
}

ecs_registry_t* ecs_init()
{
	ecs_registry_t* reg = malloc(sizeof(ecs_registry_t));
	reg->next_id.id = 0;
	reg->storage_map = hashmap_create(sizeof(ecs_id_t), sizeof(ecs_ss_t), id_hash_func, id_keyeq_func, 200, &(ecs_id_t) { -1 });
	return reg;
}

void ecs_cleanup(ecs_registry_t* reg) {
	hashmap_destroy(reg->storage_map);
}

bool ecs_register_component(ecs_registry_t* reg, uint32_t component_size, ecs_id_t component_id, uint32_t init_count) {
	ecs_ss_t* storage = hashmap_emplace(reg->storage_map, &component_id);
	if (storage) {
		ecs_ss_create_ex(storage, sizeof(ecs_id_t) + component_size, init_count, 1024); // TODO: Handle resizing ids array
		return true;
	}
	return false;
}

ecs_ss_t* ecs_component_storage(ecs_registry_t* reg, ecs_id_t component_id) {
	ecs_ss_t* storage = hashmap_find(reg->storage_map, &component_id);
	return storage;
}

uint32_t ecs_component_size(ecs_registry_t* reg, ecs_id_t component_id) {
	ecs_ss_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return 0;
	}
	return storage->slot_size;
}

ecs_id_t ecs_new_entity(ecs_registry_t* reg) {
	ecs_id_t id = { reg->next_id.id++ };
	ecs_debugf("New entity: %u", id.id);
	return id;
}

void* ecs_add_component(ecs_registry_t* reg, ecs_id_t entity_id, ecs_id_t component_id) {
	ecs_ss_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return NULL;
	}
	ecs_ss_slot_t slot;
	int res = ecs_ss_emplace(storage, entity_id, &slot);
	if (ECS_OK != res) {
		return NULL;
	}
	ecs_debugf("slot.data = %p", slot.data);
	return slot.data;
}

bool ecs_has_component(ecs_registry_t* reg, ecs_id_t entity_id, ecs_id_t component_id) {
	ecs_ss_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return false;
	}
	return ecs_ss_has(storage, entity_id);
}

void* ecs_get_component(ecs_registry_t* reg, ecs_id_t entity_id, ecs_id_t component_id) {
	ecs_ss_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		ecs_debugf("storage NOT found!");
		return NULL;
	}
	ecs_ss_slot_t slot = ecs_ss_get(storage, entity_id);
	ecs_debugf("slot.data = %p", slot.data);
	return slot.data;
}

void ecs_remove_component(ecs_registry_t* reg, ecs_id_t entity_id, ecs_id_t component_id) {
	ecs_ss_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return;
	}
	ecs_ss_erase(storage, entity_id);
}

void ecs_iter_component(ecs_registry_t* reg, ecs_id_t component_id, pfn_ecs_iter_component_func callback) {
	ecs_ss_t* storage = hashmap_find(reg->storage_map, &component_id);
	if (!storage) {
		return;
	}
	for (uint32_t i = 0; i < storage->slot_count; i++) {
		ecs_id_t id = ecs_ss_getid(storage, i);
		ecs_ss_slot_t slot = ecs_ss_getslot(storage, i);
		callback(reg, id, slot.data);
	}
}

void ecs_system_v(ecs_registry_t* reg, pfn_ecs_iter_func func, uint32_t ncomps, va_list vcomps) {
	ecs_id_t comps[ncomps];
	ecs_ss_t* pools[ncomps];
	uint32_t minidx = 0;
	uint32_t mincount = UINT32_MAX;
	for (uint32_t i = 0; i < ncomps; i++) {
		ecs_id_t comp = va_arg(vcomps, ecs_id_t);
		ecs_ss_t* pool = hashmap_find(reg->storage_map, &comp);
		assert(pool);
		comps[i] = comp; pools[i] = pool;
		if (pool->slot_count < mincount) {
			minidx = i; mincount = pool->slot_count;
		}
	}
	if (mincount == 0) {
		return;
	}
	for (uint32_t slot_idx = 0; slot_idx < pools[minidx]->slot_count; slot_idx++) {
		ecs_id_t eid = pools[minidx]->dense_ids[slot_idx];
		bool has_all = true;
		for (uint32_t comp_idx = 0; comp_idx < ncomps; comp_idx++) {
			if (comp_idx == minidx) continue;
			ecs_ss_slot_t slot = ecs_ss_get(pools[comp_idx], eid);
			if (!slot.data) {
				has_all = false; break;
			}
		}
		if (has_all) {
			func(reg, eid, ncomps, comps);
		}
	}
}

void ecs_system(ecs_registry_t* reg, pfn_ecs_iter_func func, uint32_t ncomps, ...) {
	va_list vcomps;
	va_start(vcomps, ncomps);
	ecs_system_v(reg, func, ncomps, vcomps);
	va_end(vcomps);
}
