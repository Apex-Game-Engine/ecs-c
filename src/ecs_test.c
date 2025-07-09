#include "ecs.h"

#include <stdio.h>

/********************************************************************/
/* Driver */

typedef struct MeshRenderer {
	id_t meshId;
	id_t materialId;
	uint32_t flags;
} MeshRenderer;

typedef struct HUDElement {
	id_t fontId;
	const char* text;
} HUDElement;

int ecs_test() {
	bool success;
	ecs_registry_t reg;
	ecs_init(&reg);

	success = ecs_register_component(&reg, sizeof(MeshRenderer), hash("MeshRenderer"), 64);
	printf("Component 'MeshRenderer' %s registered\n", success ? "successfully" : "NOT");
	success = ecs_register_component(&reg, sizeof(HUDElement), hash("HUDElement"), 64);
	printf("Component 'HUDElement' %s registered\n", success ? "successfully" : "NOT");

	id_t entity = ecs_new_entity(&reg);
	{
		MeshRenderer* mr = (MeshRenderer*)ecs_add_component(&reg, entity, hash("MeshRenderer"));
		mr->meshId = 817;
		mr->materialId = 213;
	}
	{
		HUDElement* hud = (HUDElement*)ecs_add_component(&reg, entity, hash("HUDElement"));
		hud->fontId = 1001;
		hud->text = "This is a text box";
	}
	{
		MeshRenderer* mr = (MeshRenderer*)ecs_get_component(&reg, entity, hash("MeshRenderer"));
		if (!mr) {
			printf("ERROR\n");
		}
		else {
			printf("MeshRenderer:: meshId : %lu | materialId : %lu | flags : %u\n", mr->meshId, mr->materialId, mr->flags);
		}
	}
	{
		HUDElement* hud = (HUDElement*)ecs_get_component(&reg, entity, hash("HUDElement"));
		if (!hud) {
			printf("ERROR\n");
		}
		else {
			printf("HUDElement:: fontId : %lu | text : %s\n", hud->fontId, hud->text);
		}
	}

	ecs_cleanup(&reg);
	return 0;
}

int main() {
	return ecs_test();
}

