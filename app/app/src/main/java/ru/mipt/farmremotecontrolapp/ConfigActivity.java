package ru.mipt.farmremotecontrolapp;

import static android.os.Environment.isExternalStorageManager;
import static java.util.Objects.isNull;
import static ru.mipt.farmremotecontrolapp.Utils.pathFromIntent;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.navigation.ui.AppBarConfiguration;

import org.json.JSONException;
import org.json.JSONObject;

import java.nio.file.Path;

import ru.mipt.ru.mipt.farmremotecontrolapp.R;

public class ConfigActivity extends AppCompatActivity {

    private AppBarConfiguration appBarConfiguration;
//    private ActivityConfigBinding binding;

    public static final int OPEN_CONFIG_REQUEST_CODE = 1;
    public static final int ASK_FILES_PERMISSION_REQUEST_CODE = 1;
    private static final String TAG = "ConfigActivity";

    /*

    TextView[] tw = new TextView[FarmConfig.PROPERTIES_NUMBER];
    SeekBar[] sb = new SeekBar[FarmConfig.PROPERTIES_NUMBER];
    EditText[] et = new EditText[FarmConfig.PROPERTIES_NUMBER];
    LinearLayout[] linearLayout = new LinearLayout[FarmConfig.PROPERTIES_NUMBER];
    FarmConfig farmConfig = FarmConfig.fromFile(null);

     */
    LayoutGenerator layoutGenerator = null;

    //@RequiresApi(api = Build.VERSION_CODES.O)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_config);

        LayoutGenerator.GeneratorConfiguration generatorConfiguration = new LayoutGenerator.GeneratorConfiguration(new LayoutGenerator.GeneratorConfigurationEntry[]{
                LayoutGenerator.numberRunnerEntryFabric("pump_interval_days", "pump_interval_days", 0.5f, 6.5f, 0.5f),
                LayoutGenerator.timeEntryFabric("pump_start", "pump_start"),
                LayoutGenerator.numberRunnerEntryFabric("pump_volume_ml", "pump_volume_ml", 0, 10000, 10),
                LayoutGenerator.numberRunnerEntryFabric("heatlamp_target_temp", "heatlamp_target_temp", 20, 40, 0.1f),
                LayoutGenerator.timeEntryFabric("growlight_on", "growlight_on"),
                LayoutGenerator.timeEntryFabric("growlight_off", "growlight_off"),
        });

        LinearLayout linearLayout = findViewById(R.id.linear_vertical_container_config);
        if(isNull(linearLayout)){Log.d(TAG, "LinearLayout is null");}
        layoutGenerator = new LayoutGenerator(ConfigActivity.this, linearLayout, generatorConfiguration);

        if(Utils.existsInternalStorage("farm001.json", ConfigActivity.this)){
            try {
                layoutGenerator.setJSONObject(new JSONObject(Utils.readInternalStorage("farm001.json", ConfigActivity.this)));
            } catch (JSONException e) {
                Log.d(TAG, e.getMessage());
            }
        }

        /*
        FarmConfig temporaryFarmConfig = farmConfig;

        Intent intent = getIntent();
        byte[] configBytes = intent.getByteArrayExtra("config");
        if(isNull(configBytes)) {
            Path path = pathFromIntent(intent);
            if (hasFilePermission()) {
                temporaryFarmConfig = FarmConfig.fromFile(path);
            }
        } else {
            //temporaryFarmConfig = FarmConfig.fromByteArray(configBytes);
        }
        LinearLayout linear_vertical_container = findViewById(R.id.linear_vertical_container);

        //create FarmConfig.PROPERTIES_NUMBER rows with text, seekbar and input for number
        for(int n = 0; n < FarmConfig.PROPERTIES_NUMBER; n ++){
            linearLayout[n] = new LinearLayout(this);
            tw[n] = new TextView(this);
            sb[n] = new SeekBar(this);
            et[n] = new EditText(this);

            setViewWeight(linearLayout[n], 1);
            linearLayout[n].getLayoutParams().height = LinearLayout.LayoutParams.WRAP_CONTENT;

            tw[n].setText(FarmConfig.PROPERTIES_NAMES[n]);
            setViewWeight(tw[n], 2);


            setViewWeight(sb[n], 2);
            sb[n].setMin(FarmConfig.PROPERTIES_MIN[n]);
            sb[n].setMax(FarmConfig.PROPERTIES_MAX[n]);
//            sb[n].setProgress(farmConfig.getProperty(n));
            sb[n].getLayoutParams().height = LinearLayout.LayoutParams.MATCH_PARENT;
            sb[n].getLayoutParams().width = LinearLayout.LayoutParams.MATCH_PARENT;
            sb[n].setEnabled(true);
            sb[n].setVisibility(View.VISIBLE);

            et[n].setInputType(InputType.TYPE_CLASS_NUMBER);
            setViewWeight(et[n], 2);
//            et[n].setText("" + farmConfig.getProperty(n));
//            sb[n].setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
//                EditText editText = et[finalN];
//                FarmConfig editable_config = config;
//                int editable_n = finalN;
//                @Override
//                public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
//                    editText.setText(String.valueOf(i));
//                    editable_config.setProperty(editable_n, i);
//                }
//
//                @Override
//                public void onStartTrackingTouch(SeekBar seekBar) {
//                }
//
//                @Override
//                public void onStopTrackingTouch(SeekBar seekBar) {
//                }
//            });
//
//            et[n].setOnEditorActionListener(new TextView.OnEditorActionListener() {
//                SeekBar seekBar = sb[finalN];
//                FarmConfig editable_config = config;
//                int editable_n = finalN;
//                @Override
//                public boolean onEditorAction(TextView textView, int i, KeyEvent keyEvent) {
//                    seekBar.setProgress((int)Integer.valueOf(textView.getText().toString()));
//                    editable_config.setProperty(editable_n, (int)Integer.valueOf(textView.getText().toString()));
//                    return false;
//                }
//            });
            linearLayout[n].addView(tw[n]);
            linearLayout[n].addView(sb[n]);
            linearLayout[n].addView(et[n]);
            linear_vertical_container.addView(linearLayout[n]);
        }
        loadConfig(temporaryFarmConfig);
*/

