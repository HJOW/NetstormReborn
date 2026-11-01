#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 대본의 명시 <$태그>와 송금 이름표를 실행 없이 읽어 후속 복원 범위를 좁힌다."""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re
import socket
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 원문 대본의 저장소다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile

REVIEW = {'addrandom','destroyat','startsmoke','lightning','createrandomguns'}  # 현재 일반 입력/실제 부모를 추가로 대조할 이름 후보다. 유무만으로 미구현을 단정하지 않는다.


def main():
    """원본 이름표/499개 파일의 태그를 읽고 새 지정 보고서에만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert not args.output.exists(),'이전 보고서는 덮어쓰지 않는다'
    binary=ROOT/'originals/Netstorm.exe'
    pe=pefile.PE(str(binary))
    names={}
    for entry in range(0x542b80,0x543500,8):  # 현재 표의 정렬된 포인터 쌍 중 보호 문자열/명령 전역 범위만 읽는다.
        pointer,glob=struct.unpack('<II',pe.get_data(entry-0x400000,8))
        if 0x400000<=pointer<0x540000 and 0x542700<=glob<0x542b80:
            name=pe.get_data(pointer-0x400000,100).split(b'\0')[0].decode('ascii')
            names[name.lower()]=dict(name=name,entry=f'{entry:08x}',globalAddress=f'{glob:08x}',command=f'{struct.unpack("<I",pe.get_data(glob-0x400000,4))[0]:08x}')
    files=sorted((ROOT/'originals/d').glob('*.english'))
    uses=collections.defaultdict(set)
    unmatched=collections.defaultdict(set)
    source_manifest=[]
    for path in files:  # 저장된 원문을 읽으며 프로세스/설정/원본에는 쓰지 않는다.
        raw=path.read_bytes()
        source_manifest.append((path.relative_to(ROOT).as_posix(),hashlib.sha256(raw).hexdigest()))
        for match in re.finditer(r'<\s*\$\s*([A-Za-z][A-Za-z0-9_]*)',raw.decode('latin-1')):  # 문자열 치환/Config 확장 전의 명시 태그만 센다.
            key=match[1].lower()
            (uses if key in names else unmatched)[key].add(path.name)
    referenced=[]
    for key,paths in sorted(uses.items()):  # 대소문자 차이를 합치고 전체 파일 수와 대표 경로를 기록한다.
        referenced.append(dict(**names[key],files=len(paths),examples=sorted(paths)[:6],status='일반 입력/부모 추가 대조 후보' if key in REVIEW else '기존 연결/이번 송금 이름 연결을 별도 구현 자료로 확인; 태그 목록 자체는 실행 근거가 아님'))
    money=[value for key,value in sorted(names.items()) if key in ('sendbucks','sendxbucks','send700bucks','sendallbucks','send300bucks','sendallbucks2')]
    assert len(money)==6
    record=dict(unit=687,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),kind='read-only PE and literal script inventory',gameExecutions=0,nativeExecutions=0,sourceFiles=len(files),scannedNameEntries=len(names),moneyNames=money,referenced=referenced,notInScannedCommandTable={name:sorted(paths)[:6] for name,paths in sorted(unmatched.items())},limits='499개 loose .english의 명시 <$태그>만 읽는다. TARC/Config 확장/동적 이름·Menu 둘째 인자·모든 HTML 태그의 검증이 아니며 알려진 태그와 미등록 오자도 구별해야 한다. REVIEW 후보는 누락 판정이 아니라 원문 부모 추가 대조 목록이다.',sourceManifestSha256=hashlib.sha256(json.dumps(source_manifest,ensure_ascii=False,separators=(',',':')).encode('utf-8')).hexdigest(),sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in ('originals/Netstorm.exe','tools/cpp_scenario_script_inventory.py','AGENTS.md')})
    args.output.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(files=len(files),entries=len(names),referenced=len(referenced),moneyNames=len(money),review=[row['name'] for row in referenced if row['name'].lower() in REVIEW])))


if __name__=='__main__':
    main()
