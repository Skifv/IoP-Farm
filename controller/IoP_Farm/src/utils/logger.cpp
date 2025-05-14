#include "utils/logger.h"
#include "network/mqtt_manager.h"
#include <stdarg.h>

namespace farm::log
{
    using namespace farm::config;
    using namespace farm::log;

    // Реализация SerialTransport
    void SerialTransport::write(Level level, const String& message)
    {
        Serial.println(message);
    }

    // Реализация StandardFormatter
    String StandardFormatter::format(Level level, const char* message) const
    {
        return String(getLevelPrefix(level)) + message;
    }

    const char* StandardFormatter::getLevelPrefix(Level level)
    {
        switch (level)
        {
            case Level::Error:   return constants::ERROR_PREFIX;
            case Level::Warning: return constants::WARN_PREFIX;
            case Level::Info:    return constants::INFO_PREFIX;
            case Level::Farm:    return constants::FARM_PREFIX;
            case Level::Debug:   return constants::DEBUG_PREFIX;
            case Level::Test:    return constants::TEST_PREFIX;
            default:             return "";
        }
    }

    // Реализация ColorFormatter
    String ColorFormatter::format(Level level, const char* message) const
    {
        return String(getLevelColor(level)) + String(getLevelPrefix(level)) + 
               message + String(constants::COLOR_RESET);
    }
    
    const char* ColorFormatter::getLevelColor(Level level)
    {
        switch (level)
        {
            case Level::Error:   return constants::COLOR_RED;
            case Level::Warning: return constants::COLOR_YELLOW;
            case Level::Info:    return constants::COLOR_WHITE;
            case Level::Farm:    return constants::COLOR_GREEN;
            case Level::Debug:   return constants::COLOR_BLUE;
            case Level::Test:    return constants::COLOR_MAGENTA;
            default:             return constants::COLOR_WHITE;
        }
    }

    // Реализация MQTTLogTransport
    MQTTLogTransport::MQTTLogTransport() : lastSendTime(0), logBuffer()
    {
#ifdef USE_FREERTOS
        // Создаем мьютекс для безопасного доступа к буферу логов
        logMutex = xSemaphoreCreateMutex();
#endif
    }

    MQTTLogTransport::~MQTTLogTransport()
    {
#ifdef USE_FREERTOS
        if (logMutex != NULL) {
            vSemaphoreDelete(logMutex);
        }
#endif
    }
    
    void MQTTLogTransport::write(Level level, const String& message)
    {
        // Проверяем, что уровень сообщения достаточен для отправки
        if (static_cast<int>(level) <= static_cast<int>(constants::MQTT_LOG_MIN_LEVEL))
        {
#ifdef USE_FREERTOS
            if (logMutex != NULL && xSemaphoreTake(logMutex, portMAX_DELAY) == pdTRUE) 
            {
#endif
            // Проверяем размер буфера и удаляем старые сообщения, если он переполнен
            if (logBuffer.size() >= constants::MAX_BUFFER_SIZE)
            {
                logBuffer.erase(logBuffer.begin());
            }
            
            logBuffer.push_back(message);
                
#ifdef USE_FREERTOS
                xSemaphoreGive(logMutex);
            }
#endif
        
            processLogs();
        }
    }
    
