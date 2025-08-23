#pragma once

#include <Arduino.h>
#include "I2SSpeaker.h"

/**
 * AudioSamples class for pre-generated audio effects and samples
 * 
 * This class provides a collection of pre-generated audio samples like beeps,
 * alarms, notification sounds, and other common audio effects that can be
 * played through an I2SSpeaker instance.
 */
class AudioSamples {
public:
    /**
     * Audio sample types
     */
    enum SampleType {
        BEEP_SHORT,         // Short beep (200ms)
        BEEP_LONG,          // Long beep (500ms)
        DOUBLE_BEEP,        // Two short beeps
        TRIPLE_BEEP,        // Three short beeps
        CONFIRMATION,       // Rising tone confirmation
        ERROR,              // Descending error tone
        STARTUP,            // Startup melody
        NOTIFICATION,       // Notification chime
        ALARM_SOFT,         // Soft alarm sound
        ALARM_URGENT,       // Urgent alarm sound
        CLICK,              // UI click sound
        SUCCESS,            // Success sound
        WARNING,            // Warning sound
        POWER_ON,           // Power on sound
        POWER_OFF           // Power off sound
    };

    /**
     * Waveform types for tone generation
     */
    enum WaveformType {
        SINE,               // Smooth sine wave
        SQUARE,             // Sharp square wave
        TRIANGLE,           // Triangle wave
        SAWTOOTH,           // Sawtooth wave
        NOISE               // White noise
    };

    /**
     * Constructor
     * 
     * @param speaker Pointer to initialized I2SSpeaker instance
     */
    AudioSamples(I2SSpeaker* speaker);

    /**
     * Destructor
     */
    ~AudioSamples();

    /**
     * Play a pre-defined audio sample
     * 
     * @param sampleType Type of sample to play
     * @param volume Volume level (0.0 to 1.0)
     * @return true if successful, false otherwise
     */
    bool playSample(SampleType sampleType, float volume = 0.5f);

    /**
     * Generate and play a custom beep
     * 
     * @param frequency Frequency in Hz
     * @param duration Duration in milliseconds
     * @param volume Volume level (0.0 to 1.0)
     * @param waveform Waveform type
     * @return true if successful, false otherwise
     */
    bool playBeep(int frequency, int duration, float volume = 0.5f, 
                  WaveformType waveform = SINE);

    /**
     * Play a sequence of tones
     * 
     * @param frequencies Array of frequencies
     * @param durations Array of durations (in ms)
     * @param count Number of tones in sequence
     * @param volume Volume level (0.0 to 1.0)
     * @param pauseBetween Pause between tones in ms
     * @return true if successful, false otherwise
     */
    bool playToneSequence(const int* frequencies, const int* durations, 
                         int count, float volume = 0.5f, int pauseBetween = 50);

    /**
     * Play DTMF (telephone) tone
     * 
     * @param digit Digit to play (0-9, *, #, A-D)
     * @param duration Duration in milliseconds
     * @param volume Volume level (0.0 to 1.0)
     * @return true if successful, false otherwise
     */
    bool playDTMF(char digit, int duration = 200, float volume = 0.5f);

    /**
     * Generate white noise
     * 
     * @param duration Duration in milliseconds
     * @param volume Volume level (0.0 to 1.0)
     * @return true if successful, false otherwise
     */
    bool playWhiteNoise(int duration, float volume = 0.3f);

    /**
     * Play a frequency sweep
     * 
     * @param startFreq Starting frequency in Hz
     * @param endFreq Ending frequency in Hz
     * @param duration Total duration in milliseconds
     * @param volume Volume level (0.0 to 1.0)
     * @return true if successful, false otherwise
     */
    bool playFrequencySweep(int startFreq, int endFreq, int duration, float volume = 0.5f);

    /**
     * Generate a custom waveform sample
     * 
     * @param frequency Frequency in Hz
     * @param duration Duration in milliseconds
     * @param amplitude Amplitude (0.0 to 1.0)
     * @param waveform Waveform type
     * @param buffer Output buffer for samples
     * @param bufferSize Size of buffer in samples
     * @return Number of samples generated
     */
    size_t generateWaveform(int frequency, int duration, float amplitude,
                           WaveformType waveform, int16_t* buffer, size_t bufferSize);

    /**
     * Set default sample rate for generated samples
     * 
     * @param sampleRate Sample rate in Hz
     */
    void setSampleRate(uint32_t sampleRate);

    /**
     * Get current sample rate
     * 
     * @return Current sample rate in Hz
     */
    uint32_t getSampleRate() const;

    /**
     * Check if speaker is available and ready
     * 
     * @return true if speaker is ready, false otherwise
     */
    bool isReady() const;

private:
    I2SSpeaker* _speaker;
    uint32_t _sampleRate;
    
    /**
     * Generate a single waveform sample
     * 
     * @param phase Current phase (0 to 2*PI)
     * @param waveform Waveform type
     * @return Sample value (-1.0 to 1.0)
     */
    float generateWaveformSample(float phase, WaveformType waveform);

    /**
     * Apply fade in/out to prevent audio clicks
     * 
     * @param buffer Buffer containing samples
     * @param sampleCount Number of samples
     * @param fadeInSamples Number of samples for fade in
     * @param fadeOutSamples Number of samples for fade out
     */
    void applyFade(int16_t* buffer, size_t sampleCount, 
                   size_t fadeInSamples, size_t fadeOutSamples);

    /**
     * Generate DTMF tone pair
     * 
     * @param digit Character to generate tone for
     * @param lowFreq Output for low frequency
     * @param highFreq Output for high frequency
     * @return true if valid digit, false otherwise
     */
    bool getDTMFFrequencies(char digit, int* lowFreq, int* highFreq);

    /**
     * Mix two frequencies together
     * 
     * @param freq1 First frequency
     * @param freq2 Second frequency
     * @param duration Duration in milliseconds
     * @param amplitude Amplitude (0.0 to 1.0)
     * @param buffer Output buffer
     * @param bufferSize Buffer size in samples
     * @return Number of samples generated
     */
    size_t generateDualTone(int freq1, int freq2, int duration, float amplitude,
                           int16_t* buffer, size_t bufferSize);
};
