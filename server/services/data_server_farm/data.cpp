#include <iostream>
#include <sqlite3.h>
#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

const string MQTT_BROKER = "tcp://localhost:1883";
const string MQTT_TOPIC = "/farm001/data";
const string DB_FILE = "/home/tovarichkek/services/data_server_farm/data.db";

class MQTTListener : public virtual mqtt::callback {
    sqlite3* db;
    
    void create_table() {
        const char* sql = 
            "CREATE TABLE IF NOT EXISTS sensor_data ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "timestamp_unix INTEGER,"
            "temperature_DHT22 REAL,"
            "temperature_DS18B20 REAL,"
            "humidity REAL,"
            "water_level REAL,"
            "soil_moisture REAL,"
            "light_intensity REAL);";
            
        if (sqlite3_exec(db, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
            throw runtime_error(sqlite3_errmsg(db));
        }
    }

public:
    MQTTListener() {
        if (sqlite3_open(DB_FILE.c_str(), &db) != SQLITE_OK) {
            throw runtime_error(sqlite3_errmsg(db));
        }
        create_table();
    }

    ~MQTTListener() {
        sqlite3_close(db);
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        try {
            auto j = json::parse(msg->get_payload());
            
            // Получаем текущее время в Unix time
            auto now = chrono::system_clock::now();
            auto timestamp = chrono::duration_cast<chrono::seconds>(
                now.time_since_epoch()).count();
            
            sqlite3_stmt* stmt;
            const char* sql = "INSERT INTO sensor_data (timestamp_unix, "
                "temperature_DHT22, temperature_DS18B20, humidity, "
                "water_level, soil_moisture, light_intensity) "
                "VALUES (?, ?, ?, ?, ?, ?, ?);";
            
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
                cerr << "Prepare error: " << sqlite3_errmsg(db) << endl;
                return;
            }
            
            // Привязываем параметры
            sqlite3_bind_int64(stmt, 1, timestamp);
            sqlite3_bind_double(stmt, 2, j["temperature_DHT22"].get<double>());
            sqlite3_bind_double(stmt, 3, j["temperature_DS18B20"].get<double>());
            sqlite3_bind_double(stmt, 4, j["humidity"].get<double>());
            sqlite3_bind_double(stmt, 5, j["water_level"].get<double>());
            sqlite3_bind_double(stmt, 6, j["soil_moisture"].get<double>());
            sqlite3_bind_double(stmt, 7, j["light_intensity"].get<double>());
            
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                cerr << "Insert error: " << sqlite3_errmsg(db) << endl;
            }
            
            sqlite3_finalize(stmt);
        }
        catch (const exception& e) {
            cerr << "Error processing message: " << e.what() << endl;
        }
    }
};

int main() {
    try {
        mqtt::async_client client(MQTT_BROKER, "mqtt2sql");
        MQTTListener listener;
        
        client.set_callback(listener);
        client.connect()->wait();
        client.subscribe(MQTT_TOPIC, 1);
        
        cout << "Service started. Press Enter to exit..." << endl;
        while(true){
		sleep(1);
	}

        client.unsubscribe(MQTT_TOPIC)->wait();
        client.disconnect()->wait();
    }
    catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
