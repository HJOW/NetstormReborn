#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Player+24→진단 접두→현재 GumpForm 알림을 보호 PE로 독립 실행한다."""
import argparse
import collections
import csv
import json
import math
import struct
from pathlib import Path
import decomp_missionframe_oracle as common
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

# 실제 raw/현재등록표와 외부 가상UI 호출만 격리 메모리에 제공한다.
ROOT=common.ROOT
POOL,GUMP,VTABLE=0x12000000,0x13000000,0x13000100
STOP,STACK=common.STOP,common.STACK
HANDLE=STOP+0x100
PARENT=50
REGISTRY=0x563e38
FIELDS=[('cursorA',0x568af8,4),('cursorB',0x54d4e0,4),('problem',0x5318ec,4),('fallback',0x540cb8,4),('checks',0x5c89c4,4),
    ('battle',0x595344,4),('local',0x540c70,4),('peer',0x594fd0,4),('first',0x531890,2),('gumpType',0x54107c,4),('event',0x5427bc,4)]
KEYS=tuple(key for key,_,_ in FIELDS)+('money','layout','mutation','form','reason','wrapper')
FIXTURE=ROOT/'cpppj/tests/fixtures/player-content-notice-x86.tsv'
REPORT=ROOT/'cpppj/player-content-notice.json'


def case(**changes):
    """현재 SP/연결/진단/체인의 명시 입력만 구성한다."""
    value=dict(zip(KEYS,(3,3,77,9,0,1,1,1,0,3,0x2000035,common.bits(0.0,4),0,0,0x12345678,0xfedcba98,1)));value.update(changes);return value


def cases():
    """현재 진단 접두와 내용물 체인의 필터/순서/조회 뒤 변경을 독립 입력한다."""
    result=[]
    # SP 임계 직전/동일/직후와 unordered를 실제 x87로 실행한다.
    for money in (0.0,6999.5,7000.0,7000.5,10000.0,float('nan'),float('inf')):
        # DWORD 비영 잔여검사의 감김도 별도 입력이다.
        for checks in (0,1,2,0xffffffff):
            # SP 허용은 현재 DWORD 비영으로 판정한다.
            for battle in (0,1):result.append(case(money=common.bits(money,4),checks=checks,battle=battle))
    # 네 진단코드 조건과 정확WORD -1 비교를 분리한다.
    for unequal in (0,1):
        # NULL SP분산배열과 비NULL opaque토큰의 비교다.
        for peer in (0,1):
            # 첫목록값은WORD 전체만 의미가 있다.
            for first in (0,0xffff,0x8000):result.append(case(cursorB=3+unequal,peer=peer,first=first))
    # raw상태비트/종류보다현재Form타입DWORD와등록표를사용한다.
    for layout in range(6):
        # 부모wrapper와직접UI호출은같은몸체를실행한다.
        for wrapper in (0,1):result.append(case(layout=layout,wrapper=wrapper,checks=1,money=common.bits(7000.5,4)))
    # 조건부SP/각UI효과가현재전역과다음링크/등록표를변경한다.
    for mutation in (1,2,3):
        # head가없는경로도진단접두를실행한다.
        for layout in (0,2,4):result.append(case(checks=1,money=common.bits(0.0,4),peer=0,layout=layout,mutation=mutation))
    result.append(case(layout=2,gumpType=259))
    unique={}
    # 동일원본입력을새고유수로합산하지않는다.
    for value in result:unique.setdefault(json.dumps(value,sort_keys=True),value)
    return list(unique.values())


