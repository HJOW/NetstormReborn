#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""부모534와 기존 Builder 소비자 골든2를 구분하고 소비자 검사 교정을 whole 출처에 연결한다."""
import argparse
import difflib
import hashlib
import importlib.util
import json
from pathlib import Path

# 부모/생산/534 결과는 그대로 두고 선택 소비자 검사만 교정한다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY='cpppj/hist/fortworldloa-075e2'
TEST='cpppj/tests/FortWorldLoadBindingTests.cpp'
REPORT=ROOT/'cpppj/fortworldload-consumer-lineage.json'
PREVIOUS='cpppj/fortworldload-binding.json'
GOLDEN='cpppj/fortworldload-builder-consumer.json'


def pin(name):
    """전체 바이트 크기/SHA를 고정한다."""
    raw=(ROOT/name).read_bytes();return dict(path=name,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())


def module(name,path):
    """기존 감사기를 수정 없이 읽어 해당 whole 출처 감사를 재사용한다."""
    spec=importlib.util.spec_from_file_location(name,ROOT/path);result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result


def previous():
    """이전 부모534/생산/전체52 의존성은 소비자 한파일의 역사 view만 지정해 감사한다."""
    old=module('fortworld_binding_before_consumer','tools/analyze_fortworldload_binding.py');original=old.pin
    def historical(name):
        """교정한 TEST만 전체 snapshot을 읽으며 원래 기록 path는 보존한다."""
        value=original(f'{HISTORY}/{name}' if name==TEST else name);value['path']=name;return value
    old.pin=historical;old.verify()
    module('fortworld_actual_builder_golden','tools/decomp_fortworld_builder_consumer_oracle.py').verify()


def record():
    """전체 이전24파일과 정확 byte 역변환/별도 골든2를 최초 기록한다."""
    if REPORT.exists():raise RuntimeError('FortWorld consumer successor 덮어쓰기 금지')
    previous();before=(ROOT/f'{HISTORY}/{TEST}').read_bytes();after=(ROOT/TEST).read_bytes()
    changes=[dict(old_offset=i,new_offset=j,old_hex=before[i:k].hex(),new_hex=after[j:l].hex())
        for tag,i,k,j,l in difflib.SequenceMatcher(None,before,after,autojunk=False).get_opcodes() if tag!='equal']
    proof=dict(host='HJOW-Athlon',date='2026-10-14',parent_native_unique=534,parent_normal_unique=252,parent_assert_unique=282,
        additional_existing_body_consumer_golden=2,new_body_completion_unique=0,production_changed=False,
        old_files=[pin(path.relative_to(ROOT).as_posix()) for path in sorted((ROOT/HISTORY).rglob('*')) if path.is_file()],
        current_test=pin(TEST),previous_binding=pin(PREVIOUS),golden_report=pin(GOLDEN),
        golden_fixture=pin('cpppj/tests/fixtures/fortworldload-builder-consumer-x86.tsv'),
        golden_tool=pin('tools/decomp_fortworld_builder_consumer_oracle.py'),tool=pin('tools/analyze_fortworldload_consumer_lineage.py'),
        byte_inverse=changes,cpp_tests=3,cpp_test_result='새13 공유등록/실행대기',limitations=[
        'record3의 실제 유효칸3×20은 목표60이다. MayReplace return1도 quota를소비하므로 정확 Retype 수를20 또는60으로 단정하지 않는다.',
        'Farm 0초과/60이하 범위는 선택 연결 검사이며 원본 Farm 최종 Retype 수의 독립 동일성 완료 근거가 아니다.',
        '새 소비자 골든2는 동일 zero 입력의 실제 Reset/PlaceAll/Add/Bounds에서 Place/Add/Bounds 각1회와 정확3청크/전체3204BYTE를 관찰했다.',
        'C++의 초기 Territory120/물리코드/전체청크/3204BYTE와 Generate 직전 전체출력을 골든에 대조한다. 부모534의 A5 경계 입력과 구분한다.',
        '현재 source6파일 중 test1만 바뀌며 부모534/native/report/fixture/기존 Builder 몸체와 이전52개 의존성은 변경하지 않는다.'])
    REPORT.write_text(json.dumps(proof,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """전체 이전 TEST로 정확 역변환하고 원본/생산/소비자 골든 계수를 구분한다."""
    proof=json.loads(REPORT.read_text(encoding='utf-8'));current=(ROOT/TEST).read_bytes();restored=bytearray(current)
    # 뒤쪽 차이부터 되돌려 offset 변화를 앞선 바이트에 전파하지 않는다.
    for change in reversed(proof['byte_inverse']):
        offset=change['new_offset'];added=bytes.fromhex(change['new_hex']);removed=bytes.fromhex(change['old_hex'])
        assert restored[offset:offset+len(added)]==added;restored[offset:offset+len(added)]=removed
    assert restored==(ROOT/f'{HISTORY}/{TEST}').read_bytes()
    assert not proof['production_changed'] and proof['parent_native_unique']==534 and proof['additional_existing_body_consumer_golden']==2 and proof['new_body_completion_unique']==0
    # 현재 검사와 원문 보고서/도구/골든/전체 과거 바이트를 모두 고정한다.
    for item in [proof['current_test'],proof['previous_binding'],proof['golden_report'],proof['golden_fixture'],proof['golden_tool'],proof['tool'],*proof['old_files']]:assert pin(item['path'])==item,item['path']
    previous();print('FortWorld consumer 감사: 부모534불변/기존Builder골든2/새몸체0/전체24역사+TEST정확역변환')


# --record는 최초 successor이며 기본 실행은 전체 현재 감사를 수행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',action='store_true')
    if parser.parse_args().record:record()
    verify()
