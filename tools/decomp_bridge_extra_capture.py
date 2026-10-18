#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 다리 extra2의 기본 프레임/추가 그림과 보호60프레임 캡처를 별도 동결한다."""
import argparse,csv,json,socket,struct
from decomp_bridge_collapse import Oracle as BridgeOracle,inputs,ROOT,POOL,TYPES,BUFFER,OUT,SHAPE,digest

FIXTURE=ROOT/'cpppj/tests/fixtures/bridge-extra-capture-x86.tsv'  # 이전603행을 고치지 않는 실제 extra2 출력이다.
REPORT=ROOT/'cpppj/bridge-extra-capture.json'  # 실제 기본4바이트 몸체와 debug0 추가 그림의 범위를 명시한다.


class Oracle(BridgeOracle):
    """기존 실제 캡처 환경에 가상+78/+a4의 실제 몸체만 추가한다."""
    def __init__(self):
        """정밀 표에서 누락된4바이트 getter는 보호 PE 원문을 직접 확인한다."""
        super().__init__();assert bytes(self.mu.mem_read(0x4222f0,4))==bytes.fromhex('8b4124c3');self.allowed.append((0x4222f0,0x4222f4))
        assert bytes(self.mu.mem_read(0x5453ac,4))==bytes(4)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:row=next(row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==0x4223c0)
        # 실제 추가 그림 함수의 불연속 범위만 허용한다.
        for part in row['ranges'].split(';'):
            low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """추가 가상 호출을 세며 debug0의 원본 ret8까지 실제 실행한다."""
        if address in (0x4222f0,0x4223c0):self.calls[f'{address:08x}']+=1
        super().instruction(mu,address,size,data)

    def extra_capture(self,frame,owner,color,precision):
        """extra2와 실제5034c8의 가상 표를 사용해 원본 전체 픽셀을 관찰한다."""
        mu=self.mu;self.maps(82,82,self.candidates,precision);self.mode=1;self.allocations=[];block=self.blocks[12]
        record=bytearray(500);struct.pack_into('<I',record,0,20);struct.pack_into('<I',record,0xdc,SHAPE+block.offset);struct.pack_into('<I',record,0x114,60);struct.pack_into('<2i',record,0x1dc,0,0);mu.mem_write(TYPES+82*500,bytes(record))
        raw=bytearray(50);struct.pack_into('<I',raw,0,0x5034c8);raw[10]=82;raw[34]=owner;raw[40]=2;struct.pack_into('<i',raw,36,frame);mu.mem_write(POOL+2500,bytes(raw))
        mu.mem_write(0x5c8464,struct.pack('<I',POOL));mu.mem_write(0x59ab20,struct.pack('<I',TYPES));mu.mem_write(0x59a918,struct.pack('<I',65536));mu.mem_write(0x542414,struct.pack('<I',1));mu.mem_write(0x531b08+owner*4,struct.pack('<I',color))
        context=struct.pack('<5I',0x12345678,11,22,33,44);mu.mem_write(0x5b5ddc,context);mu.mem_write(BUFFER,b'\xa5'*0x10000);mu.mem_write(OUT,bytes(16));before=bytes(mu.mem_read(POOL+2500,50))
        self.call(0x4a7db0,0,struct.pack('<5I',50,OUT,OUT+4,OUT+8,OUT+12),0,precision,result=BUFFER);self.returns+=1
        metadata=bytes(mu.mem_read(OUT,16));width,height=struct.unpack('<2i',metadata[:8]);assert self.allocations==[width*height]
        assert bytes(mu.mem_read(0x5b5ddc,20))==context and bytes(mu.mem_read(POOL+2500,50))==before
        return metadata.hex(),bytes(mu.mem_read(BUFFER,width*height)).hex()


def main():
    """추가 가상 분기의 새 원문 생성과 SHA 감사 중 하나를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 원문을 새 계산 결과로 덮어쓰지 않는다.
        for name,wanted in report['sha256'].items():assert digest(ROOT/name)==wanted,name
        rows=[row.split('\t') for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==report['rows']==600
        assert [(int(row[0]),int(row[1]),int(row[2])) for row in rows]==inputs();print('다리 extra2 전체600캡처 SHA 감사');return
    oracles=[Oracle(),Oracle()];rows=[];previous={tuple(row.split('\t')[1:4]):tuple(row.split('\t')[4:]) for row in (ROOT/'cpppj/tests/fixtures/bridge-collapse-x86.tsv').read_text('utf-8').splitlines() if row.startswith('C\t')}
    # 고유600입력의 실제 추가 그림 출력은 이전 입력의 plain 결과와도 각각 비교한다.
    for frame,owner,color in inputs(args.smoke):
        outputs=[oracle.extra_capture(frame,owner,color,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1]==previous[(str(frame),str(owner),str(color))];rows.append((str(frame),str(owner),str(color),*outputs[0]))
    if args.smoke:print('다리 extra2 실제 getter/추가 그림·양 끝 프레임 smoke 정상 반환');return
    assert all(oracle.calls['004222f0']==oracle.calls['004223c0']==600 for oracle in oracles)
    FIXTURE.write_text('# frame owner color metadata16 pixels; actual extra2/debugLife0\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originals/d/_shapes.shp','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_owner_capture.py','tools/decomp_bridge_collapse.py','tools/decomp_bridge_extra_capture.py','cpppj/tests/fixtures/bridge-collapse-x86.tsv',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',unit=180,rows=600,normalReturns=sum(oracle.returns for oracle in oracles),instructions=sum(oracle.instructions for oracle in oracles),precisionDifferences=0,debugLife=0,sameAsPrevious600=True,nativeCalls=dict(oracles[0].calls),sha256={name:digest(ROOT/name) for name in paths},limits=['실제4222f0의 현재 DWORD 프레임 반환/4223c0의 debug0 ret8과 전체 capture/mapped RLE를 실행한다. 두 가상 그림 대체0이다.','현재 extra2/기준점0/단위 배율/owner0·9색/60프레임의600입력이며 이전603행과 별개로 동결한다.','debugLife5453ac의 PE 기본0만 확인했다. 개발용 수명 숫자 표시1의 글꼴/출력/CD판은 새 증명 범위가 아니다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('다리 추가 캡처 독립 원본',600,report['normalReturns'],report['instructions'])


if __name__=='__main__':main()
