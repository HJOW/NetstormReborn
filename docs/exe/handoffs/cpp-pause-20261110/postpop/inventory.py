"""자산83VT의 원본 가상20과 현행 NoWorld 최초 후처리 진입만 정적으로 집계한다."""
from pathlib import Path
import hashlib,json,re,socket,struct

root=Path.cwd() # 실제 공유 저장소 루트다.
out=Path(__file__).resolve().parent # 읽기 전용 감사 산출물만 별도로 보관한다.
image=(root/'originals/Netstorm.exe').read_bytes() # 보호 PE는 읽기만 한다.
pe=struct.unpack_from('<I',image,60)[0];opt=pe+24;base=struct.unpack_from('<I',image,opt+28)[0];sections=[] # 실제 주소 변환 자료다.
for index in range(struct.unpack_from('<H',image,pe+6)[0]): # 선언된 PE 섹션을 그대로 읽는다.
    at=opt+struct.unpack_from('<H',image,pe+20)[0]+index*40;size,va,disk_size,disk_at=struct.unpack_from('<IIII',image,at+8);sections.append((va,max(size,disk_size),disk_at))
def dword(address):
    """보호 PE 주소의 실제 DWORD를 반환한다."""
    for va,size,at in sections: # 주소를 포함하는 섹션만 선택한다.
        if va<=address-base<va+size:return struct.unpack_from('<I',image,at+address-base-va)[0]
    raise ValueError(hex(address))
original=json.loads((root/'cpppj/no-world-regular-audit-20261109.json').read_text(encoding='utf-8')) #87개 중 asset83의 주소 목록만 재사용하며 옛 Regular 판정은 사용하지 않는다.
tables=sorted(int(row['vtable'],16) for row in original['rows'] if row['kind']=='asset');assert len(tables)==len(set(tables))==83
groups={} # 같은 실제 가상20을 공유하는 행을 묶는다.
for table in tables: # 각 PE 슬롯은 독립적으로 다시 읽는다.
    function=dword(table+0x20);groups.setdefault(function,[]).append(table)
text=(root/'cpppj/src/o/RawAssetPostPop.cpp').read_text(encoding='utf-8');patch=text[text.index('kPatchTables{{'):text.index('kCdTables{{')]
dedicated={int(table,16):kind for table,kind in re.findall(r'\{0x([0-9a-f]+),([^}]+)\}',patch)} #24개 기존 전용 자산 부류다.
long_tables={0x509148:'FactoryPostPop',0x509210:'FactoryPostPop',0x503d80:'ResourceCarrierPostPop',0x503e50:'ResourceCarrierPostPop',0x50e568:'ResourceCarrierPostPop',0x5040c0:'BalloonBody',0x504198:'BalloonBody',0x50a820:'DamageablePostPop',0x50a8e8:'DamageablePostPop',0x50a748:'BuriedPostPop',0x50e888:'IceTower',0x509470:'FencePostPop',0x50dc30:'NuggetPostPop',0x508cc8:'DudePostPop',0x502c38:'BombPostPop',0x502cf0:'BombPostPop',0x502db0:'BombPostPop',0x50da88:'GeyserPostPop'} #現1264 실제 DefaultResourcePostPop 분기18개다.
source=(root/'cpppj/src/o/SquidPostPop.cpp').read_text(encoding='utf-8');pop=(root/'cpppj/src/o/PopVtables.inc').read_text(encoding='utf-8');pop_patch=pop[:pop.index('kCdPopVtables')]
supported={int(value,16) for value in re.findall(r'0x([0-9a-f]+)U',pop_patch)}|{0x50c3b0,0x509810,0x5070c0} #마지막 미연결 거부 뒤의 기존 공통 허용표다.
prefixes={0x5034c8:'BridgePrefix',0x506d98:'IslandPrefix',0x506f08:'SurfacePrefix'} #RawBridgeConnect/RawIslandPostPop 헤더의 실제 현재 상수와 PE를 대조했다.
rows=[] #진입 연결과 큰 하위 완료를 구별한다.
for table in tables: #실제83행을 한 번씩 판정한다.
    function=dword(table+0x20);status='missing';body=None;boundary='현재가상후처리몸체미복원'
    if table in long_tables:status='long-entry';body=long_tables[table];boundary='선택된 큰 Runtime/AI 하위는 별도 감사'
    elif table in prefixes:status='long-prefix';body=prefixes[table];boundary='선택된 큰 Runtime/AI 하위는 별도 감사'
    elif table==0x5070c0:status='nonfirst-only';body='Teleport';boundary='flags1에서 teleportPrefix 미연결'
    elif table==0x50f540:status='nonfirst-only';body='Residence';boundary='DefaultResourcePostPop의 flags1에서 전용자원 몸체 미설치'
    elif function==0x4b0d30 and table in supported:status='common-entry';body='PostPopBase';boundary='공통 AI/선택 재검사 큰 하위는 별도 감사'
    elif table in dedicated:body=dedicated[table];boundary='DefaultResourcePostPop의 전용자원 몸체 미설치'
    rows.append(dict(vtable=f'{table:08x}',postPop=f'{function:08x}',status=status,body=body,boundary=boundary))
report=dict(host=socket.gethostname(),date='2026-11-10',baseProduct=1264,sourceSnapshot={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in ['cpppj/src/o/SquidPostPop.cpp','cpppj/src/o/RawAssetPostPop.cpp','cpppj/src/o/PopVtables.inc','cpppj/src/client/RawSurfaceStorage.cpp','cpppj/src/client/RawCampaignSession.cpp']},originalSha256=hashlib.sha256(image).hexdigest(),rows=rows,groups={f'{function:08x}':[f'{value:08x}' for value in values] for function,values in groups.items()},executions=0,productChanges=0,fullNoWorldComplete=False)
(out/'inventory.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
for function,values in groups.items(): #검토할 실제 PE 함수별 주소와 현행 진입 분류만 출력한다.
    selected=[row for row in rows if int(row['postPop'],16)==function];print(f'{function:08x}',len(values),','.join(f'{value:08x}' for value in values),' | ',','.join(sorted({row['status'] for row in selected})))
