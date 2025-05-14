#include "utils/web_server_manager.h"
#include <Arduino.h>
#include "utils/logger_factory.h"
#include "network/wifi_manager.h"
#include "config/config_manager.h"

namespace farm::net
{
    // Инициализация статического экземпляра
    std::shared_ptr<WebServerManager> WebServerManager::instance = nullptr;
    
    // Конструктор
    WebServerManager::WebServerManager(std::shared_ptr<farm::log::ILogger> logger)
        : server(), logger(logger)
    {
        // Если логгер не передан, создаем новый с помощью фабрики
        if (!this->logger) 
        {
            this->logger = farm::log::LoggerFactory::createSerialLogger(farm::log::Level::Info);
        }
    }
    
    // Деструктор
    WebServerManager::~WebServerManager()
    {
        // Если сервер был запущен, останавливаем его
        if (isInitialized)
        {
            server.stop();
            logger->log(farm::log::Level::Info, "[WebServer] Веб-сервер остановлен");
        }
    }
    
    // Получение экземпляра синглтона
    std::shared_ptr<WebServerManager> WebServerManager::getInstance(std::shared_ptr<farm::log::ILogger> logger)
    {
        if (instance == nullptr)
        {
            instance = std::shared_ptr<WebServerManager>(new WebServerManager(logger));
        }
        return instance;
    }
    
    // Вспомогательный метод для получения строки MIME-типа
    const char* WebServerManager::getMimeStr(Mime mime)
    {
        switch (mime)
        {
            case Mime::HTML:   return "text/html";
            case Mime::PLAIN:  return "text/plain";
            default:           return "text/plain";
        }
    }

    bool WebServerManager::initialize()
    {
        auto wifiManager = MyWiFiManager::getInstance();

        if (!wifiManager->isConnected())
        {
            logger->log(farm::log::Level::Warning, "[WebServer] Нет подключения к WiFi для инициализации веб-сервера");
            return false;
        }

        if (isInitialized)
        {
            logger->log(farm::log::Level::Info, "[WebServer] Веб-сервер уже был инициализирован");
            return true;
        }

        // Загрузка учетных данных из конфигурации
        auto configManager = farm::config::ConfigManager::getInstance(logger);
       
        if (configManager->hasKey(farm::config::ConfigType::Passwords, "WebServerUser") && 
            configManager->hasKey(farm::config::ConfigType::Passwords, "WebServerPass")) 
        {
            username = configManager->getValue<String>(farm::config::ConfigType::Passwords, "WebServerUser");
            password = configManager->getValue<String>(farm::config::ConfigType::Passwords, "WebServerPass");
        }
        else 
        {
            logger->log(farm::log::Level::Warning, "[WebServer] Не найдены учетные данные для веб-сервера в конфигурации");
        }
        
        setupHandlers();
        
        // Запуск сервера
        server.begin();
        isInitialized = true;
        
        logger->log(farm::log::Level::Farm, "[WebServer] Веб-сервер для обновления прошивки доступен по адресу: http://%s", 
            wifiManager->getIPAddress().c_str());
        
        return true;
    }
    
    void WebServerManager::setupHandlers()
    {
        // Главная страница с формой для загрузки прошивки
        server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
        
        // Обработчик для формы обновления
        server.on("/update", HTTP_GET, [this]() { this->handleUpdate(); });
        
        // Обработчик для загрузки файла прошивки
        server.on("/update", HTTP_POST, 
            [this]() {
                // Сервер отправляет результат после завершения загрузки
                server.sendHeader("Connection", "close");
                server.send(static_cast<int>(HttpStatus::OK), 
                          getMimeStr(Mime::PLAIN), 
                          Update.hasError() ? "FAIL" : "OK");
                delay(1000);
                if (!Update.hasError())
                {
                    ESP.restart();
                }
                else
                {
                    // TODO: Обработка ошибки обновления
                }
            },
            [this]() {
                // *Аутентификация была проверена ранее
                this->handleDoUpdate();
            }
        );
        
        server.onNotFound([this]() { this->handleNotFound(); });
    }
    
