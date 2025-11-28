#ifndef MARS_H
#include <mars.hpp>
#endif

typedef struct {
    MarsNode* head;
    MarsNode* tail;
} MarsLinkedList;

static MarsError marsPush(MarsLinkedList* list, char const* name) {
    if(list == nullptr || name == nullptr) return marsMakeError(MARS_BAD_FUNCTION_CALL, "Bad call to function: marsPush");
    MarsNode* newNode = SDL_malloc(sizeof(MarsNode));
    if(newNode == nullptr) {
        return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }
    newNode->name = name;
    newNode->prev = list->tail;
    newNode->next = nullptr;
    if(list->head == nullptr) {
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
    while(iterator != nullptr && strcmp(iterator->name, name) != 0) {
        iterator = iterator->next;
    }
    if(iterator == nullptr) return;
    if(iterator->prev != nullptr) {
        iterator->prev->next = iterator->next;
    }
    else {
        list->head = iterator->next;
    }
    if(iterator->next != nullptr) {
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
    while(iterator != nullptr) {
        iterator = iterator->next;
        SDL_free(iterator->prev);
    }
    list->head = nullptr;
    list->tail = nullptr;
}