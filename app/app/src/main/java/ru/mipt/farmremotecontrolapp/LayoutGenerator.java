package ru.mipt.farmremotecontrolapp;

import android.app.DatePickerDialog;
import android.app.TimePickerDialog;
import android.content.Context;
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.DatePicker;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.TimePicker;

import org.json.JSONException;
import org.json.JSONObject;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.LocalTime;
import java.time.ZoneOffset;
import java.time.format.DateTimeFormatter;
import java.util.Locale;
import java.util.Date;

public class LayoutGenerator {

    public static class GeneratorConfiguration {
       LayoutGenerator.GeneratorConfigurationEntry[] entries;
        public GeneratorConfiguration(GeneratorConfigurationEntry[] generatorConfigurationEntries){
            this.entries = generatorConfigurationEntries;
        }
    }

    public static class GeneratorConfigurationEntry{
        public FIELD_TYPES fieldType;
        public String fieldName;
        public String fieldNameRu;
        public float min = Float.MIN_VALUE;
        public float max = Float.MAX_VALUE;
        //public String defaultString = null;
        //public int defaultInt;
        public GeneratorConfigurationEntry[] children = null;
        public float multiplier = 1.0f;
        public long secondOfDay = 0;
        public long epochDay = 0;
        //public GeneratorConfiguration child = null;
    }

    enum FIELD_TYPES {
        //NUMBER,
        NUMBER_RUNNER,
        TIME,
        TITLE,
        DATE,
        ENCLOSING,
        DATE_TIME,
        //TEXT,
        //BUTTON
    }

    enum JSON_FORMATS {
        EPOCH_DATE,
        UNIX_TIME_OF_DATE,
        UNIX_TIME,
        STRING_TIME,
    }
    /*
    static class InputFieldBehaviour {
        enum behaviour {

        }
        boolean[] behaviours = new boolean[behaviour.values().length];

        InputFieldBehaviour(behaviour[] bs){
            for(behaviour b: bs){
                behaviours[(int)b] = true;
            }
        }
    }*/

    public static final String TAG = "LayoutGenerator";


    public abstract class InputField {
        GeneratorConfigurationEntry entry;
        String fieldName;
        LayoutGenerator layoutGenerator;
        Context context;

        public InputField(LayoutGenerator layoutGenerator, GeneratorConfigurationEntry entry){
            this.entry = entry;
            this.context = layoutGenerator.context;
            this.fieldName = entry.fieldName;
            this.layoutGenerator = layoutGenerator;
        }

        public  abstract void checkValidity();

        public abstract void putValueToJSON(JSONObject jsonObject);
        //jsonObject.put(fieldName, ... );

        public abstract void appendView(LinearLayout linearLayout);
        //linearLayout.addView(view);

        public abstract void setJSON(JSONObject jsonObject);

    }

    public class NumberRunnerField extends InputField{
        float multiplier;
        int number;
        TextView tw;
        SeekBar sb;
        EditText et;

        public NumberRunnerField(LayoutGenerator layoutGenerator, GeneratorConfigurationEntry entry) {
            super(layoutGenerator, entry);
            multiplier = entry.multiplier;
        }

        public void checkValidity(){
            if(number < 0){this.number = 0;}
            if(number > (entry.max - entry.min)/multiplier){this.number = (int)((entry.max - entry.min)/multiplier);}
        }



        @Override
        public void putValueToJSON(JSONObject jsonObject) {
            //TODO
            try {
                jsonObject.put(fieldName, ((float)number)*multiplier + entry.min);
            } catch (JSONException e) {
                Log.d(TAG, e.getMessage());
            }
        }

        @Override
        public void appendView(LinearLayout linearLayout){
            LinearLayout horisontalLinearLayout = new LinearLayout(context);
            horisontalLinearLayout.setOrientation(LinearLayout.HORIZONTAL);
            horisontalLinearLayout.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

            tw = new TextView(context);
            sb = new SeekBar(context);
            et = new EditText(context);

            et.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_NORMAL | InputType.TYPE_NUMBER_FLAG_SIGNED);

            sb.setMax(Math.round((entry.max - entry.min)/multiplier));

