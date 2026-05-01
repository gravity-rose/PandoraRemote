package com.pandoraremote;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.util.Log;

import com.getpebble.android.kit.PebbleKit;

public class PandoraNotificationListener extends NotificationListenerService {

    private static final String TAG = "PandoraNotifListener";
    private static final String PANDORA_PACKAGE = "com.pandora.android";

    private static PandoraNotificationListener sInstance;
    private static Notification.Action sThumbsUp;
    private static Notification.Action sThumbsDown;
    private static Notification.Action sSkipNext;
    private static Notification.Action sSkipPrev;
    private static Notification.Action sPlayPause;

    private static String sStation = "";
    private static String sArtist = "";
    private static String sSong = "";
    private static boolean sIsPlaying = false;
    private static boolean sWatchAppLaunched = false;

    @Override
    public void onCreate() {
        super.onCreate();
        sInstance = this;
        Log.d(TAG, "NotificationListener created");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        sInstance = null;
    }

    @Override
    public void onListenerConnected() {
        Log.d(TAG, "Listener connected, checking active notifications");
        for (StatusBarNotification sbn : getActiveNotifications()) {
            if (PANDORA_PACKAGE.equals(sbn.getPackageName())) {
                Log.d(TAG, "Found existing Pandora notification");
                processNotification(sbn);
                break;
            }
        }
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn) {
        if (!PANDORA_PACKAGE.equals(sbn.getPackageName())) return;
        processNotification(sbn);
    }

    private void processNotification(StatusBarNotification sbn) {
        Notification notification = sbn.getNotification();
        Bundle extras = notification.extras;

        String title = extras.getString(Notification.EXTRA_TITLE, "");
        String text = extras.getString(Notification.EXTRA_TEXT, "");
        String subText = extras.getString(Notification.EXTRA_SUB_TEXT, "");

        sSong = title;
        sArtist = text;
        sStation = subText;

        Notification.Action[] actions = notification.actions;
        if (actions != null) {
            mapActions(actions);
        }

        sIsPlaying = (notification.flags & Notification.FLAG_ONGOING_EVENT) != 0;

        Log.d(TAG, "Pandora update: " + sSong + " by " + sArtist + " on " + sStation
                + " playing=" + sIsPlaying
                + " [actions: " + (actions != null ? actions.length : 0) + "]");

        if (!sIsPlaying || (title.isEmpty() && text.isEmpty())) {
            return;
        }

        if (!sWatchAppLaunched) {
            sWatchAppLaunched = true;
            startService(new Intent(this, PebbleCommService.class));
            try {
                PebbleKit.startAppOnPebble(this, PebbleCommService.WATCHAPP_UUID);
                Log.d(TAG, "Launched watch app from Pandora notification");
            } catch (SecurityException e) {
                Log.w(TAG, "Could not launch watch app: " + e.getMessage());
            }
        }

        PebbleCommService.sendMetadataToWatch(this, sStation, sArtist, sSong, sIsPlaying);
    }

    @Override
    public void onNotificationRemoved(StatusBarNotification sbn) {
        if (PANDORA_PACKAGE.equals(sbn.getPackageName())) {
            sIsPlaying = false;
            sWatchAppLaunched = false;
        }
    }

    private void mapActions(Notification.Action[] actions) {
        sThumbsUp = null;
        sThumbsDown = null;
        sSkipNext = null;
        sSkipPrev = null;
        sPlayPause = null;

        for (Notification.Action action : actions) {
            if (action.title == null) continue;
            String label = action.title.toString().toLowerCase();
            Log.d(TAG, "  Action label: \"" + label + "\"");

            if (label.contains("thumb") && label.contains("up")) {
                sThumbsUp = action;
            } else if (label.contains("thumb") && label.contains("down")) {
                sThumbsDown = action;
            } else if (label.contains("skip") || label.contains("next")) {
                sSkipNext = action;
            } else if (label.contains("replay") || label.contains("prev") || label.contains("back") || label.contains("rewind")) {
                sSkipPrev = action;
            } else if (label.contains("play") || label.contains("pause")) {
                sPlayPause = action;
            }
        }
    }

    public static void fireThumbsUp() {
        fireAction(sThumbsUp, "ThumbsUp");
    }

    public static void fireThumbsDown() {
        fireAction(sThumbsDown, "ThumbsDown");
    }

    public static void fireSkipNext() {
        fireAction(sSkipNext, "SkipNext");
    }

    public static void fireSkipPrev() {
        fireAction(sSkipPrev, "SkipPrev");
    }

    public static void firePlayPause() {
        fireAction(sPlayPause, "PlayPause");
    }

    private static void fireAction(Notification.Action action, String name) {
        if (action == null) {
            Log.w(TAG, name + " action not available");
            return;
        }
        try {
            action.actionIntent.send();
            Log.d(TAG, "Fired " + name);
        } catch (PendingIntent.CanceledException e) {
            Log.e(TAG, "Failed to fire " + name, e);
        }
    }

    public static String getStation() { return sStation; }
    public static String getArtist() { return sArtist; }
    public static String getSong() { return sSong; }
    public static boolean isPlaying() { return sIsPlaying; }
}
