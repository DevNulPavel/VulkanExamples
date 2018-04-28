package com.example.devnul.vulkanexample;

import android.app.Activity;
import android.content.res.Configuration;
import android.os.Bundle;
import android.content.pm.ActivityInfo;
import android.view.View;
import android.widget.RelativeLayout;


public class MainActivity extends Activity {
    private VulkanSurfaceView vulkanView = null;

    ////////////////////////////////////////////////////////////////////////////////////

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        this.vulkanView = new VulkanSurfaceView(this.getApplicationContext(), getAssets());
        setContentView(vulkanView);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        // Checks the orientation of the screen
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
        } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT){
        }
    }
}
