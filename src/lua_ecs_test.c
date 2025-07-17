#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdbool.h>
#include <assert.h>

#include "ecs.h"

#define lua_debugf(fmt, ...) // DEBUG_CHANNEL(lua, fmt, ##__VA_ARGS__)

#define nameof(type)    (((type*)0), #type)

#define DEFAULT_ECS_POOL_INIT_COUNT 128

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
	ecs_id_t id = hash32_id(id_str);
	ecs_register_component(reg, sizeof(int), id, DEFAULT_ECS_POOL_INIT_COUNT);
	lua_setfield(L, -2, "__name"); /* table.__name = id_str */
	lua_setfield(L, LUA_REGISTRYINDEX, id_str);
	assert(lua_gettop(L) == 0);
	return 0;
}

int lua_ecs_CreateEntity(lua_State* L) {
	ecs_registry_t* reg = get_ecs_registry_from_lua(L);
	ecs_id_t entity = ecs_new_entity(reg);
	ecs_id_t* lua_entity = (ecs_id_t*)lua_newuserdata(L, sizeof(ecs_id_t));
	*lua_entity = entity;
	luaL_setmetatable(L, nameof(ecs_entity_t));
	return 1;
}

int lua_ecs_entity_AddComponent(lua_State* L) {
	ecs_registry_t* reg = get_ecs_registry_from_lua(L);
	ecs_id_t entity = *(ecs_id_t*)luaL_checkudata(L, 1, nameof(ecs_entity_t));
	const char* id_str = luaL_checkstring(L, 2);
	ecs_id_t id = hash32_id(id_str);
	void* comp = ecs_add_component(reg, entity, id);
	lua_debugf("reg: %p | entity: %u | id_str: %s | id: %u | comp: %p", reg, entity.id, id_str, id.id, comp);
	int type = lua_rawget(L, LUA_REGISTRYINDEX);
	lua_debugf("type: %s", lua_typename(L, type));
	if (type == LUA_TNIL) { // Native component
		lua_pop(L, 2);
		// TODO: if there exists a metatable for the native type, then push comp as light user data with that metatable
		assert(lua_gettop(L) == 0);
		return 0;
	}
	if (type == LUA_TTABLE) { // Lua component
		lua_newtable(L);
		lua_insert(L, -2);
		lua_setmetatable(L, -2);
		lua_copy(L, -1, -2);
		int* luacomp = (int*)comp;
		*luacomp = luaL_ref(L, LUA_REGISTRYINDEX);
		lua_debugf("luacomp: %d", *luacomp);
	}
	assert(lua_gettop(L) == 1);
	return 1;
}

int lua_ecs_entity_HasComponent(lua_State* L) {
	ecs_registry_t* reg = get_ecs_registry_from_lua(L);
	ecs_id_t entity = *(ecs_id_t*)luaL_checkudata(L, 1, nameof(ecs_entity_t));
	const char* id_str = luaL_checkstring(L, 2);
	ecs_id_t id = hash32_id(id_str);
	lua_pop(L, 2);
	bool has_comp = ecs_has_component(reg, entity, id);
	lua_pushboolean(L, has_comp);
	assert(lua_gettop(L) == 1);
	return 1;
}

int lua_ecs_entity_GetId(lua_State* L) {
	ecs_id_t entity = *(ecs_id_t*)luaL_checkudata(L, 1, nameof(ecs_entity_t));
	lua_pop(L, 1);
	lua_pushinteger(L, entity.id);
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

lua_State* L;

void ecs_position_callback(ecs_registry_t* reg, ecs_id_t entity, void* comp) {
	Vector3* pos = (Vector3*)comp;
	printf("Position : %u (%f, %f, %f)\n", entity.id, pos->x, pos->y, pos->z);
}

void ecs_money_callback(ecs_registry_t* reg, ecs_id_t entity, void* comp) {
	(void)reg; 
	lua_debugf("comp: %p", comp);
	int refid = *(int*)comp;
	lua_rawgeti(L, LUA_REGISTRYINDEX, refid);
	lua_getfield(L, -1, "balance");
	double balance = luaL_checknumber(L, -1);
	printf("[Money] Entity (%u) : %lf\n", entity.id, balance);
	lua_pop(L, 2);
}

void movement_system(ecs_registry_t* reg, ecs_id_t entity, uint32_t ncomps, ecs_id_t* comps) {
	(void)reg; (void)ncomps; (void)comps;
	printf("[Position, Velocity] Entity: %u\n", entity.id);
}

int main() {
	ecs_registry_t* reg = ecs_init();

	ecs_register_component(reg, sizeof(Vector3), hash32_id("Position"), DEFAULT_ECS_POOL_INIT_COUNT);
	ecs_register_component(reg, sizeof(Vector3), hash32_id("Velocity"), DEFAULT_ECS_POOL_INIT_COUNT);

	L = luaL_newstate();
	lua_pushstring(L, nameof(ecs_registry_t));
	lua_pushlightuserdata(L, reg);
	lua_rawset(L, LUA_REGISTRYINDEX);

	luaL_openlibs(L);
	luaL_requiref(L, "ecs", lua_openecs, true);

	LUA_CHECK(L, luaL_dofile(L, "ecs_test.lua"));

	{
		void* comp = ecs_component_storage(reg, hash32_id("Money"));
		assert(comp != NULL);
	}

	{
		printf("Movement System : Position, Velocity\n");
		ecs_system(reg, movement_system, 2, hash32_id("Position"), hash32_id("Velocity"));
	}

	{
		// ecs_iter_component(reg, hash("Position"), ecs_position_callback);
		ecs_iter_component(reg, hash32_id("Money"), ecs_money_callback);
	}

	lua_close(L);

	ecs_cleanup(reg);
	return 0;
}
