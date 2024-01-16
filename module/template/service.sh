#!/system/bin/sh

MODDIR="${0%/*}"

alias resetprop="/data/adb/ap/bin/resetprop"

# resetprop_hexpatch [-f|--force] <prop name> <new value>
resetprop_hexpatch() {
    case "$1" in
        -f|--force) local FORCE=1; shift;;
    esac

    local NAME="$1"
    local NEWVALUE="$2"
    local CURVALUE="$(resetprop "$NAME")"

    [ ! "$NEWVALUE" -o ! "$CURVALUE" ] && return 1
    [ "$NEWVALUE" = "$CURVALUE" -a ! "$FORCE" ] && return 1

    local NEWLEN=${#NEWVALUE}
    if [ -f /dev/__properties__ ]; then
        local PROPFILE=/dev/__properties__
    else
        local PROPFILE="/dev/__properties__/$(resetprop -Z "$NAME")"
    fi
    local NAMEOFFSET=$(echo $(strings -t d "$PROPFILE" | grep "$NAME") | cut -d ' ' -f 1)

    #<hex 2-byte change counter><flags byte><hex length of prop value><prop value + nul padding to 92 bytes><prop name>
    local NEWHEX="$(printf '%02x' "$NEWLEN")$(printf "$NEWVALUE" | od -A n -t x1 -v | tr -d ' \n')$(printf "%$((92-NEWLEN))s" | sed 's/ /00/g')"

    printf "Patch '$NAME' to '$NEWVALUE' in '$PROPFILE' @ 0x%08x -> \n[0000??$NEWHEX]\n" $((NAMEOFFSET-96))

    echo -ne "\x00\x00" \
        | dd obs=1 count=2 seek=$((NAMEOFFSET-96)) conv=notrunc of="$PROPFILE"
    echo -ne "$(printf "$NEWHEX" | sed -e 's/.\{2\}/&\\x/g' -e 's/^/\\x/' -e 's/\\x$//')" \
        | dd obs=1 count=93 seek=$((NAMEOFFSET-93)) conv=notrunc of="$PROPFILE"
}

# resetprop_if_diff <prop name> <expected value>
resetprop_if_diff() {
    local NAME="$1"
    local EXPECTED="$2"
    local CURRENT="$(resetprop "$NAME")"

    [ -z "$CURRENT" ] || [ "$CURRENT" == "$EXPECTED" ] || resetprop_hexpatch "$NAME" "$EXPECTED"
}

# resetprop_if_match <prop name> <value match string> <new value>
resetprop_if_match() {
    local NAME="$1"
    local CONTAINS="$2"
    local VALUE="$3"

    [[ "$(resetprop "$NAME")" == *"$CONTAINS"* ]] && resetprop_hexpatch "$NAME" "$VALUE"
}

{
  # Reset props after boot completed to avoid breaking some weird devices/ROMs...
  while [ "$(getprop sys.boot_completed)" != "1" ]
  do
    sleep 1
  done

  # Xiaomi cross region flash...
  # See https://github.com/topjohnwu/Magisk/pull/2470
  resetprop_if_match ro.boot.hwc CN GLOBAL
  resetprop_if_match ro.boot.hwcountry China GLOBAL

  # some stupid banking apps check this prop
  resetprop_if_diff sys.oem_unlock_allowed 0

  # Load MagiskHide props
  resetprop -n --file "$MODDIR/hide.prop"

  # Hide that we booted from recovery when magisk is in recovery mode
  resetprop_if_match "ro.bootmode" "recovery" "unknown"
  resetprop_if_match "ro.boot.bootmode" "recovery" "unknown"
  resetprop_if_match "vendor.boot.bootmode" "recovery" "unknown"

  resetprop --delete ro.build.selinux

}&