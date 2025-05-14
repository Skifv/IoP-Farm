#include "config/config_manager.h"

/*
    Для загрузки файлов в SPIFFS: pio run --target uploadfs
*/

namespace farm::config
{
    using farm::log::Level;
    using farm::log::LoggerFactory;

    // Инициализация статического экземпляра (паттерн Singleton)
    std::shared_ptr<ConfigManager> ConfigManager::instance = nullptr;

    ConfigManager::ConfigManager(std::shared_ptr<farm::log::ILogger> logger)
        : dataConfig(),     // В ArduinoJson 7 не нужно указывать размер JSON-документа
          systemConfig(),   // В ArduinoJson 7 не нужно указывать размер JSON-документа
          commandConfig(),  // В ArduinoJson 7 не нужно указывать размер JSON-документа
          mqttConfig(),     // В ArduinoJson 7 не нужно указывать размер JSON-документа
          passwordsConfig() // В ArduinoJson 7 не нужно указывать размер JSON-документа
    {
        if (!logger) 
        {
            this->logger = LoggerFactory::createSerialLogger(Level::Info);
        } 
        else 
        {
            this->logger = logger;
        }
    }

    // Получение экземпляра (паттерн Singleton)
    std::shared_ptr<ConfigManager> ConfigManager::getInstance(std::shared_ptr<farm::log::ILogger> logger)
    {
        if (instance == nullptr) 
        {
            // Используем явное создание вместо make_shared, т.к. конструктор приватный
            instance = std::shared_ptr<ConfigManager>(new ConfigManager(logger));
        }
        return instance;
    }

    ConfigManager::~ConfigManager()
    {
        logger->log(Level::Debug, "[Config] Освобождение памяти ConfigManager");

        dataConfig.clear();
        systemConfig.clear();
        commandConfig.clear();
        mqttConfig.clear();
        passwordsConfig.clear();
    }

    bool ConfigManager::initialize()
    {
        logger->log(Level::Farm, "[Config] Инициализация SPIFFS");
        
        if (!SPIFFS.begin(true)) {
            logger->log(Level::Error, "[Config] Ошибка инициализации SPIFFS");
            return false;
        }
        
        bool allFilesExist = true;
        
        // Проверка дефолтных файлов конфигурации
        if (!SPIFFS.exists(paths::DEFAULT_DATA_CONFIG)) {
            logger->log(Level::Error, 
                       "[Config] Отсутствует файл: %s", 
                       paths::DEFAULT_DATA_CONFIG);
            allFilesExist = false;
        }
        
        if (!SPIFFS.exists(paths::DEFAULT_SYSTEM_CONFIG)) {
            logger->log(Level::Error, 
                       "[Config] Отсутствует файл: %s", 
                       paths::DEFAULT_SYSTEM_CONFIG);
            allFilesExist = false;
        }
        
        if (!SPIFFS.exists(paths::DEFAULT_COMMAND_CONFIG)) {
            logger->log(Level::Error, 
                       "[Config] Отсутствует файл: %s", 
                       paths::DEFAULT_COMMAND_CONFIG);
            allFilesExist = false;
        }
        
        if (!SPIFFS.exists(paths::DEFAULT_MQTT_CONFIG)) {
            logger->log(Level::Error, 
                       "[Config] Отсутствует файл: %s", 
                       paths::DEFAULT_MQTT_CONFIG);
            allFilesExist = false;
        }

        if (!SPIFFS.exists(paths::DEFAULT_PASSWORDS_CONFIG)) {
        logger->log(Level::Error, 
                    "[Config] Отсутствует файл: %s", 
                    paths::DEFAULT_PASSWORDS_CONFIG);
        allFilesExist = false;
        }

        if (!allFilesExist) {
            logger->log(Level::Error, 
                       "[Config] Отсутствуют необходимые файлы. "
                       "Убедитесь, что файлы загружены в SPIFFS с помощью команды: "
                       "pio run --target uploadfs");
            return false;
        }

        // Вызывается при самой первой прошивке ESP32, далее по желанию 
        // (необходимо выбрать соответствующую environment)
        // Загружаются настройки по умолчанию
#ifdef LOAD_DEFAULT_CONFIGS
        logger->log(Level::Info, "[Config] Загрузка стандартных настроек");

        clearAllConfigs();
        loadAllDefaultConfigs();
#else
        // Загружаются последние сохранённые настройки
        logger->log(Level::Info, "[Config] Загрузка последних конфигураций из SPIFFS");

        clearAllConfigsWithoutSaving();
        loadAllConfigs();
#endif

        return true;
    }

    bool ConfigManager::clearConfig(ConfigType type)
    {
        auto& doc = getConfigDocument(type);
        doc.clear();
        logger->log(Level::Debug, "[Config] %s очищен", getConfigPath(type));
        return saveConfig(type);
    }

