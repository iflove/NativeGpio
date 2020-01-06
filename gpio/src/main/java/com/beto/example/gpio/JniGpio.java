package com.beto.example.gpio;

import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.Keep;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;

@Keep
public class JniGpio {
    private static final String TAG = "JniGpio";

    /**
     *
     * @param pinNum gpio编号
     * @return true 成功
     */
    public native static boolean export(int pinNum);

    /**
     *
     * @param pinNum gpio编号
     * @return true 成功
     */
    public native static boolean unexport(int pinNum);

    /**
     *
     * @param pinNum gpio编号
     * @param direction 0: in 1: out
     * @return  true 成功
     */
    public native static boolean setDirection(int pinNum, int direction);

    /**
     *
     * @param pinNum gpio编号
     * @return -1 失败 0: in 1: out
     */
    public native static int getDirection(int pinNum);

    /**
     *
     * @param pinNum gpio编号
     * @param value true: 高电平1 false: 低电平0
     * @return true 成功
     */
    public native static boolean setValue(int pinNum, boolean value);

    /**
     *
     * @param pinNum gpio编号
     * @return true: 高电平1 false: 低电平0
     */
    public native static boolean getValue(int pinNum);

    public static boolean grantRWPermissions(String path) {
        Log.i(TAG, "grantRWPermissions: " + path);
        if (TextUtils.isEmpty(path)) {
            return false;
        }
        File f = new File(path);
        if (!f.exists()) {
            return false;
        }
        boolean already = f.canRead() && f.canWrite();
        if (already) {
            return true;
        }
        Process su = null;
        OutputStream outputStream = null;
        try {
            su = Runtime.getRuntime().exec("su");
            outputStream = su.getOutputStream();
            outputStream.write(("chmod 666 " + path + "\n").getBytes());
            outputStream.flush();
            outputStream.write("exit\n".getBytes());
            outputStream.flush();
            su.waitFor();
            return true;
        } catch (IOException | InterruptedException e) {
            Log.w(TAG, "chmod fail ");
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException ignored) {
                }
            }
            if (su != null) {
                su.destroy();
            }
        }
        return false;
    }

    public interface GpioCallback{

    }
}