            tw.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT, 1.0f));
            sb.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT, 1.0f));
            et.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT, 1.0f));

            tw.setText(fieldName);
            sb.setProgress(number);
            et.setText("" + (number*multiplier + entry.min));

            sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                EditText editText = et;
                LayoutGenerator.NumberRunnerField numberInputField = NumberRunnerField.this;

                @Override
                public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                    editText.setText(String.valueOf(((float)i)*multiplier + entry.min));
                    numberInputField.number = i;
                }
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {
                }
                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                }
            });

            et.setOnEditorActionListener(new TextView.OnEditorActionListener() {
                SeekBar seekBar = sb;
                LayoutGenerator.NumberRunnerField numberInputField = NumberRunnerField.this;

                @Override
                public boolean onEditorAction(TextView textView, int i, KeyEvent keyEvent) {
                    float temp = (float) Float.valueOf(textView.getText().toString());
                    temp -= entry.min;
                    numberInputField.number = Math.round(temp / multiplier);
                    numberInputField.checkValidity();
                    seekBar.setProgress(number);
                    et.setText(String.valueOf(((float)numberInputField.number)*multiplier + entry.min));
                    return false;
                }
            });

            horisontalLinearLayout.addView(tw);
            horisontalLinearLayout.addView(sb);
            horisontalLinearLayout.addView(et);

            linearLayout.addView(horisontalLinearLayout);

            this.checkValidity();
        }

        @Override
        public void setJSON(JSONObject jsonObject) {
            try {
                this.number = (int)((jsonObject.getDouble(fieldName) - entry.min)/multiplier);
                this.sb.setProgress(number);
                this.et.setText(String.valueOf(number*multiplier + entry.min));
                this.checkValidity();
            } catch (JSONException e) {
                Log.d(TAG, e.getMessage());
            }
        }
    }

    public class TimeField extends InputField{
        TimePicker timePicker;//TODO:remove
        TimePickerDialog timePickerDialog;
        int hours, minutes;
        TextView tw;
        Button bt;

        public TimeField(LayoutGenerator layoutGenerator, GeneratorConfigurationEntry entry) {
            super(layoutGenerator, entry);
        }


        @Override
        public void checkValidity() {

        }

        @Override
        public void putValueToJSON(JSONObject jsonObject) {
            try {
                jsonObject.put(fieldName, getTime());
            } catch (JSONException e) {
                Log.d(TAG, e.getMessage());
            }
        }

        private String getTime(){
            return (hours >= 10 ? "" : "0") + hours + ":" + (minutes >= 10 ? "" : "0") + minutes;
        }

        @Override
        public void appendView(LinearLayout linearLayout) {//TODO

            DateTimeFormatter dateTimeFormatter = DateTimeFormatter.ofPattern("HH:mm");

            tw = new TextView(context);
            bt = new Button(context);

            TimePickerDialog.OnTimeSetListener onTimeSetListener = new TimePickerDialog.OnTimeSetListener() {
                @Override
                public void onTimeSet(TimePicker view, int hourOfDay, int minute) {
                    TimeField.this.hours = hourOfDay;
                    TimeField.this.minutes = minute;
                    bt.setText(getTime());
                }
            };

            timePickerDialog = new TimePickerDialog(context, onTimeSetListener, hours, minutes, true);

            //LinearLayout horisontalLinearLayout = new LinearLayout(context);
            //horisontalLinearLayout.setOrientation(LinearLayout.HORIZONTAL);
            //horisontalLinearLayout.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));



            tw.setText(fieldName);

            bt.setText(getTime());

            bt.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    timePickerDialog.show();
                }
            });
            //horisontalLinearLayout.addView(tw);
            //horisontalLinearLayout.addView(bt);

            linearLayout.addView(tw);
            linearLayout.addView(bt);

            //linearLayout.addView(horisontalLinearLayout);
            /*
            timePicker = new TimePicker(context);

            timePicker.setOnTimeChangedListener(new TimePicker.OnTimeChangedListener() {
                @Override
                public void onTimeChanged(TimePicker view, int hourOfDay, int minute) {
                    TimeField.this.secondOfDay = (hourOfDay >= 10 ? "" : "0") + hourOfDay + ":" + (minute >= 10 ? "" : "0") + minute;
                }
            });
            linearLayout.addView(timePicker);*/
        }

        @Override
        public void setJSON(JSONObject jsonObject) {
            String time = getTime();
            try {
                time = jsonObject.getString(fieldName);
            } catch (JSONException e) {
                Log.d(TAG, e.getMessage());
            }
            DateTimeFormatter dateTimeFormatter = DateTimeFormatter.ofPattern("HH:mm");
            try {
                String[] split = time.split(":");
                hours = Integer.valueOf(split[0]);
                minutes = Integer.valueOf(split[1]);
                bt.setText(getTime());
            } catch (Exception e) {
                Log.d(TAG, e.getMessage());
            }
        }
    }

    public class TitleField extends InputField{

        public TitleField(LayoutGenerator layoutGenerator, GeneratorConfigurationEntry entry) {
            super(layoutGenerator, entry);
        }

        @Override
        public void checkValidity() {
        }

        @Override
        public void putValueToJSON(JSONObject jsonObject) {
        }

        @Override
        public void appendView(LinearLayout linearLayout) {
            TextView textView = new TextView(context);
            textView.setText(entry.fieldNameRu);
            linearLayout.addView(textView);
        }

        @Override
        public void setJSON(JSONObject jsonObject) {
        }
    }

    public class DateField extends InputField{
        DatePickerDialog datePickerDialog;
        long epochDay;

        public DateField(LayoutGenerator layoutGenerator, GeneratorConfigurationEntry entry) {
            super(layoutGenerator, entry);
        }


        @Override
        public void checkValidity() {
            if(epochDay < 0){epochDay = LocalDate.now().toEpochDay();}
        }

        @Override
        public void putValueToJSON(JSONObject jsonObject) {
            try {
                jsonObject.put(fieldName, epochDay);
            } catch (JSONException e) {
                Log.d(TAG, e.getMessage());
            }
        }

        @Override
        public void appendView(LinearLayout linearLayout) {
            datePickerDialog = new DatePickerDialog(context);
            datePickerDialog.setOnDateSetListener(new DatePickerDialog.OnDateSetListener() {
                @Override
                public void onDateSet(DatePicker view, int year, int month, int dayOfMonth) {
                    LocalDate localDate = LocalDate.of(year, month+1, dayOfMonth);
                    LocalDateTime localDateTime = LocalDateTime.of(localDate, LocalTime.now());
                    DateField.this.epochDay = localDateTime.toEpochSecond(ZoneOffset.of("+03:00"));
                }
            });
            Button dateButton = new Button(context);
            dateButton.setText(this.entry.fieldNameRu);
            dateButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    datePickerDialog.show();
                }
            });
            linearLayout.addView(dateButton);

        }

        @Override
        public void setJSON(JSONObject jsonObject) {
        }
    }

    public class DateTimeField extends InputField {

        DatePickerDialog datePickerDialog;
        TimePickerDialog timePickerDialog;
        long epochDay, secondOfDay;

        public DateTimeField(LayoutGenerator layoutGenerator, GeneratorConfigurationEntry entry) {
            super(layoutGenerator, entry);
            secondOfDay = entry.secondOfDay;
            epochDay = entry.epochDay;
        }

        @Override
        public void checkValidity() {

        }

        @Override
        public void putValueToJSON(JSONObject jsonObject) {
            try {
                jsonObject.put(fieldName, LocalDateTime.of(LocalDate.ofEpochDay(epochDay), LocalTime.ofSecondOfDay(secondOfDay)).toEpochSecond(ZoneOffset.of("+03:00")));
            } catch (JSONException e) {
            }
        }

        @Override
        public void appendView(LinearLayout linearLayout) {
            DateTimeFormatter hmFormat = DateTimeFormatter.ofPattern("HH:mm");
            DateTimeFormatter ymdFormat = DateTimeFormatter.ofPattern("d MMM uuuu");

            Button dateButton = new Button(context);
            datePickerDialog = new DatePickerDialog(context);
            datePickerDialog.setOnDateSetListener(new DatePickerDialog.OnDateSetListener() {
                @Override
                public void onDateSet(DatePicker view, int year, int month, int dayOfMonth) {
                    LocalDate localDate = LocalDate.of(year, month+1, dayOfMonth);
                    DateTimeField.this.epochDay = localDate.toEpochDay();
                    dateButton.setText(LocalDate.of(year, month, dayOfMonth).format(ymdFormat));
                }
            });
            LocalDateTime localDateTime = LocalDateTime.of(LocalDate.ofEpochDay(epochDay), LocalTime.ofSecondOfDay(secondOfDay));
            datePickerDialog.updateDate(localDateTime.getYear(), localDateTime.getMonthValue() - 1, localDateTime.getDayOfMonth());
            dateButton.setText(LocalDate.of(localDateTime.getYear(), localDateTime.getMonthValue(), localDateTime.getDayOfMonth()).format(ymdFormat));

            dateButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    datePickerDialog.show();
                }
            });

            Button timeButton = new Button(context);

            TimePickerDialog.OnTimeSetListener onTimeSetListener = new TimePickerDialog.OnTimeSetListener() {
                @Override
                public void onTimeSet(TimePicker view, int hourOfDay, int minute) {
                    DateTimeField.this.secondOfDay = LocalTime.of(hourOfDay, minute).toSecondOfDay();
                    timeButton.setText(LocalTime.of(hourOfDay, minute).format(hmFormat));
                }
            };
            timePickerDialog = new TimePickerDialog(context, onTimeSetListener, 0, 0, true);
            timePickerDialog.updateTime(localDateTime.getHour(), localDateTime.getMinute());
            timeButton.setText(LocalTime.of(localDateTime.getHour(), localDateTime.getMinute()).format(hmFormat));

            LinearLayout horisontalLinearLayout = new LinearLayout(context);
            horisontalLinearLayout.setOrientation(LinearLayout.HORIZONTAL);
            horisontalLinearLayout.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

            timeButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    timePickerDialog.show();
                }
            });


            TextView tw = new TextView(context);
            tw.setText(entry.fieldNameRu);
            timeButton.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f));
            dateButton.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f));
            linearLayout.addView(tw);
            horisontalLinearLayout.addView(dateButton);
            horisontalLinearLayout.addView(timeButton);

            linearLayout.addView(horisontalLinearLayout);
        }

        @Override
        public void setJSON(JSONObject jsonObject) {//TODO
        }
    }

    public class EnclosingField extends InputField{
        InputField[] children;

        public EnclosingField(LayoutGenerator layoutGenerator, GeneratorConfigurationEntry entry) {
            super(layoutGenerator, entry);
            this.children = toInputFields(entry.children, linearLayout, layoutGenerator);
        }

        @Override
        public void checkValidity() {
            for (int i = 0; i < children.length; i++){
                children[i].checkValidity();
            }
        }

        @Override
        public void putValueToJSON(JSONObject jsonObject) {
            try {
                JSONObject jsonObject1 = new JSONObject();
                for (int i = 0; i < children.length; i++) {
                    children[i].putValueToJSON(jsonObject1);
                }
                jsonObject.put(fieldName, jsonObject1);
            }catch (JSONException e){
                Log.d(TAG, e.getMessage());
            }
        }

        @Override
        public void appendView(LinearLayout linearLayout) {
            for(int i = 0; i < this.children.length; i++){
                this.children[i].appendView(linearLayout);
            }
        }

        @Override
        public void setJSON(JSONObject jsonObject) {
            try {
                JSONObject jsonObject1 = jsonObject.getJSONObject(fieldName);
                for (int i = 0; i < children.length; i++) {
                    children[i].putValueToJSON(jsonObject1);
                }
            }catch (JSONException e){
                Log.d(TAG, e.getMessage());
            }
        }
    }


    static GeneratorConfigurationEntry numberRunnerEntryFabric(String fieldName, String fieldNameRu, float min, float max, float multiplier){//TODO
        GeneratorConfigurationEntry entry = new GeneratorConfigurationEntry();
        entry.fieldName = fieldName;
        entry.fieldNameRu = fieldNameRu;
        entry.fieldType = FIELD_TYPES.NUMBER_RUNNER;
        entry.min = (min);
        entry.max = (max);
        entry.multiplier = multiplier;
        return entry;
    }

    static GeneratorConfigurationEntry timeEntryFabric(String fieldName, String fieldNameRu){//TODO
        GeneratorConfigurationEntry entry = new GeneratorConfigurationEntry();
        entry.fieldName = fieldName;
        entry.fieldNameRu = fieldNameRu;
        entry.fieldType = FIELD_TYPES.TIME;
        return entry;
    }

    static GeneratorConfigurationEntry titleEntryFabric(String fieldNameRu){
        GeneratorConfigurationEntry entry = new GeneratorConfigurationEntry();
        entry.fieldNameRu = fieldNameRu;
        entry.fieldType = FIELD_TYPES.TITLE;
        return entry;
    }

    static GeneratorConfigurationEntry dateEntryFabric(String fieldName, String fieldNameRu){
        GeneratorConfigurationEntry entry = new GeneratorConfigurationEntry();
        entry.fieldName = fieldName;
        entry.fieldNameRu = fieldNameRu;
        entry.fieldType = FIELD_TYPES.DATE;
        return entry;
    }

    static GeneratorConfigurationEntry enclosingEntryFabric(String fieldName, GeneratorConfigurationEntry[] children){
        GeneratorConfigurationEntry entry = new GeneratorConfigurationEntry();
        entry.fieldName = fieldName;
        entry.fieldType = FIELD_TYPES.ENCLOSING;
        entry.children = children;
        return entry;
    }

    static GeneratorConfigurationEntry dateTimeEntryFabric(String fieldName, String fieldNameRu, long defaultTime, long defaultDate){
        GeneratorConfigurationEntry entry = new GeneratorConfigurationEntry();
        entry.fieldName = fieldName;
        entry.fieldNameRu = fieldNameRu;
        entry.fieldType = FIELD_TYPES.DATE_TIME;
        entry.secondOfDay = defaultTime;
        entry.epochDay = defaultDate;
        return entry;
    }
    //public class NumberField extends InputField{} //unused


    //LayoutGenerator.GeneratorConfiguration generatorConfiguration = null;
    private LinearLayout linearLayout;
    private Context context;
    InputField[] inputFields;



    public LayoutGenerator(Context context, LinearLayout linearLayout, LayoutGenerator.GeneratorConfiguration configuration){
        this.context = context;
        this.linearLayout = linearLayout;
        this.inputFields = toInputFields(configuration.entries, linearLayout, this);
    }
