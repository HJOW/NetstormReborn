#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 원본 VFX 전체 배율 커널/압축 해제를 가상 표면에서 실행해 픽셀 fixture를 만든다."""
import argparse,csv,itertools,json,struct
from pathlib import Path
from decomp_graphics_oracle import GraphicsOracle,synthetic_shapes,SCRATCH,SHAPE,CANVAS,ROOT
from decomp_gump_redraw import digest
from unicorn.x86_const import UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG


class Oracle(GraphicsOracle):
    """기존 flat32 GDT/가상 표면에 실제 래퍼·변환·RLE 함수만 허용한다."""
    def __init__(self):
        """현재 PC의 전체 변환 내보내기를 사용하며 CRT/OS/assert 대체를 비운다."""
        super().__init__('originals');self.stubs={}
        with (ROOT/'extracted/vfxscale/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 실제 커널/좌표/삼각함수/압축 크기/임시 표면/실제 RLE다.
                for part in row['ranges'].split(';'):  # 함수 사이와 게임/OS 진입은 허용하지 않는다.
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.entries['ScaledWrapper']=0x4daaf0

    def pixels(self,case,control):
        """실제 전역 pane 래퍼의 절대 기준점 보정과 전체 화면 바이트를 관측한다."""
        mu=self.mu;shape=bytes.fromhex(case['shape']);width,height=case['size'];count=width*height;mu.mem_write(0x5b5ddc,struct.pack('<Iiiii',SCRATCH+32,*case['pane']));mu.mem_write(SCRATCH+32,struct.pack('<Iiii',CANVAS,width-1,height-1,1))
        mu.mem_write(SHAPE,shape);mu.mem_write(CANVAS-16,b'\xa5'*16+bytes([case['background']])*count+b'\xa5'*16);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        self.call('ScaledWrapper',[SHAPE,0,*case['position'],0,*case['scale']],purge=0)
        assert bytes(mu.mem_read(CANVAS-16,16))+bytes(mu.mem_read(CANVAS+count,16))==b'\xa5'*32 and bytes(mu.mem_read(SHAPE,len(shape)))==shape
        assert mu.reg_read(UC_X86_REG_FPCW)==control and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        return bytes(mu.mem_read(CANVAS,count)).hex()


def plan():
    """물리 유닛 없이 고정 RLE 입력12개·다섯 배율·두 clip·두 기준점을 교차한다."""
    shapes=list(itertools.islice(synthetic_shapes(),12));cases=[]
    for shape,scale,pane,position in itertools.product(shapes,((32768,32768),(65535,65535),(65536,65536),(98304,98304),(131072,131072)),((0,0,31,23),(3,2,27,20)),((12,10),(-1,1))):  # 픽셀 기대값은 만들지 않는다.
        cases.append({'shape':shape.hex(),'size':(32,24),'pane':pane,'position':position,'scale':scale,'background':199})
    return cases


def main():
    """두 정밀도/재관측을 비교하고 실제 원본 표면만 새 경로에 기록한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False);cases=plan();oracle=Oracle()
    first=[oracle.pixels(case,0x27f) for case in cases];repeat=[oracle.pixels(case,0x27f) for case in cases];extended=[oracle.pixels(case,0x37f) for case in cases];assert first==repeat==extended
    fixture=out/'scaled-pixels-x86.tsv'
    with fixture.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# id\tshape\tsize\tpane\tposition\tscale\tbackground\tpixels\n')
        for index,(case,pixels) in enumerate(zip(cases,first)):  # C++나 복원 측 계산 없이 실제 원본 표면만 저장한다.
            fields=(str(index),case['shape'],','.join(map(str,case['size'])),','.join(str(value&0xffffffff) for value in case['pane']),','.join(str(value&0xffffffff) for value in case['position']),','.join(map(str,case['scale'])),str(case['background']),pixels);stream.write('\t'.join(fields)+'\n')
    report={'cases':len(cases),'observations':len(cases)*3,'differences':0,'host':'HJOW-Athlon','lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','sourceSha256':digest(ROOT/'originals/Netstorm.exe'),'toolSha256':digest(Path(__file__)),'fixtureSha256':digest(fixture),'scope':'실제4daaf0/4028cb와 좌표/각도0/실제RLE/임시표면/현재pane의 최종 픽셀이다. 출력/CRT/assert/OS 대체가 없으며 C++/실제게임/GUI/공중3종/MP/금지재현을 실행/참조하지 않는다.'}
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False),flush=True)


if __name__=='__main__':  # 실제 화면/게임 대신 새 가상 표면만 실행한다.
    main()
