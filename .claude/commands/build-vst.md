Build the HAL VST3 target and report any errors.

Run:
```
xcodebuild -project HAL/projects/HAL-macOS.xcodeproj -target VST3 -configuration Debug 2>&1 | grep -E "error:|warning: HAL|Build succeeded|BUILD FAILED"
```

VST3 installs automatically to `~/Library/Audio/Plug-Ins/VST3/HAL.vst3`.
If BUILD SUCCEEDED, confirm the path and suggest rescanning in the DAW.
If there are errors, show them and fix them.
