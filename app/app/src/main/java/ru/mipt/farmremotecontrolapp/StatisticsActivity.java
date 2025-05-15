package ru.mipt.farmremotecontrolapp;

import static java.util.Objects.isNull;

import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import org.json.JSONException;
import org.json.JSONObject;

import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;

import ru.mipt.ru.mipt.farmremotecontrolapp.R;

//import  com.oracle.common.base.Timeout;

// Vico imports
import com.patrykandpatryk.vico.core.entry.ChartEntryModelProducer;
import com.patrykandpatryk.vico.core.entry.FloatEntry;
import com.patrykandpatryk.vico.views.chart.ChartView;
import com.patrykandpatryk.vico.core.axis.AxisPosition;
import com.patrykandpatryk.vico.core.axis.formatter.AxisValueFormatter;
import com.patrykandpatryk.vico.core.axis.horizontal.HorizontalAxis;
import com.patrykandpatryk.vico.core.axis.vertical.VerticalAxis;
import com.patrykandpatryk.vico.core.chart.line.LineChart;
import com.patrykandpatryk.vico.core.component.shape.LineComponent;
import com.patrykandpatryk.vico.core.component.text.TextComponent;
import com.patrykandpatryk.vico.core.dimensions.MutableDimensions;
import com.patrykandpatryk.vico.core.extension.ColorExtensions; // May need specific color handling
import com.patrykandpatryk.vico.views.theme.आओHorizontalAxis; // Using default theme helpers
import com.patrykandpatryk.vico.views.theme.आओVerticalAxis;
import com.patrykandpatryk.vico.views.theme.आओLineChart;

public class StatisticsActivity extends AppCompatActivity {
    final static String TAG = "StatisticsActivity";
    Statistics statistics = null;
    ChartView chartView;
    TableLayout table;
    ScheduledExecutorService scheduler;

    boolean showTable = false;

    int lastGraph = 0;

