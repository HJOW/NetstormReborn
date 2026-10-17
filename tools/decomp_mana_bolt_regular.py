#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""마나 볼트105의 실제 Regular/투영/역투영/프레임 구간을 CPU에서 격리 실행한다."""
import argparse,itertools,json,struct
from decomp_mana_bolt_arrival import Oracle as ArrivalOracle,ROOT,TYPES,POOL,VTABLE,MAP,STOP,STACK,DESTROY,SOUND,bits,sha
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

FRAME,UNPOP,POP=0x4acee0,STOP+0x310,STOP+0x320 # 외부 Frame/가상 공간 효과 경계다.
CALLER,RESULT=STOP+0x500,STOP+0x580 # 실제 float10을 caller의 fstpfloat32 경계에서 관측한다.
SHP,CODES=VTABLE+0x400,VTABLE+0x800 # 원본 TYPE가 읽는 합성 SHP/프레임 코드 입력이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/mana-bolt-regular-x86.tsv' # 과거 도착/표시 자료와 분리한 새 관측이다.
REPORT=ROOT/'cpppj/mana-bolt-regular.json' # 실제 실행/두 정밀도/현재 코드/보호 원본 SHA다.

class Oracle(ArrivalOracle):
    """실제 Regular·Finder·TYPE·497220·4971e0·497490과 현재 raw를 실행한다."""
    def __init__(self):
        """같은 보호 PE/함수 범위 로더만 재사용한다."""
        super().__init__();self.projection_abi=[];self.mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',RESULT)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)))

    def write(self,mu,access,address,size,value,data):
        """실제 카메라 설정과 caller 결과만 로컬 스택 쓰기에 추가 허용한다."""
        if RESULT<=address and address+size<=RESULT+4:return
        if 0x59a900<=address and address+size<=0x59a940:return
        return ArrivalOracle.write(self,mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """프레임/소리/가상 공간만 대체하며 좌표 함수의 ABI와 계산은 실제 원본으로 관측한다."""
        if CALLER<=address<CALLER+11:self.instructions+=1;return
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address in (FRAME,UNPOP,POP,DESTROY,SOUND):
            self.instructions+=1;self.calls[hex(address)]+=1
            if address==FRAME:
                frame,flags=struct.unpack('<II',mu.mem_read(sp+4,8));self.events.append(f'F:{frame}:{flags}');self.put(self.slot(50)+36,frame);self.return_stub(8)
            elif address==UNPOP:self.events.append('U:'+str(int.from_bytes(mu.mem_read(sp+4,4),'little')));self.return_stub(4)
            elif address==POP:
                x,y,flags=struct.unpack('<III',mu.mem_read(sp+4,12));self.events.append(f'P:{x}:{y}:{flags}');self.put(self.slot(50)+14,x);self.put(self.slot(50)+18,y);self.return_stub(12)
            elif address==DESTROY:self.events.append('K:'+str(int.from_bytes(mu.mem_read(sp+4,4),'little')));self.return_stub(4)
            else:
                x,y,name,loop=struct.unpack('<IIII',mu.mem_read(sp+4,16));value=bytes(mu.mem_read(name,18)).split(b'\0')[0];assert value==b'influenceIcon.wav' and not loop;self.events.append(f'S:{x}:{y}');self.return_stub()
            return
        if address==0x4971e0 and len(self.projection_abi)<4:
            self.projection_abi.append(dict(ecx=mu.reg_read(UC_X86_REG_ECX),ebx=mu.reg_read(UC_X86_REG_EBX),stack=list(struct.unpack('<4I',mu.mem_read(sp+4,16)))))
        return ArrivalOracle.instruction(self,mu,address,size,data)

    def invoke(self,entry,args,control,this=0,result_float=False):
        """ret N/float10/x87 제어를 실제 caller 경계로 검사한다."""
        mu=self.mu;mu.reg_write(UC_X86_REG_ECX,this);mu.reg_write(UC_X86_REG_EBX,0xa5a5a5a5);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),CALLER if result_float else STOP,*(value&0xffffffff for value in args)));mu.emu_start(entry,STOP,count=1000000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP;assert mu.reg_read(UC_X86_REG_ESP)==STACK+4+(len(args)*4 if this else 0)
        assert not(mu.reg_read(UC_X86_REG_FPSW)&0x3800) and mu.reg_read(UC_X86_REG_FPCW)==control

    def run(self,case,control):
        """독립 입력만 배치하고 기대 좌표/프레임/효과는 계산하지 않는다."""
        event,count,frame,word,present,extra,owner,profile=case;mu=self.mu;s=self.s;self.events=[]
        mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(POOL,bytes(0x200000));mu.mem_write(MAP,bytes(self.map_end-MAP))
        self.put(s['types'],TYPES);self.put(s['pool'],POOL);self.put(s['capacity'],32768);self.put(s['board'],256);mu.mem_write(s['grid'],struct.pack('<7I',0,1,256,*self.bases))
        self.put(VTABLE+0x10,DESTROY);self.put(VTABLE+0x48,UNPOP);self.put(VTABLE+0x90,POP)
        self.put(self.slot(50),VTABLE);self.put(self.slot(50)+10,105,1);self.put(self.slot(50)+12,word,2);self.put(self.slot(50)+14,bits(20.75));self.put(self.slot(50)+18,bits(21.9));self.put(self.slot(50)+34,1,1);self.put(self.slot(50)+36,frame)
        if present:
            self.put(self.slot(60)+10,100,1);self.put(self.slot(60)+40,extra,1);self.put(self.slot(60)+34,owner,1);self.put(self.slot(60)+14,bits(20.5));self.put(self.slot(60)+18,bits(21.5));self.put(TYPES+100*500+0x1d4,1);self.put(TYPES+100*500+0x1d8,1);self.put(self.bases[1]+((21//2)*128+20//2)*2,60,2)
        t=TYPES+105*500;self.put(t,100);self.put(t+0xdc,SHP);self.put(t+0x114,4);self.put(t+0x124,CODES);mu.mem_write(CODES,b'AP\x01\0AP\x02\0BP\x01\0BP\x02\0');self.put(t+0x130,0);self.put(t+0x134,2);self.put(t+0x170,2);self.put(t+0x174,2)
        self.put(SHP+8,0x124);width,hotspot=((32,5),(1,-7),(32767,32767),(-16,-8))[profile];mu.mem_write(SHP+0x100+0x18,struct.pack('<hhhh',width,10,hotspot,0))
        origin=((0,0),(319,127),(1920,1080),(2147483600,-2147483600))[profile]
        self.invoke(0x497490,list(origin),control,0x59a900);self.events=[]
        self.invoke(0x45a8e0,[event,count,bits(0.1)],control,self.slot(50),True)
        raw=self.slot(50);values=struct.unpack('<I',mu.mem_read(RESULT,4))[0],int.from_bytes(mu.mem_read(raw+36,4),'little'),int.from_bytes(mu.mem_read(raw+14,4),'little'),int.from_bytes(mu.mem_read(raw+18,4),'little')
        return (*values,';'.join(self.events)),tuple(int.from_bytes(mu.mem_read(0x59a900+i*4,4),'little') for i in range(2))

def cases():
    """기대값을 만들지 않고 현재 frame/event/count/WORD/대상/카메라 입력을 교차한다."""
    result=[]
    # signed count의 음수/감김과 현재 글자 A/B의 구간을 구별한다.
    for event,count,frame in itertools.product((0,1,2,0xffffffff),(0,1,2,7,0xffffffff,0x80000000),(0,1,2,3)):result.append((event,count,frame,0,1,1,1,0))
    # 대상 부재/preview/소유권과 WORD·signed SHP 헤더·현재 카메라를 독립 입력으로 방문한다.
    for word,target,profile in itertools.product((0,1,2,0x8000,0xffff),((0,1,1),(1,0,1),(1,9,1),(1,1,0),(1,1,1)),range(4)):
        present,extra,owner=target;result.append((1,3,2,word,present,extra,owner,profile))
    return list(dict.fromkeys(result))

def main():
    """두 정밀도 관측을 별도로 저장하고 현재 직접 의존 SHA를 검사한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        d=json.loads(REPORT.read_text(encoding='utf-8'))
        for key in ('sha256','currentSources','protectedFileSha256'):
            for path,wanted in d.get(key,{}).items():assert sha(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==d['cases']==len(cases())==196 and d['normalReturns']==392 and not d['precisionDifferentInputs']
        print('마나 볼트 주기 SHA 확인');return
    oracle=Oracle();rows=[];differences=[]
    # float10은 실제 caller fstpfloat32를 거친 관측만 C++와 비교한다.
    for index,case in enumerate(cases()):
        outputs=[oracle.run(case,control) for control in (0x027f,0x037f)]
        if outputs[0]!=outputs[1]:differences.append(index)
        rows.append((','.join(map(str,case)),*outputs[0][0],','.join(map(str,outputs[0][1]))))
    FIXTURE.write_text('# input8 return frame x y events camera2\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=['tools/decomp_mana_bolt_regular.py','tools/decomp_mana_bolt_arrival.py','tools/decomp_constructionarrival_oracle.py','extracted/refined/originals/functions.tsv','originals/Netstorm.exe','cpppj/src/o/RawManaBoltRegular.cpp','cpppj/src/o/RawManaBoltRegular.h',FIXTURE.relative_to(ROOT).as_posix()]
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon / 2026-10-16',cases=len(rows),normalReturns=len(rows)*2,instructions=oracle.instructions,precisionDifferentInputs=differences,externalCalls=dict(oracle.calls),inverseAbi=oracle.projection_abi,sha256={p:sha(ROOT/p) for p in paths}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('마나 볼트 주기',len(rows),'입력',len(rows)*2,'반환',oracle.instructions,'명령 정밀도 차이',len(differences))

if __name__=='__main__':main()
