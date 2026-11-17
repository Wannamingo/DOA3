package org.doa3.port;

import java.io.*;
import java.util.HashSet;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/** Extract only the two ARM64 ANGLE libraries. Never use ZIP names as disk paths. */
final class DriverPackage {
    static final String EGL = "libEGL_angle.so";
    static final String GLES = "libGLESv2_angle.so";
    private static final int MAX_LIBRARY = 64 * 1024 * 1024;
    private static final int MAX_METADATA = 1024 * 1024;

    private DriverPackage() {}

    static void extract(InputStream input, File destination) throws IOException {
        Set<String> found = new HashSet<>();
        int entries = 0;
        byte[] buffer = new byte[64 * 1024];
        try (ZipInputStream zip = new ZipInputStream(new BufferedInputStream(input))) {
            for (ZipEntry entry; (entry = zip.getNextEntry()) != null;) {
                if (++entries > 32) throw new IOException("Too many ZIP entries");
                String path = entry.getName();
                if (path.startsWith("lib/arm64-v8a/")) path = path.substring("lib/arm64-v8a/".length());
                else if (path.startsWith("arm64-v8a/")) path = path.substring("arm64-v8a/".length());
                if (!entry.isDirectory() && (EGL.equals(path) || GLES.equals(path))) {
                    if (!found.add(path)) throw new IOException("Duplicate library: " + path);
                    byte[] header = new byte[64];
                    new DataInputStream(zip).readFully(header);
                    if (!arm64SharedLibrary(header)) throw new IOException("Expected ARM64 ELF: " + path);
                    File target = new File(destination, path);
                    if (!target.createNewFile()) throw new IOException("Driver file already exists");
                    try (FileOutputStream output = new FileOutputStream(target)) {
                        if (!target.setReadOnly()) throw new IOException("Cannot protect driver file");
                        output.write(header);
                        drain(zip, output, buffer, MAX_LIBRARY - header.length);
                        output.getFD().sync();
                    }
                } else {
                    drain(zip, null, buffer, MAX_METADATA);
                }
                zip.closeEntry();
            }
        }
        if (found.size() != 2) throw new IOException("ZIP must contain both " + EGL + " and " + GLES);
    }

    private static void drain(InputStream input, OutputStream output, byte[] buffer, int remaining) throws IOException {
        for (int read; (read = input.read(buffer)) != -1;) {
            if (read > remaining) throw new IOException("Driver ZIP entry is too large");
            remaining -= read;
            if (output != null) output.write(buffer, 0, read);
        }
    }

    static boolean arm64SharedLibrary(byte[] h) {
        return h.length >= 64 && h[0] == 0x7f && h[1] == 'E' && h[2] == 'L' && h[3] == 'F'
            && h[4] == 2 && h[5] == 1 && h[6] == 1
            && h[16] == 3 && h[17] == 0 && (h[18] & 0xff) == 183 && h[19] == 0;
    }
}
