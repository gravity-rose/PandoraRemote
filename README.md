# PandoraRemote

Control Pandora from your Pebble watch. PandoraRemote is a Pebble watch app paired with an Android companion that lets you manage playback, rate songs, and adjust volume without reaching for your phone.

## How It Works

The Android companion monitors Pandora's notifications and relays song metadata (title, artist, station) to the watch. Button presses on the watch send commands back through the companion, which triggers Pandora's notification actions. The watch app launches automatically when Pandora starts playing.  (and Pandora starts on the phone when you launch the watch app)

## Watch App Screens

Navigate between screens with the **select** (middle) button. Press **back** to return to the previous screen.  Every screen shows the current song's title and artist, and the station name in smaller text at the top.  The different screens let you control playback, rate songs, and adjust volume.

### Screen 1: Playback

The default screen. 

| Button | Action |
|--------|--------|
| Up | Skip to next track |
| Select | Go to Rating screen |
| Down | Play / Pause |

### Screen 2: Rating

Rate the current song.

| Button | Action |
|--------|--------|
| Up | Thumbs Up |
| Select | Go to Volume screen |
| Down | Thumbs Down |

### Screen 3: Volume

Adjust media volume on the phone.

| Button | Action |
|--------|--------|
| Up | Volume Up |
| Select | About screen |
| Down | Volume Down |

## Supported Platforms

- Pebble Time (Basalt)
- Pebble Time Steel (Basalt)
- Pebble Time Round (Chalk)
- Pebble 2 (Diorite)
- Pebble Time 2 (Emery)
- Core 2 Duo (Flint)
- Core Time (Gabbro)

The original Pebble (Aplite) is not supported.

## Setup

1. Install the Android companion APK on your phone. Until the app is available on Google Play, download it from the [latest release](https://github.com/gravity-rose/PandoraRemote/releases/download/v1.4.0/PandoraRemote-1.4.0.apk)
2. After sideloading, grant the app restricted permissions and notification access. The app will walk you through this on first launch
3. Install the Pebble app from the store, or sideload from the releases page.
4. Start Pandora on your phone — the watch app launches automatically

## Developing/Building
if you want to contribute, please feel free to submit PRs.  

### Pebble App

```
cd pebble-app
pebble build
pebble install --emulator basalt
```

### Android Companion

```
cd android-companion
JAVA_HOME=/path/to/jdk-21 ./gradlew assembleDebug
```

The APK will be at `android-companion/app/build/outputs/apk/debug/app-debug.apk`.

## License

CC BY-NC-SA 4.0

See [https://creativecommons.org/licenses/by-nc-sa/4.0/](LICENSE) for details.
