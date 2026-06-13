#include "error.h"

void MessageList::freeMemory() noexcept {
	ErrorMessage* current = mHead;
	while (current) {
		ErrorMessage* toDelete = current;
		current = current->next;
		delete toDelete;
	}
}

void MessageList::copyList(const MessageList& other) noexcept {
	ErrorMessage* current = other.mHead;

	while (current) {
		pushBack(current->message);
		current = current->next;
	}
}

MessageList::~MessageList() noexcept {
	freeMemory();
}

void MessageList::clear() noexcept {
	freeMemory();
	mHead = nullptr;
	mTail = nullptr;
}

MessageList::MessageList(const MessageList& other) noexcept {
	copyList(other);
}

MessageList::MessageList(MessageList&& other) noexcept : mHead(other.mHead), mTail(other.mTail) {
	other.mHead = nullptr;
	other.mTail = nullptr;
}

MessageList& MessageList::operator=(const MessageList& other) noexcept {
	clear();
	copyList(other);
	return *this;
}

MessageList& MessageList::operator=(MessageList&& other) noexcept {
	clear();
	mHead = other.mHead;
	mTail = other.mTail;
	other.mHead = nullptr;
	other.mTail = nullptr;
	return *this;
}

void MessageList::pushBack(const std::string& message) noexcept {
	ErrorMessage* back = new ErrorMessage{ .message = message, .prev = mTail };
	if (!mHead) {
		mHead = back;
		mTail = back;
	}
	else {
		mTail->next = back;
		mTail = back;
	}
}

void MessageList::pushBack(std::string&& message) noexcept {
	ErrorMessage* back = new ErrorMessage{ .message = std::move(message), .prev = mTail };
	if (!mHead) {
		mHead = back;
		mTail = back;
	}
	else {
		mTail->next = back;
		mTail = back;
	}
}

void MessageList::pushFront(const std::string& message) noexcept {
	ErrorMessage* front = new ErrorMessage{ .message = message, .prev = nullptr, .next = mHead };
	if (!mHead) {
		mHead = front;
		mTail = front;
	}
	else {
		mHead->prev = front;
		mHead = front;
	}
}

void MessageList::pushFront(std::string&& message) noexcept {
	ErrorMessage* front = new ErrorMessage{ .message = std::move(message), .prev = nullptr, .next = mHead };
	if (!mHead) {
		mHead = front;
		mTail = front;
	}
	else {
		mHead->prev = front;
		mHead = front;
	}
}

ErrorMessage* MessageList::front() noexcept {
	return mHead;
}

const ErrorMessage* MessageList::front() const noexcept {
	return mHead;
}

ErrorMessage* MessageList::back() noexcept {
	return mTail;
}

const ErrorMessage* MessageList::back() const noexcept {
	return mTail;
}

bool MessageList::empty() const noexcept {
	return mHead == nullptr;
}
