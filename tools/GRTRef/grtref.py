#!/usr/bin/env python3
"""GRT reference runner: a GRT plugin that loads every *.grtload in ./jobs,
waits for the simulation and stores the full result (summary + all curve
points) as JSON in ./results. Standard library only.

GRT starts it as:  grtref.py --ipcfile <unix-socket-path>   (Linux)
                   grtref.py --ipcport <port>               (Windows)
Wire framing is not documented, so the reader is tolerant: it extracts JSON
objects from the byte stream by brace matching; commands are sent as one JSON
object followed by GRTREF_TERM (default "\n"; try "\\0" or "" if no answer).
"""
import json, os, shlex, socket, sys, time, traceback

HERE = os.path.dirname(os.path.abspath(__file__))
JOBS = os.path.join(HERE, "jobs")
OUT = os.path.join(HERE, "results")
LOG = os.path.join(HERE, "grtref.log")
TERM = os.environ.get("GRTREF_TERM", "\n").encode().decode("unicode_escape").encode()


def log(*a):
    line = time.strftime("%H:%M:%S ") + " ".join(str(x) for x in a)
    with open(LOG, "a", encoding="utf-8") as f:
        f.write(line + "\n")


class Link:
    def __init__(self, sock):
        self.s = sock
        self.buf = b""
        self.events = []

    def _extract(self):
        """Pop one complete JSON object from the buffer (brace matching)."""
        b = self.buf
        i = 0
        while i < len(b) and b[i:i + 1] != b"{":
            i += 1
        depth = 0
        instr = esc = False
        for j in range(i, len(b)):
            c = b[j:j + 1]
            if instr:
                if esc: esc = False
                elif c == b"\\": esc = True
                elif c == b'"': instr = False
            elif c == b'"': instr = True
            elif c == b"{": depth += 1
            elif c == b"}":
                depth -= 1
                if depth == 0:
                    raw, self.buf = b[i:j + 1], b[j + 1:]
                    return json.loads(raw.decode("utf-8", "replace"))
        return None

    def recv(self, timeout):
        end = time.time() + timeout
        while True:
            msg = self._extract()
            if msg is not None:
                return msg
            left = end - time.time()
            if left <= 0:
                return None
            self.s.settimeout(left)
            try:
                d = self.s.recv(1 << 20)
            except socket.timeout:
                return None
            if not d:
                raise ConnectionError("GRT closed the connection")
            self.buf += d

    def command(self, name, params, timeout=20.0, retries=2):
        payload = json.dumps({name: params}).encode() + TERM
        for _ in range(retries + 1):
            self.s.sendall(payload)
            end = time.time() + timeout
            while time.time() < end:
                msg = self.recv(max(0.1, end - time.time()))
                if msg is None:
                    break
                if "Result" in msg and msg["Result"].get("command") == name:
                    return msg["Result"]
                self.events.append(msg)  # an Event_* arrived meanwhile
        raise TimeoutError(f"no answer to {name}")

    def wait_event(self, name, timeout):
        for i, m in enumerate(self.events):
            if name in m:
                return self.events.pop(i)
        end = time.time() + timeout
        while time.time() < end:
            m = self.recv(end - time.time())
            if m is None:
                break
            if name in m:
                return m
            self.events.append(m)
        return None


def num(v):
    """'3425.28 bar' -> 3425.28 ; passthrough otherwise."""
    if isinstance(v, str):
        try:
            return float(v.split()[0])
        except (ValueError, IndexError):
            return v
    return v


def toint(v):
    return int(float(v))


def tobool(v):
    return v if isinstance(v, bool) else str(v).strip().lower() in ("true", "1", "yes")


