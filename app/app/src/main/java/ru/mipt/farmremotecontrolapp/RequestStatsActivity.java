package ru.mipt.farmremotecontrolapp;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CalendarView;
import android.widget.LinearLayout;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import org.json.JSONObject;

import java.time.LocalDateTime;
import java.util.Date;

import ru.mipt.ru.mipt.farmremotecontrolapp.R;

public class RequestStatsActivity extends AppCompatActivity {
    public static final String TAG = "RequestStatsActivity";

    LayoutGenerator layoutGenerator;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_request_stats);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });


        LinearLayout linearLayout = findViewById(R.id.request_stats_layout);

        LayoutGenerator.GeneratorConfigurationEntry[] generatorConfigurationEntries = {
                //LayoutGenerator.dateTimeEntryFabric("test_field", "Тестовое поле", LocalDateTime.now().toLocalTime().toSecondOfDay(), LocalDateTime.now().toLocalDate().toEpochDay()),
                LayoutGenerator.titleEntryFabric("Введите период для отправки данных"),

                LayoutGenerator.dateTimeEntryFabric("unix_time_from", "Начиная с:", LocalDateTime.now().toLocalTime().toSecondOfDay(), LocalDateTime.now().toLocalDate().minusDays(1).toEpochDay()),
                LayoutGenerator.dateTimeEntryFabric("unix_time_to", "До:", LocalDateTime.now().toLocalTime().plusHours(1).toSecondOfDay(), LocalDateTime.now().toLocalDate().toEpochDay()),
                //LayoutGenerator.dateEntryFabric("unix_time_from", ""),
                //LayoutGenerator.dateEntryFabric("unix_time_to", ""),
        };

        layoutGenerator = new LayoutGenerator(RequestStatsActivity.this, linearLayout , new LayoutGenerator.GeneratorConfiguration(generatorConfigurationEntries));

        Button submitTime = findViewById(R.id.button_submit_time);
        submitTime.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                        Intent intent = new Intent(RequestStatsActivity.this, StatisticsActivity.class);
                        intent.putExtra("json", layoutGenerator.getJSONObject().toString());
                        RequestStatsActivity.this.startActivity(intent);
                    }
        });
    }

}