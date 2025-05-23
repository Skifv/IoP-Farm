# IoP-Farm: Умная автоматизированная ферма на ESP32

**Версия:** 1.0  
**Дата:** 10.05.2025

---

## Оглавление

- [О проекте](#о-проекте)
- [Краткие возможности](#краткие-возможности)
- [Быстрый старт: PlatformIO environments и первичная загрузка](#быстрый-старт-platformio-environments-и-первичная-загрузка)
- [Архитектура и основные компоненты](#архитектура-и-основные-компоненты)
- [Изменяемые параметры в constants.h](#изменяемые-параметры-в-constantsh)
- [Сервер-посредник и хранение данных](#сервер-посредник-и-хранение-данных)
- [Управление с телефона и мониторинг через Android-приложение IoP Farm](#управление-с-телефона-и-мониторинг-через-android-приложение-iop-farm)
- [OTA-обновления](#ota-обновления)
- [Масштабирование и расширение](#масштабирование-и-расширение)
- [Безопасность](#безопасность)
- [FAQ и советы](#faq-и-советы)
- [Важные замечания](#важные-замечания)
- [Контакты и поддержка](#контакты-и-поддержка)


---

## О проекте

**IoP-Farm** — это модульная система для автоматизации полива домашних растений на базе ESP32. Система поддерживает работу с различными датчиками и исполнительными устройствами, интеграцию с WiFi/MQTT, удаленное управление и обновление прошивки по воздуху (OTA). Проект ориентирован на максимальную надежность, расширяемость и простоту первичной настройки.

---

## Краткие возможности

- Автоматический полив по расписанию и объему воды
- Контроль температуры и автоматическое управление нагревом
- Управление освещением по расписанию
- Поддержка WiFi 2.4 ГГц (WPA2/WPA3, STA и точка доступа) для управления с телефона.
- MQTT-интеграция для сбора данных и управления
- Гибкая система конфигурирования (web-интерфейс, WiFiManager, настройка с приложения на Android)
- OTA-обновления (PlatformIO, web-интерфейс, WiFiManager)
- Логирование (Serial, MQTT)
- Модульная архитектура: легко добавлять новые датчики, актуаторы, стратегии
- Хранение всех настроек в энергонезависимой памяти (SPIFFS), автоматическая загрузка последних/дефолтных конфигов в зависимости флага компиляции
- Безопасность: аутентификация web-интерфейса при загрузки прошивки по OTA
- Масштабируемость: поддержка новых топиков MQTT, датчиков и актуаторов, стратегий работы

---

## Быстрый старт: PlatformIO environments и первичная загрузка

### Как устроен platformio.ini

В проекте определено несколько сред (environments) для разных сценариев работы и отладки. Это позволяет гибко управлять процессом прошивки, конфигурирования и обновления устройства.

#### Основные environments:

- `[env:esp32doit-devkit-v1]` — **Штатный режим без лишних логов**. Используется для финальной эксплуатации, когда не требуется подробная отладка.
- `[env:debug]` — **Режим с подробным логированием** (рекомендуется для разработки и диагностики). Выводит максимум информации в Serial и MQTT.
- `[env:load_default_configs]` — **Первичная загрузка**. Используется только при первом запуске нового микроконтроллера или для сброса к заводским настройкам. Загружает дефолтные конфиги и включает расширенное логирование.
- `[env:debugOTA_320WiFi5]`, `[env:debugOTAVenom]` — **OTA-обновления через PlatformIO**. Примеры для разных сетей. Для своей сети скопируйте любой из этих блоков, поменяйте `upload_port` на IP вашей ESP32 в локальной сети, к которой она подключается и (если вы выставили нестандартный пароль) `upload_flags` с новым паролем.

Выбор environment происходит на панели управления platformio.ini в нижней части экрана.

#### Пример добавления своей среды для OTA:

```ini
[env:debugOTA_MyNetwork]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
upload_protocol = espota
upload_port = <IP_ESP32>
upload_flags = 
    --auth=<ваш_пароль>
; остальные параметры аналогичны debug
```

Для загрузки прошивки по ArduinoOTA через созданную среду необходимо выставить в настройках platformio.ini (на панели управления в нижней части экрана) в качестве порта локальный IP ESP32 в вашей сети, компьютер при этом тоже должен быть подключен к тому же WiFi. Если все было выставлено, убедитесь, что ESP32 подключен к данному WiFi и загрузите прошивку, нажав Upload на панели управления platformio.ini.

### Порядок действий при первичной загрузке

1. **Клонируйте репозиторий и установите PlatformIO.**
2. **Загрузка файлов конфигурации в SPIFFS**
   - После любой смены конфигов или при первом запуске обязательно загрузите все файлы из папки `data/` на ESP32:
     ```
     pio run --target uploadfs
     ```
   - Это создаст в SPIFFS все необходимые конфиги (`data.json`, `config.json`, `mqtt.json`, `passwords.json` и их дефолтные версии).
3. **Загрузка дефолтных конфигов**
   - Перед началом работы необходимо загрузить файлы из папки `data/` в энергонезависимую память SPIFFS. См. пункт 2.
   - Затем обязательно используйте `[env:load_default_configs]` для самой первой прошивки нового микроконтроллера.
   - Это обеспечит корректную инициализацию всех файлов настроек во Flash.
   - После этого рекомендуется сразу перейти на `[env:debug]` для дальнейшей работы. 
4. **Подключитесь к точке доступа ESP32 и настройте WiFi/MQTT через портал.**
5. **Проверьте работу системы через приложение IoP Farm.**


**Перепрошивка**
   - Для обычной перепрошивки используйте `[env:debug]` (рекомендуется) или `[env:esp32doit-devkit-v1]` (минимум логов).
   - Для OTA-обновлений используйте соответствующую OTA-среду или создайте свою.
   - Для других способов перепрошивки (по сети) см. раздел `OTA Обновления`.

---

## Архитектура и основные компоненты

### Датчики и исполнительные устройства

- **Датчики**: DHT22 (температура и влажность), DS18B20 (температура), HC-SR04 (уровень воды), FC-28 (влажность почвы), KY-018 (освещенность), YFS401 (расход воды).
- **Актуаторы**: насос R385, тепловая лампа, фитолента.
- **Схемы подключения**: см. заголовочный файл в `include/sensors/*.h`. Также см. `scheme.pdf`, в котором подробно нарисована понятная и подробная схема подключения.

### Менеджеры и их логика

- **WiFiManager** — автоматическое подключение к WiFi, поднятие точки доступа при ошибках, настройка через captive portal.
- **MQTTManager** — подключение к серверу, подписка на топики, обработка команд, автоматическое повторное подключение при разрыве соединения.
- **ConfigManager** — хранение всех настроек в SPIFFS, поддержка дефолтных и пользовательских конфигов.
- **OTAManager** — поддержка OTA-обновлений через PlatformIO.
- **WebServerManager** — web-интерфейс для OTA с аутентификацией.

---

## Изменяемые параметры в constants.h

Практически все базовые параметры системы вынесены в файл `include/config/constants.h`. Это позволяет гибко адаптировать прошивку под свои задачи без глубокого погружения в код. После изменения большинства параметров требуется пересборка и перепрошивка устройства.

### Примеры того, что можно менять:

- **Идентификатор устройства и MQTT**
  - `mqtt::DEFAULT_DEVICE_ID` — уникальный идентификатор вашей фермы для MQTT-топиков.
  - `mqtt::DEFAULT_HOST`, `mqtt::DEFAULT_PORT` — адрес и порт MQTT-брокера по умолчанию.
- **WiFi и точка доступа**
  - `wifi::DEFAULT_AP_NAME`, `wifi::DEFAULT_AP_PASSWORD` — имя и пароль точки доступа ESP32.
  - `wifi::DEFAULT_HOSTNAME` — имя устройства в сети.
  - Таймауты подключения, интервалы переподключения (`CONNECT_TIMEOUT`, `RECONNECT_RETRY_INTERVAL` и др.).
- **Пины подключения датчиков и актуаторов**
  - Все пины для датчиков (`sensors::pins::...`) и исполнительных устройств (`actuators::pins::...`).
  - Можно переназначить под свою разводку платы.
- **Калибровочные значения и параметры датчиков**
  - Например, `sensors::calibration::FC28_DRY_VALUE`, `YFS401_CALIBRATION_FACTOR`, `HCSR04_A`, `HCSR04_B` и др.
  - Позволяет точно откалибровать систему под вашу физическую установку.
- **Пороговые значения ошибок и "нет данных"**
  - `sensors::calibration::SENSOR_ERROR_VALUE`, `NO_DATA` — значения, которые система возвращает при ошибке или отсутствии данных.
- **Параметры стратегий управления**
  - В пространствах имен `strategies::irrigation`, `strategies::heating`, `strategies::lighting` можно изменить интервалы, гистерезисы, расписания и т.д.
- **Интервалы работы системы**
  - Например, `sensors::timing::DEFAULT_READ_INTERVAL` — период опроса датчиков.
  - `actuators::CHECK_INTERVAL` — частота проверки актуаторов.
- **Логгирование**
  - В пространстве имен `log::constants` можно изменить уровни логирования, цвета, буферы, минимальный уровень для отправки логов в MQTT и др.
- **Команды управления**
  - В enum `CommandCode` можно добавить или изменить коды команд для MQTT.
- **Параметры времени**
  - `time::DEFAULT_GMT_OFFSET`, `time::NTP_PERIOD` — часовой пояс и период синхронизации времени. По умолчанию выставлен GMT+3 (Москва).

### Как менять параметры

1. Откройте файл `include/config/constants.h` в редакторе.
2. Найдите нужный параметр (или используйте поиск по имени).
3. Измените значение на свое (например, смените пин, пароль, deviceId, калибровку).
4. Сохраните файл, пересоберите и перепрошейте проект.

**Важно:**
- После изменения параметров в constants.h всегда пересобирайте проект и загружайте новую прошивку на ESP32.
- Некоторые параметры (например, deviceId, пароли, пины) критичны для работы системы — меняйте их осознанно.
- Для изменения логина/пароля web-интерфейса используйте файл `passwords.json`.

---

## Сервер-посредник и хранение данных

Для повышения надежности, безопасности и удобства работы в системе IoP-Farm реализован собственный сервер-посредник. Он выполняет следующие функции:

- **Маршрутизация всех сообщений** между ESP32 (фермой) и мобильным приложением. Все сообщения MQTT-топиков (`config`, `data`, `command`, `logs`) проходят через сервер, что позволяет централизованно управлять обменом данными.
- **Хранение данных**: сервер сохраняет все показания датчиков, команды, логи и конфигурации в собственной базе данных. Это обеспечивает:
  - Мониторинг истории и анализ данных
  - Централизованный доступ к информации с разных устройств

### Службы сервера:
- data.service (services/data_server_farm/) 
    - Подписывается на топик /farm$id$/data
    - Записывает данные от MQTT-брокера в БД(data.db)
- logger.service (services/farm_logger/)
    - Подписывается на топик /farm$id$/log
    - Записывает данные от MQTT-брокера в syslog
    - Для просмотра логов:
- logs.service (/services/logs_to_phone/)
    - Пересылает на мобильное устройство отчёт по всем показаниям фермы
    - Отправляются все отчёты, удовлетворяющие показателям "unix_time_from" - "unix_time_to"
    - В случае ошибок, неправильного формата, отправляется последняя запись
    - Для просмотра логов:
- config.service (/services/control_phone_config)
    - Принимает подключение от мобильного устройства, получает конфиг параметров сенсоров
    - Публикует в топик /farm$id$/config
- command.service (services/command_services)
    - Принимает подключение от мобильного устройства, получает команду, к-ую срочно нужно обработать на ферме
    - Публикует в топик /farm$id$/command
    
Просмотр логов одной конкретной службы:
```sh
journalctl -u $name$.service
```

Просмотреть статус + логи/добавление в автозагрузку/удаление/старт/остановка конкретной службы:
```sh
systemctl status/enable/disable/start/stop $name$.service
```

Сделать для всех служб сразу:
```sh
./farmctl status/enable/disable/start/stop
```
ВАЖНО!
Если службы НЕ РАБОТАЮТ/ОСТАНОВЛЕНЫ можно запустить вручную бинарные файлы(ОПАСНО):
В папке необходимой службы запускаем компиляцию(sh файл) и исполняем:
```sh
sh data.sh/logger.sh/logs.sh/config.sh/command.sh

# Без прикрепления к commandline в фоновом режиме
nohup DATA/LOGGER/LOGS/CONFIG/COMMAND &
```
Либо же централизовано в папке services компиляция + исполнение:
```sh
sh compile_and_run.sh
```
Удаление фоновых процессов:
```sh
sh delete_trash.sh
```

---

## Управление с телефона и мониторинг через Android-приложение IoP Farm

Система IoP-Farm поддерживает полноценное управление и мониторинг с мобильного устройства благодаря специально разработанному Android-приложению. Это позволяет:

- **Просматривать показания всех датчиков** (температура, влажность, уровень воды, освещенность и др.) в реальном времени прямо на экране телефона.
- **Управлять исполнительными устройствами** (насос, лампа нагрева, фитолента) дистанционно — запускать/останавливать вручную или по расписанию.
- **Менять параметры конфигурации** (например, расписание полива, целевые значения температуры, объем воды и т.д.) без необходимости перепрошивки устройства.

### Как работает обмен данными

Вся коммуникация между ESP32 и приложением строится на протоколе MQTT с посредником в виде сервера. Для каждого устройства (фермы) используется уникальный идентификатор `farmId` (например, `farm001`).

- **Загрузка конфигурации**: приложение получает настройки из топика `{farmId}/config`.
- **Мониторинг данных**: все показания датчиков публикуются устройством в топик `{farmId}/data`.
- **Управление и команды**: приложение отправляет команды в топик `{farmId}/command`, которые тут же обрабатываются системой.
- **Логи и диагностика**: все события, ошибки и служебные сообщения отправляются в топик `{farmId}/logs`.

### Требования

- ESP32 и телефон должны быть подключены к сети. 

**Преимущества:**
- Не требуется физический доступ к устройству для настройки и управления.
- Все изменения применяются мгновенно, без перепрошивки.
- Удобный мониторинг и диагностика в одном месте.


---

## OTA-обновления

- **PlatformIO OTA**: используйте соответствующую среду, будьте в одной сети с ESP32, пароль по умолчанию `12345678`.
- **Web-интерфейс**: доступен по адресу ESP32, требуется аутентификация (логин/пароль из `passwords.json`. Для его изменения необходимо будет заново загрузить дефолтные настройки, см. `Загрузка файлов конфигурации в SPIFFS`).
- **WiFiManager**: при потере или отсутствии связи ESP32 поднимает точку доступа, там же можно обновить прошивку в `upload`. Пароль по умолчанию для WiFiManager `12345678`, можно изменить в файле `constants.h`. Точка доступа ESP32 будет высвечена в списке доступных WiFi-сетей на вашем устройстве.

---

## Масштабирование и расширение

- Добавляйте новые датчики/актуаторы через наследование от `ISensor`/`IActuator`.
- Новые стратегии — через наследование от `IActuatorStrategy` и регистрацию в менеджере.
- Для новых топиков MQTT — пропишите их в `constants.h` и настройте обработку в `MQTTManager`.
- Для поддержки нескольких ферм используйте уникальный `deviceId`, изменив его в `constants.h`.
- Логи выводятся в Serial и/или отправляются на MQTT. Тип логгирования можно изменить в самом начале `main.cpp`, создав нужный логгер при помощи фабрики. Вы можете добавить свой способ форматирования сообщений и свой транспорт для логгирования, наследуя их от соответствующих абстрактных классов.

---

## FAQ и советы

- **Нужно ли каждый раз подключаться к WiFi?** Нет, только при первом запуске или сбросе настроек. Для работы актуаторов требуется хотя бы однократная синхронизация времени через интернет.
- **Что делать, если настройки сбились?** Можно загрузить новые настройки из приложения (предпочтительно). Если с этим возникла проблема, загрузите дефолтные файлы через `pio run --target uploadfs` (см. `Загрузка файлов конфигурации в SPIFFS`) и повторите настройку.
- **Почему я не получаю данные от расходометра?** Расходомер и не отправляет данные на сервер, он используется только насосом для корректного полива. Вы можете отследить показания расходометра в логах во время полива.

---

## Важные замечания

- Для корректной работы актуаторов требуется хотя бы однократная синхронизация времени через WiFi.
- Все схемы подключения и калибровки описаны в заголовочных файлах датчиков и нарисованы в scheme.pdf.
- Дефолтные файлы конфигурации обязательны для первого запуска!
- Светодиод на плате сигнализирует о проблемах с WiFi/MQTT (горит — нет связи с WiFi или MQTT, либо происходит подключение к WiFi).
- Данные с датчиков: `-50` — нет данных, `-100` — ошибка чтения. Данные константы можно изменить в `constants.h`.
- Для корректной работы датчиков их необходимо откалибровать:
**FC-28**: калибруется по сухой и влажной почве (см. `FC28.h`).
**HC-SR04**: требует калибровки коэффициентов (использована МНК-калибровка, см. `HCSR04.h`).
**YFS401**: калибруется по количеству импульсов на литр (см. `YFS401.h`).

---

## Контакты и поддержка

Если у вас возникли вопросы или предложения по развитию проекта — создайте issue или pull request в репозитории.

---



