cd command_phone
sh command.sh
cd ..

cd control_phone_config
sh config.sh
cd ..

cd data_server_farm
sh data.sh
cd ..

cd logs_to_phone
sh logs.sh
cd ..

cd farm_logger
sh logger.sh
cd ..

nohup ./command_phone/COMMAND &
nohup ./control_phone_config/CONFIG &
nohup ./data_server_farm/DATA &
nohup ./logs_to_phone/LOGS &
nohup ./farm_logger/LOGGER &
