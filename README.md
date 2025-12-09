# ESP32 Speaker Library

A comprehensive audio output library for ESP32 microcontrollers with I2S digital audio amplifiers, MP3 playback, and audio effects.

## Overview

This library provides a complete audio ecosystem for ESP32 devices using the new ESP-IDF v5+ I2S Standard driver. It includes I2S speaker control, MP3 streaming playback, and pre-generated audio effects for embedded applications.

Perfect for robotics projects, IoT devices, audio playback systems, notification sounds, and any application requiring high-quality digital audio output with minimal memory footprint.

## Features

- **Modern I2S Driver**: Uses ESP-IDF v5+ I2S Standard API for better performance and reliability
- **MP3 Streaming Playback**: Memory-efficient MP3 file playback with real-time streaming decode
- **Audio Effects Library**: Pre-generated sound effects including beeps, alarms, and notifications
- **Flexible Configuration**: Support for various sample rates, bit depths, and channel configurations
- **Real-time Audio**: Optimized for low-latency streaming and real-time audio generation
- **Multiple Audio Formats**: Support for 16-bit, 24-bit, and 32-bit audio
- **Stereo/Mono Output**: Configurable channel modes for different audio setups
- **Hardware Abstraction**: Clean API that isolates your application from I2S complexity
- **Memory Efficient**: Streaming architecture for playing large audio files with minimal RAM
- **Volume Control**: Real-time volume adjustment during playback
- **Buffer Management**: Manual buffer clearing and playing state tracking for clean audio transitions

## Hardware Compatibility

- **All ESP32 family**: ESP32, ESP32-S2, ESP32-S3, ESP32-C3
- **Audio Amplifiers**: MAX98357A, PCM5102A, UDA1334A, and other I2S DACs
- **Development Frameworks**: Arduino ESP32 and ESP-IDF
- **MP3 Decoder**: Native ESP-IDF MP3 decoder (built-in)
- **File Systems**: SPIFFS, LittleFS, SD card support for audio files

## Basic Usage

### Simple Audio Output

```cpp
#include "I2SSpeaker.h"

// Create an I2S speaker instance
// Parameters: data pin, clock pin, word select pin, I2S port
I2SSpeaker* speaker = new I2SSpeaker(GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27);

void setup() {
  Serial.begin(115200);
  
  // Initialize with desired configuration
  esp_err_t err = speaker->init(16000);  // 16kHz sample rate
  if (err != ESP_OK) {
    Serial.println("Speaker initialization failed");
    return;
  }
  
  // Start the I2S interface
  speaker->start();
  
  Serial.println("Speaker ready");
}

void loop() {
  // Play a simple tone
  speaker->playTone(440, 1000, 0.5f);  // 440Hz, 1 second, 50% amplitude
  delay(2000);
  
  // Write raw audio data
  int16_t audioSamples[1600]; // 100ms at 16kHz
  // ... fill buffer with audio data ...
  speaker->writeSamples(audioSamples, 1600);
  
  delay(1000);
}
```

### MP3 Streaming Playback

```cpp
#include "I2SSpeaker.h"
#include "MP3Player.h"

// Initialize I2S speaker
I2SSpeaker* speaker = new I2SSpeaker(GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27);

void setup() {
  // Initialize speaker
  speaker->init(22050, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  speaker->start();
  
  // Initialize MP3 player
  MP3Player::init(speaker);
  
  // Play MP3 file with streaming (memory efficient)
  bool success = MP3Player::playFile("/audio/music.mp3", 0.7f);
  if (success) {
    Serial.println("MP3 playback started");
  }
}
```

### Audio Effects

```cpp
#include "I2SSpeaker.h"
#include "AudioSamples.h"

I2SSpeaker* speaker = new I2SSpeaker(GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27);
AudioSamples* effects = nullptr;

void setup() {
  speaker->init(16000);
  speaker->start();
  
  // Create audio effects generator
  effects = new AudioSamples(speaker);
}

void loop() {
  // Play various sound effects
  effects->playSample(AudioSamples::BEEP_SHORT);
  delay(1000);
  
  effects->playSample(AudioSamples::NOTIFICATION);
  delay(1000);
  
  effects->playSample(AudioSamples::SUCCESS);
  delay(2000);
}
```

