#!/usr/bin/env bash
# Makes a plain (unencrypted) SQLite copy of Gordon's Reloading Tool's database, for GRT Reloaded
# (File > Settings > Database > Original database).
#
# How: the Linux version of GRT is started under gdb on a scratch COPY of its folder. Right after it has unlocked
# its database (sqlite3_key) and issues its first SQL call, gdb calls the program's own sqlite3_rekey(db, NULL, 0),
# which removes the encryption from the copy. Nothing is compiled, GRT itself and its database stay unchanged.
#
# Works on Debian/Ubuntu/Mint, Fedora/RHEL clones, Arch/Manjaro and openSUSE. What is missing (gdb, the 32-bit
# GTK2 runtime GRT needs, a virtual display when there is no desktop) is installed after asking (sudo).
#
# Put this script into the folder of the unpacked GRT Linux download (next to the GordonsReloadingTool program,
# libs/ and GordonsReloadingTool.db) and run it there:  ./grt_decrypt_db.sh [options]
#   -o, --out FILE    where to write the plain database (default: GordonsReloadingTool.plain.db in that folder)
#   -y, --yes         install missing packages without asking
#   -c, --check       only report what is found and what is missing (installs and changes nothing)
#   --headless        use a virtual display even if a desktop is running (no window appears)
#   -t, --timeout S   give up after S seconds (default 90)
#   -h, --help        this text

set -u

# the GRT folder is the folder of this script
APP="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="$APP/GordonsReloadingTool.plain.db"
YES=0
CHECK=0
HEADLESS=0
SECS=90

say()  { printf '%s\n' "$*"; }
info() { printf '\033[1m==> %s\033[0m\n' "$*"; }
warn() { printf '\033[33mWARNING: %s\033[0m\n' "$*" >&2; }
die()  { printf '\033[31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }
usage() { sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'; exit 0; }

while [ $# -gt 0 ]; do
    case "$1" in
        -o|--out) [ $# -ge 2 ] || die "$1 needs a file name"; OUT="$2"; shift ;;
        -y|--yes) YES=1 ;;
        -c|--check) CHECK=1 ;;
        --headless) HEADLESS=1 ;;
        -t|--timeout) [ $# -ge 2 ] || die "$1 needs seconds"; SECS="$2"; shift ;;
        -h|--help) usage ;;
        *) die "unknown argument $1 (see --help)" ;;
    esac
    shift
done
case "$OUT" in /*) ;; *) OUT="$PWD/$OUT" ;; esac

# ---------------------------------------------------------------------------------------------------------------
# the GRT folder

is_app() { [ -f "$1/GordonsReloadingTool" ] && [ -d "$1/libs" ] && [ -f "$1/GordonsReloadingTool.db" ]; }

check_app() {
    is_app "$APP" || die "This script must be in the folder of Gordon's Reloading Tool (next to the GordonsReloadingTool
       program, libs/ and GordonsReloadingTool.db). It is in: $APP"
}

# ---------------------------------------------------------------------------------------------------------------
# what is missing

LOADER=/lib/ld-linux.so.2   # the 32-bit dynamic loader

missing_libs() {
    [ -x "$LOADER" ] || { echo "ld-linux.so.2 (32-bit runtime)"; return; }
    local f
    for f in "$APP/GordonsReloadingTool" "$APP"/libs/*.so; do
        # "libX.so => not found", or (it stops at the first one) "error while loading shared libraries: libX.so: ..."
        LD_LIBRARY_PATH="$APP/libs" "$LOADER" --list "$f" 2>&1 |
            sed -n -e 's/^[[:space:]]*\([^[:space:]]*\) => not found.*/\1/p' -e 's/.*error while loading shared libraries: \([^:]*\):.*/\1/p'
    done | sort -u
}

have_gdb() { command -v gdb >/dev/null && gdb -nx -batch -ex 'python print("py-ok")' 2>/dev/null | grep -q py-ok; }

need_virtual_display() { [ "$HEADLESS" = 1 ] || [ -z "${DISPLAY:-}" ]; }
have_xvfb() { command -v Xvfb >/dev/null; }

