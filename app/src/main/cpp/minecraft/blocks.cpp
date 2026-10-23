#include "blocks.hpp"

#include "imgui.h"
#include "imgui_internal.h"   // ImGui::RegisterUserTexture

#include <math.h>

namespace mc
{
#include "block_textures.inl"

namespace
{
    using C = ECategory;

    // Имя, категория, верх, бок, низ, лицевая сторона, флаги
    const SBlockType kBlocks[] =
    {
        { "Grass Block",          C::Natural,    T_grass_top,          T_grass_side,          T_dirt,               T_grass_side,          0 },
        { "Dirt",                 C::Natural,    T_dirt,               T_dirt,                T_dirt,               T_dirt,                0 },
        { "Coarse Dirt",          C::Natural,    T_coarse_dirt,        T_coarse_dirt,         T_coarse_dirt,        T_coarse_dirt,         0 },
        { "Podzol",               C::Natural,    T_podzol_top,         T_podzol_side,         T_dirt,               T_podzol_side,         0 },
        { "Stone",                C::Natural,    T_stone,              T_stone,               T_stone,              T_stone,               0 },
        { "Cobblestone",          C::Natural,    T_cobblestone,        T_cobblestone,         T_cobblestone,        T_cobblestone,         0 },
        { "Mossy Cobblestone",    C::Natural,    T_mossy_cobblestone,  T_mossy_cobblestone,   T_mossy_cobblestone,  T_mossy_cobblestone,   0 },
        { "Sand",                 C::Natural,    T_sand,               T_sand,                T_sand,               T_sand,                0 },
        { "Red Sand",             C::Natural,    T_red_sand,           T_red_sand,            T_red_sand,           T_red_sand,            0 },
        { "Gravel",               C::Natural,    T_gravel,             T_gravel,              T_gravel,             T_gravel,              0 },
        { "Clay",                 C::Natural,    T_clay,               T_clay,                T_clay,               T_clay,                0 },
        { "Snow",                 C::Natural,    T_snow,               T_snow,                T_snow,               T_snow,                0 },
        { "Ice",                  C::Natural,    T_ice,                T_ice,                 T_ice,                T_ice,                 kTranslucent },
        { "Packed Ice",           C::Natural,    T_packed_ice,         T_packed_ice,          T_packed_ice,         T_packed_ice,          0 },
        { "Oak Leaves",           C::Natural,    T_oak_leaves,         T_oak_leaves,          T_oak_leaves,         T_oak_leaves,          kTranslucent },
        { "Birch Leaves",         C::Natural,    T_birch_leaves,       T_birch_leaves,        T_birch_leaves,       T_birch_leaves,        kTranslucent },
        { "Spruce Leaves",        C::Natural,    T_spruce_leaves,      T_spruce_leaves,       T_spruce_leaves,      T_spruce_leaves,       kTranslucent },
        { "Pumpkin",              C::Natural,    T_pumpkin_top,        T_pumpkin_side,        T_pumpkin_top,        T_pumpkin_face,        kFacing },
        { "Jack o'Lantern",       C::Natural,    T_pumpkin_top,        T_pumpkin_side,        T_pumpkin_top,        T_jack_o_lantern,      kFacing | kGlow },
        { "Melon",                C::Natural,    T_melon_top,          T_melon_side,          T_melon_top,          T_melon_side,          0 },
        { "Hay Bale",             C::Natural,    T_hay_top,            T_hay_side,            T_hay_top,            T_hay_side,            kPillar },

        { "Oak Log",              C::Wood,       T_oak_log_top,        T_oak_log,             T_oak_log_top,        T_oak_log,             kPillar },
        { "Spruce Log",           C::Wood,       T_spruce_log_top,     T_spruce_log,          T_spruce_log_top,     T_spruce_log,          kPillar },
        { "Birch Log",            C::Wood,       T_birch_log_top,      T_birch_log,           T_birch_log_top,      T_birch_log,           kPillar },
        { "Jungle Log",           C::Wood,       T_jungle_log_top,     T_jungle_log,          T_jungle_log_top,     T_jungle_log,          kPillar },
        { "Acacia Log",           C::Wood,       T_acacia_log_top,     T_acacia_log,          T_acacia_log_top,     T_acacia_log,          kPillar },
        { "Dark Oak Log",         C::Wood,       T_dark_oak_log_top,   T_dark_oak_log,        T_dark_oak_log_top,   T_dark_oak_log,        kPillar },
        { "Oak Planks",           C::Wood,       T_oak_planks,         T_oak_planks,          T_oak_planks,         T_oak_planks,          0 },
        { "Spruce Planks",        C::Wood,       T_spruce_planks,      T_spruce_planks,       T_spruce_planks,      T_spruce_planks,       0 },
        { "Birch Planks",         C::Wood,       T_birch_planks,       T_birch_planks,        T_birch_planks,       T_birch_planks,        0 },
        { "Jungle Planks",        C::Wood,       T_jungle_planks,      T_jungle_planks,       T_jungle_planks,      T_jungle_planks,       0 },
        { "Acacia Planks",        C::Wood,       T_acacia_planks,      T_acacia_planks,       T_acacia_planks,      T_acacia_planks,       0 },
        { "Dark Oak Planks",      C::Wood,       T_dark_oak_planks,    T_dark_oak_planks,     T_dark_oak_planks,    T_dark_oak_planks,     0 },
        { "Bookshelf",            C::Wood,       T_bookshelf_top,      T_bookshelf,           T_bookshelf_top,      T_bookshelf,           0 },
        { "Crafting Table",       C::Wood,       T_crafting_table_top, T_crafting_table_side, T_oak_planks,         T_crafting_table_front, kFacing },

        { "Stone Bricks",         C::Building,   T_stone_bricks,       T_stone_bricks,        T_stone_bricks,       T_stone_bricks,        0 },
        { "Mossy Stone Bricks",   C::Building,   T_mossy_stone_bricks, T_mossy_stone_bricks,  T_mossy_stone_bricks, T_mossy_stone_bricks,  0 },
        { "Cracked Stone Bricks", C::Building,   T_cracked_stone_bricks, T_cracked_stone_bricks, T_cracked_stone_bricks, T_cracked_stone_bricks, 0 },
        { "Chiseled Stone Bricks",C::Building,   T_chiseled_stone_bricks, T_chiseled_stone_bricks, T_chiseled_stone_bricks, T_chiseled_stone_bricks, 0 },
        { "Bricks",               C::Building,   T_bricks,             T_bricks,              T_bricks,             T_bricks,              0 },
        { "Granite",              C::Building,   T_granite,            T_granite,             T_granite,            T_granite,             0 },
        { "Diorite",              C::Building,   T_diorite,            T_diorite,             T_diorite,            T_diorite,             0 },
        { "Andesite",             C::Building,   T_andesite,           T_andesite,            T_andesite,           T_andesite,            0 },
        { "Sandstone",            C::Building,   T_sandstone_top,      T_sandstone_side,      T_sandstone_bottom,   T_sandstone_side,      0 },
        { "Red Sandstone",        C::Building,   T_red_sandstone_top,  T_red_sandstone_side,  T_red_sandstone_bottom, T_red_sandstone_side, 0 },
        { "Glass",                C::Building,   T_glass,              T_glass,               T_glass,              T_glass,               kTranslucent },
        { "Quartz Block",         C::Building,   T_quartz_top,         T_quartz_side,         T_quartz_bottom,      T_quartz_side,         0 },
        { "Terracotta",           C::Building,   T_terracotta,         T_terracotta,          T_terracotta,         T_terracotta,          0 },
        { "Mud Bricks",           C::Building,   T_mud_bricks,         T_mud_bricks,          T_mud_bricks,         T_mud_bricks,          0 },
        { "Deepslate",            C::Building,   T_deepslate_top,      T_deepslate,           T_deepslate_top,      T_deepslate,           kPillar },
        { "Cobbled Deepslate",    C::Building,   T_cobbled_deepslate,  T_cobbled_deepslate,   T_cobbled_deepslate,  T_cobbled_deepslate,   0 },
        { "Obsidian",             C::Building,   T_obsidian,           T_obsidian,            T_obsidian,           T_obsidian,            0 },
        { "Crying Obsidian",      C::Building,   T_crying_obsidian,    T_crying_obsidian,     T_crying_obsidian,    T_crying_obsidian,     kGlow },
        { "Bedrock",              C::Building,   T_bedrock,            T_bedrock,             T_bedrock,            T_bedrock,             0 },

        { "Coal Ore",             C::Ores,       T_coal_ore,           T_coal_ore,            T_coal_ore,           T_coal_ore,            0 },
        { "Iron Ore",             C::Ores,       T_iron_ore,           T_iron_ore,            T_iron_ore,           T_iron_ore,            0 },
        { "Gold Ore",             C::Ores,       T_gold_ore,           T_gold_ore,            T_gold_ore,           T_gold_ore,            0 },
        { "Diamond Ore",          C::Ores,       T_diamond_ore,        T_diamond_ore,         T_diamond_ore,        T_diamond_ore,         0 },
        { "Emerald Ore",          C::Ores,       T_emerald_ore,        T_emerald_ore,         T_emerald_ore,        T_emerald_ore,         0 },
        { "Lapis Ore",            C::Ores,       T_lapis_ore,          T_lapis_ore,           T_lapis_ore,          T_lapis_ore,           0 },
        { "Redstone Ore",         C::Ores,       T_redstone_ore,       T_redstone_ore,        T_redstone_ore,       T_redstone_ore,        0 },
        { "Block of Coal",        C::Ores,       T_coal_block,         T_coal_block,          T_coal_block,         T_coal_block,          0 },
        { "Block of Iron",        C::Ores,       T_iron_block,         T_iron_block,          T_iron_block,         T_iron_block,          0 },
        { "Block of Gold",        C::Ores,       T_gold_block,         T_gold_block,          T_gold_block,         T_gold_block,          0 },
        { "Block of Diamond",     C::Ores,       T_diamond_block,      T_diamond_block,       T_diamond_block,      T_diamond_block,       0 },
        { "Block of Emerald",     C::Ores,       T_emerald_block,      T_emerald_block,       T_emerald_block,      T_emerald_block,       0 },
        { "Block of Lapis",       C::Ores,       T_lapis_block,        T_lapis_block,         T_lapis_block,        T_lapis_block,         0 },
        { "Block of Redstone",    C::Ores,       T_redstone_block,     T_redstone_block,      T_redstone_block,     T_redstone_block,      0 },
        { "Block of Copper",      C::Ores,       T_copper_block,       T_copper_block,        T_copper_block,       T_copper_block,        0 },
        { "Amethyst Block",       C::Ores,       T_amethyst_block,     T_amethyst_block,      T_amethyst_block,     T_amethyst_block,      0 },

        { "TNT",                  C::Functional, T_tnt_top,            T_tnt_side,            T_tnt_bottom,         T_tnt_side,            0 },
        { "Furnace",              C::Functional, T_furnace_top,        T_furnace_side,        T_furnace_top,        T_furnace_front,       kFacing },
        { "Sponge",               C::Functional, T_sponge,             T_sponge,              T_sponge,             T_sponge,              0 },
        { "Slime Block",          C::Functional, T_slime,              T_slime,               T_slime,              T_slime,               kTranslucent },
        { "Glowstone",            C::Functional, T_glowstone,          T_glowstone,           T_glowstone,          T_glowstone,           kGlow },

        { "White Wool",           C::Wool,       T_wool_white,         T_wool_white,          T_wool_white,         T_wool_white,          0 },
        { "Light Gray Wool",      C::Wool,       T_wool_light_gray,    T_wool_light_gray,     T_wool_light_gray,    T_wool_light_gray,     0 },
        { "Gray Wool",            C::Wool,       T_wool_gray,          T_wool_gray,           T_wool_gray,          T_wool_gray,           0 },
        { "Black Wool",           C::Wool,       T_wool_black,         T_wool_black,          T_wool_black,         T_wool_black,          0 },
        { "Brown Wool",           C::Wool,       T_wool_brown,         T_wool_brown,          T_wool_brown,         T_wool_brown,          0 },
        { "Red Wool",             C::Wool,       T_wool_red,           T_wool_red,            T_wool_red,           T_wool_red,            0 },
        { "Orange Wool",          C::Wool,       T_wool_orange,        T_wool_orange,         T_wool_orange,        T_wool_orange,         0 },
        { "Yellow Wool",          C::Wool,       T_wool_yellow,        T_wool_yellow,         T_wool_yellow,        T_wool_yellow,         0 },
        { "Lime Wool",            C::Wool,       T_wool_lime,          T_wool_lime,           T_wool_lime,          T_wool_lime,           0 },
        { "Green Wool",           C::Wool,       T_wool_green,         T_wool_green,          T_wool_green,         T_wool_green,          0 },
        { "Cyan Wool",            C::Wool,       T_wool_cyan,          T_wool_cyan,           T_wool_cyan,          T_wool_cyan,           0 },
        { "Light Blue Wool",      C::Wool,       T_wool_light_blue,    T_wool_light_blue,     T_wool_light_blue,    T_wool_light_blue,     0 },
        { "Blue Wool",            C::Wool,       T_wool_blue,          T_wool_blue,           T_wool_blue,          T_wool_blue,           0 },
        { "Purple Wool",          C::Wool,       T_wool_purple,        T_wool_purple,         T_wool_purple,        T_wool_purple,         0 },
        { "Magenta Wool",         C::Wool,       T_wool_magenta,       T_wool_magenta,        T_wool_magenta,       T_wool_magenta,        0 },
        { "Pink Wool",            C::Wool,       T_wool_pink,          T_wool_pink,           T_wool_pink,          T_wool_pink,           0 },

        { "Netherrack",           C::NetherEnd,  T_netherrack,         T_netherrack,          T_netherrack,         T_netherrack,          0 },
        { "Nether Bricks",        C::NetherEnd,  T_nether_bricks,      T_nether_bricks,       T_nether_bricks,      T_nether_bricks,       0 },
        { "Red Nether Bricks",    C::NetherEnd,  T_red_nether_bricks,  T_red_nether_bricks,   T_red_nether_bricks,  T_red_nether_bricks,   0 },
        { "Soul Sand",            C::NetherEnd,  T_soul_sand,          T_soul_sand,           T_soul_sand,          T_soul_sand,           0 },
        { "Magma Block",          C::NetherEnd,  T_magma,              T_magma,               T_magma,              T_magma,               kGlow },
        { "Nether Wart Block",    C::NetherEnd,  T_nether_wart_block,  T_nether_wart_block,   T_nether_wart_block,  T_nether_wart_block,   0 },
        { "End Stone",            C::NetherEnd,  T_end_stone,          T_end_stone,           T_end_stone,          T_end_stone,           0 },
        { "End Stone Bricks",     C::NetherEnd,  T_end_stone_bricks,   T_end_stone_bricks,    T_end_stone_bricks,   T_end_stone_bricks,    0 },
        { "Purpur Block",         C::NetherEnd,  T_purpur,             T_purpur,              T_purpur,             T_purpur,              0 },
        { "Purpur Pillar",        C::NetherEnd,  T_purpur_pillar_top,  T_purpur_pillar,       T_purpur_pillar_top,  T_purpur_pillar,       kPillar },
    };
    constexpr int kBlockCount = (int)(sizeof(kBlocks) / sizeof(kBlocks[0]));