    // Formatter for X-axis (time)
    private AxisValueFormatter<AxisPosition.Horizontal.Bottom> timeAxisFormatter;
    // Formatter for Y-axis (values)
    private AxisValueFormatter<AxisPosition.Vertical.Start> valueAxisFormatter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_statistics);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        chartView = findViewById(R.id.chart_view);
        modelProducer = new ChartEntryModelProducer();

        // Initialize Axis Formatters
        timeAxisFormatter = (value, chartValues, textStyle) -> {
            if (statistics != null && statistics.getTime() != null && statistics.getTime().length > (int)value && value >=0) {
                SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss", Locale.getDefault());
                return sdf.format(new Date(statistics.getTime()[(int)value] * 1000));
            }
            return String.valueOf((int)value);
        };

        valueAxisFormatter = (value, chartValues, textStyle) -> String.format(Locale.US, "%.1f", value);

        setupChart();

        Intent intent = getIntent();
        String jsonString = intent.getStringExtra("json");
        try {
            JSONObject json = new JSONObject(jsonString);
            ControlMessage controlMessage = ControlMessage.getStatisticsMessage(json);
            controlMessage.setOnReceiveListener(new ControlMessage.OnReceiveListener() {
                @Override
                void receive(ControlMessage controlMessage) {
                    StatisticsActivity.this.statistics = controlMessage.getStats();
                    if (statistics != null && statistics.getTime() != null && statistics.getData(lastGraph) != null) {
                        drawGraphic(statistics.getTime(), statistics.getData(lastGraph), Statistics.STATISTICS_NAMES[lastGraph]);
                    }
                }
            });

            scheduler = Executors.newScheduledThreadPool(1);
            ScheduledFuture<?> handle = scheduler.scheduleWithFixedDelay(controlMessage, 0, 10, java.util.concurrent.TimeUnit.SECONDS);

        } catch (JSONException e) {
            Log.d(TAG, e.getMessage());
        }

        LinearLayout linearLayout = findViewById(R.id.statistics_linear_layout);

        linearLayout.removeAllViews();

        for(int i = 0; i < Statistics.STATISTICS_COUNT; i++){
            Button button = new Button(StatisticsActivity.this);
            button.setText(Statistics.STATISTICS_NAMES[i]);
            int finalI = i;
            button.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if(!isNull(statistics) && statistics.getTime() != null && statistics.getData(finalI) != null) {
                        drawGraphic(statistics.getTime(), statistics.getData(finalI), Statistics.STATISTICS_NAMES[finalI]);
                    }
                    StatisticsActivity.this.lastGraph = finalI;
                }
            });
            linearLayout.addView(button);
        }

        if (chartView.getParent() != null) {
            ((ViewGroup)chartView.getParent()).removeView(chartView);
        }
        linearLayout.addView(chartView);

        Button buttonToggleTable = new Button(StatisticsActivity.this);
        buttonToggleTable.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                ((Button)v).setText(StatisticsActivity.this.showTable ? "Show full table" : "Hide full table");
                if(!isNull(statistics)  && statistics.getTime() != null && statistics.getData(lastGraph) != null) {
                    drawGraphic(statistics.getTime(), statistics.getData(lastGraph), Statistics.STATISTICS_NAMES[lastGraph]);
                }
                StatisticsActivity.this.showTable = !StatisticsActivity.this.showTable;
                table.setVisibility(StatisticsActivity.this.showTable ? View.VISIBLE : View.GONE);
            }
        });
        buttonToggleTable.setText("Show full table");

        linearLayout.addView(buttonToggleTable);

        ScrollView scrollView = new ScrollView(this);
        scrollView.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        linearLayout.addView(scrollView);
        table = new TableLayout(this);
        table.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        scrollView.addView(table);
        table.setVisibility(View.GONE);
    }

    private void setupChart() {
        LineChart lineChart = आओLineChart();

        chartView.setChart(lineChart);
        chartView.setModelProducer(modelProducer);

        chartView.setStartAxis(आओVerticalAxis(builder -> {
            builder.setValueFormatter(valueAxisFormatter);
            return Unit.INSTANCE;
        }));
        chartView.setBottomAxis(आओHorizontalAxis(builder -> {
            builder.setValueFormatter(timeAxisFormatter);
            return Unit.INSTANCE;
        }));
    }

    private void drawGraphic(long[] timestamps, float[] data, String name) {
        if (timestamps == null || data == null) {
            Log.d(TAG, "Timestamps or data is null for chart: " + name);
            modelProducer.setEntries(new ArrayList<FloatEntry>());
            return;
        }
        if (timestamps.length != data.length) {
            Log.d(TAG, "Timestamps and data length mismatch for chart: " + name);
            modelProducer.setEntries(new ArrayList<FloatEntry>());
            return;
        }
        if (timestamps.length == 0) {
            Log.d(TAG, "Empty data for chart: " + name);
            modelProducer.setEntries(new ArrayList<FloatEntry>());
            return;
        }

        ArrayList<FloatEntry> entries = new ArrayList<>();
        for (int i = 0; i < data.length; i++) {
            entries.add(new FloatEntry((float) i, data[i]));
        }
        modelProducer.setEntries(entries);

        if (showTable) {
            table.removeAllViews();

            TableRow headerRow = new TableRow(this);
            TextView headerTime = new TextView(this);
            headerTime.setText("Time");
            headerTime.setPadding(8, 8, 8, 8);
            TextView headerValue = new TextView(this);
            headerValue.setText(name);
            headerValue.setPadding(8, 8, 8, 8);
            headerRow.addView(headerTime);
            headerRow.addView(headerValue);
            table.addView(headerRow);

            SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault());
            for (int i = 0; i < timestamps.length; i++) {
                TableRow dataRow = new TableRow(this);
                TextView timeCell = new TextView(this);
                timeCell.setText(sdf.format(new Date(timestamps[i] * 1000)));
                timeCell.setPadding(8, 8, 8, 8);

                TextView valueCell = new TextView(this);
                valueCell.setText(String.format(Locale.US, "%.2f", data[i]));
                valueCell.setPadding(8, 8, 8, 8);

                dataRow.addView(timeCell);
                dataRow.addView(valueCell);
                table.addView(dataRow);
            }
        }
        table.setVisibility(showTable ? View.VISIBLE : View.GONE);
    }

    @Override
    protected void onDestroy() {
        if (scheduler != null && !scheduler.isShutdown()) {
            scheduler.shutdown();
        }
        super.onDestroy();
    }
}