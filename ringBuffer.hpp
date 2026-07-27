#include <Windows.h>
#define rsize(x) ((x)*sizeof(float))


class AudioRingBuffer {
    float* m_pData{NULL};
    UINT32 m_capacity{0};
    UINT32 m_readPos{0};
    UINT32 m_writePos{0};
    UINT32 m_validDataCount{0};
    public:
    AudioRingBuffer()=default;
    AudioRingBuffer(const UINT32 capacity)
    : m_capacity{capacity}{
        m_pData = new float[m_capacity];
        memset(m_pData, 0, rsize(m_capacity));
    }

    ~AudioRingBuffer() {
        if (m_pData) {
            delete[] m_pData;
            m_pData = NULL;
        }
    }

    UINT32 getValidDataCount() const { return m_validDataCount; }
	UINT32 getFreeSpace() const { return m_capacity - m_validDataCount; }

    UINT32 Write(const float* pSRCData, UINT32 count) {
        if (count==0 or getFreeSpace()==0) return 0;

        UINT32 writeCount = min(count, getFreeSpace());
        UINT32 spaceToEnd = m_capacity - m_writePos;

        UINT32 firstPart = min(spaceToEnd, writeCount);
        memcpy(m_pData + m_writePos, pSRCData, rsize(firstPart));

        if (writeCount > firstPart) {
            UINT32 secondPart = writeCount - firstPart;
           memcpy(m_pData, pSRCData + firstPart, rsize(secondPart));
           m_writePos = secondPart;
        }
        else {
            m_writePos += firstPart;
            if (m_writePos == m_capacity) m_writePos=0;
        }

        m_validDataCount += writeCount;
        return writeCount;
    }

    UINT32 Read(float* pDSTData, UINT32 count) {
        if (count==0 or getValidDataCount()==0) return 0;
        UINT32 readCount = min(count, getValidDataCount());
        UINT32 spaceToEnd = m_capacity - m_readPos;
        UINT32 firstPart = min(readCount, spaceToEnd);

        memcpy(pDSTData, m_pData + m_readPos, rsize(firstPart));

        if (readCount > firstPart) {
            UINT32 secondPart = readCount - firstPart;
            memcpy(pDSTData+firstPart, m_pData, rsize(secondPart));
            m_readPos = secondPart;
        }
        else {
            m_readPos += firstPart;
            if (m_readPos == m_capacity) m_readPos = 0;
        }

        m_validDataCount -= readCount;
        return readCount;
    }
};
