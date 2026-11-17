package org.doa3.port;

import java.io.*;
import java.nio.file.Files;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public final class DriverPackageTest {
    private static byte[] library(int machine) {
        byte[] header = new byte[80];
        header[0] = 0x7f; header[1] = 'E'; header[2] = 'L'; header[3] = 'F';
        header[4] = 2; header[5] = 1; header[6] = 1;
        header[16] = 3; header[18] = (byte) machine;
        return header;
    }

    private static byte[] archive(String[] names, byte[][] payloads) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        try (ZipOutputStream zip = new ZipOutputStream(bytes)) {
            for (int i = 0; i < names.length; i++) {
                zip.putNextEntry(new ZipEntry(names[i]));
                zip.write(payloads[i]);
                zip.closeEntry();
            }
        }
        return bytes.toByteArray();
    }

    private static void check(File parent, boolean success, String[] names, byte[][] payloads) throws IOException {
        File directory = Files.createTempDirectory(parent.toPath(), "case").toFile();
        boolean accepted = false;
        try {
            DriverPackage.extract(new ByteArrayInputStream(archive(names, payloads)), directory);
            accepted = true;
            if (directory.list().length != 2) throw new AssertionError("Unexpected output file");
        } catch (IOException rejected) {
            if (success) throw rejected;
        } finally {
            for (File file : directory.listFiles()) {
                file.setWritable(true);
                Files.delete(file.toPath());
            }
            Files.delete(directory.toPath());
        }
        if (accepted != success) throw new AssertionError("Wrong ZIP validation result");
    }

    public static void main(String[] args) throws IOException {
        File out = Files.createTempDirectory("doa3-driver-test").toFile();
        byte[] arm = library(183);
        String egl = DriverPackage.EGL, gles = DriverPackage.GLES;
        try {
            check(out, true, new String[]{egl, gles, "../unexpected"}, new byte[][]{arm, arm, {1}});
            if (new File(out, "unexpected").exists()) throw new AssertionError("ZIP escaped output");
            check(out, true, new String[]{"lib/arm64-v8a/" + egl, "lib/arm64-v8a/" + gles},
                new byte[][]{arm, arm});
            check(out, false, new String[]{egl, gles}, new byte[][]{arm, library(62)});
            check(out, false, new String[]{egl}, new byte[][]{arm});
            check(out, false, new String[]{egl, "arm64-v8a/" + egl, gles}, new byte[][]{arm, arm, arm});
            check(out, false, new String[]{egl, gles}, new byte[][]{arm, new byte[8]});
        } finally {
            Files.delete(out.toPath());
        }
        System.out.println("Driver ZIP: valid ARM64 pairs imported; incorrect ABI, missing, duplicate, and truncated libraries rejected.");
    }
}