    void MQTTLogTransport::flushLogs()
    {
#ifdef USE_FREERTOS
        if (logMutex == NULL || xSemaphoreTake(logMutex, portMAX_DELAY) != pdTRUE) {
            return;
        }
#endif
        
        auto mqttManager = farm::net::MQTTManager::getInstance();
        
        // Если буфер пуст или MQTT не настроен или не подключен, выходим
        if (logBuffer.empty() || !mqttManager->isMqttInitialized() || !mqttManager->isClientConnected()) 
        {
#ifdef USE_FREERTOS
            xSemaphoreGive(logMutex);
#endif
            return;
        }
        
        String logTopic = "/" + String(mqtt::DEFAULT_DEVICE_ID) + String(mqtt::LOG_SUFFIX);
        
        // Если количество сообщений меньше или равно MAX_PACKET_SIZE, отправляем все сразу
        if (logBuffer.size() <= constants::MAX_PACKET_SIZE)
        {
            String combinedLog;
            for (const auto& log : logBuffer) 
            {
                combinedLog += log + "\n";
            }
            
            mqttManager->publishToTopicLoggerVersion(logTopic, combinedLog, mqtt::QOS_1, true);
            
            logBuffer.clear();
            lastSendTime = millis();
        }
        // Если сообщений больше MAX_PACKET_SIZE, отправляем пакетами
        else
        {
            size_t numPackets = logBuffer.size() / constants::MAX_PACKET_SIZE;
            
            // Отправляем полные пакеты
            for (size_t i = 0; i < numPackets; i++)
            {
                String packetLog;
                size_t startIdx = i * constants::MAX_PACKET_SIZE;
                size_t endIdx = (i + 1) * constants::MAX_PACKET_SIZE;
                
                for (size_t j = startIdx; j < endIdx; j++)
                {
                    packetLog += logBuffer[j] + "\n";
                }
                
                mqttManager->publishToTopicLoggerVersion(logTopic, packetLog, mqtt::QOS_1, true);
            }
            
            // Отправляем оставшиеся сообщения (если есть)
            size_t remainingStart = numPackets * constants::MAX_PACKET_SIZE;
            if (remainingStart < logBuffer.size())
            {
                String remainingLog;
                for (size_t i = remainingStart; i < logBuffer.size(); i++)
                {
                    remainingLog += logBuffer[i] + "\n";
                }
                
                mqttManager->publishToTopicLoggerVersion(logTopic, remainingLog, mqtt::QOS_1, true);
            }
            
            logBuffer.clear();
            lastSendTime = millis();
        }
        
#ifdef USE_FREERTOS
        xSemaphoreGive(logMutex);
#endif
    }
    
    void MQTTLogTransport::processLogs()
    {
        auto mqttManager = farm::net::MQTTManager::getInstance();
        
        if (!mqttManager->isMqttInitialized())
        {
            return;
        }
        
#ifdef USE_FREERTOS
        if (logMutex != NULL && xSemaphoreTake(logMutex, portMAX_DELAY) == pdTRUE) 
        {
#endif
        // Проверяем, прошло ли достаточно времени с момента последней отправки
        unsigned long currentTime = millis();
        if ((!logBuffer.empty() && 
            (currentTime - lastSendTime >= constants::MQTT_LOG_SEND_INTERVAL) ||
            (logBuffer.size() >= constants::MAX_BUFFER_SIZE)) && 
            mqttManager->isClientConnected()) 
        {
#ifdef USE_FREERTOS
                // Освобождаем мьютекс перед вызовом flushLogs, так как он сам захватит мьютекс
                xSemaphoreGive(logMutex);
#endif
            flushLogs();
        }
#ifdef USE_FREERTOS
            else
            {
                xSemaphoreGive(logMutex);
            }
        }
#endif
    }

    // Реализация Logger
    Logger::Logger(std::shared_ptr<IMessageFormatter> formatter)
        : level(Level::Info), formatter(std::move(formatter))
    {
    }

    void Logger::addTransport(std::shared_ptr<ILogTransport> transport)
    {
        transports.push_back(std::move(transport));
    }

    void Logger::log(Level level, const char* format, ...) const
    {
        if (!shouldLog(level)) return;

        char buffer[constants::LOG_BUFFER_SIZE];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        String formattedMessage = formatter->format(level, buffer);

        for (const auto& transport : transports)
        {
            transport->write(level, formattedMessage);
        }
    }

    void Logger::setLevel(Level level)
    {
        this->level = level;
    }

    Level Logger::getLevel() const
    {
        return level;
    }

    bool Logger::shouldLog(Level level) const
    {
        return static_cast<int>(level) <= static_cast<int>(this->level);
    }
} 