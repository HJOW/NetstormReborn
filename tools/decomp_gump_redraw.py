#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 Gump의 두 기본 dirty 요청과 지도 좌표별 clamp를 격리 PE에서 관측한다."""
import argparse,collections,csv,hashlib,itertools,json,struct
from pathlib import Path
from decomp_gumpcollection_oracle import GumpCollectionOracle,node,raw,token,DATA,POOL
from decomp_process_oracle import ROOT,STACK,STOP,fbits,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from decomp_drawsort_oracle import REGISTERS
from unicorn.x86_const import UC_X86_REG_FPTAG


def digest(path):
    """보호 원천/도구/출력의 전체 바이트를 식별한다."""
    with path.open('rb') as stream: return hashlib.file_digest(stream,'sha256').hexdigest()


def integers(values):
    """signed 좌표와 DWORD를 원래32비트 숫자로 직렬화한다."""
    return ','.join(str(value&0xffffffff) for value in values)


class Oracle(GumpCollectionOracle):
    """가시성/위치/절삭/clip은 실제 PE이고 마지막 Renderer 저장소만 명시 경계다."""
    def __init__(self):
        """현재 정밀 내보내기의 기본 요청과 두 사각형 전달 몸체를 실행 범위에 더한다."""
        super().__init__('originals')
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x464a50,0x464ab0,0x499310,0x499370,0x445290):  # 두 가상 기본 요청/원문 사각형 변환을 그대로 실행한다.
            for part in facts[address]['ranges'].split(';'):  # 함수 사이의 명령은 허용하지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.requests=[]  # 실제497580에 전달된 사각형/flags다. 기대 좌표는 계산하지 않는다.

    def on_instruction(self,mu,address,size,data):
        """마지막 dirty 저장소의 인수만 관측하고 정상 thiscall로 반환한다."""
        if address==0x497580:
            esp=mu.reg_read(UC_X86_REG_ESP);pointer,flags=struct.unpack('<2I',mu.mem_read(esp+4,8))
            self.requests.append([*struct.unpack('<4i',mu.mem_read(pointer,16)),flags])
            mu.reg_write(UC_X86_REG_EIP,self.u32(esp));mu.reg_write(UC_X86_REG_ESP,esp+12);return
        super().on_instruction(mu,address,size,data)

    def reset(self,control):
        """호출마다 빈 x87/비영 보존 레지스터와 독립 관측 저장소를 준비한다."""
        self.requests=[];self.entries=collections.Counter();self.projected=[];self.anchors={}
        for register,value in REGISTERS.items(): self.mu.reg_write(register,value)  # 호출자가 보존을 기대하는 실제 값이다.
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_EDX,0),(UC_X86_REG_ESP,STACK),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):
            self.mu.reg_write(register,value)  # 앞 실행의 FPU/임시 레지스터를 다음 입력에 넘기지 않는다.
        self.put(0,0xffffffff);self.writes=[(0,4),(STACK-0xf000,STACK+0x1000)]

    def finish(self,purge,control):
        """정상 반환·스택·보존 레지스터·x87/SEH의 실제 상태를 검사한다."""
        mu=self.mu
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        assert all(mu.reg_read(register)==value for register,value in REGISTERS.items())
        assert not mu.reg_read(UC_X86_REG_FPSW)&0x3800 and mu.reg_read(UC_X86_REG_FPCW)==control and self.u32(0)==0xffffffff

    def observe(self,case,control):
        """완성 노드/인수/지도에서 캐시와 마지막 사각형 요청을 원문만으로 얻는다."""
        self.reset(control);mu=self.mu
        if case['op']=='Clamp':
            pointer=DATA+0xf000;mu.mem_write(pointer-16,b'\xa5'*48);mu.mem_write(pointer,struct.pack('<4i',*case['clip']))
            self.writes.append((pointer,pointer+16));mu.reg_write(UC_X86_REG_ECX,pointer)
            mu.mem_write(STACK,struct.pack('<5I',STOP,*(value&0xffffffff for value in case['map'])))
            mu.emu_start(0x445290,STOP,count=100000);self.finish(16,control)
            assert bytes(mu.mem_read(pointer-16,16))+bytes(mu.mem_read(pointer+16,16))==b'\xa5'*32
            return [integers(struct.unpack('<4i',mu.mem_read(pointer,16))),'-','-']
        nodes=case['nodes'];mu.mem_write(DATA,bytes(0x1000))
        for index,value in enumerate(nodes):  # 실제 원본 노드와 월드 투영 조회의 SID 식별 비트다.
            mu.mem_write(token(index),raw(value));self.put(POOL+index*self.d['stride']+14,index);self.put(POOL+index*self.d['stride']+18,0)
        self.anchors={sid:(x,y,0,0) for sid,x,y in case['anchors']}
        for address,value in ((self.d['root'],token(0)),(self.d['override'],0),(self.d['pool'],POOL),(0x59a8bc,DATA+0x5000)):
            self.put(address,value)  # 원문은 같은 현재 루트/풀/Renderer를 사용한다.
        for index,value in enumerate(case['map']): self.put(0x5ca9cc+index*4,value)  # 지도는 Position 뒤의 실제499310에서 읽는다.
        self.writes += [(token(index)+108,token(index)+116) for index in range(len(nodes))]
        mu.reg_write(UC_X86_REG_ECX,token(case['target']));mu.mem_write(STACK,struct.pack('<2I',STOP,case['argument']))
        mu.emu_start(0x464a50 if case['op']=='Whole' else 0x464ab0,STOP,count=100000);self.finish(4,control)
        caches=[]
        for index,value in enumerate(nodes):  # 캐시 이외의 입력 바이트는 원문이 보존해야 한다.
            actual=bytearray(mu.mem_read(token(index),128));expected=bytearray(raw(value));caches.append(integers((token(index),*struct.unpack_from('<2i',actual,108))));actual[108:116]=expected[108:116]
            assert actual==expected
        return [';'.join(integers(request) for request in self.requests) or '-', ';'.join(caches),','.join(self.projected) or '-']


