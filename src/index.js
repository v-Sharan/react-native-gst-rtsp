"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const react_1 = __importDefault(require("react"));
const react_native_1 = require("react-native");

const NativeView =
  react_native_1.Platform.OS === 'android'
    ? react_native_1.requireNativeComponent('RNGstRtspView')
    : null;

const GstRtspView = react_1.default.forwardRef((props, ref) => {
  const nativeRef = react_1.default.useRef(null);

  react_1.default.useImperativeHandle(ref, () => ({
    stop: () => {
      const node = react_native_1.findNodeHandle(nativeRef.current);
      if (node) react_native_1.UIManager.dispatchViewManagerCommand(node, 'stop', []);
    },
  }));

  if (react_native_1.Platform.OS !== 'android' || !NativeView) {
    return react_1.default.createElement(
      react_native_1.View,
      { style: [{ backgroundColor: '#111', alignItems: 'center', justifyContent: 'center' }, props.style] },
      react_1.default.createElement(react_native_1.Text, { style: { color: '#f55', fontSize: 13 } }, 'GstRtspView: Android only')
    );
  }

  return react_1.default.createElement(NativeView, Object.assign({ ref: nativeRef }, props));
});

GstRtspView.displayName = 'GstRtspView';
exports.default = GstRtspView;
