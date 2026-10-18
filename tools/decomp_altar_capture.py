#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""제단의 실제 일반 소유자 색표/+78/빈+a4/ShapeToBuffer 전체 픽셀을 독립 관측한다."""
import argparse,csv,itertools,json,socket,struct
from decomp_owner_capture import Oracle as OwnerOracle,ROOT,POOL,TYPES,BUFFER,OUT,SHAPE,ENTRIES,digest,read_clusters,TYPE_LOAD_ORDER
from unicorn.x86_const import UC_X86_REG_EAX
FIXTURE=ROOT/'cpppj/tests/fixtures/altar-capture-x86.tsv' # 기존414fixture와 구별하는 새 독립 제단 원문이다.
REPORT=ROOT/'cpppj/altar-capture.json' # 새 전체 픽셀/원본 명령과 보호 파일 해시다.
TABLE,TYPE=0x5076b8,164 # 실제 제단 생성자의 최종 가상표와 현재 타입 번호다.

class Oracle(OwnerOracle):
    """이미 복원한 소유자 색표/캡처 원본에 실제 제단 가상 함수 두 개만 더한다."""
    def __init__(self):
        """보호PE/SHP를 읽고 실제 제단의 기본/추가 함수와 TYPE 기본 프레임을 확인한다."""
        super().__init__();self.clusters=read_clusters(self.texts[TYPE_LOAD_ORDER[TYPE-70].lower()]);self.frames=len(self.clusters)
        self.baseFrame=next((i for i,row in enumerate(self.clusters) if 'baseframe' in row[1].lower()),-1)
        image=bytes(self.mu.mem_read(TABLE+0x78,4));assert struct.unpack('<I',image)[0]==0x4acfe0 and struct.unpack('<I',self.mu.mem_read(TABLE+0xa4,4))[0]==0x41c100
        expected=bytes.fromhex('0fb6410a50e856d8feff8b802801000083c404c3');assert bytes(self.mu.mem_read(0x4acfe0,len(expected)))==expected
        self.allowed.append((0x4acfe0,0x4acfe0+len(expected)))
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for span in facts[0x41c100]['ranges'].split(';'): # 실제 빈ret8의 명령 범위도 그대로 실행한다.
            low,high=(int(value,16) for value in span.split('-'));self.allowed.append((low,high+1))
    def capture(self,frame,owner,color,zoom,extra,precision):
        """실제 제단 TYPE/SHP/표준색과 현재 raw에서 메타/전체 픽셀/문맥 불변을 읽는다."""
        mu=self.mu;self.maps(TYPE,82,self.candidates,precision);self.mode=1;self.allocations=[];record=bytearray(500);block=self.blocks[TYPE-70]
        struct.pack_into('<I',record,0,20);struct.pack_into('<I',record,0xdc,SHAPE+block.offset);struct.pack_into('<I',record,0x114,self.frames);struct.pack_into('<i',record,0x128,self.baseFrame);struct.pack_into('<2i',record,0x1dc,-2,3);mu.mem_write(TYPES+TYPE*500,bytes(record))
        raw=bytearray(50);struct.pack_into('<I',raw,0,TABLE);raw[10]=TYPE;raw[34]=owner;raw[40]=extra;struct.pack_into('<i',raw,36,frame);mu.mem_write(POOL+2500,bytes(raw))
        for address,value in ((0x5c8464,POOL),(0x59ab20,TYPES),(0x59a918,zoom),(0x542414,1),(0x531b08+owner*4,color)): # 명시 현재 원본 전역만 준비한다.
            mu.mem_write(address,struct.pack('<I',value))
        context=struct.pack('<5I',0x12345678,11,22,33,44);mu.mem_write(0x5b5ddc,context);mu.mem_write(BUFFER,b'\xa5'*0x10000);mu.mem_write(OUT,bytes(16));before=bytes(mu.mem_read(POOL+2500,50))
        self.call(0x4a7db0,0,struct.pack('<5I',50,OUT,OUT+4,OUT+8,OUT+12),0,precision,result=BUFFER);self.returns+=1
        metadata=bytes(mu.mem_read(OUT,16));width,height=struct.unpack('<2i',metadata[:8]);assert self.allocations==[width*height]
        assert bytes(mu.mem_read(0x5b5ddc,20))==context and bytes(mu.mem_read(POOL+2500,50))==before
        return metadata.hex(),bytes(mu.mem_read(BUFFER,width*height)).hex()

def cases(frames):
    """모든 실제 프레임/일반색·중립/추가·추상 비트와 별도 양수 배율을 교차한다."""
    rows=list(itertools.product(range(frames),((0,8),(1,0),(1,1),(8,2),(8,8)),(65536,),(0,2,16,18)))
    rows+=list(itertools.product((0,frames-1),((0,8),(1,1),(8,8)),(32768,98304),(0,2)))
    return list(dict.fromkeys((frame,owner,color,zoom,extra) for frame,(owner,color),zoom,extra in rows))

def main():
    """새 제품 분배 전 원문을 동결하거나 이미 동결한 전체 파일 해시/입력을 검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[row.split('\t') for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')]
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert len(rows)==report['uniqueInputs'] and [tuple(map(int,row[:5])) for row in rows]==cases(report['frames']) and all(row[5:7]==row[7:9] for row in rows)
        print('제단 전체 캡처 원문 감사',len(rows),report['normalReturns']);return
    oracles=[Oracle(),Oracle()];assert oracles[0].frames==oracles[1].frames;rows=[];pixels=0
    for index,case in enumerate(cases(oracles[0].frames)): # 기대 출력은 C++ 계산 없이 보호 원본만 실행해 얻는다.
        outputs=[oracle.capture(*case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))];assert outputs[0]==outputs[1],case;rows.append(tuple(map(str,case))+outputs[0]+outputs[1]);pixels+=len(bytes.fromhex(outputs[0][1]))
        if index%100==0:print('제단 실제 픽셀 관찰',index,flush=True)
    FIXTURE.write_text('# frame owner color zoom extra metadata53 pixels53 metadata64 pixels64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n','utf-8',newline='\n')
    paths=['originals/Netstorm.exe','originals/d/_shapes.shp','originals/netstorm.tarc','extracted/refined/originals/functions.tsv','tools/decomp_altar_capture.py','tools/decomp_owner_capture.py','tools/decomp_particle_gradient.py','tools/shp.py','cpppj/tests/fixtures/altar-capture-x86.tsv']
    report=dict(host=socket.gethostname(),date='2026-10-19',lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',frames=oracles[0].frames,baseFrame=oracles[0].baseFrame,uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),captureReturns=len(rows)*2,ownerMapReturns=len(rows)*2,pixels=pixels,precisionDifferentInputs=[],sha256={p:digest(ROOT/p) for p in paths},scope='실제제단164/VT5076b8 +78=4acfe0/+a4=빈41c100·표준소유자색·캡처/RLE전체픽셀. TYPE기준점−2,3/소유자색번호/양수fixed16/raw는명시입력,힙만장치경계. CD10.72/실제전체게임실행은0. 기존원본fixture는바꾸지않는다.')
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print('제단 독립 캡처',len(rows),'정상반환',report['normalReturns'],'전체픽셀',pixels)

if __name__=='__main__':main()