    // Обработка запросов (вызывается в loop)
    void WebServerManager::handleClient()
    {
        auto wifiManager = MyWiFiManager::getInstance();

        if (wifiManager->isConnected())
        {
            if (!isInitialized) 
            {
                if (!initialize()) 
                {
                    logger->log(farm::log::Level::Error, "[WebServer] Не удалось инициализировать веб-сервер");
                }
            }
            else
            {
                server.handleClient();
            }
        }
        else if (isInitialized)
        {
            // Если WiFi отключился, а сервер был запущен, останавливаем его
            server.stop();
            isInitialized = false;
            logger->log(farm::log::Level::Warning, "[WebServer] Веб-сервер остановлен из-за отсутствия WiFi подключения");
        }
    }
    
    // Обработчик для корневого URL
    void WebServerManager::handleRoot()
    {
        if (!checkAuth()) return;
        
        // Перенаправляем на страницу обновления
        server.sendHeader("Location", "/update", true);
        server.send(static_cast<int>(HttpStatus::FOUND), getMimeStr(Mime::PLAIN), "");
    }
    
    // Обработчик для страницы обновления
    void WebServerManager::handleUpdate()
    {
        if (!checkAuth()) return;
        
        server.send(static_cast<int>(HttpStatus::OK), getMimeStr(Mime::HTML), getUpdateHtml());
    }
    
