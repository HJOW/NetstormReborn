#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""GunProcess 삭제 가상표/기반 poison을 읽기 전용 정적 감사한다. native 관찰 수를 늘리지 않는다."""
import argparse
import json
import struct
from decomp_owner_oracle import ROOT,digest
import pefile

# 실제 가상표 슬롯과 빈 함수/기반 소멸자 bytes의 판본별 정적 계약이다.
SPEC={
 'originals':dict(binary='originals/Netstorm.exe',table=0x50b648,slots=[0x46a080,0x4437f0,0x46a090,0x40b380,0x40b380,0x4690f0,0x46b830,0x46a340],
  checks={0x46a080:'83e928',0x40b380:'c20400',0x41bd00:'b8c7cfffffc701ec195000894104894108c3'}),
 'originalCD':dict(binary='originalCD/NETSTORM.EXE',table=0x5010f8,slots=[0x436dc0,0x4139f0,0x436d90,0x434a60,0x413a10,0x434cb0,0x4342f0,0x434900],
  checks={0x434a60:'c20400',0x413a10:'c20400',0x48f570:'b8c7cfffffc701680d5000894104894108c3'}),
}
REPORT=ROOT/'cpppj/recovery-gun-lifetime-audit.json'


def generate():
    """실제 bytes와 내보낸 원문 SHA만 기록하며 코드를 실행하지 않는다."""
    editions={};files=[ROOT/'tools/analyze_gun_lifetime.py',ROOT/'tools/ghidra/gunlifetimeaudit-functions.json']
    for edition,item in SPEC.items():
        path=ROOT/item['binary'];pe=pefile.PE(str(path));image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase
        slots=list(struct.unpack('<8I',image[item['table']-base:item['table']-base+32]))
        if slots!=item['slots']:raise RuntimeError('Gun 삭제 가상표 변화: '+edition)
        for address,expected in item['checks'].items():
            if image[address-base:address-base+len(expected)//2].hex()!=expected:raise RuntimeError('Gun 정적몸체 변화: '+edition+' '+hex(address))
        editions[edition]=dict(inner_vtable=hex(item['table']),slots=[hex(value) for value in slots],pre_destroy_empty=True,parent_dying_empty=True,
            base_poison_form_parent=-12345,cleanup_in_gun_destructor=False)
        files.append(path);files.extend(ROOT/f'extracted/gunlifetimeaudit/{edition}/{name}' for name in ('creation.c','functions.tsv'))
    REPORT.write_text(json.dumps(dict(schema=1,host='VM-W11-CODEX',editions=editions,native_observations=0,os_calls=0,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in files},limitations=['정적 vtable/삭제 몸체 감사만 수행하며 Gun native2996에 관찰을 추가하지 않는다.','C++ Host Kill 검사는 실제 Form/Kernel 소유수명이다. native destructor poison 및 표적 감시/animation 전체 수명 대조로 확대하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('Gun 삭제 가상표/빈 통지/기반 poison 정적 감사 통과: 두 PE, 추가 native0')


def verify():
    """보존된 정적 감사의 SHA를 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('Gun 삭제 정적 감사 SHA 변화: '+name)
    if report['native_observations'] or report['os_calls']:raise RuntimeError('Gun 정적 감사 수치 오류')
    print('Gun 삭제 정적 감사 SHA 통과: 추가 native0')


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()
