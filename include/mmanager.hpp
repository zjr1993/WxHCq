#pragma once
#include "mpool.hpp"
constexpr auto MPOOLS = 4;

class MManager {
public:
	MManager();
	~MManager();
	static MManager* GetMManager();
	char* GetTempMemory(size_t count);
	void FreeAllTempMemory();
private:
	MPool* CreateNewPool(unsigned long Th_Id);
	MPool* GetMemoryPool(unsigned long Th_Id);
	void GrowPools();
	int numCur;
	int numMax;
	MPool* PPool;
};

char* GetTempMemory(size_t bytes);
void FreeAllTempMemory();
