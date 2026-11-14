if [ "$ARCH" != arm64 ] || [ "$API" -lt 26 ]; then
    abort "Requires an ARM64 device with Android 8.0 or newer."
fi

old_target="/data/adb/modules/$MODID/target.txt"
if [ "$old_target" != "$MODPATH/target.txt" ] && [ -f "$old_target" ]; then
    cp "$old_target" "$MODPATH/target.txt" || abort "Could not preserve target.txt."
fi
set_perm "$MODPATH/target.txt" 0 0 0644 u:object_r:system_file:s0
ui_print "- Set the target process in target.txt after reboot."
