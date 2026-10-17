#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 전체 ColorSort/팔레트 생성에서 커서의 파괴/불법/글자 그림자 세 표를 독립 관측한다."""
import argparse
import hashlib
import json
import socket
import struct
import sys
from pathlib import Path
from decomp_gumpbackground_oracle import BackgroundOracle, ROOT, CONTROLS, palette_inputs

# 원본 출력 세 표와 현재 명명 색 빨강 DWORD다. 게임/GUI 장치는 실행하지 않는다.
OUTPUTS=(0x555f38,0x556038,0x556338)
BAD_COLOR=0x5b5e58
FIXTURE=ROOT/'cpppj/tests/fixtures/preview-colors-x86.tsv'
REPORT=ROOT/'cpppj/preview-colors.json'

# 실제 바이트 SHA는 줄 끝을 정규화하지 않는다.
def sha(path):
    """기존 추출/PE/실행기/새 fixture의 실제 바이트를 고정한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generate():
    """출력 기대값은 계산하지 않고 전체 원본 생성기의 세 출력을 읽는다."""
    oracle=BackgroundOracle('originals');rows=[]
    # 실제 파일/혼합/중복 회색의 세 입력과 현재 빨강 색 번호를 명시 공급한다.
    for index,(logical,bad) in enumerate(zip(palette_inputs(),(189,13,255))):
        outputs=[]
        # 두 정밀도 모두 전체 ColorSort 생성과 전체43c010을 실행한다.
        for control in CONTROLS:
            oracle.mu.mem_write(BAD_COLOR,struct.pack('<I',bad));oracle.maps(logical,control)
            outputs.append(tuple(bytes(oracle.mu.mem_read(address,256)).hex() for address in OUTPUTS))
        if outputs[0]!=outputs[1]:raise RuntimeError('커서 색표 x87 정밀도 차이')
        rows.append([str(index),str(bad),struct.pack('<256I',*logical).hex(),*outputs[0]])
        print('원본 커서 색표 입력',index,'통과',flush=True)
    FIXTURE.write_text('# 전체 ColorSort/43c010의 파괴/불법/그림자 표 관측.\n'+'\n'.join('	'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    # 실제로 불러온 프로젝트 도구와 이번 원본 몸체만 해시로 고정한다.
    paths={Path(__file__).resolve(),ROOT/'originals/Netstorm.exe',ROOT/'originals/d/GIFCLOUD.COL',FIXTURE,*oracle.exports}
    for module in tuple(sys.modules.values()):
        path=getattr(module,'__file__',None)
        if path and Path(path).resolve().is_relative_to(ROOT/'tools'):paths.add(Path(path).resolve())
    report={'host':socket.gethostname(),'date':'2026-10-17','unit':135,'controls':list(CONTROLS),'rows':len(rows),'normalReturns':oracle.returns,'precisionDifferences':0,'instructions':oracle.instructions,'nativeCalls':dict(oracle.native_calls),'assertions':oracle.assertions,'boundaries':dict(oracle.boundaries),'files':{p.relative_to(ROOT).as_posix():sha(p) for p in sorted(paths)},'limits':['10.78 전체 팔레트 생성의 세256바이트 출력만 새 관측; 다른 여섯 표의 새 복원 주장은 아님','명명 색 빨강은 입력 DWORD, CRT/장치/게임/파일 캐시는 실행하지 않음','기존 gumpbackground 허용 몸체/스택/보존 레지스터/x87 균형/쓰기 감사를 그대로 사용']}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """직접 입력/실제 몸체/각256바이트/정상 반환과 두 제어값을 재확인한다."""
    report=json.loads(REPORT.read_text('utf-8'));rows=[line.split('	') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    for path,wanted in report['files'].items():
        if sha(ROOT/path)!=wanted:raise RuntimeError('커서 색표 SHA 불일치: '+path)
    if report['rows']!=3 or len(rows)!=3 or report['normalReturns']!=12 or report['controls']!=list(CONTROLS) or report['precisionDifferences'] or report['assertions']:raise RuntimeError('커서 색표 계수 오류')
    # 입력은 기존 원본 공급자에서만 재구성하고 출력은 바이트 폭만 확인한다.
    for index,(row,logical,bad) in enumerate(zip(rows,palette_inputs(),(189,13,255))):
        if row[:3]!=[str(index),str(bad),struct.pack('<256I',*logical).hex()] or len(row)!=6 or any(len(bytes.fromhex(value))!=256 for value in row[3:]):raise RuntimeError('커서 색표 입력/출력 오류')
    print('커서 색표 SHA/원본 입력 감사:',len(rows))


# 명시 명령으로만 새 관측 또는 기존 근거 감사를 선택한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()