def plan():
    """가시성/조상/미사용 인수·분수/비양수/감김·월드 부착과 지도 경계의 독립 입력이다."""
    cases=[]
    geometry=((10.75,20.5,12.75,9.5),(-.75,-.25,12.75,9.5),(100.25,200.75,0,9.5),(100.25,200.75,-1,9.5),(100.25,200.75,12.75,9.5),(80.5,-.5,12.25,20.5),(-16.75,-8.75,12.75,9.5),(2147483648.0,-2147483648.0,12.75,9.5),(10,20,float('nan'),9.5),(10,20,12.75,0))  # 실제 float32 저장 전의 명시 입력이다.
    maps=((84,0,1024,768),(-100,-100,400,300),(100,200,80,100))  # 일반/음수/역전 경계다.
    for op,argument,visible,parent_visible,bounds,shape in itertools.product(('Whole','Map'),(0,1,0xffffffff),(0,1,0x10001,9),(0,1),maps,geometry):  # 원문이 읽는 비트와 bool 외 인수도 독립적으로 바꾼다.
        x,y,width,height=shape;nodes=[node(0,flags=parent_visible,x=fbits(.75),y=fbits(-.25)),node(1,flags=visible,parent=0,x=fbits(x),y=fbits(y),width=fbits(width),height=fbits(height))]
        cases.append(dict(op=op,argument=argument,target=1,map=bounds,nodes=nodes,anchors=[],clip=[]))
    for op,target_world,parent_flags in itertools.product(('Whole','Map'),(False,True),(0,1,0x10001,0x2001)):  # 숨은 중간 조상과 현재 월드 부착의 위치 캐시도 확인한다.
        nodes=[node(0),node(1,parent=0,flags=parent_flags,x=fbits(5.75),y=fbits(-4.25)),node(2,parent=1,flags=0x2001 if target_world else 1,x=fbits(-2.5),y=fbits(.75))]
        if target_world: nodes[2]['parent']=0
        cases.append(dict(op=op,argument=0,target=2,map=(-100,-100,400,300),nodes=nodes,anchors=[(1,-17,27),(2,111,-93)],clip=[]))
    rectangles=((-10,-20,30,40),(0,0,10,10),(80,-1,100,20),(1024,768,1025,769),(100,50,90,40),(-2147483648,-2147483648,2147483647,2147483647))  # 포함 교차와 개별 clamp의 차이를 가르는 완성 좌표다.
    for bounds,rectangle in itertools.product(maps,rectangles): cases.append(dict(op='Clamp',argument=0,target=0,map=bounds,nodes=[],anchors=[],clip=rectangle))  # 기대 좌표는 실제445290에서만 얻는다.
    return cases


def main():
    """최초/재관측/확장 정밀도를 비교하고 제품과 독립인 완성 입력/관측을 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    cases=plan();oracle=Oracle();first=[oracle.observe(case,0x27f) for case in cases];replay=[oracle.observe(case,0x27f) for case in cases];extended=[oracle.observe(case,0x37f) for case in cases];assert first==replay==extended
    fixture=out/'gump-redraw-x86.tsv'
    with fixture.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# op\tid\targument\ttarget\tmap\tnodes\tanchors\trequests\tcaches\tprojected\tclip\n')
        for index,(case,observed) in enumerate(zip(cases,first)):  # 입력은 raw 바이트이고 결과는 원문 호출/캐시 관측이다.
            nodes=';'.join(f'{token(number)}={raw(value).hex()}' for number,value in enumerate(case['nodes'])) or '-';anchors=';'.join(integers(value) for value in case['anchors']) or '-'
            stream.write('\t'.join((case['op'],str(index),str(case['argument']),str(token(case['target'])) if case['nodes'] else '0',integers(case['map']),nodes,anchors,*observed,integers(case['clip']) or '-'))+'\n')
    report={'cases':len(cases),'observations':len(cases)*3,'differences':0,'host':'HJOW-Athlon','lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','source':'originals/Netstorm.exe','binarySha256':digest(ROOT/'originals/Netstorm.exe'),'fixtureSha256':digest(fixture),'oracleSha256':digest(Path(__file__)),'scope':'기본 Gump dirty 요청/지도 clamp의 격리 PE 관측이며 게임/GUI/네트워크/포획/희생/특정 공격/승리를 실행하지 않는다. 마지막 Renderer 저장소와 월드 투영만 명시 경계다. 새 C++는 읽거나 호출하지 않는다.'}
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False),flush=True)


if __name__=='__main__':  # 명시 생성 요청에서만 관측 파일을 만든다.
    main()
