#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제Host초안의누락group0입력과명시정수폭교정을전체이전4412근거에연결한다."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

# 생산부모/4412원본/기존2검사는불변이며새consumer입력만교정한다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY='cpppj/hist/damageablepo-fa8db'
TEST='cpppj/tests/DamageablePostDestroyHostTests.cpp'
REPORT=ROOT/'cpppj/damageablepost-host-group-lineage.json'
REPLACEMENTS=[
 ('residence.footX=residence.footY=3;','residence.footX=residence.footY=3;residence.group=0; // native와같은합성group0이주변피해Regular분기를선택한다.'),
 ('std::uint32_t>(60,6699,expected,0)','std::uint32_t>(std::uint16_t{60},6699U,expected,0U)')]


def pin(name):
    """한파일전체바이트/SHA를고정한다."""
    raw=(ROOT/name).read_bytes();return dict(path=name,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())


def previous():
    """이전whole소비자/도구/Root count연결은보존된TESTview로엄격히감사한다."""
    spec=importlib.util.spec_from_file_location('post_host_binding_before_group',ROOT/'tools/analyze_damageablepost_host_binding.py');old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old);original_pin=old.pin
    def historical(name):
        """consumer한파일만교정전whole TEST로연결하고original path는유지한다."""
        value=original_pin(f'{HISTORY}/{name}' if name==TEST else name);value['path']=name;return value
    old.pin=historical;old.verify()


def record():
    """기존전체자료/실패로그/직전history와현재consumer두줄교정을최초연결한다."""
    if REPORT.exists():raise RuntimeError('Post Host group successor 덮어쓰기 금지')
    previous();proof=dict(host='HJOW-Athlon',date='2026-10-14',new_native_unique=0,preserved_native_unique=4412,normal_unique=4412,assert_unique=0,
       old_files=[pin(path.relative_to(ROOT).as_posix()) for path in sorted((ROOT/HISTORY).rglob('*')) if path.is_file()],current_test=pin(TEST),
       previous_binding=pin('cpppj/damageablepost-host-binding.json'),tool=pin('tools/analyze_damageablepost_host_group_lineage.py'),
       replacements=REPLACEMENTS,production_changed=False,cpp_test_result='공유재빌드/실제Host1검사대기',
       limits=['초기types[89].group는기본NO_GROUP(-1)이었고원본조건group0/1 또는vortex를충족하지않아Regular를만들지않았다.',
       '입력에합성group0을명시해기존4412원본격자의주변피해분기를선택한다. 실제거주 구역의원래metadata가group0이라고주장하지않는다.',
       'tuple검사의60은명시uint16,6699/0은DWORD로지정해C4244를해소한다. 기대packed값은변경하지않는다.',
       '기존4412 fixture/원본도구/생산hcpp/기존2검사/보고서및Root초기count역사는불변이며새원본0이다.'])
    REPORT.write_text(json.dumps(proof,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """두정확문자열의역변환/전체이전history/현재consumer를대조한다."""
    proof=json.loads(REPORT.read_text(encoding='utf-8'));text=(ROOT/TEST).read_text(encoding='utf-8')
    # 각교정은정확히한곳이고두역치환뒤이전전체바이트가된다.
    for before,after in REPLACEMENTS:assert text.count(after)==1;text=text.replace(after,before,1)
    assert text.encode('utf-8')==(ROOT/f'{HISTORY}/{TEST}').read_bytes();assert not proof['production_changed'] and proof['new_native_unique']==0
    # 현재파일/이전전체자료의바이트SHA를모두엄격히검사한다.
    for value in [*proof['old_files'],proof['current_test'],proof['previous_binding'],proof['tool']]:assert pin(value['path'])==value,value['path']
    previous();print('Post Host group 감사: 기존4412불변/생산0/합성group0입력+명시정수폭2줄역변환')


# --record는새successor최초기록이고기본실행은현재전체감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',action='store_true')
    if parser.parse_args().record:record()
    verify()
