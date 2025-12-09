/**
 * AudioStreaming.ino
 * 
 * Advanced example demonstrating audio streaming and real-time audio generation
 * using the I2SSpeaker class with ESP-IDF v5+ I2S STD API.
 * 
 * This example shows:
 * 1. Continuous audio streaming
 * 2. Real-time audio generation
 * 3. Buffer management for streaming
 * 4. Multiple audio patterns and waveforms
 * 
 * Hardware connections for MAX98357A:
 * - Connect amplifier DIN to GPIO25
 * - Connect amplifier BCLK to GPIO26  
 * - Connect amplifier LRCLK to GPIO27
 * - Connect amplifier GAIN to GND for 9dB gain
 * - Connect amplifier VDD to 3.3V and GND to ground
 * - Connect speaker + and - to amplifier output
 * 
 * Library: https://github.com/jahrulnr/esp32-speaker
 */

#include <Arduino.h>
#include "I2SSpeaker.h"

// Pin definitions
#define I2S_DOUT_PIN GPIO_NUM_25
#define I2S_BCLK_PIN GPIO_NUM_26
#define I2S_LRC_PIN GPIO_NUM_27
#define I2S_PORT I2S_NUM_0

// Audio configuration
#define SAMPLE_RATE 22050                   // 22.05kHz for better quality
#define BIT_DEPTH I2S_DATA_BIT_WIDTH_16BIT
#define CHANNELS I2S_SLOT_MODE_STEREO

// Streaming parameters
#define STREAM_BUFFER_SIZE 2048             // Buffer size in samples
#define STREAM_FREQUENCY 440.0f             // Base frequency for streaming

// Create speaker instance
I2SSpeaker* speaker = nullptr;

// Streaming variables
bool isStreaming = false;
float currentPhase = 0.0f;
int waveformType = 0; // 0=sine, 1=square, 2=triangle, 3=sawtooth

void generateWaveformSample(float frequency, float amplitude, int16_t* leftSample, int16_t* rightSample) {
  float sample = 0.0f;
  float normalizedPhase = fmod(currentPhase, 2.0f * PI);
  
  switch (waveformType) {
    case 0: // Sine wave
      sample = sin(normalizedPhase);
      break;
      
    case 1: // Square wave
      sample = (normalizedPhase < PI) ? 1.0f : -1.0f;
      break;
      
    case 2: // Triangle wave
      if (normalizedPhase < PI) {
        sample = (2.0f * normalizedPhase / PI) - 1.0f;
      } else {
        sample = 3.0f - (2.0f * normalizedPhase / PI);
      }
      break;
      
    case 3: // Sawtooth wave
      sample = (normalizedPhase / PI) - 1.0f;
      break;
  }
  
  int16_t finalSample = (int16_t)(sample * amplitude * 32767);
  *leftSample = finalSample;
  *rightSample = finalSample;
  
  // Update phase for next sample
  currentPhase += 2.0f * PI * frequency / SAMPLE_RATE;
  if (currentPhase >= 2.0f * PI) {
    currentPhase -= 2.0f * PI;
  }
}

