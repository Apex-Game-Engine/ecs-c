#include "hashmap.h"

#include <stdio.h>
#include <string.h>

const uint64_t HASH64_VALUE = 0xcbf29ce484222325;
const uint64_t HASH64_PRIME = 0x100000001b3;

char static_assert__sizeof_uint64_t[(sizeof(uint64_t) == 8) ? 1 : 0];

inline uint64_t hash_str_helper(const char* const str, const uint64_t value) {
	return (str[0] == '\0') ? value : hash_str_helper(&str[1], (value ^ (uint64_t)((uint8_t)str[0])) * HASH64_PRIME);
}

inline uint64_t hash_str_const(const char* const str) {
	return hash_str_helper(str, HASH64_VALUE);
}

uint64_t hash_str(void* str) {
	return hash_str_const((const char* const)str);
}

bool streq(void* str1, void* str2) {
	return !strcmp(str1, str2);
}

typedef struct Date {
	uint32_t dd : 5;
	uint32_t mm : 4;
	uint32_t yyyy : 23;
} Date;

typedef char Username[64];
typedef char Email[64];

typedef struct User {
	int age;
	Date dob;
	char email[64];
} User;

int hashmap_test() {
	// HashMap<char[64], uint32_t>
	Username tombstone; memset(tombstone, -1, sizeof(Username));
	hashmap_t hm = hashmap_create(sizeof(Username), sizeof(User), hash_str, streq, 11, tombstone);

	{
		char k[64] = "athang213";
		User v = (User) {
			.age = 26,
			.dob = (Date) { 21, 03, 1999 },
		};
		strncpy(v.email, "athang213@gmail.com", sizeof(Email)-1);
		hashmap_insert(hm, &k, &v);
	}

	{
		Username uname = "athang213";
		User* user = (User*)hashmap_find(hm, uname);
		if (!user) {
			printf("Unknown user '%s'\n", uname);
		} else {
			printf("uname: %s\n", uname);
			printf("dob:   %02u-%02u-%04u\n", user->dob.dd, user->dob.mm, user->dob.yyyy);
			printf("email: %s\n", user->email);
		}
	}

	{
		for (int i = 0; i < 5; i++) {
			Username uname;
			snprintf(uname, sizeof(uname), "user%d", i);
			User user = (User) { .age = 20 + i, .dob = { i + 1, i + 1, 1990 + i } };
			snprintf(user.email, sizeof(user.email), "user%d@example.com", i);
			hashmap_insert(hm, &uname, &user);
		}
	}

	{
		hashmap_erase(hm, &(Username) { "user1" }, NULL);
	}

	{
		Username* uname; User* user;
		for (uint32_t index = hashmap_iternext(hm, 0, (void**)&uname, (void**)&user); index != -1; index = hashmap_iternext(hm, index, (void**)&uname, (void**)&user)) {
			printf("index: %u\n", index-1);
			printf("uname: %s\n", *uname);
			printf("dob:   %02u-%02u-%04u\n", user->dob.dd, user->dob.mm, user->dob.yyyy);
			printf("email: %s\n", user->email);
		}
	}

	hashmap_destroy(hm);
	return 0;
}

int main() {
	return hashmap_test();
}

