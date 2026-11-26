#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일본어 공식 본문의 번역 범위·명령·진행 조건·치환·링크 보존을 창 없이 검사한다."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re

# 번역 자료와 비교할 한국어 공식43파일은 같은 저장소의 읽기 전용 자산이다.
ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/lang/korean/d'
TARGET = ROOT / 'assets/lang/japanese/d'
# 첫 인자만 화면 라벨인 명령이며 나머지 명령의 숫자/인자는 전부 보존한다.
LABEL_COMMANDS = {'Button', 'ButtonInstant', 'Checked', 'Menu', 'Input'}


def visible_source(text):
    """줄 단위 설명 주석만 제외하고 실제 스크립트/화면 문자열을 반환한다."""
    return '\n'.join(line for line in text.split('\n') if not line.lstrip().startswith('//'))


def image_locations(text):
    """이미지의 순서와 소속 섹션/도움말 앵커를 기록해 번역 중 이동·누락도 찾는다."""
    result = []
    section = ''
    for line in text.split('\n'):
        # 그림이 속한 안내/도움말 주제를 유지하며 표기 라벨은 비교하지 않는다.
        if re.fullmatch(r'\s*\[[^\n]+\]\s*', line):
            section = line.strip()
        anchor = re.search(r'<a\s+name="([^"]*)"', line, re.I)
        if anchor:
            section = anchor[1]
        for picture in re.findall(r'<(?:img|picture|!)[^>]*>', line, re.I):
            result.append((section, picture))
    return result


def signatures(text, kind):
    """번역 가능한 라벨과 문장을 제외한 구조를 원문 순서 또는 중복 개수로 수집한다."""
    text = visible_source(text.replace('\r', ''))
    sections = re.findall(r'^\s*(\[[^\n]+\])\s*$', text, re.M)
    commands = []
    for key, value in re.findall(r'(?:^|>)[ \t]*\$(\w+)=(.*)$', text, re.M):
        # 원본 훈련 대본의 <p>$Button도 실행되는 명령이므로 줄 첫머리와 동일하게 대조한다.
        # 라벨 뒤의 실제 명령/인자/완료·해금 조건은 같은 바이트여야 한다.
        tail = value.split(',', 1)[1] if key in LABEL_COMMANDS and ',' in value else value
        commands.append((key, tail))
    macros = []
    for value in re.findall(r'\{([^{}]+)\}', text):
        # 기본 표시 문자열만 번역할 수 있고 치환 키와 기본값 유무는 보존한다.
        macros.append(value.split('|', 1)[0] + ('|' if '|' in value else ''))
    assignments = []
    for key, value in re.findall(r'^\s*([\w ]+?)\s*=\s*(.*?)\s*$', text, re.M):
        # config의 값은 용어이므로 키를 대조하고, 미션 실행 설정은 title 외의 값을 보존한다.
        assignments.append((key, '' if kind == 'config' or key == 'title' else value))
    return dict(sections=sections, commands=commands, macros=Counter(macros), assignments=assignments,
                scriptTags=Counter(re.findall(r'<\$[^>]*>', text)),
                links=Counter(re.findall(r'<a\s+(?:name|href)="[^"]*"', text, re.I)),
                images=image_locations(text))


def inspect(require_all=False, original_archive=None):
    """번역 구조를 한국어 번들과 비교하며 선택한 원본 아카이브도 읽기 전용으로 대조한다."""
    originals = {path.stem: path for path in SOURCE.glob('*.korean')}
    translated = sorted(TARGET.glob('*.japanese'))
    errors = []
    records = []
    kanji = set()
    # 원본 영어를 선택하면 기존 한국어 번역과 독립된 실행 명령 보존 근거를 남긴다.
    english = {}
    original_compared = 0
    if original_archive is not None:
        from taff import TaffArchive
        archive = TaffArchive(original_archive)
        for entry in archive.entries:
            # 공식 파일만 비교하고 원본 자료와 추출 디렉터리에는 쓰지 않는다.
            logical = Path(entry.name.replace('\\', '/').lstrip('/'))
            if logical.suffix.lower() == '.english' and logical.stem.lower() in originals:
                english[logical.stem.lower()] = archive.read(entry).decode('cp1252')
    for path in translated:
        # 사용자 맵/새 basename이 공식 번들에 섞이면 즉시 보고한다.
        if path.stem not in originals:
            errors.append(f'미등록 공식 이름: {path.name}')
            continue
        raw = path.read_bytes()
        if not raw.startswith(b'\xef\xbb\xbf'):
            errors.append(f'UTF-8 BOM 누락: {path.name}')
        target = raw.decode('utf-8-sig')
        source = originals[path.stem].read_text(encoding='utf-8-sig')
        left, right = signatures(source, path.stem), signatures(target, path.stem)
        for category in left:
            # 첫 차이의 구조 종류를 남기며 다른 파일/종류 검사도 계속한다.
            if left[category] != right[category]:
                errors.append(f'{path.name}: {category} 불일치')
        if original_archive is not None:
            original = english.get(path.stem)
            if original is None:
                errors.append(f'{path.name}: 원본 영어 파일 누락')
            else:
                original_compared += 1
                for category, expected in signatures(original, path.stem).items():
                    # 영어 표시 문장만 제외하고 원본 명령·조건·수치·그림까지 독립 대조한다.
                    if expected != right[category]:
                        errors.append(f'{path.name}: 원본 영어 {category} 불일치')
        prose = visible_source(target)
        if re.search(r'[가-힣]', prose):
            errors.append(f'{path.name}: 표시 본문에 한국어 잔여')
        kanji.update(character for character in prose if '\u4e00' <= character <= '\u9fff')
        records.append(dict(file=path.name, bytes=len(raw), sections=len(right['sections']), links=sum(right['links'].values()),
                            images=len(right['images']), imageSections=dict(Counter(section for section, _ in right['images']))))
    missing = sorted(set(originals) - {path.stem for path in translated})
    if require_all and missing:
        errors.append(f'미번역 공식 파일 {len(missing)}개')
    if len(kanji) > 3946:
        errors.append(f'일본어 동적 한자 슬롯3946개 초과: {len(kanji)}')
    return dict(passed=not errors, translated=len(translated), total=len(originals), missing=missing,
                uniqueKanji=len(kanji), originalCompared=original_compared, files=records, errors=errors)


def main():
    """번역 중에는 부분 범위를 검사하고 최종 관문에서는 --require-all을 사용한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--require-all', action='store_true')
    parser.add_argument('--json', type=Path)
    parser.add_argument('--original-archive', type=Path, help='선택한 원본 TAFF의 영어 실행 구조도 읽기 전용으로 비교')
    args = parser.parse_args()
    result = inspect(args.require_all, args.original_archive)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
