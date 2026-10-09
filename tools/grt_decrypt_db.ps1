<#
Makes a plain (unencrypted) SQLite copy of Gordon's Reloading Tool's database, for ChamberSim Reloaded
(File > Settings > Database > Original database). Windows counterpart of grt_decrypt_db.sh.

How: a scratch COPY of GRT is started with Frida (a tool to hook functions of a running program). When GRT has
unlocked its database (its SQLite library's sqlite3_key returns), the script calls GRT's own
sqlite3_rekey(db, NULL, 0), which removes the encryption from the copy. GRT itself and its database stay unchanged.
The two functions are found in GRT's libs\sqlite.dll by their machine code (the DLL does not name them).

Put this script (and grt_decrypt_db.cmd) into the folder of GRT (next to GordonsReloadingTool.exe, libs\ and
GordonsReloadingTool.db) and double-click grt_decrypt_db.cmd, or run in PowerShell:
    powershell -ExecutionPolicy Bypass -File .\grt_decrypt_db.ps1 [-Out FILE] [-Yes] [-Check] [-Timeout SECONDS]

Needs Python 3 and the Python package "frida"; both are installed after asking (Python with winget, frida with pip
into a private environment in %LOCALAPPDATA%\ChamberSimReloaded\decrypt-venv).
#>
param(
    [string]$Out = "",
    [switch]$Yes,
    [switch]$Check,
    [int]$Timeout = 90
)

$ErrorActionPreference = "Stop"
function Info($t) { Write-Host "==> $t" -ForegroundColor Cyan }
function Warn($t) { Write-Host "WARNING: $t" -ForegroundColor Yellow }
function Fail($t) { Write-Host "ERROR: $t" -ForegroundColor Red; exit 1 }
function Ask($q) {
    if ($Yes) { return $true }
    $a = Read-Host "$q [Y/n]"
    return ($a -eq "" -or $a -match "^[yYjJ]")
}

# ---- the GRT folder is the folder of this script ----------------------------------------------------------------
$App = $PSScriptRoot
foreach ($f in "GordonsReloadingTool.exe", "libs\sqlite.dll", "GordonsReloadingTool.db") {
    if (-not (Test-Path (Join-Path $App $f))) {
        Fail "This script must be in the folder of Gordon's Reloading Tool (next to GordonsReloadingTool.exe, libs\ and GordonsReloadingTool.db). It is in: $App"
    }
}
if ($Out -eq "") { $Out = Join-Path $App "GordonsReloadingTool.plain.db" }
$Out = [System.IO.Path]::GetFullPath($Out)
Info "GRT folder: $App"

# ---- Python ------------------------------------------------------------------------------------------------------
# The Python must be a normal build: frida has no wheel for the free-threaded builds (python3.13t, 3.14t, "py -3.14t"),
# pip then tries to build it and stops with "py_limited_api is incompatible with py_gil_disabled".
$UsablePython = "import sys, sysconfig; print('free' if sysconfig.get_config_var('Py_GIL_DISABLED') else sys.version_info[0])"

# returns @{ Exe = ...; Args = @(...) } for the first usable Python 3 found (newer versions may lack a frida wheel: 3.13 and 3.12 first)
function Find-Python {
    foreach ($cand in @(@{ Exe = "py"; Args = @("-3.13") }, @{ Exe = "py"; Args = @("-3.12") }, @{ Exe = "py"; Args = @("-3.11") },
                        @{ Exe = "py"; Args = @("-3") }, @{ Exe = "python"; Args = @() }, @{ Exe = "python3"; Args = @() })) {
        $cmd = Get-Command $cand.Exe -ErrorAction SilentlyContinue
        if (-not $cmd) { continue }
        if ($cmd.Source -like "*\WindowsApps\*") { continue }   # the Microsoft Store placeholder, not Python
        try {
            $pyArgs = $cand.Args
            $v = & $cand.Exe @pyArgs -c $UsablePython 2>$null
            if ($v -eq "3") { return $cand }
        } catch { }
    }
    return $null
}

$Py = Find-Python
$Venv = Join-Path $env:LOCALAPPDATA "ChamberSimReloaded\decrypt-venv"
$VenvPy = Join-Path $Venv "Scripts\python.exe"
# an environment made by an earlier run with a free-threaded Python cannot get frida: start again
if ((Test-Path $VenvPy) -and ((& $VenvPy -c $UsablePython 2>$null) -ne "3")) {
    Warn "The private Python environment uses a free-threaded Python; creating it again"
    Remove-Item $Venv -Recurse -Force -ErrorAction SilentlyContinue
}
$HaveFrida = (Test-Path $VenvPy) -and ((& $VenvPy -c "import frida; print('ok')" 2>$null) -eq "ok")

$Missing = @()
if (-not $Py -and -not (Test-Path $VenvPy)) { $Missing += "Python 3 (with winget: Python.Python.3.12)" }
if (-not $HaveFrida) { $Missing += "the Python package frida (with pip, into $Venv)" }
if ($Missing.Count -gt 0) {
    Write-Host "Missing:"; $Missing | ForEach-Object { Write-Host "  - $_" }
    if ($Check) { exit 1 }
    if (-not (Ask "Install now?")) { Fail "nothing installed" }
    if (-not $Py -and -not (Test-Path $VenvPy)) {
        if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
            Fail "winget is not available. Install Python 3 from https://www.python.org/downloads/ and run this script again."
        }
        Info "Installing Python"
        winget install -e --id Python.Python.3.12 --scope user --accept-package-agreements --accept-source-agreements
        $env:Path = [Environment]::GetEnvironmentVariable("Path", "User") + ";" + [Environment]::GetEnvironmentVariable("Path", "Machine")
        $Py = Find-Python
        if (-not $Py) { Fail "Python was installed but cannot be found yet: open a new window and run this script again." }
    }
    if (-not (Test-Path $VenvPy)) {
        Info "Creating a private Python environment: $Venv"
        $pyArgs = $Py.Args
        & $Py.Exe @pyArgs -m venv $Venv
        if (-not (Test-Path $VenvPy)) { Fail "could not create $Venv" }
    }
    Info "Installing frida"
    & $VenvPy -m pip install --disable-pip-version-check --quiet --upgrade frida
    if ((& $VenvPy -c "import frida; print('ok')" 2>$null) -ne "ok") { Fail "frida could not be installed (see the messages above)" }
} else {
    Write-Host "Everything needed is installed."
}
if ($Check) { exit 0 }