    bool ConfigManager::clearAllConfigsWithoutSaving()
    {
        bool success = true;

        dataConfig.clear(); 
        systemConfig.clear();
        commandConfig.clear();
        mqttConfig.clear();
        passwordsConfig.clear();

        return success;
    }

    bool ConfigManager::clearAllConfigs()
    {
        bool success = true;

        success &= clearConfig(ConfigType::Data);
        success &= clearConfig(ConfigType::System);
        success &= clearConfig(ConfigType::Command);
        success &= clearConfig(ConfigType::Mqtt);
        success &= clearConfig(ConfigType::Passwords);

        return success;
    }

    bool ConfigManager::deleteConfig(ConfigType type)
    {
        const char* path = getConfigPath(type);
        logger->log(Level::Warning, "[Config] Удаление %s", path);
        return SPIFFS.remove(path);
    }

    bool ConfigManager::deleteAllConfigs()
    {
        bool success = true;

        success &= deleteConfig(ConfigType::Data);
        success &= deleteConfig(ConfigType::System);
        success &= deleteConfig(ConfigType::Command);
        success &= deleteConfig(ConfigType::Mqtt);
        success &= deleteConfig(ConfigType::Passwords);

        return success;
    }
    
    // Загрузка JSON из файла
    bool ConfigManager::loadJsonFromFile(const char* path, JsonDocument& doc)
    {
        if (!SPIFFS.exists(path)) {
            logger->log(Level::Warning, 
                      "[Config] Файл '%s' не существует", path);
            return false;
        }
        
        File file = SPIFFS.open(path, "r");
        if (!file) {
            logger->log(Level::Error, 
                      "[Config] Не удалось открыть файл '%s'", path);
            return false;
        }
        
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (error) {
            logger->log(Level::Error, 
                      "[Config] Ошибка десериализации JSON: %s", error.c_str());
            return false;
        }

        return true;
    }

    // Сохранение JSON в файл
    bool ConfigManager::saveJsonToFile(const char* path, const JsonDocument& doc)
    {
        File file = SPIFFS.open(path, "w");
        if (!file) {
            logger->log(Level::Error, 
                      "[Config] Не удалось открыть файл '%s' для записи", path);
            return false;
        }
        
        if (serializeJson(doc, file) == 0) {
            logger->log(Level::Error, 
                      "[Config] Ошибка сериализации JSON в файл '%s'", path);
            file.close();
            return false;
        }
        
        file.close();
        logger->log(Level::Debug, 
                  "[Config] '%s' сохранен в SPIFFS", path);
        return true;
    }

    const char* ConfigManager::getConfigPath(ConfigType type) const
    {
        switch (type)
        {
            case ConfigType::Data:
                return paths::DATA_CONFIG;
            
            case ConfigType::System:
                return paths::SYSTEM_CONFIG;
            
            case ConfigType::Command:
                return paths::COMMAND_CONFIG;
            
            case ConfigType::Mqtt:
                return paths::MQTT_CONFIG;

            case ConfigType::Passwords:
                return paths::PASSWORDS_CONFIG;
            
            default:
                logger->log(farm::log::Level::Error, 
                          "[Config] Запрошен путь для неизвестного типа конфигурации");
                return nullptr;
        }
    }
    
    const char* ConfigManager::getDefaultConfigPath(ConfigType type) const
    {
        switch (type)
        {
            case ConfigType::Data:
                return paths::DEFAULT_DATA_CONFIG;
            
            case ConfigType::System:
                return paths::DEFAULT_SYSTEM_CONFIG;
            
            case ConfigType::Command:
                return paths::DEFAULT_COMMAND_CONFIG;
            
            case ConfigType::Mqtt:
                return paths::DEFAULT_MQTT_CONFIG;

            case ConfigType::Passwords:
                return paths::DEFAULT_PASSWORDS_CONFIG;
            
            default:
                logger->log(farm::log::Level::Error, 
                          "[Config] Запрошен путь для неизвестного типа дефолтной конфигурации");
                return nullptr;
        }
    }

    // Получение изменяемого JSON документа для указанного типа
    // TODO: возвращаемое значение по дефолту нехорошее, но не критично, т.к. всегда передаем валидный тип
    JsonDocument& ConfigManager::getConfigDocument(ConfigType type)
    {
        switch (type)
        {
            case ConfigType::Data:
                return dataConfig;
            
            case ConfigType::System:
                return systemConfig;
            
            case ConfigType::Command:
                return commandConfig;
            
            case ConfigType::Mqtt:
                return mqttConfig;

            case ConfigType::Passwords:
                return passwordsConfig;
            
            default:
                logger->log(farm::log::Level::Error, 
                          "[Config] Запрошен документ для неизвестного типа конфигурации");
                return dataConfig; 
        }
    }

