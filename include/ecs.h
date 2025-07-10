#pragma once

#include "hashmap.h"

typedef uint64_t id_t;
typedef struct ecs_registry_t ecs_registry_t;
typedef struct ecs_storage_t ecs_storage_t;
typedef void (*pfn_ecs_iter_component_callback)(ecs_registry_t*, id_t, void*);

struct ecs_registry_t {
	hashmap_t storage_map;
	id_t      next_id;
};

void ecs_init(ecs_registry_t* reg);
void ecs_cleanup(ecs_registry_t* reg);
bool ecs_register_component(ecs_registry_t* reg, uint32_t component_size, id_t component_id, uint32_t init_count);
void* ecs_component_storage(ecs_registry_t* reg, id_t component_id);
uint32_t ecs_component_size(ecs_registry_t* reg, id_t component_id);
id_t ecs_new_entity(ecs_registry_t* reg);
void* ecs_add_component(ecs_registry_t* reg, id_t entity_id, id_t component_id);
bool ecs_has_component(ecs_registry_t* reg, id_t entity_id, id_t component_id);
void* ecs_get_component(ecs_registry_t* reg, id_t entity_id, id_t component_id);
void ecs_iter_component(ecs_registry_t* reg, id_t component_id, pfn_ecs_iter_component_callback callback);

/*     Hashing     */
#define HASH64_VALUE (0xcbf29ce484222325)
#define HASH64_PRIME (0x100000001b3)

inline uint64_t hash_str(const char* const str, const uint64_t value) {
	return (str[0] == '\0') ? value : hash_str(&str[1], (value ^ (uint64_t)((uint8_t)str[0])) * HASH64_PRIME);
}

#define hash(str)  (hash_str(str, HASH64_VALUE))
