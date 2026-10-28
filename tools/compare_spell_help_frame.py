#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""이미 저장한 원본 Edit 전환 캡처와 클론의 도움말 그림을 비교한다. 게임은 실행하지 않는다."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image

# 두 1024×768 캡처의 같은 월드 위치에 나타난 그림 영역이다. 끝 좌표는 포함하지 않는다.
HELP_RECT = (336, 329, 490, 453)
# 가장자리의 월드 그림/겹친 간헐천을 제외한 그림 내부 80×110 영역이다.
INNER_RECT = (340, 335, 420, 445)


def digest(path):
    """원시 이미지 전체 바이트의 SHA256을 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rgb(path):
    """비교용 RGB 픽셀만 읽고 원시 파일과 게임 자산을 수정하지 않는다."""
    with Image.open(path) as source:
        return source.convert('RGB')


def compare(native_dir, clone_path):
    """원본의 그림 표시·비표시·재표시를 기존 클론 그림과 대조해 보고한다."""
    clone = rgb(clone_path)
    if clone.size != (1024, 768):
        raise ValueError('클론 기준 캡처는 1024×768이어야 합니다.')
    reference = clone.crop(INNER_RECT).tobytes()
    outer = clone.crop(HELP_RECT).tobytes()
    paths = [native_dir / 'fast-turned.png']
    paths.extend(native_dir / f'fast-turned-{index}.png' for index in range(1, 10))
    rows = []
    for path in paths:  # 동일 세션의 입력 직후·후속 아홉 캡처를 시간순으로 비교한다.
        actual = rgb(path)
        if actual.size != clone.size:
            raise ValueError(f'원본 캡처 크기 불일치: {path}')
        inside = actual.crop(INNER_RECT).tobytes()
        outside = actual.crop(HELP_RECT).tobytes()
        rows.append(dict(path=str(path), sha256=digest(path),
                         helpInteriorMatches=inside == reference,
                         interiorDifferentRgbBytes=sum(a != b for a, b in zip(inside, reference)),
                         wholeRectDifferentRgbBytes=sum(a != b for a, b in zip(outside, outer))))
    matches = [row['helpInteriorMatches'] for row in rows]
    blink = any(any(matches[:index]) and any(matches[index + 1:])
                for index, value in enumerate(matches) if not value)
    if not blink:
        raise AssertionError('그림 내부가 일치하는 표시·비표시·재표시 증거를 찾지 못했습니다.')
    return dict(clone=dict(path=str(clone_path), sha256=digest(clone_path)),
                helpRect=HELP_RECT, interiorRect=INNER_RECT, interiorPixels=80 * 110,
                originalSamples=rows, originalHelpOnOffOnObserved=True,
                matchingSamples=sum(matches), nonmatchingSamples=len(matches) - sum(matches),
                wholeScreenEqualityClaimed=False, gamesLaunched=0)


def main():
    """명시한 기존 캡처만 비교하고 새 UTF-8 결과를 작성한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-dir', type=Path, required=True)
    parser.add_argument('--clone', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = compare(args.native_dir, args.clone)
    with args.output.open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(result, stream, ensure_ascii=False, indent=2)
        stream.write('\n')
    print(f"원본 도움말 그림 표시/비표시/재표시 확인: 내부 일치 {result['matchingSamples']}/10, 픽셀 {result['interiorPixels']}")


if __name__ == '__main__':
    main()
