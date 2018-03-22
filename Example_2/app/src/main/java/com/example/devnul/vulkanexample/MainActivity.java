package com.example.devnul.vulkanexample;

import android.app.Activity;
import android.os.Bundle;



public class MainActivity extends Activity {
    private VulkanSurfaceView vulkanView = null;

    ////////////////////////////////////////////////////////////////////////////////////

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        vulkanView = new VulkanSurfaceView(this.getApplicationContext());
        setContentView(vulkanView);
    }
}
