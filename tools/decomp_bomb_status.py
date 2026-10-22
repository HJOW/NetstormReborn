#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""은신/마비의 실제 자격·적용과 방향 지정 플레어를 보호 PE에서 독립 관측한다."""
import argparse
import itertools
import json
import socket
import struct
import capstone
import pefile
from decomp_arc_spire import Oracle as BaseOracle, ROOT, HEAP, STOP, STACK, VTABLE
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from decomp_flare import Oracle as FlareOracle, inputs as flare_inputs
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

POOL=0x15000000  # 게임/OS와 분리한 실제50BYTE 풀이다.
REFRESH,UNPOP,REPOP=STOP+0x600,STOP+0x620,STOP+0x640  # 현재 가상88/48/4c의 큰 효과 경계다.
PACKET=HEAP+0x100  # 송신 observer의 명시 버퍼다.
FIXTURE=ROOT/'cpppj/tests/fixtures/bomb-status-x86.tsv'  # 새 원문이며 기존 자료를 덮어쓰지 않는다.
FLARES=ROOT/'cpppj/tests/fixtures/bomb-status-flare-x86.tsv'  # 기존139입력과 분리한 C/D 방향 원문이다.
REPORT=ROOT/'cpppj/bomb-status-20261023.json'  # 이번 PC/추가 정밀 분석/실행/SHA 근거다.


def cases():
    """운반체 제외/동맹 사제/상태/현재 TYPE·플레어의 변화와 자격을 교차한다."""
    rows=[]
    for mode,kind,genus,state,extra,same,ally in itertools.product(('I','P'),(133,136,158),(0,4,0x1000000,0x200000,0x170000),(0,7),(0,48),(0,1),(0,1)):
        rows.append((mode,kind,genus,state,extra,same,ally,0))  # 판정 결과를 계산하지 않는 명시 입력이다.
    for mode,kind,genus,mutation in itertools.product(('I','P'),(133,136,158),(0,4,0x200000),range(1,4)):
        rows.append((mode,kind,genus,0,0,0,0,mutation))  # 가상 효과 뒤 현재 TYPE/전역을 바꾼다.
    for mode,kind,genus in itertools.product(('i','p'),(133,136,158,164,126),(0,4,0x1000000,0x200000,0x170000,0x400000,0x80000000)):
        rows.append((mode,kind,genus,0,0,0,0,0))  # 범위 밖 비트와 특정 TYPE 예외다.
    for count,writing in itertools.product((0,1,100,65524),(0,1)):
        rows.append(('H',count,writing,0,0,0,0,0))  # 길이 WORD 감김과 비송신 무쓰기를 구별한다.
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """새로 찾은 두 가상 함수와 기존 자격/observer 전체를 실제 명령으로 실행한다."""
    def __init__(self):
        """원본 ret 명령까지의 정확한 새 범위와 메모리 가드를 설치한다."""
        super().__init__();self.mu.mem_map(POOL,0x200000);self.allowed=[];ranges(self,(0x41eca0,0x41fdb0,0x40b390));self.returns=0;self.extraRanges=[]
        pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
        for entry in (0x41fc50,0x41fdd0):
            code=list(capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32).disasm(pe.get_data(entry-0x400000,256),entry));end=next(i.address+i.size for i in code if i.mnemonic=='ret');self.allowed.append((entry,end));self.extraRanges.append((entry,end))  # 현재 PE와 추가 디컴파일의 시작/ret를 함께 고정한다.

    def number(self,address,width=4):
        """현재 비정렬 BYTE/WORD/DWORD를 정확한 폭으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def read(self,mu,access,address,size,value,data):
        """원본/격리 자료/가상 표/스택 밖 읽기를 거부한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0x400000,0x600000),(POOL,POOL+0x200000),(HEAP,HEAP+0x10000),(VTABLE,VTABLE+0x1000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'상태 주문 읽기 범위 {address:x}+{size}')

    def write(self,mu,access,address,size,value,data):
        """실제 상태 비트/패킷/스택 외 쓰기를 거부한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((POOL,POOL+0x200000),(HEAP,HEAP+0x10000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'상태 주문 쓰기 범위 {address:x}+{size}')

    def instruction(self,mu,address,size,data):
        """현재 TYPE 조회만 경계로 두고 효과 분기/동맹/현재 가상 호출 순서는 원문을 실행한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=[self.number(sp+4+i*4) for i in range(5)];sid=(ecx-POOL)//50
        if address in (0x41fc50,0x41fdd0,0x41eca0,0x41fdb0,0x40b390):self.calls[f'{address:08x}']+=1
        if address==0x4ac200:self.ret(self.genus^(0x200000 if self.number(ecx+10,1)==136 else 0));return
        if address in (REFRESH,UNPOP,REPOP):
            tag={REFRESH:'R',UNPOP:'U',REPOP:'P'}[address];self.event(tag,sid,*(() if tag=='R' else (args[0],)))
            if self.mutation==1:self.put(POOL+60*50+10,136,1)
            if self.mutation==2:self.put(0x541200,107)
            if self.mutation==3:self.put(POOL+60*50+11,7,1)
            self.ret(purge=0 if address==REFRESH else 4);return
        if address==0x457e20:
            self.event('F',args[0])
            if self.mutation==1:self.put(0x541200,107)
            self.ret();return
        if address==0x45a480:self.event('L',*args);self.ret();return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'상태 주문 미검토 명령 {address:x}')

    def sample(self,row,control):
        """명시 입력을 설치하고 전체 부모/대상·패킷·반환·호출만 관측한다."""
        mode,kind,self.genus,state,extra,same,ally,self.mutation=row;self.events=[]
        for sid in (50,60):
            self.mu.mem_write(POOL+sid*50,bytes(50));self.put(POOL+sid*50,VTABLE);self.put(POOL+sid*50+34,1 if sid==50 else 2,1)  # 서로 다른 두 owner를 기본 입력으로 둔다.
        self.put(POOL+50*50+12,60 if same else 0,2);self.put(POOL+60*50+10,kind,1);self.put(POOL+60*50+11,state,1);self.put(POOL+60*50+40,extra,1)
        for offset,entry in ((0x88,REFRESH),(0x48,UNPOP),(0x4c,REPOP)):self.put(VTABLE+offset,entry)  # 현재 가상 효과 토큰이다.
        for address,value in ((0x5c8464,POOL),(0x5412d0,158),(0x541278,136),(0x5412e8,164),(0x541250,126),(0x541200,106),(0x595200+(1*9+2)*4,ally)):self.put(address,value)  # 원본이 읽는 현재 전역이다.
        self.mu.mem_write(PACKET,b'\xa5'*32)
        if mode=='H':self.put(PACKET+16,kind,2);self.call(0x40b390,PACKET,struct.pack('<I',self.genus),4,control);result='-'
        elif mode in ('i','p'):
            self.call(0x41eca0 if mode=='i' else 0x41fdb0,0,struct.pack('<I',60),4,control);result=str(self.mu.reg_read(UC_X86_REG_EAX))
        else:self.call(0x41fc50 if mode=='I' else 0x41fdd0,POOL+50*50,struct.pack('<I',60),4,control);result='-'
        self.returns+=1
        return result,';'.join(self.events) or '-',b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in (50,60)).hex(),bytes(self.mu.mem_read(PACKET,32)).hex()


