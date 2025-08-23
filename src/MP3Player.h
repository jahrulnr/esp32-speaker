#pragma once

#include "I2SSpeaker.h"
#include "MP3Decoder.h"

/**
 * MP3Player class that combines MP3 decoding with I2S output streaming
 * 
 * This class provides a simple interface for playing MP3 files using
 * streaming decode + streaming I2S output for memory efficiency.
 */
class MP3Player {
public:
    /**
     * Initialize MP3 player with I2S speaker
     * 
     * @param speaker Pointer to initialized I2SSpeaker
     */
    static bool init(I2SSpeaker* speaker);

    /**
     * Play MP3 file with streaming (memory efficient)
     * 
     * @param filePath Path to MP3 file in SPIFFS
     * @param volume Volume level (0.0 to 1.0)
     * @return true if playback started successfully
     */
    static bool playFile(const String& filePath, float volume = 0.7f);

    /**
     * Play MP3 file with streaming and progress callback
     * 
     * @param filePath Path to MP3 file
     * @param volume Volume level (0.0 to 1.0)
     * @param progressCallback Callback for playback progress
     * @return true if playback completed successfully
     */
    static bool playFileWithProgress(const String& filePath, float volume = 0.7f,
                                   std::function<void(float)> progressCallback = nullptr);

    /**
     * Stop current playback
     */
    static void stop();

    /**
     * Check if currently playing
     * 
     * @return true if playing
     */
    static bool isPlaying();

    /**
     * Set volume during playback
     * 
     * @param volume Volume level (0.0 to 1.0)
     */
    static void setVolume(float volume);

    /**
     * Get current volume
     * 
     * @return Current volume level
     */
    static float getVolume();

    /**
     * Get MP3 file information without playing
     * 
     * @param filePath Path to MP3 file
     * @param info Output MP3 information
     * @return true if successful
     */
    static bool getFileInfo(const String& filePath, MP3Decoder::MP3Info* info);

private:
    static I2SSpeaker* _speaker;
    static MP3Decoder _decoder;
    static bool _initialized;
    static bool _playing;
    static float _volume;
    static std::function<void(float)> _progressCallback;
    static size_t _totalFrames;
    static size_t _processedFrames;

    /**
     * Internal streaming callback for MP3 decoder
     * 
     * @param data PCM audio data
     * @param sampleCount Number of samples
     * @param info MP3 stream information
     * @return true to continue streaming, false to stop
     */
    static bool streamingCallback(const int16_t* data, size_t sampleCount, 
                                MP3Decoder::MP3Info& info);

    /**
     * Apply volume to PCM samples
     * 
     * @param samples Input/output sample buffer
     * @param sampleCount Number of samples
     * @param volume Volume multiplier (0.0 to 1.0)
     */
    static void applyVolume(int16_t* samples, size_t sampleCount, float volume);
};
