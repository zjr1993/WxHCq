#include <iostream>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <format>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <algorithm>
#include "ringBuffer.hpp"
const REFERENCE_TIME REFTIMES_PER_SEC = 10000000;

// 安全释放宏
template <class T> void SafeRelease(T** ppT) {
    if (*ppT) { (*ppT)->Release(); *ppT = NULL; }
}

#define notEnd(x) (!((x) & MF_SOURCE_READERF_ENDOFSTREAM))

// 初始化 Source Reader 并强制输出 32-bit Float PCM
HRESULT InitSourceReader(LPCWSTR filePath, IMFSourceReader** ppReader, WAVEFORMATEX** ppAudioFormat) {
    IMFSourceReader* pReader = NULL;
    IMFMediaType* pPartialType = NULL;
    IMFMediaType* pUncompressedAudioType = NULL;
    UINT32 cbFormat = 0;

    // 1. 从本地文件路径创建 Source Reader [cite: 2]
    HRESULT hr = MFCreateSourceReaderFromURL(filePath, NULL, &pReader);
    if (FAILED(hr)) goto done;

    // 2. 选择第一个音频流，取消选择其他流（如视频流）
    hr = pReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
    if (FAILED(hr)) goto done;
    hr = pReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
    if (FAILED(hr)) goto done;

    // 3. 创建一个部分的 Media Type 来强制指定输出格式 [cite: 2]
    hr = MFCreateMediaType(&pPartialType);
    if (FAILED(hr)) goto done;

    // 设置为主类型：音频 (Audio) [cite: 2]
    hr = pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (FAILED(hr)) goto done;

    // 设置为子类型：32-bit 浮点 PCM (IEEE Float) [cite: 2, 6]
    hr = pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
    if (FAILED(hr)) goto done;

    // 4. 将这个部分类型设置给 Source Reader，强制它在底层挂载对应的解码器 [cite: 2]
    hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pPartialType);
    if (FAILED(hr)) goto done;

    // 5. 解码器协商成功后，获取包含全部详细信息（采样率、声道数等）的最终 Media Type [cite: 2, 3]
    hr = pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pUncompressedAudioType);
    if (FAILED(hr)) goto done;

    // 6. 将 MF 的 Media Type 转换为标准的 WAVEFORMATEX 结构体，供 WASAPI 使用 [cite: 3, 4]
    hr = MFCreateWaveFormatExFromMFMediaType(pUncompressedAudioType, ppAudioFormat, &cbFormat);
    if (FAILED(hr)) goto done;

    // 成功，将指针传出
    *ppReader = pReader;
    (*ppReader)->AddRef();

done:
    SafeRelease(&pReader);
    SafeRelease(&pPartialType);
    SafeRelease(&pUncompressedAudioType);
    return hr;
}

