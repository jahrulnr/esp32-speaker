/**
 * I2SSpeakerBasic.ino
 * 
 * Basic example for using the I2SSpeaker class with a digital I2S audio amplifier
 * such as MAX98357A, PCM5102A, or similar audio DACs.
 * 
 * This example:
 * 1. Initializes an I2S speaker
 * 2. Starts the I2S channel
 * 3. Plays various tones and sound effects
 * 4. Demonstrates audio data writing
 * 
 * Hardware connections for MAX98357A:
 * - Connect amplifier DIN to GPIO25 (or your chosen pin)
 * - Connect amplifier BCLK to GPIO26 (or your chosen pin)
 * - Connect amplifier LRCLK to GPIO27 (or your chosen pin)
 * - Connect amplifier GAIN to GND for 9dB gain (or VDD for 15dB)
 * - Connect amplifier VDD to 3.3V and GND to ground
 * - Connect speaker + and - to amplifier output
 * 
 * Library: https://github.com/jahrulnr/esp32-speaker
 */

#include <Arduino.h>
#include "I2SSpeaker.h"

// Pin definitions - change these to match your wiring
#define I2S_DOUT_PIN GPIO_NUM_25  // I2S data output pin
#define I2S_BCLK_PIN GPIO_NUM_26  // I2S bit clock pin
#define I2S_LRC_PIN GPIO_NUM_27   // I2S left/right clock pin
#define I2S_PORT I2S_NUM_0        // I2S port number

// Audio configuration
#define SAMPLE_RATE 16000                   // 16kHz sample rate
#define BIT_DEPTH I2S_DATA_BIT_WIDTH_16BIT  // 16-bit samples
#define CHANNELS I2S_SLOT_MODE_STEREO       // Stereo output

// Create speaker instance
I2SSpeaker* speaker = nullptr;

void playToneSequence() {
  Serial.println("Playing tone sequence...");
  
  // Play musical scale: C, D, E, F, G, A, B, C
  int frequencies[] = {261, 294, 329, 349, 392, 440, 493, 523}; // C4 to C5
  const char* notes[] = {"C", "D", "E", "F", "G", "A", "B", "C"};
  
  for (int i = 0; i < 8; i++) {
    Serial.printf("Playing note %s (%d Hz)\n", notes[i], frequencies[i]);
    speaker->playTone(frequencies[i], 500, 0.5f); // 500ms duration, 50% amplitude
    delay(100); // Short pause between notes
  }
}

void playEffects() {
  Serial.println("Playing sound effects...");
  
  // Ascending alarm
  Serial.println("Ascending alarm...");
  for (int freq = 200; freq <= 1000; freq += 50) {
    speaker->playTone(freq, 100, 0.3f);
    delay(50);
  }
  
  delay(500);
  
  // Descending alarm
  Serial.println("Descending alarm...");
  for (int freq = 1000; freq >= 200; freq -= 50) {
    speaker->playTone(freq, 100, 0.3f);
    delay(50);
  }
  
  delay(500);
  
  // Beep pattern
  Serial.println("Beep pattern...");
  for (int i = 0; i < 3; i++) {
    speaker->playTone(800, 200, 0.4f);
    delay(200);
  }
}

void demonstrateRawAudio() {
  Serial.println("Demonstrating raw audio data writing...");
  
  // Generate a buffer of audio samples (sine wave)
  const int bufferSize = 1600; // 100ms at 16kHz
  int16_t audioBuffer[bufferSize * 2]; // Stereo
  
  // Generate 440Hz sine wave
  float frequency = 440.0f;
  float amplitude = 0.4f;
  
  for (int i = 0; i < bufferSize; i++) {
    float t = (float)i / SAMPLE_RATE;
    float sineValue = sin(2.0f * PI * frequency * t);
    int16_t sample = (int16_t)(sineValue * amplitude * 32767);
    
    // Stereo output (same sample for both channels)
    audioBuffer[i * 2] = sample;     // Left channel
    audioBuffer[i * 2 + 1] = sample; // Right channel
  }
  
  // Write raw audio data
  Serial.println("Writing raw audio data...");
  int samplesWritten = speaker->writeSamples(audioBuffer, bufferSize * 2, 1000);
  Serial.printf("Wrote %d samples\n", samplesWritten);
  speaker->clear();
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000); // Give serial monitor time to open
  Serial.println("\nI2S Digital Speaker Example");
  
  // Create speaker instance
  speaker = new I2SSpeaker(I2S_DOUT_PIN, I2S_BCLK_PIN, I2S_LRC_PIN, I2S_PORT);
  
  Serial.println("Initializing I2S speaker...");
  esp_err_t err = speaker->init(SAMPLE_RATE, BIT_DEPTH, CHANNELS);
  if (err != ESP_OK) {
    Serial.printf("Failed to initialize I2S speaker: %s\n", esp_err_to_name(err));
    while (1) { delay(100); } // Halt if initialization failed
  }
  
  Serial.println("Starting I2S speaker...");
  err = speaker->start();
  if (err != ESP_OK) {
    Serial.printf("Failed to start I2S speaker: %s\n", esp_err_to_name(err));
    while (1) { delay(100); } // Halt if start failed
  }
  
  // Allow time for the speaker to stabilize
  delay(500);
  
  Serial.println("I2S speaker ready!");
  Serial.printf("Configuration: %dHz, %d-bit, %s\n", 
               speaker->getSampleRate(),
               (speaker->getBitsPerSample() == I2S_DATA_BIT_WIDTH_16BIT) ? 16 : 
               (speaker->getBitsPerSample() == I2S_DATA_BIT_WIDTH_24BIT) ? 24 : 32,
               (speaker->getChannelMode() == I2S_SLOT_MODE_MONO) ? "Mono" : "Stereo");
  Serial.println("Starting audio demonstrations...");
  Serial.println("-----------------------------------");
  
  // Preload DMA buffers to reduce initial latency
  speaker->preloadDMA();
}

void loop() {
  // Play startup tone
  Serial.println("Playing startup tone...");
  speaker->playTone(1000, 300, 0.5f);
  delay(1000);
  
  // Play tone sequence
  playToneSequence();
  delay(2000);
  
  // Play sound effects
  playEffects();
  delay(2000);
  
  // Demonstrate raw audio writing
  demonstrateRawAudio();
  delay(2000);
  
  // Show speaker status
  Serial.printf("Speaker status - Initialized: %s, Active: %s\n",
               speaker->isInitialized() ? "Yes" : "No",
               speaker->isActive() ? "Yes" : "No");
  
  Serial.println("-----------------------------------");
  Serial.println("Loop complete. Restarting in 5 seconds...");
  delay(5000);
}
