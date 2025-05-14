#include "utils/scheduler.h"
#include "utils/logger_factory.h"
#include "config/constants.h"
#include <cstdint>

using namespace farm::config::scheduler;
using namespace farm::config;
namespace farm::utils
{
    // Инициализация статического члена (паттерн Singleton)
    std::shared_ptr<Scheduler> Scheduler::instance = nullptr;
    
    Scheduler::Scheduler(std::shared_ptr<farm::log::ILogger> logger)
    {
        // Инициализация логгера, если не передан, то создаем логгер по умолчанию
        if (!logger) 
        {
            this->logger = farm::log::LoggerFactory::createSerialLogger(farm::log::Level::Info);
        } 
        else 
        {
            this->logger = logger;
        }
    }
    
    Scheduler::~Scheduler()
    {
#ifdef USE_FREERTOS
        // Останавливаем задачу планировщика, если она запущена
        stopSchedulerTask();
        
        if (eventsMutex != nullptr)
        {
            vSemaphoreDelete(eventsMutex);
            eventsMutex = nullptr;
        }
#endif
        
        logger->log(farm::log::Level::Debug, 
                  "[Scheduler] Освобождение ресурсов планировщика");
    }

    bool Scheduler::isNtpOnline() const
    {
        return NTP.online();
    }
    
    bool Scheduler::initialize(int8_t gmtOffset)
    {
        if (initialized)
        {
            logger->log(farm::log::Level::Warning,
                      "[Scheduler] Планировщик уже был инициализирован");
            return true;
        }
        
        logger->log(farm::log::Level::Farm,
                  "[Scheduler] Инициализация планировщика задач");
                  
        // Инициализация NTP
        NTP.setPeriod(time::NTP_PERIOD); // период синхронизации времени с NTP сервером
        NTP.begin(gmtOffset);

        delay(100);
        
#ifdef USE_FREERTOS
        // Инициализация мьютекса для синхронизации доступа к списку событий
        eventsMutex = xSemaphoreCreateMutex();
        if (eventsMutex == nullptr)
        {
            logger->log(farm::log::Level::Error, 
                      "[Scheduler] Не удалось создать мьютекс событий");
            return false;
        }
        
        logger->log(farm::log::Level::Debug,
                  "[Scheduler] Мьютекс событий создан");
#endif

        if (NTP.updateNow())
        {
            logger->log(farm::log::Level::Debug, 
                      "[Scheduler] NTP синхронизирован, текущее время: %s", NTP.toString().c_str());
        }
        else
        {
            // Далее просто будет проверка NTP.online() == true/false во всех методах Scheduler'a
            logger->log(farm::log::Level::Warning, 
                      "[Scheduler] NTP не синхронизирован, все функции планировщика отключены");
            logger->log(farm::log::Level::Warning, 
                      "[Scheduler] Подключите устройство к WiFi либо синхронизируйте время по RTC");
        }

        initialized = true;
        return true;
    }

    bool Scheduler::isInitialized() const
    {
        return initialized;
    }
    
    // Получение экземпляра (паттерн Singleton)
    std::shared_ptr<Scheduler> Scheduler::getInstance(std::shared_ptr<farm::log::ILogger> logger)
    {
        if (instance == nullptr) 
        {
            // Используем явное создание вместо make_shared, т.к. конструктор приватный
            instance = std::shared_ptr<Scheduler>(new Scheduler(logger));
        }
        return instance;
    }
    
    // Добавление одноразового события через указанное количество секунд
    std::uint64_t Scheduler::scheduleOnceAfter(uint32_t afterSeconds, std::function<void()> callback)
    {
        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка добавить событие до инициализации планировщика");
            return 0;
        }

        if (!isNtpOnline()) return 0;
        
