#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <syslog.h>
#include <mqtt/async_client.h>

const std::string MQTT_BROKER  = "tcp://localhost:1883";
const std::string MQTT_TOPIC   = "/farm001/log";
const std::string CLIENT_ID    = "farm_logger";
const int QOS = 1;

class LoggerCallback : public virtual mqtt::callback {
public:
    void message_arrived(mqtt::const_message_ptr msg) override {
        try {
            std::string payload = msg->get_payload();
            // Записываем сырое сообщение в syslog
            syslog(LOG_INFO, "%s", payload.c_str());
        }
        catch (const std::exception& e) {
            syslog(LOG_ERR, "Processing error: %s", e.what());
        }
    }

    void connection_lost(const std::string& cause) override {
        syslog(LOG_WARNING, "Connection lost: %s", cause.c_str());
    }

    void connected(const std::string& cause) override {
        syslog(LOG_NOTICE, "Connected to MQTT broker: %s", cause.c_str());
    }
};

int main() {
    openlog("farm_logger", LOG_PID|LOG_CONS, LOG_DAEMON);
    syslog(LOG_NOTICE, "Starting Farm Logger service");

    try {
        mqtt::async_client client(MQTT_BROKER, CLIENT_ID);
        LoggerCallback cb;
        client.set_callback(cb);

        auto connOpts = mqtt::connect_options_builder()
            .clean_session(true)
            .automatic_reconnect(true)
            .finalize();

        client.connect(connOpts)->wait();
        client.subscribe(MQTT_TOPIC, QOS)->wait();
        syslog(LOG_INFO, "Subscribed to topic: %s", MQTT_TOPIC.c_str());

        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(3600));
        }
    }
    catch (const mqtt::exception& exc) {
        syslog(LOG_ERR, "MQTT error: %s", exc.what());
    }
    catch (const std::exception& e) {
        syslog(LOG_ERR, "General error: %s", e.what());
    }

    closelog();
    return EXIT_FAILURE;
}