def slots(layout):
    """부모와세종속의전체50BYTE입력이다. 기대효과/출력은계산하지않는다."""
    raw={sid:bytearray(50) for sid in (50,60,61,62)}
    struct.pack_into('<I',raw[50],0,0x50ee08);raw[50][10]=152
    if layout:
        struct.pack_into('<H',raw[50],6,60)
        # 세종속이같은입력순서로연결되며free/dead도임의제외하지않는다.
        for sid,next_sid,typ,owner in ((60,61,6 if layout==1 else 3,7),(61,62,6,8),(62,0,3,9)):
            struct.pack_into('<H',raw[sid],4,next_sid);raw[sid][10]=typ;raw[sid][11]=3 if layout==4 else 0;struct.pack_into('<I',raw[sid],18,owner)
        if layout==5:raw[60][10]=2;raw[62][10]=2
    return raw


class Oracle:
    """현재 세몸체와실제등록표를실행하고SP/최종가상UI만관찰경계로둔다."""
    def __init__(self):
        """현재보호PE/정밀범위/가상입력과실행감사를준비한다."""
        if common.digest(common.BINARY)!=common.SHA:raise RuntimeError('Player내용물보호PE SHA불일치')
        pe=common.pefile.PE(str(common.BINARY));self.data=pe.get_memory_mapped_image();self.base=pe.OPTIONAL_HEADER.ImageBase
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);self.mu.mem_map(self.base,(len(self.data)+4095)&~4095);self.mu.mem_write(self.base,self.data)
        # 원본주소를호스트실행하지않는raw/가상UI/stack매핑이다.
        for address,size in ((POOL,0x10000),(GUMP,0x1000),(STOP,0x1000),(common.STACK_BASE,0x10000)):self.mu.mem_map(address,size)
        self.table=ROOT/'extracted/player-content-notice/originals/functions.tsv';self.allowed=[];self.entries={}
        with self.table.open(encoding='utf-8') as fp:
            # 실제선택export의포함끝주소를각각허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries[int(row['entry'],16)]=row['entry']
                # 불연속범위사이코드는허용하지않는다.
                for part in row['ranges'].split(';'):
                    low,high=(int(item,16) for item in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write);self.native=collections.Counter();self.boundaries=collections.Counter();self.returns=0

    def get(self,address,width=4):
        """원본폭그대로현재메모리를읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """명시입력/외부효과만원본폭으로기록한다."""
        self.mu.mem_write(address,(value&((1<<(8*width))-1)).to_bytes(width,'little'))

    def ret(self,result=0,cleanup=0):
        """명시외부callee의실제정리규약으로복귀한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,result);self.mu.reg_write(UC_X86_REG_EIP,self.get(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup)

    def instruction(self,mu,address,size,data):
        """실제몸체진입/현재SP/가상UI를감사하며다른함수나OS는거부한다."""
        if address in self.entries:self.native[self.entries[address]]+=1
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x40ec50:
            self.boundaries['money']+=1;owner=self.get(esp+4);self.trace.append('M:'+str(owner));self.put(self.get(esp+8),self.c['money'])
            if self.c['mutation']==1:self.put(0x540cb8,91);self.put(0x5c89c4,0)
            self.ret();return
        if address==HANDLE:
            self.boundaries['handle']+=1;token=(mu.reg_read(UC_X86_REG_ECX)-GUMP)//16;args=[self.get(esp+offset) for offset in (4,8,12)];self.trace.append('H:'+':'.join(map(str,[token,*args])))
            if self.c['mutation']==2:self.put(POOL+60*50+4,0,2);self.put(0x5427bc,0x11223344);self.put(REGISTRY+9*4,GUMP+9*16)
            elif self.c['mutation']==3:self.put(0x54107c,6)
            self.ret(cleanup=12);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미허용Player내용물실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제몸체의진단코드/잔여DWORD와stack밖쓰기를거부한다."""
        if not (common.STACK_BASE<=address and address+size<=common.STACK_BASE+0x10000 or address in (0x5318ec,0x5c89c4) and size==4):raise RuntimeError(f'미허용Player내용물쓰기 {address:08x}')

    def run(self,value):
        """현재원본입력의전체raw/현재11전역/효과순서를정상반환에서기록한다."""
        self.c=dict(value);self.trace=[];mu=self.mu;mu.mem_write(POOL,bytes(0x10000));self.put(0x5c8464,POOL)
        # 전체네슬롯의명시입력을공급한다.
        for sid,raw in slots(value['layout']).items():mu.mem_write(POOL+sid*50,bytes(raw))
        # 현재전역은정확한DWORD/WORD폭으로초기화한다.
        for key,address,width in FIELDS:self.put(address,value[key],width)
        mu.mem_write(REGISTRY,bytes(4000));self.put(VTABLE+0x28,HANDLE)
        # 등록된현재가상노드만opaque주소로공급한다.
        for token in (7,8,9):self.put(GUMP+token*16,VTABLE);self.put(REGISTRY+token*4,0 if value['layout']==3 else GUMP+token*16)
        # 저장레지스터와x87의원본초기상태다.
        for reg,sentinel in common.SAVED.items():mu.reg_write(reg,sentinel)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        entry=0x490750 if value['wrapper'] else 0x4d55e0;args=[value['form'],value['reason']] if value['wrapper'] else [PARENT,value['form'],value['reason']]
        mu.reg_write(UC_X86_REG_ECX,POOL+PARENT*50 if value['wrapper'] else 0);mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(entry,STOP,count=100000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+(8 if value['wrapper'] else 0):raise RuntimeError('Player내용물정상반환/stack오류')
        if any(mu.reg_read(reg)!=sentinel for reg,sentinel in common.SAVED.items()):raise RuntimeError('Player내용물보존레지스터오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=0x027f or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Player내용물x87정상반환오류')
        self.returns+=1
        return [','.join(str(self.get(address,width)) for _,address,width in FIELDS),'|'.join(self.trace) or '-',','.join(mu.mem_read(POOL+sid*50,50).hex() for sid in (50,60,61,62))]


def generate():
    """실제정상출력과파일해시를저장하며C++기대값을호출하지않는다."""
    oracle=Oracle();rows=[]
    # 입력별실제최상위정상반환을각각한번기록한다.
    for value in cases():rows.append([*[value[key] for key in KEYS],*oracle.run(value)])
    FIXTURE.write_text('# PATCH Player content notice 실제PE. GUI/OS0.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),Path(common.__file__),common.BINARY,FIXTURE,oracle.table,oracle.table.with_name('creation.c'),ROOT/'tools/ghidra/player-content-notice-functions.json']
    REPORT.write_text(json.dumps(dict(host=common.socket.gethostname(),date='2026-10-15',total=len(rows),returns=oracle.returns,native=dict(oracle.native),boundaries=dict(oracle.boundaries),os_calls=0,
        files={path.relative_to(ROOT).as_posix():common.digest(path) for path in paths},limits=['Player+24/실제진단접두/현재종속순회/현재Gump등록표는기계어다.SP출력과최종Gump가상handle은명시경계다.',
            'Content삭제/empty84공통삭제는기존독립기계어근거를재사용하며신규native수로더하지않는다.','실제Session/UI설치나완주검증은아니다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Player내용물실제PE',len(rows),'정상반환',dict(oracle.native),flush=True)


def verify():
    """해시/정상반환수/명시입력계획을원본재실행없이감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];plan=cases()
    # 저장근거파일의전체SHA를재검사한다.
    for path,sha in report['files'].items():
        if common.digest(ROOT/path)!=sha:raise RuntimeError('Player내용물근거SHA불일치 '+path)
    if len(rows)!=len(plan) or len(rows)!=report['returns'] or report['os_calls']:raise RuntimeError('Player내용물정상반환수불일치')
    # 기대출력은재계산하지않고입력계획열만확인한다.
    for row,value in zip(rows,plan):
        if len(row)!=20 or row[:17]!=[str(value[key]) for key in KEYS]:raise RuntimeError('Player내용물입력계획불일치')
    print('Player내용물근거감사통과',len(rows),flush=True)


def main():
    """원본관찰생성/저장근거감사중요청한동작을수행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate()


if __name__=='__main__':main()
