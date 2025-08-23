/**
 * MP3StreamingDemo.ino
 * 
 * Demonstration of streaming MP3 playback using the MP3Player class
 * that combines MP3 decoding with I2S streaming output.
 * 
 * This example shows:
 * 1. Memory-efficient MP3 streaming (no need to load entire file into RAM)
 * 2. Real-time MP3 decode → I2S output pipeline
 * 3. Volume control during playback
 * 4. Progress tracking
 * 5. Multiple MP3 file playback
 * 
 * Benefits of streaming approach:
 * - Low memory usage (only small buffers needed)
 * - Can play large MP3 files
 * - Real-time processing
 * - Responsive to user controls
 * 
 * Hardware connections for MAX98357A:
 * - Connect amplifier DIN to GPIO25
 * - Connect amplifier BCLK to GPIO26  
 * - Connect amplifier LRCLK to GPIO27
 * - Connect amplifier GAIN to GND for 9dB gain
 * - Connect amplifier VDD to 3.3V and GND to ground
 * - Connect speaker + and - to amplifier output
 * 
 * SPIFFS files needed:
 * - /audio/beep.mp3
 * - /audio/startup.mp3  
 * - /audio/notification.mp3
 * 
 * Library: https://github.com/jahrulnr/esp32-speaker
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include "I2SSpeaker.h"
#include "MP3Player.h"

// Pin definitions
#define I2S_DOUT_PIN GPIO_NUM_25
#define I2S_BCLK_PIN GPIO_NUM_26
#define I2S_LRC_PIN GPIO_NUM_27
#define I2S_PORT I2S_NUM_0

// Audio configuration
#define SAMPLE_RATE 22050
#define BIT_DEPTH I2S_DATA_BIT_WIDTH_16BIT
#define CHANNELS I2S_SLOT_MODE_STEREO

// Create speaker instance
I2SSpeaker* speaker = nullptr;

void progressCallback(float progress) {
  static int lastPercent = -1;
  int percent = (int)(progress * 100);
  
  if (percent != lastPercent && percent % 10 == 0) {
    Serial.printf("Playback progress: %d%%\n", percent);
    lastPercent = percent;
  }
}

void demonstrateBasicPlayback() {
  Serial.println("=== Basic MP3 Streaming Playback ===");
  
  // Check if files exist
  if (!SPIFFS.exists("/audio/beep.mp3")) {
    Serial.println("Warning: /audio/beep.mp3 not found");
    return;
  }
  
  Serial.println("Playing beep.mp3...");
  
  // Get file info first
  MP3Decoder::MP3Info info;
  if (MP3Player::getFileInfo("/audio/beep.mp3", &info)) {
    Serial.printf("MP3 Info: %dHz, %dch, %dkbps, %ds\n", 
                 info.sampleRate, info.channels, info.bitRate, info.duration);
  }
  
  // Play the file with streaming
  bool success = MP3Player::playFile("/audio/beep.mp3", 0.7f);
  if (success) {
    Serial.println("Playback completed successfully");
  } else {
    Serial.println("Playback failed");
  }
}

void demonstrateProgressTracking() {
  Serial.println("=== MP3 Streaming with Progress Tracking ===");
  
  if (!SPIFFS.exists("/audio/startup.mp3")) {
    Serial.println("Warning: /audio/startup.mp3 not found");
    return;
  }
  
  Serial.println("Playing startup.mp3 with progress tracking...");
  
  bool success = MP3Player::playFileWithProgress("/audio/startup.mp3", 0.6f, progressCallback);
  if (success) {
    Serial.println("Playback with progress tracking completed");
  } else {
    Serial.println("Playback failed");
  }
}

void demonstrateVolumeControl() {
  Serial.println("=== Volume Control During Streaming ===");
  
  if (!SPIFFS.exists("/audio/notification.mp3")) {
    Serial.println("Warning: /audio/notification.mp3 not found");
    return;
  }
  
  Serial.println("Playing notification.mp3 with volume changes...");
  
  // Start playback in a task so we can control volume
  xTaskCreate([](void* parameter) {
    MP3Player::playFile("/audio/notification.mp3", 0.5f);
    vTaskDelete(NULL);
  }, "MP3Playback", 8192, nullptr, 5, nullptr);
  
  // Change volume during playback
  delay(1000);
  Serial.println("Increasing volume to 80%...");
  MP3Player::setVolume(0.8f);
  
  delay(1000);
  Serial.println("Decreasing volume to 30%...");
  MP3Player::setVolume(0.3f);
  
  delay(1000);
  Serial.println("Restoring volume to 60%...");
  MP3Player::setVolume(0.6f);
  
  // Wait for playback to complete
  while (MP3Player::isPlaying()) {
    delay(100);
  }
  
  Serial.println("Volume control demo completed");
}

void demonstrateMultipleFiles() {
  Serial.println("=== Multiple File Streaming ===");
  
  const char* playlist[] = {
    "/audio/beep.mp3",
    "/audio/startup.mp3", 
    "/audio/notification.mp3"
  };
  
  const float volumes[] = {0.5f, 0.7f, 0.6f};
  
  for (int i = 0; i < 3; i++) {
    if (SPIFFS.exists(playlist[i])) {
      Serial.printf("Playing %s at %.0f%% volume...\n", playlist[i], volumes[i] * 100);
      
      unsigned long startTime = millis();
      bool success = MP3Player::playFile(playlist[i], volumes[i]);
      unsigned long endTime = millis();
      
      if (success) {
        Serial.printf("Completed in %ldms\n", endTime - startTime);
      } else {
        Serial.printf("Failed to play %s\n", playlist[i]);
      }
      
      delay(500); // Pause between tracks
    } else {
      Serial.printf("File not found: %s\n", playlist[i]);
    }
  }
}

void demonstrateInterruptPlayback() {
  Serial.println("=== Playback Interruption Demo ===");
  
  if (!SPIFFS.exists("/audio/startup.mp3")) {
    Serial.println("Warning: /audio/startup.mp3 not found");
    return;
  }
  
  // Start playback in background task
  xTaskCreate([](void* parameter) {
    Serial.println("Starting background playback...");
    MP3Player::playFile("/audio/startup.mp3", 0.6f);
    Serial.println("Background playback finished");
    vTaskDelete(NULL);
  }, "BackgroundMP3", 8192, nullptr, 5, nullptr);
  
  // Let it play for a bit
  delay(2000);
  
  if (MP3Player::isPlaying()) {
    Serial.println("Interrupting playback...");
    MP3Player::stop();
    
    // Verify it stopped
    delay(100);
    if (!MP3Player::isPlaying()) {
      Serial.println("Playback successfully interrupted");
    }
  }
  
  delay(1000);
}

void listAudioFiles() {
  Serial.println("=== Available Audio Files ===");
  
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }
  
  File root = SPIFFS.open("/audio");
  if (!root || !root.isDirectory()) {
    Serial.println("No /audio directory found");
    return;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory() && String(file.name()).endsWith(".mp3")) {
      Serial.printf("Found: %s (%d bytes)\n", file.name(), file.size());
      
      // Get MP3 info
      MP3Decoder::MP3Info info;
      if (MP3Player::getFileInfo(file.name(), &info)) {
        Serial.printf("  Format: %dHz, %dch, %dkbps\n", 
                     info.sampleRate, info.channels, info.bitRate);
      }
    }
    file = root.openNextFile();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nMP3 Streaming Demonstration");
  Serial.println("===========================");
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    while (1) { delay(100); }
  }
  
  // Create and initialize I2S speaker
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
  
  // Initialize MP3 player
  Serial.println("Initializing MP3 player...");
  if (!MP3Player::init(speaker)) {
    Serial.println("Failed to initialize MP3 player");
    while (1) { delay(100); }
  }
  
  Serial.println("MP3 streaming system ready!");
  Serial.printf("Configuration: %dHz, %d-bit, %s\n", 
               speaker->getSampleRate(),
               (speaker->getBitsPerSample() == I2S_DATA_BIT_WIDTH_16BIT) ? 16 : 24,
               (speaker->getChannelMode() == I2S_SLOT_MODE_MONO) ? "Mono" : "Stereo");
  
  // List available files
  listAudioFiles();
  
  Serial.println("Starting demonstrations...");
  Serial.println("=========================");
}

void loop() {
  // Run all demonstrations
  demonstrateBasicPlayback();
  delay(2000);
  
  demonstrateProgressTracking();
  delay(2000);
  
  demonstrateVolumeControl();
  delay(2000);
  
  demonstrateMultipleFiles();
  delay(2000);
  
  demonstrateInterruptPlayback();
  delay(2000);
  
  Serial.println("=========================");
  Serial.println("All demonstrations completed!");
  Serial.println("Restarting in 10 seconds...");
  delay(10000);
}
