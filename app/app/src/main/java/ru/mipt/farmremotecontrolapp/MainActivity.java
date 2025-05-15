package ru.mipt.farmremotecontrolapp;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import ru.mipt.ru.mipt.farmremotecontrolapp.R;

public class MainActivity extends AppCompatActivity {
    final static String TAG = "MainActivity";

    @SuppressLint("ClickableViewAccessibility")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });
        Button button = findViewById(R.id.button6);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.d(TAG,"Sending intent...");
                //Intent intent = new Intent(MainActivity.this, StatisticsActivity.class);
                Intent intent = new Intent(MainActivity.this, RequestStatsActivity.class);
                MainActivity.this.startActivity(intent);
                Log.d(TAG,"Intent sent");
            }
        });
        Button button_set_config = findViewById(R.id.set_config_button);
        button_set_config.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, ConfigActivity.class);
                MainActivity.this.startActivity(intent);
            }
        });

        Button button_commands = findViewById(R.id.button_commands);
        button_commands.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, CommandsActivity.class);
                MainActivity.this.startActivity(intent);
            }
        });
/*
        LinearLayout linearLayout = findViewById(R.id.dfgfdhg);
        linearLayout.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                Log.d(TAG, "MotionEvent");
                if(event.getAction() == MotionEvent.ACTION_POINTER_DOWN){
                    v.setBackgroundColor(Color.BLACK);
                } else if (event.getAction() == MotionEvent.ACTION_MOVE){
                    v.setBackgroundColor(Color.RED);
                } else {
                    v.setBackgroundColor(Color.WHITE);
                }
                return true;
            }
        });*/
    }
}