def tofloat(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return v


def run_job(link, path):
    name = os.path.splitext(os.path.basename(path))[0]
    link.events.clear()
    r = link.command("Load_File", {"filepath": path})
    if r.get("status") != "success":
        log("Load_File refused, retrying once:", json.dumps(r)[:300])
        time.sleep(2.0)
        r = link.command("Load_File", {"filepath": path})
        if r.get("status") != "success":
            raise RuntimeError(f"Load_File failed: {r}")
    link.wait_event("Event_TabComputed", 15.0)   # may not fire if already computed
    time.sleep(1.0)
    handle, tabinfo, stale = None, {}, True
    want = os.path.basename(path).lower()
    for attempt in range(6):                      # the tab switch may be asynchronous
        top = link.command("Get_TabOnTop", {})
        tabinfo = top.get("values", {}) or {}
        if os.path.basename(str(tabinfo.get("file", "")).replace("\\", "/")).lower() == want:
            handle, stale = toint(tabinfo["tabhandle"]), False
            break
        log("tab on top is %r, expected %r (attempt %d)" % (tabinfo.get("file"), want, attempt + 1))
        time.sleep(1.5)
    if stale:                                     # fallback: look for the tab in the list
        try:
            lst = link.command("Get_TabList", {}).get("values") or []
            for t in lst if isinstance(lst, list) else []:
                if os.path.basename(str(t.get("file", "")).replace("\\", "/")).lower() == want:
                    handle, tabinfo, stale = toint(t["tabhandle"]), t, False
                    break
        except Exception as e:
            log("Get_TabList failed:", e)
    if handle is None:
        handle = toint(tabinfo.get("tabhandle", 0))
    # the simulation may still be running (the first load of a session is slow): poll until the
    # result is complete, i.e. it has curve points or the app flags the load as invalid
    for attempt in range(12):
        res = link.command("Get_TabResults", {"tabhandle": handle}, timeout=60.0)
        if res.get("status") != "success":
            raise RuntimeError(f"Get_TabResults failed: {res}")
        v = res["values"]
        data = v.get("data", {}) or {}
        complete = toint(v.get("chunkCount", 0)) > 0 or tobool(data.get("invalidData")) \
            or tobool(data.get("PropellantChargeTooHigh")) or tobool(data.get("PropellantChargeTooSmall"))
        if complete:
            break
        log("result of %s not complete yet (attempt %d), waiting" % (name, attempt + 1))
        time.sleep(2.0)
    log("Get_TabResults raw:", json.dumps(res)[:600])
    points = []
    if tobool(v.get("chunked")):
        h, n = v["chunkStreamHandle"], toint(v["chunkCount"])
        for i in range(n):
            c = link.command("Get_Chunk", {"chunkStreamHandle": str(h), "chunkIndex": str(i)}, timeout=60.0)
            if i == 0:
                log("Get_Chunk[0] raw:", json.dumps(c)[:400])
            points.extend({k: tofloat(x) for k, x in pt.items()} for pt in c["values"]["data"])
        link.command("Close_ChunkStream", {"chunkStreamHandle": str(h)})
    out = {
        "job": os.path.basename(path),
        "tab": tabinfo,
        "stale": stale,
        "summary": v.get("data", {}),
        "summary_num": {k: num(x) for k, x in v.get("data", {}).items() if not isinstance(x, dict)},
        "points": points,
    }
    with open(os.path.join(OUT, name + ".json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=1)
    log("STALE" if stale else "OK", name, len(points), "points", "tab=", tabinfo.get("file"))


def connect(args):
    """Connect to GRT, retrying for up to 25 s (the listener may not be ready yet)."""
    end = time.time() + 25
    last = None
    while time.time() < end:
        try:
            if "--ipcfile" in args:
                path = args[args.index("--ipcfile") + 1]
                log("ipcfile", path, "exists:", os.path.exists(path),
                    "mode:", oct(os.stat(path).st_mode) if os.path.exists(path) else "-")
                s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                s.connect(path)
                return s
            if "--ipcport" in args:
                return socket.create_connection(("127.0.0.1", int(args[args.index("--ipcport") + 1])))
            log("no --ipcfile/--ipcport in", args)
            return None
        except OSError as e:
            last = e
            log("connect failed:", e)
            time.sleep(0.5)
    log("giving up:", last)
    return None


def main():
    log("started; argv =", sys.argv, "cwd =", os.getcwd(), "python =", sys.version.split()[0])
    os.makedirs(OUT, exist_ok=True)
    # GRT passes e.g. one argv element: --ipcfile "/tmp/x.ipc" (quotes included)
    s = connect(shlex.split(" ".join(sys.argv[1:])))
    if s is None:
        return 1
    link = Link(s)
    log("connected, waiting for Event_Attached")
    ev = link.wait_event("Event_Attached", 10.0)
    log("attached:", ev)
    log("jobs dir:", JOBS, "exists:", os.path.isdir(JOBS))
    jobs = sorted(f for f in os.listdir(JOBS) if f.endswith(".grtload")) if os.path.isdir(JOBS) else []
    log("found %d job files" % len(jobs))
    n_ok = n_fail = 0
    for j in jobs:
        try:
            run_job(link, os.path.join(JOBS, j))
            n_ok += 1
        except Exception:
            n_fail += 1
            log("FAIL", j, traceback.format_exc())
    with open(os.path.join(OUT, "DONE"), "w") as f:
        f.write("%s jobs=%d ok=%d failed=%d\n" % (time.strftime("%F %T"), len(jobs), n_ok, n_fail))
    log("all jobs finished")
    try:
        while link.recv(60.0) is not None or True:   # stay attached, ignore events
            pass
    except Exception:
        pass
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except SystemExit:
        raise
    except BaseException:
        log("CRASH", traceback.format_exc())
        raise
