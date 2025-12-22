#include "Sensirion_upt_ble_auto_detection.h"
#include "MqttMailingService.h"
#include <Arduino.h>
#include <MeasurementFormatting.hpp>


using namespace sensirion::upt::core;
using namespace sensirion::upt::ble_auto_detection;
using namespace sensirion::upt::mqtt;

// Configuration
constexpr auto ssid = "ap-name";
constexpr auto password = "ap-pass.";
constexpr auto broker_uri = "mqtt://mqtt.myserverdomain.com:1883";
// constexpr const auto ssl_cert =
//     "------BEGIN CERTIFICATE-----\nmy-certificate\n-----END CERTIFICATE-----";

MqttMailingService mqttMailingService;
SensirionBleScanner bleScanner = SensirionBleScanner();
std::map<uint16_t, std::vector<Measurement>> lastBleScanSamples;

void setup() {
    Serial.begin(115200);
    delay(2000); // Wait for potential serial connection

    // Configure the MQTT mailing service
    mqttMailingService.setBrokerURI(broker_uri);
    // [Optional] set a prefix that will be prepended to the topic defined in the messages
    mqttMailingService.setGlobalTopicPrefix("myExamplePrefix/bleRelay/");

    // [Optional] Uncomment next line for SSL connection
    // mqttMailingService.setSslCertificate(ssl_cert);

    // Set a formatting function to be able to send Measurements
    mqttMailingService.setMeasurementMessageFormatterFn(DefaultMeasurementFormatter{});

    // Set a topic suffix function to be able to send Measurements
    mqttMailingService.setMeasurementToTopicSuffixFn(DefaultMeasurmentToTopicSuffix{});

    ESP_LOGI("SETUP", "MQTT Mailing Service starting and connecting ....");

    // A blocking start ensure that connection is established before starting
    // sending messages.
    bool blockingStart = true;
    mqttMailingService.startWithDelegatedWiFi(ssid, password, blockingStart);

    ESP_LOGI("SETUP", "MQTT Mailing Service started and connected !");
    mqttMailingService.sendTextMessage("Device connected !", "myExamplePrefix/bleRelay/status");

    bleScanner.begin();
}

void loop() {
    delay(1000);

    bleScanner.getSamplesFromScanResults(lastBleScanSamples);

    // Iterate through all devices and their measurements
    for (const auto& device : lastBleScanSamples) {
        for (const auto& measurement : device.second) {
            // Send each measurement to MQTT
            bool successful = mqttMailingService.sendMeasurement(measurement);
            
            if(!successful){
                ESP_LOGE("MAIN", "Failed to send measurement");
            }
        }
        ESP_LOGD("MAIN", "Sent measurements for device ID: %u", device.first);
    }

    lastBleScanSamples.clear();
    delay(100);

    // ensure scanning is restarted in case of errors.
    bleScanner.keepAlive();
}