#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제41db70/41d400의 Graph 순회/재귀/사각형 변경을 격리 x86으로 관측한다."""
import argparse,itertools,json,struct
from decomp_mana_bolt_arrival import Oracle as BaseOracle,ROOT,VTABLE,STOP,STACK,bits,sha
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

RECT,OUT=VTABLE+0x100,VTABLE+0x200 # 변경되는 사각형/반환 점만 합성 영역에 놓는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/board-approach-x86.tsv' # 과거 Builder fixture는 변경하지 않는다.
REPORT=ROOT/'cpppj/board-approach.json' # 현재 PC/함수/원본/제품/관측 SHA다.

class Oracle(BaseOracle):
    """GraphAt 응답만 외부 입력이며 실제 계산·분기·재귀·CRT는 보호 PE다."""
    def write(self,mu,access,address,size,value,data):
        """실제 함수가 바꾸는 사각형과 출력 점을 로컬 스택에 추가 허용한다."""
        if RECT<=address and address+size<=RECT+16:return
        if OUT<=address and address+size<=OUT+8:return
        return BaseOracle.write(self,mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """현재 Graph 입력의 전체 조회 순서와 선택된 debug assertion만 기록한다."""
        if address==0x462b80:
            self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);x,y=struct.unpack('<ff',mu.mem_read(sp+4,8));xb,yb=struct.unpack('<II',mu.mem_read(sp+4,8));mode=self.mode
            if mode==0:value=1
            elif mode==1:value=1 if (xb,yb)==self.destination else 2
            elif mode==2:value=1 if (int(x)+int(y))%3==0 else 2
            elif mode==3:value=1 if not self.count or self.count%5==0 else 2
            elif mode==4:value=254
            else:value=1 if (xb,yb)==self.destination or (int(x)==int(self.rectangle[0]) and int(y)==int(self.rectangle[3])) else 2
            self.events.append(f'G:{xb}:{yb}:{value}');self.count+=1;mu.reg_write(UC_X86_REG_EAX,value);self.return_stub();return
        if address==0x4e0620:
            self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);line=int.from_bytes(mu.mem_read(sp+12,4),'little');self.events.append('A:'+str(line));self.return_stub();return
        return BaseOracle.instruction(self,mu,address,size,data)

    def run(self,case,control):
        """입력 사각형/점/flags/현재 Graph만 놓고 결과는 계산하지 않는다."""
        rectangle,x,y,flags,self.mode,checking=case;self.rectangle=rectangle;self.destination=(bits(x),bits(y));self.events=[];self.count=0;mu=self.mu
        mu.mem_write(RECT,struct.pack('<4f',*rectangle));self.put(0x5e4794,checking);mu.reg_write(UC_X86_REG_ECX,RECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.mem_write(STACK,struct.pack('<5I',STOP,OUT,bits(x),bits(y),flags));mu.emu_start(0x41db70,STOP,count=4000000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+20 and mu.reg_read(UC_X86_REG_EAX)==OUT
        assert not(mu.reg_read(UC_X86_REG_FPSW)&0x3800) and mu.reg_read(UC_X86_REG_FPCW)==control
        return ','.join(str(v) for v in struct.unpack('<4I',mu.mem_read(RECT,16))),','.join(str(v) for v in struct.unpack('<2I',mu.mem_read(OUT,8))),';'.join(self.events)

def cases():
    """회전 방향/확장·끝점 flags/세계 모서리/재귀/Graph254의 입력을 교차한다."""
    result=[];rectangles=((20,20,24,25),(20.25,20.75,25.5,26.25),(0,0,3,3),(253,253,258,258),(5,5,5,7),(10,10,18,18))
    # 주된 네 방향은 모든 Graph 정책에서 비교한다.
    for r,point,flags,mode in itertools.product(rectangles,((30,10),(40,25),(20,40),(0,25)),(1,2,4,7),(0,1,2,3,4,5)):result.append((r,*point,flags,mode,1))
    # 대각선 동률과 중심점/동일점은 확장 없이 추가한다.
    for point,flags,mode in itertools.product(((0,0),(40,0),(0,40),(40,40),(22,22.5)),(0,3,5,6),range(6)):result.append(((20,20,24,25),*point,flags,mode,0))
    return result

def main():
    """두 정밀도의 실제 메모리/순서를 비교하고 별도 fixture를 저장한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        d=json.loads(REPORT.read_text(encoding='utf-8'))
        for key in ('sha256','currentSources','protectedFileSha256'):
            for path,wanted in d.get(key,{}).items():assert sha(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==d['cases']==len(cases())==696 and d['normalReturns']==1392 and not d['precisionDifferentInputs']
        print('Board 접근점 SHA 확인');return
    oracle=Oracle();rows=[];differences=[]
    # 원본 결과는 재계산하지 않으며 실제 호출마다 제어 워드를 다시 설정한다.
    for index,case in enumerate(cases()):
        a=oracle.run(case,0x027f);b=oracle.run(case,0x037f)
        if a!=b:differences.append(index)
        r,x,y,flags,mode,checking=case;values=','.join(map(str,(*map(bits,r),bits(x),bits(y),flags,mode,checking)));rows.append((values,*a))
    FIXTURE.write_text('# input9 rectangle4 point2 effects\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=['tools/decomp_board_approach.py','tools/decomp_mana_bolt_arrival.py','tools/decomp_constructionarrival_oracle.py','originals/Netstorm.exe','extracted/refined/originals/functions.tsv','cpppj/src/o/RawBoardApproach.h','cpppj/src/o/RawBoardApproach.cpp',FIXTURE.relative_to(ROOT).as_posix()]
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon / 2026-10-16',cases=len(rows),normalReturns=len(rows)*2,instructions=oracle.instructions,precisionDifferentInputs=differences,sha256={p:sha(ROOT/p) for p in paths}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Board 접근점',len(rows),'입력',len(rows)*2,'반환',oracle.instructions,'명령 차이',len(differences))

if __name__=='__main__':main()
