#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_font_chars.py —— 检查字符串里的字符是否都在某个 XBF 字库 bin 中。

背景：LVGL 字库（Lvgl Font Tool V0.5 生成）为省 flash 只收 UI 需要的字，
比如 SCH_16 里没有"你好世界"，屏幕上会显示空白/方框且难排查。
烧录进 W25Q64 的内容 == 这里的 .bin（烧录时逐字节校验过），
所以查 bin 就等价于查 flash 里的字库。

XBF 格式（本工具）：
  头 8 字节: uint16 min, uint16 max, uint8 bpp, 3B 保留
  之后每个 unicode（min..max）占 4 字节 uint32 位置表, 0 表示缺字

用法:
  python check_font_chars.py <字体.bin> <文本...>
  示例:
    python check_font_chars.py ch_font/my_font_SCH_16.bin 数据显示 你好世界 设置 音乐 时钟
    python check_font_chars.py en_font/my_font_ENG_BT_16.bin "hello world"
  不带文本: 列出该字库包含的全部字符(分类统计 + 可显示列表)
"""
import struct
import sys


def load_charset(path):
    with open(path, "rb") as f:
        data = f.read()
    if len(data) < 8:
        raise SystemExit(f"文件太小(<8B), 不是 XBF 字库: {path}")
    mn, mx, bpp = struct.unpack_from("<HHB", data, 0)
    entries = {}  # cp -> pos
    for cp in range(mn, mx + 1):
        off = 8 + (cp - mn) * 4
        if off + 4 > len(data):
            break
        (pos,) = struct.unpack_from("<I", data, off)
        if pos != 0:
            entries[cp] = pos
    return (mn, mx, bpp), entries, len(data)


def show(cp):
    if cp < 0x80:
        return chr(cp)
    return f"U+{cp:04X}"


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    path, texts = sys.argv[1], sys.argv[2:]
    (mn, mx, bpp), entries, fsize = load_charset(path)
    n = len(entries)

    print(f"字库: {path}")
    print(f"  范围 min=0x{mn:04X} max=0x{mx:04X} bpp={bpp} 含字 {n} 个  bin大小={fsize}B")

    # ---- 有文本: 逐字检查 ----
    if texts:
        for text in texts:
            missing = []
            present = []
            for ch in text:
                if ch == " ":
                    present.append("空格")
                    continue
                cp = ord(ch)
                (present if cp in entries else missing).append(ch)
            status = "OK" if not missing else f"缺 {len(missing)} 字"
            print(f"\n  [{status}] {text!r}")
            if missing:
                print(f"    缺字: {' '.join(f'{c}({show(ord(c))})' for c in missing)}")
                print("    -> 用 Lvgl Font Tool 把这几个字加进字库重新生成再烧录")
            if present and not missing and len(text) > 1:
                print("    全在, 可直接用")
        return 0

    # ---- 无文本: 分类统计 ----
    cat = {"ASCII": 0, "汉字": 0, "FontAwesome": 0, "其他": 0}
    for cp in entries:
        if 0x20 <= cp < 0x7F:
            cat["ASCII"] += 1
        elif 0x4E00 <= cp <= 0x9FFF:
            cat["汉字"] += 1
        elif 0xF000 <= cp <= 0xF2FF:
            cat["FontAwesome"] += 1
        else:
            cat["其他"] += 1
    print(f"\n分类: " + "  ".join(f"{k}={v}" for k, v in cat.items()))

    ascii_ok = "".join(chr(cp) for cp in entries if 0x20 <= cp < 0x7F)
    print(f"ASCII字符: {ascii_ok!r}")
    han = sorted(cp for cp in entries if 0x4E00 <= cp <= 0x9FFF)
    print(f"汉字列表({len(han)}字): " + "".join(chr(cp) for cp in han))
    return 0


if __name__ == "__main__":
    sys.exit(main())
