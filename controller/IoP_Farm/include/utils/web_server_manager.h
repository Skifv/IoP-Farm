#pragma once

#include <WebServer.h>
#include <Update.h>
#include <memory>
#include "utils/logger.h"

namespace farm::net
{
    // Список кодов HTTP-статусов для более удобного чтения
    enum class HttpStatus
    {
        OK = 200,               // Успешный ответ
        CREATED = 201,          // Ресурс создан
        ACCEPTED = 202,         // Запрос принят, но не обработан
        NO_CONTENT = 204,       // Нет содержимого для ответа
        
        MOVED = 301,            // Ресурс перемещен навсегда
        FOUND = 302,            // Временное перенаправление
        SEE_OTHER = 303,        // См. другой ресурс
        NOT_MODIFIED = 304,     // Ресурс не изменился
        
        BAD_REQ = 400,          // Ошибка в запросе
        UNAUTH = 401,           // Требуется авторизация
        FORBIDDEN = 403,        // Доступ запрещен
        NOT_FOUND = 404,        // Ресурс не найден
        BAD_METHOD = 405,       // Метод не разрешен
        
        ERR_SERVER = 500,       // Внутренняя ошибка сервера
        NOT_IMPL = 501,         // Функционал не реализован
        BAD_GATEWAY = 502,      // Ошибка шлюза
        UNAVAILABLE = 503       // Сервис недоступен
    };
    
    // Типы MIME для удобства
    enum class Mime
    {
        HTML,                   // text/html
        PLAIN,                  // text/plain
    };
    
    // Класс для управления веб-сервером для обновления прошивки
    class WebServerManager
    {
    private:
        // Приватный конструктор (паттерн Синглтон)
        explicit WebServerManager(std::shared_ptr<farm::log::ILogger> logger = nullptr);
        
        static std::shared_ptr<WebServerManager> instance;
        
        std::shared_ptr<farm::log::ILogger> logger;
        
        WebServer server;
        
        bool isInitialized = false;
        
        // Данные для HTTP-аутентификации по умолчанию, изменяется в passwords.json
        String username = "admin";    
        String password = "admin";
        bool authEnabled = false;      
        
        void setupHandlers();
        
        // HTML шаблон для страницы обновления
        const char* getUpdateHtml();
        
        const char* getMimeStr(Mime mime);
        
        bool checkAuth();
        
        // Обработчики HTTP запросов
        void handleRoot();
        void handleUpdate();
        void handleDoUpdate();
        void handleNotFound();
        
    public:
        static std::shared_ptr<WebServerManager> getInstance(std::shared_ptr<farm::log::ILogger> logger = nullptr);
        
        // Запрещаем копирование и присваивание (паттерн Синглтон)
        WebServerManager(const WebServerManager&) = delete;
        WebServerManager& operator=(const WebServerManager&) = delete;
        
        ~WebServerManager();
        
        bool initialize();
        
        // Обработка запросов (вызывается в loop)
        void handleClient();
        
        // Методы для управления аутентификацией
        void setCredentials(const String& user, const String& pass);
        void enableAuth(bool enable);
        bool isAuthEnabled() const;
        String getUsername() const;
    };
} 