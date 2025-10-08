#!/usr/bin/env python3
"""Generate thesis plots from results_cleaned_forPAPER.csv without external deps."""
from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple
import zlib
import struct

Color = Tuple[int, int, int]

FONT: Dict[str, Sequence[str]] = {
    ' ': (
        "00000",
        "00000",
        "00000",
        "00000",
        "00000",
        "00000",
        "00000",
    ),
    '0': (
        "01110",
        "10001",
        "10011",
        "10101",
        "11001",
        "10001",
        "01110",
    ),
    '1': (
        "00100",
        "01100",
        "00100",
        "00100",
        "00100",
        "00100",
        "01110",
    ),
    '2': (
        "01110",
        "10001",
        "00001",
        "00010",
        "00100",
        "01000",
        "11111",
    ),
    '3': (
        "11110",
        "00001",
        "00001",
        "01110",
        "00001",
        "00001",
        "11110",
    ),
    '4': (
        "00010",
        "00110",
        "01010",
        "10010",
        "11111",
        "00010",
        "00010",
    ),
    '5': (
        "11111",
        "10000",
        "11110",
        "00001",
        "00001",
        "10001",
        "01110",
    ),
    '6': (
        "00110",
        "01000",
        "10000",
        "11110",
        "10001",
        "10001",
        "01110",
    ),
    '7': (
        "11111",
        "00001",
        "00010",
        "00100",
        "01000",
        "01000",
        "01000",
    ),
    '8': (
        "01110",
        "10001",
        "10001",
        "01110",
        "10001",
        "10001",
        "01110",
    ),
    '9': (
        "01110",
        "10001",
        "10001",
        "01111",
        "00001",
        "00010",
        "01100",
    ),
    'A': (
        "01110",
        "10001",
        "10001",
        "11111",
        "10001",
        "10001",
        "10001",
    ),
    'B': (
        "11110",
        "10001",
        "10001",
        "11110",
        "10001",
        "10001",
        "11110",
    ),
    'C': (
        "01110",
        "10001",
        "10000",
        "10000",
        "10000",
        "10001",
        "01110",
    ),
    'D': (
        "11100",
        "10010",
        "10001",
        "10001",
        "10001",
        "10010",
        "11100",
    ),
    'E': (
        "11111",
        "10000",
        "10000",
        "11110",
        "10000",
        "10000",
        "11111",
    ),
    'F': (
        "11111",
        "10000",
        "10000",
        "11110",
        "10000",
        "10000",
        "10000",
    ),
    'G': (
        "01110",
        "10001",
        "10000",
        "10111",
        "10001",
        "10001",
        "01111",
    ),
    'H': (
        "10001",
        "10001",
        "10001",
        "11111",
        "10001",
        "10001",
        "10001",
    ),
    'I': (
        "01110",
        "00100",
        "00100",
        "00100",
        "00100",
        "00100",
        "01110",
    ),
    'J': (
        "00111",
        "00010",
        "00010",
        "00010",
        "10010",
        "10010",
        "01100",
    ),
    'K': (
        "10001",
        "10010",
        "10100",
        "11000",
        "10100",
        "10010",
        "10001",
    ),
    'L': (
        "10000",
        "10000",
        "10000",
        "10000",
        "10000",
        "10000",
        "11111",
    ),
    'M': (
        "10001",
        "11011",
        "10101",
        "10001",
        "10001",
        "10001",
        "10001",
    ),
    'N': (
        "10001",
        "11001",
        "10101",
        "10011",
        "10001",
        "10001",
        "10001",
    ),
    'O': (
        "01110",
        "10001",
        "10001",
        "10001",
        "10001",
        "10001",
        "01110",
    ),
    'P': (
        "11110",
        "10001",
        "10001",
        "11110",
        "10000",
        "10000",
        "10000",
    ),
    'Q': (
        "01110",
        "10001",
        "10001",
        "10001",
        "10101",
        "10010",
        "01101",
    ),
    'R': (
        "11110",
        "10001",
        "10001",
        "11110",
        "10100",
        "10010",
        "10001",
    ),
    'S': (
        "01111",
        "10000",
        "10000",
        "01110",
        "00001",
        "00001",
        "11110",
    ),
    'T': (
        "11111",
        "00100",
        "00100",
        "00100",
        "00100",
        "00100",
        "00100",
    ),
    'U': (
        "10001",
        "10001",
        "10001",
        "10001",
        "10001",
        "10001",
        "01110",
    ),
    'V': (
        "10001",
        "10001",
        "10001",
        "10001",
        "01010",
        "01010",
        "00100",
    ),
    'W': (
        "10001",
        "10001",
        "10001",
        "10101",
        "10101",
        "10101",
        "01010",
    ),
    'X': (
        "10001",
        "01010",
        "00100",
        "00100",
        "00100",
        "01010",
        "10001",
    ),
    'Y': (
        "10001",
        "01010",
        "00100",
        "00100",
        "00100",
        "00100",
        "00100",
    ),
    'Z': (
        "11111",
        "00001",
        "00010",
        "00100",
        "01000",
        "10000",
        "11111",
    ),
    ':': (
        "00000",
        "00100",
        "00100",
        "00000",
        "00100",
        "00100",
        "00000",
    ),
    '.': (
        "00000",
        "00000",
        "00000",
        "00000",
        "00000",
        "00110",
        "00110",
    ),
    '-': (
        "00000",
        "00000",
        "00000",
        "01110",
        "00000",
        "00000",
        "00000",
    ),
    '/': (
        "00001",
        "00010",
        "00100",
        "01000",
        "10000",
        "00000",
        "00000",
    ),
}


