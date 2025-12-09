#include "AudioSamples.h"
#include <cmath>
#include <cstdlib>

AudioSamples::AudioSamples(I2SSpeaker* speaker) 
    : _speaker(speaker), _sampleRate(16000) {
    if (_speaker && _speaker->isInitialized()) {
        _sampleRate = _speaker->getSampleRate();
    }
}

AudioSamples::~AudioSamples() {
    // No cleanup needed - we don't own the speaker
}

bool AudioSamples::playSample(SampleType sampleType, float volume) {
    if (!isReady()) {
        return false;
    }

    // Constrain volume
    volume = constrain(volume, 0.0f, 1.0f);

    switch (sampleType) {
        case BEEP_SHORT:
            return playBeep(1000, 200, volume);

        case BEEP_LONG:
            return playBeep(1000, 500, volume);

        case DOUBLE_BEEP: {
            bool result = playBeep(1000, 150, volume);
            delay(100);
            return result && playBeep(1000, 150, volume);
        }

        case TRIPLE_BEEP: {
            bool result = playBeep(1000, 100, volume);
            delay(80);
            result = result && playBeep(1000, 100, volume);
            delay(80);
            return result && playBeep(1000, 100, volume);
        }

        case CONFIRMATION: {
            int frequencies[] = {800, 1200};
            int durations[] = {150, 200};
            return playToneSequence(frequencies, durations, 2, volume, 50);
        }

        case ERROR: {
            int frequencies[] = {400, 300};
            int durations[] = {300, 300};
            return playToneSequence(frequencies, durations, 2, volume, 100);
        }

        case STARTUP: {
            // Play startup melody: C, E, G, C (octave higher)
            int frequencies[] = {523, 659, 784, 1047};
            int durations[] = {200, 200, 200, 400};
            return playToneSequence(frequencies, durations, 4, volume, 50);
        }

        case NOTIFICATION: {
            int frequencies[] = {1000, 1500, 1000};
            int durations[] = {100, 100, 100};
            return playToneSequence(frequencies, durations, 3, volume, 50);
        }

        case ALARM_SOFT:
            return playFrequencySweep(500, 800, 1000, volume);

        case ALARM_URGENT:
            return playFrequencySweep(800, 1200, 500, volume);

        case CLICK:
            return playBeep(2000, 50, volume, SQUARE);

        case SUCCESS: {
            int frequencies[] = {523, 659, 784};
            int durations[] = {150, 150, 300};
            return playToneSequence(frequencies, durations, 3, volume, 30);
        }

        case WARNING: {
            int frequencies[] = {800, 600, 800, 600};
            int durations[] = {200, 200, 200, 200};
            return playToneSequence(frequencies, durations, 4, volume, 50);
        }

        case POWER_ON: {
            int frequencies[] = {300, 400, 500, 600, 700, 800};
            int durations[] = {100, 100, 100, 100, 100, 200};
            return playToneSequence(frequencies, durations, 6, volume, 20);
        }

        case POWER_OFF: {
            int frequencies[] = {800, 700, 600, 500, 400, 300};
            int durations[] = {100, 100, 100, 100, 100, 200};
            return playToneSequence(frequencies, durations, 6, volume, 20);
        }

        default:
            return false;
    }
}

bool AudioSamples::playBeep(int frequency, int duration, float volume, WaveformType waveform) {
    if (!isReady()) {
        return false;
    }

    // Calculate buffer size
    size_t channelCount = (_speaker->getChannelMode() == I2S_SLOT_MODE_STEREO) ? 2 : 1;
    size_t samplesNeeded = (_sampleRate * duration) / 1000;
    size_t totalSamples = samplesNeeded * channelCount;

    // Allocate buffer
    int16_t* buffer = (int16_t*)malloc(totalSamples * sizeof(int16_t));
    if (!buffer) {
        return false;
    }

    // Generate waveform
    size_t samplesGenerated = generateWaveform(frequency, duration, volume, 
                                              waveform, buffer, totalSamples);

    // Apply fade to prevent clicks
    size_t fadeLength = min(samplesNeeded / 20, (size_t)(_sampleRate * 0.005)); // 5ms or 5% of duration
    applyFade(buffer, samplesGenerated, fadeLength * channelCount, fadeLength * channelCount);

    // Play the samples
    bool result = false;
    if (samplesGenerated > 0) {
        if (!_speaker->isActive()) {
            _speaker->start();
        }
        
        int samplesWritten = _speaker->writeSamples(buffer, samplesGenerated, 1000);
        result = (samplesWritten > 0);
    }

    free(buffer);
    _speaker->clear();
    return result;
}

