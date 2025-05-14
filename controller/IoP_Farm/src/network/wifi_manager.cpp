#include "network/wifi_manager.h"
#include "network/mqtt_manager.h"
#include "config/config_manager.h"

namespace farm::net
{
    using namespace farm::config::wifi;
    using namespace farm::config::sensors;
    using farm::log::Level;
    using farm::log::LoggerFactory;
    using farm::config::ConfigType;
    using farm::log::ColorFormatter;
    
    // Инициализация статического экземпляра (паттерн Singleton)
    std::shared_ptr<MyWiFiManager> MyWiFiManager::instance = nullptr;
    
    MyWiFiManager::MyWiFiManager(std::shared_ptr<farm::log::ILogger> logger)
        : mqttServerParam(MQTT_SERVER_PARAM_ID, MQTT_SERVER_PARAM_LABEL, "", MQTT_SERVER_PARAM_LENGTH),
          mqttPortParam(MQTT_PORT_PARAM_ID, MQTT_PORT_PARAM_LABEL, MQTT_DEFAULT_PORT, MQTT_PORT_PARAM_LENGTH)
    {
        // Если логгер не передан, создаем новый с помощью фабрики
        if (!logger) 
        {
            this->logger = LoggerFactory::createSerialLogger(Level::Info);
        } 
        else 
        {
            this->logger = logger;
        }
        
        wifiManager.setDebugOutput(false);
        wifiManager.setConnectTimeout(CONNECT_TIMEOUT);
        wifiManager.setConfigPortalTimeout(PORTAL_TIMEOUT);
        wifiManager.setConfigPortalBlocking(false);

        // Колбэк при активации точки доступа
        wifiManager.setAPCallback([this](WiFiManager* wm) {
            portalActive = true;
            digitalWrite(pins::LED_PIN, HIGH);
            this->logger->log(Level::Info, 
                     "[WiFi] Config Portal активирован: %s (Pass: %s)", 
                     apName.c_str(), apPassword.c_str());
        });
        
        // Добавление возможности ввести параметры MQTT в config portal
        wifiManager.addParameter(&mqttServerParam);
        wifiManager.addParameter(&mqttPortParam); 
        wifiManager.setSaveParamsCallback([this]() { this->saveParamsCallback(); });
    
    }
    
    // Получение экземпляра (паттерн Singleton)
    std::shared_ptr<MyWiFiManager> MyWiFiManager::getInstance(std::shared_ptr<farm::log::ILogger> logger)
    {
        if (instance == nullptr) 
        {
            // Используем явное создание вместо make_shared, т.к. конструктор приватный
            instance = std::shared_ptr<MyWiFiManager>(new MyWiFiManager(logger));
        }
        return instance;
    }
    
    void MyWiFiManager::saveParamsCallback()
    {
        String mqttServer = mqttServerParam.getValue();
        String mqttPort   = mqttPortParam.getValue();
        
        // Если значения не пустые, сохраняем их
        if (mqttServer.length() > 0 || mqttPort.length() > 0)
        {
            auto mqttManager = MQTTManager::getInstance(logger);
            
            // Применяем новые настройки, если они отличаются от текущих
            if (mqttServer.length() > 0 && mqttServer != mqttManager->getMqttHost())
            {
                logger->log(Level::Info, 
                          "[WiFi] Сохранение новых настроек MQTT: сервер=%s", 
                          mqttServer.c_str());
                mqttManager->setMqttHost(mqttServer);
            }
            
            if (mqttPort.length() > 0 && mqttPort.toInt() != mqttManager->getMqttPort())
            {
                logger->log(Level::Info, 
                          "[WiFi] Сохранение новых настроек MQTT: порт=%s", 
                          mqttPort.c_str());
                mqttManager->setMqttPort(mqttPort.toInt());
            }
        }
    }
    
