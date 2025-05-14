#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <thread>
#include <boost/asio.hpp>
#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>

namespace asio = boost::asio;
using boost::asio::ip::tcp;
using json = nlohmann::json;

// Конфигурация (изменённые значения)
const std::string MQTT_BROKER = "tcp://localhost:1883";
const std::string MQTT_TOPIC = "/farm001/command";  // Изменён топик
const int TCP_PORT = 1490;                         // Изменён порт
const std::string LOG_FILE = "/var/log/phone_command.log";

class Logger {
    std::ofstream log_file;
    
public:
    Logger() {
        log_file.open(LOG_FILE, std::ios::app);
        if(!log_file.is_open()) {
            throw std::runtime_error("Cannot open log file");
        }
    }

    void log(const std::string& ip, const std::string& config) {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::tm timeinfo;
        localtime_r(&in_time_t, &timeinfo);
        
        char time_str[20];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        log_file << time_str << " | IP: " << ip 
                << " | Config: " << config << std::endl;
    }
};

class MqttSender {
    mqtt::async_client client;
    
public:
    MqttSender() : client(MQTT_BROKER, "command_phone_gateway") {
        client.connect()->wait();
    }

    void send_message(const std::string& payload) {
        auto msg = mqtt::make_message(MQTT_TOPIC, payload);  // Отправка в новый топик
        msg->set_qos(1);
        client.publish(msg)->wait();
    }
};

void handle_client(tcp::socket socket, MqttSender& sender, Logger& logger) {
    try {
        std::string client_ip = socket.remote_endpoint().address().to_string();
        
        asio::streambuf buf;
        asio::read_until(socket, buf, '\n');
        
        std::istream is(&buf);
        std::string data;
        std::getline(is, data);
        
        json parsed_config = json::parse(data);
        
        logger.log(client_ip, data);
        sender.send_message(data);
        
        std::cout << "Processed request from: " << client_ip << std::endl;
    }
    catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        std::ofstream tmp(LOG_FILE, std::ios::app);
        tmp.close();

        asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), TCP_PORT));  // Порт 1490
        MqttSender sender;
        Logger logger;

        std::cout << "Phone Command Service started on port " << TCP_PORT << std::endl;

        while(true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            
            std::thread([s = std::move(socket), &sender, &logger]() mutable {
                handle_client(std::move(s), sender, logger);
            }).detach();
        }
    }
    catch(const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
