#!/usr/bin/env python3
"""Брюшная нервная цепочка мухи (BANC v888, Bates et al. 2026) -> fly_vnc.bin для шлема.

    pip install pandas pyarrow numpy
    python tools/fly/make_vnc.py --download           # скачать первоисточники и собрать
    python tools/fly/make_vnc.py --src <папка>         # из уже скачанных файлов

Мозг мухи в моде — FlyWire FAFB (fly_brain.bin, make_brain.py), а грудной ганглий с генераторами
шага, мотонейронами ног, крыльев и жужжалец — отсюда, из коннектома BANC (Brain And Nerve Cord):
FAFB заканчивается на шее. Связаны они через нисходящие (DN, мозг -> цепочка) и восходящие
(AN, цепочка -> мозг) нейроны, сопоставленные по типу клетки и стороне (в BANC у каждого нейрона
есть его тип в FAFB).

Модель цепочки — модель скоростей Pugliese et al. 2025 ("Connectome simulations identify a central
pattern generator circuit for fly walking", github.com/smpuglie/Pugliese_cpg_2025): связи — число
синапсов (ACh +, прочие медиаторы -), не меньше 5; усиление и порог — по размеру нейрона.
На BANC эта модель при частоте DNg100 (BDN2) 15-25 Гц даёт ритм всех шести ног ~10-13 Гц.

Источники: BANC v888 — Harvard Dataverse doi:10.7910/DVN/7WTH1N (CC BY 4.0); FAFB — как в
make_brain.py. Результат (~2 МБ, с --raw — 2.6 МБ без сжатия) — в корень проекта (CMake вошьёт
его в библиотеку) или на шлем рядом с fly_brain.bin. Формат — в конце файла.
"""
import argparse
import hashlib
import struct
import sys
import urllib.request
import zlib
from pathlib import Path

import numpy as np
import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parent))
import make_brain as mb   # noqa: E402  (FAFB: файлы, суммы, скачивание)

DV = "https://dataverse.harvard.edu/api/access/datafile/"
BANC = {   # имя -> (id файла в Dataverse, MD5 из его описания)
    "banc_888_meta.feather": ("14033740", "6275eda42f98c49539d1ab513d979d09"),
    "banc_888_edgelist_simple_v3.feather": ("13918810", "08542b0771db7418ed474be60dc9886c"),
}
FLOOR = 5   # связи слабее 5 синапсов не учитываем (как в статье)


def md5(p):
    h = hashlib.md5()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def fetch_banc(dst_dir, name):
    fid, want = BANC[name]
    dst = dst_dir / name
    if dst.exists() and md5(dst) == want:
        return dst
    print("  качаю", name)
    urllib.request.urlretrieve(DV + fid, dst)
    if md5(dst) != want:
        sys.exit(f"{name}: контрольная сумма не совпала")
    return dst


def module(name):
    """Мотонейрон ноги -> мышечный модуль (по названию мышцы в BANC)."""
    n = name.lower()
    if "tergotrochanter" in n or "sternotrochanter" in n or "trochanter_extensor" in n: return "tr_ext"
    if "trochanter_flexor" in n: return "tr_flex"
    if "tibia_extensor" in n: return "ti_ext"
    if "tibia_flexor" in n: return "ti_flex"
    if "promotor" in n or "anterior_rotator" in n or "adductor" in n: return "cx_swing"
    if "remotor" in n or "posterior_rotator" in n: return "cx_stance"
    if "long_tendon" in n or "tarsus_depressor" in n or "tarsus_levator" in n: return "grip"
    return ""


LEGS = [("front_leg", "left", "L1"), ("middle_leg", "left", "L2"), ("hind_leg", "left", "L3"),
        ("front_leg", "right", "R1"), ("middle_leg", "right", "R2"), ("hind_leg", "right", "R3")]
