#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 애시드 수신/헤더와 콜백 뒤 현재 패킷·owner·가상 표를 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, POOL, TYPES, VTABLE, digest
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

ENTRIES=(0x48bfe0,0x40ad30,0x4df230,0x4ac200,0x4abaf0)  # 실제 수신·헤더·길이·genus·owner 플래그의 전체 몸체다.
PACKET,DESTROY,ALT_DESTROY=STOP+0x900,STOP+0x100,STOP+0x120  # 명시 입력/두 가상 삭제 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/acid-zap-x86.tsv'  # 최초 원문이며 기존 자료를 덮어쓰지 않는다.
REPORT=ROOT/'cpppj/acid-zap-20261022.json'  # 호스트/정상 반환/원본과 제품의 SHA 근거다.


def capture_methods():
    """보호 PE의13종 각각이 같은 기본/추가 가상 함수를 쓰는지 확인한다."""
    import pefile
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));result=[]
    for typ,table in ((70,0x508cc8),(73,0x50a1e8),(111,0x5040c0),(133,0x50e568),(135,0x503e50),(137,0x503d80),(138,0x50a050),(141,0x5040c0),(144,0x50a388),(145,0x50e738),(146,0x504198),(156,0x503f20),(158,0x50f210)):  # 정확한 타입/VT 쌍이다.
        entries=[struct.unpack('<I',pe.get_data(table-0x400000+offset,4))[0] for offset in (0x78,0xa0,0xa4)]
        assert entries==[0x4acfe0,0x41c0f0,0x41c100];result.append(dict(type=typ,vtable=f'{table:08x}',methods=[f'{entry:08x}' for entry in entries]))
    return result