    // Получение КОНСТАНТНОЙ ссылки на JSON документ для указанного типа
    // TODO: возвращаемое значение по дефолту нехорошее, но не критично, т.к. всегда передаем валидный тип
    const JsonDocument& ConfigManager::getConfigDocument(ConfigType type) const
    {
        switch (type)
        {
            case ConfigType::Data:
                return dataConfig;
            
            case ConfigType::System:
                return systemConfig;
            
            case ConfigType::Command:
                return commandConfig;
            
            case ConfigType::Mqtt:
                return mqttConfig;

            case ConfigType::Passwords:
                return passwordsConfig;
            
            default:
                logger->log(farm::log::Level::Error, 
                          "[Config] Запрошен документ для неизвестного типа конфигурации");
                return dataConfig; 
        }
    }
    
    bool ConfigManager::loadDefaultConfig(ConfigType type)
    {
        const char* defaultPath = getDefaultConfigPath(type);
        auto& doc               = getConfigDocument(type);
        
        logger->log(Level::Info, 
                  "[Config] Загрузка стандартных настроек из '%s'", defaultPath);
        
        if (!SPIFFS.exists(defaultPath)) {
            logger->log(Level::Error, 
                     "[Config] '%s' отсутствует", defaultPath);
            return false;
        }
        
        if (loadJsonFromFile(defaultPath, doc)) {
            // После загрузки дефолтной конфигурации, сохраняем её в основной файл
            return saveJsonToFile(getConfigPath(type), doc);
        }
        
        return false;
    }

    bool ConfigManager::loadAllDefaultConfigs()
    {
        bool success = true;

        success &= loadDefaultConfig(ConfigType::Data);
        success &= loadDefaultConfig(ConfigType::System);
        success &= loadDefaultConfig(ConfigType::Command);
        success &= loadDefaultConfig(ConfigType::Mqtt);
        success &= loadDefaultConfig(ConfigType::Passwords);

        return success;
    }

    bool ConfigManager::loadConfig(ConfigType type)
    {
        const char* path = getConfigPath(type);
        auto& doc       = getConfigDocument(type);
        
        doc.clear();
        
        // Если файл не существует, пробуем загрузить дефолтную конфигурацию
        if (!SPIFFS.exists(path)) {
            logger->log(Level::Warning, 
                      "[Config] '%s' отсутствует, попытка загрузки стандартных настроек", path);
            return loadDefaultConfig(type);
        }
        
        return loadJsonFromFile(path, doc);
    }

    bool ConfigManager::saveConfig(ConfigType type)
    {
        const char* path   = getConfigPath(type);
        const auto& doc    = getConfigDocument(type);
        
        return saveJsonToFile(path, doc);
    }

    bool ConfigManager::loadAllConfigs()
    {
        bool success = true;
        
        success &= loadConfig(ConfigType::Data);
        success &= loadConfig(ConfigType::System);
        success &= loadConfig(ConfigType::Command);
        success &= loadConfig(ConfigType::Mqtt);
        success &= loadConfig(ConfigType::Passwords);
        
        return success;
    }

    bool ConfigManager::saveAllConfigs()
    {
        bool success = true;
        
        success &= saveConfig(ConfigType::Data);
        success &= saveConfig(ConfigType::System);
        success &= saveConfig(ConfigType::Command);
        success &= saveConfig(ConfigType::Mqtt);
        success &= saveConfig(ConfigType::Passwords);
        
        logger->log(Level::Info, 
                  "[Config] Сохранение всех настроек %s", 
                  success ? "успешно" : "завершилось с ошибками");
        
        return success;
    }

    bool ConfigManager::hasKey(ConfigType type, const char* key) const
    {
        const auto& doc = getConfigDocument(type);
        // В ArduinoJson 7 вместо containsKey() рекомендуется использовать is<T>
        // Проверяем, существует ли ключ (не null)
        return !doc[key].isNull();
    }

    void ConfigManager::printConfig(ConfigType type) const
    {
        const auto& doc = getConfigDocument(type);
        
        // Сначала вычисляем размер необходимого буфера
        size_t jsonSize = measureJsonPretty(doc);
        
        String output;
        output.reserve(jsonSize + 1); // +1 для нулевого символа
        
        // Сериализуем в строку нужного размера
        serializeJsonPretty(doc, output);
        
        // Проверяем, что размер вывода не превышает максимально допустимый размер буфера лога
        // и что строка не содержит обрывков UTF-8 символов на границе
        // -58, т.к. в логе будет ещё "[Config] Текущая конфигурация %s:\n%s"
        // TODO: сделать по-человечески
        if (output.length() <= farm::log::constants::LOG_BUFFER_SIZE - 58)
        {
            logger->log(Level::Debug, 
                      "[Config] Текущий файл %s:\n%s", 
                      getConfigPath(type), output.c_str());
        }
        else 
        {
            if (logger->getLevel() >= Level::Debug)
            {
                logger->log(Level::Warning, 
                        "[Config] %s слишком большой для вывода в лог", 
                        getConfigPath(type));
                Serial.println(output);
            }
        }
    }

