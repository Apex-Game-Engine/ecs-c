#include "ecs.h"

#include <stdio.h>

#define test_debugf(fmt, ...) DEBUG_CHANNEL(test, fmt, ##__VA_ARGS__)

/********************************************************************/
/* Driver */

typedef struct MeshRenderer {
	ecs_id_t meshId;
	ecs_id_t materialId;
	uint32_t flags;
} MeshRenderer;

typedef struct HUDElement {
	ecs_id_t fontId;
	const char* text;
} HUDElement;

int ecs_ss_test() {
	ecs_ss_t* ss = ecs_ss_create(sizeof(MeshRenderer), 100, 32);
	//ecs_id_t ent = { 12 };
	ecs_ss_slot_t slot;

	if (ECS_OK == ecs_ss_emplace(ss, ecs_id(12), &slot)) {
		MeshRenderer* mr = (MeshRenderer*)slot.data;
		ecs_id_t mesh = { 800 }, mat = { 170 };
		*mr = (MeshRenderer) { mesh, mat };
	}

	ecs_ss_destroy(ss);
	return 0;
}
int ecs_test() {
	bool success;
	ecs_registry_t* reg = ecs_init();

	success = ecs_register_component(reg, sizeof(MeshRenderer), hash32_id("MeshRenderer"), 64);
	test_debugf("Component 'MeshRenderer' %s registered", success ? "successfully" : "NOT");
	success = ecs_register_component(reg, sizeof(HUDElement), hash32_id("HUDElement"), 64);
	test_debugf("Component 'HUDElement' %s registered", success ? "successfully" : "NOT");

	ecs_id_t entity = ecs_new_entity(reg);
	{
		MeshRenderer* mr = (MeshRenderer*)ecs_add_component(reg, entity, hash32_id("MeshRenderer"));
		mr->meshId = ecs_id(817);
		mr->materialId = ecs_id(213);
	}
	{
		HUDElement* hud = (HUDElement*)ecs_add_component(reg, entity, hash32_id("HUDElement"));
		hud->fontId = ecs_id(1001);
		hud->text = "This is a text box";
	}
	{
		MeshRenderer* mr = (MeshRenderer*)ecs_get_component(reg, entity, hash32_id("MeshRenderer"));
		if (!mr) {
			test_debugf("ERROR");
		}
		else {
			test_debugf("MeshRenderer:: meshId : %u | materialId : %u | flags : %u", mr->meshId.id, mr->materialId.id, mr->flags);
		}
	}
	{
		HUDElement* hud = (HUDElement*)ecs_get_component(reg, entity, hash32_id("HUDElement"));
		if (!hud) {
			test_debugf("ERROR");
		}
		else {
			test_debugf("HUDElement:: fontId : %u | text : %s", hud->fontId.id, hud->text);
		}
	}

	ecs_cleanup(reg);
	return 0;
}

int main() {
	int res = 0;
	res = ecs_ss_test();
	res = ecs_test();
	return res;
}

