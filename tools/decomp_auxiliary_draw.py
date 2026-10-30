#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78의 배율479340/고정 추상색485f90 추가 Draw를 독립 PE로 관측한다."""
import argparse,csv,itertools,json,struct
from pathlib import Path
from decomp_gump_redraw import Oracle as BaseOracle,digest,integers
from decomp_process_oracle import ROOT,STACK,STOP,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

RAW,TYPES=0x15008000,0x17000000  # 독립50BYTE 객체와256개 TYPE500BYTE 표다.


class Oracle(BaseOracle):
    """두 몸체/TYPE getter만 실행하고 마지막 VFX 인수와 SHP 읽기를 기록한다."""
    def __init__(self):
        """정밀C에 없는479340은 정적으로 확인한ret8까지의 정확한 PE 범위를 사용한다."""
        super().__init__();self.mu.mem_map(TYPES,0x20000);self.allowed=[(0x479340,0x479386)]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x485f90,0x4ac550,0x49a840):  # 실제 추상색/TYPE 포인터 조회다.
            for part in facts[address]['ranges'].split(';'):  # 함수 사이 코드는 실행하지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """실제 선택한 TYPE의 SHP DWORD와 VFX의7/5개 인수를 읽는다."""
        if address in (0x479374,0x485fa8):
            pointer=mu.reg_read(UC_X86_REG_EAX);self.reads.append([(pointer-TYPES)//500,self.u32(pointer+0xdc)])
        if address in (0x4daaf0,0x4dab90):
            esp=mu.reg_read(UC_X86_REG_ESP);scaled=address==0x4daaf0;args=list(struct.unpack('<'+('7I' if scaled else '5I'),mu.mem_read(esp+4,28 if scaled else 20)))
            if scaled: assert args[4]==0
            else: assert args[4]==0x556238
            self.draws.append([int(not scaled),*args[:4],*(args[5:] if scaled else (0,0))])
            mu.reg_write(UC_X86_REG_EIP,self.u32(esp));mu.reg_write(UC_X86_REG_ESP,esp+4);return
        super().on_instruction(mu,address,size,data)

    def observe(self,case,control):
        """원문 raw/TYPE 보호·정상 ABI·x87와 실제 출력/조회 순서를 보존한다."""
        self.reset(control);mu=self.mu;self.reads=[];self.draws=[];raw=bytes.fromhex(case['raw']);mu.mem_write(RAW-16,b'\xa5'*(len(raw)+32));mu.mem_write(RAW,raw)
        pointer=TYPES+raw[10]*500+0xdc;mu.mem_write(pointer-16,b'\xa5'*36);self.put(pointer,case['shape']);self.put(0x59ab20,TYPES)
        mu.reg_write(UC_X86_REG_ECX,RAW);mu.mem_write(STACK,struct.pack('<3I',STOP,case['x']&0xffffffff,case['y']&0xffffffff));mu.emu_start(0x479340 if case['op']=='Scaled' else 0x485f90,STOP,count=100000);self.finish(8,control)
        assert bytes(mu.mem_read(RAW-16,len(raw)+32))==b'\xa5'*16+raw+b'\xa5'*16 and bytes(mu.mem_read(pointer-16,36))==b'\xa5'*16+struct.pack('<I',case['shape'])+b'\xa5'*16
        return [';'.join(integers(value) for value in self.draws) or '-',';'.join(integers(value) for value in self.reads) or '-']


def plan():
    """두 몸체의 사용/미사용 비트·분수·프레임·현재 TYPE·극단 좌표를 교차한다."""
    result=[]
    for op,flags,fraction,frame,number,point in itertools.product(('Scaled','Abstract'),(0,1,0x3fff,0x4000,0x4001,0x8000,0xabcd0000,0xabcd0001),(0,1,32768,65535),(0,17,0xffffffff),(70,103,255),((10,20),(-17,-31),(2147483647,-2147483648))):  # 기대 배율/출력은 계산하지 않는다.
        raw=bytearray(b'\x6d'*50);struct.pack_into('<I',raw,0,0x50cc08 if op=='Scaled' else 0x50db78);struct.pack_into('<H',raw,8,fraction);raw[10]=number;struct.pack_into('<I',raw,26,flags);struct.pack_into('<I',raw,36,frame)
        result.append({'op':op,'raw':raw.hex(),'x':point[0],'y':point[1],'shape':0x1a000000+number*0x100})
    return result


def main():
    """두 정밀도/재관측과 원시 바이트를 새 출력 경로에만 기록한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False);cases=plan();oracle=Oracle()
    first=[oracle.observe(case,0x27f) for case in cases];repeat=[oracle.observe(case,0x27f) for case in cases];extended=[oracle.observe(case,0x37f) for case in cases];assert first==repeat==extended
    fixture=out/'auxiliary-draw-x86.tsv'
    with fixture.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# id\top\traw\tx\ty\tshape\tdraws\tshapeReads\n')
        for index,(case,observed) in enumerate(zip(cases,first)):  # 새 C++를 읽지 않고 완성 입력과 실제 PE 관측만 저장한다.
            stream.write('\t'.join((str(index),case['op'],case['raw'],str(case['x']&0xffffffff),str(case['y']&0xffffffff),str(case['shape']),*observed))+'\n')
    (out/'observed.json').write_text(json.dumps({'cases':cases,'observed':first},ensure_ascii=False,separators=(',',':'))+'\n',encoding='utf-8')
    report={'cases':len(cases),'observations':len(cases)*3,'differences':0,'host':'HJOW-Athlon','lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','sourceSha256':digest(ROOT/'originals/Netstorm.exe'),'toolSha256':digest(Path(__file__)),'fixtureSha256':digest(fixture),'dataSha256':digest(out/'observed.json'),'scope':'실제479340/485f90/4ac550/49a840이다. 마지막 배율/추상 VFX만 인수 관측 경계이며 raw/TYPE/ABI/x87를 검사한다. C++/실제게임/GUI/공중3종 전용/MP/금지 재현을 실행/참조하지 않는다.'}
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False),flush=True)


if __name__=='__main__':  # 명시 새 경로만 생성한다.
    main()
