package com.pandoraremote;

import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    private TextView mStatusText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mStatusText = findViewById(R.id.status_text);

        startService(new Intent(this, PebbleCommService.class));
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (isNotificationListenerEnabled()) {
            mStatusText.setText("PandoraRemote is running.\nWaiting for Pandora...");
        } else {
            mStatusText.setText("Notification access required.\nOpening settings...");
            startActivity(new Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS));
        }
    }

    private boolean isNotificationListenerEnabled() {
        String flat = Settings.Secure.getString(getContentResolver(), "enabled_notification_listeners");
        if (flat == null) return false;
        ComponentName cn = new ComponentName(this, PandoraNotificationListener.class);
        return flat.contains(cn.flattenToString());
    }
}