    // Атлас: сетка kCols × N ячеек по 18 пикселей (тайл 16 + поле по краям)
    constexpr int kCols  = 12;
    constexpr int kCell  = 18;
    constexpr int kRows  = (kTileCount + kCols - 1) / kCols;
    constexpr int kAtlasW = kCols * kCell;
    constexpr int kAtlasH = kRows * kCell;
}

const char* CategoryName(ECategory eCat)
{
    switch (eCat)
    {
        case C::Natural:    return "Natural";
        case C::Wood:       return "Wood";
        case C::Building:   return "Building";
        case C::Ores:       return "Ores and minerals";
        case C::Functional: return "Functional";
        case C::Wool:       return "Wool";
        case C::NetherEnd:  return "Nether and End";
        default:            return "";
    }
}

int               BlockCount()        { return kBlockCount; }
const SBlockType& Block(int iType)    { return kBlocks[iType >= 0 && iType < kBlockCount ? iType : 0]; }

uint16_t FaceTile(const SBlockType& type, int iFace, int iAxis, int iFacing, bool& bRotate)
{
    bRotate = false;
    if (type.uFlags & kPillar)
    {
        // Торцы — вдоль оси колонны; на боках волокна идут вдоль оси, отсюда поворот
        const int iFaceAxis = iFace / 2;
        if (iFaceAxis == iAxis) return iFace == kNegY ? type.uBottom : type.uTop;
        bRotate = (iAxis == 0 && iFaceAxis != 0) || (iAxis == 2 && iFaceAxis == 0);
        return type.uSide;
    }
    if (iFace == kPosY) return type.uTop;
    if (iFace == kNegY) return type.uBottom;
    if ((type.uFlags & kFacing) && iFace == iFacing) return type.uFront;
    return type.uSide;
}

static ImTextureData* CreateAtlas()
{
    // Текстура меню — sRGB, а ImGui пишет цвет как есть: храним уже линейные значения,
    // как и цвета ui::Col(). Альфа не меняется.
    unsigned char aLinear[256];
    for (int i = 0; i < 256; i++)
    {
        const float c = (float)i / 255.f;
        aLinear[i] = (unsigned char)((c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f)) * 255.f + 0.5f);
    }

