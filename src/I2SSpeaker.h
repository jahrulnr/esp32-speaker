#pragma once

#include <Arduino.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"

/**
 * I2SSpeaker class for digital audio output using ESP-IDF v5+ I2S STD API
 * 
 * This implementation uses the new ESP-IDF v5.x I2S Standard driver which provides
 * better performance, cleaner API, and improved resource management compared to
 * the legacy I2S driver.
 * 
 * Compatible with audio amplifiers like MAX98357A, PCM5102A, etc.
 */
class I2SSpeaker {
public:
    /**
     * Constructor for I2S Standard speaker
     * 
     * @param dataPin I2S data output pin (DIN/SD)
     * @param clockPin I2S bit clock pin (BCLK/SCK)
     * @param wordSelectPin I2S word select pin (WS/LRCLK)
     * @param portNum I2S port number (I2S_NUM_0 or I2S_NUM_1)
     */
    I2SSpeaker(gpio_num_t dataPin, gpio_num_t clockPin, gpio_num_t wordSelectPin, 
               i2s_port_t portNum = I2S_NUM_0);

    /**
     * Destructor - properly cleans up I2S resources
     */
    ~I2SSpeaker();

    /**
     * Initialize the I2S Standard speaker
     * 
     * @param sampleRate Sample rate in Hz (8000, 16000, 22050, 44100, 48000)
     * @param bitsPerSample Bits per sample (16, 24, or 32)
     * @param channels Number of channels (1 for mono, 2 for stereo)
     * @return ESP_OK if successful, error code otherwise
     */
    esp_err_t init(uint32_t sampleRate = 16000, i2s_data_bit_width_t bitsPerSample = I2S_DATA_BIT_WIDTH_16BIT, 
                   i2s_slot_mode_t channels = I2S_SLOT_MODE_MONO);

    /**
     * Start the I2S channel (begin transmitting data)
     * 
     * @return ESP_OK if successful, error code otherwise
     */
    esp_err_t start();

    /**
     * Stop the I2S channel
     * 
     * @return ESP_OK if successful, error code otherwise
     */
    esp_err_t stop();

    /**
     * Write audio samples to the speaker
     * 
     * @param buffer Buffer containing audio samples
     * @param bufferSize Size of buffer in bytes
     * @param bytesWritten Pointer to store actual bytes written
     * @param timeoutMs Timeout in milliseconds
     * @return ESP_OK if successful, error code otherwise
     */
    esp_err_t writeAudioData(const void* buffer, size_t bufferSize, size_t* bytesWritten, 
                            uint32_t timeoutMs = 100);

    /**
     * Write audio samples from an int16_t buffer (convenience method)
     * 
     * @param buffer Buffer containing audio samples (int16_t)
     * @param sampleCount Number of samples to write
     * @param samplesWritten Pointer to store actual samples written
     * @param timeoutMs Timeout in milliseconds
     * @return ESP_OK if successful, error code otherwise
     */
    esp_err_t writeSamples(const int16_t* buffer, size_t sampleCount, size_t* samplesWritten, 
                          uint32_t timeoutMs = 100);

    /**
     * Write audio samples and return count (for compatibility with old API)
     * 
     * @param buffer Buffer containing audio samples (int16_t)
     * @param sampleCount Number of samples to write
     * @param timeoutMs Timeout in milliseconds
     * @return Number of samples written, or -1 on error
     */
    int writeSamples(const int16_t* buffer, size_t sampleCount, uint32_t timeoutMs = 100);

    /**
     * Play a simple tone at specified frequency
     * 
     * @param frequency Frequency in Hz (20-20000)
     * @param duration Duration in milliseconds
     * @param amplitude Amplitude (0.0 to 1.0)
     * @return Number of samples played, or -1 on error
     */
    int playTone(int frequency, int duration, float amplitude = 0.5f);

    /**
     * Check if the speaker is properly initialized
     * 
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

    /**
     * Check if the I2S channel is currently active
     * 
     * @return true if active, false otherwise
     */
    bool isActive() const;

		bool isPlaying() const;

    /**
     * Get current sample rate
     * 
     * @return Sample rate in Hz
     */
    uint32_t getSampleRate() const;

    /**
     * Get current bits per sample
     * 
     * @return Bits per sample
     */
    i2s_data_bit_width_t getBitsPerSample() const;

    /**
     * Get current channel mode
     * 
     * @return Channel mode (mono/stereo)
     */
    i2s_slot_mode_t getChannelMode() const;

    /**
     * Preload DMA buffers to reduce initial latency
     * 
     * @return ESP_OK if successful, error code otherwise
     */
    esp_err_t preloadDMA();

    /**
     * Calculate the optimal buffer size for given duration
     * 
     * @param durationMs Duration in milliseconds
     * @return Buffer size in bytes
     */
    size_t calculateBufferSize(uint32_t durationMs) const;

    /**
     * Generate a sine wave tone in a buffer
     * 
     * @param frequency Frequency in Hz
     * @param duration Duration in milliseconds
     * @param amplitude Amplitude (0.0 to 1.0)
     * @param buffer Buffer to store generated samples
     * @param bufferSize Size of buffer in samples
     * @return Number of samples generated
     */
    size_t generateSineWave(int frequency, int duration, float amplitude, 
                           int16_t* buffer, size_t bufferSize);

private:
    static const char* TAG;

    // Hardware configuration
    gpio_num_t _dataPin;
    gpio_num_t _clockPin;
    gpio_num_t _wordSelectPin;
    i2s_port_t _portNum;

    // I2S configuration
    uint32_t _sampleRate;
    i2s_data_bit_width_t _bitsPerSample;
    i2s_slot_mode_t _channelMode;

    // I2S handles
    i2s_chan_handle_t _txHandle;

    // State flags
    bool _initialized;
    bool _active;
		bool _playing;

    /**
     * Configure I2S Standard channel
     * 
     * @return ESP_OK if successful, error code otherwise
     */
    esp_err_t configureChannel();

    /**
     * Get bytes per sample based on bit width
     * 
     * @return Bytes per sample
     */
    size_t getBytesPerSample() const;

    /**
     * Get number of channels as integer
     * 
     * @return Number of channels (1 or 2)
     */
    size_t getChannelCount() const;
};