## API Reference

### I2SSpeaker Class

The core I2S audio output class using ESP-IDF v5+ I2S Standard driver.

#### Constructor
```cpp
I2SSpeaker(gpio_num_t dataPin, gpio_num_t clockPin, gpio_num_t wordSelectPin, i2s_port_t portNum = I2S_NUM_0)
```
- `dataPin`: I2S data output pin (DIN/SD)
- `clockPin`: I2S bit clock pin (BCLK/SCK)
- `wordSelectPin`: I2S word select pin (WS/LRCLK)
- `portNum`: I2S port number (I2S_NUM_0 or I2S_NUM_1)

#### Core Methods
- `esp_err_t init(uint32_t sampleRate, i2s_data_bit_width_t bitsPerSample, i2s_slot_mode_t channels)`: Initialize I2S
- `esp_err_t start()`: Start I2S channel
- `esp_err_t stop()`: Stop I2S channel
- `int playTone(int frequency, int duration, float amplitude)`: Play a tone
- `int writeSamples(const int16_t* buffer, size_t sampleCount, uint32_t timeoutMs)`: Write audio samples

#### Status Methods
- `bool isInitialized() const`: Check if I2S is initialized
- `bool isActive() const`: Check if I2S channel is active
- `bool isPlaying() const`: Check if currently playing audio
- `uint32_t getSampleRate() const`: Get sample rate
- `i2s_data_bit_width_t getBitsPerSample() const`: Get bits per sample
- `i2s_slot_mode_t getChannelMode() const`: Get channel mode
- `esp_err_t clear()`: Clear speaker buffer with silence

### MP3Player Class

Static interface for streaming MP3 playback with minimal memory usage.

#### Core Methods
- `static bool init(I2SSpeaker* speaker)`: Initialize with I2S speaker
- `static bool playFile(const String& filePath, float volume)`: Play MP3 file with streaming
- `static bool playFileWithProgress(const String& filePath, float volume, callback)`: Play with progress tracking
- `static void stop()`: Stop current playback
- `static bool isPlaying()`: Check playback status
- `static void setVolume(float volume)`: Adjust volume during playback
- `static bool getFileInfo(const String& filePath, MP3Decoder::MP3Info* info)`: Get MP3 file information

### AudioSamples Class

Pre-generated audio effects and sound samples.

#### Sample Types
- `BEEP_SHORT`, `BEEP_LONG`: Basic beep sounds
- `DOUBLE_BEEP`, `TRIPLE_BEEP`: Multiple beep sequences
- `CONFIRMATION`, `SUCCESS`: Positive feedback sounds
- `ERROR`, `WARNING`: Alert and warning sounds
- `NOTIFICATION`: Gentle notification chime
- `STARTUP`, `POWER_ON`, `POWER_OFF`: System sounds
- `ALARM_SOFT`, `ALARM_URGENT`: Alarm sounds

#### Core Methods
- `AudioSamples(I2SSpeaker* speaker)`: Constructor with speaker instance
- `bool playSample(SampleType type, float volume)`: Play predefined sample
- `bool playBeep(int frequency, int duration, float amplitude)`: Custom beep
- `bool playToneSequence(const int* frequencies, const int* durations, size_t count)`: Play sequence
- `bool generateWaveform(WaveformType type, int frequency, int duration, float amplitude)`: Generate waveforms

## Hardware Connections

### MAX98357A I2S Audio Amplifier

```
ESP32          MAX98357A
GPIO25    →    DIN (Data Input)
GPIO26    →    BCLK (Bit Clock)
GPIO27    →    LRC (Left/Right Clock)
3.3V      →    VIN
GND       →    GND
           →    GAIN → GND (9dB) or VIN (15dB)
           →    Speaker + and - connections
```