bool AudioSamples::playToneSequence(const int* frequencies, const int* durations, 
                                   int count, float volume, int pauseBetween) {
    if (!isReady() || !frequencies || !durations || count <= 0) {
        return false;
    }

    bool allSuccess = true;
    for (int i = 0; i < count; i++) {
        bool success = playBeep(frequencies[i], durations[i], volume);
        allSuccess = allSuccess && success;
        
        if (i < count - 1 && pauseBetween > 0) {
            delay(pauseBetween);
        }
    }

    return allSuccess;
}

bool AudioSamples::playDTMF(char digit, int duration, float volume) {
    int lowFreq, highFreq;
    if (!getDTMFFrequencies(digit, &lowFreq, &highFreq)) {
        return false;
    }

    if (!isReady()) {
        return false;
    }

    // Calculate buffer size
    size_t channelCount = (_speaker->getChannelMode() == I2S_SLOT_MODE_STEREO) ? 2 : 1;
    size_t samplesNeeded = (_sampleRate * duration) / 1000;
    size_t totalSamples = samplesNeeded * channelCount;

    // Allocate buffer
    int16_t* buffer = (int16_t*)malloc(totalSamples * sizeof(int16_t));
    if (!buffer) {
        return false;
    }

    // Generate dual tone
    size_t samplesGenerated = generateDualTone(lowFreq, highFreq, duration, volume, 
                                              buffer, totalSamples);

    // Apply fade
    size_t fadeLength = min(samplesNeeded / 40, (size_t)(_sampleRate * 0.002)); // 2ms fade
    applyFade(buffer, samplesGenerated, fadeLength * channelCount, fadeLength * channelCount);

    // Play the samples
    bool result = false;
    if (samplesGenerated > 0) {
        if (!_speaker->isActive()) {
            _speaker->start();
        }
        
        int samplesWritten = _speaker->writeSamples(buffer, samplesGenerated, 1000);
        result = (samplesWritten > 0);
    }

    free(buffer);
    _speaker->clear();
    return result;
}

bool AudioSamples::playWhiteNoise(int duration, float volume) {
    if (!isReady()) {
        return false;
    }

    // Calculate buffer size
    size_t channelCount = (_speaker->getChannelMode() == I2S_SLOT_MODE_STEREO) ? 2 : 1;
    size_t samplesNeeded = (_sampleRate * duration) / 1000;
    size_t totalSamples = samplesNeeded * channelCount;

    // Allocate buffer
    int16_t* buffer = (int16_t*)malloc(totalSamples * sizeof(int16_t));
    if (!buffer) {
        return false;
    }

    // Generate white noise
    int16_t maxAmplitude = (int16_t)(volume * 32767);
    for (size_t i = 0; i < samplesNeeded; i++) {
        int16_t sample = (int16_t)((random(65536) - 32768) * volume);
        
        // Fill all channels
        for (size_t ch = 0; ch < channelCount; ch++) {
            buffer[i * channelCount + ch] = sample;
        }
    }

    // Apply fade to prevent clicks
    size_t fadeLength = min(samplesNeeded / 20, (size_t)(_sampleRate * 0.01)); // 10ms fade
    applyFade(buffer, totalSamples, fadeLength * channelCount, fadeLength * channelCount);

    // Play the samples
    bool result = false;
    if (!_speaker->isActive()) {
        _speaker->start();
    }
    
    int samplesWritten = _speaker->writeSamples(buffer, totalSamples, 1000);
    result = (samplesWritten > 0);

    free(buffer);
    _speaker->clear();
    return result;
}