        uint32_t targetTime = NTP.getUnix() + afterSeconds;
        return scheduleOnceAt(targetTime, callback);
    }
    
    // Добавление одноразового события в конкретное время (Datime)
    std::uint64_t Scheduler::scheduleOnceAt(const Datime& dateTime, std::function<void()> callback)
    {
        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка добавить событие до инициализации планировщика");
            return 0;
        }
        
        return scheduleOnceAt(dateTime.getUnix(), callback);
    }
    
    // Добавление одноразового события в конкретное время (unix)
    std::uint64_t Scheduler::scheduleOnceAt(uint32_t unixTime, std::function<void()> callback)
    {
        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка добавить событие до инициализации планировщика");
            return 0;
        }
        
        if (!isNtpOnline()) return 0;
        
        if (!callback) 
        {
            logger->log(farm::log::Level::Error, 
                    "[Scheduler] Попытка добавить событие с пустым обработчиком");
            return 0;
        }
         
        uint32_t currentTime = NTP.getUnix();
        
        // Если запланированное время уже прошло, переносим на завтрашний день
        if (unixTime < currentTime) 
        {
            Datime unixTimeDatime(unixTime);
            logger->log(farm::log::Level::Info, 
                       "[Scheduler] Запланированное время %s уже прошло, переносим на завтра", unixTimeDatime.toString().c_str());
            
            // Извлекаем компоненты времени из unixTime (нам нужны только часы, минуты и секунды)
            Datime origTime(unixTime);
            
            Datime currentDate(currentTime);
            
            time_t tomorrow_time = currentTime + 24*60*60; // текущее время + 24 часа
            Datime tomorrow(tomorrow_time);
            
            tomorrow.hour = origTime.hour;
            tomorrow.minute = origTime.minute;
            tomorrow.second = origTime.second;
        
            unixTime = tomorrow.getUnix();
            
            logger->log(farm::log::Level::Info, 
                       "[Scheduler] Новое запланированное время: %s", tomorrow.toString());
        }
        
        // Создаем новое одноразовое событие
        ScheduledEvent event;
        event.type = ScheduleType::ONCE;
        event.scheduledTime = unixTime;
        event.callback = callback;
        event.id = nextEventId++;

#ifdef USE_FREERTOS
        // Блокируем доступ к events с помощью мьютекса
        if (eventsMutex != nullptr && xSemaphoreTake(eventsMutex, portMAX_DELAY) != pdTRUE)
        {
            logger->log(farm::log::Level::Error, 
                      "[Scheduler] Не удалось получить мьютекс для добавления события");
            return 0;
        }
#endif

        events.push_back(event);
        
        Datime dt(unixTime);

        logger->log(farm::log::Level::Debug, 
                  "[Scheduler] Добавлено однократное событие #%llu на %s", 
                  event.id, dt.toString().c_str());
    
        
#ifdef USE_FREERTOS
        // Освобождаем мьютекс
        if (eventsMutex != nullptr)
        {
            xSemaphoreGive(eventsMutex);
        }
#endif

        return event.id;
    }
    
    // Добавление периодического события с началом через указанное количество секунд
    std::uint64_t Scheduler::schedulePeriodicAfter(uint32_t afterSeconds, uint32_t periodSeconds, std::function<void()> callback)
    {
        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка добавить событие до инициализации планировщика");
            return 0;
        }
        
        if (!isNtpOnline()) return 0;
        
        uint32_t targetTime = NTP.getUnix() + afterSeconds;
        return schedulePeriodicAt(targetTime, periodSeconds, callback);
    }
    
    // Добавление периодического события с началом в конкретное время (Datime)
    std::uint64_t Scheduler::schedulePeriodicAt(const Datime& dateTime, uint32_t periodSeconds, std::function<void()> callback)
    {
        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка добавить событие до инициализации планировщика");
            return 0;
        }
        
        return schedulePeriodicAt(dateTime.getUnix(), periodSeconds, callback);
    }
    
    // Добавление периодического события с началом в конкретное время (unix)
    std::uint64_t Scheduler::schedulePeriodicAt(uint32_t unixTime, uint32_t periodSeconds, std::function<void()> callback)
    {
        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка добавить событие до инициализации планировщика");
            return 0;
        }
        
        if (!isNtpOnline()) return 0;
        
        if (!callback) 
        {
            logger->log(farm::log::Level::Error, 
                    "[Scheduler] Попытка добавить событие с пустым обработчиком");
            return 0;
        }
        
        if (periodSeconds == 0) 
        {
            logger->log(farm::log::Level::Error, 
                      "[Scheduler] Период не может быть равен нулю");
            return 0;
        }
        
        uint32_t currentTime = NTP.getUnix();
        
        // Если запланированное время уже прошло, переносим первый запуск на завтрашний день
        if (unixTime < currentTime) 
        {
            Datime unixTimeDatime(unixTime);
            logger->log(farm::log::Level::Info, 
                       "[Scheduler] Запланированное время периодического события %s уже прошло, переносим первый запуск на завтра", unixTimeDatime.toString().c_str());
            
            // Извлекаем компоненты времени из unixTime (нам нужны только часы, минуты и секунды)
            Datime origTime(unixTime);
            
            Datime currentDate(currentTime);
            
            time_t tomorrow_time = currentTime + 24*60*60; // текущее время + 24 часа
            Datime tomorrow(tomorrow_time);
            
            tomorrow.hour = origTime.hour;
            tomorrow.minute = origTime.minute;
            tomorrow.second = origTime.second;
            
            unixTime = tomorrow.getUnix();
            
            logger->log(farm::log::Level::Info, 
                       "[Scheduler] Новое время первого запуска: %s", tomorrow.toString().c_str());
        }
        
        // Создаем новое периодическое событие
        ScheduledEvent event;
        event.type = ScheduleType::PERIODIC;
        event.scheduledTime = unixTime;
        event.periodSeconds = periodSeconds;
        event.callback = callback;
        event.id = nextEventId++;

