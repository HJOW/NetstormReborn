#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 마나103의 추가 Draw/TYPE 조회/현재 SHP 읽기를 제한 PE에서 독립 관측한다."""
import argparse,csv,itertools,json,struct
from pathlib import Path
from decomp_gump_redraw import Oracle as BaseOracle,digest,integers
from decomp_process_oracle import ROOT,STACK,STOP,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EDI

RAW,TYPES=0x15008000,0x17000000  # 부모 장치의 독립 raw 공간과256개 TYPE500바이트 표다.


def mapping(values):
    """입력/관측의 TYPE 번호와 SHP 토큰을 주소순 정수열로 직렬화한다."""
    return ';'.join(f'{key}={value}' for key,value in sorted(values.items()))


class Oracle(BaseOracle):
    """세 실제 함수만 실행하고 마지막 VFX 출력의 인수/명시 변경만 관측한다."""
    def __init__(self):
        """현재 정밀 내보내기에서473710/4ac550/49a840의 몸체만 허용한다."""
        super().__init__();self.mu.mem_map(TYPES,0x20000);self.allowed=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x473710,0x4ac550,0x49a840):  # 추가 Draw와 현재 TYPE 포인터의 두 원문 getter다.
            for part in facts[address]['ranges'].split(';'):  # 함수 사이의 코드/게임/OS 진입은 허용하지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """실제 SHP 메모리 읽기와 VFX 인수를 기록하며 출력 뒤 명시 변경을 적용한다."""
        if address in (0x473738,0x47374f):
            pointer=mu.reg_read(UC_X86_REG_EDI);self.shape_reads.append([(pointer-TYPES)//500,self.u32(pointer+0xdc)])
        if address in (0x4dab90,0x4daa70):
            esp=mu.reg_read(UC_X86_REG_ESP);shadow=address==0x4dab90;args=list(struct.unpack('<'+('5I' if shadow else '4I'),mu.mem_read(esp+4,20 if shadow else 16)))
            if shadow: assert args[4]==0x557738
            self.draws.append([int(shadow),*args[:4]])
            if shadow and self.case['mutation']:
                self.put(RAW+36,self.case['newframe']);mu.mem_write(RAW+10,bytes([self.case['newtype']]))
                if self.case['mutation']==2:
                    self.put(TYPES+self.selected*500+0xdc,self.case['newshape']);mu.mem_write(RAW+12,b'\0\0')
            mu.reg_write(UC_X86_REG_EIP,self.u32(esp));mu.reg_write(UC_X86_REG_ESP,esp+4);return
        super().on_instruction(mu,address,size,data)

    def observe(self,case,control):
        """실제 ABI/보존 레지스터/x87/원본 raw·TYPE 보호 영역을 함께 확인한다."""
        self.reset(control);mu=self.mu;self.case=case;self.selected=bytes.fromhex(case['raw'])[10];self.draws=[];self.shape_reads=[]
        raw=bytes.fromhex(case['raw']);mu.mem_write(RAW-16,b'\xa5'*(len(raw)+32));mu.mem_write(RAW,raw)
        for number,shape in case['shapes'].items():  # 사용한 두 TYPE의 dc 주변도 독립 guard로 초기화한다.
            pointer=TYPES+number*500+0xdc;mu.mem_write(pointer-16,b'\xa5'*36);self.put(pointer,shape)
        self.put(0x59ab20,TYPES);mu.reg_write(UC_X86_REG_ECX,RAW);mu.mem_write(STACK,struct.pack('<3I',STOP,case['x']&0xffffffff,case['y']&0xffffffff))
        mu.emu_start(0x473710,STOP,count=100000);self.finish(8,control)
        assert bytes(mu.mem_read(RAW-16,16))+bytes(mu.mem_read(RAW+len(raw),16))==b'\xa5'*32
        shapes={}
        for number in case['shapes']:  # 명시 SHP DWORD 이외의 TYPE 바이트는 그대로 보존해야 한다.
            pointer=TYPES+number*500+0xdc;assert bytes(mu.mem_read(pointer-16,16))+bytes(mu.mem_read(pointer+4,16))==b'\xa5'*32;shapes[number]=self.u32(pointer)
        return [';'.join(integers(value) for value in self.draws),';'.join(integers(value) for value in self.shape_reads),bytes(mu.mem_read(RAW,len(raw))).hex(),mapping(shapes)]


def plan():
    """조건 WORD/프레임/큰 좌표와 출력 콜백의 현재 값 변경을 독립 교차한다."""
    result=[]
    for number,flags,frame,point,mutation in itertools.product((70,103,255),(0,1,0xffff,0x8000,0x10000,0x80000000,0x7fff),(0,17,0xffffffff),((10,20),(-17,-31),(2147483647,-2147483648),(-2147483643,2147483642)),(0,1,2)):  # 기대 좌표/출력은 계산하지 않는다.
        raw=bytearray(b'\x6d'*50);struct.pack_into('<I',raw,0,0x50c3b0);raw[10]=number;struct.pack_into('<I',raw,12,flags);struct.pack_into('<I',raw,36,frame);other=(number+1)&255
        result.append({'raw':raw.hex(),'x':point[0],'y':point[1],'shapes':{number:0x1a000000+number*0x100,other:0x1a000000+other*0x100},'mutation':mutation,'newtype':other,'newframe':0x12345678,'newshape':0x1b000000+number*0x100})
    return result


def main():
    """최초/재관측/확장 정밀도를 비교하고 새 출력 경로에만 원문 fixture를 만든다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    cases=plan();oracle=Oracle();first=[oracle.observe(case,0x27f) for case in cases];repeat=[oracle.observe(case,0x27f) for case in cases];extended=[oracle.observe(case,0x37f) for case in cases];assert first==repeat==extended
    fixture=out/'mana-additional-x86.tsv'
    with fixture.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# id\traw\tx\ty\tshapes\tmutation\tnewtype\tnewframe\tnewshape\tdraws\tshapeReads\trawOut\tshapesOut\n')
        for index,(case,observed) in enumerate(zip(cases,first)):  # 원문 완성 입력과 실제 관측만 저장하며 C++는 읽지 않는다.
            stream.write('\t'.join((str(index),case['raw'],str(case['x']&0xffffffff),str(case['y']&0xffffffff),mapping(case['shapes']),str(case['mutation']),str(case['newtype']),str(case['newframe']),str(case['newshape']),*observed))+'\n')
    (out/'observed.json').write_text(json.dumps({'cases':cases,'observed':first},ensure_ascii=False,separators=(',',':'))+'\n',encoding='utf-8')
    report={'cases':len(cases),'observations':len(cases)*3,'differences':0,'host':'HJOW-Athlon','lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','sourceSha256':digest(ROOT/'originals/Netstorm.exe'),'toolSha256':digest(Path(__file__)),'fixtureSha256':digest(fixture),'dataSha256':digest(out/'observed.json'),'scope':'실제473710/4ac550/49a840만 실행한다. VFX plain/그림자 출력은 마지막 인수 관측과 명시 raw/TYPE 변경 경계다. ABI·x87·raw/TYPE guard를 확인하며 새 C++·실제 게임·GUI·공중3종·MP·금지 재현을 실행/참조하지 않는다.'}
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False),flush=True)


if __name__=='__main__':  # 명시한 새 경로에서만 관측을 작성한다.
    main()
