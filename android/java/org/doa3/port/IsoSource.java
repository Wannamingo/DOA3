package org.doa3.port;

import android.content.Context;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.util.AtomicFile;
import java.io.*;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.*;
import org.json.*;

/** A persisted URI and verified ISO extents, never an extracted game directory. */
final class IsoSource {
    interface Progress {void bytes(long count,long total);}
    static final class Snapshot {
        final Uri uri;final long length;final String[] names;final long[] offsets,sizes;
        Snapshot(Uri uri,long length,String[] names,long[] offsets,long[] sizes){this.uri=uri;this.length=length;this.names=names;this.offsets=offsets;this.sizes=sizes;}
    }
    static final class Opened implements AutoCloseable {
        final ParcelFileDescriptor descriptor;final Snapshot snapshot;
        Opened(ParcelFileDescriptor descriptor,Snapshot snapshot){this.descriptor=descriptor;this.snapshot=snapshot;}
        public void close()throws IOException{descriptor.close();}
    }
    private static AtomicFile store(Context context){return new AtomicFile(new File(context.getFilesDir(),"iso-source.json"));}
    private static byte[] readAll(InputStream input,int limit)throws IOException{ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] b=new byte[4096];int n;while((n=input.read(b))!=-1){if(out.size()+n>limit)throw new IOException("Слишком большое описание ISO");out.write(b,0,n);}return out.toByteArray();}
    private static JSONObject identity(Context context)throws Exception{try(InputStream in=context.getAssets().open("disc-identity.json")){return new JSONObject(new String(readAll(in,65536),StandardCharsets.UTF_8));}}
    static boolean selected(Context context){return store(context).getBaseFile().isFile();}
    private static Map<String,XboxIso.Entry> entries(FileChannel channel,JSONObject expected)throws Exception{
        ByteBuffer magic=ByteBuffer.allocate(4);channel.read(magic,0);
        if(magic.position()>=2&&magic.array()[0]=='P'&&magic.array()[1]=='K')throw new IOException("ZIP не поддерживается. Выберите ISO DOA3 USA.");
        Map<String,XboxIso.Entry> actual=new XboxIso(channel).index(),result=new TreeMap<>();
        for(Map.Entry<String,XboxIso.Entry> e:actual.entrySet()){
            String name=e.getKey().toLowerCase(Locale.ROOT);
            if(!expected.has(name)||name.contains("/")||result.put(name,e.getValue())!=null||e.getValue().size!=expected.getJSONObject(name).getLong("size"))throw new IOException("ISO не соответствует DOA3 USA этой сборки");
        }
        if(result.size()!=expected.length()||result.size()!=24)throw new IOException("В ISO не хватает файлов DOA3 USA");
        return result;
    }
    private static String digest(XboxIso iso,XboxIso.Entry entry,byte[] buffer,Progress progress,long[] count,long total)throws Exception{
        MessageDigest hash=MessageDigest.getInstance("SHA-256");long size=0,last=count[0];
        try(InputStream in=iso.open(entry)){int n;while((n=in.read(buffer))!=-1){if(n==0)throw new IOException("Не удалось прочитать ISO");hash.update(buffer,0,n);size+=n;count[0]+=n;if(progress!=null&&count[0]-last>=32L*1024*1024){progress.bytes(count[0],total);last=count[0];}}}
        if(size!=entry.size)throw new IOException("ISO обрезан");
        StringBuilder hex=new StringBuilder();for(byte b:hash.digest())hex.append(String.format(Locale.ROOT,"%02x",b&255));return hex.toString();
    }
    static Snapshot attach(Context context,Uri uri,Progress progress)throws Exception{
        if(!"content".equals(uri.getScheme()))throw new IOException("Выберите ISO через системный выбор файлов");
        JSONObject expected=identity(context);Snapshot snapshot;
        ParcelFileDescriptor descriptor=context.getContentResolver().openFileDescriptor(uri,"r");if(descriptor==null)throw new IOException("Нет доступа к ISO");
        try(ParcelFileDescriptor.AutoCloseInputStream in=new ParcelFileDescriptor.AutoCloseInputStream(descriptor)){
            FileChannel channel=in.getChannel();Map<String,XboxIso.Entry> files=entries(channel,expected);XboxIso iso=new XboxIso(channel);
            long total=0;for(XboxIso.Entry e:files.values())total+=e.size;long[] count={0};byte[] buffer=new byte[4*1024*1024];
            List<String> physical=new ArrayList<>(files.keySet());physical.sort((a,b)->Long.compare(files.get(a).offset,files.get(b).offset));
            for(String name:physical)if(!digest(iso,files.get(name),buffer,progress,count,total).equals(expected.getJSONObject(name).getString("sha256")))throw new IOException("ISO повреждён или отличается от DOA3 USA: "+name);
            String[] names=files.keySet().toArray(new String[0]);long[] offsets=new long[names.length],sizes=new long[names.length];
            for(int i=0;i<names.length;i++){offsets[i]=files.get(names[i]).offset;sizes[i]=files.get(names[i]).size;}
            snapshot=new Snapshot(uri,channel.size(),names,offsets,sizes);if(progress!=null)progress.bytes(total,total);
        }
        JSONObject json=new JSONObject().put("version",1).put("uri",uri.toString()).put("length",snapshot.length);JSONArray files=new JSONArray();
        for(int i=0;i<snapshot.names.length;i++)files.put(new JSONObject().put("name",snapshot.names[i]).put("offset",snapshot.offsets[i]).put("size",snapshot.sizes[i]));json.put("files",files);
        AtomicFile store=store(context);FileOutputStream out=null;
        try{out=store.startWrite();out.write(json.toString().getBytes(StandardCharsets.UTF_8));store.finishWrite(out);}catch(Exception error){if(out!=null)store.failWrite(out);throw error;}
        return snapshot;
    }
    static Opened open(Context context)throws Exception{
        JSONObject saved;try(InputStream in=store(context).openRead()){saved=new JSONObject(new String(readAll(in,65536),StandardCharsets.UTF_8));}
        if(saved.getInt("version")!=1)throw new IOException("Выберите ISO заново");Uri uri=Uri.parse(saved.getString("uri"));
        if(!"content".equals(uri.getScheme()))throw new IOException("Недопустимый источник ISO");
        ParcelFileDescriptor descriptor=context.getContentResolver().openFileDescriptor(uri,"r");if(descriptor==null)throw new IOException("ISO недоступен. Выберите его заново.");
        boolean okay=false;
        try{
            JSONObject expected=identity(context);String[] names;long[] offsets,sizes;long length;
            try(ParcelFileDescriptor.AutoCloseInputStream in=new ParcelFileDescriptor.AutoCloseInputStream(ParcelFileDescriptor.dup(descriptor.getFileDescriptor()))){
                FileChannel channel=in.getChannel();length=channel.size();if(length!=saved.getLong("length"))throw new IOException("ISO изменён. Выберите его заново.");
                Map<String,XboxIso.Entry> actual=entries(channel,expected);JSONArray files=saved.getJSONArray("files");
                if(files.length()!=actual.size())throw new IOException("Повреждено описание ISO");names=new String[files.length()];offsets=new long[names.length];sizes=new long[names.length];Set<String> seen=new HashSet<>();
                for(int i=0;i<names.length;i++){JSONObject file=files.getJSONObject(i);String name=file.getString("name");XboxIso.Entry entry=actual.get(name);if(entry==null||!seen.add(name)||entry.offset!=file.getLong("offset")||entry.size!=file.getLong("size"))throw new IOException("Каталог ISO изменён. Выберите его заново.");names[i]=name;offsets[i]=entry.offset;sizes[i]=entry.size;}
                if(!digest(new XboxIso(channel),actual.get("default.xbe"),new byte[1024*1024],null,new long[]{0},0).equals(expected.getJSONObject("default.xbe").getString("sha256")))throw new IOException("Неверная версия XBE в ISO");
            }
            okay=true;return new Opened(descriptor,new Snapshot(uri,length,names,offsets,sizes));
        }finally{if(!okay)descriptor.close();}
    }
}