    // Обработчик для процесса обновления
    void WebServerManager::handleDoUpdate()
    {
        if (!checkAuth()) return;
        
        HTTPUpload& upload = server.upload();
        
        if (upload.status == UPLOAD_FILE_START) 
        {
            logger->log(farm::log::Level::Info, "[WebServer] Обновление: начало приема файла '%s'", upload.filename.c_str());
            
            // Запуск процесса обновления
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) 
            {
                logger->log(farm::log::Level::Error, "[WebServer] недостаточно места для обновления");
                Update.printError(Serial);
            }
        } 
        else if (upload.status == UPLOAD_FILE_WRITE) 
        {
            // Пишем полученные данные в раздел обновления
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) 
            {
                logger->log(farm::log::Level::Error, "[WebServer] Ошибка записи данных обновления");
                Update.printError(Serial);
            }
        } 
        else if (upload.status == UPLOAD_FILE_END) 
        {
            if (Update.end(true)) 
            {
                logger->log(farm::log::Level::Info, 
                          "[WebServer] Обновление успешно завершено. Загружено %u байт. Перезагрузка...", 
                          upload.totalSize);
            } 
            else 
            {
                logger->log(farm::log::Level::Error, "[WebServer] Ошибка при завершении обновления");
                Update.printError(Serial);
            }
        }
    }
    
    // Обработчик для неизвестных запросов
    void WebServerManager::handleNotFound()
    {
        if (!checkAuth()) return;
        
        // Просто перенаправляем на страницу обновления
        server.sendHeader("Location", "/update", true);
        server.send(static_cast<int>(HttpStatus::FOUND), getMimeStr(Mime::PLAIN), "");
    }

        bool WebServerManager::isAuthEnabled() const
    { 
        return authEnabled; 
    }

    String WebServerManager::getUsername() const 
    { 
        return username; 
    }
    
    bool WebServerManager::checkAuth()
    {
        if (!authEnabled)
            return true;  // Аутентификация отключена
        
        if (!server.authenticate(username.c_str(), password.c_str()))
        {   
            server.requestAuthentication();
            logger->log(farm::log::Level::Warning, "[WebServer] Попытка доступа без авторизации");
            return false;
        }
        
        return true;  // Аутентификация успешна
    }
    
    void WebServerManager::setCredentials(const String& user, const String& pass)
    {
        username = user;
        password = pass;
        logger->log(farm::log::Level::Info, "[WebServer] Установлены новые учетные данные для веб-сервера");
        
        // Сохраняем в configManager
        auto configManager = farm::config::ConfigManager::getInstance(logger);
        configManager->setValue(farm::config::ConfigType::Passwords, "WebServerUser", user);
        configManager->setValue(farm::config::ConfigType::Passwords, "WebServerPass", pass);
        configManager->saveConfig(farm::config::ConfigType::Passwords);
    }
    
    // Включение/отключение аутентификации
    void WebServerManager::enableAuth(bool enable)
    {
        authEnabled = enable;
        logger->log(farm::log::Level::Info, "[WebServer] Аутентификация для веб-сервера %s", 
                  enable ? "включена" : "отключена");
    }

    // HTML шаблон для страницы обновления
    const char* WebServerManager::getUpdateHtml()
    {
        static const char updateHtml[] PROGMEM = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>IoP Farm - Обновление прошивки</title>
            <style>
                body { font-family: Arial, sans-serif; margin: 0; padding: 20px; color: #333; }
                h1 { color: #4CAF50; }
                .container { max-width: 600px; margin: 0 auto; }
                .card { background: #fff; border-radius: 5px; padding: 15px; margin-bottom: 20px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
                .form-group { margin-bottom: 15px; }
                label { display: block; margin-bottom: 5px; font-weight: bold; }
                progress { width: 100%; height: 20px; }
                .btn { background-color: #4CAF50; color: white; border: none; padding: 10px 15px; text-align: center; cursor: pointer; border-radius: 4px; }
                .alert { padding: 15px; border-radius: 4px; margin-bottom: 15px; display: none; }
                .alert-success { background-color: #dff0d8; color: #3c763d; border: 1px solid #d6e9c6; }
                .alert-danger { background-color: #f2dede; color: #a94442; border: 1px solid #ebccd1; }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>IoP Farm - Обновление прошивки</h1>
                
                <div class="alert alert-success" id="successAlert">Прошивка успешно загружена. Устройство перезагружается...</div>
                <div class="alert alert-danger" id="errorAlert">Ошибка при загрузке прошивки</div>
                
                <div class="card">
                    <h2>Загрузка новой прошивки</h2>
                    <form method="POST" action="/update" enctype="multipart/form-data" id="uploadForm">
                        <div class="form-group">
                            <label for="firmware">Выберите файл прошивки (.bin):</label>
                            <input type="file" id="firmware" name="firmware" accept=".bin">
                        </div>
                        <div class="form-group" id="progressContainer" style="display:none;">
                            <label>Прогресс загрузки:</label>
                            <progress id="progressBar" value="0" max="100"></progress>
                            <span id="progressStatus">0%</span>
                        </div>
                        <button class="btn" type="submit" id="uploadBtn">Загрузить прошивку</button>
                    </form>
                </div>

                <div class="footer">
                    &copy; IoP Farm. Версия системы: 1.0
                </div>
            </div>
            
            <script>
                document.getElementById('uploadForm').addEventListener('submit', function(e) {
                    e.preventDefault();
                    
                    const fileInput = document.getElementById('firmware');
                    if (!fileInput.files.length) {
                        showAlert('errorAlert', 'Пожалуйста, выберите файл прошивки');
                        return;
                    }
                    
                    const file = fileInput.files[0];
                    const formData = new FormData();
                    formData.append('firmware', file);
                    
                    const xhr = new XMLHttpRequest();
                    
                    xhr.open('POST', '/update');
                    
                    // Отображение прогресса загрузки
                    document.getElementById('progressContainer').style.display = 'block';
                    document.getElementById('uploadBtn').disabled = true;
                    
                    xhr.upload.addEventListener('progress', function(e) {
                        if (e.lengthComputable) {
                            const percentComplete = (e.loaded / e.total) * 100;
                            document.getElementById('progressBar').value = percentComplete;
                            document.getElementById('progressStatus').textContent = Math.round(percentComplete) + '%';
                        }
                    });
                    
                    xhr.onload = function() {
                        if (xhr.status === 200) {
                            showAlert('successAlert', 'Прошивка успешно загружена. Устройство перезагружается...');
                            setTimeout(function() {
                                window.location.reload();
                            }, 15000);
                        } else {
                            showAlert('errorAlert', 'Ошибка при загрузке прошивки: ' + xhr.statusText);
                            document.getElementById('uploadBtn').disabled = false;
                        }
                    };
                    
                    xhr.onerror = function() {
                        showAlert('errorAlert', 'Ошибка при загрузке прошивки');
                        document.getElementById('uploadBtn').disabled = false;
                    };
                    
                    xhr.send(formData);
                });
                
                function showAlert(id, message) {
                    const alert = document.getElementById(id);
                    alert.textContent = message;
                    alert.style.display = 'block';
                    
                    if (id === 'errorAlert') {
                        setTimeout(() => {
                            alert.style.display = 'none';
                        }, 5000);
                    }
                }
            </script>
        </body>
        </html>
        )rawliteral";
        return updateHtml;
    }
}
