#!/usr/bin/env bash
#
# run-in-gui-session.sh — thin wrapper, kept so existing G-key bindings keep
# working. The implementation now lives in automateLinux so every launcher in the
# suite (keyboard-server, dashboard, daemon) shares ONE way of opening a GUI app
# from a system service.
#
# Discovering the session environment, which is all this script used to do, was
# only half the job. A process spawned by a service also stays in that service's
# cgroup, and systemd's default KillMode=control-group kills the whole cgroup on
# restart. Ptyxis is single-instance, so whichever service happens to fork its
# singleton agent owns every terminal window on the desktop — meaning a
# keyboard-server restart could close all of them. That is not theoretical: on
# 2026-07-30 a dashboard restart did exactly that and orphaned two live Claude
# sessions.
#
# Usage: run-in-gui-session.sh <command> [args...]
#
# No fallbacks: the shared script fails loudly if the graphical session or the
# user manager can't be reached.
set -euo pipefail

exec /opt/automateLinux/utilities/runInGuiSession.sh "$@"
