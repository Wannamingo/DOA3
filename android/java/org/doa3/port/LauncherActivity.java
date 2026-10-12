package org.doa3.port;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.content.SharedPreferences;
import android.widget.*;
import java.io.*;
import java.util.*;
import org.json.JSONObject;

public class LauncherActivity extends Activity {
    private TextView status;
    private Button play,importButton;
    private CheckBox swapButtons;
    private Spinner touchControls,resolution,displayAspect,language;
    private SharedPreferences preferences;
    private boolean english;
    private volatile boolean importing;
    File disc(){return new File(getFilesDir(),"disc");}
    private String text(String ru,String en){return english ? en : ru;}
    static String savedAspect(SharedPreferences preferences){
        String value=preferences.getString("displayAspect",null);
        if("4:3".equals(value)||"16:9".equals(value)||"source".equals(value))return value;
        return preferences.getBoolean("aspectCorrected",false)&&preferences.getBoolean("display43",false) ? "4:3" : "source";
    }
    private void addChoice(LinearLayout layout,String title,Spinner choice){
        LinearLayout row=new LinearLayout(this);row.setOrientation(LinearLayout.HORIZONTAL);row.setGravity(android.view.Gravity.CENTER_VERTICAL);
        TextView label=new TextView(this);label.setText(title);label.setTextSize(16);row.addView(label);
        row.addView(choice,new LinearLayout.LayoutParams(0,LinearLayout.LayoutParams.WRAP_CONTENT,1));layout.addView(row);
    }
    private boolean saveOptions(){
        String aspect=new String[]{"source","4:3","16:9"}[displayAspect.getSelectedItemPosition()];
        return preferences.edit().putString("displayAspect",aspect).putString("language",language.getSelectedItemPosition()==1 ? "en" : "ru")
            .putInt("resolution",resolution.getSelectedItemPosition()).putBoolean("swapButtons",swapButtons.isChecked())
            .putInt("touchControls",touchControls.getSelectedItemPosition()).commit();
    }
    @Override public void onCreate(Bundle saved){
        super.onCreate(saved);
        preferences=getSharedPreferences("controls",MODE_PRIVATE);
        english="en".equals(preferences.getString("language","ru"));
        LinearLayout layout=new LinearLayout(this);layout.setOrientation(LinearLayout.VERTICAL);int pad=(int)(20*getResources().getDisplayMetrics().density);layout.setPadding(pad,pad,pad,pad);
        TextView title=new TextView(this);title.setText("Dead or Alive 3");title.setTextSize(26);layout.addView(title);
        status=new TextView(this);status.setPadding(0,pad,0,pad);layout.addView(status);
        displayAspect=new Spinner(this);displayAspect.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{text("Исходные пропорции","Original proportions"),"4:3","16:9"}));
        String aspect=savedAspect(preferences);displayAspect.setSelection("4:3".equals(aspect)?1:"16:9".equals(aspect)?2:0);addChoice(layout,text("Формат","Aspect ratio"),displayAspect);
        language=new Spinner(this);language.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"Русский","English"}));language.setSelection(english?1:0);addChoice(layout,text("Язык приложения","App language"),language);
        swapButtons=new CheckBox(this);swapButtons.setText(text("Поменять A/B и X/Y на геймпаде","Swap A/B and X/Y on the gamepad"));swapButtons.setChecked(preferences.getBoolean("swapButtons",false));layout.addView(swapButtons);
        TextView touchLabel=new TextView(this);touchLabel.setText(text("Экранные кнопки","Touch controls"));layout.addView(touchLabel);
        touchControls=new Spinner(this);touchControls.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{text("Авто — скрывать при подключённом геймпаде","Auto — hide when a gamepad is connected"),text("Показывать","Show"),text("Скрывать","Hide")}));touchControls.setSelection(preferences.getInt("touchControls",0));layout.addView(touchControls);
        TextView resolutionLabel=new TextView(this);resolutionLabel.setText(text("Внутреннее разрешение","Internal resolution"));layout.addView(resolutionLabel);
        resolution=new Spinner(this);resolution.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"1× — 720 × 480",text("2× — 1440 × 960","2× — 1440 × 960"),"3× — 2160 × 1440"}));resolution.setSelection(preferences.getInt("resolution",0));layout.addView(resolution);
        Button mappings=new Button(this);mappings.setText(text("Назначение кнопок","Button assignments"));layout.addView(mappings);
        mappings.setOnClickListener(v->{if(saveOptions())startActivity(new Intent(this,ControlMappingActivity.class));});
        play=new Button(this);play.setText(text("Играть","Play"));layout.addView(play);play.setOnClickListener(v->{if(saveOptions())startActivity(new Intent(this,GameActivity.class));else status.setText(text("Не удалось сохранить настройки.","Could not save settings."));});
        importButton=new Button(this);importButton.setText(text("Выбрать ISO · Читать напрямую","Select ISO · Read directly"));layout.addView(importButton);
        importButton.setOnClickListener(v->{Intent intent=IsoPicker.intent();startActivityForResult(intent,1);});
        TextView help=new TextView(this);help.setText(text("Формат и язык применяются при запуске игры.\nL1 — Black, R1 — White, L2/R2 — триггеры Xbox.\nЕсли буквы кнопок не совпадают, включите перестановку A/B и X/Y.\nСохранения хранятся в приложении.\nВыберите ISO DOA3 USA. Образ читается напрямую и должен оставаться на устройстве.","Aspect ratio and language apply when the game starts.\nL1 — Black, R1 — White, L2/R2 — Xbox triggers.\nIf the button labels differ, enable A/B and X/Y swapping.\nSaves are stored in the application.\nSelect your DOA3 USA ISO. It is read directly and must remain available."));layout.addView(help);ScrollView scroll=new ScrollView(this);scroll.addView(layout);setContentView(scroll);refresh();play.requestFocus();
        language.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){
            public void onNothingSelected(AdapterView<?> parent){}
            public void onItemSelected(AdapterView<?> parent,android.view.View view,int position,long id){
                String selected=position==1 ? "en" : "ru";
                if(!selected.equals(preferences.getString("language","ru"))&&saveOptions())recreate();
            }
        });
    }
    @Override protected void onResume(){super.onResume();if(status!=null&&!importing)refresh();}
    private void refresh(){
        boolean ready=false;
        try(IsoSource.Opened iso=IsoSource.open(this)){ready=true;}catch(Exception ignored){}
        play.setEnabled(ready&&!importing);
        status.setText(ready ? text("ISO подключён. Чтение напрямую.","ISO connected. Reading directly.")
            : text("Выберите ISO DOA3 USA. Сохранения хранятся отдельно.","Select a DOA3 USA ISO. Saves are stored separately."));
    }
    @Override protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);if(request!=1||result!=RESULT_OK||data==null||importing)return;
        Uri uri=data.getData();if(uri==null)return;
        try{IsoPicker.persist(this,data);}catch(Exception error){status.setText(error.getMessage());return;}
        importing=true;play.setEnabled(false);importButton.setEnabled(false);language.setEnabled(false);
        status.setText(text("Проверка ISO без копирования…","Checking ISO without copying…"));
        new Thread(()->{String resultText;
            try{IsoPicker.requireGameStopped(this);IsoSource.attach(this,uri,(count,total)->runOnUiThread(()->status.setText(text("Проверено ","Verified ")+(count/1048576)+" / "+(total/1048576)+" MiB")));resultText=text("ISO подключён · Чтение напрямую","ISO connected · Direct reading");}
            catch(Exception error){resultText=text("Не удалось подключить ISO: ","Could not connect ISO: ")+error.getMessage();}
            final String message=resultText;runOnUiThread(()->{importing=false;importButton.setEnabled(true);language.setEnabled(true);refresh();status.setText(message);});
        },"doa3-iso-verify").start();
    }
}
