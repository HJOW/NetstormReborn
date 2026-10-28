#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Edit 비석→스펠 수명을 유지하면서 월드 도움말 그림의 임시 제외를 확인한다."""
import argparse
import shutil
from pathlib import Path

from PIL import Image
from cpp_bridge_ritual_runtime import GAME, ROOT, save, sha
from cpp_object_conversion_smoke import one
from cpp_spell_lifetime_smoke import LATE, SIZES, check_converted
from cpp_world_smoke import states


def run(output, source, original_picture):
    """두 화면비에서 실제 전환·도움말 프레임 도달·그림 제외·후속 삭제를 확인한다."""
    output.mkdir(parents=True, exist_ok=False)
    with Image.open(original_picture) as native:
        reference = native.convert('RGB').crop((340, 335, 420, 445)).tobytes()
    binary = sha(GAME)
    results = []
    for width, height in SIZES:  # 같은 원본 Fort의 별도 사본으로 16:9와 4:3을 순서대로 검사한다.
        data = output / f'data-{width}x{height}'
        shutil.copytree(source, data)
        directory = output / f'{width}x{height}'
        result = one(directory, data, width, height, LATE)
        assert sha(GAME) == binary
        observed = states(directory / 'states.tsv')
        checked = check_converted(directory / 'states.tsv', result['run'], width, height)
        camera = [int(value) for value in observed['restored-spell']['camera']]
        left, top = 112 * 16 - camera[0], 118 * 11 - camera[1]
        box = (left + 4, top + 6, left + 84, top + 116)
        with Image.open(directory / 'restored-spell.bmp') as image:
            actual = image.convert('RGB').crop(box).tobytes()
        assert actual != reference, '원본 도움말 그림이 월드에 남아 있습니다.'
        checked.update(binarySha256=binary, nativePictureSha256=sha(original_picture),
                       helpInteriorRect=box, helpPictureAbsent=True,
                       differentRgbBytes=sum(a != b for a, b in zip(actual, reference)),
                       currentFrameStillTwo=True, inheritedRegularsPreserved=True)
        save(directory / 'help-summary.json', checked)
        results.append(checked)
    assert sha(GAME) == binary
    result = dict(results=results, binarySha256=binary, helpPictureSuppressed=True,
                  originalFrameAndRegularsPreserved=True, attackFlyersNotTested=True)
    save(output / 'summary.json', result)
    return result


def main():
    """명시한 격리 사본만 사용하며 원본 폴더를 실행용 쓰기 경로로 받지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--original-picture', type=Path, required=True)
    args = parser.parse_args()
    source = args.data_root.resolve()
    assert not any(source == (ROOT / name).resolve() or (ROOT / name).resolve() in source.parents
                   for name in ('originals', 'originalCD'))
    assert {path.stem.casefold() for path in (source / 'd').glob('*.fort')} == {'dissolvedalliance'}
    result = run(args.output.resolve(), source, args.original_picture.resolve())
    print(f"스펠 도움말 그림 임시 제외·원래 예약·효과·삭제: {len(result['results'])}개 통과")


if __name__ == '__main__':
    main()