bool AudioSamples::playFrequencySweep(int startFreq, int endFreq, int duration, float volume) {
    if (!isReady()) {
        return false;
    }

    // Calculate buffer size
    size_t channelCount = (_speaker->getChannelMode() == I2S_SLOT_MODE_STEREO) ? 2 : 1;
    size_t samplesNeeded = (_sampleRate * duration) / 1000;
    size_t totalSamples = samplesNeeded * channelCount;

    // Allocate buffer
    int16_t* buffer = (int16_t*)malloc(totalSamples * sizeof(int16_t));
    if (!buffer) {
        return false;
    }

    // Generate frequency sweep
    int16_t maxAmplitude = (int16_t)(volume * 32767);
    float phase = 0.0f;
    
    for (size_t i = 0; i < samplesNeeded; i++) {
        // Calculate current frequency (linear interpolation)
        float t = (float)i / samplesNeeded;
        float currentFreq = startFreq + (endFreq - startFreq) * t;
        
        // Generate sample
        float sineValue = sin(phase);
        int16_t sample = (int16_t)(sineValue * maxAmplitude);
        
        // Update phase
        phase += 2.0f * PI * currentFreq / _sampleRate;
        if (phase >= 2.0f * PI) {
            phase -= 2.0f * PI;
        }
        
        // Fill all channels
        for (size_t ch = 0; ch < channelCount; ch++) {
            buffer[i * channelCount + ch] = sample;
        }
    }

    // Apply fade
    size_t fadeLength = min(samplesNeeded / 20, (size_t)(_sampleRate * 0.01)); // 10ms fade
    applyFade(buffer, totalSamples, fadeLength * channelCount, fadeLength * channelCount);

    // Play the samples
    bool result = false;
    if (!_speaker->isActive()) {
        _speaker->start();
    }
    
    int samplesWritten = _speaker->writeSamples(buffer, totalSamples, 1000);
    result = (samplesWritten > 0);

    free(buffer);
    _speaker->clear();
    return result;
}

size_t AudioSamples::generateWaveform(int frequency, int duration, float amplitude,
                                     WaveformType waveform, int16_t* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) {
        return 0;
    }

    size_t channelCount = (_speaker->getChannelMode() == I2S_SLOT_MODE_STEREO) ? 2 : 1;
    size_t samplesPerChannel = bufferSize / channelCount;
    size_t actualSamples = _min(samplesPerChannel, (_sampleRate * duration) / 1000);

    float angularFreq = 2.0f * PI * frequency / _sampleRate;
    int16_t maxAmplitude = (int16_t)(amplitude * 32767);

    for (size_t i = 0; i < actualSamples; i++) {
        float phase = angularFreq * i;
        float sampleValue = generateWaveformSample(phase, waveform);
        int16_t sample = (int16_t)(sampleValue * maxAmplitude);
        
        // Fill all channels with the same sample
        for (size_t ch = 0; ch < channelCount; ch++) {
            buffer[i * channelCount + ch] = sample;
        }
    }

    return actualSamples * channelCount;
}

float AudioSamples::generateWaveformSample(float phase, WaveformType waveform) {
    float normalizedPhase = fmod(phase, 2.0f * PI);
    
    switch (waveform) {
        case SINE:
            return sin(normalizedPhase);
            
        case SQUARE:
            return (normalizedPhase < PI) ? 1.0f : -1.0f;
            
        case TRIANGLE:
            if (normalizedPhase < PI) {
                return (2.0f * normalizedPhase / PI) - 1.0f;
            } else {
                return 3.0f - (2.0f * normalizedPhase / PI);
            }
            
        case SAWTOOTH:
            return (normalizedPhase / PI) - 1.0f;
            
        case NOISE:
            return ((float)random(65536) / 32768.0f) - 1.0f;
            
        default:
            return sin(normalizedPhase);
    }
}

