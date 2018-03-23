package com.example.devnul.vulkanexample;

import android.content.Context;
import android.content.res.AssetManager;
import android.view.SurfaceView;
import android.view.SurfaceHolder;


public class VulkanSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    private VulkanDrawThread vulkanDrawThread = null;
    private AssetManager assetManager = null;


    public VulkanSurfaceView(Context context, AssetManager inAssetManager) {
        super(context);
        assetManager = inAssetManager;
        getHolder().addCallback(this);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        vulkanDrawThread = new VulkanDrawThread(getHolder(), assetManager);
        vulkanDrawThread.setRunning(true);
        vulkanDrawThread.start();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        boolean retry = true;
        vulkanDrawThread.setRunning(false);
        while (retry) {
            try {
                if (vulkanDrawThread.isAlive()){
                    vulkanDrawThread.join();
                }
                retry = false;
            } catch (InterruptedException e) {
                // если не получилось, то будем пытаться еще и еще
            }
        }
    }
}
