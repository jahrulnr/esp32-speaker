#include "MP3Player.h"
#include <cstring>

// Static member definitions
I2SSpeaker* MP3Player::_speaker = nullptr;
MP3Decoder MP3Player::_decoder;
bool MP3Player::_initialized = false;
bool MP3Player::_playing = false;
float MP3Player::_volume = 0.7f;
std::function<void(float)> MP3Player::_progressCallback = nullptr;
size_t MP3Player::_totalFrames = 0;
size_t MP3Player::_processedFrames = 0;

bool MP3Player::init(I2SSpeaker* speaker) {
    if (!speaker || !speaker->isInitialized()) {
        return false;
    }

    _speaker = speaker;
    
    if (!_decoder.init()) {
        return false;
    }

    _initialized = true;
    _playing = false;
    return true;
}

bool MP3Player::playFile(const String& filePath, float volume) {
    return playFileWithProgress(filePath, volume, nullptr);
}

bool MP3Player::playFileWithProgress(const String& filePath, float volume, 
                                   std::function<void(float)> progressCallback) {
    if (!_initialized || !_speaker || _playing) {
        return false;
    }

    // Set volume and progress callback
    _volume = constrain(volume, 0.0f, 1.0f);
    _progressCallback = progressCallback;
    _totalFrames = 0;
    _processedFrames = 0;

    // Ensure I2S is started
    if (!_speaker->isActive()) {
        esp_err_t err = _speaker->start();
        if (err != ESP_OK) {
            return false;
        }
    }

    // Start streaming MP3 decode
    _playing = true;
    bool success = _decoder.startStreaming(filePath, streamingCallback);
    
    if (!success) {
        _playing = false;
        return false;
    }

    // Process frames until streaming is complete
    while (_decoder.isStreaming() && _playing) {
        if (!_decoder.processStreamFrame()) {
            break;
        }
        
        // Small delay to prevent watchdog timeout
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Cleanup
    if (_decoder.isStreaming()) {
        _decoder.stopStreaming();
    }
    
    _playing = false;
    _progressCallback = nullptr;
    _speaker->clear();
    
    return success;
}

void MP3Player::stop() {
    if (_playing) {
        _playing = false;
        
        if (_decoder.isStreaming()) {
            _decoder.stopStreaming();
        }
    }
}

bool MP3Player::isPlaying() {
    return _playing;
}

void MP3Player::setVolume(float volume) {
    _volume = constrain(volume, 0.0f, 1.0f);
}

float MP3Player::getVolume() {
    return _volume;
}

bool MP3Player::getFileInfo(const String& filePath, MP3Decoder::MP3Info* info) {
    if (!_initialized || !info) {
        return false;
    }
    
    return _decoder.getFileInfo(filePath, info);
}

bool MP3Player::streamingCallback(const int16_t* data, size_t sampleCount, 
                                 MP3Decoder::MP3Info& info) {
    if (!_speaker || !_playing || !data || sampleCount == 0) {
        return false;
    }

    // Create a copy of the data to apply volume
    int16_t* volumeAdjustedSamples = (int16_t*)malloc(sampleCount * sizeof(int16_t));
    if (!volumeAdjustedSamples) {
        return false;
    }

    // Copy and apply volume
    memcpy(volumeAdjustedSamples, data, sampleCount * sizeof(int16_t));
    applyVolume(volumeAdjustedSamples, sampleCount, _volume);

    // Stream to I2S
    size_t samplesWritten;
    esp_err_t result = _speaker->writeSamples(volumeAdjustedSamples, sampleCount, 
                                            &samplesWritten, 100);

    free(volumeAdjustedSamples);

    if (result != ESP_OK) {
        return false; // Stop streaming on I2S error
    }

    // Update progress
    _processedFrames++;
    if (_progressCallback && _totalFrames > 0) {
        float progress = (float)_processedFrames / _totalFrames;
        _progressCallback(progress);
    }

    return _playing; // Continue streaming if still playing
}

void MP3Player::applyVolume(int16_t* samples, size_t sampleCount, float volume) {
    if (!samples || sampleCount == 0 || volume <= 0.0f) {
        return;
    }

    if (volume >= 1.0f) {
        return; // No adjustment needed
    }

    for (size_t i = 0; i < sampleCount; i++) {
        samples[i] = (int16_t)(samples[i] * volume);
    }
}
