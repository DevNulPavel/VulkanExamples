package com.example.devnul.vulkanexample;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.graphics.Canvas;


public class VulkanDrawThread extends Thread{
    static {
        System.loadLibrary("native-lib");
    }

    private boolean runFlag = false;
    private SurfaceHolder surfaceHolder = null;

    VulkanDrawThread(SurfaceHolder surfaceHolder){
        this.surfaceHolder = surfaceHolder;
    }

    public void setRunning(boolean run) {
        synchronized (this) {
            runFlag = run;
        }
    }

    @Override
    public void run() {
        // TODO: самая простая реализация, на сворачивание игры рушится контекст, на запуск - заново все создается

        Surface surface = surfaceHolder.getSurface();
        vulkanInit(surface);

        while (runFlag) {
            Canvas canvas = null;
            try {
                canvas = surfaceHolder.lockCanvas(null);
                synchronized (this) {
                    vulkanDraw();
                }
            } finally {
                if (canvas != null) {
                    // отрисовка выполнена. выводим результат на экран
                    surfaceHolder.unlockCanvasAndPost(canvas);
                }
            }
        }

        vulkanDestroy();
    }

    // Нативные методы
    public native void vulkanInit(Object surface);
    public native void vulkanDraw();
    public native void vulkanDestroy();
}