#include "I2SSpeaker.h"
#include <cstring>
#include <cmath>

const char* I2SSpeaker::TAG = "I2SSpeaker";

I2SSpeaker::I2SSpeaker(gpio_num_t dataPin, gpio_num_t clockPin, gpio_num_t wordSelectPin, 
                       i2s_port_t portNum)
    : _dataPin(dataPin), _clockPin(clockPin), _wordSelectPin(wordSelectPin), _portNum(portNum),
      _sampleRate(16000), _bitsPerSample(I2S_DATA_BIT_WIDTH_16BIT), _channelMode(I2S_SLOT_MODE_STEREO),
      _txHandle(nullptr), _initialized(false), _active(false) {
    
    ESP_LOGI(TAG, "I2SSpeaker created for port %d, pins: DATA=%d, CLK=%d, WS=%d", 
             _portNum, _dataPin, _clockPin, _wordSelectPin);
}

I2SSpeaker::~I2SSpeaker() {
    if (_active) {
        stop();
    }
    
    if (_txHandle) {
        i2s_del_channel(_txHandle);
        _txHandle = nullptr;
    }
    
    ESP_LOGI(TAG, "I2SSpeaker destroyed");
}

esp_err_t I2SSpeaker::init(uint32_t sampleRate, i2s_data_bit_width_t bitsPerSample, 
                          i2s_slot_mode_t channels) {
    if (_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    _sampleRate = sampleRate;
    _bitsPerSample = bitsPerSample;
    _channelMode = channels;

    ESP_LOGI(TAG, "Initializing I2S Standard: %lu Hz, %d-bit, %s", 
             _sampleRate, 
             (_bitsPerSample == I2S_DATA_BIT_WIDTH_16BIT) ? 16 : 
             (_bitsPerSample == I2S_DATA_BIT_WIDTH_24BIT) ? 24 : 32,
             (_channelMode == I2S_SLOT_MODE_MONO) ? "mono" : "stereo");

    esp_err_t ret = configureChannel();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    _initialized = true;
    ESP_LOGI(TAG, "I2S Standard initialized successfully");
    return ESP_OK;
}

esp_err_t I2SSpeaker::configureChannel() {
    // Create I2S TX channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(_portNum, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = false; // Auto clear DMA buffer when TX underflow
    
    esp_err_t ret = i2s_new_channel(&chan_cfg, &_txHandle, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S TX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure I2S Standard
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sampleRate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(_bitsPerSample, _channelMode),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,    // Not using MCLK
            .bclk = _clockPin,
            .ws = _wordSelectPin,
            .dout = _dataPin,
            .din = I2S_GPIO_UNUSED,     // Not using input
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(_txHandle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S standard mode: %s", esp_err_to_name(ret));
        i2s_del_channel(_txHandle);
        _txHandle = nullptr;
        return ret;
    }

    ESP_LOGI(TAG, "I2S channel configured successfully");
    return ESP_OK;
}

esp_err_t I2SSpeaker::start() {
    if (!_initialized) {
        ESP_LOGE(TAG, "Speaker not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (_active) {
        ESP_LOGW(TAG, "Already started");
        return ESP_OK;
    }

    esp_err_t ret = i2s_channel_enable(_txHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    _active = true;
    ESP_LOGI(TAG, "I2S channel started");
    return ESP_OK;
}

esp_err_t I2SSpeaker::stop() {
    if (!_active) {
        return ESP_OK;
    }

    esp_err_t ret = i2s_channel_disable(_txHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    _active = false;
    ESP_LOGI(TAG, "I2S channel stopped");
    return ESP_OK;
}

esp_err_t I2SSpeaker::writeAudioData(const void* buffer, size_t bufferSize, size_t* bytesWritten, 
                                    uint32_t timeoutMs) {
    if (!_initialized) {
        ESP_LOGE(TAG, "Speaker not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!_active) {
        ESP_LOGE(TAG, "Speaker not started");
        return ESP_ERR_INVALID_STATE;
    }

    if (!buffer || bufferSize == 0) {
        ESP_LOGE(TAG, "Invalid buffer or size");
        return ESP_ERR_INVALID_ARG;
    }
        
    _playing = true;
    if (timeoutMs != portMAX_DELAY) timeoutMs = pdMS_TO_TICKS(timeoutMs);
    esp_err_t ret = i2s_channel_write(_txHandle, buffer, bufferSize, bytesWritten, 
                                     timeoutMs);
    
    if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT) {
        ESP_LOGE(TAG, "Failed to write audio data: %s", esp_err_to_name(ret));
    }

    _playing = false;
    return ret;
}

esp_err_t I2SSpeaker::writeSamples(const int16_t* buffer, size_t sampleCount, size_t* samplesWritten, 
                                  uint32_t timeoutMs) {
    if (!buffer || sampleCount == 0) {
        ESP_LOGE(TAG, "Invalid buffer or sample count");
        return ESP_ERR_INVALID_ARG;
    }

    size_t bytesToWrite = sampleCount * sizeof(int16_t);
    size_t bytesWritten;

    esp_err_t ret = writeAudioData(buffer, bytesToWrite, &bytesWritten, timeoutMs);
    
    if (samplesWritten) {
        *samplesWritten = bytesWritten / sizeof(int16_t);
    }

    return ret;
}

int I2SSpeaker::writeSamples(const int16_t* buffer, size_t sampleCount, uint32_t timeoutMs) {
    size_t samplesWritten;
    esp_err_t ret = writeSamples(buffer, sampleCount, &samplesWritten, timeoutMs);
    
    if (ret == ESP_OK) {
        return samplesWritten;
    } else {
        return -1;
    }
}

int I2SSpeaker::playTone(int frequency, int duration, float amplitude) {
    if (!_initialized) {
        ESP_LOGE(TAG, "Speaker not initialized");
        return -1;
    }

    // Constrain parameters
    frequency = constrain(frequency, 20, 20000);
    amplitude = constrain(amplitude, 0.0f, 1.0f);

    // Calculate buffer size needed
    size_t samplesNeeded = (_sampleRate * duration) / 1000;
    size_t channelCount = getChannelCount();
    size_t totalSamples = samplesNeeded * channelCount;

    // Allocate buffer
    int16_t* toneBuffer = (int16_t*)malloc(totalSamples * sizeof(int16_t));
    if (!toneBuffer) {
        ESP_LOGE(TAG, "Failed to allocate tone buffer");
        return -1;
    }

    // Generate sine wave
    size_t samplesGenerated = generateSineWave(frequency, duration, amplitude, 
                                              toneBuffer, totalSamples);

    // Play the tone
    int result = -1;
    if (samplesGenerated > 0) {
        if (!_active) {
            start();
        }
        
        result = writeSamples(toneBuffer, samplesGenerated, 1000);
    }

    free(toneBuffer);
    return result;
}

size_t I2SSpeaker::generateSineWave(int frequency, int duration, float amplitude, 
                                   int16_t* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) {
        return 0;
    }

    size_t channelCount = getChannelCount();
    size_t samplesPerChannel = bufferSize / channelCount;
    size_t actualSamples = _min(samplesPerChannel, (_sampleRate * duration) / 1000);

    float angularFreq = 2.0f * PI * frequency / _sampleRate;
    int16_t maxAmplitude = (int16_t)(amplitude * 32767);

    for (size_t i = 0; i < actualSamples; i++) {
        float sineValue = sin(angularFreq * i);
        int16_t sample = (int16_t)(sineValue * maxAmplitude);
        
        // Fill all channels with the same sample
        for (size_t ch = 0; ch < channelCount; ch++) {
            buffer[i * channelCount + ch] = sample;
        }
    }

    return actualSamples * channelCount;
}

bool I2SSpeaker::isInitialized() const {
    return _initialized;
}

bool I2SSpeaker::isActive() const {
    return _active;
}

bool I2SSpeaker::isPlaying() const {
        return _playing;
}

uint32_t I2SSpeaker::getSampleRate() const {
    return _sampleRate;
}

i2s_data_bit_width_t I2SSpeaker::getBitsPerSample() const {
    return _bitsPerSample;
}

i2s_slot_mode_t I2SSpeaker::getChannelMode() const {
    return _channelMode;
}

size_t I2SSpeaker::calculateBufferSize(uint32_t durationMs) const {
    size_t samplesPerMs = _sampleRate / 1000;
    size_t totalSamples = samplesPerMs * durationMs;
    size_t channelCount = getChannelCount();
    size_t bytesPerSample = getBytesPerSample();
    
    return totalSamples * channelCount * bytesPerSample;
}

size_t I2SSpeaker::getBytesPerSample() const {
    switch (_bitsPerSample) {
        case I2S_DATA_BIT_WIDTH_16BIT:
            return 2;
        case I2S_DATA_BIT_WIDTH_24BIT:
            return 3;
        case I2S_DATA_BIT_WIDTH_32BIT:
            return 4;
        default:
            return 2;
    }
}

size_t I2SSpeaker::getChannelCount() const {
    switch (_channelMode) {
        case I2S_SLOT_MODE_MONO:
            return 1;
        case I2S_SLOT_MODE_STEREO:
            return 2;
        default:
            return 2;
    }
}

esp_err_t I2SSpeaker::clear() {
    if (!_initialized || !_active) {
        return ESP_ERR_INVALID_STATE;
    }

    // Create a buffer of silence
    size_t bufferSize = calculateBufferSize(1000); // silence duration on ms
    uint8_t* silenceBuffer = (uint8_t*)calloc(bufferSize, 1);
    
    if (!silenceBuffer) {
        ESP_LOGE(TAG, "Failed to allocate silence buffer");
        return ESP_ERR_NO_MEM;
    }

    size_t bytesWritten;
    esp_err_t ret = writeAudioData(silenceBuffer, bufferSize, &bytesWritten, portMAX_DELAY);
    
    free(silenceBuffer);
    return ret;
}
