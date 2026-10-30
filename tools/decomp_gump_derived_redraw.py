# -*- coding: utf-8 -*-
"""원문 장식/메뉴 행의 사각형·위치 캐시·투영을 격리 PE에서 관측하고 독립 fixture를 만든다."""
import argparse,csv,itertools,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path.cwd()/'tools'))  # 기존 제한 PE 장치를 읽는다.
from decomp_gump_redraw import Oracle as BaseOracle,integers,digest
from decomp_gumpcollection_oracle import node,raw,token,DATA,POOL
from decomp_process_oracle import ROOT,STACK,STOP,fbits,UC_X86_REG_ECX


class Oracle(BaseOracle):
    """실제 Position/CRT/XYWH 전달을 유지하고 마지막 Renderer 저장소만 관측한다."""
    def __init__(self):
        """현재 PC가 만든 두 부분 내보내기에서 대상 함수 몸체만 허용한다."""
        super().__init__()
        for directory,address in (('menuwidgets',0x465e60),('menuinteraction',0x476e00)):  # 실제 장식/메뉴 행의 두 진입점이다.
            with (ROOT/f'extracted/{directory}/originals/functions.tsv').open(encoding='utf-8') as stream:
                facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
            for part in facts[address]['ranges'].split(';'):  # 함수 사이 바이트는 실행하지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def observe(self,case,control):
        """가시성·현재 크기·행 번호·위치·캐시와 원본 flags를 독립 관측한다."""
        self.reset(control);mu=self.mu;mu.mem_write(DATA,bytes(0x1000));expected=[]
        for index,encoded in enumerate(case['nodes']):  # 완성 바이트에는 메뉴 파생의 원래 offset/height 배열도 포함된다.
            value=bytes.fromhex(encoded);mu.mem_write(token(index),value);expected.append(value)
            self.put(POOL+index*self.d['stride']+14,index);self.put(POOL+index*self.d['stride']+18,0)
        self.anchors={sid:(x,y,0,0) for sid,x,y in case['anchors']}
        for address,value in ((self.d['root'],token(0)),(self.d['override'],0),(self.d['pool'],POOL),(0x59a8bc,DATA+0x5000)):
            self.put(address,value)  # 원문의 같은 현재 전역만 공급한다.
        self.writes += [(token(index)+108,token(index)+116) for index in range(2)]
        mu.reg_write(UC_X86_REG_ECX,token(1));mu.mem_write(STACK,struct.pack('<2I',STOP,case['argument']))
        mu.emu_start(0x465e60 if case['op']=='Decoration' else 0x476e00,STOP,count=100000);self.finish(4,control);caches=[]
        for index,value in enumerate(expected):  # 허용 캐시 이외에는 전체 파생 객체도 보존해야 한다.
            actual=bytearray(mu.mem_read(token(index),len(value)));caches.append(integers((token(index),*struct.unpack_from('<2i',actual,108))))
            actual[108:116]=value[108:116];assert actual==value
        return [';'.join(integers(value) for value in self.requests) or '-',';'.join(caches),','.join(self.projected) or '-']


def plan():
    """기대 계산 없이 원문이 실제 읽는 float/가시성/인수와 월드 투영을 나열한다."""
    cases=[]
    geometry=((10.75,20.5,12.75,9.5,-.75,4.75),(-.75,-.25,12.75,9.5,2.5,3.5),(100,200,-3.5,-3.5,-10.75,1.5),(100,200,0,0,0,0),(80.5,-.5,-4,-4,20,-1),(2147483648.0,-2147483648.0,12.75,9.5,-.25,20),(10,20,float('nan'),9.5,0,1),(10,20,12.75,float('inf'),-.75,float('nan')))  # 행 offset/height도 독립 입력이다.
    for op,argument,visible,parent_visible,world,shape in itertools.product(('Decoration','Compact','Row'),(0,3,39),(0,1,0x10001,9),(0,1),(False,True),geometry):
        x,y,width,height,offset,row_height=shape  # 생성자나 C++가 만든 상태를 기대 입력으로 사용하지 않는다.
        root=raw(node(0,flags=parent_visible,x=fbits(.75),y=fbits(-.25)))
        target=bytearray(raw(node(1,flags=visible|(0x2000 if world else 0),parent=0,x=fbits(x),y=fbits(y),width=fbits(width),height=fbits(height))))
        target.extend(bytes(0xb90-len(target)));struct.pack_into('<I',target,0x220,0x1000000 if op=='Compact' else 0)
        struct.pack_into('<I',target,0xd0+argument*4,fbits(row_height));struct.pack_into('<I',target,0x17c+argument*4,fbits(offset))
        cases.append({'op':op,'argument':argument,'nodes':[root.hex(),target.hex()],'anchors':[[1,-17,27]] if world else []})
    return cases


def main():
    """별도 새 디렉토리에 입력/두 정밀도·재실행 관측과 바이트 해시만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False);cases=plan();oracle=Oracle()
    first=[oracle.observe(case,0x27f) for case in cases];repeat=[oracle.observe(case,0x27f) for case in cases];extended=[oracle.observe(case,0x37f) for case in cases];assert first==repeat==extended
    data={'cases':cases,'observed':first};(output/'observed.json').write_text(json.dumps(data,ensure_ascii=False,separators=(',',':'))+'\n',encoding='utf-8')
    fixture=output/'gump-derived-redraw-x86.tsv'
    with fixture.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# op\tid\targument\troot\ttarget\tmenuFlags\trowHeight\toffset\tanchors\trequests\tcaches\tprojected\n')
        for index,(case,observed) in enumerate(zip(cases,first)):  # C++를 읽지 않고 완성 원문 입력의 사용 필드와 실제 관측만 저장한다.
            argument=case['argument'];target=bytes.fromhex(case['nodes'][1]);number=lambda offset:struct.unpack_from('<I',target,offset)[0]
            anchors=';'.join(','.join(str(value&0xffffffff) for value in anchor) for anchor in case['anchors']) or '-'
            stream.write('\t'.join((case['op'],str(index),str(argument),case['nodes'][0],target[:128].hex(),str(number(0x220)),str(number(0xd0+argument*4)),str(number(0x17c+argument*4)),anchors,*observed))+'\n')
    report={'fixtureSha256':digest(fixture),'cases':len(cases),'observations':len(cases)*3,'differences':0,'sourceSha256':digest(ROOT/'originals/Netstorm.exe'),'toolSha256':digest(Path(__file__)),'dataSha256':digest(output/'observed.json'),'host':'HJOW-Athlon','lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','scope':'격리 PE 실제 장식/메뉴 행/Position/CRT/XYWH다. 마지막 Renderer 저장소/월드 투영만 경계이며 게임·GUI·새 C++·fixture는 실행/참조/수정하지 않는다.'}
    (output/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False),flush=True)


if __name__=='__main__':  # 명시한 새 관측 경로만 생성한다.
    main()