        Button buttonOpenFile = findViewById(R.id.button_open_file);
        //Button buttonUploadConfig = findViewById(R.id.button_upload_config);
        buttonOpenFile.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent()
                        .setType("*/*")
                        .setAction(Intent.ACTION_GET_CONTENT);
                startActivityForResult(Intent.createChooser(intent, "Select a file"), OPEN_CONFIG_REQUEST_CODE);
            }
        });

        Button buttonUploadConfig = findViewById(R.id.button_upload_config);
        buttonUploadConfig.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //ControlMessage controlMessage = ControlMessage.setConfigMessage(farmConfig.toJSONObject());
                ControlMessage controlMessage = ControlMessage.setConfigMessage(layoutGenerator.getJSONObject());
                        Thread thread = new Thread(controlMessage);
                thread.start();

                Utils.writeInternalStorage("farm001.json", layoutGenerator.getJSONObject().toString(), ConfigActivity.this);

            }
        });

    }
/*
    private void loadConfig(FarmConfig config){
//        new Thread(() -> {
            if (isNull(config)) {
                Log.d(TAG, "Tried to load null config");
                return;
            }
            this.farmConfig = config;
            for (int n = 0; n < FarmConfig.PROPERTIES_NUMBER; n++) {
                sb[n].setProgress(config.getProperty(n));
                et[n].setText("" + config.getProperty(n));

                int finalN = n;
                sb[n].setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                    EditText editText = et[finalN];
                    FarmConfig editable_config = config;
                    int editable_n = finalN;

                    @Override
                    public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                        editText.setText(String.valueOf(i));
                        editable_config.setProperty(editable_n, i);
                    }

                    @Override
                    public void onStartTrackingTouch(SeekBar seekBar) {
                    }

                    @Override
                    public void onStopTrackingTouch(SeekBar seekBar) {
                    }
                });

                et[n].setOnEditorActionListener(new TextView.OnEditorActionListener() {
                    SeekBar seekBar = sb[finalN];
                    FarmConfig editable_config = config;
                    int editable_n = finalN;

                    @Override
                    public boolean onEditorAction(TextView textView, int i, KeyEvent keyEvent) {
                        seekBar.setProgress((int) Integer.valueOf(textView.getText().toString()));
                        editable_config.setProperty(editable_n, (int) Integer.valueOf(textView.getText().toString()));
                        return false;
                    }
                });
            }
//        }).run();
    }*/

    @RequiresApi(api = Build.VERSION_CODES.R)
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode == OPEN_CONFIG_REQUEST_CODE && resultCode == RESULT_OK) {
            Path path = pathFromIntent(data);
            if(!hasFilePermission()){return;}
            //FarmConfig config = FarmConfig.fromFile(path);
            //loadConfig(config);
            layoutGenerator.setJSONObject(Utils.jsonFromFile(path));
        }
        if(requestCode == ASK_FILES_PERMISSION_REQUEST_CODE){
            Log.d(TAG, "Asked for file permission");
        }
    }


    private boolean hasFilePermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            Log.d(TAG, "File access permission not granted");
            // Разрешение не предоставлено; запрашиваем его.
//            if (Build.VERSION.SDK_INT >= 35) {
//                Log.d(TAG, "shouldShowRequestPermissionRationale returned "+shouldShowRequestPermissionRationale(new String(Manifest.permission.MANAGE_EXTERNAL_STORAGE), 0));
//            }

            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    ASK_FILES_PERMISSION_REQUEST_CODE);
            return false;
//            Uri uri = Uri.parse("package:"+BuildConfig.APPLICATION_ID);
//            startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, uri));
//        }
//        return (ContextCompat.checkSelfPermission(this, Manifest.permission.MANAGE_EXTERNAL_STORAGE)
//                == PackageManager.PERMISSION_GRANTED);
        } else {
            return true;
        }
    }

}