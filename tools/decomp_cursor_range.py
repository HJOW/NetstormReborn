#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 사정거리 생성·직선/원형 프레임·통지·소멸의 전체 필드와 가상 효과를 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_cursor_mana import Oracle as ManaOracle, ROOT, STACK, STOP, POOL, TYPES, PROCESS, VTABLE, POINT, DETACH, UNPOP, DESTROY, POSITION, KILL_RETURN, bits, time_words, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

ENTRIES=(0x474900,0x474520,0x474280,0x474450,0x474790,0x474870,0x474b80,0x474c30,0x474ea0,0x474ed0,0x469120,0x46aa50,0x468e70,0x468dc0,0x4d4ce0,0x4d4c60,0x4ac200,0x4ac550,0x49ace0,0x41d180,0x4e5e8a,0x4eded0,0x4ee1b7,0x4edf9a)  # 기존의 순수 보조 함수도 실제로 실행한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/cursor-range-x86.tsv'  # 마나와 별개인 새 원문이다.
REPORT=ROOT/'cpppj/cursor-range.json'  # 정상 반환·정밀도 차이·실행 범위와 SHA를 기록한다.


def cases():
    """결과를 계산하지 않고 타입/상태/시각과 외부 효과 입력을 조합한다."""
    base=[0,0,0,0,100,bits(20),2,bits(4),4,0,0,1,0,0,0,15,1,8,256,0,1,3,*time_words(5.25),*time_words(5),*time_words(0),bits(100),bits(110),bits(99),bits(108),bits(101),bits(111),bits(20),bits(1),1,8,0,bits(15),bits(-1),bits(-1),bits(150),2,4,3,0xfffffffb,bits(16),bits(11),0xfffffffd,5,3,4,bits(30),101,*time_words(5),bits(25)]  # 명시 입력58DWORD다.
    result=[]
    # 생성은 group·테마·포함물·A/B·사거리·현재 방향을 교차한다.
    for group,theme,flags,distance,mode in itertools.product((0,1,3,6),(0,1,2),(0,0x100,0x10000),(10,20),(0,2)):
        row=base.copy();row[1:7]=group,theme,flags,140 if group==0 and theme==2 else 100,bits(distance),mode;result.append(row)
    # 원형 frame의 현재 개수·반경·시간 차·종료/숨김이다.
    for count,radius,delta,stopping,hidden in itertools.product((0,1,8),(0,4,20),(0,.125,2),(0,1),(0,1)):
        row=base.copy();row[0]=1;row[37]=count;row[7]=bits(radius);row[9:11]=stopping,hidden;row[24:26]=time_words(5.25-delta);result.append(row)
    # Wind 관측각의 양 끝·회전·범위 밖 위치와 현재 시각 재조회다.
    for arc,now,mutation in itertools.product(((330,30),(150,210),(0,60)),(0.0,5.25,10.5),(0,1,5)):
        row=base.copy();row[0]=1;row[7]=bits(20);row[40:42]=map(bits,arc);row[22:24]=time_words(now);row[19]=mutation;result.append(row)
    # 직선 표시의 실제 개수·허용 방향·숨김·이동 위상·즉시 종료다.
    for count,allowed,hidden,delta,stopping in itertools.product((1,4,10),(0,5,15),(0,1),(.01,.25,1),(0,1)):
        row=base.copy();row[0]=1;row[1]=1;row[36]=0;row[8]=count;row[15]=allowed;row[10]=hidden;row[9]=stopping;row[24:26]=time_words(5.25-delta);result.append(row)
    # 원형의 dead 진단은 표시를 건너뛰고 다른 마커로 계속한다.
    for flags in (2,3):
        row=base.copy();row[0]=1;row[13]=flags;result.append(row)
    # event1은100칸 고정이며 SID0을 건너뛴다.
    for event,info,zeros,flags in itertools.product((0,1,3,9),(0,1,0xffffffff),(0,1,2),(0,4,3)):
        row=base.copy();row[0]=2;row[11:15]=event,info,flags,zeros;result.append(row)
    # 소멸은 free만 제외하고 dead에는 가상 삭제를 호출한다.
    for zeros,flags,token,mutation in itertools.product((0,1,2),(0,1,2,3),(0,1,2),(0,3)):
        row=base.copy();row[0]=3;row[14]=zeros;row[13]=flags;row[16]=token;row[19]=mutation;result.append(row)
    # new 실패와 실제 생성 뒤 프레임/종료/자기 삭제의 연속 호출이다.
    for operation,group,mutation,success in itertools.product((4,5),(0,1),(0,2,6),(0,1)):
        row=base.copy();row[0]=operation;row[1]=group;row[19:21]=mutation,success;result.append(row)
    return list(dict.fromkeys(tuple(value&0xffffffff for value in row) for row in result))


