#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제10.78 커서 취득/취소/임시 삭제의 전체 전역·WORD·가상 효과 순서를 격리 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_particle_gradient import Oracle as CallOracle,ROOT,STACK,STOP,digest
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

POOL,TYPES,VTABLE,IMAGE,PROCESS=0x12000000,0x12100000,0x12200000,0x12300000,0x12400000  # 격리 raw/TYPE/가상 표/SHP/마나 프로세스다.
SOURCE,NOTICE,DETACH,UNPOP,DESTROY,EVENT=[STOP+0x100+32*index for index in range(6)]  # 가상 효과 경계의 서로 다른 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/cursor-lifecycle-x86.tsv'  # 새 원문이며 기존 내려놓기2058행을 덮어쓰지 않는다.
REPORT=ROOT/'cpppj/cursor-lifecycle.json'  # 실제 PC/함수/경계/고유 입력·SHA 근거다.
# 비교 순서대로 현재 전역 주소를 지정한다. 호스트 주소로 바뀌는 TYPE/SHP 식별값은 따로 정규화한다.
GLOBALS=[0x5325c8,0x55a284,0x55a288,0x55a2a4,0x55a2a0,0x55a2a8,0x55a2ac,0x55a2e0,0x55a274,0x55a27c,0x55a278,
    0x55a1b8,0x55a268,0x55a198,0x55a19c,0x55a250,0x55a26c,0x55a1a0,0x55a270,0x55a1d0]
WIDTHS,HEIGHTS=[20,33,-3,120,55],[10,30,-1,20,17]  # 일반 글자 측정 경계의 명시 입력이다.


def cases():
    """성공 취득/기존 플래그 반환/잠금/죽은 Form·부모/콜백 재조회를 별도 입력으로 만든다."""
    result=[]
    # 열: operation,flags,oldWord,argument,typeFlags,costBits,lock,preview,dead,parentFlags,highlightChange,prepareChange,mana,range,legalGump,manaGump,imageVariant.
    base=[0,0x160,0x20,0,0,struct.unpack('<I',struct.pack('<f',250.75))[0],1,0,0,0,0,0,1,0,0,0,0]
    # TYPE/flags/기존 WORD/기본 프레임·GUI 선택·마나/범위의 우선순위를 교차한다.
    for flags,old,argument,kind in itertools.product((0,0x41,0x160,0x180),(8,0x20,0x80,0xffff),(0,7),(0,4,0x200,0x800,0xa00)):
        row=base.copy();row[1:5]=flags,old,argument,kind;result.append(row)
    # NaN/음수/정수64비트 변환과 signed 그림 치수·측정 최대값을 별도로 관찰한다.
    for cost,image in itertools.product((0,0x80000000,0xc0700000,0x4f000000,0x4f32d05e,0x7fc01234),(0,1)):
        row=base.copy();row[5]=cost;row[16]=image;result.append(row)
    # 첫 마나0/두 번째 프로세스/GUI 상태 변경을 교차한다. 기존 커서 프로세스 덮어쓰기를 보존한다.
    for mana,ran,legal,gump in itertools.product((0,1),(0,1),(0,1),(0,1)):
        row=base.copy();row[4]=0xa00;row[12:16]=mana,ran,legal,gump;result.append(row)
    # 취소는 원본 BYTE0x80/WORD 복구와 현재 깊이/preview/임시 개수를 읽는다.
    for lock,preview,dead,word,flags,parent in itertools.product((0,1,0xffffffff),(0,50),(0,1),(8,0x80,0x8080),(0,0x80,0x100,0x180,0x200),(0,4)):
        row=base.copy();row[0]=1;row[1]=flags;row[2]=word;row[6:10]=lock,preview,dead,parent;result.append(row)
    # 강조가 잠금/현재 Form을 바꾸거나 준비가 실제 임시 개수를 줄이는 입력이다.
    for highlight,prepare in itertools.product((0,1,2),(0,1)):
        row=base.copy();row[0]=1;row[6]=0;row[7]=50;row[10:12]=highlight,prepare;result.append(row)
    # 별도Activate는 force와 flags0x200 및 음수 signed 개수를 검사한다.
    for force,flags,count in itertools.product((0,1),(0,0x200),(0,2,0xffffffff)):
        row=base.copy();row[0]=2;row[1]=flags;row[6]=force;row[7]=count;result.append(row)
    return list(dict.fromkeys(tuple(row) for row in result))


