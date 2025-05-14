#pragma once

#include <ArduinoJson.h>  // Используется ArduinoJson 7.x
#include <FS.h>
#include <SPIFFS.h>
#include <memory>
#include <optional>
#include "utils/logger_factory.h"
#include "constants.h"

// TODO: сейчас команда для начальной загрузки файлов конфигурации в память: pio run --target uploadfs

/*
    Для загрузки файлов в SPIFFS: pio run --target uploadfs
    Для скачивания содержимого SPIFFS можно использовать: pio run --target downloadfs
*/

namespace farm::config
{
    // Менеджер конфигурации - синглтон
    class ConfigManager
    {
    private:
        // Приватный конструктор (паттерн Синглтон)
        explicit ConfigManager(std::shared_ptr<farm::log::ILogger> logger = nullptr);
        static std::shared_ptr<ConfigManager> instance;
        
        std::shared_ptr<farm::log::ILogger> logger;
        
        // JSON документы для разных типов конфигураций
        JsonDocument dataConfig;     // Данные от датчиков
        JsonDocument systemConfig;   // Системная конфигурация
        JsonDocument commandConfig;  // Команды управления
        JsonDocument mqttConfig;     // Конфигурация MQTT
        JsonDocument passwordsConfig; // Пароли и учетные данные
        
        // Методы для работы с конфигурацией
        bool loadJsonFromFile(const char* path, JsonDocument& doc);       // Из памяти в JSON документ
        bool saveJsonToFile(const char* path, const JsonDocument& doc);   // Из JSON документа в память
        
        const char* getConfigPath(ConfigType type) const;
        const char* getDefaultConfigPath(ConfigType type) const;

        JsonDocument& getConfigDocument(ConfigType type);
        const JsonDocument& getConfigDocument(ConfigType type) const;

        // Очистка всех JSON объектов без сохранения в энергонезависимую память
        bool clearAllConfigsWithoutSaving();

    public:
        static std::shared_ptr<ConfigManager> getInstance(std::shared_ptr<farm::log::ILogger> logger = nullptr);
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;
        
        ~ConfigManager();
        
        bool initialize();
        
        bool loadConfig(ConfigType type);  // Из памяти в JSON документ
        bool saveConfig(ConfigType type);  // Из JSON документа в память
        
        bool loadAllConfigs();
        bool saveAllConfigs();

        bool clearConfig(ConfigType type);
        bool clearAllConfigs();

        bool deleteConfig(ConfigType type);
        bool deleteAllConfigs();

        bool loadDefaultConfig(ConfigType type);
        bool loadAllDefaultConfigs();
        
        bool hasKey(ConfigType type, const char* key) const;
        
        // Вывод текущей конфигурации в Serial (!!!) для отладки
        void printConfig(ConfigType type) const;
        
        // Получение JSON строки по типу
        String getConfigJson(ConfigType type) const;
        
        // Обновление конфигурации из JSON строки без сохранения в энергонезависимую память
        bool updateFromJson(ConfigType type, const String& jsonString);

        // Вывод информации о файловой системе SPIFFS через Serial (!!!)
        void printSpiffsInfo() const;

        // Шаблонный метод для получения значения из JSON, необходимо определять в заголовочном файле
        template<typename T>
        T getValue(ConfigType type, const char* key) const
        {
            const auto& doc = getConfigDocument(type);
            
            if (doc[key].isNull()) {
                logger->log(farm::log::Level::Error, 
                          "[Config] Ключ '%s' не найден", key);
            }
            
            return doc[key].as<T>();
        }
        
        // Шаблонный метод для установки значения в JSON, необходимо определять в заголовочном файле
        template<typename T>
        void setValue(ConfigType type, const char* key, const T& value)
        {
            auto& doc = getConfigDocument(type);
            doc[key] = value;
        }
    };
}
