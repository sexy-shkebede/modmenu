#!/usr/bin/env python3
"""Собирает текстуры блоков из tools/minecraft/textures/*.png в C++-заголовок.

    python3 tools/minecraft/make_textures.py

Результат — app/src/main/cpp/minecraft/block_textures.inl: перечисление тайлов
(ETile) и их пиксели 16×16 в RGBA (sRGB, прямая альфа). Своя текстура: положи PNG
в textures/, добавь строку в TILES и сошлись на тайл в minecraft/blocks.cpp.

Нужен только Python 3 (без Pillow): PNG разбирается здесь же. Поддерживаются
8-битные PNG без чересстрочности — палитровые, серые, RGB и RGBA. У анимированных
(кадры столбиком) берётся первый кадр.
"""

import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "textures")
OUT = os.path.join(HERE, "..", "..", "app", "src", "main", "cpp", "minecraft", "block_textures.inl")

# Цвета травы и листвы — как в Minecraft для равнин (серые текстуры красятся умножением).
GRASS = 0x91BD59
OAK_LEAVES = 0x77AB2F
BIRCH_LEAVES = 0x80A755
SPRUCE_LEAVES = 0x619961

# Тайл: имя → файл, или (файл, цвет) — покрасить, или список слоёв, наложенных по альфе.
TILES = [
    ("grass_top",            ("mcl_core_grass_block_top.png", GRASS)),
    ("grass_side",           ["default_dirt.png", ("mcl_core_grass_block_side_overlay.png", GRASS)]),
    ("dirt",                 "default_dirt.png"),
    ("coarse_dirt",          "mcl_core_coarse_dirt.png"),
    ("podzol_top",           "mcl_core_dirt_podzol_top.png"),
    ("podzol_side",          "mcl_core_dirt_podzol_side.png"),
    ("stone",                "default_stone.png"),
    ("cobblestone",          "default_cobble.png"),
    ("mossy_cobblestone",    "default_mossycobble.png"),
    ("sand",                 "default_sand.png"),
    ("red_sand",             "mcl_core_red_sand.png"),
    ("gravel",               "default_gravel.png"),
    ("clay",                 "default_clay.png"),
    ("snow",                 "default_snow.png"),
    ("ice",                  "default_ice.png"),
    ("packed_ice",           "mcl_core_ice_packed.png"),
    ("oak_leaves",           ("default_leaves.png", OAK_LEAVES)),
    ("birch_leaves",         ("mcl_core_leaves_birch.png", BIRCH_LEAVES)),
    ("spruce_leaves",        ("mcl_core_leaves_spruce.png", SPRUCE_LEAVES)),
    ("pumpkin_top",          "farming_pumpkin_top.png"),
    ("pumpkin_side",         "farming_pumpkin_side.png"),
    ("pumpkin_face",         "farming_pumpkin_face.png"),
    ("jack_o_lantern",       "farming_pumpkin_face_light.png"),
    ("melon_top",            "farming_melon_top.png"),
    ("melon_side",           "farming_melon_side.png"),
    ("hay_top",              "mcl_farming_hayblock_top.png"),
    ("hay_side",             "mcl_farming_hayblock_side.png"),

    ("oak_log",              "default_tree.png"),
    ("oak_log_top",          "default_tree_top.png"),
    ("spruce_log",           "mcl_core_log_spruce.png"),
    ("spruce_log_top",       "mcl_core_log_spruce_top.png"),
    ("birch_log",            "mcl_core_log_birch.png"),
    ("birch_log_top",        "mcl_core_log_birch_top.png"),
    ("jungle_log",           "default_jungletree.png"),
    ("jungle_log_top",       "default_jungletree_top.png"),
    ("acacia_log",           "default_acacia_tree.png"),
    ("acacia_log_top",       "default_acacia_tree_top.png"),
    ("dark_oak_log",         "mcl_core_log_big_oak.png"),
    ("dark_oak_log_top",     "mcl_core_log_big_oak_top.png"),
    ("oak_planks",           "default_wood.png"),
    ("spruce_planks",        "mcl_core_planks_spruce.png"),
    ("birch_planks",         "mcl_core_planks_birch.png"),
    ("jungle_planks",        "default_junglewood.png"),
    ("acacia_planks",        "default_acacia_wood.png"),
    ("dark_oak_planks",      "mcl_core_planks_big_oak.png"),
    ("bookshelf",            "default_bookshelf.png"),
    ("bookshelf_top",        "mcl_books_bookshelf_top.png"),
    ("crafting_table_top",   "crafting_workbench_top.png"),
    ("crafting_table_side",  "crafting_workbench_side.png"),
    ("crafting_table_front", "crafting_workbench_front.png"),

    ("stone_bricks",         "default_stone_brick.png"),
    ("mossy_stone_bricks",   "mcl_core_stonebrick_mossy.png"),
    ("cracked_stone_bricks", "mcl_core_stonebrick_cracked.png"),
    ("chiseled_stone_bricks","mcl_core_stonebrick_carved.png"),
    ("bricks",               "default_brick.png"),
    ("granite",              "mcl_core_granite.png"),
    ("diorite",              "mcl_core_diorite.png"),
    ("andesite",             "mcl_core_andesite.png"),
    ("sandstone_top",        "mcl_core_sandstone_top.png"),
    ("sandstone_side",       "mcl_core_sandstone_normal.png"),
    ("sandstone_bottom",     "mcl_core_sandstone_bottom.png"),
    ("red_sandstone_top",    "mcl_core_red_sandstone_top.png"),
    ("red_sandstone_side",   "mcl_core_red_sandstone_normal.png"),
    ("red_sandstone_bottom", "mcl_core_red_sandstone_bottom.png"),
    ("glass",                "default_glass.png"),
    ("obsidian",             "default_obsidian.png"),
    ("crying_obsidian",      "mcl_core_crying_obsidian.png"),
    ("bedrock",              "mcl_core_bedrock.png"),
    ("deepslate",            "mcl_deepslate.png"),
    ("deepslate_top",        "mcl_deepslate_top.png"),
    ("cobbled_deepslate",    "mcl_deepslate_cobbled.png"),
    ("mud_bricks",           "mcl_mud_bricks.png"),
    ("terracotta",           "hardened_clay.png"),
    ("quartz_top",           "mcl_nether_quartz_block_top.png"),
    ("quartz_side",          "mcl_nether_quartz_block_side.png"),
    ("quartz_bottom",        "mcl_nether_quartz_block_bottom.png"),

    ("coal_ore",             "mcl_core_coal_ore.png"),
    ("iron_ore",             "mcl_core_iron_ore.png"),
    ("gold_ore",             "mcl_core_gold_ore.png"),
    ("diamond_ore",          "mcl_core_diamond_ore.png"),
    ("emerald_ore",          "mcl_core_emerald_ore.png"),
    ("lapis_ore",            "mcl_core_lapis_ore.png"),
    ("redstone_ore",         "mcl_core_redstone_ore.png"),
    ("coal_block",           "default_coal_block.png"),
    ("iron_block",           "default_steel_block.png"),
    ("gold_block",           "default_gold_block.png"),
    ("diamond_block",        "default_diamond_block.png"),
    ("emerald_block",        "mcl_core_emerald_block.png"),
    ("lapis_block",          "mcl_core_lapis_block.png"),
    ("redstone_block",       "redstone_redstone_block.png"),
    ("copper_block",         "mcl_copper_block.png"),
    ("amethyst_block",       "mcl_amethyst_amethyst_block.png"),

    ("tnt_top",              "default_tnt_top.png"),
    ("tnt_side",             "default_tnt_side.png"),
    ("tnt_bottom",           "default_tnt_bottom.png"),
    ("furnace_top",          "default_furnace_top.png"),
    ("furnace_side",         "default_furnace_side.png"),
    ("furnace_front",        "default_furnace_front.png"),
    ("sponge",               "mcl_sponges_sponge.png"),
    ("slime",                "mcl_core_slime.png"),

    ("wool_white",           "wool_white.png"),
    ("wool_light_gray",      "wool_grey.png"),
    ("wool_gray",            "wool_dark_grey.png"),
    ("wool_black",           "wool_black.png"),
    ("wool_brown",           "wool_brown.png"),
    ("wool_red",             "wool_red.png"),
    ("wool_orange",          "wool_orange.png"),
    ("wool_yellow",          "wool_yellow.png"),
    ("wool_lime",            "mcl_wool_lime.png"),
    ("wool_green",           "wool_dark_green.png"),
    ("wool_cyan",            "wool_cyan.png"),
    ("wool_light_blue",      "mcl_wool_light_blue.png"),
    ("wool_blue",            "wool_blue.png"),
    ("wool_purple",          "wool_violet.png"),
    ("wool_magenta",         "wool_magenta.png"),
    ("wool_pink",            "wool_pink.png"),

    ("netherrack",           "mcl_nether_netherrack.png"),
    ("nether_bricks",        "mcl_nether_nether_brick.png"),
    ("red_nether_bricks",    "mcl_nether_red_nether_brick.png"),
    ("soul_sand",            "mcl_nether_soul_sand.png"),
    ("magma",                "mcl_nether_magma.png"),
    ("nether_wart_block",    "mcl_nether_nether_wart_block.png"),
    ("glowstone",            "mcl_nether_glowstone.png"),
    ("end_stone",            "mcl_end_end_stone.png"),
    ("end_stone_bricks",     "mcl_end_end_bricks.png"),
    ("purpur",               "mcl_end_purpur_block.png"),
    ("purpur_pillar",        "mcl_end_purpur_pillar.png"),
    ("purpur_pillar_top",    "mcl_end_purpur_pillar_top.png"),
]


