#include "utils/logger_factory.h"

namespace farm::log
{
    // Создание стандартного логгера с выводом в Serial
    std::shared_ptr<ILogger> LoggerFactory::createSerialLogger(Level level)
    {
        auto formatter = std::make_shared<StandardFormatter>();
        auto logger = std::make_shared<Logger>(formatter);
        
        auto transport = std::make_shared<SerialTransport>();
        logger->addTransport(transport);
    
        logger->setLevel(level);
        
        return logger;
    }
    
    // Создание цветного логгера с выводом в Serial
    std::shared_ptr<ILogger> LoggerFactory::createColorSerialLogger(Level level)
    {
        auto formatter = std::make_shared<ColorFormatter>();
        auto logger = std::make_shared<Logger>(formatter);
        auto transport = std::make_shared<SerialTransport>();
        logger->addTransport(transport);
        
        logger->setLevel(level);
        
        return logger;
    }

    // Создать логгер с отправкой в MQTT
    std::shared_ptr<ILogger> LoggerFactory::createMQTTLogger(Level level)
    {
        auto formatter = std::make_shared<StandardFormatter>();
        auto logger = std::make_shared<Logger>(formatter);
        
        auto transport = std::make_shared<MQTTLogTransport>();
        logger->addTransport(transport);
        
        logger->setLevel(level);
        
        return logger;
    }

    // Создать цветной логгер с отправкой в MQTT
    std::shared_ptr<ILogger> LoggerFactory::createColorMQTTLogger(Level level)
    {
        auto formatter = std::make_shared<ColorFormatter>();
        auto logger = std::make_shared<Logger>(formatter);
        
        auto transport = std::make_shared<MQTTLogTransport>();
        logger->addTransport(transport);
        
        logger->setLevel(level);
        
        return logger;
    }

    // Создать логгер с отправкой в MQTT и Serial
    std::shared_ptr<ILogger> LoggerFactory::createSerialMQTTLogger(Level level)
    {
        auto formatter = std::make_shared<StandardFormatter>();
        auto logger = std::make_shared<Logger>(formatter);
        
        auto serialTransport = std::make_shared<SerialTransport>();
        logger->addTransport(serialTransport);
        
        auto mqttTransport = std::make_shared<MQTTLogTransport>();
        logger->addTransport(mqttTransport);

        logger->setLevel(level);

        return logger;
    }

    // Создать цветной логгер с отправкой в MQTT и Serial
    std::shared_ptr<ILogger> LoggerFactory::createColorSerialMQTTLogger(Level level)
    {
        auto formatter = std::make_shared<ColorFormatter>();
        auto logger = std::make_shared<Logger>(formatter);
        
        auto serialTransport = std::make_shared<SerialTransport>();
        logger->addTransport(serialTransport);
        
        auto mqttTransport = std::make_shared<MQTTLogTransport>();
        logger->addTransport(mqttTransport);

        logger->setLevel(level);
        return logger;
    }
} 