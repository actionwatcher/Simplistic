#include "fifobuffer.h"

FIFOBuffer::FIFOBuffer(int size) {
    buffer = new char[size];
    capacity = size;
    front = 0;
    rear = -1;
    count = 0;
}

FIFOBuffer::~FIFOBuffer() {
    delete[] buffer;
}

void FIFOBuffer::append(char c) {
    if (is_full()) {
        return;
    }

    rear = (rear + 1) % capacity;
    buffer[rear] = c;
    count++;
}

void FIFOBuffer::drop() {
    if (count == 0) {
        // Buffer is empty
        return;
    }

    rear = (rear - 1 + capacity) % capacity;
    count--;
}


char FIFOBuffer::get() {
    if (count == 0) {
        return 0xff;
    }
    char c = buffer[front];
    front = (front + 1) % capacity;
    count--;
    return c;
}

void FIFOBuffer::putback() {
    if (is_full()) {
        return;
    }
    front = (front - 1 + capacity) % capacity;
    count++;
}

char FIFOBuffer::peek() const {
    if (count == 0) {
        // Buffer is empty
        return 0xff;
    }

    return buffer[front];
}

int FIFOBuffer::size() const {
    return count;
}

bool FIFOBuffer::is_full() const {
    return count == capacity;
}

void FIFOBuffer::clear() {
    front = 0;
    rear = -1;
    count = 0;
}