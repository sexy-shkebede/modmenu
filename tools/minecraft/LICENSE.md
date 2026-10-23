# Текстуры блоков

Файлы в `textures/` взяты из игры [Mineclonia](https://codeberg.org/mineclonia/mineclonia)
(папки `mods/ITEMS/*/textures`) без изменений. По её `LEGAL.md` это текстуры паков:

- **Pixel Perfection** by XSSheep — <https://www.planetminecraft.com/texture_pack/131pixel-perfection/>
- **Pixel Perfection Legacy** by Nova Wostra — <https://www.curseforge.com/minecraft/texture-packs/pixel-perfection-legacy>

Лицензия: [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).

Изменения: `make_textures.py` перекрашивает серые текстуры травы и листвы в цвета
равнинного биома, накладывает траву на бок блока земли и сохраняет всё в
`app/src/main/cpp/minecraft/block_textures.inl`. Этот файл (производная работа)
распространяется на тех же условиях CC BY-SA 4.0.

Mineclonia и эти паки не связаны с Mojang и Microsoft. Оригинальные текстуры Minecraft
принадлежат Mojang, поэтому в проект не входят. Свой пак 16×16 подключается так же:
положи PNG в `textures/`, поправь `TILES` в `make_textures.py` и перезапусти скрипт.