    bool MyWiFiManager::initialize()
    {
        logger->log(Level::Farm, "[WiFi] Инициализация WIFI");
        
        setAccessPointCredentials(DEFAULT_AP_NAME, DEFAULT_AP_PASSWORD);
        setHostName(DEFAULT_HOSTNAME);

        // Включение встроенной отладки WiFiManager
        // Не пишем RESET для выключения цветного вывода, потому что при выводе НАШИХ сообщений он обновляется явным образом
#if defined(IOP_DEBUG) && defined(COLOR_SERIAL_LOG)
        setDebugOutput(true, 
        String(ColorFormatter::getLevelColor(Level::Debug)) + 
        String("[DEBUG] [WM]   ")); 
#elif defined(IOP_DEBUG)
        setDebugOutput(true, String("[DEBUG] [WM]   "));
#endif
        
        if (hostName.length() > 0) 
        {
            WiFi.setHostname(hostName.c_str());
        }
        
        // Получаем текущие настройки MQTT для отображения в портале
        auto mqttManager   = MQTTManager::getInstance(logger);
        if (mqttManager->isMqttConfigured())
        {
            auto configManager   = farm::config::ConfigManager::getInstance(logger);
            String currentServer = configManager->getValue<String>(ConfigType::Mqtt, "host");
            int16_t currentPort  = configManager->getValue<int16_t>(ConfigType::Mqtt, "port");
            
            // Устанавливаем текущие значения для их отображения в config portal
            mqttServerParam.setValue(currentServer.c_str(), MQTT_SERVER_PARAM_LENGTH);
            mqttPortParam.setValue(String(currentPort).c_str(), MQTT_PORT_PARAM_LENGTH);
        }
        
        
        // Попытка автоматического подключения
        logger->log(Level::Info, "[WiFi] Запуск автоподключения к сети, таймаут: %d с", CONNECT_TIMEOUT);
        bool connected = wifiManager.autoConnect(
            apName.c_str(), 
            apPassword.length() > 0 ? apPassword.c_str() : nullptr
        );
        
        if (connected) 
        {
            // Если подключение успешно, значит настройки были сохранены или введены
            // в процессе работы портала конфигурации
            digitalWrite(pins::LED_PIN, LOW);
            logger->log(Level::Farm, 
                      "[WiFi] Подключено к %s (IP: %s, RSSI: %d дБм)", 
                      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
            
            // Если подключение произошло к новой сети через портал, дополнительно логируем
            if (portalActive) 
            {
                logger->log(Level::Info,
                         "[WiFi] Сохранены новые учетные данные сети");
            }
        }
        else
        {
            logger->log(Level::Warning, 
                      "[WiFi] Не удалось установить соединение при инициализации");
        }
        
        return connected;
    }
    
    // Поддержание WiFi соединения - вызывать в цикле loop()
    void MyWiFiManager::maintainConnection()
    {
        wifiManager.process();
        
        if (portalActive) 
        {
            if (WiFi.status() == WL_CONNECTED) 
            {
                logger->log(Level::Farm, 
                          "[WiFi] Соединение установлено через Config Portal");
                logger->log(Level::Farm, 
                          "[WiFi] Сеть: %s, IP: %s", 
                          WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
                
                // Закрываем портал только если он действительно активен
                if (isConfigPortalActive())
                {
                    digitalWrite(pins::LED_PIN, LOW);
                    logger->log(Level::Debug, "[WiFi] Остановка Config Portal");
                    wifiManager.stopConfigPortal();
                }
                
                digitalWrite(pins::LED_PIN, LOW);
                portalActive      = false;
                reconnectAttempts = 0;
            }
            
            return; // Пока портал активен, не выполняем другие проверки
        }
        
        // Периодическая проверка WiFi соединения
        unsigned long currentMillis = millis();
        if (currentMillis - lastCheckTime >= CHECK_INTERVAL) 
        {
            lastCheckTime = currentMillis;
            
            if (WiFi.status() != WL_CONNECTED) 
            {
                // Первая потеря соединения
                if (reconnectAttempts == 0) {
                    logger->log(Level::Warning, "[WiFi] Соединение потеряно");
                }
                
                if (currentMillis - lastReconnectTime >= RECONNECT_RETRY_INTERVAL) {
                    lastReconnectTime = currentMillis;
                    reconnectAttempts++;
                    
                    logger->log(Level::Info, 
                              "[WiFi] Попытка переподключения %d из %d", 
                              reconnectAttempts, MAX_RECONNECT_ATTEMPTS);
                    
                    // После достижения максимального числа попыток переподключения запускаем портал
                    if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) 
                    {
                        logger->log(Level::Warning, 
                                  "[WiFi] Не удалось переподключиться после %d попыток, запуск Config Portal", 
                                  reconnectAttempts);
                        
                        portalActive = true;
                        digitalWrite(pins::LED_PIN, HIGH);
                        wifiManager.startConfigPortal(
                            apName.c_str(), 
                            apPassword.length() > 0 ? apPassword.c_str() : nullptr
                        );
                        
                        reconnectAttempts = 0;
                    }
                    else 
                    {
                        WiFi.reconnect();
                    }
                }
            }
            else if (reconnectAttempts > 0) {
                // Если соединение восстановлено после попыток переподключения
                logger->log(Level::Farm, 
                          "[WiFi] Переподключено к %s (IP: %s, RSSI: %d дБм)", 
                          WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
                reconnectAttempts = 0;
            }
        }
    }
    
    // Включение/выключение встроенной отладки WiFiManager
    void MyWiFiManager::setDebugOutput(bool enable)
    {
        wifiManager.setDebugOutput(enable);
        logger->log(Level::Debug, 
                  "[WiFi] Отладочный вывод %s", 
                  enable ? "включен" : "отключен");
    }

    // Включение/выключение встроенной отладки WiFiManager с префиксом
    void MyWiFiManager::setDebugOutput(bool enable, const String& prefix)
    {
        wifiManager.setDebugOutput(enable, prefix);
        logger->log(Level::Debug, 
                  "[WiFi] Отладочный вывод %s с префиксом: %s",
                  enable ? "включен" : "отключен", prefix.c_str());
    }

    String MyWiFiManager::getIPAddress() const
    {
        if (isConnected())
        {
            return WiFi.localIP().toString();
        }
        logger->log(Level::Warning, "[WiFi] Попытка получения IP-адреса при отсутствии соединения");
        return "";
    }
    
    bool MyWiFiManager::isConnected() const
    {
        return WiFi.status() == WL_CONNECTED;
    }
        
    bool MyWiFiManager::isConfigPortalActive()
    {
        return wifiManager.getConfigPortalActive();
    }
        
    void MyWiFiManager::setAccessPointCredentials(const String& name, const String& password)
    {
        apName     = name;
        apPassword = password;
        logger->log(Level::Info, 
                  "[WiFi] Установлены учетные данные точки доступа: %s (пароль: %s)", 
                     apName.c_str(), apPassword.c_str());
    }
    
    void MyWiFiManager::setHostName(const String& name)
    {
        hostName = name;
        if (name.length() > 0)
        {
            WiFi.setHostname(name.c_str());
            logger->log(Level::Debug, 
                      "[WiFi] Установлено имя хоста: %s", name.c_str());
        }
    }
        
    bool MyWiFiManager::resetSettings()
    {   
        logger->log(Level::Warning, "[WiFi] Удаление сохраненных учетных данных");
        
        wifiManager.resetSettings();
        
        return true; 
    }
        
    // Проверка наличия сохраненных настроек WiFi
    // TODO: исправить ошибку. После сброса настроек все равно видит сохраненные настройки. 
    // Не рекомендуется использовать
    bool MyWiFiManager::checkWifiSaved()
    {
        bool isSaved = wifiManager.getWiFiIsSaved();
        
        if (isSaved) 
        {
            logger->log(Level::Info, 
                      "[WiFi] Найдены сохраненные учетные данные");
            
            // На этом этапе WiFi.SSID() еще недоступен, WiFiManager покажет SSID позже
            // во время подключения (см. лог "*wm:Connecting to SAVED AP: xxx")
        }
        else
        {
            logger->log(Level::Info, 
                      "[WiFi] Сохраненные сети не найдены");
        }
        
        return isSaved;
    }
}
