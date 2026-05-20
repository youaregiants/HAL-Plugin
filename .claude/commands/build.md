Build the HAL standalone APP target and report any errors.

Run:
```
xcodebuild -project HAL/projects/HAL-macOS.xcodeproj -target APP -configuration Debug 2>&1 | grep -E "error:|warning: HAL|Build succeeded|BUILD FAILED"
```

If BUILD SUCCEEDED, say so and remind the user to run `open /Users/christopherhlee/Applications/HAL.app` to see the result.

If there are errors, show them and fix them.