/*
    public LayoutGenerator(Context context, LinearLayout linearLayout, LayoutGenerator.GeneratorConfigurationEntry[] generatorConfigurationEntries){
        this.context = context;
        this.linearLayout = linearLayout;
        this.inputFields = new InputField[generatorConfigurationEntries.length];
        for(int i = 0; i < inputFields.length; i ++){
            //inputFields[i] =generatorConfigurationEntries[i].toField();
            switch (generatorConfigurationEntries[i].fieldType){
                case NUMBER_RUNNER:
                    inputFields[i] = new NumberRunnerField(LayoutGenerator.this, generatorConfigurationEntries[i]);
                    break;
                case TIME:
                    inputFields[i] = new TimeField(LayoutGenerator.this, generatorConfigurationEntries[i]);
                    break;
                case TITLE:
                    inputFields[i] = new TitleField(LayoutGenerator.this, generatorConfigurationEntries[i]);
                    break;
                case DATE:
                    inputFields[i] = new DateField(LayoutGenerator.this, generatorConfigurationEntries[i]);
                    break;
                case ENCLOSING:
                    inputFields[i] = new EnclosingField(LayoutGenerator.this, generatorConfigurationEntries[i]);
                default:
                    break;
            }
        }
        for(int i = 0; i < inputFields.length; i ++){
            inputFields[i].appendView(linearLayout);
        }
        //TODO
    }*/

    public InputField[] toInputFields(LayoutGenerator.GeneratorConfigurationEntry[] entries, LinearLayout linearLayout, LayoutGenerator layoutGenerator){
        InputField[] inputFields = new InputField[entries.length];
        for(int i = 0; i < inputFields.length; i ++){
            //inputFields[i] =generatorConfigurationEntries[i].toField();
            switch (entries[i].fieldType){
                case NUMBER_RUNNER:
                    inputFields[i] = new NumberRunnerField(layoutGenerator, entries[i]);
                    break;
                case TIME:
                    inputFields[i] = new TimeField(layoutGenerator, entries[i]);
                    break;
                case TITLE:
                    inputFields[i] = new TitleField(layoutGenerator, entries[i]);
                    break;
                case DATE:
                    inputFields[i] = new DateField(layoutGenerator, entries[i]);
                    break;
                case ENCLOSING:
                    inputFields[i] = new EnclosingField(layoutGenerator, entries[i]);
                    break;
                case DATE_TIME:
                    inputFields[i] = new DateTimeField(layoutGenerator, entries[i]);
                    break;
                default:
                    break;
            }
        }
        for(int i = 0; i < inputFields.length; i ++){
            inputFields[i].appendView(linearLayout);
        }
        return inputFields;
    }

    public JSONObject getJSONObject(){
        JSONObject jsonObject = new JSONObject();
        for(int i = 0; i < inputFields.length; i ++){
            inputFields[i].putValueToJSON(jsonObject);
        }
        Log.d(TAG, jsonObject.toString());
        return jsonObject;
    }

    public void setJSONObject(JSONObject jsonObject){
        for(int i = 0; i < inputFields.length; i ++){
            inputFields[i].setJSON(jsonObject);
        }
    }

}