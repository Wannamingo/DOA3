package org.doa3.port;

import java.io.*;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;

/** Integration check against the user's real ISO and authenticated file list. */
public final class XboxIsoIntegrationTest {
    public static void main(String[] args) throws Exception {
        Map<String,String[]> expected=new HashMap<>();
        for(String line:Files.readAllLines(Paths.get(args[1]),StandardCharsets.UTF_8)){
            String[] fields=line.split("\t");
            expected.put(fields[0].toLowerCase(Locale.ROOT),fields);
        }
        long total=0;
        try(FileInputStream input=new FileInputStream(args[0])){
            XboxIso iso=new XboxIso(input.getChannel());
            Map<String,XboxIso.Entry> files=iso.index();
            if(files.size()!=expected.size())throw new IOException("Unexpected file count: "+files.size());
            byte[] buffer=new byte[1024*1024];
            for(Map.Entry<String,XboxIso.Entry> entry:files.entrySet()){
                String[] identity=expected.remove(entry.getKey().toLowerCase(Locale.ROOT));
                if(identity==null||entry.getValue().size!=Long.parseLong(identity[1]))throw new IOException("Unexpected ISO entry");
                MessageDigest hash=MessageDigest.getInstance("SHA-256");
                try(InputStream file=iso.open(entry.getValue())){
                    int count;while((count=file.read(buffer))!=-1){hash.update(buffer,0,count);total+=count;}
                }
                StringBuilder hex=new StringBuilder();
                for(byte value:hash.digest())hex.append(String.format(Locale.ROOT,"%02x",value&255));
                if(!hex.toString().equals(identity[2]))throw new IOException("ISO file hash mismatch: "+identity[0]);
            }
            if(!expected.isEmpty())throw new IOException("Missing game files");
            System.out.println("ISO integration passed: "+files.size()+" files, "+total+" bytes, all SHA-256 identities match.");
        }
    }
}
