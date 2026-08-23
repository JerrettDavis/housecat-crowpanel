Import("env")

from os.path import isdir, join

framework = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
if not framework:
    framework = join(env.subst("$PROJECT_PACKAGES_DIR"), "framework-arduinoespressif32")
if not isdir(framework):
    raise RuntimeError("Arduino ESP32 framework package is not installed")
env.Append(CPPPATH=[
    join(framework, "libraries", "Network", "src"),
    join(framework, "libraries", "FS", "src"),
])
