#pragma once

#include <AsyncMqttClient.h>
#include "utils/logger_factory.h"
#include "config/config_manager.h"
#include "config/constants.h"
#include <memory>

namespace farm::logic 
{ 
    class ActuatorsManager; 
}

namespace farm::net
{
    using namespace farm::config;
    using namespace farm::log;

    // Менеджер MQTT соединения - синглтон
    class MQTTManager
    {
    private:
        // Приватный конструктор (паттерн Синглтон)
        explicit MQTTManager(std::shared_ptr<ILogger> logger = nullptr);
        static std::shared_ptr<MQTTManager> instance;
        
        std::shared_ptr<ILogger> logger;
        
        std::shared_ptr<ConfigManager> configManager;
        
        AsyncMqttClient mqttClient;
        
        // Информация об MQTT брокере
        String serverDomain;
        uint16_t serverPort = 0;
        
        // Время последней проверки соединения с MQTT брокером
        unsigned long lastCheckTime = 0;
        
        bool isConnecting = false;
        bool isConnected = false;
        bool isInitialized = false;
        
        uint8_t reconnectAttempts = 0;
        
        unsigned long lastReconnectTime = 0;
        
        // Колбэки для MQTT
        void onMqttConnect(bool sessionPresent);
        void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
        void onMqttSubscribe(uint16_t packetId, uint8_t qos);
        void onMqttUnsubscribe(uint16_t packetId);
        void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, 
                          size_t len, size_t index, size_t total);
        void onMqttPublish(uint16_t packetId);
        
        bool setupMQTTClient();
        
        // Обработка полученной команды из топика /command
        void handleCommand(const CommandCode& command);

        // Имя устройства, буффер для setClientId (необходим для решения проблемы с const char* и c_str()!)
        char deviceIdBuffer[mqtt::DEVICE_ID_MAX_LENGTH];
    public:
        // Получение экземпляра синглтона
        static std::shared_ptr<MQTTManager> getInstance(std::shared_ptr<ILogger> logger = nullptr);
        MQTTManager(const MQTTManager&) = delete;
        MQTTManager& operator=(const MQTTManager&) = delete;
        
        ~MQTTManager();

        // Инициализация MQTT и попытка первичного подключения
        bool initialize();
        
        bool setMqttHost(const String& host);
        bool setMqttPort(uint16_t port);
        bool setMqttDeviceId(const String& deviceId);

        const String& getMqttHost() const;
        int16_t getMqttPort() const;
        String getDeviceId() const;
        
        // Метод для одновременной установки всех параметров MQTT
        bool applyMqttSettings(const String& host, uint16_t port, const String& deviceId = "");
        
        // Поддержание MQTT соединения - вызывать в цикле loop()
        void maintainConnection();
        
        // Публикация данных в топик /data
        bool publishData(uint8_t qos = mqtt::QOS_1, bool retain = true);
        
        // Публикация в произвольный топик
        bool publishToTopic(const String& topic, const String& payload, 
                            uint8_t qos = mqtt::QOS_1, bool retain = true);

        // Публикация в топик логгера (/logs)
        bool publishToTopicLoggerVersion(const String& topic, const String& payload, 
                            uint8_t qos = mqtt::QOS_1, bool retain = true);

        // Подписка на конкретный топик
        uint16_t subscribeToTopic(const String& topic, uint8_t qos = mqtt::QOS_1);
    
        // Подписка на все стандартные топики
        bool subscribeToAllTopics(uint8_t qos = mqtt::QOS_1);
        
        // Отписка от конкретного топика
        uint16_t unsubscribeFromTopic(const String& topic);
        // Отписка от всех подписанных топиков
        bool unsubscribeFromAllTopics();

        bool isMqttInitialized() const;
        
        bool isClientConnected() const;
        
        bool isMqttConfigured() const;
        
        String getMqttTopic(ConfigType type) const;
    };
}
