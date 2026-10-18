#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의83개 기존VT에서 실제+5c/+98를 읽어 공통 Handle/HP 분배 범위를 고정한다."""
import argparse,hashlib,json,re,struct
from pathlib import Path
import pefile
ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT/'cpppj/tests/fixtures/damageable-regular-tables-x86.tsv'
REPORT=ROOT/'cpppj/damageable-regular-tables.json'
HEADER=ROOT/'cpppj/src/client/CampaignDamageableRegularTables.inc'
def sha(path):
    """현재 원문 SHA256이다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    """실제 표 데이터만 동결한다. 함수 실행/완주 관측 수에 합산하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in report['sha256'].items():assert sha(ROOT/path)==wanted,path
        print('Damageable 실제VT83개 +5c/+98 데이터·해시 일치');return
    source=(ROOT/'cpppj/src/o/CampaignDamageVtables.inc').read_text('utf-8').split('kCampaignDamageHitEntries{{',1)[1].split('}};',1)[0];tables=[int(v,16) for v in re.findall(r'\{0x([0-9a-fA-F]+)U,',source)];assert len(set(tables))==83
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase;rows=[]
    for table in tables:
        handle=struct.unpack_from('<I',image,table-base+0x5c)[0];hit=struct.unpack_from('<I',image,table-base+0x98)[0];rows.append((table,handle,hit,int(handle==0x44b900 and hit in (0x44c8e0,0x4add10))))
    groups={'kCommonDamageableRegularTables':[r[0] for r in rows if r[3]],'kGeneralDamageableHitTables':[r[0] for r in rows if r[2]==0x44c8e0],'kBaseDamageableHitTables':[r[0] for r in rows if r[2]==0x4add10]};assert 0x50b888 in groups['kCommonDamageableRegularTables'] and 0x50b9b8 in groups['kCommonDamageableRegularTables']
    text='// 보호10.78의 기존83VT에서 읽은 실제+5c/+98 분배 범위다. 원본 주소를 호스트 함수로 호출하지 않는다.\n'
    for name,values in groups.items():
        text+='// 실제 공통 Handle 또는 실제 HP 몸체의 현재VT 기록값이다.\nconstexpr std::array<std::uint32_t,'+str(len(values))+'> '+name+'{'+','.join(hex(value)+'U' for value in values)+'};\n'
    HEADER.write_text(text,'utf-8',newline='\n');FIXTURE.write_text('# 실제83VT의 +5c/+98와 공통Damageable Handle 여부. 함수 실행은 기존 AssetRegular/HP 원문을 재사용한다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n','utf-8',newline='\n');paths=['originals/Netstorm.exe','cpppj/src/o/CampaignDamageVtables.inc','cpppj/src/o/RawAssetRegular.cpp','cpppj/src/o/RawDamageableHit.cpp','tools/decomp_damageable_regular_tables.py',FIXTURE.relative_to(ROOT).as_posix(),HEADER.relative_to(ROOT).as_posix()];REPORT.write_text(json.dumps(dict(unit=191,scope='실제 PE 데이터83행. 새 함수 실행 관찰0/OS0. 기존 원본대조 몸체에 전달할 분배만 복원한다.',rows=83,groups={name:len(values) for name,values in groups.items()},sha256={path:sha(ROOT/path) for path in paths}),ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(83,{name:len(values) for name,values in groups.items()})
if __name__=='__main__':main()
