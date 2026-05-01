/*
 * Project Motion2MidiPeripheral
 * Author: Jacob Schjødt Worm
 * Date:
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Ble.h"
#include "Particle.h"
#include "adxl343.h"
#include "features.h"
#include "random_forest_model1.h"
#include <math.h>

#define BUF_LENGTH 200
#define NUM_SAMPLES 100
#define N_CLASSES 8
#define HYST_THRESHOLD_STAY 0.5
#define HYST_THRESHOLD_CHANGE 0.6

// Forward declarations:
void readAccelerometer();

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO);

// Declaring acceleromet-related variables and accelerometer timer
ADXL343 accelerometer;
Timer accelerometerTicker(10, readAccelerometer); // 10ms: samplerate 100Hz

unsigned long ms, microsStart, msPrev, microsDuration;
float accelX = 0.0f;
float accelY = 0.0f;
float accelZ = 0.0f;

// Ringbuffer til accelerometerdata
float accelCircBuf[3][BUF_LENGTH] = {0.0f};
uint16_t writePos = 0;

bool newSample = false;

// Buffer og struct til featureberegning
float samples[3][NUM_SAMPLES] = {0};

typedef struct {
  float x[NUM_SAMPLES];
  float y[NUM_SAMPLES];
  float z[NUM_SAMPLES];
  float mag[NUM_SAMPLES];
} Sample_buf;

Sample_buf sampleBuffer;

enum State { STATE_WAITING, STATE_CONNECT, STATE_RUNNING, STATE_FINISH };
State state = STATE_WAITING;

int prev_out = 4;

void readAccelerometer() {
  // add timestamp im millis
  // ms = millis() - msStart;
  accelerometer.readAccelerationG(&accelX, &accelY, &accelZ);
  accelX *= 1000;
  accelY *= 1000;
  accelZ *= 1000;
  // Log.info("X: %0.3f g, Y: %0.3f g, Z: %0.3f g", accelX, accelY, accelZ);
  accelCircBuf[0][writePos] = accelX;
  accelCircBuf[1][writePos] = accelY;
  accelCircBuf[2][writePos] = accelZ;

  writePos += 1;
  writePos = writePos % BUF_LENGTH;
  newSample = true;
}

// setup() runs once, when the device is first turned on
void setup() {
  Particle.connect();
  initBle();

  // initialization
  Serial.begin(115200);
  Log.info("Initializing ADXL343...");
  if (!accelerometer.begin()) {
    Log.error("Failed to initialize ADXL343.");
  }
  accelerometerTicker.start();
  msPrev = millis();
}

void loop() {

  if ((millis() - msPrev) > 100) { // Kør inferencing hver 100 milliSekunder
    microsStart = micros();
    msPrev = millis();
    uint16_t writePosTemp = writePos; // Gemmer writeposition så sampling ikke ændrer den
    for (int i = 0; i < 3; i++) { // Fylder buffer med 100 samples
      for (int j = 0; j < NUM_SAMPLES; j++) {
        int readPos =
            (writePosTemp + BUF_LENGTH - NUM_SAMPLES + j) % BUF_LENGTH;
        samples[i][j] = accelCircBuf[i][readPos];
      }
    }
    int n = NUM_SAMPLES;
    for (int i = 0; i < n; i++) {
      sampleBuffer.x[i] = samples[0][i];
      sampleBuffer.y[i] = samples[1][i];
      sampleBuffer.z[i] = samples[2][i];
      sampleBuffer.mag[i] =
          sqrt(samples[0][i] * samples[0][i] + samples[1][i] * samples[1][i] +
               samples[2][i] * samples[2][i]);
    }
    Features_t feats;
    microsStart = micros();
    extract_features(&sampleBuffer.x[0], &sampleBuffer.y[0], &sampleBuffer.z[0],
                     &sampleBuffer.mag[0], NUM_SAMPLES, &feats);
    int16_t features_int16[N_FEATURES] = {0};
    float features_array[N_FEATURES] = {0};
    features_to_array(&feats, features_array);
    apply_scaler(features_array, features_int16);
    microsDuration = micros() - microsStart;
    // Log.info("Feature beregning og inferencing duration, miikrosekunder: %d",
    // microsDuration);
    float probabs[8] = {0};
    const int ret = random_forest_model_predict_proba(
        features_int16, N_FEATURES, probabs, N_CLASSES);
    if (ret < 0) {
      Log.info("ERROR");
    }

    int out = prev_out;
    float bestProb = 0.0f;
    int bestClass = 4;

    // Find den mest sandsynlige klasse
    for (int i = 0; i < N_CLASSES; i++) {
      if (probabs[i] > bestProb) {
        bestProb = probabs[i];
        bestClass = i;
      }
    }

    // Hvis ingen klasse er tydelig nok, gå til 4
    if (bestProb < HYST_THRESHOLD_STAY) {
      out = 4;
    } else {
      // Hvis vi skifter klasse, kræv højere sandsynlighed
      if (bestClass != prev_out) {
        if (bestProb >= HYST_THRESHOLD_CHANGE) {
          out = bestClass;
        } else {
          out = prev_out;
        }
      } else {
        out = bestClass;
      }
    }

    prev_out = out;

    const char *gesture = "Other";
    switch (out) {
    case 0:
      gesture = "OneDown";
      break;
    case 1:
      gesture = "OneUp";
      break;
    case 2:
      gesture = "TwoDown";
      break;
    case 3:
      gesture = "TwoUp";
      break;
    case 4:
      gesture = "Other";
      break;
    case 5:
      gesture = "SlowDown";
      break;
    case 6:
      gesture = "SlowUp";
      break;
    case 7:
      gesture = "Vibrato";
      break;
    }
    Log.info("Output class: %s", gesture);
    // Send gesture via Bluetooth:

    uint8_t payload = (uint8_t)out;
    sendGestureBluetooth(payload);
  }
}