    String ConfigManager::getConfigJson(ConfigType type) const
    {
        const auto& doc = getConfigDocument(type);
        
        // Сначала вычислить размер необходимого буфера
        size_t jsonSize = measureJsonPretty(doc);
        // Затем создать строку нужного размера
        String jsonString;
        jsonString.reserve(jsonSize + 1); // +1 для нулевого терминатора
        // И только потом сериализовать
        serializeJson(doc, jsonString);
        
        return jsonString;
    }

    // Перезапись JSON файла из строки
    bool ConfigManager::updateFromJson(ConfigType type, const String& jsonString)
    {
        JsonDocument tempDoc;
        
        DeserializationError error = deserializeJson(tempDoc, jsonString);
        if (error) 
        {
            logger->log(Level::Error, 
                      "[Config] Ошибка десериализации JSON: %s", error.c_str());
            return false;
        }
        
        auto& targetDoc = getConfigDocument(type);
        
        // Рекурсивная функция для слияния JSON объектов с любой глубиной вложенности
        // TODO: написать функцию явно, а не использовать лямбду
        std::function<void(JsonVariant, JsonVariant)> mergeJson = 
            [&mergeJson](JsonVariant src, JsonVariant dest) 
            {
                // Если источник не является объектом, нечего сливать
                if (!src.is<JsonObject>()) 
                {
                    return;
                }
                
                JsonObject srcObj = src.as<JsonObject>();
                
                // Если целевой объект не существует, создаем его
                if (!dest.is<JsonObject>()) 
                {
                    dest.to<JsonObject>();
                }
                
                JsonObject destObj = dest.as<JsonObject>();
                
                // Обходим все ключи в источнике
                for (JsonPair kv : srcObj) 
                {
                    const char* key       = kv.key().c_str();
                    JsonVariant srcValue  = kv.value();
                    
                    // Если значение в источнике - объект, рекурсивно сливаем
                    if (srcValue.is<JsonObject>()) 
                    {
                        mergeJson(srcValue, destObj[key]);
                    } 
                    // Если значение в источнике - массив, заменяем целиком
                    else if (srcValue.is<JsonArray>()) 
                    {
                        destObj[key] = srcValue;
                    } 
                    // Для всех остальных типов просто копируем значение
                    else 
                    {
                        destObj[key] = srcValue;
                    }
                }
            };
        
        // Запускаем рекурсивное слияние
        mergeJson(tempDoc, targetDoc);
        
        logger->log(Level::Info, 
                  "[Config] %s обновлен из JSON-строки", 
                  getConfigPath(type));
        
        return true;
    }

    // Вывод информации о файловой системе SPIFFS через Serial; только для дебага
    void ConfigManager::printSpiffsInfo() const
    {
        #ifdef IOP_DEBUG
        Serial.println("Анализ файловой системы SPIFFS");
        
        if (!SPIFFS.begin(true))
        {
            Serial.println("Ошибка при монтировании SPIFFS!");
            return;
        }
        
        Serial.println("\n--- Информация о файловой системе ---");
        Serial.print("Общий размер: ");
        Serial.print(SPIFFS.totalBytes() / 1024);
        Serial.println(" КБ");
        
        Serial.print("Использовано: ");
        Serial.print(SPIFFS.usedBytes() / 1024);
        Serial.println(" КБ");
        
        Serial.print("Свободно: ");
        Serial.print((SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1024);
        Serial.println(" КБ");
        
        Serial.println("\n--- Список файлов ---");
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        int fileCount = 0;
        
        while (file)
        {
            fileCount++;
            Serial.println("\n--- Файл #" + String(fileCount) + " ---");
            Serial.print("Имя: ");
            Serial.println(file.name());
            
            Serial.print("Размер: ");
            Serial.print(file.size());
            Serial.println(" байт");
            
            Serial.println("Содержимое:");
            Serial.println("--------------------------------------");
            
            while (file.available())
            {
                String line = file.readStringUntil('\n');
                Serial.println(line);
            }
            
            Serial.println("--------------------------------------");
            
            file = root.openNextFile();
        }
        
        if (fileCount == 0)
        {
            Serial.println("Файлы не найдены!");
        }
        else
        {
            Serial.print("\nВсего файлов: ");
            Serial.println(fileCount);
        }
        
        Serial.println("\n===== Анализ файловой системы завершен =====");
        #endif
    }
}
