#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Win32 격리 관측기로 원본 기하 함수를 실제 x87에서 실행하거나 원문 SHA를 감사한다."""
import argparse,collections,json,socket,subprocess
from pathlib import Path
from decomp_cursor_lifecycle import ROOT,digest
FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-target-hardware.tsv' # 실제 CPU 결과를 에뮬레이터 원문과 분리한다.
REPORT=ROOT/'cpppj/carrier-target-hardware.json' # 실제 관측 범위/차이와 보호 근거를 기록한다.
SOURCE=ROOT/'cpppj/tests/fixtures/carrier-target-x86.tsv' # 입력 순서와 비교용 두 정밀도 원문이다.


def rows(path):
    """원문을 고치지 않고 주석을 제외한 전체 행을 읽는다."""
    return [line.split('\t') for line in path.read_text('utf-8').splitlines() if line and not line.startswith('#')]


def compare(hardware,source):
    """실제 CPU와 에뮬레이터의 효과/좌표 차이를 계산해 따로 보존한다."""
    differences=[];keys=[]
    # 모든 실제 반환의 입력 번호와 두 제어어를 대조한다.
    for row in hardware:
        assert len(row)==5;index,control=map(int,row[:2]);assert control in (0x27f,0x37f);case=source[index];assert int(case[0].split(',')[0]) in (0,1);keys.append((index,control));offset=1 if control==0x27f else 4;state=case[offset+1].split(',');assert row[2]==case[offset]
        before=','.join(state[1:5]);status=int(row[4]);assert not status&0x3800
        if row[3]!=before:differences.append(dict(index=index,control=control,emulator=before,hardware=row[3],status=status))
    expected=[(i,c) for i,r in enumerate(source) if int(r[0].split(',')[0]) in (0,1) for c in (0x27f,0x37f)];assert keys==expected
    return differences


def main():
    """verify는 실행하지 않으며 관측 모드는 명시 helper를 원본 엔트리 없이 구동한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--helper',type=Path);args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 보호 원본과 현재 추적 소스/기대값의 전체 SHA를 검사한다.
        for path,want in report['sha256'].items():assert digest(ROOT/path)==want,path
        hardware=rows(FIXTURE);assert len(hardware)==report['normalReturns'];assert compare(hardware,rows(SOURCE))==report['emulatorDifferences'];print('실제 x87 기하 감사:',len(hardware),'정상 반환/',len(report['emulatorDifferences']),'에뮬레이터 차이');return
    if args.helper is None:parser.error('관측에는 --helper로 별도 Win32 관측기를 지정해야 합니다')
    assert digest(ROOT/'originals/Netstorm.exe')=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
    result=subprocess.run([str(args.helper.resolve()),str(ROOT/'originals/Netstorm.exe'),str(SOURCE)],capture_output=True,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0),check=True);assert not result.stderr
    hardware=[line.split('\t') for line in result.stdout.decode('ascii').splitlines()];differences=compare(hardware,rows(SOURCE));assert len(hardware)==2040 and len(differences)==36
    FIXTURE.write_text('# inputIndex control effects outputX,outputY,invalidX,invalidY statusWord\n'+'\n'.join('\t'.join(row) for row in hardware)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/native_carrier_geometry.cpp','tools/carrier-geometry-hardware/CMakeLists.txt','tools/decomp_carrier_target_hardware.py','tools/decomp_carrier_target.py','cpppj/tests/fixtures/carrier-target-x86.tsv','cpppj/tests/fixtures/carrier-target-hardware.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),entries=['0041d400','0041d530','0041ce90'],normalReturns=len(hardware),uniqueInputs=len(hardware)//2,emulatorDifferences=differences,helperExecutableObservedSha256=digest(args.helper),controls=[0x27f,0x37f],boundaries='원본 PE의 기하 세 함수와 상수/실패점을 실제 Win32 x87에서 실행했다. 보호 파일은 읽기 전용이고 별도 helper 전용 큰 섹션에 복사한다. 원본 엔트리/import/게임 초기화/창은 실행하지 않는다. 진단만 명시 callback이다. 반환 포인터/입력 사각형/출력 양쪽 canary/제어어·TOP0과 복사 이미지 전체의 허용2DWORD 외 불변을 검사한다. helper 자체의 Win32 프로세스는 실행한다. 18개입력/두정밀도36회에서 Unicorn의 FLD/FSTP signaling NaN quiet 누락을 기록하며 이전 관측은 바꾸지 않는다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('실제 x87 관측',len(hardware),'정상 반환/에뮬레이터 차이',len(differences))


if __name__=='__main__':main()