#ifdef USE_FREERTOS
        // Блокируем доступ к events с помощью мьютекса
        if (eventsMutex != nullptr && xSemaphoreTake(eventsMutex, portMAX_DELAY) != pdTRUE)
        {
            logger->log(farm::log::Level::Error, 
                      "[Scheduler] Не удалось получить мьютекс для добавления периодического события");
            return 0;
        }
#endif

        events.push_back(event);
        
        Datime dt(unixTime);
        logger->log(farm::log::Level::Debug, 
                  "[Scheduler] Добавлено периодическое событие #%llu с началом %s и периодом %d с.", 
                  event.id, dt.toString().c_str(), periodSeconds);
    
        
#ifdef USE_FREERTOS
        // Освобождаем мьютекс
        if (eventsMutex != nullptr)
        {
            xSemaphoreGive(eventsMutex);
        }
#endif

        return event.id;
    }

    bool Scheduler::removeScheduledEvent(std::uint64_t eventId)
    {
        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                    "[Scheduler] Попытка удалить событие до инициализации планировщика");
            return false;
        }

        if (!isNtpOnline()) return false;
        
#ifdef USE_FREERTOS
        // Блокируем доступ к events с помощью мьютекса
        if (eventsMutex != nullptr && xSemaphoreTake(eventsMutex, portMAX_DELAY) != pdTRUE)
        {
            logger->log(farm::log::Level::Error, 
                      "[Scheduler] Не удалось получить мьютекс для удаления события");
            return false;
        }
#endif
        
        auto it = std::find_if(events.begin(), events.end(), 
                            [eventId](const ScheduledEvent& e) { return e.id == eventId; });
        
        bool result = false;
        
        if (it != events.end()) 
        {
            logger->log(farm::log::Level::Debug,
                      "[Scheduler] Удалено событие с ID %llu", eventId);
            events.erase(it);
            result = true;
        }
        else 
        {
            logger->log(farm::log::Level::Warning, 
                      "[Scheduler] Событие с ID %llu не найдено", eventId);
            result = false;
        }
        
#ifdef USE_FREERTOS
        // Освобождаем мьютекс
        if (eventsMutex != nullptr)
        {
            xSemaphoreGive(eventsMutex);
        }
#endif
        
        return result;
    }
    
    void Scheduler::clearAllEvents()
    {
        if (!isNtpOnline()) return;

        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка очистить события до инициализации планировщика");
            return;
        }
        
#ifdef USE_FREERTOS
        // Блокируем доступ к events с помощью мьютекса
        if (eventsMutex != nullptr && xSemaphoreTake(eventsMutex, portMAX_DELAY) != pdTRUE)
        {
            logger->log(farm::log::Level::Error, 
                      "[Scheduler] Не удалось получить мьютекс для очистки событий");
            return;
        }
#endif
        
        events.clear();
        logger->log(farm::log::Level::Info, 
                  "[Scheduler] Все события удалены");
                  
#ifdef USE_FREERTOS
        // Освобождаем мьютекс
        if (eventsMutex != nullptr)
        {
            xSemaphoreGive(eventsMutex);
        }