HRESULT ReadAndVerifyPCM(IMFSourceReader* pReader, WAVEFORMATEX* pWaveFormat) {
    HRESULT hr = S_OK;
    IMFSample* pSample = NULL;
    IMFMediaBuffer* pBuffer = NULL;
    DWORD streamIndex = MF_SOURCE_READER_FIRST_AUDIO_STREAM;

    std::cout << "\n>>> Starting PCM Read Verification (Skipping Silence)..." << std::endl;

    DWORD flags = 0;
    LONGLONG timestamp = 0;
    bool foundNonZero = false;
    int chunkCount = 0;

    // 循环读取，直到找到非 0 数据或到达文件末尾
    while (!foundNonZero) {
        hr = pReader->ReadSample(streamIndex, 0, NULL, &flags, &timestamp, &pSample);

        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
            std::cout << "Reached end of stream. No audio data found." << std::endl;
            break;
        }

        if (pSample) {
            hr = pSample->ConvertToContiguousBuffer(&pBuffer);
            if (SUCCEEDED(hr)) {
                BYTE* pAudioData = NULL;
                DWORD cbBufferLength = 0;
                hr = pBuffer->Lock(&pAudioData, NULL, &cbBufferLength);

                if (SUCCEEDED(hr)) {
                    DWORD bytesPerFrame = pWaveFormat->nChannels * sizeof(float);
                    DWORD numFrames = cbBufferLength / bytesPerFrame;
                    float* pFloatData = reinterpret_cast<float*>(pAudioData);
                    chunkCount++;

                    // 遍历当前块的所有帧，寻找非零采样
                    for (DWORD i = 0; i < numFrames; ++i) {
                        bool isSilence = true;

                        // 检查所有声道
                        for (WORD c = 0; c < pWaveFormat->nChannels; ++c) {
                            if (pFloatData[i * pWaveFormat->nChannels + c] != 0.0f) {
                                isSilence = false;
                                break;
                            }
                        }

                        // 一旦发现非 0 数据
                        if (!isSilence) {
                            std::cout << "Skipped " << (chunkCount - 1) << " silent blocks." << std::endl;
                            std::cout << "Found actual audio in Block #" << chunkCount
                                << " starting at Frame " << i << "!" << std::endl;

                            // 打印非零开始的前 100 帧
                            DWORD framesToPrint = min(numFrames - i, (DWORD)100);
                            std::cout << "\nFirst 5 active audio frames:" << std::endl;
                            for (DWORD j = 0; j < framesToPrint; ++j) {
                                std::cout << "Frame " << (i + j) << ": [ ";
                                for (WORD c = 0; c < pWaveFormat->nChannels; ++c) {
                                    std::cout << pFloatData[(i + j) * pWaveFormat->nChannels + c] << " ";
                                }
                                std::cout << "]" << std::endl;
                            }
                            foundNonZero = true; // 标记已找到，准备退出外层 while 循环
                            break;
                        }
                    }
                    pBuffer->Unlock();
                }
                pBuffer->Release();
            }
            pSample->Release();
        }
    }

    std::cout << "<<< Verification Complete.\n" << std::endl;
    return hr;
}

HRESULT InitWASAPI(WAVEFORMATEX* pWaveFormat, IAudioClient** ppAudioClient, IAudioRenderClient** ppRenderClient, HANDLE* pEvent) {
    HRESULT hr = S_OK;
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioRenderClient* pRenderClient = NULL;
    HANDLE hEvent = NULL;
    DWORD streamFlag  = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | 
                        AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | 
                        AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;

    std::cout << "\n>>> Starting WASAPI Initialization..." << std::endl;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) goto done;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        std::cerr << "Failed to get default audio endpoint." << std::endl;
        goto done;
    }

    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) goto done;
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlag, REFTIMES_PER_SEC, 0, pWaveFormat, NULL);

    if (FAILED(hr)) {
        std::cerr << "IAudioClient::Initialize failed. The audio format might not be supported in shared mode." << std::endl;
        goto done;
    }
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL) {
        hr = E_FAIL;
        goto done;
    }

    hr = pAudioClient->SetEventHandle(hEvent);
    if (FAILED(hr)) goto done;

    hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient);
    std::cout << "WASAPI Initialized Successfully!" << std::endl;

    *ppAudioClient = pAudioClient;
    (*ppAudioClient)->AddRef();

    *ppRenderClient = pRenderClient;
    (*ppRenderClient)->AddRef();

    *pEvent = hEvent;


done:
    SafeRelease(&pEnumerator);
    SafeRelease(&pDevice);
    SafeRelease(&pAudioClient);
    SafeRelease(&pRenderClient);
    if (FAILED(hr) && hEvent) {
        CloseHandle(hEvent);
    }
    return hr;
}

