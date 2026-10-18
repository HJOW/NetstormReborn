#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""생산 idle의 다리 공급·품질/재고·비용 전환을 원본10.78에서 독립 관측한다."""
import argparse,collections,csv,json,socket,struct
from pathlib import Path
from decomp_factory_menu import Oracle as BaseOracle,ROOT,POOL,TEXT,digest,fbits
from decomp_priestmenu import PLAYERS,STACK,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

ENTRIES=(0x43ef10,0x40eb80,0x4b2270,0x4b1b20,0x44daa0,0x4af0b0,0x4af0e0,0x418070,0x4180a0,0x4257c0,0x40f100,0x40ee90,0x4e49c0,0x4e81da,0x4f2a49,0x4f2a7b,0x4f2929,0x4f29bb) # 실제 공급/시간/탐색/패턴과 CRT 기본 정밀도 몸체다.
NODE,GRIDS,TIMES=TEXT+0x1000,TEXT+0x2000,TEXT+0x4000 # 패널/두 격자/188개double의 분리된 입력 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/production-idle-x86.tsv' # 새 C++ 전에 동결할 원본 출력이다.
REPORT=ROOT/'cpppj/production-idle.json' # 정상 반환과 입력/출처를 기록한다.

# double을 십진 문자열의 반올림 없이 명시 입력으로 저장한다.
def dbits(value):return struct.unpack('<Q',struct.pack('<d',value))[0]