@dataclass
class Canvas:
    width: int
    height: int
    pixels: List[List[Color]]

    @classmethod
    def create(cls, width: int, height: int, color: Color = (255, 255, 255)) -> "Canvas":
        return cls(width, height, [[color for _ in range(width)] for _ in range(height)])

    def set_pixel(self, x: int, y: int, color: Color) -> None:
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y][x] = color

    def fill_rect(self, x0: int, y0: int, x1: int, y1: int, color: Color) -> None:
        if x0 > x1:
            x0, x1 = x1, x0
        if y0 > y1:
            y0, y1 = y1, y0
        for y in range(max(0, y0), min(self.height, y1)):
            row = self.pixels[y]
            for x in range(max(0, x0), min(self.width, x1)):
                row[x] = color

    def stroke_rect(self, x0: int, y0: int, x1: int, y1: int, color: Color) -> None:
        self.fill_rect(x0, y0, x1, y0 + 1, color)
        self.fill_rect(x0, y1 - 1, x1, y1, color)
        self.fill_rect(x0, y0, x0 + 1, y1, color)
        self.fill_rect(x1 - 1, y0, x1, y1, color)

    def draw_text(self, x: int, y: int, text: str, color: Color, scale: int = 2) -> None:
        cursor_x = x
        for ch in text.upper():
            pattern = FONT.get(ch, FONT[' '])
            for row_idx, row in enumerate(pattern):
                for col_idx, bit in enumerate(row):
                    if bit == '1':
                        for dy in range(scale):
                            for dx in range(scale):
                                self.set_pixel(cursor_x + col_idx * scale + dx, y + row_idx * scale + dy, color)
            cursor_x += (len(pattern[0]) + 1) * scale

    def to_png(self, path: Path) -> None:
        raw = bytearray()
        for row in self.pixels:
            raw.append(0)
            for r, g, b in row:
                raw.extend((r, g, b))
        compressed = zlib.compress(bytes(raw), 9)
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('wb') as fh:
            fh.write(b"\x89PNG\r\n\x1a\n")
            fh.write(_png_chunk(b'IHDR', struct.pack('>IIBBBBB', self.width, self.height, 8, 2, 0, 0, 0)))
            fh.write(_png_chunk(b'IDAT', compressed))
            fh.write(_png_chunk(b'IEND', b''))


def _png_chunk(tag: bytes, data: bytes) -> bytes:
    return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)


def read_numeric_columns(csv_path: Path) -> Dict[str, List[float]]:
    data: Dict[str, List[float]] = {}
    with csv_path.open(newline='') as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            for key, value in row.items():
                if value is None:
                    continue
                value = value.strip()
                if not value:
                    continue
                try:
                    numeric = float(value)
                except ValueError:
                    continue
                data.setdefault(key, []).append(numeric)
    return data


def percentile(sorted_values: Sequence[float], q: float) -> float:
    if not sorted_values:
        raise ValueError('Empty sequence')
    if q <= 0:
        return sorted_values[0]
    if q >= 100:
        return sorted_values[-1]
    pos = (len(sorted_values) - 1) * q / 100.0
    lower = math.floor(pos)
    upper = math.ceil(pos)
    if lower == upper:
        return sorted_values[int(pos)]
    fraction = pos - lower
    return sorted_values[lower] + (sorted_values[upper] - sorted_values[lower]) * fraction


def format_label(name: str) -> str:
    return name.replace('_', ' ').upper()


def draw_histograms(canvas: Canvas, columns: Sequence[str], data: Dict[str, List[float]], area: Tuple[int, int, int, int]) -> None:
    left, top, right, bottom = area
    heading_color = (33, 33, 33)
    axis_color = (120, 120, 120)
    bar_color = (79, 129, 189)
    row_height = (bottom - top) // max(1, len(columns))
    for idx, column in enumerate(columns):
        values = data[column]
        row_top = top + idx * row_height
        chart_top = row_top + 40
        chart_bottom = min(bottom, row_top + row_height - 20)
        chart_left = left + 100
        chart_right = right - 40
        canvas.draw_text(chart_left, row_top + 5, format_label(column), heading_color, scale=2)
        canvas.stroke_rect(chart_left, chart_top, chart_right, chart_bottom, axis_color)
        if not values:
            continue
        min_val = min(values)
        max_val = max(values)
        if math.isclose(min_val, max_val):
            max_val = min_val + 1
        bins = 20
        counts = [0 for _ in range(bins)]
        span = max_val - min_val
        for value in values:
            ratio = (value - min_val) / span
            index = min(bins - 1, max(0, int(ratio * bins)))
            counts[index] += 1
        max_count = max(counts) or 1
        bin_width = (chart_right - chart_left) / bins
        for bin_idx, count in enumerate(counts):
            bar_height = 0 if max_count == 0 else int((count / max_count) * (chart_bottom - chart_top - 4))
            x0 = int(chart_left + bin_idx * bin_width) + 1
            x1 = int(chart_left + (bin_idx + 1) * bin_width) - 1
            y0 = chart_bottom - bar_height
            canvas.fill_rect(x0, y0, max(x0 + 1, x1), chart_bottom - 1, bar_color)
        avg_value = sum(values) / len(values)
        label = f"AVG: {avg_value:.2f}"
        canvas.draw_text(chart_right - 200, row_top + 5, label, heading_color, scale=2)


