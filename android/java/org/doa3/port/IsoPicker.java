package org.doa3.port;
import android.app.ActivityManager;
import android.content.*;
import java.io.IOException;
import java.util.List;

final class IsoPicker {
    static Intent intent(){return new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*")
        .putExtra(Intent.EXTRA_MIME_TYPES,new String[]{"application/x-iso9660-image","application/x-iso-image","application/x-cd-image","application/octet-stream"})
        .addCategory(Intent.CATEGORY_OPENABLE).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);}
    static void persist(Context context,Intent result)throws IOException{
        try{context.getContentResolver().takePersistableUriPermission(result.getData(),Intent.FLAG_GRANT_READ_URI_PERMISSION);}
        catch(SecurityException unavailable){throw new IOException("Нет постоянного доступа к ISO. Выберите образ во внутренней памяти или на SD-карте.",unavailable);}
    }
    static void requireGameStopped(Context context)throws IOException{
        ActivityManager manager=(ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningAppProcessInfo> processes=manager.getRunningAppProcesses();
        if(processes!=null)for(ActivityManager.RunningAppProcessInfo process:processes)if((context.getPackageName()+":game").equals(process.processName))throw new IOException("Сначала выйдите из игры через Select");
    }
}
