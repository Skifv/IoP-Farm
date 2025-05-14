#pragma once

// Это не планировщик для параллельных задач, а "календарный" планировщик
// для выполнения задач в определенные моменты времени
// Тем не менее, он работает параллельно основному коду

#include <Arduino.h>
#include <GyverNTP.h>
#include <Stamp.h>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>
#include "utils/logger.h"
#include "config/constants.h"

// Определяем наличие FreeRTOS
#if defined(CONFIG_FREERTOS_ENABLE) || defined(ESP_PLATFORM)
#define USE_FREERTOS 1
#endif

#ifdef USE_FREERTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#endif

namespace farm::utils
{
    enum class ScheduleType
    {
        ONCE,       // Выполнить один раз
        PERIODIC    // Выполнять периодически
    };
    
    // Структура для события в планировщике
    struct ScheduledEvent
    {
        ScheduleType type;              // Тип события
        uint32_t scheduledTime;         // Время выполнения (unix-время)
        uint32_t periodSeconds = 0;     // Период повторения в секундах (для PERIODIC)
        uint32_t lastExecutionTime = 0; // Время последнего выполнения
        bool executed = false;          // Флаг выполнения (для ONCE)
        std::function<void()> callback; // Функция-обработчик
        
        // Идентификатор для удаления
        std::uint64_t id = 0;
    };
    
    class Scheduler 
    {
    private:
        static std::shared_ptr<Scheduler> instance;
        
        std::vector<ScheduledEvent> events;
        
        std::shared_ptr<farm::log::ILogger> logger;
        
        // ID для следующего события
        // uint64_t - 8 байт, поэтому даже если событие создается каждую секунду, 
        // то тип переполнится через ~584 млрд лет
        std::uint64_t nextEventId = 1;
        
        bool initialized = false;
        
        explicit Scheduler(std::shared_ptr<farm::log::ILogger> logger = nullptr);
        
        bool isTimeMatch(uint32_t currentTime, uint32_t scheduledTime) const;
        
        bool wasTodayExecuted(const ScheduledEvent& event, uint32_t currentTime) const;
        
#ifdef USE_FREERTOS
        // Хэндл задачи планировщика
        TaskHandle_t schedulerTaskHandle = nullptr;
        
        // Мьютекс для защиты доступа к вектору событий
        /* Зачем? Например, планировщик проходит по вектору событий, и в то же время генерируется 
         событие. Тогда вектор событий будет изменен, и планировщик будет проходить по нему некорректно
         поэтому мьютекс защищает вектор событий от изменения */
        SemaphoreHandle_t eventsMutex = nullptr;
        
        // Период проверки расписания в тиках RTOS
        TickType_t checkInterval = pdMS_TO_TICKS(farm::config::scheduler::DEFAULT_CHECK_INTERVAL_MS);
        
        // Для задачи планировщика
        bool taskShouldExit = false;
        
        // Статическая функция для запуска задачи в FreeRTOS
        static void schedulerTaskFunction(void* parameters);
#endif
        
    public:
        // Получить экземпляр (паттерн Синглтон)
        static std::shared_ptr<Scheduler> getInstance(std::shared_ptr<farm::log::ILogger> logger = nullptr);
        
        Scheduler(const Scheduler&) = delete;
        Scheduler& operator=(const Scheduler&) = delete;
        
        ~Scheduler();
        
        bool isNtpOnline() const;
        
        bool initialize(int8_t gmtOffset = farm::config::time::DEFAULT_GMT_OFFSET);
        
        bool isInitialized() const;
        
        // Добавить одноразовое событие через указанное количество секунд
        std::uint64_t scheduleOnceAfter(uint32_t afterSeconds, std::function<void()> callback);
        
        // Добавить одноразовое событие в конкретное время
        std::uint64_t scheduleOnceAt(const Datime& dateTime, std::function<void()> callback);
        std::uint64_t scheduleOnceAt(uint32_t unixTime, std::function<void()> callback);
        
        // Добавить периодическое событие с началом через указанное количество секунд
        std::uint64_t schedulePeriodicAfter(uint32_t afterSeconds, uint32_t periodSeconds, std::function<void()> callback);
        
        // Добавить периодическое событие с началом в конкретное время
        std::uint64_t schedulePeriodicAt(const Datime& dateTime, uint32_t periodSeconds, std::function<void()> callback);
        std::uint64_t schedulePeriodicAt(uint32_t unixTime, uint32_t periodSeconds, std::function<void()> callback);
        
        bool removeScheduledEvent(std::uint64_t eventId);
        
        void clearAllEvents();
        
        // Проверить и выполнить запланированные события (вызывать в loop)
        void checkSchedule();
        
        size_t getEventCount() const;
        
#ifdef USE_FREERTOS
        // Запустить задачу планировщика в FreeRTOS
        bool startSchedulerTask(uint8_t priority = farm::config::scheduler::SCHEDULER_TASK_PRIORITY, 
                              uint32_t stackSize = farm::config::scheduler::SCHEDULER_STACK_SIZE);
        
        bool stopSchedulerTask();
        
        bool isSchedulerTaskRunning() const;
        
        // Установка интервала проверки расписания (в миллисекундах)
        void setCheckInterval(uint32_t intervalMs);
#endif
    };
} 