#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 메인 배치 상수와 원본 글꼴 폭을 읽는다. 게임/원문 몸체를 실행하지 않는다(698)."""
import argparse
import hashlib
import json
from pathlib import Path
import socket
import struct
import sys
from taff import TaffArchive

ROOT = Path(__file__).resolve().parents[1]  # 원문과 산출물을 찾는 저장소 루트다.
sys.path.insert(0, str(ROOT/'extracted/oracle-python'))
import pefile
import capstone

LABELS = ('Campaign','Multiplayer','Demo','Help','Edit','Credits','Options','Quit','Intro')  # 004d0120의 표시 항목이며 Intro는 별도 조건이다.
SIZES = ((640,480),(800,600),(1024,768),(1280,720),(1280,800),(1366,768))  # 현재 제품이 공개한 여섯 해상도다.


def main():
    """정적 원문과 글꼴의 독립 숫자를 새 보고서에만 기록하며 기존 보고서는 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    out = parser.parse_args().output.resolve()
    assert not out.exists(), '이전 보고서는 보존한다'
    binary = ROOT/'originals/Netstorm.exe'
    pe = pefile.PE(str(binary))
    gap, = struct.unpack('<f',pe.get_data(0x502314-0x400000,4))
    half, = struct.unpack('<f',pe.get_data(0x500ee4-0x400000,4))
    assert gap == 4.0 and half == 0.5
    path = Path('d/!Arial.normal.14.700.chfnt')
    loose = ROOT/'originals'/path
    if loose.exists():
        font = loose.read_bytes()
        origin = loose.relative_to(ROOT).as_posix()
    else:
        archive = TaffArchive(str(ROOT/'originals/netstorm.tarc'))
        entry = next(entry for entry in archive.entries if entry.name.replace('\\','/').strip('/').lower()==path.as_posix().lower())
        font, origin = archive.read(entry), 'originals/netstorm.tarc:'+entry.name
    assert font.startswith(b'BitmapFontData\x1a')
    advances = struct.unpack_from('<256i',font,36)
    widths = {label:12+sum(advances[byte] for byte in label.encode('ascii')) for label in LABELS}  # 원문 Stone Fit의 글리프 advance 합+12다.
    common = max(widths.values())
    assert common == 75
    machine = capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    native = [dict(address=f'{ins.address:08x}',mnemonic=ins.mnemonic,operands=ins.op_str) for ins in machine.disasm(pe.get_data(0x4ce940-0x400000,0x2b0),0x4ce940)]
    rows = [dict(width=w,height=h,buttonWidth=common,firstX=(w-(common*4+gap*3))*half,firstY=h//3+55,secondY=h//3+78) for w,h in SIZES]  # 004ce940의 일반8항목 배치식을 정적으로 평가한 값이며 원문 실행 관측은 아니다.
    record = dict(unit=698,host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),kind='static PE and original font audit',
                  gameExecutions=0,nativeBodyExecutions=0,gap=gap,half=half,fontSource=origin,fontSha256=hashlib.sha256(font).hexdigest(),fittedWidths=widths,
                  rows=rows,nativeInstructions=native,sourceSha256=hashlib.sha256(binary.read_bytes()).hexdigest())
    out.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(widths=widths,rows=rows),ensure_ascii=False))


if __name__ == '__main__':
    main()
