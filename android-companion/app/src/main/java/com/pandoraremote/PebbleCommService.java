package com.pandoraremote;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

import java.util.UUID;

public class PebbleCommService extends Service {

    private static final String TAG = "PebbleCommService";
    static final UUID WATCHAPP_UUID = UUID.fromString("87b85f04-54ce-4b5e-8941-14a46c9d3855");
    private static final String PANDORA_PACKAGE = "com.pandora.android";

    private static final int KEY_COMMAND = 0;
    private static final int KEY_STATION = 1;
    private static final int KEY_ARTIST = 2;
    private static final int KEY_SONG = 3;
    private static final int KEY_PLAY_STATE = 4;

    private static final int CMD_THUMBS_UP = 1;
    private static final int CMD_THUMBS_DOWN = 2;
    private static final int CMD_NEXT = 3;
    private static final int CMD_PREVIOUS = 4;
    private static final int CMD_PLAY_PAUSE = 5;
    private static final int CMD_REQUEST_INFO = 6;
    private static final int CMD_VOLUME_UP = 7;
    private static final int CMD_VOLUME_DOWN = 8;

    private PebbleKit.PebbleDataReceiver mDataReceiver;

    @Override
    public void onCreate() {
        super.onCreate();
        registerPebbleReceiver();
        Log.d(TAG, "PebbleCommService started");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mDataReceiver != null) {
            unregisterReceiver(mDataReceiver);
            mDataReceiver = null;
        }
    }

    private void registerPebbleReceiver() {
        mDataReceiver = new PebbleKit.PebbleDataReceiver(WATCHAPP_UUID) {
            @Override
            public void receiveData(Context context, int transactionId, PebbleDictionary data) {
                PebbleKit.sendAckToPebble(context, transactionId);

                Long cmdValue = data.getInteger(KEY_COMMAND);
                if (cmdValue == null) return;

                int cmd = cmdValue.intValue();
                Log.d(TAG, "Received command: " + cmd);

                switch (cmd) {
                    case CMD_THUMBS_UP:
                        PandoraNotificationListener.fireThumbsUp();
                        break;
                    case CMD_THUMBS_DOWN:
                        PandoraNotificationListener.fireThumbsDown();
                        break;
                    case CMD_NEXT:
                        PandoraNotificationListener.fireSkipNext();
                        break;
                    case CMD_PREVIOUS:
                        PandoraNotificationListener.fireSkipPrev();
                        break;
                    case CMD_PLAY_PAUSE:
                        PandoraNotificationListener.firePlayPause();
                        break;
                    case CMD_REQUEST_INFO:
                        launchPandoraIfNeeded(context);
                        sendMetadataToWatch(context,
                            PandoraNotificationListener.getStation(),
                            PandoraNotificationListener.getArtist(),
                            PandoraNotificationListener.getSong(),
                            PandoraNotificationListener.isPlaying());
                        break;
                    case CMD_VOLUME_UP:
                        adjustVolume(context, AudioManager.ADJUST_RAISE);
                        break;
                    case CMD_VOLUME_DOWN:
                        adjustVolume(context, AudioManager.ADJUST_LOWER);
                        break;
                }
            }
        };

        IntentFilter filter = new IntentFilter("com.getpebble.action.app.RECEIVE");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(mDataReceiver, filter, Context.RECEIVER_EXPORTED);
        } else {
            registerReceiver(mDataReceiver, filter);
        }
        Log.d(TAG, "Registered PebbleDataReceiver with RECEIVER_EXPORTED");
    }

    private void adjustVolume(Context context, int direction) {
        AudioManager audio = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        if (audio != null) {
            audio.adjustStreamVolume(AudioManager.STREAM_MUSIC, direction, 0);
            Log.d(TAG, "Volume adjusted: " + (direction == AudioManager.ADJUST_RAISE ? "up" : "down"));
        }
    }

    private void launchPandoraIfNeeded(Context context) {
        if (PandoraNotificationListener.isPlaying()) return;
        PackageManager pm = context.getPackageManager();
        Intent launchIntent = pm.getLaunchIntentForPackage(PANDORA_PACKAGE);
        if (launchIntent != null) {
            launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(launchIntent);
            Log.d(TAG, "Launched Pandora from watch request");
        } else {
            Log.w(TAG, "Pandora not installed");
        }
    }

    public static void sendMetadataToWatch(Context context, String station, String artist, String song, boolean isPlaying) {
        PebbleDictionary dict = new PebbleDictionary();
        dict.addString(KEY_STATION, station != null ? station : "");
        dict.addString(KEY_ARTIST, artist != null ? artist : "");
        dict.addString(KEY_SONG, song != null ? song : "");
        dict.addInt32(KEY_PLAY_STATE, isPlaying ? 1 : 0);
        PebbleKit.sendDataToPebble(context, WATCHAPP_UUID, dict);
    }
}
