package ru.mipt.farmremotecontrolapp;

import static java.util.Objects.isNull;

import android.app.Dialog;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import org.json.JSONObject;

import ru.mipt.ru.mipt.farmremotecontrolapp.R;

public class FarmSelectionActivity extends AppCompatActivity {

    static String TAG = FarmSelectionActivity.class.getName();

    JSONObject farms;
    LinearLayout linearLayout;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_farm_selection);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        farms = Farm.loadFarms(FarmSelectionActivity.this);




        Button newFarmButton = findViewById(R.id.button_new_farm);
        newFarmButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Dialog dialog = new Dialog(FarmSelectionActivity.this);
                dialog.setContentView(R.layout.dialog_new_farm);

                EditText IPInput = dialog.findViewById(R.id.ip_input_text);
                EditText IDInput = dialog.findViewById(R.id.id_input_number);

                Button cancelButton = dialog.findViewById(R.id.cancel_button);
                Button applyButton = dialog.findViewById(R.id.apply_button);

                dialog.show();

                applyButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        Farm farm = new Farm(IPInput.getText().toString(), Integer.parseInt(IDInput.getText().toString()));
                        Farm.appendToJSON(farm, farms);
                        dialog.hide();
                    }
                });
            }
        });

        linearLayout = findViewById(R.id.farm_selection_linear_layout);
        addFarms(Farm.fromJSON(farms));

    }

    void addFarms(Farm[] fs){
        if(isNull(fs) || fs.length == 0){
            Log.d(TAG, "No farms");
            return;
        }
        linearLayout.removeAllViews();
        for(Farm farm: fs){
            LinearLayout linearLayout1 = new LinearLayout(FarmSelectionActivity.this);
            linearLayout1.setOrientation(LinearLayout.HORIZONTAL);

            ImageView imageView = new ImageView(FarmSelectionActivity.this);
            imageView.setImageResource(R.mipmap.ic_launcher);
            linearLayout1.addView(imageView);

            LinearLayout linearLayout2 = new LinearLayout(FarmSelectionActivity.this);

            TextView textView = new TextView(FarmSelectionActivity.this);
            textView.setText(farm.getIp());
            linearLayout2.addView(textView);

            TextView textView1 =  new TextView(FarmSelectionActivity.this);
            textView.setText(new Integer(farm.getFarmId()).toString());
            linearLayout2.addView(textView1);

            Button button = new Button(FarmSelectionActivity.this);
            button.setText("Proceed to the farm");
            button.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Farm.current = farm;
                    Intent intent = new Intent(FarmSelectionActivity.this, MainActivity.class);
                    FarmSelectionActivity.this.startActivity(intent);
                }
            });
            linearLayout2.addView(button);

        }
    }
}