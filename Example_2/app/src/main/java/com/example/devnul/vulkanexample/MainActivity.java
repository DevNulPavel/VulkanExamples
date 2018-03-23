package com.example.devnul.vulkanexample;

import android.app.Activity;
import android.os.Bundle;



public class MainActivity extends Activity {
    private VulkanSurfaceView vulkanView = null;

    ////////////////////////////////////////////////////////////////////////////////////

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        this.vulkanView = new VulkanSurfaceView(this.getApplicationContext(), getAssets());
        setContentView(vulkanView);
    }
}