#endif
    }
    
    bool Scheduler::isTimeMatch(uint32_t currentTime, uint32_t scheduledTime) const
    {
        return currentTime >= scheduledTime;
    }
    
    bool Scheduler::wasTodayExecuted(const ScheduledEvent& event, uint32_t currentTime) const
    {
        if (event.lastExecutionTime == 0) 
        {
            return false;
        }
        
        Datime current(currentTime);
        Datime startOfDay(current.year, current.month, current.day, 0, 0, 0);
        
        return event.lastExecutionTime >= startOfDay.getUnix();
    }
    
    // Проверка и выполнение запланированных событий, вызывается из задачи планировщика или из main.cpp в loop()
    void Scheduler::checkSchedule()
    {
        if (!initialized) 
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка проверить расписание до инициализации планировщика");
            return;
        }
        
        if (!isNtpOnline()) return;
        
        uint32_t currentTime = NTP.getUnix();
        
#ifdef USE_FREERTOS
        // Блокируем доступ к events с помощью мьютекса
        if (eventsMutex != nullptr && xSemaphoreTake(eventsMutex, portMAX_DELAY) != pdTRUE)
        {
            logger->log(farm::log::Level::Error, 
                      "[Scheduler] Не удалось получить мьютекс для проверки расписания");
            return;
        }
#endif
        
        // Создаем временный список для хранения событий, которые нужно удалить
        std::vector<size_t> eventsToRemove;
        
        // Обрабатываем все события
        for (size_t i = 0; i < events.size(); i++) 
        {
            auto& event = events[i];
            
            switch (event.type) 
            {
                case ScheduleType::ONCE:
                    // Проверяем одноразовые события
                    if (isTimeMatch(currentTime, event.scheduledTime) && !event.executed) 
                    {
                        logger->log(farm::log::Level::Debug, 
                                  "[Scheduler] Выполнение одноразового события #%llu", event.id);
                        
                        // Сохраняем копию callback перед освобождением мьютекса
                        // Зачем? callback может быть удален из вектора событий из основного кода
                        // во время выполнения callback, и тогда мы получим segmentation fault
                        auto callback = event.callback;
                        
                        event.executed = true;
                        event.lastExecutionTime = currentTime;
                        
                        eventsToRemove.push_back(i);
                        
#ifdef USE_FREERTOS
                        if (eventsMutex != nullptr)
                        {
                            xSemaphoreGive(eventsMutex);
                        }
#endif
    
                        callback();
                        
#ifdef USE_FREERTOS
                        if (eventsMutex != nullptr && xSemaphoreTake(eventsMutex, portMAX_DELAY) != pdTRUE)
                        {
                            logger->log(farm::log::Level::Error, 
                                      "[Scheduler] Не удалось получить мьютекс после выполнения события");
                            return;
                        }
#endif
                    }
                    break;
                    
                case ScheduleType::PERIODIC:
                    // Проверяем периодические события
                    if (isTimeMatch(currentTime, event.scheduledTime)) 
                    {
                        // Если событие ещё не выполнялось или прошло достаточно времени с последнего выполнения
                        if (event.lastExecutionTime == 0 || 
                            (currentTime - event.lastExecutionTime) >= event.periodSeconds) 
                        {
                            logger->log(farm::log::Level::Debug, 
                                      "[Scheduler] Выполнение периодического события #%llu", event.id);
                            
                            // Сохраняем копию callback перед освобождением мьютекса
                            auto callback = event.callback;
                            
                            event.lastExecutionTime = currentTime;
                            
#ifdef USE_FREERTOS
                            if (eventsMutex != nullptr)
                            {
                                xSemaphoreGive(eventsMutex);
                            }
#endif
                            
                            callback();
                            
#ifdef USE_FREERTOS

                            if (eventsMutex != nullptr && xSemaphoreTake(eventsMutex, portMAX_DELAY) != pdTRUE)
                            {
                                logger->log(farm::log::Level::Error, 
                                          "[Scheduler] Не удалось получить мьютекс после выполнения события");
                                return;
                            }
#endif

                        }
                    }
                    break;

                default:
                    logger->log(farm::log::Level::Error,
                              "[Scheduler] Неизвестный тип события: %d", static_cast<int>(event.type));
                    break;
            }
        }        
        
        // Удаляем выполненные одноразовые события
        events.erase(
            std::remove_if(events.begin(), events.end(),
                       [](const ScheduledEvent& e) { 
                           return e.type == ScheduleType::ONCE && e.executed; 
                       }),
            events.end()
        );
        
