#pragma once
#include "logger.h"
#include <memory>

namespace farm::log
{
    // Фабрика для создания предварительно настроенных логгеров
    class LoggerFactory
    {
    public:
        // Стандартный логгер с Serial-выводом
        static std::shared_ptr<ILogger> createSerialLogger(Level level = Level::Info);
        
        // Цветной логгер с Serial-выводом
        static std::shared_ptr<ILogger> createColorSerialLogger(Level level = Level::Info);
        
        // Логгер с отправкой в MQTT
        static std::shared_ptr<ILogger> createMQTTLogger(Level level = Level::Info);

        // Цветной логгер с отправкой в MQTT
        static std::shared_ptr<ILogger> createColorMQTTLogger(Level level = Level::Info);

        // Логгер с отправкой в MQTT и Serial
        static std::shared_ptr<ILogger> createSerialMQTTLogger(Level level = Level::Info);
        
        // Цветной логгер с отправкой в MQTT и Serial
        static std::shared_ptr<ILogger> createColorSerialMQTTLogger(Level level = Level::Info);
    };
} 