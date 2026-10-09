#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 MayPlace 최종 구간/에필로그와 실제 표면/관계/지역 helper를 대체 없이 실행한다.

앞부분의 누적 지역 변수만 입력한다. 전체 MayPlace/게임/OS/창은 실행하지 않는다.
"""
import argparse
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,STACK,STOP,digest
from decomp_canonpermission_oracle import SPEC as PERMISSION
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제 최종 구간/관찰 지점·표면/지역 전역·함수 지역 변수다.
SPEC={
 'originals':dict(entry=0x49bde0,condition=0x49bf85,surface=0x40eaf0,valid=0x40ea40,restricted=0x46f260,
    map=0x542514,capacity=0x5c847c,regions=0x59aa88,sentinel=0x50c038,islands=0x568b60,bypass=0x59ab30,blocked=0x59ab2c,
    permission=0x10,ground=0x18,only=0x24,bridge=0x28,prior=0x70,self=0x14,
    ret=0x27c,qx=0x284,qy=0x288,owner=0x290),
 'originalCD':dict(entry=0x445a9c,condition=0x445cb5,
    map=0x5670cc,capacity=0x5395f4,regions=0x565a30,sentinel=0x505eb0,bypass=0x51cc00,blocked=0x51cbfc,
    permission=0x20,ground=0x44,only=0x50,bridge=0x74,prior=0x78,can=0x12c,
    ret=0x22c,qx=0x234,qy=0x238,owner=0x10),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 분석 전용 지도와 실제 풀 용량·특수 지역 목록의 정상 레코드 수다.
MAP=0x14000000
CAPACITY,ISLANDS=24000,128
FIXTURE=ROOT/'cpppj/tests/fixtures/canonrelations-x86.tsv'
REPORT=ROOT/'cpppj/recovery-canonrelations-evidence.json'
KEYS=('genus','flags','ground','only','bridge','permission','bypass','current','owner','editor','allies','relation','sid','x','y','region','sentinel','restricted','blocked','prior')


def bits(value):
    """입력 좌표의 단정도 비트값을 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """독립 누적 상태/마스크와 방향 관계·지도 경계 입력만 만든다."""
    cases=[];genera=(0,4,0x4000,0x8000,0x10000,0x20000,0x200,0x4200,0x34000,0x204000,0x200000,0x200200)
    # 지면/권한 조합과 우회 마스크를 실제 최종 구간에서 계산하게 한다.
    for index,values in enumerate(itertools.product(genera,(0,0x400),(0,1),(0,1),(0,1),(0,1),(0,1))):
        genus,flags,ground,only,bridge,permission,bypass=values
        cases.append(dict(genus=genus,flags=flags,ground=ground,only=only,bridge=bridge,permission=permission,bypass=bypass,
            current=index%3,owner=(index//3)%3,editor=(index//9)%2,allies=(index//18)%2,relation=(0,1,0xffffffff)[index%3],
            sid=(0,50,CAPACITY)[index%3],x=bits(20),y=bits(21),region=(0,4,127)[index%3],sentinel=127,
            restricted=index%2,blocked=73,prior=0))
    # 같은 소유자/편집기/비활성 동맹도 마지막 직접 표 읽기에서 독립 비교한다.
    for index,values in enumerate(itertools.product(genera,(0,1,2),(0,1,2),(0,1),(0,1),(0,1,0xffffffff))):
        genus,current,owner,editor,allies,relation=values
        cases.append(dict(cases[0],genus=genus,ground=1,permission=1,current=current,owner=owner,
            editor=editor,allies=allies,relation=relation,sid=50,restricted=0,prior=(0,9,0xffffffff)[index%3]))
    # 보정 전/후 다른 번호가 있는 지도와 절삭 경계/부호 BYTE 요청자를 대조한다.
    positions=((20,21),(20.00005,21),(20.00011,21),(19.99999,21),(20.25,21.5),(-0.9,0),(0,0),
        (255,255),(255.00005,255),(255.0002,255),(256,0),(-1.999,0))
    for index,(genus,pos,sid) in enumerate(itertools.product((0,0x4000,0x8000,0x200),positions,(0,50,CAPACITY,65535))):
        cases.append(dict(cases[0],genus=genus,ground=1,permission=1,editor=0,allies=1,current=1,
            owner=(1,0x1ff,0x100,0x102)[index%4],relation=index%2,sid=sid,x=bits(pos[0]),y=bits(pos[1]),restricted=index%2))
    # signed 지역/sentinel과 우회 후에도 특수 지역 읽기가 남는 입력이다. 없는 지역 assert는 제외한다.
    for genus,region,sentinel,restricted in itertools.product((0x200,0x200200),(0,4,127,255),(127,0xffffffff),(0,1,0xffffffff)):
        if region==255 and sentinel!=0xffffffff:continue
        cases.append(dict(cases[0],genus=genus,ground=1,permission=1,region=region,sentinel=sentinel,restricted=restricted))
    # 전체 MayPlace 접두가 아닌 최종 구간만의 좌표 읽기 생략을 관찰한다.
    for genus,prior in itertools.product((0x200000,0x204000,0x200200),(0,9,0xffffffff)):
        cases.append(dict(cases[0],genus=genus,x=0x7fc12345,y=0x7f800000,prior=prior))
    return cases


class RelationsOracle(OwnerOracle):
    """원본 후반/정상 반환과 helper를 대체 없이 실행하고 상태를 관찰한다."""
    def __init__(self,edition):
        """새 함수 목록의 실제 범위와 현재 지도/정상 용량 풀을 준비한다."""
        super().__init__(edition);self.s=SPEC[edition];self.p=PERMISSION[edition]
        old=(128*self.stride+4095)&~4095;full=(CAPACITY*self.stride+4095)&~4095
        self.mu.mem_map(POOL+old,full-old);self.mu.mem_map(MAP,0x20000)
        self.mu.mem_write(self.s['map'],struct.pack('<I',MAP));self.mu.mem_write(self.s['capacity'],struct.pack('<I',CAPACITY))
        self.exports=[ROOT/f'extracted/canonrelations/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.fragments=0;self.conditions=0
        with self.exports[1].open(encoding='utf-8') as fp:
            # 새 내보내기의 불연속 몸체 범위만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 함수의 각 물리 명령 범위를 등록한다.
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """거부값을 덮어쓰는 에필로그 전에 실제 레지스터/지역 변수만 관찰한다."""
        if address==self.s['condition']:
            self.foreign=mu.reg_read(UC_X86_REG_EAX) if self.edition=='originals' else struct.unpack('<I',mu.mem_read(STACK+self.s['prior'],4))[0]
            self.conditions+=1
        if self.edition!='originals' and address==0x445ce7:self.rejected=mu.reg_read(UC_X86_REG_ESI)
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """원본 누적 입력에서 후반 판정/반환/스택/x87·읽기 전용 자료를 관찰한다."""
        self.instructions=0;self.foreign=case['prior'];self.rejected=0;s=self.s;p=self.p;mu=self.mu;own=TYPES+82*self.type_stride
        mu.mem_write(own,bytes(self.type_stride));mu.mem_write(own+0xe8,struct.pack('<II',case['flags'],case['genus']))
        raw=bytearray([0xcd]*(128*self.stride));raw[50*self.stride+self.o['owner']]=case['current'];raw[51*self.stride+self.o['owner']]=(case['current']+1)%3
        mu.mem_write(POOL,bytes(raw));mu.mem_write(MAP,struct.pack('<H',case['sid'])*65536)
        # 지도 보정이 생략되면 다른 owner를 읽게 하는 네 칸 자료를 입력한다.
        for x,y,sid in ((20,21,case['sid']),(21,21,51 if case['sid']==50 else case['sid']),
                (20,22,51 if case['sid']==50 else case['sid']),(21,22,case['sid'])):
            mu.mem_write(MAP+2*(y*256+x),struct.pack('<H',sid))
        # 원래 소유자→요청자 한 칸만 채워 반대 방향/다른 표면 owner 읽기를 구별한다.
        table=[0]*81;request=case['owner']&255;request=request if request<128 else request-256
        index=(case['current']*9+request)&0xffffffff
        if index<81:table[index]=case['relation']
        mu.mem_write(p['relations'],struct.pack('<81I',*table))
        # 최종 판정이 다시 읽는 현재 전역과 signed 지역 BYTE를 입력한다.
        for name,value in (('editor',case['editor']),('allies',case['allies'])):mu.mem_write(p[name],struct.pack('<I',value))
        mu.mem_write(s['regions'],bytes([case['region']])*144);mu.mem_write(s['sentinel'],struct.pack('<I',case['sentinel']))
        mu.mem_write(s['bypass'],struct.pack('<I',case['bypass']));mu.mem_write(s['blocked'],struct.pack('<I',case['blocked']))
        if self.edition=='originals':
            data=bytearray(4+ISLANDS*32);struct.pack_into('<I',data,0,ISLANDS)
            # 원본 목록의 모든 정상 레코드를 실제 +4 제한/+8 존재 필드로 만든다.
            for i in range(ISLANDS):struct.pack_into('<II',data,4+i*32,case['restricted'],1)
            mu.mem_write(s['islands'],bytes(data))
        self.write_ranges=[(s['blocked'],s['blocked']+4)]
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        mu.mem_write(STACK,bytes(0x400));saved=(preserved[UC_X86_REG_EDI],preserved[UC_X86_REG_ESI],preserved[UC_X86_REG_EBX],preserved[UC_X86_REG_EBP]) if self.edition=='originals' else (preserved[UC_X86_REG_EBP],preserved[UC_X86_REG_EDI],preserved[UC_X86_REG_ESI],preserved[UC_X86_REG_EBX])
        mu.mem_write(STACK,struct.pack('<4I',*saved));mu.mem_write(STACK+s['ret'],struct.pack('<I',STOP))
        # 함수 앞부분에서 계산한 부울/이전 거부와 실제 좌표 인자를 공급한다.
        for name in ('permission','ground','only','bridge','prior'):mu.mem_write(STACK+s[name],struct.pack('<I',case[name]))
        mu.mem_write(STACK+s['qx'],struct.pack('<I',case['x']));mu.mem_write(STACK+s['qy'],struct.pack('<I',case['y']))
        owner=(case['owner']&255);owner=owner if owner<128 else owner-256
        mu.mem_write(STACK+s['owner'],struct.pack('<I',case['owner'] if self.edition=='originals' else owner&0xffffffff))
        if self.edition=='originals':mu.mem_write(STACK+s['self'],struct.pack('<I',own))
        else:mu.reg_write(UC_X86_REG_EBX,own)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.emu_start(s['entry'],STOP,count=15000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+s['ret']+28:raise RuntimeError('최종 구간 정상 반환/스택 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('최종 구간 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('최종 구간 x87 오류')
        self.fragments+=1
        rejected=struct.unpack('<I',mu.mem_read(STACK+0x14,4))[0] if self.edition=='originals' else self.rejected
        ground=struct.unpack('<I',mu.mem_read(STACK+(0x18 if self.edition=='originals' else s['can']),4))[0]
        return [mu.reg_read(UC_X86_REG_EAX),struct.unpack('<I',mu.mem_read(STACK+s['permission'],4))[0],ground,
            struct.unpack('<I',mu.mem_read(s['blocked'],4))[0],rejected,self.foreign,struct.unpack('<I',mu.mem_read(STACK+s['prior'],4))[0],
            zlib.adler32(mu.mem_read(POOL,128*self.stride)),zlib.adler32(mu.mem_read(MAP,0x20000)),zlib.adler32(mu.mem_read(p['relations'],324))]


def generate(smoke=False):
    """두 정밀도의 실제 명령 관찰이 같을 때만 새 독립 fixture/근거를 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_canonpermission_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/canonrelations-functions.json',FIXTURE}
    # 세 PE를 별도 실행기로 정상 반환까지 대조한다.
    for edition in SPECS:
        oracle=RelationsOracle(edition);cases=inputs()
        if smoke:cases=cases[::max(1,len(cases)//50)]
        # 기대값은 Python 규칙 없이 실제 두 실행의 관찰값만 사용한다.
        for case in cases:
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError(f'x87 차이: {edition} {case}')
            rows.append([edition,*[case[key] for key in KEYS],*values[0]])
        editions[edition]=dict(cases=len(cases),fragments=oracle.fragments,conditions=oracle.conditions,native_calls=dict(oracle.native_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 최종 관계 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 MayPlace 최종 구간/에필로그·표면/관계/지역 helper. 대체 없음. 전체 MayPlace 미실행.\n# edition '+' '.join(KEYS)+' result permissionOut canGround blockedOut relationRejected foreignRejected priorOut rawAdler mapAdler relationAdler\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,substitutions=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['최종 구간/실제 성공·실패 에필로그·ESP/ret 24/보존 레지스터/x87 정상 반환',
            '앞부분 지면/권한/지역/이전 거부만 입력하며 전체 MayPlace/decoder/finder/Player는 실행하지 않음',
            '패치 표면/포인터 유효/방향 관계/특수 지역 getter 실제 몸체, CD 표면/관계 인라인 실제 명령, 함수 대체 없음',
            '원본 특수 지역 존재 assert에 닿는 입력 제외, 전체 자산/미션/게임/OS/창 검증 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """입력 도구/PE/내보내기/fixture SHA와 실제 구간/조건/helper 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    # 저장 당시 모든 근거 파일의 바이트와 비교한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    cases=inputs()
    if len(rows)!=3*len(cases) or len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls'] or report['substitutions']:raise RuntimeError('최종 관계 근거 오류')
    # 실제 후반 진입/마지막 조건 관찰과 패치의 helper 실행을 감사한다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition]
        # 입력 순서/열 계약도 재구성하여 관찰 행과 근거 개수를 함께 검사한다.
        if len(selected)!=len(cases) or any(len(row)!=31 or [int(v) for v in row[1:21]]!=[case[key] for key in KEYS] for row,case in zip(selected,cases)):raise RuntimeError('최종 관계 입력/열 오류')
        if len(selected)!=item['cases'] or item['fragments']!=2*len(selected) or item['assertions']:raise RuntimeError('최종 구간/행/assert 오류')
        if item['conditions']!=2*sum(not(int(r[1])&0x200000) for r in selected):raise RuntimeError('최종 직접 조건 누락')
        if edition=='originals':
            # 내보낸 helper 모두 실제 입력에서 실행했는지 확인한다.
            for name in ('surface','valid','restricted'):
                if not item['native_calls'].get(f'{SPEC[edition][name]:08x}',0):raise RuntimeError('실제 helper 누락: '+name)
            if not item['native_calls'].get(f'{PERMISSION[edition]["related"]:08x}',0):raise RuntimeError('실제 방향 관계 helper 누락')
        if not item['native_calls'].get('004e49c0' if edition=='originals' else '004f161c',0):raise RuntimeError('실제 CRT 절삭 누락')
    print(f'canonrelations 검증 통과: {len(rows)}개')


if __name__=='__main__':
    # 생성/소규모 실행/저장 감사는 원본 게임 실행과 무관한 제한 분석이다.
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
