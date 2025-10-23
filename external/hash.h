#pragma once
#define HASH_CAPACITY 1 << 10

struct HashSet{
    unsigned capacity;
    unsigned length;
    void ** values;
    void ** keys;
} typedef hashSet_t;

hashSet_t* hashInit();
unsigned hashAdd(hashSet_t* set, void* value);
unsigned hashPut(hashSet_t* set, long long hash, void* value);
int hashContains(hashSet_t* set, long long hash, void* value);
int hashContainsHash(hashSet_t* set, long long hash);
void hashDelete(hashSet_t* set, void* value);
long long hash(void* value);
unsigned hashRetrieveIndex(long long hash, unsigned capacity);
void hashResize(hashSet_t* set);
