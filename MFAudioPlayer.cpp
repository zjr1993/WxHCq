#include <iostream>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <format>
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

// 安全释放宏
template <class T> void SafeRelease(T** ppT) {
    if (*ppT) { (*ppT)->Release(); *ppT = NULL; }
}

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

//HRESULT ReadAndVerifyPCM(IMFSourceReader* pReader, WAVEFORMATEX* pWaveFormat) {
//    HRESULT hr = S_OK;
//    IMFSample* pSample = NULL;
//    IMFMediaBuffer* pBuffer = NULL;
//    DWORD streamIndex = MF_SOURCE_READER_FIRST_AUDIO_STREAM;
//    std::cout << "\n>>> Starting PCM Read Verification..." << std::endl;
//
//    DWORD flags = 0;
//    LONGLONG timestamp = 0;
//
//    hr = pReader->ReadSample(streamIndex, 0, NULL, &flags, &timestamp, &pSample);
//    if (FAILED(hr)) {
//        std::cerr << "ReadSample failed." << std::endl;
//        return hr;
//    }
//
//    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
//        std::cout << "Reached end of stream immediately." << std::endl;
//        return S_OK;
//    }
//
//    if (pSample) {
//        hr = pSample->ConvertToContiguousBuffer(&pBuffer);
//        if (SUCCEEDED(hr)) {
//            BYTE* pAudioData = NULL;
//            DWORD cbBufferLength = 0;
//
//            hr = pBuffer->Lock(&pAudioData, NULL, &cbBufferLength);
//            if (SUCCEEDED(hr)) {
//                DWORD bytesPerFrame = pWaveFormat->nChannels * sizeof(float);
//                DWORD numFrames = cbBufferLength / bytesPerFrame;
//                std::cout << std::format("Successfully read {} bytes of PCM data.", cbBufferLength) << std::endl;
//                std::cout << std::format("Total frames in this block: {} frames.\n", cbBufferLength) << std::endl;
//
//                float* pFloatData = reinterpret_cast<float*>(pAudioData);
//
//                std::cout << "First 5 audio frames (Values typically between -1.0 and 1.0):" << std::endl;
//
//                DWORD framesToPrint = min(numFrames, (DWORD)10);
//
//                for (DWORD i = 0; i < framesToPrint; ++i) {
//                    std::cout << "Frame " << i << ": [ ";
//                    for (WORD c = 0; c < pWaveFormat->nChannels; ++c) {
//                        std::cout << pFloatData[i * pWaveFormat->nChannels + c] << " ";
//                    }
//                    std::cout << "]" << std::endl;
//                }
//                pBuffer->Unlock();
//            }
//            pBuffer->Release();
//        }
//        pSample->Release();
//    }
//    std::cout << "<<< Verification Complete.\n" << std::endl;
//    return hr;
//}


// 连续读取 PCM 数据，跳过静音段，直到找到真实的波形数据
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




int main() {
    std::cout << "Starting Audio Engine Initialization..." << std::endl;

    // COM 和 MF 初始化
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) return -1;
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) { CoUninitialize(); return -1; }

    std::cout << "COM and Media Foundation initialized successfully!" << std::endl;

    // ==========================================
    // 核心代码：加载本地音频文件
    // ==========================================
    IMFSourceReader* pReader = NULL;
    WAVEFORMATEX* pWaveFormat = NULL;

    // 请替换为你电脑上一首真实存在的音频文件路径 (注意使用宽字符 L)
    LPCWSTR audioFilePath = L"C:\\Fedora40_202606\\Eglish_study\\KamalaHarris\\KamalaHarris01.mp3";

    std::wcout << L"Loading file: " << audioFilePath << std::endl;
    hr = InitSourceReader(audioFilePath, &pReader, &pWaveFormat);

    if (SUCCEEDED(hr)) {
        std::cout << "\n--- Audio Format Decoded Successfully ---" << std::endl;
        std::cout << "Sample Rate: " << pWaveFormat->nSamplesPerSec << " Hz" << std::endl;
        std::cout << "Channels: " << pWaveFormat->nChannels << std::endl;
        std::cout << "Bit Depth: " << pWaveFormat->wBitsPerSample << " bits" << std::endl;
        std::cout << "Format Tag: " << pWaveFormat->wFormatTag << " (3 = IEEE Float)" << std::endl;
        std::cout << "-----------------------------------------\n" << std::endl;

        ReadAndVerifyPCM(pReader, pWaveFormat);
    }
    else {
        std::cerr << "Failed to initialize Source Reader. Error: " << std::hex << hr << std::endl;
    }

    // ==========================================
    // 清理资源
    // ==========================================
    if (pWaveFormat) {
        CoTaskMemFree(pWaveFormat); // WAVEFORMATEX 是用 CoTaskMemAlloc 分配的，必须用此函数释放
    }
    SafeRelease(&pReader);

    MFShutdown();
    CoUninitialize();

    std::cout << "Engine shut down safely." << std::endl;
    return 0;
}
