import React from 'react';
import { ViewStyle } from 'react-native';

export type GstState = 'playing' | 'paused' | 'ready' | 'stopped';

export interface GstRtspViewProps {
  url: string;
  paused?: boolean;
  latency?: number;
  onPlaying?: () => void;
  onStateChange?: (event: { state: GstState }) => void;
  onError?: (event: { error: string }) => void;
  style?: ViewStyle;
}

export interface GstRtspViewRef {
  stop: () => void;
}

declare const GstRtspView: React.ForwardRefExoticComponent<
  GstRtspViewProps & React.RefAttributes<GstRtspViewRef>
>;

export default GstRtspView;
