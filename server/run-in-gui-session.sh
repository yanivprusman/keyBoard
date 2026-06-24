#!/usr/bin/env bash
#
# run-in-gui-session.sh — exec a command inside the active graphical session.
#
# The keyboard-server runs as a system service with no session environment, so
# GUI apps it launches on a G-key press (e.g. a terminal) have no display/bus to
# attach to. This wrapper discovers the active wayland/x11 session, exports its
# environment, then exec's the given command.
#
# Usage: run-in-gui-session.sh <command> [args...]
#
# No fallbacks: if the active graphical session can't be determined, it fails
# loudly rather than guessing a uid/display.
set -euo pipefail

# Find the active graphical (wayland or x11) login session.
sid=""
while read -r id _rest; do
    [ -n "$id" ] || continue
    state=$(loginctl show-session "$id" -p State --value 2>/dev/null || true)
    type=$(loginctl show-session "$id" -p Type --value 2>/dev/null || true)
    if [ "$state" = "active" ] && { [ "$type" = "wayland" ] || [ "$type" = "x11" ]; }; then
        sid="$id"
        break
    fi
done < <(loginctl list-sessions --no-legend 2>/dev/null)

if [ -z "$sid" ]; then
    echo "run-in-gui-session: no active graphical session found" >&2
    exit 1
fi

uid=$(loginctl show-session "$sid" -p User --value)
export XDG_RUNTIME_DIR="/run/user/${uid}"
export DBUS_SESSION_BUS_ADDRESS="unix:path=${XDG_RUNTIME_DIR}/bus"

# Pull the display vars the GUI client needs from the user's systemd manager
# (GNOME imports WAYLAND_DISPLAY/DISPLAY into it at session start).
env_block=$(systemctl --user show-environment 2>/dev/null || true)
wayland=$(printf '%s\n' "$env_block" | sed -n 's/^WAYLAND_DISPLAY=//p' | head -1)
display=$(printf '%s\n' "$env_block" | sed -n 's/^DISPLAY=//p' | head -1)
[ -n "$wayland" ] && export WAYLAND_DISPLAY="$wayland"
[ -n "$display" ] && export DISPLAY="$display"

exec "$@"
