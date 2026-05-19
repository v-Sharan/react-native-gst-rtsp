import React, { forwardRef, useRef, useImperativeHandle } from 'react';
import {
  requireNativeComponent,
  UIManager,
  findNodeHandle,
  Platform,
  View,
  Text,
  StyleSheet,
  ViewStyle,
} from 'react-native';

/* ─── Types ─────────────────────────────────────────────────────────────── */

export type GstState = 'playing' | 'paused' | 'ready' | 'stopped';

export interface GstRtspViewProps {
  /** RTSP stream URL e.g. rtsp://user:pass@192.168.1.1:554/stream */
  url: string;
  /** Pause/resume without tearing down the pipeline */
  paused?: boolean;
  /** Jitter-buffer latency ms. Lower = less delay. Default: 200 */
  latency?: number;
  /** Called when the first video frame is rendered */
  onPlaying?: () => void;
  /** Called when the GStreamer pipeline state changes */
  onStateChange?: (event: { state: GstState }) => void;
  /** Called on pipeline errors */
  onError?: (event: { error: string }) => void;
  style?: ViewStyle;
}

export interface GstRtspViewRef {
  /** Hard-stop the pipeline */
  stop: () => void;
}

/* ─── Native component ───────────────────────────────────────────────────── */

const NativeView =
  Platform.OS === 'android'
    ? requireNativeComponent<GstRtspViewProps>('RNGstRtspView')
    : null;

/* ─── Exported component ─────────────────────────────────────────────────── */

const GstRtspView = forwardRef<GstRtspViewRef, GstRtspViewProps>((props, ref) => {
  const nativeRef = useRef<any>(null);

  useImperativeHandle(ref, () => ({
    stop: () => {
      const node = findNodeHandle(nativeRef.current);
      if (node) UIManager.dispatchViewManagerCommand(node, 'stop', []);
    },
  }));

  if (Platform.OS !== 'android' || !NativeView) {
    return (
      <View style={[styles.fallback, props.style]}>
        <Text style={styles.fallbackText}>GstRtspView: Android only</Text>
      </View>
    );
  }

  return <NativeView ref={nativeRef} {...props} />;
});

GstRtspView.displayName = 'GstRtspView';

export default GstRtspView;

const styles = StyleSheet.create({
  fallback: { backgroundColor: '#111', alignItems: 'center', justifyContent: 'center' },
  fallbackText: { color: '#f55', fontSize: 13 },
});
