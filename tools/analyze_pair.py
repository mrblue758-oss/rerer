#!/usr/bin/env python3
"""Aprende recetas de deco comparando un nivel SIN deco contra el MISMO nivel CON deco.

uso:  python3 tools/analyze_pair.py sin_deco.gmd con_deco.gmd

Muestra, por cada objeto de gameplay, que piezas de deco aparecen en su mismo lugar
(id, rotacion relativa, z-layer, capa del editor, canal), y cuanta aureola hay alrededor
de los bloques por anillo de distancia. Con eso se actualiza la tabla de Core.cpp.
"""
import base64, collections, re, sys, zlib


def load(path):
    s = open(path, encoding="utf-8", errors="ignore").read()
    t = re.search(r"<k>k4</k><s>(.*?)</s>", s, re.S).group(1)
    t += "=" * (-len(t) % 4)
    raw = zlib.decompress(base64.urlsafe_b64decode(t), 47).decode("latin1")
    objs = []
    for o in raw.split(";")[1:]:
        if o:
            kv = o.split(",")
            objs.append(dict(zip(kv[0::2], kv[1::2])))
    return objs


def key(o):
    return tuple(o.get(k) for k in ("1", "2", "3", "4", "5", "6"))


def main(a, b):
    A, B = load(a), load(b)
    rem = collections.Counter(key(o) for o in A)
    deco = []
    for o in B:
        if rem[key(o)] > 0:
            rem[key(o)] -= 1
        else:
            deco.append(o)
    print(f"base={len(A)} objetos | con deco={len(B)} | deco agregada={len(deco)} | base perdida={sum(rem.values())}")

    pos = collections.defaultdict(list)
    for o in A:
        pos[(float(o["2"]), float(o["3"]))].append(o)
    recipes = collections.defaultdict(collections.Counter)
    for o in deco:
        for g in pos.get((float(o["2"]), float(o["3"])), []):
            rel = (float(o.get("6", 0)) - float(g.get("6", 0))) % 360
            recipes[g["1"]][(o["1"], int(rel), o.get("24", "0"), o.get("20", "0"),
                             "c22" if "22" in o else "-")] += 1
    print("\n== RECETAS (objeto de gameplay -> piezas en el mismo lugar) ==")
    cnt = collections.Counter(o["1"] for o in A)
    for gid, c in sorted(recipes.items(), key=lambda kv: -cnt[kv[0]]):
        print(f"gameplay {gid} (x{cnt[gid]}):")
        for (did, rel, z, el, d22), n in c.most_common(6):
            print(f"   deco {did:>5}  rotRel={rel:<3} z={z:<3} capaEditor={el:<2} {d22}  x{n}")

    print("\n== IDs de deco agregada ==")
    print(collections.Counter(o["1"] for o in deco).most_common(25))

    print("\n== canales de color en la deco (key 21) ==")
    print(collections.Counter(o.get("21") for o in deco if o["1"] not in ("899", "1007")))

    print("\n== TRIGGERS agregados ==")
    trig = collections.Counter(o["1"] for o in deco if o["1"] in ("899", "1007", "901", "1006", "1346", "1347", "1268"))
    print(dict(trig))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