CLASS = {"ventral_nerve_cord_intrinsic": 1, "motor": 2, "ascending": 3, "sensory": 4, "sensory_ascending": 4}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--src", default=".", help="папка с исходными файлами (BANC и FAFB)")
    ap.add_argument("--download", action="store_true", help="скачать исходные файлы в --src")
    ap.add_argument("--out", default="fly_vnc.bin")
    ap.add_argument("--raw", action="store_true", help="без сжатия zlib")
    args = ap.parse_args()
    src = Path(args.src)
    src.mkdir(parents=True, exist_ok=True)
    p = {}
    for name in BANC:
        p[name] = fetch_banc(src, name) if args.download else mb.find(src, name)
    for name in ("Completeness_783.csv", "neuron_annotations.tsv"):
        p[name] = mb.fetch(src, name) if args.download else mb.find(src, name)
    missing = [k for k, v in p.items() if v is None]
    if missing:
        sys.exit(f"нет {missing} в {src} — добавь --download")

    # ---- BANC: нейроны (без глии) и связи
    import pyarrow.feather as feather
    cols = ["banc_888_id", "side", "region", "super_class", "cell_type", "fafb_cell_type",
            "body_part_effector", "cell_sub_class", "neurotransmitter_predicted", "volume_nm3", "position"]
    m = feather.read_table(p["banc_888_meta.feather"], columns=cols).to_pandas()
    m = m[~m.super_class.isin(["glia", "not_a_neuron", "trachea"])].reset_index(drop=True)
    gid = pd.Series(np.arange(len(m)), index=m.banc_888_id.astype("int64"))
    e = feather.read_table(p["banc_888_edgelist_simple_v3.feather"], columns=["pre", "post", "count"]).to_pandas()
    pre = e.pre.astype("int64").map(gid); post = e.post.astype("int64").map(gid)
    ok = pre.notna() & post.notna() & (e["count"] >= FLOOR)
    pre = pre[ok].to_numpy(np.int64); post = post[ok].to_numpy(np.int64); cnt = e["count"][ok].to_numpy(np.int64)

    vnc = (m.region.fillna("") == "ventral_nerve_cord").to_numpy()
    dn = (m.super_class == "descending").to_numpy()
    ct = m.cell_type.fillna("").to_numpy()
    nt = m.neurotransmitter_predicted.fillna("").to_numpy()
    sign = np.where(nt == "acetylcholine", 1, -1)                       # как Pugliese: не ACh — тормозный
    vi = np.flatnonzero(vnc); di = np.flatnonzero(dn)
    Nv, Nd = len(vi), len(di)
    local = np.full(len(m), -1, np.int64); local[vi] = np.arange(Nv); local[di] = Nv + np.arange(Nd)
    k = vnc[post] & (local[pre] >= 0)                                  # входы цепочки: из неё самой и из DN
    pre, post, w = local[pre[k]], local[post[k]], (cnt[k] * sign[pre[k]]).clip(-8191, 8191)
    o = np.lexsort((post, pre)); pre, post, w = pre[o], post[o], w[o]
    cptr = np.zeros(Nv + Nd + 1, np.uint32); cptr[1:] = np.cumsum(np.bincount(pre, minlength=Nv + Nd))
    syn = (post.astype(np.uint32) | ((w.astype(np.int32) << 18).astype(np.uint32))).astype("<u4")
    print(f"цепочка: нейронов {Nv}, DN {Nd}, связей {len(syn)}, синапсов {int(np.abs(w).sum())}")

    size = m.volume_nm3.to_numpy(float).copy()
    med = np.nanmedian(size[vnc | dn]); size[~np.isfinite(size) | (size <= 0)] = med; size = (size / med).astype("<f4")
    # точка нейрона: воксели BANC 4×4×45 нм -> 0.25 мкм
    pos = np.zeros((Nv, 3), "<i2")
    for j, i in enumerate(vi):
        try:
            xyz = np.array([float(v) for v in str(m.position[i]).split(",")]) * (4.0, 4.0, 45.0) / 250.0
            pos[j] = np.clip(xyz, -32768, 32767)
        except ValueError:
            pass
    cls = np.array([CLASS.get(m.super_class[i], 0) for i in vi], np.uint8)

    # ---- FAFB: номера нейронов модели мозга (порядок Completeness), типы и стороны
    comp = pd.read_csv(p["Completeness_783.csv"], index_col=0)
    fidx = {int(r): i for i, r in enumerate(comp.index.values)}
    a = pd.read_csv(p["neuron_annotations.tsv"], sep="\t", low_memory=False, usecols=["root_id", "cell_type", "super_class", "side"])
    a = a.drop_duplicates("root_id"); a["idx"] = a.root_id.map(fidx); a = a[a.idx.notna()]
    def fafb_of(cls_name):
        f = a[a.super_class == cls_name]
        by_ts = f.groupby(["cell_type", "side"]).idx.apply(lambda s: [int(x) for x in s]).to_dict()
        by_t = f.groupby("cell_type").idx.apply(lambda s: [int(x) for x in s]).to_dict()
        return by_ts, by_t
    def key(i):
        t = m.fafb_cell_type[i]
        return t if isinstance(t, str) and t else ct[i]
    dts, dt_ = fafb_of("descending")
    dn_map = []
    for i in di:
        t = key(i)
        dn_map.append(dts.get((t, m.side[i]), dt_.get(t, []))[:16])
    print(f"DN сопоставлено с FAFB: {sum(1 for x in dn_map if x)} из {Nd}")
    ats, _ = fafb_of("ascending")
    an_map = {}
    for j, i in enumerate(vi):
        if m.super_class[i] != "ascending": continue
        for f in ats.get((key(i), m.side[i]), []):
            an_map.setdefault(f, []).append(j)
    print(f"AN: нейронов мозга FAFB, которых ведёт цепочка: {len(an_map)}")

    # ---- группы: мышечные модули каждой ноги, крылья, жужжальца, прыжок
    sub = m.cell_sub_class.fillna("").to_numpy(); leg = m.body_part_effector.fillna("").to_numpy()
    side = m.side.fillna("").to_numpy()
    mn = [j for j, i in enumerate(vi) if m.super_class[i] == "motor"]
    groups = {}
    for L, s_, nm in LEGS:
        for mod in ("cx_swing", "cx_stance", "tr_ext", "tr_flex", "ti_ext", "ti_flex", "grip"):
            groups[f"{nm}_{mod}"] = [j for j in mn if leg[vi[j]] == L and side[vi[j]] == s_ and module(ct[vi[j]]) == mod]
        groups[f"{nm}_all"] = [j for j in mn if leg[vi[j]] == L and side[vi[j]] == s_]
    for s_, S in (("left", "L"), ("right", "R")):
        groups[f"wing_power_{S}"] = [j for j in mn if sub[vi[j]] == "wing_power_motor_neuron" and side[vi[j]] == s_]
        groups[f"wing_steer_{S}"] = [j for j in mn if sub[vi[j]] == "wing_steering_motor_neuron" and side[vi[j]] == s_]
        groups[f"haltere_{S}"] = [j for j in mn if "haltere" in sub[vi[j]] and side[vi[j]] == s_]
        groups[f"ttm_{S}"] = [j for j in mn if "tergotrochanter" in ct[vi[j]] and leg[vi[j]] == "middle_leg" and side[vi[j]] == s_]
        groups[f"neck_{S}"] = [j for j in mn if leg[vi[j]] == "neck" and side[vi[j]] == s_]
    for t in ("DNg100", "DNa02", "DNp01", "MDN", "DNp09", "DNb08", "DNg84", "DNg35"):
        groups["dn_" + t] = [Nv + d for d, i in enumerate(di) if ct[i] == t]
    print({k: len(v) for k, v in groups.items() if not k.endswith(("_grip", "_ti_ext"))})

    body = bytearray()
    body += cptr.astype("<u4").tobytes() + syn.tobytes() + size[vi].tobytes() + size[di].tobytes() + pos.tobytes() + cls.tobytes()
    for f in dn_map:
        body += struct.pack("<B", len(f)) + np.asarray(f, "<u4").tobytes()
    body += struct.pack("<I", len(an_map))
    for f, js in sorted(an_map.items()):
        js = js[:255]
        body += struct.pack("<IB", f, len(js)) + np.asarray(js, "<u4").tobytes()
    body += struct.pack("<I", len(groups))
    for name, ids in groups.items():
        body += struct.pack("<B", len(name)) + name.encode() + struct.pack("<I", len(ids)) + np.asarray(ids, "<u4").tobytes()
    payload = bytes(body) if args.raw else zlib.compress(bytes(body), 6)
    with open(args.out, "wb") as f:
        f.write(b"FLYV" + struct.pack("<IIIIII", 1, Nv, Nd, len(syn), 0 if args.raw else 1, len(body)))
        f.write(payload)
    print(f"готово: {args.out}, {Path(args.out).stat().st_size / 1e6:.1f} МБ")


if __name__ == "__main__":
    main()

# Формат fly_vnc.bin (little-endian):
#   "FLYV", u32 версия = 1, u32 Nv (нейронов цепочки), u32 Nd (нисходящих входов), u32 число связей M,
#   u32 флаги (1 = данные сжаты zlib), u32 размер данных без сжатия
#   данные: u32 cptr[Nv + Nd + 1]   исходящие связи: 0..Nv-1 — нейроны цепочки, Nv.. — DN
#           u32 syn[M]              биты 0..17 — нейрон цепочки, 18..31 — знак × число синапсов (int14)
#           f32 size[Nv + Nd]       размер (объём / медиана) — усиление делится, порог умножается
#           i16 pos[Nv][3]          координаты BANC, 0.25 мкм
#           u8  class[Nv]           1 интернейрон, 2 мотонейрон, 3 восходящий, 4 сенсорный
#           Nd раз: u8 n, u32 fafb[n]       DN: нейроны мозга FAFB, чья средняя частота — его частота
#           u32 Na, Na раз: u32 fafb, u8 n, u32 vnc[n]   AN: нейрон FAFB и нейроны цепочки, которые его ведут
#           u32 число групп, затем группы: u8 длина имени, имя, u32 n, u32 индексы[n]
