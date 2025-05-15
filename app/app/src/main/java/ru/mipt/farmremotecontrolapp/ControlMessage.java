package ru.mipt.farmremotecontrolapp;

import static java.util.Objects.isNull;

import android.util.Log;

import org.json.JSONObject;

import java.io.*;
import java.net.*;


public class ControlMessage implements Runnable {
    public  static String TAG = ControlMessage.class.getName();
    enum flags {
        NO_FLAG,
        SET_CONFIG,
        GET_CONFIG,
        GET_STATS,
        CHECK_FARM,
        SEND_COMMAND,
    };

    //public static final int PORT = 5003;

    public static abstract class OnReceiveListener {
        abstract void receive(ControlMessage controlMessage);
    }

    private flags flag = flags.NO_FLAG;
    private boolean readyStatus = false;
    private JSONObject sendMessage = null;
    //private FarmConfig farmConfig = null;
    private ControlMessage.OnReceiveListener onReceiveListener = null;
    //private Thread thread = null;
    private Statistics stats = null;

    public static ControlMessage getStatisticsMessage(JSONObject timePeriod){
        ControlMessage controlMessage = new ControlMessage();
        controlMessage.flag = flags.GET_STATS;
        controlMessage.sendMessage = timePeriod;
        return controlMessage;
    }

    public static ControlMessage setConfigMessage(JSONObject configMessage){
        ControlMessage controlMessage = new ControlMessage();
        controlMessage.flag = flags.SET_CONFIG;
        controlMessage.sendMessage = configMessage;
        return controlMessage;
    }

    public static ControlMessage sendCommandMessage(JSONObject command){
        ControlMessage controlMessage = new ControlMessage();
        controlMessage.flag = flags.SEND_COMMAND;
        controlMessage.sendMessage = command;
        return controlMessage;
    }

    private ControlMessage(){
    }

    private int getPort(){
        switch (flag){
            case GET_STATS:
                return 1488;
            case SET_CONFIG:
                return 1489;
            case SEND_COMMAND:
                return 1490;
            default:
                return 0;
        }
    };

    public boolean isReady(){
        return readyStatus;
    }

    public void setOnReceiveListener(ControlMessage.OnReceiveListener listener){
        if(isNull(onReceiveListener)){
            this.onReceiveListener = listener;
        }
    }
/*
    private byte[] receive(DataInputStream dIn) throws IOException{
        byte[] bytes = new byte[4];
        if(dIn.read(bytes, 0,4)!=4){return null;}
        ByteBuffer b = ByteBuffer.wrap(bytes);
        int length = b.getInt();
        bytes = new byte[length];
        if(dIn.read(bytes, 0, length)!=length){return null;}
        return bytes;
    }

    private void send(DataOutputStream dOut, byte[] bytearray) throws IOException {
        dOut.writeInt(bytearray.length);
        dOut.write(bytearray);
    }*/

    private void sendJSON(DataOutputStream dOut, JSONObject json){
        try {
            dOut.write(json.toString().getBytes());
            //dOut.writeBytes(json.toString());
        } catch (Exception e){
            Log.d(TAG, e.getMessage());
        }
    }

    public Statistics getStats(){
        return this.stats;
    }

    @Override
    public void run() {
        try {
            stats = null;
            Socket socket = new Socket("103.137.250.154", getPort());
            // Setup output stream to send data to the server
            DataOutputStream dOut = new DataOutputStream(socket.getOutputStream());
            // Setup input stream to receive data from the server
            DataInputStream dIn = new DataInputStream(socket.getInputStream());
            //dOut.write(flag);

            switch (flag){
                case SET_CONFIG:
                    Log.d(TAG, sendMessage.toString());
                    sendJSON(dOut, sendMessage);
                    dOut.writeBytes("\n");
                    break;
                case GET_STATS:
                    sendJSON(dOut, sendMessage);
                    dOut.writeBytes("\n");
                    this.stats = Statistics.fromStream(dIn);
                    break;
                case SEND_COMMAND:
                    sendJSON(dOut, sendMessage);
                    dOut.writeBytes("\n");
            }

            // Close the socket
            socket.close();

            if(!isNull(onReceiveListener)) onReceiveListener.receive(this);

        } catch (IOException ioException){
            Log.d(TAG, ioException.getMessage());
        }
    }
}