class DirectionFlare(FlareOracle):
    """기존 원문 실행기의 명시 입력을 그대로 쓰되 진입 인자만 실제45a480의5개로 공급한다."""
    def instruction(self,mu,address,size,data):
        """기본 wrapper 진입 지점에서 요청한 C/D 방향으로 직접 호출 harness를 전환한다."""
        if address==0x45a6c0:
            sp=mu.reg_read(UC_X86_REG_ESP);old=self.words(sp,4);self.mu.mem_write(sp,struct.pack('<6I',old[0],old[1],old[2],old[3],106,self.direction));mu.reg_write(UC_X86_REG_EIP,0x45a480);return
        super().instruction(mu,address,size,data)


def main():
    """새 원문 관측과 현재 바이트의 읽기 전용 검증을 분리한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for p,h in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/p)==h,p  # 제품 출력으로 원문을 다시 만들지 않는다.
        print('상태 주문 원문/SHA 확인');return
    first,second=Oracle(),Oracle();rows=[]
    for row in cases():
        a=first.sample(row,0x27f);b=second.sample(row,0x37f);assert a==b,row;rows.append((','.join(map(str,row)),*a))  # 두 정밀도의 정상 반환/전체 비트가 같아야 한다.
    FIXTURE.write_text('# input result effects parent,target packet32\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    flare=DirectionFlare();flares=[]
    for row,direction in itertools.product((r for r in flare_inputs() if r[0]=='Spawn'),(2,3)):
        flare.direction=direction;a=flare.run(row,0x27f);b=flare.run(row,0x37f);assert a==b,(row,direction);flares.append((str(direction),','.join(map(str,row[1:])),*map(str,a)))  # 기존 A 방향 원문139행을 변경하지 않고 새 두 방향을 관측한다.
    FLARES.write_text('# direction input return random slots effects\n'+'\n'.join('\t'.join(r) for r in flares)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','extracted/cpp-bomb-status-20261023-decomp.c','tools/decomp_bomb_status.py','tools/decomp_flare.py','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix(),FLARES.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',additionalDecompileHost=socket.gethostname(),additionalDecompileDate='2026-10-23',uniqueInputs=len(rows),normalReturns=first.returns+second.returns,nativeInstructions=first.instructions+second.instructions,precisionDifferentInputs=[],extraRanges=first.extraRanges,nativeCalls=dict(first.calls+second.calls),flareInputs=len(flares),flareReturns=flare.returns,flareInstructions=flare.instructions,sha256={p:digest(ROOT/p) for p in paths},limitations=['은신/마비 적용·자격/observer 및 방향 지정 플레어 전체 root다. 상태 가상88/48/4c·TYPE조회·457e20·45a480은 첫 원문에서 큰 경계이며 플레어는 별도 원문에서 전체를 실행한다.','전체 GUI/사용자 플레이/미션·Edit 완료로 확대하지 않는다.10.72 새 상태 효과 대응은 미확정이다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('상태 주문',len(rows),facts['normalReturns'],facts['nativeInstructions'],'플레어',len(flares),flare.returns)


if __name__=='__main__':main()