def draw_boxplots(canvas: Canvas, columns: Sequence[str], data: Dict[str, List[float]], area: Tuple[int, int, int, int]) -> None:
    left, top, right, bottom = area
    heading_color = (33, 33, 33)
    axis_color = (120, 120, 120)
    box_color = (203, 75, 75)
    whisker_color = (60, 60, 60)

    valid_columns = [column for column in columns if data[column]]
    if not valid_columns:
        return
    all_values = [value for column in valid_columns for value in data[column]]
    min_val = min(all_values)
    max_val = max(all_values)
    if math.isclose(min_val, max_val):
        max_val = min_val + 1
    span = max_val - min_val
    plot_left = left + 120
    plot_right = right - 80
    plot_top = top + 20
    plot_bottom = bottom - 80
    canvas.stroke_rect(plot_left, plot_top, plot_right, plot_bottom, axis_color)
    canvas.draw_text(plot_left, top - 10 if top > 20 else 5, "RUNTIME BOXPLOT", heading_color, scale=2)

    column_width = (plot_right - plot_left) / max(1, len(valid_columns))
    for idx, column in enumerate(valid_columns):
        values = sorted(data[column])
        q1 = percentile(values, 25)
        median = percentile(values, 50)
        q3 = percentile(values, 75)
        iqr = q3 - q1
        lower_whisker = max(min(values), q1 - 1.5 * iqr)
        upper_whisker = min(max(values), q3 + 1.5 * iqr)

        x_center = plot_left + column_width * (idx + 0.5)
        half_width = max(10, int(column_width / 4))

        def y_for(value: float) -> int:
            return int(plot_bottom - (value - min_val) / span * (plot_bottom - plot_top))

        y_q1 = y_for(q1)
        y_q3 = y_for(q3)
        y_med = y_for(median)
        y_low = y_for(lower_whisker)
        y_high = y_for(upper_whisker)

        canvas.fill_rect(int(x_center - half_width), y_q3, int(x_center + half_width), y_q1, box_color)
        canvas.fill_rect(int(x_center - half_width), y_med, int(x_center + half_width), y_med + 2, whisker_color)
        canvas.fill_rect(int(x_center), y_high, int(x_center) + 1, y_q3, whisker_color)
        canvas.fill_rect(int(x_center), y_q1, int(x_center) + 1, y_low, whisker_color)
        canvas.fill_rect(int(x_center - half_width), y_high, int(x_center + half_width), y_high + 1, whisker_color)
        canvas.fill_rect(int(x_center - half_width), y_low, int(x_center + half_width), y_low + 1, whisker_color)

        label = format_label(column)
        canvas.draw_text(int(x_center - half_width), plot_bottom + 20, label, heading_color, scale=2)
        canvas.draw_text(int(x_center - half_width), plot_bottom + 50, f"MED: {median:.2f}", heading_color, scale=2)


def select_columns(data: Dict[str, List[float]], needle: str) -> List[str]:
    cols = [name for name in data if needle in name.lower() and 'pct' not in name.lower()]
    return sorted(cols)


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Plot thesis results without external dependencies")
    parser.add_argument('--input', type=Path, default=Path('results/results_cleaned_forPAPER.csv'), help='Path to the CSV results file.')
    parser.add_argument('--output', type=Path, default=Path('docs/figs'), help='Directory to store generated figures.')
    args = parser.parse_args(argv)

    if not args.input.exists():
        raise SystemExit(f"CSV file not found: {args.input}")

    data = read_numeric_columns(args.input)
    delay_columns = select_columns(data, 'delay')
    time_columns = select_columns(data, 'time')

    if not delay_columns:
        print('No delay columns detected; nothing to plot.')
    else:
        canvas = Canvas.create(1200, 200 * len(delay_columns) + 120)
        canvas.draw_text(40, 20, 'DELAY HISTOGRAMS', (33, 33, 33), scale=3)
        draw_histograms(canvas, delay_columns, data, (20, 80, canvas.width - 20, canvas.height - 20))
        canvas.to_png(args.output / 'delay_hist.png')

    if not time_columns:
        print('No time columns detected; nothing to plot for runtime.')
    else:
        canvas = Canvas.create(1200, 600)
        draw_boxplots(canvas, time_columns, data, (20, 120, canvas.width - 20, canvas.height - 20))
        canvas.to_png(args.output / 'runtime_boxplot.png')

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
