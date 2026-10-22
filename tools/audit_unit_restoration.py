#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""워크샾25종의 실제 타입/생성자/가상 진입을 고정한다. 실행 연결 완료 판정 도구는 아니다."""
import hashlib
import json
import re
import socket
import sys
from pathlib import Path
import pefile
from shp import load_type_texts

# 원본 워크샾25종과 별도 캠페인용 썬 캐논을 혼동하지 않는다.
ROOT=Path(__file__).resolve().parents[1]
GROUPS={'sun':[71,72,78,111,133,136],'rain':[76,129,143,145,146,147,156],'wind':[75,137,139,140,141,142],'thunder':[77,79,131,135,148,149]}
REPORT=ROOT/'cpppj/unit-restoration-inventory-20261022.json'


def sha(path):
    """원본/자료/도구의 전체 바이트 근거다."""
    return hashlib.sha256((ROOT/path).read_bytes()).hexdigest()


def main():
    """원본 로드 순서/현재 생성자 레시피와 실제 PE의 진입 주소를 대조한다."""
    if '--verify' in sys.argv:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for p,h in facts['sha256'].items():assert sha(p)==h,p  # 바이트 근거가 바뀌면 재생성한다.
        assert len(facts['units'])==25;print('Unit inventory verified: 25');return
    constructor_path='cpppj/src/o/TypeConstructors.inc';recipe_path='cpppj/src/o/DerivedConstructors.inc';order_path='cpppj/src/o/TypeLoadOrder.cpp'
    constructors=[int(v,16) for v in re.findall(r'0x([0-9a-fA-F]+)U',(ROOT/constructor_path).read_text(encoding='utf-8').split('kPatchConstructors{')[1].split('};')[0])]
    recipes={int(a,16):int(b,16) for a,b in re.findall(r'\{0x([0-9a-fA-F]+)U,0x([0-9a-fA-F]+)U',(ROOT/recipe_path).read_text(encoding='utf-8').split('kCdRecipes')[0])}
    names=re.findall(r'"([a-zA-Z0-9]+)"',(ROOT/order_path).read_text(encoding='utf-8').split('kAssetTypes{')[1].split('};')[0])
    texts=load_type_texts(str(ROOT/'originals'));pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));rows=[]
    for theme,numbers in GROUPS.items():  # 사용자가 지정한 원소별6/7/6/6을 원본 타입 번호로 고정한다.
        for number in numbers:  # 생성자는 실행하지 않고 PE의 실제 가상 표를 읽는다.
            name=names[number-70];label=re.search(r'description\s*=\s*"([^"]+)"',texts[name.lower()]).group(1);ctor=constructors[number];table=recipes[ctor]
            entries={key:f'{int.from_bytes(pe.get_data(table-0x400000+offset,4),"little"):08x}' for key,offset in [('post_pop',32),('pre_destroy',20),('regular',92),('input',108)]}
            rows.append({'theme':theme,'type':number,'name':name,'description':label,'constructor':f'{ctor:08x}','vtable':f'{table:08x}',**entries})
    assert len(rows)==25 and len({r['type'] for r in rows})==25
    paths=['originals/Netstorm.exe','originals/netstorm.tarc',constructor_path,recipe_path,order_path,'tools/audit_unit_restoration.py','tools/shp.py','tools/taff.py']
    facts={'host':socket.gethostname(),'counts':{k:len(v) for k,v in GROUPS.items()},'meaning':'이 표는 원본 타입/주소 근거다. 개별 처리기 존재나 전체 플레이·캠페인 연결 완료를 의미하지 않는다.','units':rows,'extra_campaign_type':{'type':128,'name':'sunCannon'},'sha256':{p:sha(p) for p in paths}}
    REPORT.write_bytes((json.dumps(facts,ensure_ascii=False,indent=2)+'\n').encode('utf-8'));print('Unit inventory: 25, Sun Cannon extra')


if __name__=='__main__':main()
