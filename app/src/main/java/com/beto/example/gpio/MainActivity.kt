package com.beto.example.gpio

import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_main.*
import java.io.OutputStream

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
//        val cmd = "chmod 666 /sys/class/gpio/export"
//        execChmod(cmd)
        // Example of a call to a native method
        sample_text.text = "gpio demo"
        val time = System.currentTimeMillis()
        val gpio1 = 1162
        val export = JniGpio.export(gpio1)
        export.apply {
            JniGpio.setDirection(gpio1, 1)
            val direction = JniGpio.getDirection(gpio1)
            Log.i(TAG, "1162=> getDirection: $direction")
            var value = JniGpio.getValue(gpio1)
            Log.i(TAG, "1162=> getValue: $value")
            Log.i(TAG, "1162=>  set value: " + JniGpio.setValue(gpio1, true))
            value = JniGpio.getValue(gpio1)
            Log.i(TAG, "1162=> getValue: $value")
        }

        val gpio2 = 1163
        JniGpio.export(gpio2).apply {
            val value = JniGpio.getValue(gpio2)
            Log.i(TAG, "1163=> value: $value")
        }
        Log.i(TAG, "const: " + (System.currentTimeMillis() - time))
//        JniGpio.unexport(gpio1)
//        JniGpio.unexport(gpio2)
        Log.i(TAG, "const: " + (System.currentTimeMillis() - time))
    }

    private fun execChmod(cmd: String) {
        var exec: Process? = null
        var outputStream: OutputStream? = null
        try {
            exec = Runtime.getRuntime().exec("su")
            outputStream = exec.outputStream

            outputStream.write("$cmd\n".toByteArray())
            outputStream.flush()
            outputStream.write("exit\n".toByteArray())
            outputStream.flush()
            outputStream.close()
            exec.waitFor()
        } catch (e: Exception) {
        } finally {
            outputStream?.close()
            exec?.destroy()
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */

    companion object {
        const val TAG = "MainActivity"

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-gpio-lib")
        }
    }

//    external fun start_jniThread()
//    external fun stop_jniThread()
}
