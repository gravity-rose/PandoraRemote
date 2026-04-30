package com.pandoraremote;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

import java.util.UUID;

public class PebbleCommService extends Service {

    private static final String TAG = "PebbleCommService";
    private static final UUID WATCHAPP_UUID = UUID.fromString("a1b2c3d4-e5f6-7890-abcd-ef1234567890");

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
                        sendMetadataToWatch(context,
                            PandoraNotificationListener.getStation(),
                            PandoraNotificationListener.getArtist(),
                            PandoraNotificationListener.getSong(),
                            PandoraNotificationListener.isPlaying());
                        break;
                }
            }
        };
        PebbleKit.registerReceivedDataHandler(this, mDataReceiver);
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
