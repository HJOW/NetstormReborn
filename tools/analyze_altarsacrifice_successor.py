#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""제단초기smoke/중심helper확대/실제Frame단계입력/최종근거closure를원문과동일입력으로연결한다."""
import argparse,hashlib,json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 현재작업저장소다.
INITIAL=ROOT/'cpppj/hist/altarsacrifi-af04e' # 8/7·최초30행의당시원문이다.
FRAME=ROOT/'cpppj/hist/altarsacrifi-6e4d0' # 중심확대후첫전수2866의당시원문이다.
REPORT=ROOT/'cpppj/altarsacrifice-successor.json' # 추가합산없는현재source/동일입력근거다.
BODY=('cpppj/src/o/RawAltarSacrifice.h','cpppj/src/o/RawAltarSacrifice.cpp') # 처음부터동작본문이불변인새전용모듈이다.

def sha(path):
    """원문UTF8바이트전체의SHA256을읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def rows(path):
    """실제관찰을판본·JSON입력키로읽으며중복은거부한다."""
    result={}
    for line in path.read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):continue
        row=line.split('\t');key=(row[0],row[1]);assert key not in result;result[key]=row
    return result

def restore_levels(row):
    """기대상위결과를계산하지않고초기해시단계입력BYTE3개만현재1로연결한다."""
    result=row.copy();images=result[4].split(';');offset=33 if result[0]=='originals' else 31
    for index in range(3):
        raw=bytearray.fromhex(images[index]);assert raw[offset]==0;raw[offset]=1;images[index]=raw.hex()
    result[4]=';'.join(images);return result

def build():
    """두역사원문과현재native자료의실제동일입력/불변production을감사한다."""
    current=ROOT/'cpppj/altarsacrifice.json';data=json.loads(current.read_text(encoding='utf-8'));assert data['total']==2866 and not data['smoke'];currentRows=rows(ROOT/'cpppj/tests/fixtures/altarsacrifice-x86.tsv');records=[];files={Path(__file__),current,ROOT/'tools/decomp_altarsacrifice_oracle.py',ROOT/'cpppj/tests/fixtures/altarsacrifice-x86.tsv'}
    for history in (INITIAL,FRAME):
        oldReport=history/'cpppj/altarsacrifice.json';old=json.loads(oldReport.read_text(encoding='utf-8'));oldFixture=history/'cpppj/tests/fixtures/altarsacrifice-x86.tsv';oldRows=rows(oldFixture);assert len(oldRows)==old['total'];preserved={}
        # 당시저장한파일은모두당시report SHA와일치해야한다. 당시외부helper파일은원본수치재합산대상이아니다.
        for name,value in old['files'].items():
            saved=history/name
            if saved.is_file():assert sha(saved)==value,name;preserved[name]=value;files.add(saved)
        for name in BODY:assert (history/name).read_bytes()==(ROOT/name).read_bytes();files.add(ROOT/name)
        assert set(oldRows)<=set(currentRows);unchanged=0
        # Frame초기단계이외모든반환/효과/좌표/HP/WORD/RNG/Player결과는당시관찰그대로다.
        for key,row in oldRows.items():assert restore_levels(row)==currentRows[key],key;unchanged+=row==currentRows[key]
        records.append(dict(history=history.relative_to(ROOT).as_posix(),total=old['total'],smoke=old['smoke'],same_input_rows=len(oldRows),unchanged_output_rows=unchanged,only_output_change='slots5/6/7의현재1칸SHP·해시1단계와일치하는초기level BYTE0→1',preserved_report_sha=sha(oldReport),preserved_files=preserved));files.add(oldReport);files.add(oldFixture)
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    return dict(host='HJOW-Athlon',decompile_date='2026-10-14',current_normal=2866,asserts=0,added_by_rerun=0,initial_smoke_counted=0,failed_partial_attempts_counted=0,old_dais668_added=0,old_helpers_added=0,production_whole_bytes_unchanged=True,stages=records,scope='상위Altar/DaisPre/Finish관찰및Frame입력교정. 실제전체DamageablePostDestroy/파생Pop및GameWorld의의식완주는완료로세지않는다.',files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)})

def main(verify):
    """현재감사결과를만들거나이미저장된결과와전수동일성을확인한다."""
    value=build()
    if verify:assert json.loads(REPORT.read_text(encoding='utf-8'))==value
    else:REPORT.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('제단동일입력successor/불변production감사:',value['current_normal'])

# 명시CLI요청만저장/현재역사원문감사를실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
