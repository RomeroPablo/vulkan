#include "hash.h"
#include <stdio.h>
#include <stdlib.h>

hashSet_t* hashInit(){
    hashSet_t* set = (hashSet_t*)malloc(sizeof(hashSet_t));
    set->keys = (void**)calloc(HASH_CAPACITY, sizeof(void **));
    set->values = (void**)calloc(HASH_CAPACITY, sizeof(void **));
    return set;
}

unsigned hashAdd(hashSet_t* set, void* value){
    return hashPut(set, hash(value), value);
};

unsigned hashPut(hashSet_t* set, long long hash, void* value){
    if(hashContainsHash(set, hash)){
        if(set->keys[hashRetrieveIndex(hash, set->capacity)] == value){
            return 0;
        }
        hashResize(set);
        return hashPut(set, hash, value);
    }

    set->keys[hashRetrieveIndex(hash, set->capacity)] = value;
    set->values[set->length++] = value;
    return 1;
};

int hashContains(hashSet_t* set, long long hash, void* value){
    return set->keys[hashRetrieveIndex(hash, set->capacity)] == value ? 1 : 0;
};

int hashContainsHash(hashSet_t* set, long long hash){
    return set->keys[hashRetrieveIndex(hash, set->capacity)] ? 1 : 0;
};

void hashDelete(hashSet_t* set, void* value){
    set->keys[hashRetrieveIndex(hash(value), set->capacity)] = NULL;
};

long long hash(void* value){
    char* str = (char*)value;
    int a = 1;
    int b = 0;
    const int MODADLER = 65521;
    for(int i = 0; str[i] != '\0'; i++){
        a = (a + str[i] % MODADLER);
        b = (b + a) % MODADLER;
    }
    return (b << 16) | a;
};

unsigned hashRetrieveIndex(long long hash, unsigned capacity){
    return (capacity - 1) & (hash ^ (hash >> 12));
};

void hashResize(hashSet_t* set){
    void **keysResized = (void**)calloc((set->capacity <<= 1), sizeof(void**));
    for(int i = 0; i < set->length; i++)
        keysResized[hashRetrieveIndex(hash(set->values[i]), set->capacity)] = set->values[i];

    free(set->keys);
    set->keys = keysResized;
    void **newValues = (void**)realloc(set->values, set->capacity * sizeof(void**));
    set->values = newValues;
};