def read_png(path):
    """PNG → (ширина, высота, список (r, g, b, a))."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(path + ": not a PNG")
    pos, idat, palette, trns = 8, b"", None, None
    width = height = depth = ctype = interlace = 0
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        chunk = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            width, height, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
        elif kind == b"PLTE":
            palette = [tuple(chunk[i:i + 3]) for i in range(0, len(chunk), 3)]
        elif kind == b"tRNS":
            trns = chunk
        elif kind == b"IDAT":
            idat += chunk
        elif kind == b"IEND":
            break
    if interlace:
        raise ValueError(path + ": interlaced PNG is not supported")

    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    if ctype == 3:
        if depth not in (1, 2, 4, 8):
            raise ValueError(path + ": unsupported palette depth")
    elif depth != 8:
        raise ValueError(path + ": only 8-bit PNG is supported")
    bpp = max(1, channels * depth // 8)                  # байт на пиксель для фильтров
    stride = (width * channels * depth + 7) // 8
    raw = zlib.decompress(idat)

    rows, prev = [], bytearray(stride)
    for y in range(height):
        ftype = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if ftype == 1:
                line[i] = (line[i] + a) & 0xFF
            elif ftype == 2:
                line[i] = (line[i] + b) & 0xFF
            elif ftype == 3:
                line[i] = (line[i] + (a + b) // 2) & 0xFF
            elif ftype == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 0xFF
        rows.append(line)
        prev = line

    pixels = []
    for line in rows:
        for x in range(width):
            if ctype == 3:
                per = 8 // depth
                idx = (line[x // per] >> (8 - depth * (x % per + 1))) & ((1 << depth) - 1)
                r, g, b = palette[idx]
                a = trns[idx] if trns and idx < len(trns) else 255
            elif ctype == 0:
                r = g = b = line[x]
                a = 0 if trns and struct.unpack(">H", trns[:2])[0] == r else 255
            elif ctype == 4:
                r = g = b = line[x * 2]
                a = line[x * 2 + 1]
            elif ctype == 2:
                r, g, b = line[x * 3:x * 3 + 3]
                a = 255
                if trns and struct.unpack(">HHH", trns[:6]) == (r, g, b):
                    a = 0
            else:
                r, g, b, a = line[x * 4:x * 4 + 4]
            pixels.append((r, g, b, a))
    return width, height, pixels


def load_tile(name):
    w, h, px = read_png(os.path.join(SRC, name))
    if w != 16 or h < 16 or h % 16:
        raise ValueError("%s: expected 16x16 (or 16-wide animation strip), got %dx%d" % (name, w, h))
    return px[:256]


def tint(px, rgb):
    # Как в Minecraft: яркость серой текстуры × цвет биома
    tr, tg, tb = (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF
    out = []
    for r, g, b, a in px:
        lum = (r * 299 + g * 587 + b * 114) / 1000.0
        out.append((int(lum * tr / 255 + 0.5), int(lum * tg / 255 + 0.5), int(lum * tb / 255 + 0.5), a))
    return out


def layer(spec):
    if isinstance(spec, tuple):
        return tint(load_tile(spec[0]), spec[1])
    return load_tile(spec)


def build(spec):
    if not isinstance(spec, list):
        return layer(spec)
    out = layer(spec[0])
    for top in spec[1:]:
        over = layer(top)
        mixed = []
        for (r0, g0, b0, a0), (r1, g1, b1, a1) in zip(out, over):
            t = a1 / 255.0
            mixed.append((int(r0 + (r1 - r0) * t + 0.5), int(g0 + (g1 - g0) * t + 0.5),
                          int(b0 + (b1 - b0) * t + 0.5), max(a0, a1)))
        out = mixed
    return out


def main():
    names = [n for n, _ in TILES]
    if len(set(names)) != len(names):
        sys.exit("duplicate tile names")
    lines = [
        "// Сгенерировано tools/minecraft/make_textures.py — не править руками.",
        "// Текстуры: Pixel Perfection (XSSheep) и Pixel Perfection Legacy (Nova Wostra), CC BY-SA 4.0,",
        "// в сборке Mineclonia. Трава и листва перекрашены скриптом. Подробности — tools/minecraft/LICENSE.md.",
        "// Пиксель — 0xAABBGGRR (байты в памяти: R, G, B, A), sRGB, прямая альфа.",
        "",
        "enum ETile : unsigned short",
        "{",
    ]
    for n in names:
        lines.append("    T_%s," % n)
    lines += ["    kTileCount", "};", "", "static const unsigned kTilePixels[kTileCount][256] =", "{"]
    for n, spec in TILES:
        px = build(spec)
        words = ["0x%08X" % ((a << 24) | (b << 16) | (g << 8) | r) for r, g, b, a in px]
        lines.append("    { // %s" % n)
        for i in range(0, 256, 8):
            lines.append("        " + ", ".join(words[i:i + 8]) + ",")
        lines.append("    },")
    lines += ["};", ""]
    with open(OUT, "w", newline="\n") as f:
        f.write("\n".join(lines))
    print("%d tiles -> %s" % (len(TILES), os.path.relpath(OUT)))


if __name__ == "__main__":
    main()
