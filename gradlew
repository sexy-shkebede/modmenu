#!/bin/sh

# Gradle wrapper launcher. The wrapper JAR is intentionally not bundled in this
# source archive; Android Studio can use its configured Gradle distribution.
# If you need ./gradlew from a terminal, run `gradle wrapper` once after
# installing Gradle 8.9 to generate gradle/wrapper/gradle-wrapper.jar.

if command -v gradle >/dev/null 2>&1; then
  exec gradle "$@"
fi

echo "Gradle is not installed on PATH. Open this project in Android Studio and sync it, or install Gradle 8.9 and run: gradle wrapper" >&2
exit 1