### PCM5102A I2S DAC

```
ESP32          PCM5102A
GPIO25    →    DIN
GPIO26    →    BCK
GPIO27    →    LCK
3.3V      →    VCC
GND       →    GND
           →    AGND → GND
           →    OUTL/OUTR → Audio output
```

## Advanced Usage

### Streaming MP3 Playback with Progress

```cpp
#include "I2SSpeaker.h"
#include "MP3Player.h"

I2SSpeaker* speaker = new I2SSpeaker(GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27);

void progressCallback(float progress) {
  Serial.printf("Playback progress: %.1f%%\n", progress * 100);
}

void setup() {
  // Initialize speaker
  speaker->init(22050, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  speaker->start();
  
  // Initialize MP3 player
  MP3Player::init(speaker);
  
  // Play with progress tracking
  MP3Player::playFileWithProgress("/audio/song.mp3", 0.8f, progressCallback);
}

void loop() {
  // Volume control during playback
  if (MP3Player::isPlaying()) {
    // Fade volume down
    for (float vol = 0.8f; vol >= 0.2f; vol -= 0.1f) {
      MP3Player::setVolume(vol);
      delay(500);
    }
    
    // Fade volume up
    for (float vol = 0.2f; vol <= 0.8f; vol += 0.1f) {
      MP3Player::setVolume(vol);
      delay(500);
    }
  }
}
```

### Custom Audio Effects

```cpp
#include "AudioSamples.h"

AudioSamples* effects = new AudioSamples(speaker);

void playNotificationSequence() {
  // Play notification sequence
  effects->playSample(AudioSamples::NOTIFICATION, 0.6f);
  delay(200);
  effects->playSample(AudioSamples::BEEP_SHORT, 0.4f);
  delay(100);
  effects->playSample(AudioSamples::BEEP_SHORT, 0.4f);
}

void playAlarmSequence() {
  // Custom alarm sequence with different frequencies
  int frequencies[] = {800, 1000, 800, 1000, 800};
  int durations[] = {200, 200, 200, 200, 500};
  
  effects->playToneSequence(frequencies, durations, 5);
}

void playStartupSound() {
  // Generate custom waveform
  effects->generateWaveform(AudioSamples::SINE, 440, 300, 0.5f);
  delay(100);
  effects->generateWaveform(AudioSamples::SINE, 554, 300, 0.5f);
  delay(100);
  effects->generateWaveform(AudioSamples::SINE, 659, 500, 0.5f);
}
```

### Real-time Audio Synthesis

```cpp
// Real-time synthesis task
void audioSynthesisTask(void* parameter) {
  const int bufferSize = 512;
  int16_t audioBuffer[bufferSize * 2]; // Stereo
  float phase = 0.0f;
  
  while (true) {
    // Generate audio in real-time
    for (int i = 0; i < bufferSize; i++) {
      float sample = generateSynthSample(&phase);
      int16_t intSample = (int16_t)(sample * 32767);
      
      audioBuffer[i * 2] = intSample;     // Left
      audioBuffer[i * 2 + 1] = intSample; // Right
    }
    
    // Stream to hardware
    speaker->writeSamples(audioBuffer, bufferSize * 2, 100);
    
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// Start synthesis task
xTaskCreate(audioSynthesisTask, "AudioSynth", 4096, nullptr, 5, nullptr);
```

## Performance Optimization

### Buffer Size Guidelines
- **Low Latency Audio**: 64-256 samples per buffer
- **Standard Audio Playback**: 512-1024 samples per buffer  
- **High Quality Streaming**: 1024-4096 samples per buffer
- **MP3 Streaming**: Automatic buffer management (2-8KB chunks)

### Real-time Considerations
```cpp
// Use appropriate task priorities for audio threads
xTaskCreatePinnedToCore(audioTask, "Audio", 4096, nullptr, 
                       configMAX_PRIORITIES - 1, nullptr, 1);

// For MP3 playback, the library handles buffering automatically
MP3Player::init(speaker); // Handles all streaming optimization internally
```