HRESULT StartPlaybackLoop(IAudioClient* pAudioClient, IAudioRenderClient* pRenderClient,
HANDLE hAudioEvent, IMFSourceReader* pReader, WAVEFORMATEX* pWaveFormat) {
    HRESULT hr=S_OK;
    UINT32 bufferFrameCount = 0;
    UINT32 numFramesPadding = 0;
    UINT32 numFramesAvailable=0;
    IMFSample* pLeftIMFSample = NULL;
    UINT32 alreadyWrite = 0;
    BYTE* pData = NULL;
    bool bPlaying = true;
    DWORD readerFlags = 0;
    WORD channels = pWaveFormat->nChannels;
    bool isEnd = false;
    
    // GetBufferSize函数可以获得 [声卡缓冲区帧数] bufferFrameCount 大小
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);

    // 创建一个环形缓冲区，大小设定为声卡缓冲区的3倍
    // 注意 [帧数] 乘以 [通道数量] 才是采样点数量 (一个采样点占据一个浮点数的字节数量 )
    UINT32 ringBufferCapacity = bufferFrameCount * 10 * channels;
    AudioRingBuffer ringBuffer {ringBufferCapacity};
    
    std::cout << "\n >>> Starting Audio Playback Loop with Ring Buffer."<<std::endl;

    // 开启声卡
    hr = pAudioClient->Start();

    // 进入循环，不断的从MF源写入数据到环形缓冲区，然后再从环形缓冲区读到声卡缓冲区
    // [MF]-->
    //         -->[IMFSample<=>IMFMediaBuffer]-->
    //                                            -->[pRawData<=>pFloatData]
    // pFloatData---Write---->|环形缓冲区|######|******|

    // 声卡不断从环形缓冲区读取数据
    // |         bufferFrameCount          |
    // |numFramesAvailable|numFramesPadding|
    // |环形缓冲区|######|******|-->
    //                             --read-->|numFramesAvailable|
    // |########0000000000| 如果读入的数据量小于numFramesAvailable，剩余空姐需要用 0 填充

    while (bPlaying) {
        // 调用 waitForSingleObject 线程不就会暂时挂起，等待事件 hAudioEvent 触发
        // 参数2000代表该线程最多挂起2秒
        DWORD waitResult = WaitForSingleObject(hAudioEvent, 2000);
        if (waitResult != WAIT_OBJECT_0) {
            // 如果是正常情况，等待后函数WaitForSingleObject返回的是 WAIT_OBJECT_0
            // 只要不是这种情况，立马终止循环，结束播放程序
            std::cerr << "Audio device stalled or event timeout." << std::endl;
            break; 
        }


        // ==========================================
        // 1. 生产者逻辑：从 MF 读取数据并填入蓄水池
        // ==========================================
        // 只要环形缓冲区还能装下至少 12288 帧（一个安全的阈值），且文件没读完，就一直读
        // 注释：对大多数的音频格式，虽然readSample函数返回的数据大小不一样，但是都会远小于12288帧
        // 因此我们可以放心的认为缓冲区能装下所有readSample得到的数据

        UINT32 threshold = min(4096, bufferFrameCount) * channels;
        
        while (ringBuffer.getFreeSpace() > threshold and (notEnd(readerFlags) or pLeftIMFSample!=NULL))
        {
            // 循环写入数据, 直到文件结尾或者环形缓冲区空间不够
            IMFSample* pSample = NULL;

            if (pLeftIMFSample!=NULL) {
                // 存在遗留数据, 优先处理遗留采样数据
                pSample = pLeftIMFSample;
            }
            else {
                // 不存在遗留数据，那么就去和MF申请新采样数据
                LONGLONG timestamp = 0;
                // ReadSample函数给IMFSample指针赋值为指向采样数据的指针
                hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,NULL,&readerFlags,&timestamp,&pSample);
                if (FAILED(hr) or (readerFlags & MF_SOURCE_READERF_STREAMTICK)){
                    std::cerr << "warning: MF_SOURCE_READERF_STREAMTICK" << std::endl;
                    continue;
                }
                alreadyWrite = 0 ;
            }
            IMFMediaBuffer* pMediaBuffer = NULL;
            // 将采样数据 IMFSample 映射为连续内存 IMFMediaBuffer 指针
            if (pSample == NULL) break;
            hr = pSample->ConvertToContiguousBuffer(&pMediaBuffer);
            if (SUCCEEDED(hr)) {
                BYTE* pRawData = NULL;
                DWORD cbLength = 0;
                // IMFMediaBuffer内存通过调用lock方法给出真正的内存指针 pRawData, cbLength代表数据大小
                hr = pMediaBuffer->Lock(&pRawData, NULL, &cbLength);

                if (SUCCEEDED(hr)) {
                        // 数据是浮点数，因此指针转成浮点数指针
                    float* pFloatData = reinterpret_cast<float*>(pRawData);

                    // ringBuffer的方法接受的参数是采样点个数
                    UINT32 samplesToRead = cbLength / sizeof(float);

                    UINT32 actualWriteNum = ringBuffer.Write(pFloatData + alreadyWrite, samplesToRead - alreadyWrite);
                    if (actualWriteNum < samplesToRead - alreadyWrite) {
                        alreadyWrite += actualWriteNum;
                        if (pLeftIMFSample == NULL) {
                            // 之前写入没溢出，则保存这个采样pSample
                            pLeftIMFSample = pSample;
                            // AddRef()增加引用防止资源被释放
                            pLeftIMFSample->AddRef();
                        }
                    }
                    else {
                        // 完美写完数据
                        if (pLeftIMFSample != NULL){
                            // 剩余采样pSample存在, 则先释放内存
                            pLeftIMFSample->Release();
                            pLeftIMFSample = NULL;
                            alreadyWrite = 0;
                        }
                    }
                    // 解除锁定
                    pMediaBuffer->Unlock();
                }
                    // 释放资源
                pMediaBuffer->Release();
            }
            else {
                std::cerr << "some error happen in ConvertToContiguousBuffer" << std::endl;
            }
            // 释放资源
            if (pSample != pLeftIMFSample) pSample->Release();
        }

        // ==========================================
        // 2. 消费者逻辑：从蓄水池取水，喂给声卡
        // ==========================================
        // 当前已经存在的数据大小 numFramesPadding
        hr = pAudioClient->GetCurrentPadding(&numFramesPadding);

        // numFramesAvailable代表剩余需要多大数据量来填充声卡缓冲区
        numFramesAvailable = bufferFrameCount - numFramesPadding;

        if (numFramesAvailable >0 ) {
            // pData绑定缓冲区
            hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
            if (SUCCEEDED(hr)) {
                // samplesNeeded采样点数
                UINT32 samplesNeeded = numFramesAvailable * channels;

                // 转成float指针
                float* pWasapiFloatData = reinterpret_cast<float*> (pData);

                // 实际读取了多少数据
                UINT32 samplesRead = ringBuffer.Read(pWasapiFloatData, samplesNeeded);

                // std::cerr << samplesRead << std::endl;
                if (samplesRead < samplesNeeded) {
                    // 如果读取的数据量不够，那么必须将剩余的缓冲区填0
                    std::cerr << "<<< Playback Stopped112." << std::endl;
                    std::fill_n(pWasapiFloatData + samplesRead, samplesNeeded - samplesRead, 0.0f);
                }

                // 提交声卡缓存
                hr = pRenderClient->ReleaseBuffer(numFramesAvailable, 0);
            }
            else {
                std::cerr << "<<< Playback Stopped112." << std::endl;
            }
        }

        // ==========================================
        // 3. 退出条件：文件读完了，且环形缓冲区里的存货也播完了
        // ==========================================
        if ((readerFlags & MF_SOURCE_READERF_ENDOFSTREAM) and ringBuffer.getValidDataCount()==0) {
            hr = pAudioClient->GetCurrentPadding(&numFramesPadding);
            if (SUCCEEDED(hr) && numFramesPadding > 0) {
                // 计算这些没播完的帧，需要多少毫秒才能播完
                // 公式： (剩余帧数 * 1000) / 采样率
                DWORD sleepTimeMs = (numFramesPadding * 1000) / pWaveFormat->nSamplesPerSec;
                
                std::cout << "Draining WASAPI hardware buffer... waiting " 
                          << sleepTimeMs << " ms for the final notes to finish." << std::endl;
                
                // 多等 50 毫秒作为安全余量，确保绝对播完
                Sleep(sleepTimeMs + 50);
            }
            std::cout << "End of stream and buffer flushed. Stopping playback." << std::endl;
            bPlaying = false;
        }
    }
    // 关闭声卡
    pAudioClient->Stop();
    std::cout << "<<< Playback Stopped." << std::endl;
    return hr;
}


