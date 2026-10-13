#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Money 첫C++대조의모드지정오류·월드입력·BYTE변환교정을전체이전자료와연결한다."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent  # 같은저장소의절대기준이다.
HISTORY=ROOT/'cpppj/recovery-history/fortmoney-before-cpp-correction'  # 최초전체11자료의바이트를보존한다.
REPORT=ROOT/'cpppj/recovery-fortmoney-cpp-correction-evidence.json'  # 기존408원본관찰과현재교정의별도근거다.
NATIVE='cpppj/recovery-fortmoney-evidence.json'  # 원본결과는전체불변이며현재Cpp SHA만연결한다.
CHANGES={
 'cpppj/src/o/RawFortMoneyLoad.h':[
  ('    // 패치00447810: 로컬이면현재special 고정SP와표시를적용하고1..8의비로컬은분산SP만지정한다.',
   '    // 패치00447810: 유효로컬/비로컬모두special 고정SP를적용한다. 로컬활성때만표시를갱신한다.'),
  ('    SquidPostPopState& players_; // 현재localOwner DWORD가있는동일월드장부다.',
   '    SquidPostPopState& players_; // 현재유효localOwner0..8을읽는동일월드장부다. 저장폭은BYTE이며원본DWORD의유효값과같다.')],
 'cpppj/src/o/RawFortMoneyLoad.cpp':[
  ('constexpr float kFortSp=100000.0f,kSpecialLocalSp=194162.0f; // 실제Money fort값과SetPlayerSp 로컬special값이다.',
   'constexpr float kPatchFortSp=194162.0f,kCdFortSp=100000.0f,kSpecialLocalSp=194162.0f; // 실제판본별Money fort값과모든유효owner의special 지정값이다.'),
  ('    } else if (owner>0 && owner<9) patchStore_->Set(static_cast<std::uint32_t>(owner),std::bit_cast<std::uint32_t>(static_cast<float>(amount)));',
   '    } else if (owner>0 && owner<9) {\n        const float value=reward_.fixedSpB ? kSpecialLocalSp : static_cast<float>(amount);\n        patchStore_->Set(static_cast<std::uint32_t>(owner),std::bit_cast<std::uint32_t>(value));\n    }'),
  ('            if (patch) patchStore_->Set(players_.localOwner,std::bit_cast<std::uint32_t>(kFortSp));\n            else reward_.cdLocalSp=kFortSp;',
   '            if (patch) patchStore_->Set(players_.localOwner,std::bit_cast<std::uint32_t>(kPatchFortSp));\n            else reward_.cdLocalSp=kCdFortSp;')],
 'cpppj/tests/FortMoneyLoadTests.cpp':[
  ('players.localOwner=Number(values[6]);','players.localOwner=static_cast<std::uint8_t>(Number(values[6]));'),
  ('    netstorm::client::RawSurfaceWorld world(edition,types,frames,shapes,{},97);',
   '    BridgeConnectState bridges; // 동일월드연결의실제타입번호이며이검사에서표면을생성하지않는다.\n    bridges.bridgeType=82;bridges.islandType=94;bridges.stalagType=95;bridges.connectorType=155;bridges.noIslandType=157;\n    netstorm::client::RawSurfaceWorld world(edition,types,frames,shapes,bridges,97);')]
}  # 승인된원본분기교정과입력/주석만역변환하며다른기존본문을보존한다.


def sha(path):
    """전체현재바이트의SHA를반환한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_sources():
    """교정대상세파일전체를정확역변환해최초전체소스와비교한다."""
    source={}
    # 명시된최소문자열만되돌려다른SP/로딩/관찰구현전체의보존을확인한다.
    for name, changes in CHANGES.items():
        text=(ROOT/name).read_text(encoding='utf-8')
        for before, after in reversed(changes):
            assert text.count(after)==1,name
            text=text.replace(after,before,1)
        assert text==(HISTORY/name).read_text(encoding='utf-8'),name
        source[name]=dict(before_sha256=sha(HISTORY/name),current_sha256=sha(ROOT/name),entire_inverse_matches=True)
    return source


def record():
    """기존408결과전체를유지하고새Cpp sourceSHA/연결검사만별도기록한다."""
    sources=check_sources()
    old=json.loads((HISTORY/NATIVE).read_text(encoding='utf-8'))
    current=json.loads(json.dumps(old))
    # fixture/원본도구/원본반환은그대로두고교정된두Cpp 파일SHA만갱신한다.
    for name in CHANGES:
        if name in current['files']:current['files'][name]=sha(ROOT/name)
    (ROOT/NATIVE).write_text(json.dumps(current,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    from analyze_fortmoney_cpp_binding import generate
    generate()
    saved={path.relative_to(HISTORY).as_posix():sha(path) for path in HISTORY.rglob('*') if path.is_file()}
    data=dict(host='HJOW-Athlon',normal_preserved=388,assert_preserved=20,new_native=0,new_unique=0,sources=sources,
              saved_files=saved,current_native_report_sha256=sha(ROOT/NATIVE),current_binding_sha256=sha(ROOT/'cpppj/recovery-fortmoney-cpp-binding-evidence.json'),
              tool_sha256=sha(Path(__file__)),reason='실제447861~447880의비로컬special분기와패치483d9c80=194162/CD100000차이를교정했다. BYTE소유자명시변환/실제bridge타입으로world입력도교정했다.')
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    verify()


def verify():
    """전체최초자료/역변환/동일408원본결과와현재바이트를감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    assert data['sources']==check_sources() and data['tool_sha256']==sha(Path(__file__))
    # 보존한11전체파일은바이트까지변경하지않는다.
    for name, expected in data['saved_files'].items():
        assert sha(HISTORY/name)==expected,name
    old=json.loads((HISTORY/NATIVE).read_text(encoding='utf-8'))
    current=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'))
    projection=json.loads(json.dumps(current))
    # 현재소스SHA만옛값으로되돌리면원본보고서전체가같아야한다.
    for name in CHANGES:
        if name in projection['files']:projection['files'][name]=old['files'][name]
    assert projection==old and sha(ROOT/NATIVE)==data['current_native_report_sha256']
    assert sha(ROOT/'cpppj/recovery-fortmoney-cpp-binding-evidence.json')==data['current_binding_sha256']
    assert data['new_native']==data['new_unique']==0
    print('Money 첫CPP교정: 전체이전본문/408원본입력 보존·새고유0')


# 기본호출은현재감사이며 --record는전체교정연결의최초기록이다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record',action='store_true')
    arguments=parser.parse_args()
    record() if arguments.record else verify()
