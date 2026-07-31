#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cstring>
#include <type_traits>

template<typename T>
class AudioRingBuffer {
    static_assert(std::is_trivially_copyable<T>::value,
        "AudioRingBuffer uses memcpy/memset internally; T must be trivially copyable.");
    using pT = T*;
    pT p_data {nullptr};

    UINT32 u_capacity {0};
    UINT32 u_read {0};
    UINT32 u_write {0};

    UINT32 u_valid_data {0};

    UINT64 l_total_read {0};
    UINT64 l_total_write {0};

    public:
        AudioRingBuffer() = default;
        explicit AudioRingBuffer(UINT32 capacity);
        ~AudioRingBuffer();

        AudioRingBuffer(const AudioRingBuffer&) = delete;
        AudioRingBuffer& operator=(const AudioRingBuffer&) = delete;

        AudioRingBuffer(AudioRingBuffer&& other) noexcept;
        AudioRingBuffer& operator=(AudioRingBuffer&& other) noexcept;

        UINT32 getNumberData() const;
        UINT32 getFreeSpace() const;
        UINT64 getTotalRead() const;
        UINT64 getTotalWrite() const;
        UINT32 getCapacity() const;

        void reset(UINT32 newCapacity);

        void clear();

        UINT32 write(const pT p_scr, UINT32 number);

        UINT32 read(pT p_dst, UINT32 number);

        UINT32 _writeN(const pT p_scr, UINT32 number);

        UINT32 _readN(pT p_dst, UINT32 number);

};