#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 생성자83VT의 실제 객체메뉴+a8/+ac를 읽거나 읽기 전용으로 감사한다."""
import argparse,collections,json,socket,struct
from decomp_squid_selection import tables
from decomp_cursor_lifecycle import ROOT,digest,Oracle
TABLE=ROOT/'cpppj/src/client/ObjectMenuSlots.inc' #실제 PE의 두 DWORD를 보존한 생성 자료다.
REPORT=ROOT/'cpppj/object-menu-slots.json' #정적 읽기 범위와 보호/도구/생성 자료의 해시다.

def rows():
    """기존 독립 생성자VT 목록을 쓰고 실제 슬롯은 보호PE에서 새로 읽는다."""
    image=Oracle()
    return [(v,*struct.unpack('<II',image.mu.mem_read(v+0xa8,8))) for v,*_ in tables()]

def generated(values):
    """C++의원본주소데이터만작성하며호스트함수포인터로사용하지않는다."""
    return '// 보호10.78 생성자83VT의 실제+a8/+ac다. 호스트 함수 포인터가 아니다.\nconstexpr std::array<ObjectMenuSlot,83> kObjectMenuSlots{{\n'+''.join('    {'+','.join(f'0x{v:08x}U' for v in row)+'},\n' for row in values)+'}};\n'

def main():
    """최초 정적 관측을 기록하거나 현재 자료를 변경하지 않고 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();values=rows();text=generated(values);assert len(values)==83
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));assert report['slots']==[list(r) for r in values] and TABLE.read_text('utf-8')==text
        for path,want in report['sha256'].items():assert digest(ROOT/path)==want,path
        print('객체메뉴83VT 정적감사/실행0');return
    TABLE.write_text(text,encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','cpppj/src/o/DerivedConstructors.inc','tools/decomp_squid_selection.py','tools/decomp_object_menu_slots.py','cpppj/src/client/ObjectMenuSlots.inc')
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),slots=values,populateEntries={f'{a:08x}':n for a,n in collections.Counter(r[1] for r in values).items()},openEntries={f'{a:08x}':n for a,n in collections.Counter(r[2] for r in values).items()},normalReturns=0,boundaries='10.78의83생성자VT에서+a8/+ac두DWORD를정적으로읽었다. 새본문실행/게임/OS창실행은0이며CD메뉴슬롯대조도아니다. 기존생성자VT목록의독립출처를재사용하고새슬롯값은C++실행에서만들지않았다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('객체메뉴슬롯',len(values),'작성',len(report['populateEntries']),'열기',len(report['openEntries']))
if __name__=='__main__':main()
