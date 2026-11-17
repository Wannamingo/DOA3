package org.doa3.port;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.AtomicFile;
import android.widget.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.UUID;

/** Launcher-only driver settings. The game process reads the active slot directly. */
public final class GpuDriverActivity extends Activity {
    private static final int PICK_DRIVER = 241;
    private static final String LIBRARY_DIRECTORY = "gpu-drivers";
    private boolean english, busy;
    private TextView status;
    private Button useSystem, useCustom, importDriver;

    private static File directory(Context context) {
        return new File(context.getFilesDir(), LIBRARY_DIRECTORY);
    }

    private static AtomicFile selection(Context context) {
        return new AtomicFile(new File(directory(context), "selection"));
    }

    private static String readSelection(Context context) {
        try (DataInputStream in = new DataInputStream(selection(context).openRead())) {
            int length = in.readUnsignedByte();
            if (length != 36 && length != 37) return "";
            byte[] data = new byte[length];
            in.readFully(data);
            String value = new String(data, StandardCharsets.US_ASCII);
            return value.matches("!?[0-9a-f]{8}(-[0-9a-f]{4}){3}-[0-9a-f]{12}") ? value : "";
        } catch (IOException unavailable) {
            return "";
        }
    }

    private static void select(Context context, String value) throws IOException {
        AtomicFile file = selection(context);
        byte[] data = value.getBytes(StandardCharsets.US_ASCII);
        FileOutputStream stream = file.startWrite();
        try {
            stream.write(data.length);
            stream.write(data);
            file.finishWrite(stream);
        } catch (IOException error) {
            file.failWrite(stream);
            throw error;
        }
        if (!value.equals(readSelection(context))) throw new IOException("Could not save driver selection");
    }

    private static File installed(Context context, String slot) {
        if (slot.isEmpty()) return null;
        File folder = new File(directory(context), slot.startsWith("!") ? slot.substring(1) : slot);
        return new File(folder, DriverPackage.EGL).isFile() && new File(folder, DriverPackage.GLES).isFile() ? folder : null;
    }

    static File activeDriver(Context context) {
        String slot = readSelection(context);
        return slot.startsWith("!") ? null : installed(context, slot);
    }

    private String text(String russian, String english) { return this.english ? english : russian; }

    @Override public void onCreate(Bundle saved) {
        super.onCreate(saved);
        english = "en".equals(getSharedPreferences("controls", MODE_PRIVATE).getString("language", "ru"));
        setTitle(text("Драйвер GPU", "GPU driver"));
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        int dp = Math.round(24 * getResources().getDisplayMetrics().density);
        column.setPadding(dp, dp, dp, dp);
        TextView title = new TextView(this);
        title.setText(getTitle());
        title.setTextSize(24);
        column.addView(title);
        status = new TextView(this);
        column.addView(status);
        TextView description = new TextView(this);
        description.setText(text(
            "Экспериментальный ANGLE для OpenGL ES. ZIP должен содержать совместимую пару ARM64: libEGL_angle.so и libGLESv2_angle.so. Выбор применяется при следующем запуске игры.",
            "Experimental ANGLE for OpenGL ES. The ZIP must contain a matching ARM64 pair: libEGL_angle.so and libGLESv2_angle.so. Selection applies the next time the game starts."));
        column.addView(description);
        useSystem = button(column, text("Системный драйвер", "Use system driver"), () -> change(false));
        useCustom = button(column, text("Использовать ANGLE", "Use imported ANGLE"), () -> change(true));
        importDriver = button(column, text("Импортировать ANGLE ZIP", "Import ANGLE ZIP"), this::confirmImport);
        button(column, text("Готово", "Done"), this::finish);
        ScrollView scroll = new ScrollView(this);
        scroll.addView(column);
        setContentView(scroll);
        refresh();
    }

    private Button button(LinearLayout parent, String label, Runnable action) {
        Button button = new Button(this);
        button.setText(label);
        button.setOnClickListener(v -> action.run());
        parent.addView(button);
        return button;
    }

    private void refresh() {
        String slot = readSelection(this);
        boolean available = installed(this, slot) != null;
        boolean active = available && !slot.startsWith("!");
        status.setText(busy ? text("Проверка ZIP…", "Checking ZIP…")
            : active ? text("Выбран ANGLE", "ANGLE selected")
            : text("Выбран системный драйвер", "System driver selected"));
        useSystem.setEnabled(!busy && active);
        useCustom.setEnabled(!busy && available && !active);
        importDriver.setEnabled(!busy);
    }

    private void change(boolean custom) {
        try {
            String slot = readSelection(this);
            if (installed(this, slot) == null) return;
            if (slot.startsWith("!")) slot = slot.substring(1);
            select(this, custom ? slot : "!" + slot);
        } catch (IOException error) {
            showError(error);
        }
        refresh();
    }

    private void confirmImport() {
        new AlertDialog.Builder(this)
            .setMessage(text("Драйвер выполняется с разрешениями приложения. Выбирайте ZIP из источника, которому доверяете.",
                             "The driver runs with the app's permissions. Only select a ZIP from a source you trust."))
            .setNegativeButton(android.R.string.cancel, null)
            .setPositiveButton(text("Выбрать ZIP", "Select ZIP"), (dialog, button) -> {
                Intent picker = new Intent(Intent.ACTION_OPEN_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("*/*");
                try { startActivityForResult(picker, PICK_DRIVER); }
                catch (RuntimeException unavailable) { showError(unavailable); }
            }).show();
    }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != PICK_DRIVER || result != RESULT_OK || data == null || data.getData() == null || busy) return;
        Uri uri = data.getData();
        busy = true;
        refresh();
        new Thread(() -> {
            Exception failure = null;
            try { install(getApplicationContext(), uri); }
            catch (Exception error) { failure = error; }
            Exception completed = failure;
            runOnUiThread(() -> {
                busy = false;
                if (!isDestroyed()) {
                    refresh();
                    if (completed != null) showError(completed);
                }
            });
        }, "doa3-angle-import").start();
    }

    private static synchronized void install(Context context, Uri uri) throws IOException {
        IsoPicker.requireGameStopped(context);
        File parent = directory(context);
        File previous = installed(context, readSelection(context));
        String slot = UUID.randomUUID().toString();
        File folder = new File(parent, slot);
        if (!folder.mkdirs()) throw new IOException("Cannot create driver storage");
        boolean saved = false;
        try (InputStream input = context.getContentResolver().openInputStream(uri)) {
            if (input == null) throw new IOException("Cannot open driver ZIP");
            DriverPackage.extract(input, folder);
            select(context, slot);
            saved = true;
        } finally {
            if (!saved) deleteSlot(folder);
        }
        if (previous != null) deleteSlot(previous);
    }

    private static void deleteSlot(File folder) {
        new File(folder, DriverPackage.EGL).delete();
        new File(folder, DriverPackage.GLES).delete();
        folder.delete();
    }

    private void showError(Exception error) {
        Toast.makeText(this, text("Драйвер: ", "Driver: ") + error.getMessage(), Toast.LENGTH_LONG).show();
    }
}
