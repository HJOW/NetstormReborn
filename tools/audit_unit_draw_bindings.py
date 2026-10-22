#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""25종/비행·성장 파생의 Draw 분배를 보호PE 가상 진입과 현재 소스에 대조한다."""
import argparse
import hashlib
import json
import re
import socket
from pathlib import Path
import pefile

ROOT=Path(__file__).resolve().parents[1]  # 원본과 현재 복원 소스의 공통 루트다.
REPORT=ROOT/'cpppj/unit-draw-bindings-20261023.json'  # 정적 진입 근거와 실제 표시 검증 범위를 구별한다.
SOURCES=('cpppj/src/o/RawPlainAssetDraw.cpp','cpppj/src/o/RawPlainUnitCapture.cpp','cpppj/src/o/RawPlainWorkshopCapture.cpp')  # 정확한 타입/VT 배열의 세 소스다.


def sha(path):
    """현재 바이트를 계산하며 원문 기대값이나 제품 출력은 만들지 않는다."""
    return hashlib.sha256((ROOT/path).read_bytes()).hexdigest()


def main():
    """정적 분배 근거를 생성하거나 동결된 주소/현재 구현의 SHA만 읽기 전용 검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert sha(path)==wanted,path  # 별도 실행 결과를 재관측하여 덮지 않는다.
        assert len(facts['workshop'])==25 and sum(row['route']=='plain' for row in facts['workshop'])==19 and len(facts['derived'])==4;print('25종 Draw 주소/SHA 확인');return
    inventory=json.loads((ROOT/'cpppj/unit-restoration-inventory-20261022.json').read_text(encoding='utf-8'));pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));pairs=set()
    for path in SOURCES:  # 이름이나 genus가 아닌 현재 소스의 정확한 두 필드를 읽는다.
        pairs.update((int(kind),int(table,16)) for kind,table in re.findall(r'\{(\d+),0x([a-f0-9]+)\}',(ROOT/path).read_text(encoding='utf-8')))
    def row(kind,table,name):
        """같은 가상 표의 현재 기본/공급/추가 그림 진입을 보호 이미지에서 읽는다."""
        methods={f'{off:x}':f'{int.from_bytes(pe.get_data(table-0x400000+off,4),"little"):08x}' for off in (0x78,0xa0,0xa4)};plain=(kind,table) in pairs
        if plain:assert methods['78']=='004acfe0' and methods['a4']=='0041c100' and methods['a0']=='0041c0f0',(kind,methods)
        return dict(type=kind,name=name,vtable=f'{table:08x}',route='plain' if plain else 'existing-special',methods=methods)
    workshop=[row(unit['type'],int(unit['vtable'],16),unit['name']) for unit in inventory['units']]
    assert {entry['type'] for entry in workshop if entry['route']=='existing-special'}=={71,72,75,76,77,131}
    derived=[row(kind,table,name) for kind,table,name in ((73,0x50a1e8,'Whirligig'),(138,0x50a050,'Dust Devil'),(144,0x50a388,"Man o'War"),(150,0x50e888,'Growing Ice Tower'))]
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','cpppj/unit-restoration-inventory-20261022.json',*SOURCES,'cpppj/src/o/RawPlainAssetDraw.h','cpppj/src/o/CMakeLists.txt','cpppj/src/client/RawCampaignFactoryDisplay.cpp','cpppj/src/client/GameWorld.cpp','cpppj/src/client/SpriteDraw.cpp','cpppj/src/client/OwnerColorMaps.cpp','cpppj/src/client/SpriteColorMaps.cpp','cpppj/tests/CampaignSessionTests.cpp','cpppj/tests/fixtures/spritedraw-x86.tsv','tools/audit_unit_draw_bindings.py')
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',nativeDraw='00498220',workshop=workshop,derived=derived,sha256={p:sha(p) for p in paths},limitations=['정적 주소/분배 근거이며 실제 화면/전체 수명 완료 판정은 아니다.','일반 Draw 본체의 기존 독립 spritedraw-x86.tsv와 새 실제25종 표시 검사를 별도 검증한다.','전체25종 게임 동작/사용자 GUI 플레이/전체 미션 및 Edit 완료 증거로 확대하지 않는다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('25종 Draw:',len(workshop),'일반',sum(entry['route']=='plain' for entry in workshop),'파생',len(derived))


if __name__=='__main__':main()
