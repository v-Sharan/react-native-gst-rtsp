const { withAppBuildGradle, withMainApplication, withProjectBuildGradle, createRunOncePlugin } = require('@expo/config-plugins');

/**
 * Expo Config Plugin for react-native-gst-rtsp
 *
 * Automatically patches:
 *  1. android/build.gradle        → adds gstreamerRoot property
 *  2. android/app/build.gradle    → registers externalNativeBuild + NDK ABIs
 *  3. MainApplication.java/kt     → registers RNGstRtspPackage
 *
 * Usage in app.json / app.config.js:
 *   {
 *     "plugins": [
 *       ["react-native-gst-rtsp", { "gstreamerRoot": "/path/to/gstreamer-android" }]
 *     ]
 *   }
 */

// ─── 1. Root build.gradle — add gstreamerRoot ext property ────────────────
const withGstreamerRootGradle = (config, { gstreamerRoot }) => {
  return withProjectBuildGradle(config, (mod) => {
    const contents = mod.modResults.contents;
    const tag = '// react-native-gst-rtsp: gstreamerRoot';

    if (contents.includes(tag)) return mod;   // already patched

    const rootValue = gstreamerRoot
      ? `"${gstreamerRoot}"`
      : `System.getenv("GSTREAMER_ROOT_ANDROID") ?: ""`;

    const injection = `
${tag}
ext {
    gstreamerRoot = ${rootValue}
}
`;
    // Inject before the first "allprojects" block
    mod.modResults.contents = contents.replace(
      /allprojects\s*\{/,
      `${injection}\nallprojects {`
    );
    return mod;
  });
};

// ─── 2. App build.gradle — add NDK / externalNativeBuild ─────────────────
const withGstreamerAppGradle = (config) => {
  return withAppBuildGradle(config, (mod) => {
    const contents = mod.modResults.contents;
    const tag = '// react-native-gst-rtsp: ndk config';

    if (contents.includes(tag)) return mod;

    const ndkBlock = `
        ${tag}
        ndk {
            abiFilters "armeabi-v7a", "arm64-v8a", "x86", "x86_64"
        }
        externalNativeBuild {
            ndkBuild {
                arguments "GSTREAMER_ROOT_ANDROID=\${rootProject.ext.gstreamerRoot}"
            }
        }`;

    // Insert inside defaultConfig { ... }
    mod.modResults.contents = contents.replace(
      /(defaultConfig\s*\{)/,
      `$1${ndkBlock}`
    );

    // Add externalNativeBuild block after defaultConfig closing brace
    const externalBlock = `
    // react-native-gst-rtsp: external build
    externalNativeBuild {
        ndkBuild {
            path "\${project(':react-native-gst-rtsp').projectDir}/src/main/jni/Android.mk"
        }
    }
`;
    mod.modResults.contents = mod.modResults.contents.replace(
      /(compileOptions\s*\{)/,
      `${externalBlock}\n    $1`
    );

    return mod;
  });
};

// ─── 3. MainApplication — register RNGstRtspPackage ──────────────────────
const withGstreamerMainApplication = (config) => {
  return withMainApplication(config, (mod) => {
    const contents = mod.modResults.contents;
    const isKotlin = mod.modResults.language === 'kt';

    if (contents.includes('RNGstRtspPackage')) return mod;  // already added

    if (isKotlin) {
      mod.modResults.contents = contents
        .replace(
          /import expo\.modules\.ReactNativeHostWrapper/,
          `import expo.modules.ReactNativeHostWrapper\nimport com.gstrtsp.RNGstRtspPackage`
        )
        .replace(
          /PackageList\(this\)\.packages/,
          `PackageList(this).packages.apply { add(RNGstRtspPackage()) }`
        );
    } else {
      // Java
      mod.modResults.contents = contents
        .replace(
          /import com\.facebook\.react\.ReactNativeHost;/,
          `import com.facebook.react.ReactNativeHost;\nimport com.gstrtsp.RNGstRtspPackage;`
        )
        .replace(
          /new PackageList\(this\)\.getPackages\(\)/,
          `new PackageList(this).getPackages(); packages.add(new RNGstRtspPackage()); return packages`
        );

      // Fallback for older Expo templates
      if (!mod.modResults.contents.includes('RNGstRtspPackage')) {
        mod.modResults.contents = contents.replace(
          /protected List<ReactPackage> getPackages\(\) \{[\s\S]*?return packages;[\s\S]*?\}/,
          (match) => match.replace(
            'return packages;',
            'packages.add(new RNGstRtspPackage());\n      return packages;'
          )
        );
      }
    }

    return mod;
  });
};

// ─── Compose all patches ──────────────────────────────────────────────────
const withGstRtsp = (config, options = {}) => {
  const opts = { gstreamerRoot: null, ...options };

  config = withGstreamerRootGradle(config, opts);
  config = withGstreamerAppGradle(config);
  config = withGstreamerMainApplication(config);

  return config;
};

module.exports = createRunOncePlugin(withGstRtsp, 'react-native-gst-rtsp', '1.0.0');