    ImTextureData* pTex = IM_NEW(ImTextureData)();
    pTex->Create(ImTextureFormat_RGBA32, kAtlasW, kAtlasH);
    for (int t = 0; t < kTileCount; t++)
    {
        const int x0 = (t % kCols) * kCell, y0 = (t / kCols) * kCell;
        for (int y = -1; y <= 16; y++)
            for (int x = -1; x <= 16; x++)
            {
                const int      sx = x < 0 ? 0 : (x > 15 ? 15 : x), sy = y < 0 ? 0 : (y > 15 ? 15 : y);
                const unsigned px = kTilePixels[t][sy * 16 + sx];
                unsigned char* d  = (unsigned char*)pTex->GetPixelsAt(x0 + 1 + x, y0 + 1 + y);
                d[0] = aLinear[px & 0xFFu];
                d[1] = aLinear[(px >> 8) & 0xFFu];
                d[2] = aLinear[(px >> 16) & 0xFFu];
                d[3] = (unsigned char)(px >> 24);
            }
    }
    pTex->UseColors = true;
    ImGui::RegisterUserTexture(pTex);
    return pTex;
}

ImTextureRef ContextAtlas()
{
    ImGuiIO& io = ImGui::GetIO();
    if (!io.UserData) io.UserData = CreateAtlas();
    return static_cast<ImTextureData*>(io.UserData)->GetTexRef();
}

SUv TileUv(uint16_t uTile)
{
    const int x0 = (uTile % kCols) * kCell + 1, y0 = (uTile / kCols) * kCell + 1;
    return { (float)x0 / (float)kAtlasW, (float)y0 / (float)kAtlasH,
             (float)(x0 + 16) / (float)kAtlasW, (float)(y0 + 16) / (float)kAtlasH };
}
}
