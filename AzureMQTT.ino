#include "var.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <AzureIoTHubMQTTClient.h>
#include "var.h"

WiFiClientSecure tlsClient;
AzureIoTHubMQTTClient client(tlsClient, IOTHUB_HOSTNAME, DEVICE_ID, DEVICE_KEY);
WiFiEventHandler  e1, e2;
//TODO:test2
//todo:test1
const int LED_PIN = 2; //Pin to turn on/of LED a command from IoT Hub
unsigned long lastMillis = 0;

void connectToIoTHub(); // 
void onMessageCallback(const MQTT::Publish& msg);

void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
#ifdef INFO
	Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());
#endif // INFO
	connectToIoTHub();
}

void onSTADisconnected(WiFiEventStationModeDisconnected event_info) {
#ifdef INFO
	Serial.printf("Disconnected from SSID: %s\n", event_info.ssid.c_str());
	Serial.printf("Reason: %d\n", event_info.reason);
#endif
}

void onClientEvent(const AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEvent event) {
	if (event == AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEventConnected) {
#ifdef INFO
		Serial.println("Connected to Azure IoT Hub");
#endif
		client.onMessage(onMessageCallback);
	}
}

void onActivateRelayCommand(String cmdName, JsonVariant jsonValue) {
	JsonObject& jsonObject = jsonValue.as<JsonObject>();
	if (jsonObject.containsKey("Parameters")) {
		auto params = jsonValue["Parameters"];
		auto isAct = (params["Activated"]);
		if (isAct) {
			Serial.println("Activated true");
			digitalWrite(LED_PIN, HIGH);
		}
		else {
			Serial.println("Activated false");
			digitalWrite(LED_PIN, LOW);
		}
	}
}

void setup() {
	Serial.begin(115200);
	while (!Serial) {
		yield();
	}
	delay(2000);

	Serial.setDebugOutput(true);

	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	const char *AP_SSID = WIFI_SSID;
	const char *AP_PASS = WIFI_PASS;
	WiFi.begin(AP_SSID, AP_PASS);

	//Handle WiFi events
	e1 = WiFi.onStationModeGotIP(onSTAGotIP); // As soon WiFi is connected, start the Client
	e2 = WiFi.onStationModeDisconnected(onSTADisconnected);

	//Handle client events
	client.onEvent(onClientEvent);
	client.onCloudCommand("ActivateRelay", onActivateRelayCommand);
}

void onMessageCallback(const MQTT::Publish& msg) {
	if (msg.payload_len() == 0) {
		return;
	}
#ifdef INFO
	Serial.println(msg.payload_string());
#endif
}

void connectToIoTHub() {
#ifdef INFO
	if (client.begin())
		Serial.println("MQTT connected");
	else
		Serial.println("Could not connect to MQTT");
#endif

}

void readSensor(float *temp, float *press) {
	//If you don't have the sensor
	*temp = 20 + (rand() % 10 + 2);
	*press = 90 + (rand() % 8 + 2);
}

void loop() {

	//MUST CALL THIS in loop()
	client.run();

	if (client.connected()) {
		if (millis() - lastMillis > 3000) {
			lastMillis = millis();

			float temp, press;
			readSensor(&temp, &press);
			time_t currentTime = now();
			// You can do this to publish payload to IoT Hub
			
			//String payload = "{\"DeviceId\":\"" + String(DEVICE_ID) + "\", \"MTemperature\":" + String(temp) + ", \"EventTime\":" + String(currentTime) + "}";
			//Serial.println(payload);

			//client.publish(MQTT::Publish("devices/" + String(DEVICE_ID) + "/messages/events/", payload).set_qos(1));
			//client.sendEvent(payload);
			
			// Publish informations
			AzureIoTHubMQTTClient::KeyValueMap keyVal = { { "MTemperature", temp },{ "MPressure", press },{ "DeviceId", DEVICE_ID },{ "EventTime", currentTime } };
			client.sendEventWithKeyVal(keyVal);
		}
	}
	else {}
	delay(1000); // <- fixes some issues with WiFi stability
}