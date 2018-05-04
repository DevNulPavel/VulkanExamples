package com.example.devnul.vulkanexample;

import android.content.res.AssetManager;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.graphics.Canvas;


public class VulkanDrawThread extends Thread{
    static {
        System.loadLibrary("vulkan_cpp_code");
        //System.loadLibrary("MGD");
    }

    private boolean runFlag = false;
    private SurfaceHolder surfaceHolder = null;
    private AssetManager assetManager = null;

    VulkanDrawThread(SurfaceHolder surfaceHolder, AssetManager inAssetManager){
        this.surfaceHolder = surfaceHolder;
        this.assetManager = inAssetManager;
        setPriority(MAX_PRIORITY);
    }

    public void setRunning(boolean run) {
        synchronized (this) {
            runFlag = run;
        }
    }

    @Override
    public void run() {
        // TODO: самая простая реализация, на сворачивание игры рушится контекст, на запуск - заново все создается
        Surface surface = this.surfaceHolder.getSurface();
        int width = this.surfaceHolder.getSurfaceFrame().width();
        int height = this.surfaceHolder.getSurfaceFrame().height();
        vulkanInit(surface, width, height, this.assetManager);

        while (runFlag) {
            vulkanDraw();
        }

        /*Canvas canvas = null;
        while (runFlag) {
            try {
                canvas = this.surfaceHolder.lockCanvas(null);
                synchronized (this) {
                    vulkanDraw();
                }
            } finally {
                if (canvas != null) {
                    // отрисовка выполнена. выводим результат на экран
                    this.surfaceHolder.unlockCanvasAndPost(canvas);
                    canvas = null;
                }
            }
        }*/

        vulkanDestroy();
    }

    // Нативные методы
    public native void vulkanInit(Object surface, int width, int height, Object assetManager);
    public native void vulkanDraw();
    public native void vulkanDestroy();
}