class Oracle(ManaOracle):
    """기존 격리 CPU/ABI 검사에 실제 사정거리 함수를 추가하고 계산은 대체하지 않는다."""
    def __init__(self):
        """보호 PE와 현재 정밀 범위를 읽고 필드512바이트만 허용한다."""
        self.range_ready=False;super().__init__();self.range_ready=True
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 범위 사이의 미확인 명령은 계속 거부한다.
        for entry in ENTRIES:
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        # 기존 Crossbow 관측에서도 확인한 CRT fmod 본문과 단순 꼬리 호출의 정확한 범위다.
        self.allowed.extend(((0x4e5e94,0x4e5eb2),(0x4ee254,0x4ee25b)))
        self.put(VTABLE+0xb8,0x4d4ce0)

    def mutate(self,kind):
        """처음 선택한 외부 효과만 현재 개수/시각을 바꾸는 명시 입력이다."""
        if self.mutated:return
        mode=self.case[19]
        if (mode,kind) not in ((1,'D'),(2,'V'),(3,'X'),(4,'D'),(5,'O'),(6,'A')):return
        self.mutated=True
        if mode in (1,2,3,4,5):self.put(PROCESS+0x1a4,1);self.put(PROCESS+0x1a8,1)
        if mode==4:self.put(PROCESS+0x1e8,0)
        now=struct.unpack('<d',self.mu.mem_read(0x55b4d0,8))[0];self.mu.mem_write(0x55b4d0,struct.pack('<d',now+{1:.125,2:.25,5:.5,6:1.25}.get(mode,0)))

    def instruction(self,mu,address,size,data):
        """할당/Host/가상 효과와 현재 자료만 경계로 두고 몸체와 순수 보조는 실행한다."""
        if not self.range_ready:return super().instruction(mu,address,size,data)
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        esp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<9I',mu.mem_read(esp+4,36));ecx=mu.reg_read(UC_X86_REG_ECX);sid=(ecx-POOL)//50
        if address==0x4e4391:
            self.instructions+=1;self.event('N',args[0]);assert args[0]==0x200;self.ret(PROCESS if self.case[20] else 0);return
        if address==0x4af530:
            self.instructions+=1;assert args[:2]==(104,2);result=50+self.created;self.created+=1;self.event('C',*args[:2],result);self.ret(POOL+result*50);return
        if address==0x41bd20:
            self.instructions+=1;assert ecx==PROCESS;self.event('A',*args[:4]);self.put(PROCESS+4,args[2] or 300);self.put(PROCESS+8,args[1]);self.mutate('A');self.ret(PROCESS,16);return
        if address==0x4d4d00:self.instructions+=1;self.event('M',sid);self.ret(self.case[6]);return
        if address==0x4ade70:
            self.instructions+=1;self.event('P',sid);mu.mem_write(args[0],bytes(16));self.ret(args[0],8);return
        if address==0x419850:
            self.instructions+=1;self.event('H',(ecx-TYPES)//500,args[0],args[1]);self.put(POINT+0x120,self.case[49],2);self.put(POINT+0x122,self.case[50],2);self.ret(POINT+0x100,8);return
        if address==0x4ac9f0:
            self.instructions+=1;self.event('T',sid,*args[:3]);assert args[:3]==(1,0,0);self.ret(251,12);return
        if address==0x4d4ce0:self.event('F',sid,args[0])
        if address==0x469120:self.event('B',*args[:9])
        if address==0x41bf10:
            self.instructions+=1;assert ecx==PROCESS;self.event('K',args[0]);self.killed=True;self.kill_return=self.get(esp);self.kill_esp=esp;self.put(esp-4,KILL_RETURN);mu.reg_write(UC_X86_REG_ESP,esp-4);mu.reg_write(UC_X86_REG_EIP,0x474870);return
        if address==0x4c2630:
            self.instructions+=1;assert bytes(mu.mem_read(args[1],18))==b'Dead circle range!';self.event('Q',args[0]);self.ret();return
        return super().instruction(mu,address,size,data)

    def write(self,mu,access,address,size,value,data):
        """현재 필드512바이트·raw·알려진 전역/스택 밖 쓰기를 거부한다."""
        if not any(low<=address and address+size<=high for low,high in ((0,4),(POOL,POOL+0x10000),(PROCESS,PROCESS+0x200),(STACK-0x7f000,STACK+0x1000),(0x55a274,0x55a278),(0x592990,0x592f60))):raise RuntimeError(f'사정거리 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """SEH 외NULL 읽기와 허용하지 않은 영역 읽기를 거부한다."""
        if not any(low<=address and address+size<=high for low,high in ((0,4),(0x400000,self.image_end),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(PROCESS,PROCESS+0x200),(VTABLE,VTABLE+0x1000),(POINT,POINT+0x1000),(STACK-0x7f000,STACK+0x1000))):raise RuntimeError(f'사정거리 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,case,precision):
        """매번 새 입력을 준비해 전체 필드/raw102개/전체 효과/연속 호출을 읽는다."""
        assert len(case)==58;self.case=case;self.events=[];self.created=0;self.mutated=False;self.killed=False;mu=self.mu
        mu.mem_write(PROCESS,b'\xa5'*0x200);self.put(0,0);self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x55a274,0 if not case[16] else PROCESS if case[16]==1 else PROCESS+4)
        for address,value in ((0x531928,case[18]),(0x52f838,case[35]),(0x59a92c,case[47]),(0x59a930,case[48]),(0x5409c0,case[53])):self.put(address,value)
        mu.mem_write(0x55b4d0,struct.pack('<2I',*case[22:24]))
        # 타입 입력은 원본 TYPE 바이트에 주고 조회/계산은 실제 코드가 수행한다.
        for number in (100,101,128,140):
            for offset,value in ((0x9c,case[1]),(0x98,case[2]),(0xec,case[3]),(0x44,case[57] if number==case[54] else case[5]),(0x1d4,case[51]),(0x1d8,case[52]),(0x1dc,case[45]),(0x1e0,case[46])):self.put(TYPES+number*500+offset,value)
            for index in range(4):self.put(TYPES+number*500+0x15c+4*index,index)
        # A/B Run은 입력이며 애니메이션 프레임 기대값이 아니다.
        for variant in range(2):self.put(TYPES+104*500+0x130+4*variant,case[43]+variant*10);self.put(TYPES+104*500+0x170+4*variant,case[44]+variant)
        for sid in (*range(50,150),250,251):
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=case[4] if sid==250 else 6 if sid==251 else 104;raw[11]=0 if sid>=250 else case[13];raw[34]=1;raw[40]=case[17] if sid==250 else 0;struct.pack_into('<H',raw,6,300 if sid==250 else 0);struct.pack_into('<2I',raw,14,*(case[28:30] if sid>=250 else case[30:32]));struct.pack_into('<I',raw,36,2);mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(POOL+251*50+18,case[54],2)
        self.put(PROCESS,0x50c498);self.put(PROCESS+4,300);self.put(PROCESS+8,250);self.put(PROCESS+12,case[36]);self.put(PROCESS+16,int(case[1]==6))
        # 독립 상태 입력의 전체100칸과 실제 읽는 필드를 같은 폭으로 준비한다.
        for index in range(100):self.put(PROCESS+20+4*index,0 if case[14]==2 or case[14]==1 and not index else 50+index)
        for offset,value in ((0x1a4,case[37]),(0x1a8,case[8]),(0x1ac,case[37]),(0x1b0,case[34]),(0x1b4,case[7]),(0x1b8,case[9]),(0x1bc,bits(5)),(0x1c0,case[39]),(0x1c4,case[40]),(0x1c8,case[41]),(0x1cc,case[42]),(0x1e8,case[10]),(0x1ec,case[38])):self.put(PROCESS+offset,value)
        mu.mem_write(PROCESS+0x1d0,struct.pack('<2I',*case[24:26]));mu.mem_write(PROCESS+0x1d8,struct.pack('<2I',*case[55:57]));mu.mem_write(PROCESS+0x1e0,struct.pack('<2I',*case[26:28]))
        for side in range(4):self.put(PROCESS+0x1f0+4*side,int(bool(case[15]&(1<<side))))
        snapshots=[];returned=0
        def execute(entry,arguments=b'',purge=0,this=PROCESS):
            """실제 ABI/정상 반환/x87를 검사한 뒤512바이트 전체를 보존한다."""
            self.call(entry,this,arguments,purge,precision);self.returns+=1;snapshots.append(bytes(mu.mem_read(PROCESS,0x200)).hex())
        if case[0] in (0,5):
            execute(0x474520,struct.pack('<II',250,0),8)
            if case[0]==5:
                for _ in range(case[21]):
                    now=struct.unpack('<d',mu.mem_read(0x55b4d0,8))[0];mu.mem_write(0x55b4d0,struct.pack('<d',now+.125));execute(0x474b80)
                execute(0x474790,struct.pack('<III',3,0,0),12)
                for _ in range(3):
                    if self.killed:break
                    now=struct.unpack('<d',mu.mem_read(0x55b4d0,8))[0];mu.mem_write(0x55b4d0,struct.pack('<d',now+.25));execute(0x474b80)
                if not self.killed:execute(0x474870)
        elif case[0]==1:execute(0x474b80)
        elif case[0]==2:execute(0x474790,struct.pack('<III',case[11],case[12],0x87654321),12)
        elif case[0]==3:execute(0x474870)
        elif case[0]==4:execute(0x474900,struct.pack('<II',250,0),0,0);returned=int(mu.reg_read(UC_X86_REG_EAX)==PROCESS)
        raw=[]
        # 가상 표의 합성 주소만1로 정규화하고 나머지46바이트를 그대로 보존한다.
        for sid in (*range(50,150),250,251):
            data=bytearray(mu.mem_read(POOL+sid*50,50));struct.pack_into('<I',data,0,1);raw.append(data.hex())
        token=self.get(0x55a274);globals_=(0 if not token else 1 if token==PROCESS else 2,*struct.unpack('<2I',mu.mem_read(0x55b4d0,8)),returned)
        return (';'.join(self.events) or '-',bytes(mu.mem_read(PROCESS,0x200)).hex(),','.join(map(str,globals_)),''.join(raw),'|'.join(snapshots))


def main():
    """전체 입력의 두 정밀도 결과를 저장하고 동일한 둘째 출력만 생략 기호로 표시한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs'];print('사정거리 SHA 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 기대값은 실제 두 CPU 실행으로만 만들고 차이가 있는 둘째 출력도 모두 보존한다.
    for case in cases():
        try:results=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))]
        except Exception:
            print("중단 입력",len(rows),case,flush=True);raise
        same=results[0]==results[1]
        if not same:different.append(len(rows))
        rows.append((','.join(map(str,case)),*results[0],*(('-',)*5 if same else results[1])))
    FIXTURE.write_text('# input58 events53 fields53 globals53 raw102slots53 steps53 events64 fields64 globals64 raw64 steps64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_cursor_mana.py','tools/decomp_cursor_range.py',FIXTURE.relative_to(ROOT).as_posix())  # 현재 관측 의존이다.
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=[f'{entry:08x}' for entry in ENTRIES],uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),instructions=sum(oracle.instructions for oracle in oracles),actualEntries=dict(sum((oracle.calls for oracle in oracles),Counter())),precisionDifferentInputs=different,sha256={path:digest(ROOT/path) for path in paths},limitations=['생성·직선/원형 frame·통지·소멸·방향/각도/픽셀/사거리/CRT 절삭과 나머지는 실제 몸체다.','할당/Host·현재 공격 모드·중심·사용하지 않는 footprint 결과·SHP 헤더·내용물/객체 가상 효과·진단은 명시 경계다.','기본53비트가 제품 대상이며64비트 차이는 덮어쓰지 않는다. 실제 GUI/사용자 플레이 증거는 아니다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('사정거리:',len(rows),'고유 입력, 정상 반환',facts['normalReturns'],'정밀도 차이',len(different))


if __name__=='__main__':main()
