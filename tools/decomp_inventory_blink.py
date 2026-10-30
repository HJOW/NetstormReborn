# -*- coding: utf-8 -*-
"""생산 항목 점멸의 실제 PE/Idle표/기본 사각형과 전체 노드를 독립 fixture로 관측한다."""
import csv,itertools,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path.cwd()/'tools'))  # 기존 제한 PE/Position/CRT 장치를 읽는다.
from decomp_gump_redraw import Oracle as BaseOracle,integers,digest
from decomp_gumpcollection_oracle import node,raw,token,DATA,POOL
from decomp_process_oracle import ROOT,STACK,STOP,fbits,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP


class Oracle(BaseOracle):
    """실제 점멸/Idle/Position/절삭/사각형을 실행하고 벽시계만 명시 응답한다."""
    def __init__(self):
        """정밀 내보내기에서 점멸/기본 Idle 함수의 정확한 몸체만 허용한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x43d6e0,0x465330):  # 실제 항목 Idle와 단일 공유 Idle표 변경이다.
            for part in facts[address]['ranges'].split(';'):  # 함수 간 코드는 실행하지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """원문 가상 호출 시점의 visible을 기록하며 실제 기본 몸체를 계속 실행한다."""
        if address==0x460d70:
            esp=mu.reg_read(UC_X86_REG_ESP);pointer=self.u32(esp+4);value=self.times[self.time_calls]
            if not self.time_calls and self.first_deadline is not None: mu.mem_write(token(1)+0xa8,struct.pack('<d',self.first_deadline))  # 시각 조회 뒤 현재 기한을 읽는 순서를 가르는 명시 외부 효과다.
            self.time_calls+=1
            self.trace.append(['clock',self.u32(token(1)+4),value]);mu.mem_write(pointer,struct.pack('<d',value));mu.reg_write(UC_X86_REG_EAX,pointer)
            mu.reg_write(UC_X86_REG_EIP,self.u32(esp));mu.reg_write(UC_X86_REG_ESP,esp+4);return
        if address in (0x464a50,0x465330): self.trace.append(['redraw' if address==0x464a50 else 'idle',self.u32(token(1)+4),self.u32(mu.reg_read(UC_X86_REG_ESP)+4)])  # 호출 순서/가시성은 기대 계산 없이 읽는다.
        super().on_instruction(mu,address,size,data)

    def observe(self,case,control):
        """원문 객체/Pool/Idle표를 독립 초기화하고 정상 반환·전체 보존 바이트를 검사한다."""
        self.reset(control);mu=self.mu;self.times=case['times'];self.time_calls=0;self.first_deadline=case.get('firstDeadline');self.trace=[];mu.mem_write(DATA,bytes(0x1000))
        root=raw(node(0,flags=case['parentVisible']));value=bytearray(raw(node(1,flags=case['flags'],parent=0,x=fbits(10.75),y=fbits(20.5),width=fbits(75),height=fbits(20))))
        value.extend(bytes(0xc0-len(value)));struct.pack_into('<I',value,0,0x5068f4);struct.pack_into('<I',value,0xa0,1);struct.pack_into('<d',value,0xa8,case['until']);value[0xb0]=case['blink']
        mu.mem_write(token(0),root);mu.mem_write(token(1),bytes(value));mu.mem_write(0x5634d0,bytes(2000))
        if case['flags']&0x80: self.put(0x5634d0,token(1))
        for address,number in ((self.d['root'],token(0)),(self.d['override'],0),(self.d['pool'],POOL),(0x59a8bc,DATA+0x5000)):
            self.put(address,number)  # 원문이 같은 루트/Pool/Renderer를 읽는다.
        mu.mem_write(POOL+50+21,b'\xff');self.anchors={};self.writes += [(token(1)+4,token(1)+8),(token(1)+108,token(1)+116),(token(1)+0xa8,token(1)+0xb1),(0x5634d0,0x5634d0+2000),(POOL+50+21,POOL+50+22)]
        mu.reg_write(UC_X86_REG_ECX,token(1));mu.mem_write(STACK,struct.pack('<I',STOP));mu.emu_start(0x43d6e0,STOP,count=100000);self.finish(0,control)
        actual=bytearray(mu.mem_read(token(1),len(value)));snapshot=bytes(actual).hex()
        for low,high in ((4,8),(108,116),(0xa8,0xb1)): actual[low:high]=value[low:high]  # 원문이 쓰는 필드 이외는 전체0xc0 보존을 요구한다.
        assert actual==value and bytes(mu.mem_read(token(0),len(root)))==root
        return {'inputRoot':root.hex(),'inputTarget':bytes(value).hex(),'trace':self.trace,'target':snapshot,'idle':list(struct.unpack('<500I',mu.mem_read(0x5634d0,2000))),'poolByte':int(mu.mem_read(POOL+50+21,1)[0]),'requests':self.requests,'timeCalls':self.time_calls}


def timebits(value):
    """NaN을 포함한 입력/출력 시각의 정확한 double 비트를 직렬화한다."""
    return f'{struct.unpack("<Q",struct.pack("<d",value))[0]:016x}'


def main():
    """원문이 읽는 카운터/visible/조상/두 시각을 독립 교차한다."""
    output=Path(sys.argv[1]);output.mkdir(parents=True,exist_ok=False)
    timing=((0,[0,1]),(1,[.5,.75]),(1,[1,1.25]),(1,[2,2.25]),(float('nan'),[2,2.25]),(1,[float('nan'),2.25]))  # 첫 조회와 재예약 조회가 서로 다른 명시 벽시계다.
    cases=[{'blink':blink,'flags':flags,'parentVisible':parent,'until':until,'times':times} for blink,flags,parent,(until,times) in itertools.product((0,1,4),(0x80,0x81,0x10081),(0,1),timing)]  # 숨김/표시/죽은 표시를 독립 입력으로 사용한다.
    cases.extend({'blink':1,'flags':0x81,'parentVisible':1,'until':until,'times':[1,1.25],'firstDeadline':changed} for until,changed in ((0,100),(100,0)))  # 첫 시각 반환 중 기한 변경의 양방향 경계다.
    oracle=Oracle();first=[oracle.observe(case,0x27f) for case in cases];repeat=[oracle.observe(case,0x27f) for case in cases];extended=[oracle.observe(case,0x37f) for case in cases]
    assert json.dumps(first,sort_keys=True)==json.dumps(repeat,sort_keys=True)==json.dumps(extended,sort_keys=True)
    (output/'observed.json').write_text(json.dumps({'cases':cases,'observed':first},ensure_ascii=False,separators=(',',':'))+'\n',encoding='utf-8')
    fixture=output/'inventory-blink-x86.tsv'
    with fixture.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# id\troot\tinput\ttimes\ttarget\ttrace\tidle\tpoolByte\trequests\ttimeCalls\tfirstDeadline\n')
        for index,(case,observed) in enumerate(zip(cases,first)):  # 입력 노드/두 시각과 실제 관측 상태만 저장하며 C++ 결과를 읽지 않는다.
            times=','.join(timebits(value) for value in case['times']);trace=';'.join(f'{name},{flags},{timebits(value) if name=="clock" else value}' for name,flags,value in observed['trace']) or '-'
            requests=';'.join(integers(value) for value in observed['requests']) or '-'
            stream.write('\t'.join((str(index),observed['inputRoot'],observed['inputTarget'],times,observed['target'],trace,integers(observed['idle']),str(observed['poolByte']),requests,str(observed['timeCalls']),timebits(case['firstDeadline']) if 'firstDeadline' in case else '-'))+'\n')
    report={'cases':len(cases),'observations':len(cases)*3,'differences':0,'sourceSha256':digest(ROOT/'originals/Netstorm.exe'),'toolSha256':digest(Path(__file__)),'fixtureSha256':digest(fixture),'dataSha256':digest(output/'observed.json'),'host':'HJOW-Athlon','lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','scope':'점멸43d6e0/Idle465330/기본Redraw464a50/Position/CRT/XYWH가 실제 PE다. 벽시계와 마지막Renderer 저장소만 명시 경계다. 새 C++/실제 게임/GUI/MP/금지 재현/공중3종은 실행하지 않는다.'}
    (output/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False),flush=True)


if __name__=='__main__':  # 명시 새 관측 폴더만 작성한다.
    main()
