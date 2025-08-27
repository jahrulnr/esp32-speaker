#include "MP3Decoder.h
#include <esp_heap_caps.h>

MP3Decoder::MP3Decoder() 
    : _decoder(nullptr), _initialized(false), _streaming(false),
      _inputBuffer(nullptr), _outputBuffer(nullptr), _streamBuffer(nullptr),
      _bytesLeft(0), _readPtr(nullptr), _firstFrame(true) {
}

MP3Decoder::~MP3Decoder() {
    if (_streaming) {
        stopStreaming();
    }
    
    if (_decoder) {
        MP3FreeDecoder(_decoder);
    }
    
    if (_inputBuffer) {
        heap_caps_free(_inputBuffer);
    }
    
    if (_outputBuffer) {
        heap_caps_free(_outputBuffer);
    }
}

bool MP3Decoder::init() {
    if (_initialized) {
        return true;
    }
    
    // Create MP3 decoder instance
    _decoder = MP3InitDecoder();
    if (!_decoder) {
        return false;
    }
    
    // Allocate input and output buffers
    _inputBuffer = (uint8_t*)heap_caps_malloc(INPUT_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
    _outputBuffer = (int16_t*)heap_caps_malloc(OUTPUT_BUFFER_SIZE * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
    
    if (!_inputBuffer || !_outputBuffer) {
        if (_inputBuffer) heap_caps_free(_inputBuffer);
        if (_outputBuffer) heap_caps_free(_outputBuffer);
        if (_decoder) MP3FreeDecoder(_decoder);
        _inputBuffer = nullptr;
        _outputBuffer = nullptr;
        _decoder = nullptr;
        return false;
    }
    
    _initialized = true;
    return true;
}

bool MP3Decoder::decodeFile(const String& filePath, int16_t** pcmBuffer, size_t* pcmSize, MP3Info* info) {
    if (!_initialized) {
        return false;
    }
    
    // Open file from SPIFFS
    File file = SPIFFS.open(filePath, "r");
    if (!file) {
        return false;
    }
    
    size_t fileSize = file.size();
    if (fileSize == 0) {
        file.close();
        return false;
    }
    
    // Allocate buffer for MP3 data
    uint8_t* mp3Data = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
    if (!mp3Data) {
        file.close();
        return false;
    }
    
    // Read entire file into memory
    size_t bytesRead = file.readBytes((char*)mp3Data, fileSize);
    file.close();
    
    if (bytesRead != fileSize) {
        heap_caps_free(mp3Data);
        return false;
    }
    
    // Decode the MP3 data
    bool result = decodeInternal(mp3Data, fileSize, pcmBuffer, pcmSize, info);
    
    heap_caps_free(mp3Data);
    return result;
}

bool MP3Decoder::decodeData(const uint8_t* mp3Data, size_t mp3Size, int16_t** pcmBuffer, size_t* pcmSize, MP3Info* info) {
    if (!_initialized || !mp3Data || mp3Size == 0) {
        return false;
    }
    
    return decodeInternal(mp3Data, mp3Size, pcmBuffer, pcmSize, info);
}

bool MP3Decoder::decodeInternal(const uint8_t* mp3Data, size_t mp3Size, int16_t** pcmBuffer, size_t* pcmSize, MP3Info* info) {
    if (!pcmBuffer || !pcmSize) {
        return false;
    }
    
    *pcmBuffer = nullptr;
    *pcmSize = 0;
    
    // Estimate PCM buffer size (MP3 compression is typically 10:1)
    size_t estimatedPCMSize = mp3Size * 10;
    int16_t* pcmData = (int16_t*)heap_caps_malloc(estimatedPCMSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
    if (!pcmData) {
        return false;
    }
    
    const uint8_t* readPtr = mp3Data;
    size_t bytesLeft = mp3Size;
    size_t pcmOffset = 0;
    size_t totalPCMSamples = 0;
    
    MP3FrameInfo frameInfo;
    bool firstFrame = true;
    
    while (bytesLeft > 0) {
        // Find sync word
        int offset = MP3FindSyncWord((unsigned char*)readPtr, bytesLeft);
        if (offset < 0) {
            break; // No more sync words found
        }
        
        readPtr += offset;
        bytesLeft -= offset;
        
        // Get frame info
        int result = MP3GetNextFrameInfo(_decoder, &frameInfo, (unsigned char*)readPtr);
        if (result != 0) {
            break; // Error getting frame info
        }
        
        // Store info from first frame
        if (firstFrame && info) {
            info->sampleRate = frameInfo.samprate;
            info->channels = frameInfo.nChans;
            info->bitRate = frameInfo.bitrate;
            info->valid = true;
            firstFrame = false;
        }
        
        // Check if we need more space for PCM data
        size_t samplesNeeded = frameInfo.outputSamps;
        if ((totalPCMSamples + samplesNeeded) * sizeof(int16_t) > estimatedPCMSize) {
            estimatedPCMSize *= 2;
            int16_t* newBuffer = (int16_t*)heap_caps_realloc(pcmData, estimatedPCMSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
            if (!newBuffer) {
                heap_caps_free(pcmData);
                return false;
            }
            pcmData = newBuffer;
        }
        
        // Decode frame
        try {
            result = MP3Decode(_decoder, (unsigned char**)&readPtr, (int*)&bytesLeft, 
                            pcmData + totalPCMSamples, 0);
        } catch(...) {
            result = 0;
        }
        
        if (result != 0) {
            if (result == ERR_MP3_INDATA_UNDERFLOW) {
                break; // End of data
            } else {
                // Other error, but continue trying to decode
                readPtr++;
                if (bytesLeft > 0)
                    bytesLeft--;
                continue;
            }
        }
        
        totalPCMSamples += frameInfo.outputSamps;
    }
    
    if (totalPCMSamples == 0) {
        heap_caps_free(pcmData);
        return false;
    }
    
    // Resize buffer to actual size
    int16_t* finalBuffer = (int16_t*)heap_caps_realloc(pcmData, totalPCMSamples * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
    if (finalBuffer) {
        pcmData = finalBuffer;
    }
    
    *pcmBuffer = pcmData;
    *pcmSize = totalPCMSamples;
    
    return true;
}

bool MP3Decoder::getFileInfo(const String& filePath, MP3Info* info) {
    if (!_initialized || !info) {
        return false;
    }
    
    // Open file
    File file = SPIFFS.open(filePath, "r");
    if (!file) {
        return false;
    }
    
    size_t fileSize = file.size();
    if (fileSize == 0) {
        file.close();
        return false;
    }
    
    // Read first few KB to get frame info
    size_t readSize = min(fileSize, (size_t)4096);
    uint8_t* buffer = (uint8_t*)heap_caps_malloc(readSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
    if (!buffer) {
        file.close();
        return false;
    }
    
    size_t bytesRead = file.readBytes((char*)buffer, readSize);
    file.close();
    
    if (bytesRead == 0) {
        heap_caps_free(buffer);
        return false;
    }
    
    // Find first frame and get info
    const uint8_t* readPtr = buffer;
    size_t bytesLeft = bytesRead;
    
    int offset = MP3FindSyncWord((unsigned char*)readPtr, bytesLeft);
    if (offset < 0) {
        heap_caps_free(buffer);
        return false;
    }
    
    readPtr += offset;
    bytesLeft -= offset;
    
    MP3FrameInfo frameInfo;
    int result = MP3GetNextFrameInfo(_decoder, &frameInfo, (unsigned char*)readPtr);
    
    heap_caps_free(buffer);
    
    if (result != 0) {
        return false;
    }
    
    info->sampleRate = frameInfo.samprate;
    info->channels = frameInfo.nChans;
    info->bitRate = frameInfo.bitrate;
    info->valid = true;

    // Print MP3 info for debugging
    Serial.printf("MP3 Info: SampleRate=%d Hz, Channels=%d, BitRate=%d kbps\n", info->sampleRate, info->channels, info->bitRate);

    // Estimate duration (rough calculation)
    if (frameInfo.bitrate > 0) {
        info->duration = (fileSize * 8) / frameInfo.bitrate;
    } else {
        info->duration = 0;
    }
    
    return true;
}

void MP3Decoder::freePCMBuffer(int16_t* pcmBuffer) {
    if (pcmBuffer) {
        heap_caps_free(pcmBuffer);
    }
}

bool MP3Decoder::startStreaming(const String& filePath, StreamCallback callback) {
    if (!_initialized || _streaming) {
        return false;
    }
    
    // Open the file
    _streamFile = SPIFFS.open(filePath, "r");
    if (!_streamFile) {
        return false;
    }
    
    // Allocate streaming buffer
    _streamBuffer = (uint8_t*)heap_caps_malloc(STREAM_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
    if (!_streamBuffer) {
        _streamFile.close();
        return false;
    }
    
    // Get file information
    if (!getFileInfo(filePath, &_streamInfo)) {
        heap_caps_free(_streamBuffer);
        _streamFile.close();
        return false;
    }
    
    // Initialize streaming state
    _callback = callback;
    _bytesLeft = 0;
    _readPtr = _streamBuffer;
    _firstFrame = true;
    _streaming = true;
    
    // Fill the buffer with initial data
    if (!fillStreamBuffer()) {
        stopStreaming();
        return false;
    }
    
    return true;
}

bool MP3Decoder::processStreamFrame() {
    if (!_streaming || !_initialized) {
        return false;
    }
    
    // Find the next MP3 frame sync word
    int offset = MP3FindSyncWord((unsigned char*)_readPtr, _bytesLeft);
    if (offset < 0) {
        // No sync word found in current buffer, try to fill more data
        if (!fillStreamBuffer()) {
            return false; // End of file or error
        }
        return processStreamFrame(); // Try again with new data
    }
    
    // Move to the sync word position
    _readPtr += offset;
    _bytesLeft -= offset;
    
    // Get frame info
    MP3FrameInfo frameInfo;
    int result = MP3GetNextFrameInfo(_decoder, &frameInfo, (unsigned char*)_readPtr);
    if (result != 0) {
        // Invalid frame, skip one byte and try again
        _readPtr++;
        _bytesLeft--;
        if (_bytesLeft == 0) {
            if (!fillStreamBuffer()) {
                return false;
            }
        }
        return processStreamFrame();
    }
    
    // Update stream info from first valid frame
    if (_firstFrame) {
        _streamInfo.sampleRate = frameInfo.samprate;
        _streamInfo.channels = frameInfo.nChans;
        _streamInfo.bitRate = frameInfo.bitrate;
        _streamInfo.valid = true;
        _firstFrame = false;
    }
    
    // Decode the frame
    int result2 = 0;
    try {
        result2 = MP3Decode(_decoder, (unsigned char**)&_readPtr, (int*)&_bytesLeft, _outputBuffer, 0);
    } catch(...) {
        result2 = -1;
    }
    
    if (result2 != 0) {
        if (result2 == ERR_MP3_INDATA_UNDERFLOW) {
            // Need more data
            if (!fillStreamBuffer()) {
                return false;
            }
            return processStreamFrame();
        } else {
            // Other error, skip this frame
            _readPtr++;
            _bytesLeft--;
            if (_bytesLeft == 0) {
                if (!fillStreamBuffer()) {
                    return false;
                }
            }
            return processStreamFrame();
        }
    }
    
    // Successfully decoded a frame
    if (_callback) {
        // Call the callback with the decoded data
        if (!_callback(_outputBuffer, frameInfo.outputSamps, _streamInfo)) {
            // Callback returned false, stop streaming
            stopStreaming();
            return false;
        }
    }
    
    // If we're running low on data, fill the buffer
    if (_bytesLeft < INPUT_BUFFER_SIZE && _streamFile.available()) {
        fillStreamBuffer();
    }
    
    return true;
}

void MP3Decoder::stopStreaming() {
    if (!_streaming) {
        return;
    }
    
    _streaming = false;
    
    if (_streamFile) {
        _streamFile.close();
    }
    
    if (_streamBuffer) {
        heap_caps_free(_streamBuffer);
        _streamBuffer = nullptr;
    }
    
    _bytesLeft = 0;
    _readPtr = nullptr;
    _callback = nullptr;
}

bool MP3Decoder::fillStreamBuffer() {
    // If no file or end of file, return false
    if (!_streamFile || !_streamFile.available()) {
        return false;
    }
    
    // If we have bytes left, move them to the beginning of the buffer
    if (_bytesLeft > 0 && _readPtr != _streamBuffer) {
        memmove(_streamBuffer, _readPtr, _bytesLeft);
    }
    
    _readPtr = _streamBuffer;
    
    // Fill the rest of the buffer
    size_t spaceLeft = STREAM_BUFFER_SIZE - _bytesLeft;
    size_t bytesRead = _streamFile.read(_streamBuffer + _bytesLeft, spaceLeft);
    
    if (bytesRead == 0) {
        return false; // No more data
    }
    
    _bytesLeft += bytesRead;
    return true;
}
