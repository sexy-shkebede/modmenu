#!/usr/bin/env python3
"""Коннектом мухи (FlyWire v783, модель Shiu et al. 2024) -> fly_brain.bin для шлема.

    pip install pandas pyarrow numpy
    python tools/fly/make_brain.py --download            # скачать первоисточники и собрать
    python tools/fly/make_brain.py --src <папка>          # из уже скачанных файлов

<папка> — где лежат Connectivity_783.parquet, Completeness_783.csv и neuron_annotations.tsv
(например, brain_model/ и data/ из репозитория fly-connectome: файлы ищутся и там, и там).
Источники и контрольные суммы — те же, что в fly-connectome/setup/get_data.py.

Результат (~50 МБ, с --raw без сжатия ~62 МБ) либо оставить в корне проекта — CMake вошьёт его в
libopenxrhooktest.so (без сжатия он читается прямо из библиотеки, без распаковки в память), либо
положить на шлем: adb push fly_brain.bin /sdcard/Android/obb/<пакет игры>/
(или в /sdcard/Android/data/<пакет игры>/files/). Формат — в конце файла.

Группы входов и выходов — как в fly-connectome/tiktok/senses.py (SENSES и READOUTS), плюс запах (odor_L/R).
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

SHIU = "https://raw.githubusercontent.com/philshiu/Drosophila_brain_model/91bdd1e7dcf193f3e7ca5a8933497fcef63b7960/"
ANN = ("https://raw.githubusercontent.com/flyconnectome/flywire_annotations/17fc57722002e1a7d38cdd0c89ac382bf92718da/"
       "supplemental_files/Supplemental_file1_neuron_annotations.tsv")
FILES = {
    "Connectivity_783.parquet": (SHIU + "Connectivity_783.parquet", "efeb23fb99098e9c390f6869969b2a121a2ee92c833cfc45ecb2c1d8e1af0347"),
    "Completeness_783.csv": (SHIU + "Completeness_783.csv", "bbb847a4cc2caaa7a16349722d220c087317b946d148d4d592d94d250617a311"),
    "neuron_annotations.tsv": (ANN, "9a4f8b2f843196074431ebd7cd883536afa1be86c8a4ce90970441e8be81d1be"),
}
CRLF_OK = {"Completeness_783.csv": "52b0ac6094cd32c546f8d4c341e094376f48f4e791f8db9b166de5dff8199ea4"}

# ---- группы нейронов: fly-connectome/tiktok/senses.py ----
MN9_ID = 720575940660219265
SUGAR_IDS_630 = [720575940624963786, 720575940630233916, 720575940637568838, 720575940638202345,
                 720575940617000768, 720575940630797113, 720575940632889389, 720575940621754367,
                 720575940621502051, 720575940640649691, 720575940639332736, 720575940616885538,
                 720575940639198653, 720575940620900446, 720575940617937543, 720575940632425919,
                 720575940633143833, 720575940612670570, 720575940628853239, 720575940629176663,
                 720575940611875570]
LC10 = ["LC10a", "LC10c-1", "LC10c-2", "LC10d", "LC10e"]
LOOM = ["LPLC2", "LC4"]
FTB_LPT = ["HSE", "HSN", "HSS", "VCH", "DCH"]
BTF_LPT = ["H1", "H2"]
# Запах сладкого и бродящего (уксус, фрукты): обонятельные рецепторы антенн, которые отвечают на
# него сильнее всего (Semmelhack & Wang 2009: DM1, DM2, DM4, VA2, VM2, DP1m). Нет в senses.py —
# добавлено для мухи в игре: капли сахара пахнут
ODOR = ["ORN_DM1", "ORN_DM2", "ORN_DM4", "ORN_VA2", "ORN_VM2", "ORN_DP1m"]
GROUPS = {
    # входы
    "obj_L": dict(types=LC10, side="left"),   "obj_R": dict(types=LC10, side="right"),
    "loom_L": dict(types=LOOM, side="left"),  "loom_R": dict(types=LOOM, side="right"),
    "ftb_L": dict(types=["T4a", "T5a"], side="left"), "ftb_R": dict(types=["T4a", "T5a"], side="right"),
    "btf_L": dict(types=["T4b", "T5b"], side="left"), "btf_R": dict(types=["T4b", "T5b"], side="right"),
    "lpt_ftb_L": dict(types=FTB_LPT, side="left"), "lpt_ftb_R": dict(types=FTB_LPT, side="right"),
    "lpt_btf_L": dict(types=BTF_LPT, side="left"), "lpt_btf_R": dict(types=BTF_LPT, side="right"),
    "hear_A": dict(prefix=["JO-A"]), "hear_B": dict(prefix=["JO-B"]),
    "puff": dict(types=["BM_Ant", "BM_Fr", "BM_FrOr"]), "wind": dict(prefix=["JO-C", "JO-E"]),
    "sugar": dict(ids=SUGAR_IDS_630), "bitter": dict(sub_class="bitter"),
    "odor_L": dict(types=ODOR, side="left"), "odor_R": dict(types=ODOR, side="right"),
    # выходы
    "steer_L": dict(types=["DNa01", "DNa02"], side="left"), "steer_R": dict(types=["DNa01", "DNa02"], side="right"),
    "head_L": dict(types=["DNp15"], side="left"), "head_R": dict(types=["DNp15"], side="right"),
    "gf": dict(types=["DNp01"]), "fear": dict(types=["DNp01", "DNp02", "DNp04", "DNp11"]),
    "mn9": dict(ids=[MN9_ID]), "groom": dict(types=["DNg84", "DNg35"]),
    "walk": dict(types=["DNp09"]), "back": dict(types=["MDN"]),
    "hear2": dict(auto="jo_ab_top60"), "hear_jo": dict(prefix=["JO-A", "JO-B"]),
}
SUPER = ["optic", "central", "sensory", "visual_projection", "ascending", "descending",
         "sensory_ascending", "visual_centrifugal", "motor", "endocrine"]   # код = индекс + 1, 0 — нет


def sha256(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def find(src, name):
    for p in (src / name, src / "brain_model" / name, src / "data" / name):
        if p.exists():
            return p
    return None


def fetch(dst_dir, name):
    url, want = FILES[name]
    dst = dst_dir / name
    if dst.exists() and sha256(dst) in (want, CRLF_OK.get(name)):
        return dst
    print("  качаю", name)
    urllib.request.urlretrieve(url, dst)
    got = sha256(dst)
    if got not in (want, CRLF_OK.get(name)):
        sys.exit(f"{name}: контрольная сумма не совпала ({got})")
    return dst


def select(ann, idx, d):
    if "ids" in d:
        return [idx[r] for r in d["ids"] if r in idx]
    if "sub_class" in d:
        a = ann[ann.cell_sub_class.astype(str) == d["sub_class"]]
        return [idx[r] for r in a.index if r in idx]
    ct = ann.cell_type.astype(str)
    if "prefix" in d:
        m = np.zeros(len(ann), bool)
        for p in d["prefix"]:
            m |= ct.str.startswith(p).values
    else:
        m = ct.isin(d["types"]).values
    a = ann[m]
    if d.get("side"):
        a = a[a.side == d["side"]]
    return [idx[r] for r in a.index if r in idx]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--src", default=".", help="папка с исходными файлами")
    ap.add_argument("--download", action="store_true", help="скачать исходные файлы в --src")
    ap.add_argument("--out", default="fly_brain.bin")
    ap.add_argument("--raw", action="store_true", help="без сжатия zlib")
    args = ap.parse_args()
    src = Path(args.src)
    src.mkdir(parents=True, exist_ok=True)
    paths = {}
    for name in FILES:
        paths[name] = fetch(src, name) if args.download else find(src, name)
        if paths[name] is None:
            sys.exit(f"нет {name} в {src} — добавь --download")

    comp = pd.read_csv(paths["Completeness_783.csv"], index_col=0)
    ids = comp.index.values
    N = len(ids)
    idx = {int(r): i for i, r in enumerate(ids)}
    con = pd.read_parquet(paths["Connectivity_783.parquet"],
                          columns=["Presynaptic_Index", "Postsynaptic_Index", "Excitatory x Connectivity"])
    pre = con["Presynaptic_Index"].values.astype(np.int64)
    post = con["Postsynaptic_Index"].values.astype(np.int64)
    w = con["Excitatory x Connectivity"].values.astype(np.int64)
    assert N < (1 << 18) and w.min() >= -(1 << 13) and w.max() < (1 << 13), "не влезает в 18 + 14 бит"
    order = np.lexsort((post, pre))                       # исходящие синапсы по пресинаптическому нейрону
    pre, post, w = pre[order], post[order], w[order]
    cptr = np.zeros(N + 1, np.uint32)
    cptr[1:] = np.cumsum(np.bincount(pre, minlength=N))
    syn = (post.astype(np.uint32) | ((w.astype(np.int32) << 18).astype(np.uint32))).astype("<u4")
    print(f"нейронов {N}, связей {len(syn)}, синапсов {int(np.abs(w).sum())}")

    ann = pd.read_csv(paths["neuron_annotations.tsv"], sep="\t", low_memory=False,
                      usecols=["root_id", "pos_x", "pos_y", "pos_z", "super_class", "cell_sub_class", "cell_type", "side"])
    ann = ann.drop_duplicates("root_id").set_index("root_id")
    groups = {}
    for name, d in GROUPS.items():
        if d.get("auto") == "jo_ab_top60":
            # слуховые нейроны 2-го порядка: топ-60 по входу от JO-A/B (как senses._hear2)
            jo = np.zeros(N, np.float64)
            jo[select(ann, idx, dict(prefix=["JO-A", "JO-B"]))] = 1.0
            w_in = np.bincount(post, weights=w * jo[pre], minlength=N)
            bad = ann.index[ann.super_class.isin(["sensory", "sensory_ascending", "descending"])]
            w_in[[idx[r] for r in bad if r in idx]] = -1
            groups[name] = [int(i) for i in np.argsort(-w_in, kind="stable")[:60]]
        else:
            groups[name] = select(ann, idx, d)
        if not groups[name]:
            sys.exit(f"пустая группа {name}")
    print({k: len(v) for k, v in groups.items()})

    # координаты (воксели FlyWire 4×4×40 нм) -> мкм × 4 в int16; класс нейрона — для раскраски
    a = ann.reindex(ids)
    pos_nm = np.stack([a.pos_x.values * 4.0, a.pos_y.values * 4.0, a.pos_z.values * 40.0], 1)
    pos = np.nan_to_num(pos_nm / 250.0, nan=0.0).round().clip(-32768, 32767).astype("<i2")
    sup = np.array([SUPER.index(s) + 1 if s in SUPER else 0 for s in a.super_class.values], np.uint8)

    body = bytearray()
    body += cptr.astype("<u4").tobytes() + syn.tobytes() + pos.tobytes() + sup.tobytes()
    body += struct.pack("<I", len(groups))
    for name, members in groups.items():
        nb = name.encode()
        body += struct.pack("<B", len(nb)) + nb + struct.pack("<I", len(members))
        body += np.asarray(members, "<u4").tobytes()
    payload = bytes(body) if args.raw else zlib.compress(bytes(body), 6)
    with open(args.out, "wb") as f:
        f.write(b"FLY1" + struct.pack("<IIIII", 1, N, len(syn), 0 if args.raw else 1, len(body)))
        f.write(payload)
    print(f"готово: {args.out}, {Path(args.out).stat().st_size / 1e6:.1f} МБ")


if __name__ == "__main__":
    main()

# Формат fly_brain.bin (little-endian):
#   "FLY1", u32 версия = 1, u32 N, u32 число связей M, u32 флаги (1 = данные сжаты zlib), u32 размер данных без сжатия
#   данные: u32 cptr[N+1]                    исходящие связи нейрона j: syn[cptr[j] .. cptr[j+1])
#           u32 syn[M]                       биты 0..17 — постсинаптический нейрон, 18..31 — знак × число синапсов (int14)
#           i16 pos[N][3]                    координаты сомы/точки нейрона, единица 0.25 мкм (FlyWire: x вправо, y вниз)
#           u8  super_class[N]               1 optic, 2 central, 3 sensory, 4 visual_projection, ... (0 — нет аннотации)
#           u32 число групп, затем группы:  u8 длина имени, имя, u32 n, u32 индексы[n]