void AudioSamples::applyFade(int16_t* buffer, size_t sampleCount, 
                            size_t fadeInSamples, size_t fadeOutSamples) {
    if (!buffer || sampleCount == 0) {
        return;
    }

    // Apply fade in
    for (size_t i = 0; i < min(fadeInSamples, sampleCount); i++) {
        float fadeMultiplier = (float)i / fadeInSamples;
        buffer[i] = (int16_t)(buffer[i] * fadeMultiplier);
    }

    // Apply fade out
    size_t fadeOutStart = (sampleCount > fadeOutSamples) ? (sampleCount - fadeOutSamples) : 0;
    for (size_t i = fadeOutStart; i < sampleCount; i++) {
        float fadeMultiplier = (float)(sampleCount - i) / fadeOutSamples;
        buffer[i] = (int16_t)(buffer[i] * fadeMultiplier);
    }
}

bool AudioSamples::getDTMFFrequencies(char digit, int* lowFreq, int* highFreq) {
    // DTMF frequency table
    switch (digit) {
        case '1': *lowFreq = 697; *highFreq = 1209; return true;
        case '2': *lowFreq = 697; *highFreq = 1336; return true;
        case '3': *lowFreq = 697; *highFreq = 1477; return true;
        case 'A': *lowFreq = 697; *highFreq = 1633; return true;
        
        case '4': *lowFreq = 770; *highFreq = 1209; return true;
        case '5': *lowFreq = 770; *highFreq = 1336; return true;
        case '6': *lowFreq = 770; *highFreq = 1477; return true;
        case 'B': *lowFreq = 770; *highFreq = 1633; return true;
        
        case '7': *lowFreq = 852; *highFreq = 1209; return true;
        case '8': *lowFreq = 852; *highFreq = 1336; return true;
        case '9': *lowFreq = 852; *highFreq = 1477; return true;
        case 'C': *lowFreq = 852; *highFreq = 1633; return true;
        
        case '*': *lowFreq = 941; *highFreq = 1209; return true;
        case '0': *lowFreq = 941; *highFreq = 1336; return true;
        case '#': *lowFreq = 941; *highFreq = 1477; return true;
        case 'D': *lowFreq = 941; *highFreq = 1633; return true;
        
        default: return false;
    }
}

size_t AudioSamples::generateDualTone(int freq1, int freq2, int duration, float amplitude,
                                     int16_t* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) {
        return 0;
    }

    size_t channelCount = (_speaker->getChannelMode() == I2S_SLOT_MODE_STEREO) ? 2 : 1;
    size_t samplesPerChannel = bufferSize / channelCount;
    size_t actualSamples = _min(samplesPerChannel, (_sampleRate * duration) / 1000);

    float angularFreq1 = 2.0f * PI * freq1 / _sampleRate;
    float angularFreq2 = 2.0f * PI * freq2 / _sampleRate;
    int16_t maxAmplitude = (int16_t)(amplitude * 32767 * 0.5f); // 0.5 to prevent clipping when mixing

    for (size_t i = 0; i < actualSamples; i++) {
        float sample1 = sin(angularFreq1 * i);
        float sample2 = sin(angularFreq2 * i);
        float mixedSample = sample1 + sample2;
        int16_t sample = (int16_t)(mixedSample * maxAmplitude);
        
        // Fill all channels with the same sample
        for (size_t ch = 0; ch < channelCount; ch++) {
            buffer[i * channelCount + ch] = sample;
        }
    }

    return actualSamples * channelCount;
}

void AudioSamples::setSampleRate(uint32_t sampleRate) {
    _sampleRate = sampleRate;
}

uint32_t AudioSamples::getSampleRate() const {
    return _sampleRate;
}

bool AudioSamples::isReady() const {
    return (_speaker != nullptr && _speaker->isInitialized());
}
