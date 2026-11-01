#!/usr/bin/env python3

import re
import sys
from pathlib import Path


VARIABLE_PATTERN = re.compile(r"\$var\s+\S+\s+(\d+)\s+\S+\s+(.+?)\s+\$end$")


def main(arguments):
    if len(arguments) != 2:
        raise SystemExit("usage: make_gtkw.py <input.vcd> <output.gtkw>")

    vcd_path = Path(arguments[0])
    save_path = Path(arguments[1])
    signals = {1: [], 32: []}
    for line in vcd_path.read_text(encoding="utf-8").splitlines():
        match = VARIABLE_PATTERN.fullmatch(line)
        if match:
            width, name = match.groups()
            width = int(width)
            if width not in signals:
                signals[width] = []
            signals[width].append((name, width))

    if not any(signals.values()):
        raise ValueError(f"{vcd_path}: no VCD variables found")

    with save_path.open("w", encoding="utf-8") as save:
        save.write("[*] GTKWave Analyzer save file\n")
        save.write(f'[dumpfile] "{vcd_path.name}"\n')
        save.write("[timestart] 0\n")
        save.write("[treeopen] controller\n")
        for width, radix in ((1, "@28"), (32, "@24")):
            if signals[width]:
                save.write(f"{radix}\n")
                for signal, signal_width in signals[width]:
                    suffix = f"[{signal_width - 1}:0]" if signal_width > 1 else ""
                    save.write(f"controller.{signal}{suffix}\n")

    print(f"Wrote {save_path}")


if __name__ == "__main__":
    main(sys.argv[1:])
