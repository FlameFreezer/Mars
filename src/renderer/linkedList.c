#ifndef MARS_H
#include <mars.h>
#endif

struct MarsNode_T {
    char const* name;
    struct MarsNode_T* next;
    struct MarsNode_T* prev;
};

typedef struct MarsNode_T MarsNode;

typedef struct {
    MarsNode* head;
    MarsNode* tail;
} MarsLinkedList;

static MarsError marsPush(MarsLinkedList* list, char const* name) {
    if(!list || !name) return marsMakeError(MARS_MISC_ERROR, "Bad call to function: marsPush");
    MarsNode* newNode = SDL_malloc(sizeof(MarsNode));
    if(!newNode) {
        return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }
    newNode->name = name;
    newNode->prev = list->tail;
    newNode->next = nullptr;
    if(!list->head) {
        list->head = newNode;
        list->tail = newNode;
    }
    else {
        list->tail->next = newNode;
    }
    return MARS_SUCCESS;
}

static void marsRemove(MarsLinkedList* list, char const* name) {
    if(!list || !name) return;
    //Find the desired node
    MarsNode* iterator = list->head;
    while(iterator && strcmp(iterator->name, name)) {
        iterator = iterator->next;
    }
    if(!iterator) return;
    if(iterator->prev) {
        iterator->prev->next = iterator->next;
    }
    else {
        list->head = iterator->next;
    }
    if(iterator->next) {
        iterator->next->prev = iterator->prev;
    }
    else {
        list->tail = iterator->prev;
    }
    SDL_free(iterator);
}

static void marsClear(MarsLinkedList* list) {
    if(!list) return;
    MarsNode* iterator = list->head;
    while(iterator) {
        iterator = iterator->next;
        SDL_free(iterator->prev);
    }
    list->head = nullptr;
    list->tail = nullptr;
}