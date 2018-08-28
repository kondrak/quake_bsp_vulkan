package pl.kondrak.quakebspviewer;

import android.app.NativeActivity;
import android.os.Bundle;

public class MainActivity extends NativeActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
}