# ---- decrypt a scratch copy ----------------------------------------------------------------------------------------
$Work = Join-Path ([System.IO.Path]::GetTempPath()) ("grt-decrypt-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $Work | Out-Null
try {
    Info "Preparing a scratch copy of GRT"
    Copy-Item (Join-Path $App "GordonsReloadingTool.exe") $Work
    Copy-Item (Join-Path $App "GordonsReloadingTool.db") $Work
    foreach ($f in "GordonsReloadingTool.cfg", "GordonsReloadingTool.dbempty") {
        if (Test-Path (Join-Path $App $f)) { Copy-Item (Join-Path $App $f) $Work }
    }
    Copy-Item (Join-Path $App "libs") (Join-Path $Work "libs") -Recurse
    if (Test-Path (Join-Path $App "locales")) { Copy-Item (Join-Path $App "locales") (Join-Path $Work "locales") -Recurse }

    # the hook, run inside GRT by Frida
    @'
const KEY = "55 8B EC 51 83 3D ?? ?? ?? ?? 00 53 56 57 8B F0 75 0B B8 15 00 00 00";
const REKEY = "55 8B EC 83 E4 F8 83 EC 1C 83 3D ?? ?? ?? ?? 00 53 56 57 75 12 8B 4D 08 B8 15 00 00 00 89 41 2C";
// the scratch copy's folder name (unique): only its database is decrypted
const WORKDIR = "__WORKDIR__".toLowerCase();
let hooked = false;
let lastDb = "";
let done = false;

function moduleNamed(name) {
    return Process.enumerateModules().find(m => m.name.toLowerCase() === name) || null;
}
function exportOf(mod, fn) {
    return mod.findExportByName ? mod.findExportByName(fn) : Module.findExportByName(mod.name, fn);
}

function findOne(mod, pattern, what) {
    const hits = Memory.scanSync(mod.base, mod.size, pattern);
    if (hits.length !== 1) throw new Error(what + " found " + hits.length + " times in sqlite.dll (another GRT version?)");
    return hits[0].address;
}

function hookSqlite() {
    const mod = moduleNamed("sqlite.dll");
    if (!mod || hooked) return;
    hooked = true;
    try {
        const key = findOne(mod, KEY, "sqlite3_key");
        const rekeyAddr = findOne(mod, REKEY, "sqlite3_rekey");
        // both test the same "encryption enabled" flag: a check that the right pair was found
        if (!key.add(6).readPointer().equals(rekeyAddr.add(11).readPointer()))
            throw new Error("sqlite3_key and sqlite3_rekey do not belong together");
        const rekey = new NativeFunction(rekeyAddr, "int", ["pointer", "pointer", "int"], "mscdecl");
        send({log: "sqlite.dll at " + mod.base + ", hooks installed"});
        // sqlite3_key gets the database in EAX (an internal calling convention), the key on the stack
        Interceptor.attach(key, {
            onEnter() { this.db = this.context.eax; },
            onLeave(rc) {
                if (done) return;
                if (rc.toInt32() !== 0) { send({log: "sqlite3_key failed: " + rc.toInt32()}); return; }
                if (!lastDb.toLowerCase().includes("\\" + WORKDIR + "\\")) { send({log: "not the copy, left alone: " + lastDb}); return; }
                done = true;
                send({log: "unlocked: " + lastDb});
                send({rekey: rekey(this.db, ptr(0), 0)});
            }
        });
    } catch (e) {
        send({error: e.message});
    }
}

// sqlite.dll is loaded by GRT at startup: hook it as soon as it is there; and note which .db file is opened
const kernel = moduleNamed("kernelbase.dll") || moduleNamed("kernel32.dll");
for (const fn of ["LoadLibraryExW", "LoadLibraryW", "LoadLibraryExA", "LoadLibraryA"]) {
    const addr = exportOf(kernel, fn);
    if (addr) Interceptor.attach(addr, { onLeave() { hookSqlite(); } });
}
const createFile = exportOf(kernel, "CreateFileW");
Interceptor.attach(createFile, {
    onEnter(args) {
        const path = args[0].isNull() ? "" : args[0].readUtf16String();
        if (/\.db$/i.test(path)) lastDb = path.replace(/^\\\\\?\\/, "");
    }
});
hookSqlite();
'@ | Set-Content -Encoding UTF8 (Join-Path $Work "hook.js")

    # the driver: starts GRT, waits for the result, ends GRT
    @'
import frida, sys, threading, time
import json, os
exe, script_path, timeout = sys.argv[1], sys.argv[2], float(sys.argv[3])
workdir = os.path.basename(os.path.dirname(os.path.abspath(exe)))   # grt-decrypt-<guid>
done = threading.Event()
result = {"rc": None, "error": None}

def on_message(msg, data):
    if msg["type"] == "send":
        p = msg["payload"]
        if "log" in p: print("   " + p["log"], flush=True)
        if "error" in p: result["error"] = p["error"]; done.set()
        if "rekey" in p:
            result["rc"] = p["rekey"]
            if p["rekey"] == 0: done.set()
            else: print("   sqlite3_rekey returned %d" % p["rekey"], flush=True)
    elif msg["type"] == "error":
        result["error"] = msg.get("description"); done.set()

pid = frida.spawn([exe])
session = frida.attach(pid)
source = open(script_path, encoding="utf-8-sig").read()
source = source.replace('"__WORKDIR__"', json.dumps(workdir))
script = session.create_script(source)
script.on("message", on_message)
script.load()
frida.resume(pid)
done.wait(timeout)
time.sleep(2)          # let SQLite finish writing
try:
    frida.kill(pid)
except Exception:
    pass
if result["error"]:
    print("ERROR " + result["error"]); sys.exit(2)
if result["rc"] != 0:
    print("ERROR no successful sqlite3_rekey (rc=%s)" % result["rc"]); sys.exit(3)
'@ | Set-Content -Encoding UTF8 (Join-Path $Work "drive.py")

    Info "Running GRT (up to $Timeout s); its window may appear for a moment, just ignore it"
    Push-Location $Work
    try {
        & $VenvPy (Join-Path $Work "drive.py") (Join-Path $Work "GordonsReloadingTool.exe") (Join-Path $Work "hook.js") $Timeout
        $rc = $LASTEXITCODE
    } finally { Pop-Location }

    $db = Join-Path $Work "GordonsReloadingTool.db"
    $bytes = New-Object byte[] 15
    $fs = [System.IO.File]::OpenRead($db); [void]$fs.Read($bytes, 0, 15); $fs.Close()
    $head = [System.Text.Encoding]::ASCII.GetString($bytes)
    if ($rc -ne 0 -or $head -ne "SQLite format 3") {
        Fail "the database could not be decrypted (exit code $rc). Please send the messages above."
    }
    Copy-Item $db $Out -Force
    @'
import sqlite3, sys
c = sqlite3.connect(sys.argv[1])
n = {t: c.execute("select count(*) from %s where geloescht = 0" % t).fetchone()[0] for t in ("caliber", "projectile", "propellant")}
print("    %(caliber)d calibers, %(projectile)d projectiles, %(propellant)d propellants" % n)
'@ | Set-Content -Encoding UTF8 (Join-Path $Work "count.py")
    & $VenvPy (Join-Path $Work "count.py") $Out
    Info "Done: $Out"
    Write-Host "Choose this file in ChamberSim Reloaded: File > Settings > Database > Original database."
} finally {
    Get-Process GordonsReloadingTool -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$Work*" } | Stop-Process -Force
    Remove-Item $Work -Recurse -Force -ErrorAction SilentlyContinue
}
