package com.pandoraremote;

import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    private TextView mStatusText;
    private View mSetupView;
    private View mStep1View;
    private View mStep2View;
    private TextView mStep1Title;
    private TextView mStep2Title;
    private TextView mVersionText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mStatusText = findViewById(R.id.status_text);
        mSetupView = findViewById(R.id.setup_view);
        mStep1View = findViewById(R.id.step1_view);
        mStep2View = findViewById(R.id.step2_view);
        mStep1Title = findViewById(R.id.step1_title);
        mStep2Title = findViewById(R.id.step2_title);
        mVersionText = findViewById(R.id.version_text);

        Button appSettingsButton = findViewById(R.id.btn_app_settings);
        appSettingsButton.setOnClickListener(v -> {
            Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
            intent.setData(Uri.fromParts("package", getPackageName(), null));
            startActivity(intent);
        });

        Button grantAccessButton = findViewById(R.id.btn_grant_access);
        grantAccessButton.setOnClickListener(v ->
            startActivity(new Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
        );

        mVersionText.setText("v" + getVersionName());

        startService(new Intent(this, PebbleCommService.class));
    }

    @Override
    protected void onResume() {
        super.onResume();
        boolean listenerEnabled = isNotificationListenerEnabled();
        boolean needsRestrictedSettings = Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU;

        if (listenerEnabled) {
            mSetupView.setVisibility(View.GONE);
            mStatusText.setVisibility(View.VISIBLE);
            String song = PandoraNotificationListener.getSong();
            if (song != null && !song.isEmpty()) {
                String artist = PandoraNotificationListener.getArtist();
                String station = PandoraNotificationListener.getStation();
                boolean playing = PandoraNotificationListener.isPlaying();
                mStatusText.setText("Pandora found.\n"
                    + (playing ? "Playing" : "Paused") + ": " + song
                    + "\nby " + artist
                    + "\non " + station);
            } else {
                mStatusText.setText("PandoraRemote is running.\nWaiting for Pandora...");
            }
        } else {
            mSetupView.setVisibility(View.VISIBLE);
            mStatusText.setVisibility(View.GONE);

            if (needsRestrictedSettings) {
                mStep1View.setVisibility(View.VISIBLE);
                mStep2Title.setText(R.string.notification_access_title);
            } else {
                mStep1View.setVisibility(View.GONE);
                mStep2Title.setText(R.string.notification_access_title_no_step);
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_set_permissions) {
            startActivity(new Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS));
            return true;
        } else if (id == R.id.action_about) {
            showAboutDialog();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void showAboutDialog() {
        String about = getString(R.string.about_text, getVersionName());
        new AlertDialog.Builder(this)
            .setTitle("About")
            .setMessage(about)
            .setPositiveButton("OK", null)
            .show();
    }

    private String getVersionName() {
        try {
            PackageInfo info = getPackageManager().getPackageInfo(getPackageName(), 0);
            return info.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            return "unknown";
        }
    }

    private boolean isNotificationListenerEnabled() {
        String flat = Settings.Secure.getString(getContentResolver(), "enabled_notification_listeners");
        if (flat == null) return false;
        ComponentName cn = new ComponentName(this, PandoraNotificationListener.class);
        return flat.contains(cn.flattenToString());
    }
}
