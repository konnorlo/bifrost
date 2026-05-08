"""
Training sketch for TinyTimbreEncoder.

This is not a finished training script. It is a Codex-friendly scaffold.
Keep training outside the plugin runtime. Export to ONNX only after the DSP-only
plugin works.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import torch
import torch.nn as nn


class TinyTimbreEncoder(nn.Module):
    def __init__(self, class_count: int = 8, macro_count: int = 8, quality_count: int = 4):
        super().__init__()
        self.encoder = nn.Sequential(
            nn.Conv2d(1, 16, kernel_size=3, padding=1),
            nn.GELU(),
            nn.Conv2d(16, 32, kernel_size=3, stride=2, padding=1),
            nn.GELU(),
            nn.Conv2d(32, 32, kernel_size=3, stride=2, padding=1),
            nn.GELU(),
            nn.AdaptiveAvgPool2d((1, 1)),
            nn.Flatten(),
            nn.Linear(32, 64),
            nn.GELU(),
            nn.Linear(64, 16),
        )
        self.class_head = nn.Linear(16, class_count)
        self.macro_head = nn.Sequential(nn.Linear(16, macro_count), nn.Sigmoid())
        self.quality_head = nn.Sequential(nn.Linear(16, quality_count), nn.Sigmoid())

    def forward(self, mel_patch: torch.Tensor):
        embedding = self.encoder(mel_patch)
        return {
            "embedding": embedding,
            "class_logits": self.class_head(embedding),
            "macro_init": self.macro_head(embedding),
            "quality": self.quality_head(embedding),
        }


def export_dummy_onnx(path: Path) -> None:
    model = TinyTimbreEncoder().eval()
    dummy = torch.randn(1, 1, 64, 16)
    torch.onnx.export(
        model,
        dummy,
        str(path),
        input_names=["mel_patch"],
        output_names=["embedding", "class_logits", "macro_init", "quality"],
        opset_version=18,
        dynamic_axes=None,
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--export-dummy", type=Path)
    args = parser.parse_args()
    if args.export_dummy:
        export_dummy_onnx(args.export_dummy)


if __name__ == "__main__":
    main()
