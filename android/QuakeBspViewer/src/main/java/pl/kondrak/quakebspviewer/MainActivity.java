package pl.kondrak.quakebspviewer;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.view.View;
import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity
{
    private AssetManager assetMgr;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        assetMgr = getResources().getAssets();
        initAssetManager(assetMgr);

        View v = getWindow().getDecorView();
        v.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN | 
                                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | 
                                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        if(hasFocus) {
            View v = getWindow().getDecorView();
            v.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN | 
                                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | 
                                    View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        }
    }

    private static native void initAssetManager(AssetManager mgr);
}
