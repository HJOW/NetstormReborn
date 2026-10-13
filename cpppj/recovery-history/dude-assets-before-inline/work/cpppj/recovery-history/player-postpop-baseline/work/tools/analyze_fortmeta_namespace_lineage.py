#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""첫 공유빌드 namespace 닫힘 2BYTE 교정과 이전950 원본/전체출처를 연결한다."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

# 원본/도구/행을재관찰하거나덮어쓰지않고현재CPP한파일만최소교정한다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY='cpppj/recovery-history/fortmeta-before-namespace-fix'
CPP='cpppj/src/o/RawFortMetaLoad.cpp'
NATIVE='cpppj/recovery-fortmeta-evidence.json'
BINDING='cpppj/recovery-fortmeta-cpp-binding-evidence.json'
REPORT=ROOT/'cpppj/recovery-fortmeta-namespace-lineage-evidence.json'


def pin(name):
    """현재전체파일의크기/SHA를고정한다."""
    raw=(ROOT/name).read_bytes();return dict(path=name,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())


def module(name,path):
    """보존된원문도구를그대로읽되출처조회에명시historical view를공급한다."""
    spec=importlib.util.spec_from_file_location(name,ROOT/path);result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result


def old_native():
    """950개전체출처의CPP한파일만교정전whole snapshot으로엄격하게연결한다."""
    report=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'))
    # PE/tool/export/fixture/다른CPP는현재전체바이트,해당CPP만보존된전체바이트로대조한다.
    for name,expected in report['files'].items():
        source=f'{HISTORY}/{name}' if name==CPP else name;assert pin(source)['sha256']==expected,name
    observed=module('fortmeta_native_before_namespace','tools/decomp_fortmeta_oracle.py');original_digest=observed.digest
    observed.digest=lambda path:original_digest(ROOT/f'{HISTORY}/{CPP}' if Path(path).resolve()==(ROOT/CPP).resolve() else path)
    observed.verify();return report


def old_binding():
    """초기binding전체tool/의존성과950본문을보존된CPPview로연결한다."""
    observed=module('fortmeta_binding_before_namespace','tools/analyze_fortmeta_cpp_binding.py');original_pin=observed.pin
    def historical_pin(name):
        """교정전CPP의original path는유지하고전체바이트SHA만그whole snapshot에서읽는다."""
        item=original_pin(f'{HISTORY}/{name}' if name==CPP else name);item['path']=name;return item
    observed.pin=historical_pin;observed.native=old_native;observed.verify()


def record():
    """교정전전체자료/실패로그와한brace역변환및현재CPP출처를최초기록한다."""
    if REPORT.exists():raise RuntimeError('FortMeta namespace lineage 덮어쓰기 금지')
    old_native();old_binding();saved=[]
    # 교정전source/report/tool/spec/docs/fixture/최초실패로그를whole파일로기록한다.
    for path in sorted((ROOT/HISTORY).rglob('*')):
        if path.is_file():saved.append(pin(path.relative_to(ROOT).as_posix()))
    proof=dict(host='HJOW-Athlon',date='2026-10-14',new_native_unique=0,preserved_native_unique=950,normal_unique=394,assert_unique=556,
      old_files=saved,current_cpp=pin(CPP),current_native=pin(NATIVE),current_binding=pin(BINDING),tool=pin('tools/analyze_fortmeta_namespace_lineage.py'),
      added_bytes_hex='7d0a',changed_cpp_files=1,cpp_test_result='공유재빌드/3검사대기',
      limits=['첫두공유build의C1075는namespace닫힘brace누락이다. source EOF에 }와LF 두BYTE만추가했다.',
      '950개fixture·원본출력·native tool·초기보고서·다른CPP/test는전체바이트불변이며새원본0이다.',
      '초기보고서의CPP SHA는교정전whole파일을가리키고현재CPP는이successor로연결한다. 초기빌드실패를삭제하지않는다.'])
    REPORT.write_text(json.dumps(proof,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """정확한2BYTE역제거와기존전체원본/의존성·현재source출처를감사한다."""
    proof=json.loads(REPORT.read_text(encoding='utf-8'));before=(ROOT/f'{HISTORY}/{CPP}').read_bytes();current=(ROOT/CPP).read_bytes()
    assert current==before+b'}\n' and proof['new_native_unique']==0 and proof['preserved_native_unique']==950
    # 각whole snapshot과현재successor파일을완전히대조한다.
    for item in [*proof['old_files'],proof['current_cpp'],proof['current_native'],proof['current_binding'],proof['tool']]:assert pin(item['path'])==item,item['path']
    old_native();old_binding();print('FortMeta namespace 감사: 정확한brace2BYTE/950행불변/추가native0')


# --record는첫successor연결이고기본은전체원문/현재출처감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',action='store_true')
    if parser.parse_args().record:record()
    verify()