def cases():
    """필터/현재 제외TYPE/owner/방향·입력 무시·재조회와 보정 없는 선언길이를 교차한다."""
    base=(0,0,0,133,0x10000,164,150,2,1,1,0,0,1,0x61,11);rows=[]
    for vf,pf,typ,genus in itertools.product((0,1,2,3,4,8,128),(0,1,2,3,4,8),(133,150,164,158),(0,0x10000,0x200000,0x210000)):
        row=list(base);row[1:5]=vf,pf,typ,genus;rows.append(tuple(row))
    for owner,orientation,mutation in itertools.product((0,1,8,255),(-128,-1,0,1,127),range(11)):
        row=list(base);row[7:9]=owner,orientation;row[11]=mutation;rows.append(tuple(row))
    for sender,flags,altar,growing in itertools.product((0,1,0xffffffff),(0,16,0xffffffff),(133,164),(133,150)):
        row=list(base);row[5:7]=altar,growing;row[9:11]=sender,flags;rows.append(tuple(row))
    for protocol,message,length in itertools.product((0,1,0x7f,0x80,0xff),(0,0x61,0xff),(0,1,3,4,11,0xffff)):
        row=list(base);row[0]=1;row[12:15]=protocol,message,length;rows.append(tuple(row))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """가상 효과/음향/캡처만 분리하며 필터와 현재 필드 계산을 원문으로 실행한다."""
    def __init__(self):
        """기존 격리 PE/메모리에 새 함수 범위와 엄격한 읽기 가드만 설치한다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.mu.hook_add(UC_HOOK_MEM_READ,self.read)
        self.put(VTABLE+0x10,DESTROY);self.put(VTABLE+0x310,ALT_DESTROY)

    def mutate(self,phase):
        """소리/캡처의 명시 콜백이 같은 raw와 현재 패킷을 바꾸는 입력이다."""
        m=self.case[11]
        if phase=='sound':
            if m==1:self.put(POOL+60*50+34,7,1)
            if m==2:self.put(PACKET+6,61,2)
            if m==3:self.put(PACKET+4,51,2)
            if m==4:self.put(PACKET+8,63,2);self.put(PACKET+10,255,1)
            if m==5:self.put(POOL+50*50,VTABLE+0x300)
        if phase=='dissolve':
            if m==6:self.put(POOL+60*50+34,9,1)
            if m==7:self.put(POOL+50*50,VTABLE+0x300)
            if m==8:self.put(POOL+50*50+11,3,1)
            if m==9:self.put(PACKET+6,61,2)
            if m==10:self.put(POOL+60*50+34,0,1);self.put(POOL+50*50,VTABLE+0x300)

    def instruction(self,mu,address,size,data):
        """원본 외부 호출의 모든 인수와 당시 현재 가상 표를 순서대로 기록한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<4I',mu.mem_read(sp+4,16))
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x49a840:self.ret(TYPES+args[0]*0x1ec);return
        if address==0x4a9d70:
            self.events.append('S:'+':'.join(map(str,args[:2]))+':'+self.string(args[2])+':'+str(args[3]));self.mutate('sound');self.ret();return
        if address==0x450510:self.event('F',*args);self.mutate('dissolve');self.ret(0x12345678);return
        if address in (DESTROY,ALT_DESTROY):self.event('D' if address==DESTROY else 'd',(mu.reg_read(UC_X86_REG_ECX)-POOL)//50,args[0]);self.ret(0,4);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'애시드 범위 밖 실행 {address:x}')

    def write(self,mu,access,address,size,value,data):
        """수신은 스택만, 헤더 생성은 현재 네 바이트만 쓰도록 제한한다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or PACKET<=address and address+size<=PACKET+4:return
        raise RuntimeError(f'애시드 범위 밖 쓰기 {address:x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호 PE·명시 raw/TYPE/가상 표/패킷/스택 밖 읽기를 차단한다."""
        spans=((0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(VTABLE,VTABLE+0x1000),(STOP,STOP+0x1000),(STACK-0x7f000,STACK+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'애시드 범위 밖 읽기 {address:x}+{size}')

    def sample(self,case,precision):
        """현재 입력만 공급하며 필터·재조회·출력은 실제 정상 반환에서 읽는다."""
        self.case=case;self.events=[];c=case
        for sid in (50,51,60,61,62,63):  # 대상 두 개와 폴 네 개는 원본50바이트다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=c[3] if sid<60 else 145;raw[11]=c[1] if sid==50 else c[2] if sid==60 else 0;raw[34]=c[7] if sid==60 else 3;struct.pack_into('<2f',raw,14,20.75,21.5);self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(0x5c8464,POOL);self.put(TYPES+c[3]*0x1ec+0xec,c[4]);self.put(0x5412e8,c[5]);self.put(0x5412b0,c[6]);self.mu.mem_write(PACKET,struct.pack('<BB4Hb',c[12]&0x7f,c[13],c[14],50,60,62,c[8]))
        self.put(0x52ee42,c[13],1);self.put(0x52ee43,c[12],1);self.put(0x52ee44,c[14],2)
        entry=0x40ad30 if c[0] else 0x48bfe0;self.call(entry,PACKET if c[0] else 0,b'' if c[0] else struct.pack('<3I',c[9],c[10],PACKET),0,precision);self.returns+=1
        return ';'.join(self.events) or '-',','.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in (50,51,60,61,62,63)),bytes(self.mu.mem_read(PACKET,11)).hex()


def main():
    """고유 입력을 두 정밀도로 관찰하거나 동결 원문/제품의 SHA만 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'));assert capture_methods()==facts['plainUnitCaptureMethods']
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 실제 비교 바이트의 출처를 확인한다.
        rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs'] and len({row[0] for row in rows})==len(rows);print('애시드 수신 SHA/고유 입력:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[]
    for case in cases():  # 다른 정밀도 결과를 추가 입력으로 세지 않는다.
        results=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))];assert results[0]==results[1],case;rows.append((','.join(map(str,case)),*results[0]))
    FIXTURE.write_text('# input effects raws packet\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_acid_zap.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),plainUnitCaptureMethods=capture_methods(),normalReturns=sum(oracle.returns for oracle in oracles),nativeInstructions=sum(oracle.instructions for oracle in oracles),precisionDifferentInputs=[],nativeCalls=dict(sum((oracle.calls for oracle in oracles),collections.Counter())),sha256={path:digest(ROOT/path) for path in paths},limitations=['48bfe0/40ad30/4df230/4ac200/4abaf0 전체는 원문이며 TYPE 주소 조회·소리·분해 생성·가상 삭제만 명시 경계다.','콜백 뒤 현재 패킷과 원래 대상/첫 폴의 raw 슬롯을 별도 관측한다. 이 자료만으로145의범위 포획이나 실제 캠페인 연결을 완료 처리하지 않는다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('애시드 수신:',len(rows),'입력/두 정밀도 차이0')


if __name__=='__main__':main()
