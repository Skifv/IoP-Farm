#pragma once
#include <Arduino.h>
#include <string>
#include <vector>
#include <memory>
#include "config/constants.h"

// Определяем наличие FreeRTOS
#if defined(CONFIG_FREERTOS_ENABLE) || defined(ESP_PLATFORM)
#define USE_FREERTOS 1
#endif

#ifdef USE_FREERTOS
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#endif

namespace farm::log
{
    // Транспорт для вывода сообщений
    class ILogTransport
    {
    public:
        virtual ~ILogTransport() = default;
        virtual void write(Level level, const String& message) = 0;
    };
    
    // Транспорт для Serial
    class SerialTransport : public ILogTransport
    {
    public:
        void write(Level level, const String& message) override;
    };

    // Транспорт для MQTT
    class MQTTLogTransport : public ILogTransport
    {
    private:
        std::vector<String> logBuffer;
        unsigned long lastSendTime;
#ifdef USE_FREERTOS
        SemaphoreHandle_t logMutex;
#endif

    public:
        MQTTLogTransport();
        ~MQTTLogTransport();
        
        void write(Level level, const String& message) override;
        
        // Принудительная отправка логов
        void flushLogs();
        
        void processLogs();
    };

    // Форматтер для сообщений
    class IMessageFormatter
    {
    public: 
        virtual ~IMessageFormatter() = default;
        virtual String format(Level level, const char* message) const = 0;
    };
    
    // Стандартный форматтер с префиксами
    class StandardFormatter : public IMessageFormatter
    {
    public:
        String format(Level level, const char* message) const override;
        static const char* getLevelPrefix(Level level);
    };
    
    // Цветной форматтер с ANSI-цветами
    class ColorFormatter : public StandardFormatter
    {
    public:
        String format(Level level, const char* message) const override;
        static const char* getLevelColor(Level level);
    };

    // Интерфейс логгера
    class ILogger
    {
    public:
        virtual void log(Level level, const char* format, ...) const = 0;
        virtual void setLevel(Level level) = 0;
        virtual Level getLevel() const = 0;
        virtual ~ILogger() = default;
    };

    // Реализация логгера
    class Logger : public ILogger
    {
    private:
        Level level;
        std::vector<std::shared_ptr<ILogTransport>> transports;
        std::shared_ptr<IMessageFormatter> formatter;
        
    public:
        Logger(std::shared_ptr<IMessageFormatter> formatter);
        
        void addTransport(std::shared_ptr<ILogTransport> transport);
        void log(Level level, const char* format, ...) const override;
        void setLevel(Level level) override;
        Level getLevel() const override;
        
    private:
        bool shouldLog(Level level) const;
    };
} 