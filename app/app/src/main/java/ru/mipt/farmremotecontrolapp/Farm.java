package ru.mipt.farmremotecontrolapp;

import static java.util.Objects.isNull;

import android.content.Context;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class Farm {
    static String TAG = Farm.class.getName();
    private String ip;
    private int farmId;
    public Farm(String ip, int farmId){
        this.ip = ip;
        this.farmId = farmId;
    }
    //public String toString(){}

    static Farm current = null;

    static Farm farmFromJSON(JSONObject jsonObject){
        try{
            return new Farm(jsonObject.getString("ip"), jsonObject.getInt("id"));
        } catch (JSONException e) {
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    static JSONObject toJSON(Farm farm){
        try {
            JSONObject jsonObject = new JSONObject();
            jsonObject.put("ip", farm.ip);
            jsonObject.put("id", farm.farmId);
            return jsonObject;
        } catch (Exception e) {
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    static Farm[] fromJSON(JSONObject jsonObject){
        try{
            JSONArray jsonArray = jsonObject.getJSONArray("farms");
            if(isNull(jsonArray)){return null;}
            int len = jsonArray.length();
            Farm[] farms = new Farm[len];
            for(int i = 0; i < len; i++ ){
                farms[i] = farmFromJSON(jsonArray.getJSONObject(len));
            }
            return farms;
        } catch (JSONException e) {
            Log.d(TAG, e.getMessage());
        }
        return new Farm[0];
    }

    static void appendToJSON(Farm farm, JSONObject jsonObject){
        JSONArray jsonArray = jsonObject.optJSONArray("farms");
        if(isNull(jsonArray)) return;
        jsonArray.put(toJSON(farm));
    }

    static void saveFarms(JSONObject jsonObject, Context context){
        Utils.writeInternalStorage("farms.json", jsonObject.toString(), context);
    }

    static JSONObject loadFarms(Context context){
        try {
            return new JSONObject(Utils.readInternalStorage("farms.json", context));
        } catch (JSONException e) {
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    public int getFarmId() {
        return farmId;
    }

    public String getIp() {
        return ip;
    }
}
