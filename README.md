# react-native-gst-rtsp

GStreamer-powered RTSP video player for React Native & Expo (Android).

Plays H.264 / H.265 streams from IP cameras, NVRs, and media servers with hardware-accelerated decoding via GStreamer's native pipeline.

---

## Install

```bash
# From GitHub (no npm publish needed)
npm install github:YOUR_USERNAME/react-native-gst-rtsp

# or with yarn
yarn add YOUR_USERNAME/react-native-gst-rtsp
```

### Prerequisites — GStreamer Android SDK

This package requires the GStreamer Android universal binaries. You only need to do this once per machine.

```bash
# Download (pick the latest 1.x release)
wget https://gstreamer.freedesktop.org/data/pkg/android/1.24.0/gstreamer-1.0-android-universal-1.24.0.tar.xz

# Extract anywhere — e.g. your home directory
mkdir -p ~/gstreamer-android
tar -xf gstreamer-1.0-android-universal-1.24.0.tar.xz -C ~/gstreamer-android/
```

---

## Setup

### Expo (Bare Workflow) — recommended

**1. Prebuild if you haven't already**
```bash
npx expo prebuild
```

**2. Add the config plugin to `app.json`**
```json
{
  "expo": {
    "plugins": [
      [
        "react-native-gst-rtsp",
        {
          "gstreamerRoot": "/Users/yourname/gstreamer-android/gstreamer-1.0-android-universal-1.24.0"
        }
      ]
    ]
  }
}
```

**3. Run**
```bash
npx expo run:android
```

That's it — the config plugin automatically patches `MainApplication.java`, `build.gradle`, and wires up the NDK build. No manual Java edits needed.

---

### Plain React Native

**1. Add GStreamer path to `android/gradle.properties`**
```properties
gstreamerRoot=/Users/yourname/gstreamer-android/gstreamer-1.0-android-universal-1.24.0
```

**2. Register the package in `android/app/src/main/java/.../MainApplication.java`**
```java
import com.gstrtsp.RNGstRtspPackage;   // ← add import

@Override
protected List<ReactPackage> getPackages() {
    return Arrays.<ReactPackage>asList(
        new MainReactPackage(),
        new RNGstRtspPackage()          // ← add this
    );
}
```

**3. Add permissions to `android/app/src/main/AndroidManifest.xml`**
```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
```

**4. Run**
```bash
npx react-native run-android
```

---

## Usage

```tsx
import GstRtspView from 'react-native-gst-rtsp';

export default function CameraScreen() {
  return (
    <GstRtspView
      url="rtsp://192.168.1.100:554/stream"
      style={{ flex: 1 }}
      onPlaying={() => console.log('▶ playing')}
      onStateChange={({ state }) => console.log('state:', state)}
      onError={({ error }) => console.error('error:', error)}
    />
  );
}
```

### Pause / resume

```tsx
const [paused, setPaused] = useState(false);

<GstRtspView
  url="rtsp://..."
  paused={paused}
  style={{ flex: 1 }}
/>

<Button title={paused ? 'Resume' : 'Pause'} onPress={() => setPaused(p => !p)} />
```

### Imperative stop via ref

```tsx
import GstRtspView, { GstRtspViewRef } from 'react-native-gst-rtsp';

const ref = useRef<GstRtspViewRef>(null);

<GstRtspView ref={ref} url="rtsp://..." style={{ flex: 1 }} />
<Button title="Stop" onPress={() => ref.current?.stop()} />
```

---

## Props

| Prop | Type | Default | Description |
|------|------|---------|-------------|
| `url` | `string` | **required** | RTSP stream URL |
| `paused` | `boolean` | `false` | Pause/resume |
| `latency` | `number` | `200` | Jitter buffer (ms) |
| `onPlaying` | `() => void` | — | First frame decoded |
| `onStateChange` | `({state}) => void` | — | Pipeline state changed |
| `onError` | `({error}) => void` | — | Pipeline error |
| `style` | `ViewStyle` | — | Layout |

### States

`'stopped'` → `'ready'` → `'paused'` → `'playing'`

---

## Supported formats

| Video | Audio |
|-------|-------|
| H.264 | AAC (MPEG4-GENERIC, MP4A-LATM) |
| H.265 | G.711 µ-law (PCMU) |

Transport: UDP / UDP-Multicast / TCP — auto-negotiated.

---

## Latency tuning

```tsx
<GstRtspView latency={50}   .../>  {/* LAN camera, ultra low latency */}
<GstRtspView latency={200}  .../>  {/* default */}
<GstRtspView latency={1000} .../>  {/* poor WAN / cellular */}
```

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `RNGstRtspView was not found` | Add `RNGstRtspPackage` to `MainApplication.java` and rebuild |
| Build fails: `GSTREAMER_ROOT_ANDROID not defined` | Set `gstreamerRoot` in `gradle.properties` or config plugin options |
| Black screen, no error | Ensure `url` prop is set and the surface has non-zero dimensions |
| `System.loadLibrary` crash | ABI mismatch — confirm device ABI is in `abiFilters` in `Application.mk` |
| High latency on LAN camera | Try `latency={50}` |

---

## Project structure

```
react-native-gst-rtsp/
├── app.plugin.js              ← Expo config plugin entry
├── plugin/
│   └── index.js               ← Config plugin implementation
├── src/
│   ├── GstRtspView.tsx        ← React component (TypeScript source)
│   ├── index.js               ← Compiled JS (no build step needed)
│   ├── index.ts               ← TS re-export
│   └── index.d.ts             ← Type declarations
├── android/
│   ├── build.gradle
│   └── src/main/
│       ├── jni/
│       │   ├── gst_rtsp_jni.c     ← GStreamer pipeline + JNI bridge
│       │   ├── Android.mk         ← NDK build + plugin selection
│       │   └── Application.mk     ← ABI / STL settings
│       └── java/com/gstrtsp/
│           ├── RNGstRtspView.java         ← SurfaceView
│           ├── RNGstRtspViewManager.java  ← ViewManager
│           └── RNGstRtspPackage.java      ← ReactPackage
└── package.json
```

---

## License

MIT
