#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Draw가 지형 수집기에 전달하는 월드 사각형/행 제한을 격리 x86에서 관측한다."""
import argparse,csv,itertools,json,platform,struct
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,STACK
from unicorn.x86_const import UC_X86_REG_EBP,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
ENTRY=0x499758  # 앞단 뒤 전체 지도 환산부터 실제 첫 dirty/수집기 인자 계산을 실행한다.


def bits(value):
    """입력 세계 좌표를 명시 단정도 원문 비트로 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


class Oracle(MemoryOracle):
    """상위 계산/정수·실수 clip은 PE이며 UI/건설/화면 장치만 명시 경계다."""
    def __init__(self):
        """실제 정밀 함수 범위의 두 clip 몸체를 기존 보호 PE에 합성한다."""
        super().__init__();self.allowed=[(ENTRY,0x499bfd)]  # 첫 지형 수집기까지 원문만 실행한다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x445290,0x4970c0):  # 정수 화면/단정도 세계 clip은 stub으로 계산하지 않는다.
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """원문이 실제 지형 수집기에 넘긴 네 float 비트와 행 제한만 기록한다."""
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x433410:
            args=self.words(esp+4,8);self.result=[*args[:4],args[6]];mu.emu_stop();return
        if address in (0x466070,0x487f10,0x4a1580): self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed): raise RuntimeError(f'Renderer 환산 관측 밖 명령 {address:08x}')

    def run(self,case,control):
        """한 dirty와 명시 지도/카메라로 실제 첫 지형 수집 호출까지 도달한다."""
        width,height,*rest=case;self.result=None  # 앞 실행 결과를 다음 호출 성공으로 쓰지 않는다.
        self.mu.mem_write(POOL,bytes(0x800));self.mu.mem_write(STACK-0x1000,bytes(0x1100))
        for address,value in ((0x5c78f0,width),(0x5c78f4,height),(0x531860,width),(0x531864,height),(0x5c78d4,0),(0x59a8c0,POOL+0x10000),(0x59a8c4,POOL+0x30000),(0x59a8c8,POOL+0x20000),(0x59a900,case[10]),(0x59a904,case[11]),(POOL+8,16),(POOL+12,11),(POOL+0x64,1)):
            self.put(address,value)  # 현재 전역/Renderer 입력을 실제 위치에 공급한다.
        for index,value in enumerate(case[2:6]): self.put(0x5ca9cc+index*4,value)  # 현재 지도LTRB다.
        for index,value in enumerate(case[6:10]): self.put(POOL+0x68+index*4,value)  # 현재 화면 dirtyLTRB다.
        for register,value in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_EBP,POOL),(UC_X86_REG_EBX,0),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):
            self.mu.reg_write(register,value)  # 실제 부모 구간의 스택/this/빈 x87 계약이다.
        self.mu.emu_start(ENTRY,0x499bfd,count=10000);assert self.result is not None,case
        return self.result


def main():
    """세 화면비/지도·부분 dirty/분수 카메라의 최초와 독립 재관측만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    cases=[]
    for width,height in ((1024,768),(1280,720),(1280,800)):
        for map_rect,dirty,world in itertools.product(((0,0,width,height),(84,0,width,height-50)),((0,0,width,height),(0,0,10,10),(100,200,301,401),(width-1,height-1,width,height),(84,30,85,31)),((0.0,0.0),(61.125,71.875),(-0.5,0.5))):
            cases.append([width,height,*map_rect,*dirty,bits(world[0]),bits(world[1])])  # 기대값과 독립인 실제 화면/세계 입력이다.
    oracle=Oracle();first=[oracle.run(case,0x27f) for case in cases];replay=[oracle.run(case,0x27f) for case in cases];extended=[oracle.run(case,0x37f) for case in cases];assert first==replay==extended
    with (out/'renderer-projection-x86.tsv').open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# 화면WH/mapLTRB/dirtyLTRB/worldXYbits / 실제지형인자4bits·행제한\n')
        for case,result in zip(cases,first): stream.write('\t'.join(map(str,(*case,*result)))+'\n')  # 실제 전달 인자만 기대값으로 저장한다.
    report={'unit':620,'host':platform.node(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','cases':len(cases),'observations':len(cases)*3,'differences':0,'scope':'499758 이후 실제 환산/두 clip과 첫 지형 수집 인자다. UI/건설/화면 장치는 명시 경계이며 지형 수집기 몸체/게임GUI/MP/공격/포획/희생/승리를 실행하지 않았다.'}
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
