#include "ringBuffer.hpp"
#include <utility>

template <typename T>
AudioRingBuffer<T>::AudioRingBuffer(UINT32 capacity) : u_capacity{capacity}
{
    if (capacity > 0)
    {
        p_data = new T[capacity];
        std::memset(p_data, 0, sizeof(T) * capacity);
    }
}

template <typename T>
AudioRingBuffer<T>::~AudioRingBuffer()
{
    if (p_data != nullptr)
        delete[] p_data;
}

template <typename T>
AudioRingBuffer<T>::
    AudioRingBuffer(AudioRingBuffer &&other) noexcept
{
    p_data = std::exchange(other.p_data, nullptr);
    u_capacity = std::exchange(other.u_capacity, 0);
    u_valid_data = std::exchange(other.u_valid_data, 0);

    u_read = other.u_read;
    u_write = other.u_write;
    l_total_read = other.l_total_read;
    l_total_write = other.l_total_write;
}

template <typename T>
AudioRingBuffer<T> &AudioRingBuffer<T>::operator=(AudioRingBuffer &&other) noexcept
{
    if (this != &other)
    {
        delete[] p_data;
        p_data = std::exchange(other.p_data, nullptr);
        u_capacity = std::exchange(other.u_capacity, 0);
        u_valid_data= std::exchange(other.u_valid_data, 0);

        u_read = other.u_read;
        u_write = other.u_write;
        l_total_read = other.l_total_read;
        l_total_write = other.l_total_write;
    }
    return *this;
}

template <typename T>
UINT32 AudioRingBuffer<T>::getNumberData() const{
    return u_valid_data;
}

template <typename T>
UINT32 AudioRingBuffer<T>::getFreeSpace() const{
    return u_capacity - u_valid_data;
}

template <typename T>
UINT64 AudioRingBuffer<T>::getTotalRead() const{
    return l_total_read;
}

template <typename T>
UINT64 AudioRingBuffer<T>::getTotalWrite() const{
    return l_total_write;
}

template <typename T>
UINT32 AudioRingBuffer<T>::getCapacity() const {
    return u_capacity;
}

template <typename T>
void AudioRingBuffer<T>::reset(UINT32 newCapacity) {
    if (u_valid_data > newCapacity or newCapacity==0 or u_valid_data==0) return;
    pT p = new T[newCapacity];
    // void *__cdecl memset(void *_Dst, int _Val, size_t _Size)
    // _Val only low 8_bit is valid
    std::memset(p, 0, sizeof(T) * newCapacity);
    if (u_read > u_write) {
        // two segment
        std::memcpy(p, p_data + u_read, sizeof(T) * (u_capacity - u_read));
        std::memcpy(p + (u_capacity - u_read), p_data, sizeof(T) * u_write);
    }
    else {
        std::memcpy(p, p_data + u_read, sizeof(T) * (u_write - u_read));
    }

    // u_valid_data does not change
    u_read = 0;
    u_write = u_valid_data;
    u_capacity = newCapacity;
    delete[] p_data;
    p_data = p;
}

template <typename T>
void AudioRingBuffer<T>::clear() {
    u_read=u_write=u_valid_data=0;
    l_total_read=l_total_write=0;
    std::memset(p_data, 0, sizeof(T) * u_capacity);
}

template <typename T>
UINT32 AudioRingBuffer<T>::write(const pT p_scr, UINT32 number) {
        if (number==0 or getFreeSpace()==0) return 0;

        UINT32 writeCount = min(number, getFreeSpace());
        UINT32 spaceToEnd = u_capacity - u_write;

        UINT32 firstPart = min(spaceToEnd, writeCount);
        memcpy(p_data + u_write, p_scr, sizeof(T) * firstPart);

        if (writeCount > firstPart) {
            UINT32 secondPart = writeCount - firstPart;
           memcpy(p_data, p_scr + firstPart, sizeof(T) * secondPart);
           u_write = secondPart;
        }
        else {
            u_write += firstPart;
            if (u_write == u_capacity) u_write=0;
        }

        u_valid_data += writeCount;
        l_total_write += writeCount;
        return writeCount;
    }