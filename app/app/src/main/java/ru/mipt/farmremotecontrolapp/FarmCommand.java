package ru.mipt.farmremotecontrolapp;

import org.json.JSONException;
import org.json.JSONObject;

public class FarmCommand {
    public static String TAG = FarmCommand.class.getName();

//    private class Command{
//        String command;
//        String runame;
//    }

    public static final String[] COMMAND_NAMES = {
            "ESP_RESET",
            "PUMP_ON",
            "PUMP_OFF",
            "GROWLIGHT_ON",
            "GROWLIGHT_OFF",
            "HEATLAMP_ON",
            "HEATLAMP_OFF",
            "FARM_ON",
            "FARM_OFF"
    };
    public static String[] COMMAND_NAMES_RU = {
            "Перезагрузка ESP32",
            "Включенить насос",
            "Выключить насос",
            "Включение светодиодной ленты",
            "Выключение светодиодной ленты",
            "Включить нагрев",
            "Выключить нагрев",
            "Включить ферму",
            "Выключить ферму"
    };
    public static int COMMAND_COUNT = COMMAND_NAMES.length;

    private int command;
    public JSONObject toJson(){
        try {
            JSONObject jsonObject = new JSONObject();
            jsonObject.put("command", command);
            return jsonObject;
        } catch (JSONException e) {
            return null;
        }
    }
    public FarmCommand(int number){
        if(number < 0){number = 0;}
        if(number >= COMMAND_COUNT){number = COMMAND_COUNT - 1;}
        this.command = number;
    }
}
