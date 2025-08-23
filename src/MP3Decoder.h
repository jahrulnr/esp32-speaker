#pragma once

#include <Arduino.h>
#include <SPIFFS.h>
#include <functional>

// Include the ESP32 Helix MP3 decoder library
extern "C" {
    #include "mp3dec.h"
}

/**
 * MP3Decoder class for decoding MP3 files
 * 
 * Uses the ESP32 Helix MP3 decoder library to decode MP3 files
 * and convert them to PCM data for playback through speakers.
 * 
 * Supports both full decoding and streaming decoding modes.
 */
class MP3Decoder {
public:
    struct MP3Info {
        int sampleRate;
        int channels;
        int bitRate;
        int duration;      // in seconds
        bool valid;
    };

    // Callback for streaming data
    using StreamCallback = std::function<bool(const int16_t* data, size_t len, MP3Info& info)>;

    MP3Decoder();
    ~MP3Decoder();

    /**
     * Initialize the MP3 decoder
     * @return true if initialization was successful
     */
    bool init();

    /**
     * Decode MP3 file from SPIFFS
     * @param filePath Path to MP3 file in SPIFFS
     * @param pcmBuffer Output buffer for PCM data (will be allocated)
     * @param pcmSize Output size of PCM data
     * @param info Output MP3 file information
     * @return true if decoding was successful
     */
    bool decodeFile(const String& filePath, int16_t** pcmBuffer, size_t* pcmSize, MP3Info* info = nullptr);

    /**
     * Decode MP3 data from memory
     * @param mp3Data Input MP3 data
     * @param mp3Size Size of MP3 data
     * @param pcmBuffer Output buffer for PCM data (will be allocated)
     * @param pcmSize Output size of PCM data
     * @param info Output MP3 file information
     * @return true if decoding was successful
     */
    bool decodeData(const uint8_t* mp3Data, size_t mp3Size, int16_t** pcmBuffer, size_t* pcmSize, MP3Info* info = nullptr);

    /**
     * Start streaming decoding of a file
     * @param filePath Path to MP3 file
     * @param callback Callback function to receive PCM data
     * @return true if successfully started streaming
     */
    bool startStreaming(const String& filePath, StreamCallback callback);

    /**
     * Process next frame in streaming mode
     * @return true if a frame was processed, false if end of stream or error
     */
    bool processStreamFrame();
    
    /**
     * Stop streaming and clean up resources
     */
    void stopStreaming();
    
    /**
     * Check if streaming is active
     * @return true if streaming is in progress
     */
    bool isStreaming() const { return _streaming; }
    
    /**
     * Get MP3 file information without full decoding
     * @param filePath Path to MP3 file
     * @param info Output MP3 information
     * @return true if information was retrieved successfully
     */
    bool getFileInfo(const String& filePath, MP3Info* info);

    /**
     * Free PCM buffer allocated by decode functions
     * @param pcmBuffer Buffer to free
     */
    void freePCMBuffer(int16_t* pcmBuffer);

    /**
     * Check if decoder is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return _initialized; }

private:
    HMP3Decoder _decoder;
    bool _initialized;
    bool _streaming;
    
    static const size_t INPUT_BUFFER_SIZE = 2048;
    static const size_t OUTPUT_BUFFER_SIZE = 4608; // Max PCM samples per frame
    static const size_t STREAM_BUFFER_SIZE = 8192; // Size of streaming buffer
    
    uint8_t* _inputBuffer;
    int16_t* _outputBuffer;
    uint8_t* _streamBuffer;     // Buffer for streaming data
    File _streamFile;           // File handle for streaming
    size_t _bytesLeft;          // Bytes left in the streaming buffer
    uint8_t* _readPtr;          // Current read position in streaming buffer
    bool _firstFrame;           // Flag for first frame processing
    MP3Info _streamInfo;        // MP3 info for streaming
    StreamCallback _callback;   // Callback for streaming data
    
    bool decodeInternal(const uint8_t* mp3Data, size_t mp3Size, int16_t** pcmBuffer, size_t* pcmSize, MP3Info* info);
    bool fillStreamBuffer();    // Fill the streaming buffer with more data
};
