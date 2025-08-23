/**
 * AudioSamplesDemo.ino
 * 
 * Demonstration of the AudioSamples class for pre-generated audio effects
 * and samples using the I2SSpeaker with ESP-IDF v5+ I2S STD API.
 * 
 * This example shows:
 * 1. Various beep types and sound effects
 * 2. DTMF (telephone) tones
 * 3. Different waveform types
 * 4. Audio sequences and patterns
 * 5. White noise generation
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
#include "AudioSamples.h"

// Pin definitions
#define I2S_DOUT_PIN GPIO_NUM_25
#define I2S_BCLK_PIN GPIO_NUM_26
#define I2S_LRC_PIN GPIO_NUM_27
#define I2S_PORT I2S_NUM_0

// Audio configuration
#define SAMPLE_RATE 16000
#define BIT_DEPTH I2S_DATA_BIT_WIDTH_16BIT
#define CHANNELS I2S_SLOT_MODE_STEREO

// Create speaker and audio samples instances
I2SSpeaker* speaker = nullptr;
AudioSamples* audioSamples = nullptr;

void demonstrateBasicSamples() {
  Serial.println("=== Basic Audio Samples ===");
  
  // Basic beeps
  Serial.println("Short beep...");
  audioSamples->playSample(AudioSamples::BEEP_SHORT, 0.6f);
  delay(500);
  
  Serial.println("Long beep...");
  audioSamples->playSample(AudioSamples::BEEP_LONG, 0.6f);
  delay(500);
  
  Serial.println("Double beep...");
  audioSamples->playSample(AudioSamples::DOUBLE_BEEP, 0.6f);
  delay(1000);
  
  Serial.println("Triple beep...");
  audioSamples->playSample(AudioSamples::TRIPLE_BEEP, 0.6f);
  delay(1000);
}

void demonstrateSystemSounds() {
  Serial.println("=== System Sound Effects ===");
  
  Serial.println("Confirmation sound...");
  audioSamples->playSample(AudioSamples::CONFIRMATION, 0.7f);
  delay(1000);
  
  Serial.println("Error sound...");
  audioSamples->playSample(AudioSamples::ERROR, 0.7f);
  delay(1000);
  
  Serial.println("Success sound...");
  audioSamples->playSample(AudioSamples::SUCCESS, 0.7f);
  delay(1000);
  
  Serial.println("Warning sound...");
  audioSamples->playSample(AudioSamples::WARNING, 0.7f);
  delay(1000);
  
  Serial.println("Notification sound...");
  audioSamples->playSample(AudioSamples::NOTIFICATION, 0.7f);
  delay(1000);
}

void demonstrateStartupShutdown() {
  Serial.println("=== Startup/Shutdown Sounds ===");
  
  Serial.println("Power on sequence...");
  audioSamples->playSample(AudioSamples::POWER_ON, 0.6f);
  delay(2000);
  
  Serial.println("Startup melody...");
  audioSamples->playSample(AudioSamples::STARTUP, 0.6f);
  delay(2000);
  
  Serial.println("Power off sequence...");
  audioSamples->playSample(AudioSamples::POWER_OFF, 0.6f);
  delay(2000);
}

void demonstrateAlarms() {
  Serial.println("=== Alarm Sounds ===");
  
  Serial.println("Soft alarm...");
  audioSamples->playSample(AudioSamples::ALARM_SOFT, 0.5f);
  delay(1500);
  
  Serial.println("Urgent alarm...");
  audioSamples->playSample(AudioSamples::ALARM_URGENT, 0.5f);
  delay(1500);
  
  Serial.println("UI click...");
  audioSamples->playSample(AudioSamples::CLICK, 0.4f);
  delay(500);
}

void demonstrateWaveforms() {
  Serial.println("=== Different Waveforms ===");
  
  const char* waveNames[] = {"Sine", "Square", "Triangle", "Sawtooth"};
  AudioSamples::WaveformType waveforms[] = {
    AudioSamples::SINE,
    AudioSamples::SQUARE,
    AudioSamples::TRIANGLE,
    AudioSamples::SAWTOOTH
  };
  
  for (int i = 0; i < 4; i++) {
    Serial.printf("Playing %s wave (440Hz)...\n", waveNames[i]);
    audioSamples->playBeep(440, 800, 0.5f, waveforms[i]);
    delay(1200);
  }
}

void demonstrateDTMFTones() {
  Serial.println("=== DTMF (Telephone) Tones ===");
  
  char digits[] = "123A456B789C*0#D";
  
  Serial.println("Playing telephone keypad tones...");
  for (int i = 0; i < strlen(digits); i++) {
    Serial.printf("DTMF: %c\n", digits[i]);
    audioSamples->playDTMF(digits[i], 200, 0.4f);
    delay(300);
  }
}

void demonstratePhoneNumber() {
  Serial.println("=== Dialing Phone Number ===");
  
  // Play a sample phone number: 555-0123
  char phoneNumber[] = "5550123";
  
  Serial.println("Dialing: 555-0123");
  for (int i = 0; i < strlen(phoneNumber); i++) {
    Serial.printf("Digit: %c\n", phoneNumber[i]);
    audioSamples->playDTMF(phoneNumber[i], 150, 0.4f);
    delay(200);
  }
}

void demonstrateFrequencySweeps() {
  Serial.println("=== Frequency Sweeps ===");
  
  Serial.println("Low to high sweep (200Hz -> 2000Hz)...");
  audioSamples->playFrequencySweep(200, 2000, 2000, 0.5f);
  delay(500);
  
  Serial.println("High to low sweep (2000Hz -> 200Hz)...");
  audioSamples->playFrequencySweep(2000, 200, 2000, 0.5f);
  delay(500);
  
  Serial.println("Quick sweep (500Hz -> 1500Hz)...");
  audioSamples->playFrequencySweep(500, 1500, 1000, 0.5f);
  delay(500);
}

void demonstrateWhiteNoise() {
  Serial.println("=== White Noise ===");
  
  Serial.println("Short burst of white noise...");
  audioSamples->playWhiteNoise(500, 0.3f);
  delay(1000);
  
  Serial.println("Longer white noise (2 seconds)...");
  audioSamples->playWhiteNoise(2000, 0.2f);
  delay(500);
}

void demonstrateCustomSequences() {
  Serial.println("=== Custom Tone Sequences ===");
  
  // Play "Happy Birthday" melody (simplified)
  Serial.println("Playing Happy Birthday melody...");
  int happyBirthday[] = {
    262, 262, 294, 262, 349, 330,  // Happy birthday to you
    262, 262, 294, 262, 392, 349,  // Happy birthday to you
    262, 262, 523, 440, 349, 330, 294,  // Happy birthday dear...
    466, 466, 440, 349, 392, 349   // Happy birthday to you
  };
  int durations[] = {
    250, 250, 500, 500, 500, 1000,
    250, 250, 500, 500, 500, 1000,
    250, 250, 500, 500, 500, 500, 1000,
    250, 250, 500, 500, 500, 1000
  };
  
  audioSamples->playToneSequence(happyBirthday, durations, 25, 0.4f, 50);
  delay(2000);
  
  // Play a simple alarm pattern
  Serial.println("Playing alarm pattern...");
  int alarmFreqs[] = {800, 1000, 800, 1000, 800, 1000};
  int alarmDurations[] = {200, 200, 200, 200, 200, 200};
  audioSamples->playToneSequence(alarmFreqs, alarmDurations, 6, 0.6f, 100);
  delay(1000);
}

void demonstrateVolumeControl() {
  Serial.println("=== Volume Control Demo ===");
  
  float volumes[] = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f};
  
  for (int i = 0; i < 5; i++) {
    Serial.printf("Playing beep at %.0f%% volume...\n", volumes[i] * 100);
    audioSamples->playBeep(1000, 300, volumes[i]);
    delay(600);
  }
  
  Serial.println("Volume fade demonstration...");
  for (float vol = 0.1f; vol <= 0.8f; vol += 0.1f) {
    audioSamples->playBeep(800, 200, vol);
    delay(250);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nAudio Samples Demonstration");
  Serial.println("===========================");
  
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
  
  // Create audio samples instance
  audioSamples = new AudioSamples(speaker);
  
  // Preload DMA buffers
  speaker->preloadDMA();
  
  Serial.println("Audio system ready!");
  Serial.printf("Configuration: %dHz, %d-bit, %s\n", 
               speaker->getSampleRate(),
               (speaker->getBitsPerSample() == I2S_DATA_BIT_WIDTH_16BIT) ? 16 : 24,
               (speaker->getChannelMode() == I2S_SLOT_MODE_MONO) ? "Mono" : "Stereo");
  Serial.printf("AudioSamples ready: %s\n", audioSamples->isReady() ? "Yes" : "No");
  Serial.println("Starting demonstrations...");
  Serial.println("=========================");
  
  delay(1000);
}

void loop() {
  // Play startup sound to begin demonstration
  Serial.println("Starting audio demonstration cycle...");
  audioSamples->playSample(AudioSamples::STARTUP, 0.5f);
  delay(2000);
  
  // Run all demonstrations
  demonstrateBasicSamples();
  delay(1000);
  
  demonstrateSystemSounds();
  delay(1000);
  
  demonstrateStartupShutdown();
  delay(1000);
  
  demonstrateAlarms();
  delay(1000);
  
  demonstrateWaveforms();
  delay(1000);
  
  demonstrateDTMFTones();
  delay(1000);
  
  demonstratePhoneNumber();
  delay(1000);
  
  demonstrateFrequencySweeps();
  delay(1000);
  
  demonstrateWhiteNoise();
  delay(1000);
  
  demonstrateCustomSequences();
  delay(1000);
  
  demonstrateVolumeControl();
  delay(1000);
  
  // End with power off sound
  Serial.println("Demonstration cycle complete!");
  audioSamples->playSample(AudioSamples::POWER_OFF, 0.5f);
  
  Serial.println("=========================");
  Serial.println("Waiting 10 seconds before next cycle...");
  delay(10000);
}