class Oracle(CallOracle):
    """세 수명 몸체와 실제 CRT 절삭만 실행하고 UI/월드 가상 효과를 기록한다."""
    def __init__(self):
        """보호 PE와 격리 자료를 준비하며 실제 파일/OS/창에 진입하지 않는다."""
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);image=pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000,(len(image)+4095)&~4095);self.mu.mem_write(0x400000,image)
        # SEH FS:0도 격리 페이지이며 원본 운영체제의 예외 체인에 쓰지 않는다.
        for address,size in ((0,0x1000),(POOL,0x10000),(TYPES,0x20000),(VTABLE,0x1000),(IMAGE,0x1000),(PROCESS,0x1000),(STACK-0x7f000,0x80000),(STOP,0x1000)):self.mu.mem_map(address,size)
        self.put(VTABLE,SOURCE);self.put(VTABLE+0x10,DESTROY);self.put(VTABLE+0x24,NOTICE);self.put(VTABLE+0x48,DETACH);self.put(VTABLE+0x4c,UNPOP);self.put(VTABLE+0x1c,EVENT);self.put(PROCESS,VTABLE)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.allowed=[];self.returns=0
        # 실제 세 몸체와 _ftol의 불연속 범위만 허용한다.
        for entry in (0x447000,0x446e70,0x445aa0,0x4e49c0):
            # 각 구간의 마지막 명령 바이트도 포함한다.
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def put(self,address,value,width=4):
        """DWORD 감김과 실제 WORD 폭으로 격리 입력/경계 결과를 쓴다."""
        self.mu.mem_write(address,int(value&((1<<(8*width))-1)).to_bytes(width,'little'))

    def get(self,address):
        """현재 비정렬 DWORD를 little endian으로 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def string(self,address):
        """명시 문구/비용의 NUL 종료 바이트를 읽는다."""
        result=bytearray()
        # 실제256바이트 서식 버퍼보다 큰 문자열을 허용하지 않는다.
        for offset in range(256):
            value=self.mu.mem_read(address+offset,1)[0]
            if not value:return result.decode('ascii')
            result.append(value)
        raise ValueError('커서 문자열 NUL 없음')

    def ret(self,value=0,purge=0):
        """경계의 실제 cdecl/thiscall 스택 정리 규약으로 복귀한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);target=self.get(esp);self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def event(self,letter,*values):
        """전체 DWORD/문자열 인자를 호출 순서대로 기록한다."""
        self.events.append(':'.join(map(str,(letter,*values))))

    def instruction(self,mu,address,size,data):
        """외부 효과가 현재 전역을 바꾸는 명시 입력도 실행하며 본체 계산을 대체하지 않는다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<6I',mu.mem_read(esp+4,24));ecx=mu.reg_read(UC_X86_REG_ECX);sid=(ecx-POOL)//50
        if address==SOURCE:self.event('U',sid);self.ret(77);return
        if address in (NOTICE,DETACH,UNPOP,DESTROY):
            letter={NOTICE:'N',DETACH:'D',UNPOP:'O',DESTROY:'K'}[address];self.event(letter,sid,*args[:2 if address==NOTICE else 1]);self.ret(0,8 if address==NOTICE else 4);return
        if address==EVENT:self.event('E',1 if ecx==PROCESS else 2,*args[:3]);self.ret(0,12);return
        if address==0x4d5bb0:
            self.event('H',args[0]);change=self.case[10]
            if change==1:self.put(0x5325c8,3)
            if change==2:self.put(0x55a2a8,51)
            self.ret();return
        if address==0x4458e0:
            self.event('P');self.prepared+=1
            if self.case[11] and self.prepared==1:self.put(0x55a1b8,1)
            self.ret();return
        if address==0x4a4410:self.event('C',args[0]);self.ret();return
        if address==0x41bf10:self.event('R',1 if ecx==PROCESS else 2,args[0]);self.ret(0,4);return
        if address==0x4a9cb0:self.event('S',self.string(args[0]),*args[1:]);self.ret();return
        if address==0x40eec0:self.event('A',args[0]);self.ret();return
        if address==0x466710:self.event('X');self.ret();return
        if address==0x49a840:self.ret(TYPES+args[0]*500);return
        if address==0x445b30:
            self.event('B');self.put(0x55a1d8,100);self.put(0x55a1dc,101);self.put(0x55a1b8,2);self.put(0x55a27c,44 if self.case[14] else 0);self.ret();return
        if address in (0x4741d0,0x474900):
            mana=address==0x4741d0;self.event('M' if mana else 'G',args[0],args[1]);value=PROCESS if mana and self.case[12] else PROCESS+16 if not mana else 0
            if mana:self.put(0x55a278,45 if self.case[15] else 0)
            self.ret(value);return
        if address==0x49adc0:self.event('F',(ecx-TYPES)//500);self.ret(IMAGE);return
        if address==0x474240:self.event('L',1);self.ret(self.case[13]);return
        if address==0x445440:self.event('Q');self.put(0x55a278,46);self.ret();return
        if address==0x4de9a0:self.ret(args[0],4);return
        if address==0x4a3df0:
            text=self.string(args[0]);index=self.measured;self.measured+=1;context=','.join(map(str,struct.unpack('<8I',mu.mem_read(args[3],32))));self.event('T',text,context,args[4]);self.put(args[1],WIDTHS[index] if index<5 else 37);self.put(args[2],HEIGHTS[index] if index<5 else (12 if not self.case[16] else -9));self.ret();return
        if address==0x4e58e9:
            assert self.string(args[1])=='%d';value=struct.unpack('<i',struct.pack('<I',args[2]))[0];mu.mem_write(args[0],str(value).encode('ascii')+b'\0');self.ret(len(str(value)));return
        if address==0x446b50:self.event('Z',args[0],args[1]);self.ret();return
        if address==0x4e0620:raise RuntimeError('원본 진단: '+self.string(args[0]))
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'커서 수명 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제 커서 전역·대상 raw·격리 SEH/스택 밖 쓰기를 차단한다."""
        if not any(low<=address and address+size<=high for low,high in ((0,4),(0x5325c8,0x532624),(0x55a198,0x55a440),(POOL,POOL+0x10000),(STACK-0x7f000,STACK+0x1000))):raise RuntimeError(f'커서 수명 범위 밖 쓰기 {address:08x}')

    def sample(self,case,precision):
        """입력을 새로 주고 실제 함수의 정상 반환·전체 상태와 raw50/51/77을 읽는다."""
        self.case=case;self.events=[];self.prepared=self.measured=0;mu=self.mu
        mu.mem_write(0x55a198,b'\0'*(0x55a440-0x55a198));mu.mem_write(0,b'\0'*4)
        # raw의 무관한 바이트를 표식으로 채워 WORD 쓰기 경계를 검증한다.
        for sid in (50,51,77,100,101):
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,VTABLE);raw[11]=case[8] if sid in (50,51) else case[9] if sid==77 else 0;struct.pack_into('<H',raw,12,case[3]);struct.pack_into('<H',raw,18,100);struct.pack_into('<H',raw,20,case[2]);mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(0x5c8464,POOL);self.put(TYPES+100*500+0x118,11);self.put(TYPES+100*500+0xec,case[4]);self.put(TYPES+100*500+0xc4,case[5]);self.put(TYPES+92*500+0x114,4);self.put(TYPES+92*500+0xdc,IMAGE);self.put(IMAGE+0x20,0x100)
        mu.mem_write(IMAGE+0x100-36+24,struct.pack('<4h',23 if not case[16] else -11,19 if not case[16] else -7,-3,5))
        self.put(0x55a280,TYPES+100*500);self.put(0x55a284,100);self.put(0x55a2ac,case[2]);self.put(0x55a2e0,case[1]);self.put(0x5325c8,case[6]);self.put(0x55a2a8,case[7] if case[0]!=2 else 0);self.put(0x55a274,PROCESS if case[0]==1 else 0)
        self.put(0x55a1b8,case[7] if case[0]==2 else 2);self.put(0x55a1d8,100);self.put(0x55a1dc,101);self.put(0x5e4794,0);self.put(0x5bec2c,17);self.put(0x59afd8,19)
        mu.mem_write(0x5325e4,struct.pack('<8I',2,0,0,0,0,0,1,1));mu.mem_write(0x532604,struct.pack('<8I',2,3,0,0,0,0,1,1));self.put(0x5cae94,0xfffffffd);self.put(0x5cae98,47)
        entry=(0x447000,0x446e70,0x445aa0)[case[0]];arguments=struct.pack('<II',50,case[1]) if not case[0] else struct.pack('<I',case[6]) if case[0]==2 else b''
        self.call(entry,0,arguments,0,precision);self.returns+=1
        values=[self.get(address) for address in GLOBALS];values[8]=0 if not values[8] else 1 if values[8]==PROCESS else 2
        values += [self.get(0x55a254+4*index) for index in range(5)]+[self.get(0x55a1a4+4*index) for index in range(5)]
        values += list(struct.unpack('<8I',mu.mem_read(0x5325e4,32)))+list(struct.unpack('<8I',mu.mem_read(0x532604,32)))
        values += [self.get(0x55a1d8+4*index) for index in range(30)]+[(self.get(0x55a280)-TYPES)//500,int(self.get(0x55a2d4)!=0)]
        raws=[]
        # 합성 VT 주소도 호스트 토큰1로 정규화하며 나머지46바이트는 그대로 비교한다.
        for sid in (50,51,77):raw=bytearray(mu.mem_read(POOL+50*sid,50));struct.pack_into('<I',raw,0,1);raws.append(bytes(raw).hex())
        return (';'.join(self.events) or '-',','.join(map(str,values)),self.string(0x55a1bc),*raws)


def main():
    """고유 입력을 두 실제 x87 정밀도에서 관찰하거나 이미 저장한 근거만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 원본 기대값은 재계산하지 않는다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==facts['uniqueInputs'] and len({row[0] for row in rows})==len(rows);print('커서 수명 SHA/고유 입력 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[]
    # 계산 결과를 Python 구현으로 만들지 않고 실제 두 원문 실행끼리 비교한다.
    for case in cases():
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1];rows.append((','.join(map(str,case)),*outputs[0]))
    FIXTURE.write_text('# input17 events globals78 costText raw50 raw51 raw77\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_cursor_lifecycle.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=['00447000','00446e70','00445aa0'],uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),precisionDifferentInputs=[],sha256={path:digest(ROOT/path) for path in paths},limitations=['세 수명 몸체와 실제 CRT 절삭을 실행한다. Take의 실제 두 번째 cdecl 인자를 확인한다.','가상 효과/위치 준비·판정/임시 생성/마나·범위/번역·측정/sprintf는 명시 경계다.','비정상 TYPE/SHP 진단·30칸 초과·차단 teleporter는 정상 입력 수에 합산하지 않는다. 실제 GUI/건설 배치 완료를 뜻하지 않는다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('커서 수명:',len(rows),'고유 입력, 두 정밀도 차이0')


if __name__=='__main__':main()