# 각 열은 결과와 무관한 초기 전역/노드/내용물 입력이다.
def inputs():
    base=[1,0,5,9,9,4,0,1,dbits(0),dbits(0),dbits(0),1,1,0,0,0,0,0,fbits(500),0,0x40,0,1,3,3,2,dbits(0)]
    axes={0:(0,2,3),1:(1,),2:(0,),3:(0,),4:(0,),5:(0,1,2),6:(1,),7:(0,),8:tuple(dbits(v) for v in (0.1,14.999,15,30,1000000.125)),9:tuple(dbits(v) for v in (-0.25,0.099,5.999,6,6.1,100.25)),10:(dbits(1),dbits(31)),11:(0,),12:(0,-1,2,255),13:(1,2),14:(1,),15:(3,),16:(1,4,5,0xffffffff),17:(199,200,400,500,16777217),18:tuple(fbits(v) for v in (0,199.5,200,399.99,400,500.5,float('nan'))),19:(1,2,3,255),20:(0,8,0x140,0x48),21:(-1,1,60),22:(0,),23:(0,1,2),24:(0,1,255),25:(0,1,255),26:(dbits(0.001),dbits(30))}
    rows=[base.copy()]
    # 실제 경계의 한 축씩 바꾸어 원본 분기를 식별한다.
    for column,values in axes.items():
        for value in values:
            row=base.copy();row[column]=value;rows.append(row)
    # 신전 소실/AI/편집/품질 선택과 재고 상한을 교차한다.
    for editor in (0,1):
        for quality in (0,1,2):
            for selected in (0,0x100):
                row=base.copy();row[1]=editor;row[9]=dbits(6);row[19]=quality;row[20]|=selected;row[21]=60;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """생산 계산은 원문으로 실행하고 현재 조회/생성·삭제/UI 장치만 명시 경계로 둔다."""
    def __init__(self):
        super().__init__();self.allowed=[];self.hits=collections.Counter();self.boundaries=collections.Counter();self.returns=0
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        # 실제 정밀 함수표의 모든 불연속 범위를 포함한다.
        for entry in ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        if not any(low==0 for low,high,flags in self.mu.mem_regions()):self.mu.mem_map(0,0x1000)

    # 외부 장치의 반환 규약만 복원하며 계산 결과를 입력으로 공급하지 않는다.
    def instruction(self,mu,address,size,data):
        self.recent.append(hex(address))
        if any(low<=address and address+size<=high for low,high in self.allowed):
            self.instructions+=1
            if address in ENTRIES:self.hits[hex(address)]+=1
            return
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,6);ecx=mu.reg_read(UC_X86_REG_ECX);self.boundaries[hex(address)]+=1
        if address in (0x48fc60,0x48fc90):self.ret(self.case[2])
        elif address==0x44f760:self.ret(0)
        elif address==0x460d70:self.mu.mem_write(args[0],struct.pack('<Q',self.case[8]));self.ret(args[0])
        elif address==0x40ec50:self.put(args[1],self.case[18]);self.ret()
        elif address==0x4558c0:self.events.append('R:'+str(args[0]));self.ret(1234)
        elif address==0x4afa70:self.events.append('C:'+str((ecx-POOL)//50)+','+','.join(str(v) for v in args[:5]));self.ret(0,20)
        elif address==0x4c2400:self.ret((self.case[23]>>(args[0]==133))&1)
        elif address==TEXT+0xe000:self.events.append('D:'+str((ecx-POOL)//50));self.ret(0,4)
        elif address==TEXT+0xe010:self.events.append('U:'+str(1 if ecx==GRIDS+0x200 else 2));self.ret()
        elif address==TEXT+0xe020:self.ret(0,16)
        else:raise RuntimeError('생산 idle 미검토 명령 '+hex(address)+' '+str(list(self.recent))+' 입력 '+str(getattr(self,'case',None)))

    # 원본 CRT의 기본 정밀도 설정을 직접 실행해 기준 제어어를 확인한다.
    def default_control(self):
        self.mu.reg_write(UC_X86_REG_FPCW,0x37f);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<I',STOP))
        self.mu.emu_start(0x4e81da,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        return self.mu.reg_read(UC_X86_REG_FPCW)

    # 입력은 새로 공급하고 정상 반환/모든 보존 레지스터와 실제 출력만 읽는다.
    def run(self,case,control):
        self.case=case;self.events=[]
        mode,editor,parent,temple,oldTemple,slots,ai,generator,wall,game,bridgeUntil,stock,limit,rate,fast,forced,serial,oldMoney,cash,quality,flags,deadline,pattern,allowed,q1,q2,until=case
        self.mu.mem_write(NODE,bytes(0xe4));self.mu.mem_write(GRIDS,bytes(0x400));self.mu.mem_write(TIMES,bytes(188*8));self.mu.mem_write(PLAYERS,bytes(9*180));raw=bytearray(64*50)
        struct.pack_into('<H',raw,5*50+6,6);raw[5*50+10]=152;raw[5*50+34]=1
        # 진짜 next/내용물 TYPE6/WORD·BYTE 입력이다. 새 생성/삭제 효과는 경계로 관측만 한다.
        for sid,kind,mask,amount,argument,nxt in ((6,82,flags,quality,pattern,7),(7,128,8,q1,0,8),(8,133,8,q2,0,0)):
            start=sid*50;struct.pack_into('<IHH',raw,start,TEXT+0xf000,nxt,0);raw[start+10]=6;struct.pack_into('<HH',raw,start+18,kind,mask);struct.pack_into('<H',raw,start+12,argument);raw[start+30]=amount
        struct.pack_into('<I',raw,6*50+31,deadline&0xffffffff);self.mu.mem_write(POOL,bytes(raw))
        for offset,value in ((0xb0,GRIDS),(0xb4,GRIDS+0x200),(0xd8,oldMoney),(0xdc,oldTemple),(0xe0,slots)):self.put(NODE+offset,value)
        self.mu.mem_write(NODE+0xc8,struct.pack('<Q',bridgeUntil));self.mu.mem_write(0x55b4d0,struct.pack('<Q',game))
        # 공통 부모는 이미 부착한 상태다. 끝의 실제 Push/Attach 호출은 장치 경계로만 관측한다.
        for grid in (GRIDS,GRIDS+0x200):
            self.put(grid,TEXT+0xf100);self.put(grid+36,parent);self.put(grid+76,fbits(1.25));self.put(grid+80,fbits(2.5))
        for at,value in ((TEXT+0xf000+16,TEXT+0xe000),(TEXT+0xf100+24,TEXT+0xe010),(TEXT+0xf100+32,TEXT+0xe020),(PLAYERS+180+0x7c,temple),(PLAYERS+180+0xb0,ai),(0x59534c,PLAYERS),(0x5c8464,POOL),(0x5c847c,64),(0x541088,6),(0x540bc4,1),(0x5e4794,0),(0x594fb8,int(mode==2)),(0x594fbc,int(mode==1)),(0x594fc8,int(mode==3)),(0x55a4a4,0),(0x568af8,0),(0x54d4e0,0),(0x5c89c4,0),(0x594fd0,1),(0x531890,0),(0x5c85a4,editor),(0x540cac,1),(0x540c70,1),(0x595344,1),(0x557d80,TIMES),(0x541348,188),(0x5318f4,stock),(0x54db74,limit),(0x54db78,fast),(0x54db7c,forced),(0x557dac,serial),(0x5326cc,generator),(0x531b2c,1),(0x5411a0,82),(0x52f7e2,rate)):
            self.put(at,value,1 if at==0x52f7e2 else 4)
        for kind,cost in ((128,400),(133,200)):
            self.put(0x59ab50+kind*4,cost+kind*23);self.mu.mem_write(TIMES+kind*8,struct.pack('<Q',until))
        self.put(0,0xffffffff);saved={UC_X86_REG_EBX:0x11111111,UC_X86_REG_ESI:0x22222222,UC_X86_REG_EDI:0x33333333,UC_X86_REG_EBP:0x44444444}
        # 각 실행에서 ABI/x87/SEH의 명시 초기값을 복원한다.
        for register,value in saved.items():self.mu.reg_write(register,value)
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_EDX,0),(UC_X86_REG_ECX,NODE),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):self.mu.reg_write(register,value)
        self.mu.mem_write(STACK,struct.pack('<I',STOP));beforePlayers=bytes(self.mu.mem_read(PLAYERS,9*180));self.mu.emu_start(0x43ef10,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4 and self.words(0,1)==(0xffffffff,)
        assert all(self.mu.reg_read(register)==value for register,value in saved.items()) and self.mu.reg_read(UC_X86_REG_FPCW)==control and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        assert bytes(self.mu.mem_read(PLAYERS,9*180))==beforePlayers
        self.returns+=1
        return [bytes(self.mu.mem_read(NODE+0xc8,8)).hex()+','+str(self.words(NODE+0xd8,1)[0])+','+str(self.words(NODE+0xdc,1)[0]),bytes(self.mu.mem_read(POOL+6*50,3*50)).hex(),bytes(self.mu.mem_read(TIMES,188*8)).hex(),str(self.words(0x557dac,1)[0]),';'.join(self.events) or '-']


# 계산 출력을 원본에서만 생성하고 두 정밀도의 동일성을 확인한 뒤 저장한다.
def generate():
    oracle=Oracle();rows=[];differences=[];default=oracle.default_control();assert default&0x300==0x200
    for case in inputs():
        first=oracle.run(case,0x27f);second=oracle.run(case,0x37f)
        if first!=second:differences.append(dict(input=case,columns=[i for i,(a,b) in enumerate(zip(first,second)) if a!=b],primary=first,extended=second))
        rows.append([','.join(map(str,case)),*first])
    FIXTURE.write_text('# 명시입력 패널필드 전체내용물150BYTE 전체188타이머 추첨순번 효과순서\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_factory_menu.py',ROOT/'tools/decomp_priestmenu.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE]
    report=dict(host=socket.gethostname(),date='2026-10-18',unit=179,rows=len(rows),normalReturns=oracle.returns,instructions=oracle.instructions,defaultControl=default,defaultPrecisionBits=53,precisionDifferences=len(differences),precisionDetails=differences,controls=[0x27f,0x37f],nativeCalls=dict(oracle.hits),boundaries=dict(oracle.boundaries),sha256={p.relative_to(ROOT).as_posix():digest(p) for p in paths},limits=['43ef10 전체 진입에서 공급·품질·재고·금액 전환의 실제 계산을 실행한다. 현재 기술/SP/허용표 조회·난수 입력·생성/삭제·UI 효과는 명시 경계다. 생성·삭제 경계는 풀을 바꾸지 않으며 같은 실물 사슬의 연결은 별도 게임 검사로 검증한다.','기준은 원본 CRT __setdefaultprecision을 직접 실행해 확인한53비트(027f)다. 보조64비트(037f)와 다른 입력도 제거하지 않고 차이를 별도 보존한다.','초기 perTYPE 시계는 확보된 배열이고 dirty=0/부모 부착/보호 추적 정상 입력이다. 공통 초기화/Deck 동기화/재부착/보호 추적 전체 분기를 새 대조로 확대하지 않는다.10.78만 관측하며 원본 게임 창/새 디컴파일/보호 파일 변경 없음.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('생산 idle 독립 원본',len(rows),oracle.returns,oracle.instructions)


# 동결한 기대값을 재생성하지 않고 출처와 실제 입력·반환 수만 감사한다.
def verify():
    r=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    for name,wanted in r['sha256'].items():assert digest(ROOT/name)==wanted,name
    assert len(rows)==r['rows']==len(inputs()) and r['normalReturns']==len(rows)*2 and r['defaultControl']&0x300==0x200 and r['defaultPrecisionBits']==53 and r['precisionDifferences']==len(r['precisionDetails'])
    for row,case in zip(rows,inputs()):assert len(row)==6 and row[0]==','.join(map(str,case))
    print('생산 idle 동결/SHA/입력 감사',len(rows))


# 명시 CLI 요청만 원본 관측 파일을 생성한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
