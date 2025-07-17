#pragma once

#include "hashmap.h"

typedef struct ecs_id_t ecs_id_t; /* entity and component id */
typedef struct ecs_ss_t ecs_ss_t; /* sparse set */
typedef struct ecs_ss_slot_t ecs_ss_slot_t; /* sparse set slot */
typedef struct ecs_registry_t ecs_registry_t; /* regsitry */
typedef enum ecs_result_t ecs_result_t;
typedef void (*pfn_ecs_iter_component_func)(ecs_registry_t* reg, ecs_id_t entity, void* comp);
typedef void (*pfn_ecs_iter_func)(ecs_registry_t* reg, ecs_id_t entity, uint32_t ncomps, ecs_id_t* compids);

enum ecs_result_t {
	ECS_ERROR = -1,
	ECS_OK,
	ECS_INVALID_ARG,
	ECS_OUT_OF_MEMORY,
	ECS_FOUND,
	ECS_NOT_FOUND,
};

#define ECS_NULL ((uint32_t)(-1))
#define ECS_NULL_ID ((ecs_id_t) { (uint32_t)(-1) })

struct ecs_id_t { uint32_t id; };
struct ecs_ss_slot_t { void* data; };

ecs_ss_t* 		ecs_ss_create(uint32_t slot_size, uint32_t sparse_size, uint32_t dense_size);
void 			ecs_ss_destroy(ecs_ss_t* ss);

bool 			ecs_ss_has(ecs_ss_t* ss, ecs_id_t entity_id);
ecs_ss_slot_t 	ecs_ss_get(ecs_ss_t* ss, ecs_id_t entity_id);

int 			ecs_ss_emplace(ecs_ss_t* ss, ecs_id_t entity_id, ecs_ss_slot_t* pslot);
int 			ecs_ss_insert(ecs_ss_t* ss, ecs_id_t entity_id, ecs_ss_slot_t slot);
int 			ecs_ss_erase(ecs_ss_t* ss, ecs_id_t entity_id);
int 			ecs_ss_pop(ecs_ss_t* ss, ecs_id_t entity_id, ecs_ss_slot_t slot);

ecs_id_t 		ecs_ss_getid(ecs_ss_t* ss, uint32_t idx);
ecs_ss_slot_t  	ecs_ss_getslot(ecs_ss_t* ss, uint32_t idx);

ecs_registry_t* ecs_init();
void 			ecs_cleanup(ecs_registry_t* reg);
bool 			ecs_register_component(ecs_registry_t* reg, uint32_t component_size, ecs_id_t component_id, uint32_t init_count);
ecs_ss_t*		ecs_component_storage(ecs_registry_t* reg, ecs_id_t component_id);
uint32_t 		ecs_component_size(ecs_registry_t* reg, ecs_id_t component_id);
ecs_id_t 		ecs_new_entity(ecs_registry_t* reg);
void* 			ecs_add_component(ecs_registry_t* reg, ecs_id_t entity_id, ecs_id_t component_id);
bool 			ecs_has_component(ecs_registry_t* reg, ecs_id_t entity_id, ecs_id_t component_id);
void* 			ecs_get_component(ecs_registry_t* reg, ecs_id_t entity_id, ecs_id_t component_id);
void 			ecs_iter_component(ecs_registry_t* reg, ecs_id_t component_id, pfn_ecs_iter_component_func callback);
void 			ecs_system(ecs_registry_t* reg, pfn_ecs_iter_func func, uint32_t ncomps, ...);

#define ecs_id(id)  ((ecs_id_t) { id })

/*     Hashing     */
#define HASH64_VALUE (0xcbf29ce484222325)
#define HASH64_PRIME (0x100000001b3)

static inline uint64_t hash64_fnv1a(const char* const str, const uint64_t value) {
	return (str[0] == '\0') ? value : hash64_fnv1a(&str[1], (value ^ (uint64_t)((uint8_t)str[0])) * HASH64_PRIME);
}

#define HASH32_VALUE (0x811c9dc5)
#define HASH32_PRIME (0x1000193)

static inline uint32_t hash32_fnv1a(const char* const str, const uint32_t value) {
	return (str[0] == '\0') ? value : hash32_fnv1a(&str[1], (value ^ (uint32_t)((uint8_t)str[0])) * HASH32_PRIME);
}

#define hash64(str)    		(hash64_fnv1a(str, HASH64_VALUE))
#define hash32(str)    		(hash32_fnv1a(str, HASH32_VALUE))
#define hash64_id(str)    	(ecs_id(hash64(str)))
#define hash32_id(str) 		(ecs_id(hash32(str)))

#define STR(s) #s
#define DEBUG_CHANNEL(channel, fmt, ...) (printf("[" STR(channel) "] <%s:%d> :: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__))