# ---------------------------------------------------------------------------------------------------------------
# the distribution and its packages

FAMILY=unknown
detect_distro() {
    local id="" like=""
    if [ -r /etc/os-release ]; then
        # shellcheck disable=SC1091
        . /etc/os-release
        id="${ID:-}"; like="${ID_LIKE:-}"
        DISTRO_NAME="${PRETTY_NAME:-$id}"
    fi
    case " $id $like " in
        *" debian "*|*" ubuntu "*) FAMILY=debian ;;
        *" fedora "*|*" rhel "*|*" centos "*) FAMILY=fedora ;;
        *" arch "*) FAMILY=arch ;;
        *" suse "*|*" opensuse "*|*opensuse*) FAMILY=suse ;;
    esac
}

SUDO=""
as_root() { if [ "$(id -u)" = 0 ]; then "$@"; else sudo "$@"; fi; }

# packages for: $1 = libs (32-bit GTK2 runtime), gdb, xvfb
packages_for() {
    case "$FAMILY:$1" in
        debian:libs)
            local gtk=libgtk2.0-0t64 atk=libatk1.0-0t64
            apt-cache show "$gtk:i386" >/dev/null 2>&1 || gtk=libgtk2.0-0   # before Debian 13 / Ubuntu 24.04
            apt-cache show "$atk:i386" >/dev/null 2>&1 || atk=libatk1.0-0
            echo "libc6:i386 $gtk:i386 $atk:i386 libxi6:i386 libxrandr2:i386 libxcursor1:i386 libxinerama1:i386" ;;
        debian:gdb) echo "gdb" ;;
        debian:xvfb) echo "xvfb" ;;
        fedora:libs) echo "glibc.i686 gtk2.i686 libXi.i686 libXrandr.i686 libXcursor.i686 libXinerama.i686" ;;
        fedora:gdb) echo "gdb" ;;
        fedora:xvfb) echo "xorg-x11-server-Xvfb" ;;
        arch:libs) echo "lib32-glibc lib32-gtk2 lib32-libxi lib32-libxrandr lib32-libxcursor lib32-libxinerama" ;;
        arch:gdb) echo "gdb" ;;
        arch:xvfb) echo "xorg-server-xvfb" ;;
        suse:libs) echo "glibc-32bit libgtk-2_0-0-32bit libXi6-32bit libXrandr2-32bit libXcursor1-32bit libXinerama1-32bit" ;;
        suse:gdb) echo "gdb" ;;
        suse:xvfb) echo "xorg-x11-server-Xvfb" ;;
    esac
}

install_packages() {
    local pkgs="$*"
    case "$FAMILY" in
        debian)
            if ! dpkg --print-foreign-architectures | grep -qx i386; then
                info "Enabling 32-bit packages (dpkg --add-architecture i386)"
                as_root dpkg --add-architecture i386 || die "could not enable the i386 architecture"
            fi
            as_root apt-get update || die "apt-get update failed"
            # the package names depend on the release: decide now that the i386 lists are there
            pkgs="$(for p in $pkgs; do [ "$p" = LIBS ] && packages_for libs || printf '%s ' "$p"; done)"
            # shellcheck disable=SC2086
            as_root apt-get install -y $pkgs ;;
        fedora)
            # shellcheck disable=SC2086
            as_root dnf install -y ${pkgs//LIBS/$(packages_for libs)} ;;
        arch)
            if [[ "$pkgs" == *LIBS* ]] && ! pacman -Sl multilib >/dev/null 2>&1; then
                die "32-bit packages need the [multilib] repository: enable it in /etc/pacman.conf, run 'sudo pacman -Sy', then start this script again"
            fi
            local libs
            libs="$(packages_for libs)"
            if [[ "$pkgs" == *LIBS* ]] && ! pacman -Si lib32-gtk2 >/dev/null 2>&1; then
                warn "lib32-gtk2 is not in the repositories; install it from the AUR (e.g. 'yay -S lib32-gtk2') and run this script again"
                libs="${libs/lib32-gtk2/}"
            fi
            # shellcheck disable=SC2086
            as_root pacman -S --needed --noconfirm ${pkgs//LIBS/$libs} ;;
        suse)
            # shellcheck disable=SC2086
            as_root zypper --non-interactive install ${pkgs//LIBS/$(packages_for libs)} ;;
        *) die "unknown distribution: install gdb (with Python), the 32-bit GTK2 runtime and Xvfb yourself" ;;
    esac
}