## Examples

The library includes several comprehensive example applications:

- **I2SSpeakerBasic**: Basic tone generation and raw audio output
- **MP3StreamingDemo**: Complete MP3 streaming playback demonstration
- **AudioEffectsDemo**: Pre-generated sound effects and audio samples
- **SynthDemo**: Real-time audio synthesis and waveform generation
- **VolumeControlDemo**: Dynamic volume control during playback

## Project Integration

### PlatformIO Dependencies
Add to your `platformio.ini`:
```ini
# No external MP3 libraries needed - uses built-in ESP-IDF Helix decoder
build_flags = 
    -DBOARD_HAS_PSRAM    ; Enable PSRAM for ESP32-S3
```

### Arduino IDE Setup
1. No external libraries required - uses built-in ESP-IDF Helix MP3 decoder
2. Copy esp32-speaker folder to your Arduino libraries directory
3. Include the headers: `#include "I2SSpeaker.h"`, `#include "MP3Player.h"`

### SPIFFS File System
For MP3 playback, prepare your SPIFFS with audio files:
```
data/
  audio/
    music.mp3
    notification.mp3
    startup.mp3
```

Upload SPIFFS using PlatformIO: `pio run -t uploadfs`

## Why Choose This Library?

1. **Complete Audio Ecosystem**: I2S output + MP3 playback + Audio effects in one package
2. **Memory Efficient**: Streaming MP3 decode with constant 20KB RAM usage vs MB traditional approaches
3. **Modern Architecture**: Built on ESP-IDF v5+ I2S Standard driver with better performance
4. **Real-world Ready**: Designed for embedded systems with limited resources
5. **Easy Integration**: Simple static interfaces for common audio tasks
6. **Hardware Flexible**: Works with various I2S audio hardware (MAX98357A, PCM5102A, etc.)
7. **Production Tested**: Used in robotics and IoT projects requiring reliable audio output
8. **Comprehensive Examples**: Complete working demonstrations for all features

## Compatibility

- **ESP-IDF**: v5.0 and later (uses modern I2S Standard driver)
- **Arduino ESP32**: v2.0.0 and later 
- **Hardware**: All ESP32 family microcontrollers (ESP32, ESP32-S2, ESP32-S3, ESP32-C3)
- **Audio Hardware**: I2S DACs and amplifiers (MAX98357A, PCM5102A, UDA1334A, etc.)
- **File Systems**: SPIFFS, LittleFS, SD card support for MP3 files
- **Dependencies**: Native ESP-IDF Helix MP3 decoder (no external libraries required)

## Troubleshooting

### Common Issues

**No Audio Output:**
- Check I2S pin connections (DIN, BCLK, LRC)
- Verify speaker initialization: `speaker->isInitialized()`
- Ensure I2S channel is started: `speaker->start()`
- Check volume levels and amplifier gain settings

**MP3 Playback Issues:**
- Verify SPIFFS is mounted and files exist
- Check MP3 file format (supported: 8-48kHz, mono/stereo)
- Initialize MP3Player: `MP3Player::init(speaker)`
- Monitor memory usage for very long files

**Distorted Audio:**
- Check sample rate matching between source and I2S config
- Verify bit depth configuration (16-bit recommended for most cases)
- Ensure adequate power supply for amplifier
- Check for buffer underruns in high CPU load scenarios
- Use `speaker->clear()` between audio segments to prevent bleed-through

**Build Errors:**
- Ensure ESP-IDF v5.0+ and Arduino ESP32 v2.0+ (includes Helix MP3 decoder)
- Include required headers: `#include "I2SSpeaker.h"`
- No external MP3 libraries needed - uses built-in ESP-IDF components

## License

MIT License - See LICENSE file for details.

## Contributing

Contributions are welcome! Please read the contributing guidelines and submit pull requests for any improvements.

## Support

For issues, questions, or contributions, please visit the [GitHub repository](https://github.com/jahrulnr/esp32-speaker).
