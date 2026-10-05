#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""복원한 다리 검사 CLI의 모든 모양·회전을 두 원본 PE와 독립 타입 파서에 대조한다."""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
import pefile
from cpp_assets_smoke import read_data, expected_code
from taff import TaffArchive
from typefile import parse_type

# 저장소 루트와 결과 경로. 기존 게임 파일은 읽기만 한다.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT/'extracted/cpp-bridge-smoke/report.json'
# 원본 두 판본의 모양 표·회전 글자 표 주소.
TABLES = {'originals': (0x52f998,0x531590), 'originalCD': (0x514a80,0x516680)}
# 다리 모양과 레코드 크기.
PATTERNS, RECORD_SIZE = 26, 72


def digest(path):
    """검사 전후 실제 입력 파일 SHA-256을 구한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def expected(edition):
    """C++ 표/반복자를 쓰지 않고 PE 모양·회전 표와 실제 타입 코드로 출력 전체를 만든다."""
    root = ROOT/edition
    pe = pefile.PE(str(root/('Netstorm.exe' if edition=='originals' else 'NETSTORM.EXE')))
    pattern_address, side_address = TABLES[edition]
    table = pe.get_data(pattern_address-pe.OPTIONAL_HEADER.ImageBase, PATTERNS*RECORD_SIZE)
    turns = struct.unpack('<64I',pe.get_data(side_address-pe.OPTIONAL_HEADER.ImageBase,256))
    archive = TaffArchive(str(root/'netstorm.tarc'))
    definition = parse_type(read_data(root,archive,'d/bridge.type').decode('cp1252'))
    codes = [expected_code(label,flags) for label,flags,_images in definition.clusters]
    rows = []
    total = 0
    # 원본 순서로 각 모양을 읽는다.
    for shape in range(PATTERNS):
        record = table[shape*RECORD_SIZE:(shape+1)*RECORD_SIZE]
        weight,width,height = struct.unpack_from('<3i',record)
        total += weight
        rows.append(['Pattern',str(shape),str(weight),str(width),str(height)])
        # 원본의 네 회전을 각각 좌표 변환 식으로 계산한다.
        for rotation in range(4):
            cells = []
            # 원본 셀은 y*width+x 순서다. 빈 칸은 그림을 내지 않는다.
            for index in range(width*height):
                variation,side,_reserved,label = record[12+index*4:16+index*4]
                if side == ord('.'):
                    continue
                x,y = index % width,index // width
                x,y = [(x,y),(height-1-y,x),(width-1-x,height-1-y),(y,width-1-x)][rotation]
                code = bytes([turns[rotation*16+side-ord('A')],ord('P'),variation-ord('0'),0])
                frame = next(i for i,candidate in enumerate(codes) if candidate[:3]==code[:3])
                cells.append((y,x,frame,chr(code[0]),code[2],label-ord('a')))
            # C++는 회전 뒤 y/x 순회다. 원본 패턴 순서에 의존하지 않는 비교를 한다.
            for y,x,frame,side,number,label in sorted(cells):
                rows.append(['Cell',str(shape),str(rotation),str(frame),str(x),str(y),side,str(number),str(label)])
    return [['BridgePatterns',str(PATTERNS),str(total)]]+rows


def main():
    """검사 CLI만 실행하고 두 판본의 전체 입력 보호와 모양·프레임 결과를 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,default=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe')
    args = parser.parse_args()
    protected = {}
    # 두 원본 폴더의 기존 파일을 모두 해시한다. 게임은 구동하지 않는다.
    for edition in TABLES:
        for path in (ROOT/edition).rglob('*'):
            if path.is_file(): protected[path] = digest(path)
    report = {}
    try:
        # 새 C++의 정적 검사 명령만 실행한다.
        for edition in TABLES:
            command = [str(args.exe),'--inspect-bridges',str(ROOT/edition)]
            if edition == 'originalCD': command.append('--cd')
            result = subprocess.run(command,capture_output=True,timeout=60,check=True)
            rows = [line.split('\t') for line in result.stdout.decode('utf-8').splitlines()]
            assert rows == expected(edition), f'{edition} 다리 모양/프레임 출력 불일치'
            report[edition] = {'patterns':26,'rotations':104,'cells':sum(row[0]=='Cell' for row in rows),
                'weight_total':int(rows[0][2]),'output_sha256':hashlib.sha256(result.stdout).hexdigest()}
    finally:
        after = {}
        # 추가·삭제·바이트 변화 모두 검사한다.
        for edition in TABLES:
            for path in (ROOT/edition).rglob('*'):
                if path.is_file(): after[path] = digest(path)
        assert protected == after, '원본 파일 존재 상태/해시 변화'
    report['original_files_unchanged'] = True
    report['protected_files'] = len(protected)
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))


# 도구를 직접 실행할 때만 검사한다.
if __name__ == '__main__':
    main()
