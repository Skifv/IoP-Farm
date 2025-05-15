package ru.mipt.farmremotecontrolapp;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import org.json.JSONObject;

import ru.mipt.ru.mipt.farmremotecontrolapp.R;

public class CommandsActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_commands);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        LinearLayout linearLayout = findViewById(R.id.commands_layout);

        for(int i = 0; i < FarmCommand.COMMAND_COUNT; i ++){
            Button button = new Button(CommandsActivity.this);
            button.setText(FarmCommand.COMMAND_NAMES_RU[i]);
            int finalI = i;
            button.setOnClickListener(new View.OnClickListener() {
                int num = finalI;
                @Override
                public void onClick(View v) {
                    FarmCommand farmCommand = new FarmCommand(num);
                    JSONObject jsonObject = farmCommand.toJson();
                    ControlMessage controlMessage = ControlMessage.sendCommandMessage(jsonObject);
                    Thread thread = new Thread(controlMessage);
                    thread.start();
                }
            });
            linearLayout.addView(button);
        }

    }
}