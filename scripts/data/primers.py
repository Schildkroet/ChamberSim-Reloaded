"""Primer types of the program and the primer products of the published load tables.

The program knows the primer by type (src/core/primer.h): SP, SPM, LP, LPM (small / large pistol, magnum), SR, SRM, LR, LRM (rifle) and
BMG (.50 BMG). classify() maps the texts of the load tables ("CCI 450", "FED 205M, SR Match", "Large Rifle Magnum, Federal 215",
"REM 9 1/2") to a type, or None. Federal's suffix M is Match (GM210M is a standard large rifle primer), not magnum.
"""
import re

TYPES = ("SP", "SPM", "LP", "LPM", "SR", "SRM", "LR", "LRM", "BMG")

# product numbers per maker (the text is upper case, without spaces between number and fraction)
PRODUCTS = {
    "CCI": {"500": "SP", "550": "SPM", "300": "LP", "350": "LPM", "400": "SR", "41": "SR", "BR4": "SR", "450": "SRM",
            "200": "LR", "34": "LR", "BR2": "LR", "250": "LRM", "35": "BMG"},
    "FED": {"100": "SP", "100M": "SP", "200": "SPM", "150": "LP", "150M": "LP", "155": "LPM", "155M": "LPM", "205": "SR",
            "205M": "SR", "205MAR": "SR", "210": "LR", "210M": "LR", "215": "LRM", "215M": "LRM"},
    "REM": {"1½": "SP", "11/2": "SP", "1.5": "SP", "1": "SP", "5½": "SPM", "51/2": "SPM", "5": "SPM", "2½": "LP", "21/2": "LP",
            "2": "LP", "6½": "SR", "61/2": "SR", "6": "SR", "7½": "SR", "71/2": "SR", "7": "SR", "9½": "LR", "91/2": "LR",
            "9": "LR", "9½M": "LRM", "91/2M": "LRM", "9M": "LRM"},
    "WIN": {"SP": "SP", "WSP": "SP", "SPM": "SPM", "WSPM": "SPM", "LP": "LP", "WLP": "LP", "LPM": "LPM", "WLPM": "LPM",
            "SR": "SR", "WSR": "SR", "LR": "LR", "WLR": "LR", "LRM": "LRM", "WLRM": "LRM"},
}
MAKER = {"CCI": "CCI", "FED": "FED", "FEDERAL": "FED", "GM": "FED", "REM": "REM", "REMINGTON": "REM", "WIN": "WIN",
         "WINCHESTER": "WIN"}
WORDS = [  # size words: "Large Rifle Magnum", "SR Match", "small pistol"
    (r"\bSMALL\s+PISTOL\s+MAG", "SPM"), (r"\bLARGE\s+PISTOL\s+MAG", "LPM"), (r"\bSMALL\s+RIFLE\s+MAG", "SRM"),
    (r"\bLARGE\s+RIFLE\s+MAG", "LRM"), (r"\bSMALL\s+PISTOL", "SP"), (r"\bLARGE\s+PISTOL", "LP"), (r"\bSMALL\s+RIFLE", "SR"),
    (r"\bLARGE\s+RIFLE", "LR"), (r"\b50\s*BMG\b", "BMG"), (r"\b(SPM|LPM|SRM|LRM)\b", None), (r"\b(SP|LP|SR|LR)\b", None),
]


def product(text: str) -> str | None:
    """The type of a named product ("CCI 450", "Fed. 215M", "GM210M", "WLR", "REM 9 1/2"), else None."""
    t = text.upper().replace(".", " ").replace("#", " ")
    t = re.sub(r"(\d)\s+(1/2|½)", r"\1\2", t)  # "9 1/2" -> "91/2"
    m = re.match(r"\s*(GM)(\d+M?)\b", t)  # Federal Gold Medal: GM210M
    if m: return PRODUCTS["FED"].get(m[2])
    m = re.match(r"\s*(W[SL][PR]M?)\b", t)  # Winchester WLR, WSPM
    if m: return PRODUCTS["WIN"].get(m[1])
    m = re.match(r"\s*([A-Z]+)\s*-?\s*([0-9A-Z½/]+(?:\s*M\b)?)", t)
    if not m or m[1] not in MAKER: return None
    code = m[2].replace(" ", "")
    return PRODUCTS[MAKER[m[1]]].get(code)


def classify(text: str) -> str | None:
    """Primer type of a load table's primer text, else None. A named product wins over size words; a "Mag" anywhere in the text makes it the
    magnum type ("WIN LR Magnum", and "REM 7 1/2, SR Magnum" as Hodgdon labels it)."""
    if not text: return None
    for part in re.split(r"[,;()]", text):
        if (p := product(part)):
            return p + "M" if p in ("SP", "LP", "SR", "LR") and re.search(r"\bMAG(NUM)?\b", text.upper()) else p
    t = text.upper()
    for pat, code in WORDS:
        m = re.search(pat, t)
        if m: return code or m[1]
    return None


if __name__ == "__main__":
    for s in ["CCI 450", "FED 205M, SR Match", "Large Rifle Magnum, Federal 215", "REM 9½", "REM 9 1/2", "Fed 155M", "GM210M",
              "WIN SR", "Small Pistol", "CCI BR4", "CCI #35", "Fed. 215", "Large Rifle, Remington 9 1/2", "WLR", "Federal 210"]:
        print(f"{s:36s} {classify(s)}")