# ---------------------------------------------------------------------------------------------------------------

check_app
detect_distro
info "GRT folder:   $APP"
info "Distribution: ${DISTRO_NAME:-unknown} ($FAMILY)"

MISSING_LIBS="$(missing_libs)"
NEED=""
WHAT=""
if [ -n "$MISSING_LIBS" ]; then NEED="$NEED LIBS"; WHAT="$WHAT
  - 32-bit libraries GRT needs: $(echo "$MISSING_LIBS" | tr '\n' ' ')"; fi
if ! have_gdb; then NEED="$NEED $(packages_for gdb)"; WHAT="$WHAT
  - gdb with Python support"; fi
if need_virtual_display && ! have_xvfb; then NEED="$NEED $(packages_for xvfb)"; WHAT="$WHAT
  - Xvfb (a virtual display: $( [ "$HEADLESS" = 1 ] && echo 'asked for by --headless' || echo 'no desktop session found'))"; fi

if [ -n "$NEED" ]; then
    say "Missing:$WHAT"
    if [ "$FAMILY" = unknown ]; then
        die "unknown distribution: please install these yourself and run the script again"
    fi
    shown="${NEED//LIBS/$( [ "$FAMILY" = debian ] && echo 'the 32-bit GTK2 runtime (libgtk2.0-0…:i386 and the X11 libraries)' || packages_for libs)}"
    say "Packages to install:${shown}"
    if [ "$CHECK" = 1 ]; then exit 1; fi
    if [ "$YES" != 1 ]; then
        read -r -p "Install them now with sudo? [Y/n] " answer
        case "${answer:-y}" in [yY]*) ;; *) die "nothing installed" ;; esac
    fi
    install_packages $NEED
    MISSING_LIBS="$(missing_libs)"
    [ -z "$MISSING_LIBS" ] || die "still missing after the installation: $(echo "$MISSING_LIBS" | tr '\n' ' ')"
    have_gdb || die "gdb with Python support is still missing"
    if need_virtual_display && ! have_xvfb; then die "Xvfb is still missing"; fi
else
    say "Everything needed is installed."
fi
[ "$CHECK" = 1 ] && exit 0

# ---------------------------------------------------------------------------------------------------------------
# decrypt a scratch copy

WORK="$(mktemp -d)"
XVFB_PID=""
cleanup() {
    pkill -f "$WORK/GordonsReloadingTool" 2>/dev/null
    [ -n "$XVFB_PID" ] && kill "$XVFB_PID" 2>/dev/null
    rm -rf "$WORK"
}
trap cleanup EXIT

if need_virtual_display; then
    for n in $(seq 90 120); do [ -e "/tmp/.X11-unix/X$n" ] || [ -e "/tmp/.X$n-lock" ] || break; done
    Xvfb ":$n" -screen 0 1280x1024x24 -nolisten tcp >/dev/null 2>&1 &
    XVFB_PID=$!
    export DISPLAY=":$n"
    sleep 1
    kill -0 "$XVFB_PID" 2>/dev/null || die "Xvfb did not start"
    info "Using a virtual display ($DISPLAY)"
else
    info "Using the desktop's display ($DISPLAY): a GRT window may appear for a moment, just ignore it"
fi