void streamAudio(float frequency, float amplitude, int durationMs) {
  Serial.printf("Streaming %s wave at %.1fHz for %dms\n", 
               (waveformType == 0) ? "sine" : 
               (waveformType == 1) ? "square" : 
               (waveformType == 2) ? "triangle" : "sawtooth",
               frequency, durationMs);
  
  isStreaming = true;
  currentPhase = 0.0f;
  
  int16_t streamBuffer[STREAM_BUFFER_SIZE * 2]; // Stereo buffer
  size_t totalSamples = (SAMPLE_RATE * durationMs) / 1000;
  size_t samplesGenerated = 0;
  
  unsigned long startTime = millis();
  
  while (samplesGenerated < totalSamples && isStreaming) {
    // Generate buffer of samples
    size_t samplesToGenerate = min(STREAM_BUFFER_SIZE, totalSamples - samplesGenerated);
    
    for (size_t i = 0; i < samplesToGenerate; i++) {
      int16_t leftSample, rightSample;
      generateWaveformSample(frequency, amplitude, &leftSample, &rightSample);
      
      streamBuffer[i * 2] = leftSample;      // Left channel
      streamBuffer[i * 2 + 1] = rightSample; // Right channel
    }
    
    // Write buffer to speaker
    size_t samplesWritten;
    esp_err_t result = speaker->writeSamples(streamBuffer, samplesToGenerate * 2, 
                                           &samplesWritten, 100);
    
    if (result != ESP_OK) {
      Serial.printf("Streaming error: %s\n", esp_err_to_name(result));
      break;
    }
    
    samplesGenerated += samplesToGenerate;
    
    // Show progress every 1000 samples
    if (samplesGenerated % 10000 == 0) {
      float progress = (float)samplesGenerated / totalSamples * 100.0f;
      Serial.printf("Streaming progress: %.1f%%\n", progress);
    }
    
    // Small delay to prevent watchdog timeout
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  
  isStreaming = false;
  unsigned long endTime = millis();
  
  Serial.printf("Streaming completed: %d samples in %ldms\n", 
               samplesGenerated, endTime - startTime);
  speaker->clear();
}

void demonstrateFrequencySweep() {
  Serial.println("Demonstrating frequency sweep...");
  
  float startFreq = 200.0f;
  float endFreq = 2000.0f;
  int sweepDuration = 5000; // 5 seconds
  int stepMs = 50;          // 50ms per step
  int steps = sweepDuration / stepMs;
  
  isStreaming = true;
  currentPhase = 0.0f;
  waveformType = 0; // Sine wave for sweep
  
  int16_t sweepBuffer[STREAM_BUFFER_SIZE * 2];
  
  for (int step = 0; step < steps && isStreaming; step++) {
    // Calculate current frequency (logarithmic sweep)
    float t = (float)step / steps;
    float frequency = startFreq * pow(endFreq / startFreq, t);
    
    // Generate samples for this step
    size_t samplesPerStep = (SAMPLE_RATE * stepMs) / 1000;
    size_t samplesGenerated = 0;
    
    while (samplesGenerated < samplesPerStep && isStreaming) {
      size_t samplesToGenerate = min(STREAM_BUFFER_SIZE, 
                                   samplesPerStep - samplesGenerated);
      
      for (size_t i = 0; i < samplesToGenerate; i++) {
        int16_t leftSample, rightSample;
        generateWaveformSample(frequency, 0.3f, &leftSample, &rightSample);
        
        sweepBuffer[i * 2] = leftSample;
        sweepBuffer[i * 2 + 1] = rightSample;
      }
      
      // Write to speaker
      size_t samplesWritten;
      speaker->writeSamples(sweepBuffer, samplesToGenerate * 2, &samplesWritten, 100);
      
      samplesGenerated += samplesToGenerate;
    }
    
    // Show progress every 20 steps
    if (step % 20 == 0) {
      Serial.printf("Sweep progress: %.1fHz (%.1f%%)\n", 
                   frequency, (float)step / steps * 100.0f);
    }
  }
  
  isStreaming = false;
  speaker->clear();
  Serial.println("Frequency sweep completed");
}

void demonstrateWaveforms() {
  Serial.println("Demonstrating different waveforms...");
  
  const char* waveNames[] = {"Sine", "Square", "Triangle", "Sawtooth"};
  float frequency = 440.0f; // A4 note
  float amplitude = 0.4f;
  int duration = 2000; // 2 seconds each
  
  for (int wave = 0; wave < 4; wave++) {
    waveformType = wave;
    Serial.printf("Playing %s wave...\n", waveNames[wave]);
    streamAudio(frequency, amplitude, duration);
    delay(500); // Pause between waveforms
  }
}

void demonstrateStereoEffect() {
  Serial.println("Demonstrating stereo panning effect...");
  
  waveformType = 0; // Sine wave
  float frequency = 523.0f; // C5 note
  int duration = 4000; // 4 seconds
  
  isStreaming = true;
  currentPhase = 0.0f;
  
  int16_t stereoBuffer[STREAM_BUFFER_SIZE * 2];
  size_t totalSamples = (SAMPLE_RATE * duration) / 1000;
  size_t samplesGenerated = 0;
  
  while (samplesGenerated < totalSamples && isStreaming) {
    size_t samplesToGenerate = _min(STREAM_BUFFER_SIZE, totalSamples - samplesGenerated);
    
    for (size_t i = 0; i < samplesToGenerate; i++) {
      // Calculate pan position (oscillates between left and right)
      float t = (float)(samplesGenerated + i) / totalSamples;
      float panAngle = t * 4.0f * PI; // 2 complete cycles
      float leftGain = (cos(panAngle) + 1.0f) * 0.5f;   // 0.0 to 1.0
      float rightGain = (sin(panAngle) + 1.0f) * 0.5f;  // 0.0 to 1.0
      
      // Generate base sample
      float sample = sin(currentPhase);
      currentPhase += 2.0f * PI * frequency / SAMPLE_RATE;
      if (currentPhase >= 2.0f * PI) {
        currentPhase -= 2.0f * PI;
      }
      
      // Apply panning
      int16_t leftSample = (int16_t)(sample * leftGain * 0.5f * 32767);
      int16_t rightSample = (int16_t)(sample * rightGain * 0.5f * 32767);
      
      stereoBuffer[i * 2] = leftSample;
      stereoBuffer[i * 2 + 1] = rightSample;
    }
    
    // Write to speaker
    size_t samplesWritten;
    speaker->writeSamples(stereoBuffer, samplesToGenerate * 2, &samplesWritten, 100);
    
    samplesGenerated += samplesToGenerate;
    
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  
  isStreaming = false;
  Serial.println("Stereo panning effect completed");
  speaker->clear();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nI2S Audio Streaming Example");
  
  // Create and initialize speaker
  speaker = new I2SSpeaker(I2S_DOUT_PIN, I2S_BCLK_PIN, I2S_LRC_PIN, I2S_PORT);
  
  Serial.println("Initializing I2S speaker...");
  esp_err_t err = speaker->init(SAMPLE_RATE, BIT_DEPTH, CHANNELS);
  if (err != ESP_OK) {
    Serial.printf("Failed to initialize I2S speaker: %s\n", esp_err_to_name(err));
    while (1) { delay(100); }
  }
  
  Serial.println("Starting I2S speaker...");
  err = speaker->start();
  if (err != ESP_OK) {
    Serial.printf("Failed to start I2S speaker: %s\n", esp_err_to_name(err));
    while (1) { delay(100); }
  }
  
  // Preload DMA buffers
  speaker->preloadDMA();
  
  Serial.println("I2S speaker ready for streaming!");
  Serial.printf("Configuration: %dHz, %d-bit, %s\n", 
               speaker->getSampleRate(),
               (speaker->getBitsPerSample() == I2S_DATA_BIT_WIDTH_16BIT) ? 16 : 24,
               (speaker->getChannelMode() == I2S_SLOT_MODE_MONO) ? "Mono" : "Stereo");
  Serial.println("Starting audio streaming demonstrations...");
  Serial.println("========================================");
}

void loop() {
  // Demonstrate different waveforms
  demonstrateWaveforms();
  delay(1000);
  
  // Demonstrate frequency sweep
  demonstrateFrequencySweep();
  delay(1000);
  
  // Demonstrate stereo effects
  demonstrateStereoEffect();
  delay(1000);
  
  Serial.println("========================================");
  Serial.println("All demonstrations completed. Restarting in 3 seconds...");
  delay(3000);
}
