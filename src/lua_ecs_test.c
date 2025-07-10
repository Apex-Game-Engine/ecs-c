#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdbool.h>
#include <assert.h>

#include "ecs.h"

#define nameof(type)    (((type*)0), #type)

#define DEFAULT_ECS_POOL_INIT_COUNT 32

typedef struct ecs_entity_t ecs_entity_t;

ecs_registry_t* get_ecs_registry_from_lua(lua_State* L) {
	lua_getfield(L, LUA_REGISTRYINDEX, nameof(ecs_registry_t));
	ecs_registry_t* reg = (ecs_registry_t*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return reg;
}

int lua_ecs_RegisterComponent(lua_State* L) {
	ecs_registry_t* reg = get_ecs_registry_from_lua(L);
	luaL_checktype(L, 1, LUA_TTABLE);
	const char* id_str = luaL_checkstring(L, 2);
	uint64_t id = hash(id_str);
	ecs_register_component(reg, sizeof(int), id, DEFAULT_ECS_POOL_INIT_COUNT);
	lua_setfield(L, -2, "__name"); /* table.__name = id_str */
	lua_setfield(L, LUA_REGISTRYINDEX, id_str);
	assert(lua_gettop(L) == 0);
	return 0;
}

int lua_ecs_CreateEntity(lua_State* L) {
	ecs_registry_t* reg = get_ecs_registry_from_lua(L);
	id_t entity = ecs_new_entity(reg);
	id_t* lua_entity = (id_t*)lua_newuserdata(L, sizeof(id_t));
	*lua_entity = entity;
	luaL_setmetatable(L, nameof(ecs_entity_t));
	return 1;
}

int lua_ecs_entity_AddComponent(lua_State* L) {
	ecs_registry_t* reg = get_ecs_registry_from_lua(L);
	id_t entity = *(id_t*)luaL_checkudata(L, 1, nameof(ecs_entity_t));
	printf("AddComponent :: entity = %lu\n", entity);
	const char* id_str = luaL_checkstring(L, 2);
	id_t id = hash(id_str);
	lua_pop(L, 2);
	int* comp = ecs_add_component(reg, entity, id);
	lua_newtable(L);
	luaL_setmetatable(L, nameof(ecs_entity_t));
	lua_pushvalue(L, -1);
	*comp = luaL_ref(L, -1);
	assert(lua_gettop(L) == 1);
	return 1;
}

int lua_ecs_entity_HasComponent(lua_State* L) {
	ecs_registry_t* reg = get_ecs_registry_from_lua(L);
	id_t entity = *(id_t*)luaL_checkudata(L, 1, nameof(ecs_entity_t));
	const char* id_str = luaL_checkstring(L, 2);
	id_t id = hash(id_str);
	lua_pop(L, 2);
	bool has_comp = ecs_has_component(reg, entity, id);
	lua_pushboolean(L, has_comp);
	assert(lua_gettop(L) == 1);
	return 1;
}

int lua_ecs_entity_GetId(lua_State* L) {
	id_t entity = *(id_t*)luaL_checkudata(L, 1, nameof(ecs_entity_t));
	lua_pop(L, 1);
	lua_pushinteger(L, entity);
	return 1;
}

int lua_openecs(lua_State* L) {
	const luaL_Reg ecs_F[] = {
		{ "RegisterComponent", lua_ecs_RegisterComponent },
		{ "CreateEntity", lua_ecs_CreateEntity },
		{ NULL, NULL }
	};
	luaL_newlib(L, ecs_F); /* ecs = {} */

	const luaL_Reg ecs_entity_M[] = {
		{ "AddComponent", lua_ecs_entity_AddComponent },
		// { "GetComponent", lua_ecs_entity_GetComponent },
		{ "HasComponent", lua_ecs_entity_HasComponent },
		{ "GetId", lua_ecs_entity_GetId },
		{ NULL, NULL }
	};

	luaL_newmetatable(L, nameof(ecs_entity_t));
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index"); /* ecs.entity.__index = ecs.entity */
	luaL_setfuncs(L, ecs_entity_M, 0);
	lua_pop(L, 1);

	return 1;
}

typedef struct { float x, y, z; } Vector3;

#define LUA_CHECK(L, cmd) do { if (LUA_OK != (cmd)) { const char* _err = luaL_checkstring(L, -1); printf("[ERROR] <%s:%d> Lua error : %s\n", __FUNCTION__, __LINE__, _err); } } while (0)

void ecs_position_callback(ecs_registry_t* reg, id_t entity, void* comp) {
	Vector3* pos = (Vector3*)comp;
	printf("Position : %lu (%f, %f, %f)\n", entity, pos->x, pos->y, pos->z);
}

int main() {
	ecs_registry_t reg;
	ecs_init(&reg);

	ecs_register_component(&reg, sizeof(Vector3), hash("Position"), DEFAULT_ECS_POOL_INIT_COUNT);

	lua_State* L = luaL_newstate();
	lua_pushstring(L, nameof(ecs_registry_t));
	lua_pushlightuserdata(L, &reg);
	lua_rawset(L, LUA_REGISTRYINDEX);

	luaL_openlibs(L);
	luaL_requiref(L, "ecs", lua_openecs, true);

	LUA_CHECK(L, luaL_dofile(L, "ecs_test.lua"));

	{
		// void* comp = ecs_component_storage(&reg, hash("Money"));
		// assert(comp != NULL);
	}

	{
		ecs_iter_component(&reg, hash("Position"), ecs_position_callback);
	}

	lua_close(L);

	ecs_cleanup(&reg);
	return 0;
}