int main() {
    std::cout << "Starting Audio Engine Initialization..." << std::endl;

    // COM 和 MF 初始化
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)){
        std::cerr << "Failed to initialize COM library." << std::endl;
        return -1;
    }
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::cerr << "Failed to startup MF." << std::endl;
        CoUninitialize();
        return -1;
    }

    std::cout << "COM and Media Foundation initialized successfully!" << std::endl;

    // ==========================================
    // 核心代码：加载本地音频文件
    // ==========================================
    IMFSourceReader* pReader          = NULL;
    WAVEFORMATEX* pWaveFormat         = NULL;
    IAudioClient* pAudioClient        = NULL;
    IAudioRenderClient* pRenderClient = NULL;
    HANDLE hAudioEvent                = NULL;

    // 请替换为你电脑上一首真实存在的音频文件路径 (注意使用宽字符 L)
    LPCWSTR audioFilePath = L"C:\\Fedora40_202606\\Eglish_study\\KamalaHarris\\KamalaHarris02.mp3";

    std::wcout << L"Loading file: " << audioFilePath << std::endl;

    // 初始化解码器
    hr = InitSourceReader(audioFilePath, &pReader, &pWaveFormat);

    if (SUCCEEDED(hr)) {
        std::cout << "\n--- Audio Format Decoded Successfully ---" << std::endl;
        std::cout << "Sample Rate: " << pWaveFormat->nSamplesPerSec << " Hz" << std::endl;
        std::cout << "Channels: " << pWaveFormat->nChannels << std::endl;
        std::cout << "Bit Depth: " << pWaveFormat->wBitsPerSample << " bits" << std::endl;
        std::cout << "Format Tag: " << pWaveFormat->wFormatTag << " (3 = IEEE Float)" << std::endl;
        std::cout << "-----------------------------------------\n" << std::endl;

        // 初始化声卡
        hr = InitWASAPI(pWaveFormat, &pAudioClient, &pRenderClient, &hAudioEvent);

        if (SUCCEEDED(hr)) {
            std::cout << "-> WASAPI initialized and connected to default audio endpoint." << std::endl;
            StartPlaybackLoop(pAudioClient,pRenderClient, hAudioEvent, pReader, pWaveFormat);
            CloseHandle(hAudioEvent);
            SafeRelease(&pRenderClient);
            SafeRelease(&pAudioClient);
        }
        else {
            std::cerr << "Failed to initialize WASAPI." << std::endl;
        }
        // ==========================================
        // 清理资源
        // ==========================================
        if (pWaveFormat) {
            CoTaskMemFree(pWaveFormat); // WAVEFORMATEX 是用 CoTaskMemAlloc 分配的，必须用此函数释放
        }
        if (pReader) SafeRelease(&pReader);
    }
    else {
        std::cerr << "Failed to initialize Source Reader. Error: " << std::hex << hr << std::endl;
    }
    MFShutdown();
    CoUninitialize();

    std::cout << "Engine shut down safely." << std::endl;
    return 0;
}
