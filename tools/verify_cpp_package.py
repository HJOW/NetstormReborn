#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""cpppj 배포 ZIP의 모든 항목을 저장소 원천과 SHA-256으로 대조한다(M8 최종 관문 도구).

사용법:
    python tools/verify_cpp_package.py <zip 경로> [--extract <빈 폴더>] [--json <보고서 경로>]

검사 내용:
  1. 항목 대부분을 원천(빌드 산출물/저장소/originals/originalCD)과 바이트 단위로 대조한다.
  2. 필수 항목(Netstorm.exe, R.exe, 글꼴+라이선스, 한국어 번역표/본문, 실행 배치 파일)이 있는지 확인한다.
  3. 금지 항목(사용자 상태 파일, 내려받은 지도 잔재)이 없는지 확인한다.
  4. 글꼴 파일이 Netstorm.exe 안에 내장되어 있지 않은지(글꼴 시그니처 0) 확인한다.
  5. BUILD_INFO.txt에 적힌 Netstorm.exe/R.exe SHA-256이 ZIP 안의 실제 바이트와 같은지 확인한다.
--extract를 주면 같은 ZIP을 빈 폴더에 풀어 파일 수/크기가 ZIP과 같은지도 확인한다.
(2026-11-11 M8 최종 관문을 K/S/B/L 단위 뒤에도 한 번에 되풀이하려고 신규 작성)
"""
import argparse
import hashlib
import json
import re
import sys
import zipfile
from pathlib import Path

# 저장소 최상위 폴더다. 이 스크립트는 tools/ 아래에 있다.
ROOT = Path(__file__).resolve().parents[1]
# ZIP 안에서 모든 항목을 감싸는 최상위 폴더 이름이다(package-cpppj.ps1의 $PackageRootName).
PACKAGE_ROOT = 'NetstormReborn-cpppj'
# 빌드 산출물 폴더다.
BUILD_BIN = ROOT / 'cpppj' / 'build' / 'bin' / 'Release'
# 반드시 있어야 하는 항목(패키지 루트 기준 상대 경로)이다.
REQUIRED = [
    'Netstorm.exe', 'R.exe', 'NetstormSmacker.dll', 'Netstorm.bat', 'Netstorm-SavedMode.bat', 'README.txt', 'BUILD_INFO.txt',
    'fonts/D2Coding-License.txt', 'lang/korean/xlat.korean',
    'game/netstorm.tarc', 'game/Netstorm.exe', 'game/d/setup.cfg',
]
# 있으면 안 되는 항목의 패턴이다. 사용자 상태 파일과 내려받은 지도 묶음 잔재다.
FORBIDDEN = [re.compile(p, re.I) for p in (r'(^|/)options\.cfg$', r'fullscreenStateFile\.dat$', r'\.php$', r'(^|/)(Thumbs\.db|desktop\.ini)$')]
# 글꼴 파일 시그니처다. Netstorm.exe에 있으면 글꼴 내장 위반이다.
FONT_SIGNATURES = {b'ttcf': 'TTC', b'wOF2': 'WOFF2', b'OTTO': 'OpenType/CFF', b'\x00\x01\x00\x00\x00': 'TrueType'}
# 실행 파일 이름 그대로 대조할 빌드 산출물이다.
BUILD_FILES = {'Netstorm.exe', 'R.exe', 'NetstormSmacker.dll'}
# 생성 텍스트(스크립트가 만든 내용)라 원천 대조 대신 비어 있지 않음만 확인하는 항목이다.
GENERATED = {'Netstorm.bat', 'Netstorm-SavedMode.bat', 'README.txt', 'BUILD_INFO.txt'}
# Visual C++ 런타임 DLL은 설치된 Visual Studio에서 복사되므로 이름과 PE 시그니처만 확인한다.
RUNTIME_DLL = re.compile(r'^(msvcp140|vcruntime140|concrt140)[a-z_0-9]*\.dll$', re.I)


def sha256_bytes(data):
    """바이트열의 SHA-256 16진 문자열(소문자)을 반환한다."""
    return hashlib.sha256(data).hexdigest()


def sha256_file(path):
    """파일을 청크로 읽어 SHA-256 16진 문자열(소문자)을 반환한다."""
    digest = hashlib.sha256()
    with open(path, 'rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def resolve_source(relative):
    """패키지 루트 기준 상대 경로에 대응하는 저장소 원천 경로(없으면 None)를 찾는다."""
    if relative in BUILD_FILES:
        return BUILD_BIN / relative
    if relative == 'LICENSE.txt':
        return ROOT / 'LICENSE'
    if relative.startswith('fonts/'):
        return ROOT / 'fonts' / relative[len('fonts/'):]
    if relative.startswith('lang/'):
        return ROOT / 'assets' / 'lang' / relative[len('lang/'):]
    if relative.startswith('third-party/'):
        rest = relative[len('third-party/'):]
        # 저장소 폴더 이름은 third_party이며 ZIP은 하이픈 이름을 쓴다.
        return ROOT / 'cpppj' / 'third_party' / rest
    if relative.startswith('game/'):
        rest = relative[len('game/'):]
        candidates = [ROOT / 'originals' / rest]
        if rest.startswith('MOVIE/'):  # 인트로 영상은 originalCD에서 읽어 game\MOVIE에 넣는다.
            candidates.append(ROOT / 'originalCD' / rest)
            candidates.append(ROOT / 'originalCD' / 'MOVIE' / rest[len('MOVIE/'):])
        if rest in ('Admin.server', 'Update.server', 'Guild.server'):  # 원천에 없으면 클론 호환 페이지를 동봉한다.
            candidates.append(ROOT / 'cpppj' / 'assets' / 'server' / rest)
        for candidate in candidates:
            if candidate.is_file():
                return candidate
        return None
    return None


def main():
    """ZIP 전체를 대조하고 결과를 출력/저장한다. 문제가 있으면 종료 코드 1이다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('zip', type=Path)
    parser.add_argument('--extract', type=Path, help='압축을 풀어 파일 수/크기를 확인할 빈 폴더')
    parser.add_argument('--json', type=Path, help='보고서를 저장할 경로')
    args = parser.parse_args()
    report = dict(zip=str(args.zip), entries=0, verified=0, generated=0, runtime=0, directories=0, mismatches=[], unmapped=[], missingRequired=[],
                  forbidden=[], fontSignaturesInExe=[], buildInfo={}, extract=None)
    with zipfile.ZipFile(args.zip) as archive:
        bad = archive.testzip()  # CRC 검사다.
        if bad:
            report['mismatches'].append('CRC 오류: ' + bad)
        names = [info.filename for info in archive.infolist()]
        report['entries'] = len(names)
        prefix = PACKAGE_ROOT + '/'
        present = set()
        for info in archive.infolist():
            if info.is_dir():
                report['directories'] += 1
                continue
            if not info.filename.startswith(prefix):
                report['unmapped'].append(info.filename + ' (패키지 루트 밖)')
                continue
            relative = info.filename[len(prefix):]
            present.add(relative)
            if any(pattern.search(relative) for pattern in FORBIDDEN):
                report['forbidden'].append(relative)
            data = archive.read(info)
            if relative in GENERATED:
                if not data.strip():
                    report['mismatches'].append(relative + ' 비어 있음')
                report['generated'] += 1
                continue
            if RUNTIME_DLL.match(relative):
                if data[:2] != b'MZ':
                    report['mismatches'].append(relative + ' PE 시그니처 아님')
                report['runtime'] += 1
                continue
            source = resolve_source(relative)
            if source is None or not source.is_file():
                report['unmapped'].append(relative)
                continue
            if sha256_bytes(data) != sha256_file(source):
                report['mismatches'].append(relative + ' != ' + str(source))
            else:
                report['verified'] += 1
        report['missingRequired'] = [name for name in REQUIRED if name not in present]
        # 글꼴 내장 검사: exe 안의 글꼴 시그니처 개수다(TrueType 시그니처는 우연 일치가 많아 TTC/WOFF2/OTF만 센다).
        exe = archive.read(prefix + 'Netstorm.exe')
        for signature, label in FONT_SIGNATURES.items():
            if label == 'TrueType':
                continue
            count = exe.count(signature)
            if count:
                report['fontSignaturesInExe'].append(f'{label}:{count}')
        # BUILD_INFO의 SHA-256과 실제 바이트를 대조한다.
        info_text = archive.read(prefix + 'BUILD_INFO.txt').decode('utf-8-sig', 'replace')
        for name in ('Netstorm.exe', 'R.exe'):
            actual = sha256_bytes(archive.read(prefix + name)).lower()
            recorded = actual in info_text.lower()
            report['buildInfo'][name] = dict(sha256=actual, recordedInBuildInfo=recorded)
            if not recorded:
                report['mismatches'].append('BUILD_INFO에 ' + name + ' SHA-256이 없음')
        if args.extract:
            args.extract.mkdir(parents=True, exist_ok=True)
            if any(args.extract.iterdir()):
                report['mismatches'].append('--extract 폴더가 비어 있지 않음')
            else:
                archive.extractall(args.extract)
                files = [path for path in args.extract.rglob('*') if path.is_file()]
                total = sum(path.stat().st_size for path in files)
                expected_files = sum(1 for info in archive.infolist() if not info.is_dir())
                expected_total = sum(info.file_size for info in archive.infolist() if not info.is_dir())
                report['extract'] = dict(files=len(files), expectedFiles=expected_files, bytes=total, expectedBytes=expected_total)
                if len(files) != expected_files or total != expected_total:
                    report['mismatches'].append('압축 해제 파일 수/크기가 ZIP과 다름')
    ok = not (report['mismatches'] or report['unmapped'] or report['missingRequired'] or report['forbidden'] or report['fontSignaturesInExe'])
    report['passed'] = ok
    text = json.dumps(report, ensure_ascii=False, indent=2)
    if args.json:
        args.json.write_text(text + '\n', encoding='utf-8')
    print(text if not ok else json.dumps({k: v for k, v in report.items() if k not in ('mismatches', 'unmapped', 'missingRequired', 'forbidden')}, ensure_ascii=False, indent=2))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
