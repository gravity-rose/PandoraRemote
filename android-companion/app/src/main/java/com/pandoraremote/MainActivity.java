package com.pandoraremote;

import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
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
    private TextView mVersionText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mStatusText = findViewById(R.id.status_text);
        mSetupView = findViewById(R.id.setup_view);
        mVersionText = findViewById(R.id.version_text);

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
        if (isNotificationListenerEnabled()) {
            mSetupView.setVisibility(View.GONE);
            mStatusText.setVisibility(View.VISIBLE);
            mStatusText.setText("PandoraRemote is running.\nWaiting for Pandora...");
        } else {
            mSetupView.setVisibility(View.VISIBLE);
            mStatusText.setVisibility(View.GONE);
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