info "Preparing a scratch copy of GRT"
cp "$APP/GordonsReloadingTool" "$WORK/"
cp "$APP/GordonsReloadingTool.cfg" "$WORK/" 2>/dev/null
cp -r "$APP/libs" "$WORK/"
for d in plugins locales; do [ -d "$APP/$d" ] && cp -r "$APP/$d" "$WORK/"; done
cp "$APP/GordonsReloadingTool.db" "$WORK/"
for f in "$WORK"/libs/*.so; do ln -sf "libs/$(basename "$f")" "$WORK/$(basename "$f")"; done
chmod +x "$WORK/GordonsReloadingTool"

# 32-bit cdecl at function entry: [esp]=ret, [esp+4]=arg1, +8=arg2, +12=arg3
cat > "$WORK/hook.py" <<'PYEOF'
import gdb, struct

st = {"db": None, "stopped": False}

def rd(a, n):
    return bytes(gdb.selected_inferior().read_memory(a, n))

def u32(a):
    return struct.unpack("<I", rd(a, 4))[0]

class KeyHook(gdb.Breakpoint):
    def __init__(self):
        super().__init__("*sqlite3_key", internal=True)
    def stop(self):
        esp = int(gdb.parse_and_eval("$esp")) & 0xffffffff
        st["db"] = u32(esp + 4)
        print("### sqlite3_key db=0x%x" % st["db"])
        return False

class SqlHook(gdb.Breakpoint):
    """Stops (once) at the first SQL call after the key was set."""
    def __init__(self, sym):
        super().__init__("*" + sym, internal=True)
        self.sym = sym
    def stop(self):
        if st["db"] and not st["stopped"]:
            st["stopped"] = True
            print("### stopping at first %s after key" % self.sym)
            return True
        return False

def install(ev):
    if (ev.new_objfile.filename or "").endswith("sqlite.so") and not st.get("hooked"):
        st["hooked"] = True
        KeyHook()
        for s in ("sqlite3_prepare_v2", "sqlite3_prepare", "sqlite3_prepare16_v2",
                  "sqlite3_exec", "sqlite3_get_table", "sqlite3_step"):
            try:
                SqlHook(s)
            except Exception as e:
                print("### cannot hook %s: %s" % (s, e))

gdb.events.new_objfile.connect(install)

def rekey():
    if not st["stopped"]:
        print("### FAILED: never reached a SQL call after sqlite3_key")
        return
    expr = "((int(*)(void*,const void*,int))sqlite3_rekey)((void*)%d,(void*)0,0)" % st["db"]
    print("### sqlite3_rekey rc=%s" % gdb.parse_and_eval(expr))
PYEOF

cat > "$WORK/gdbcmds" <<'GDBEOF'
set pagination off
set confirm off
set debuginfod enabled off
source hook.py
run
python rekey()
kill
GDBEOF

info "Running GRT under gdb (up to ${SECS}s)"
LOG="$WORK/gdb.log"
( cd "$WORK" && DEBUGINFOD_URLS="" timeout -k 5 "$SECS" gdb -nx -q -batch -x gdbcmds ./GordonsReloadingTool ) > "$LOG" 2>&1

if head -c 15 "$WORK/GordonsReloadingTool.db" | grep -q "SQLite format 3"; then
    mkdir -p "$(dirname "$OUT")"
    cp "$WORK/GordonsReloadingTool.db" "$OUT"
    if command -v python3 >/dev/null; then
        python3 - "$OUT" <<'PYEOF' || warn "the file was written but could not be read back"
import sqlite3, sys
c = sqlite3.connect("file:%s?mode=ro" % sys.argv[1], uri=True)
counts = {t: c.execute("select count(*) from %s where geloescht = 0" % t).fetchone()[0]
          for t in ("caliber", "projectile", "propellant")}
print("    %(caliber)d calibers, %(projectile)d projectiles, %(propellant)d propellants" % counts)
PYEOF
    fi
    info "Done: $OUT"
    say "Choose this file in GRT Reloaded: File > Settings > Database > Original database."
else
    REPORT="$APP/grt_decrypt_report.txt"
    { echo "=== grt_decrypt_db.sh $(date -Is) ==="; uname -a; echo "distribution: ${DISTRO_NAME:-?} ($FAMILY)";
      gdb --version | head -1; echo "--- gdb ---"; grep -v -iE "Gtk-|theme|^\[(New|Thread)" "$LOG"; } > "$REPORT"
    die "the database could not be decrypted. Details: $REPORT"
fi
