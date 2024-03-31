/**
 * @brief This example demonstrates how to retrieve the unix time from a NTP
 * server.
 */

#include <Arduino.h>
#include <http_client.h>
#include <led_ctrl.h>
#include <log.h>
#include <Arduino_KNN.h>
#include <lte.h>


#define NOTIFICATION_URL ""

int pin = 19;
unsigned long durationHigh;
unsigned long durationLow;
float threshold = 0.67;

// Create a new KNNClassifier, input values are array of floats
KNNClassifier anomKNN(1);

bool charging = false;
bool panic = false;

void setup() {

    LedCtrl.begin();
    LedCtrl.startupCycle();

    Log.begin(115200);
    pinConfigure(PIN_PA4, PIN_DIR_INPUT);

    // Start LTE modem and connect to the operator
    if (!Lte.begin()) {
        Log.error(F("Failed to connect to operator"));
        return;
    }

    Log.infof(F("Connected to operator: %s\r\n"), Lte.getOperator().c_str());

    Log.raw("Building KNN anomaly detector for Volt Valet!");
    Log.raw("\n");

    Log.raw("Adding training data... ");
    Log.raw("\n");

    // add examples to KNN
    float example1[] = { 1.0, 815.0 };
    float example2[] = { 1.0, 818.0 };
    float example3[] = { 1.0, 820.0 };
    float example4[] = { 1.0, 808.0 };
    float example5[] = { 1.0, 802.0 };
    float example6[] = { 1.0, 804.0 };
    float example7[] = { 1.0, 803.0 };
    float example8[] = { 1.0, 195.0 };
    float example9[] = { 0.0, 194.0 };
    float example10[] = { 0.0, 196.0 };
    float example11[] = { 0.0, 198.0 };
    float example12[] = { 0.0, 197.0 };
    float example13[] = { 0.0, 188.0 };
    float example14[] = { 0.0, 187.0 };
    float example15[] = { 0.0, 191.0 };
    float example16[] = { 0.0, 190.0 };
    float example17[] = { 1.0, 772.0 };
    float example18[] = { 1.0, 769.0 };
    float example19[] = { 1.0, 770.0 };

    anomKNN.addExample(example1, 800); // add example for class 1, or high
    anomKNN.addExample(example2, 800); // add example for class 1, or high
    anomKNN.addExample(example3, 800); // add example for class 1, or high
    anomKNN.addExample(example4, 800); // add example for class 1, or high
    anomKNN.addExample(example5, 800); // add example for class 1, or high
    anomKNN.addExample(example6, 800); // add example for class 1, or high
    anomKNN.addExample(example7, 800); // add example for class 1, or high
    anomKNN.addExample(example17, 800); // add example for class 1, or high
    anomKNN.addExample(example18, 800); // add example for class 1, or high
    anomKNN.addExample(example19, 800); // add example for class 1, or high
    anomKNN.addExample(example8, 190); // add example for class 0, or low
    anomKNN.addExample(example9, 190); // add example for class 0, or low
    anomKNN.addExample(example10, 190); // add example for class 0, or low
    anomKNN.addExample(example11, 190); // add example for class 0, or low
    anomKNN.addExample(example12, 190); // add example for class 0, or low
    anomKNN.addExample(example13, 190); // add example for class 0, or low
    anomKNN.addExample(example14, 190); // add example for class 0, or low
    anomKNN.addExample(example15, 190); // add example for class 0, or low
    anomKNN.addExample(example16, 190); // add example for class 0, or low

    // Get and print out the KNN count
    Log.raw("\tanomKNN.getCount() = ");
    Log.raw(String(anomKNN.getCount()));
    Log.raw("\n");

    if (!HttpClient.configure(NOTIFICATION_URL, 443, true)) {
        Log.errorf(F("Failed to configure HTTPS for the domain %s\r\n"),
                   NOTIFICATION_URL);
        return;
    }
    HttpResponse response;
    response = HttpClient.post("/prod/charger", "{\"message\": \"Started monitoring!\"}", "Content-Type: application/json", HttpClient.CONTENT_TYPE_APPLICATION_JSON);
    Log.infof(F("POST - HTTP status code: %u, data size: %u\r\n"),
              response.status_code,
              response.data_size);
    String body = HttpClient.readBody(response.data_size + 16);

    if (body != "") {
        Log.infof(F("Body: %s\r\n"), body.c_str());
    }

    Log.raw("Begin monitoring!\n");
}

void loop() {
  bool lowanomaly = false;
  bool highanomaly = false;
  durationHigh = pulseIn(PIN_PA4, HIGH);
  durationLow = pulseIn(PIN_PA4, LOW);
  
  if (durationHigh > 0) {
    charging = false;
    panic = false;
    float highinput[] = {1.0, durationHigh};
    int classification = anomKNN.classify(highinput, 2); // classify input with K=2
    float confidence = anomKNN.confidence();
    if (classification != 800) {
      highanomaly = true;
    }
    if (confidence < threshold) {
      highanomaly = true;
    }
  }
  if (durationLow > 0) {
    charging = false;
    panic = false;
    float lowinput[] = {0.0, durationLow};
    int classification = anomKNN.classify(lowinput, 2); // classify input with K=2
    float confidence = anomKNN.confidence();
    if (classification != 190) {
      lowanomaly = true;
    }
    if (confidence < threshold) {
      lowanomaly = true;
    }
  }
  if (lowanomaly && highanomaly && !panic) {
    panic = true;
    HttpResponse response;
    response = HttpClient.post("/prod/charger", "{\"message\": \"Charger has faulted!\"}", "Content-Type: application/json", HttpClient.CONTENT_TYPE_APPLICATION_JSON);
    Log.infof(F("POST - HTTP status code: %u, data size: %u\r\n"),
              response.status_code,
              response.data_size);
    delay(1UL * 60 * 60 * 1000); // Wait for an hour
  }
  if (durationHigh == 0 && durationLow == 0 && !charging) {
    charging = true;
    HttpResponse response;
    response = HttpClient.post("/prod/charger", "{\"message\": \"Charger has begun charging\"}", "Content-Type: application/json", HttpClient.CONTENT_TYPE_APPLICATION_JSON);
    Log.infof(F("POST - HTTP status code: %u, data size: %u\r\n"),
              response.status_code,
              response.data_size);
    delay(8UL * 60 * 60 * 1000); // Wait up to 8 hours
  }
  if (durationHigh == 0 && durationLow == 0 && charging) {
    charging = false;
    // Fault condition!
    HttpResponse response;
    response = HttpClient.post("/prod/charger", "{\"message\": \"Charger may be off or cannot communicate with our cloud.\"}", "Content-Type: application/json", HttpClient.CONTENT_TYPE_APPLICATION_JSON);
    Log.infof(F("POST - HTTP status code: %u, data size: %u\r\n"),
              response.status_code,
              response.data_size);
  }
}
