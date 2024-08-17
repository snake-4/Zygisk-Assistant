MODPATH="${0%/*}"
. $MODPATH/common_func.sh

### Conditional sensitive properties

# Magisk Recovery Mode
resetprop_if_match ro.boot.mode recovery unknown
resetprop_if_match ro.bootmode recovery unknown
resetprop_if_match vendor.boot.mode recovery unknown

# SELinux
resetprop_if_diff ro.boot.selinux enforcing
if [ -n "$(resetprop ro.build.selinux)" ]; then
    resetprop --delete ro.build.selinux
fi

# Toybox cat is used to preserve the file access time
if [ "$(toybox cat /sys/fs/selinux/enforce)" = "0" ]; then
    chmod 640 /sys/fs/selinux/enforce
    chmod 440 /sys/fs/selinux/policy
fi

### Conditional late sensitive properties

{
until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 1
done

# Avoid bootloop on some Xiaomi devices
resetprop_if_diff ro.secureboot.lockstate locked

# Avoid breaking Realme fingerprint scanners
resetprop_if_diff ro.boot.flash.locked 1
resetprop_if_diff ro.boot.realme.lockstate 1

# Avoid breaking Oppo fingerprint scanners
resetprop_if_diff ro.boot.vbmeta.device_state locked

# Avoid breaking OnePlus display modes/fingerprint scanners
resetprop_if_diff vendor.boot.verifiedbootstate green

# Avoid breaking OnePlus/Oppo fingerprint scanners on OOS/ColorOS 12+
resetprop_if_diff ro.boot.verifiedbootstate green
resetprop_if_diff ro.boot.veritymode enforcing
resetprop_if_diff vendor.boot.vbmeta.device_state locked

# Other
resetprop_if_diff sys.oem_unlock_allowed 0
resetprop_if_diff ro.adb.secure 1

}&
