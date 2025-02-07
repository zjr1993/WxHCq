#pragma once

constexpr auto POOLSIZE = 10480;

class MPool{
    public:
    MPool();
    ~MPool();
    void DestroyPool();
    char* GetBlock(size_t count);
    void FreePool();
    unsigned long Thread_Id;
    char* Pool;
    size_t Offset;
};