#ifdef USE_FREERTOS
        if (eventsMutex != nullptr)
        {
            xSemaphoreGive(eventsMutex);
        }
#endif
    }

    size_t Scheduler::getEventCount() const
    {
        return events.size();
    }

#ifdef USE_FREERTOS

    // Функция задачи FreeRTOS
    void Scheduler::schedulerTaskFunction(void *parameters)
    {
        Scheduler* scheduler = static_cast<Scheduler*>(parameters);
        
        while (!scheduler->taskShouldExit)
        {
            scheduler->checkSchedule();
            
            vTaskDelay(scheduler->checkInterval);
        }
        
        scheduler->logger->log(farm::log::Level::Farm, 
                           "[Scheduler] Задача планировщика остановлена");
        
        vTaskDelete(NULL);
    }
    
    bool Scheduler::startSchedulerTask(uint8_t priority, uint32_t stackSize)
    {
        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка запустить задачу планировщика до инициализации");
            return false;
        }
    
        if (isSchedulerTaskRunning())
        {
            logger->log(farm::log::Level::Warning, 
                      "[Scheduler] Задача планировщика уже запущена");
            return false;
        }
        
        taskShouldExit = false;
        
        BaseType_t result = xTaskCreate(
            schedulerTaskFunction,   // Функция задачи планировщика
            "SchedulerTask",         // Имя задачи планировщика
            stackSize,               // Размер стека 
            this,                    // Параметр (указатель на экземпляр класса)
            priority,                // Приоритет задачи
            &schedulerTaskHandle     // Хэндл задачи для управления
        );
        
        if (result != pdPASS)
        {
            logger->log(farm::log::Level::Error, 
                      "[Scheduler] Не удалось создать задачу планировщика");
            return false;
        }
        
        logger->log(farm::log::Level::Farm, 
                  "[Scheduler] Задача планировщика запущена с приоритетом %d", priority);
        
        return true;
    }
    
    bool Scheduler::stopSchedulerTask()
    {
        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка остановить задачу планировщика до инициализации");
            return false;
        }
        
        if (!isSchedulerTaskRunning())
        {
            logger->log(farm::log::Level::Warning, 
                      "[Scheduler] Задача планировщика не запущена");
            return false;
        }
        
        taskShouldExit = true;
        
        // Ждем завершения задачи (с таймаутом)
        for (int i = 0; i < MAX_STOP_ATTEMPTS; i++)
        {
            if (!isSchedulerTaskRunning())
            {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(TASK_STOP_CHECK_INTERVAL_MS));
        }
        
        // Если задача ВСЕ ЕЩЕ запущена, удаляем ее принудительно
        if (isSchedulerTaskRunning())
        {
            logger->log(farm::log::Level::Warning, 
                      "[Scheduler] Задача планировщика не остановилась самостоятельно, удаляем принудительно");
            vTaskDelete(schedulerTaskHandle);
        }
        
        schedulerTaskHandle = nullptr;
        
        logger->log(farm::log::Level::Farm, 
                  "[Scheduler] Задача планировщика остановлена");
        
        return true;
    }
    
    bool Scheduler::isSchedulerTaskRunning() const
    {
        if (!initialized)
        {
            return false;
        }
        
        if (schedulerTaskHandle == nullptr)
        {
            return false;
        }
        
        eTaskState state = eTaskGetState(schedulerTaskHandle);
        
        return (state != eDeleted && state != eInvalid);
    }

    // Установление интервал работы планировщика
    void Scheduler::setCheckInterval(uint32_t intervalMs)
    {
        if (!initialized)
        {
            logger->log(farm::log::Level::Error,
                     "[Scheduler] Попытка установить интервал до инициализации планировщика");
            return;
        }
        
        checkInterval = pdMS_TO_TICKS(intervalMs);
        
        logger->log(farm::log::Level::Debug, 
                  "[Scheduler] Установлен интервал проверки расписания: %d мс", intervalMs);
    }
#endif
} 