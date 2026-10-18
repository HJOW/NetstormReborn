#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 원본의 실제 썬 캐논 SHP로00469190 전체 사정선 계산을 실행한다."""
import argparse,csv,itertools,json,socket,struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,TYPES,TEXT,STACK,STOP,fbits,digest
from cpp_assets_smoke import read_data
from taff import TaffArchive
from shp import read_blocks
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

SHAPE,OUTPUT=TEXT+0x1000,TEXT+0x6000 # 실제 프레임 표/헤더와 독립 결과 버퍼다.
ENTRIES=(0x469190,0x469120,0x49a840,0x49ace0,0x4d4c60,0x419850,0x41d180,0x40e880,0x40e8f0,0x40e800,0x41d0d0,0x41d3e0,0x41d3f0,0x41d2d0,0x4e49c0) # 계산과 TYPE/SHP getter를 모두 실제 실행한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/cannon-asset-ray-x86.tsv' # 제품 수정 전에 동결하는 실제 자산 원문이다.
REPORT=ROOT/'cpppj/cannon-asset-ray.json' # 실행·현재 자산·경계를 명시한다.


def inputs():
    """실제 실패 좌표/대체 후보/소수·지도 양 경계와8방향을 모두 보존한다."""
    return [(fbits(x),fbits(y),direction) for x,y in ((118,115),(118,118),(118,120),(50.75,52.5),(2.5,2.5),(250.5,250.5)) for direction in range(8)]


class Oracle(MemoryOracle):
    """원본 TYPE 입력과 보호 SHP 헤더를 실제 getter에 공급하며 계산을 대체하지 않는다."""
    def __init__(self):
        """전체 실제 함수 범위와 자산의 원문36바이트를 준비한다."""
        super().__init__();rows={int(r['entry'],16):r for r in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')};self.allowed=[];self.returns=0;self.calls=Counter()
        # 실제 함수의 불연속 구간만 허용한다.
        for entry in ENTRIES:
            for part in rows[entry]['ranges'].split(';'):
                lo,hi=(int(n,16) for n in part.split('-'));self.allowed.append((lo,hi+1))
        self.shapeBytes=read_data(ROOT/'originals',TaffArchive(str(ROOT/'originals/netstorm.tarc')),'d/_shapes.shp');block=read_blocks(self.shapeBytes)[128-70]
        self.put(0x59ab20,TYPES);typ=TYPES+128*500;self.put(typ+0x44,fbits(20));self.put(typ+0x98,3);self.put(typ+0x9c,1);self.put(typ+0x1dc,0);self.put(typ+0x1e0,6);self.put(typ+0xdc,SHAPE);self.put(typ+0x114,len(block.frames)//2);self.put(0x541258,128);self.put(0x541278,136);self.put(0x59a92c,fbits(16));self.put(0x59a930,fbits(11));self.put(0x531928,256);self.put(0x54daa4,0)
        # 모든 물리 헤더를 그림/포구 값 구별 없이 원문 그대로 복사한다.
        for index,frame in enumerate(block.frames):
            header=SHAPE+0x1000+index*64;self.put(SHAPE+8+index*8,header-SHAPE+36);self.mu.mem_write(header,self.shapeBytes[frame.offset-36:frame.offset])

    def instruction(self,mu,address,size,data):
        """계산/getter/CRT를 실제 실행하며 알려지지 않은 외부 경계는 즉시 거부한다."""
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if any(lo<=address and address+size<=hi for lo,hi in self.allowed):self.instructions+=1;return
        raise RuntimeError(f'실제 자산 사정선 범위 밖 실행 {address:08x}')

    def sample(self,values,precision):
        """좌표/방향 입력의 전체4DWORD 반환과 레지스터/FPU 균형을 관찰한다."""
        x,y,direction=values;mu=self.mu;query=[x,y,fbits(struct.unpack('<f',struct.pack('<I',x))[0]-1.5),fbits(struct.unpack('<f',struct.pack('<I',y))[0]-1.5),128,0,1,0];saved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        # 비휘발과 x87의 명시 입력을 독립 호출마다 다시 설치한다.
        for register,value in saved.items():mu.reg_write(register,value)
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,0),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,precision),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):mu.reg_write(register,value)
        mu.mem_write(OUTPUT,bytes(16));mu.mem_write(STACK,struct.pack('<11I',STOP,OUTPUT,*query,direction));mu.emu_start(0x469190,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4 and all(mu.reg_read(r)==v for r,v in saved.items()) and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        self.returns+=1;return ','.join(map(str,self.words(OUTPUT,4)))


def main():
    """관측 생성과 기존 원문 SHA 검사를 구분한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 검증 중 원본 기대값을 생성하거나 덮어쓰지 않는다.
        for name,wanted in report['sha256'].items():assert digest(ROOT/name)==wanted,name
        assert len([r for r in FIXTURE.read_text('utf-8').splitlines() if r and not r.startswith('#')])==report['rows']==len(inputs());print('실제 캐논 SHP 사정선 SHA/원문 감사',report['rows']);return
    oracle=Oracle();rows=[]
    # 두 정밀도의 실제 기계어 결과가 다르면 사례를 삭제하지 않고 멈춘다.
    for values in (inputs()[:2] if args.smoke else inputs()):
        first=oracle.sample(values,0x27f);second=oracle.sample(values,0x37f);assert first==second,values;rows.append(','.join(map(str,values))+'\t'+first)
    if args.smoke:print('실제 자산 사정선 정상 반환',oracle.returns);return
    FIXTURE.write_text('# xBits yBits direction / actual rectangle bits; TYPE128 range20 theme3 hotspot0,6 scale16,11\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    files=['originals/Netstorm.exe','originals/netstorm.tarc','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/cpp_assets_smoke.py','tools/shp.py','tools/taff.py','tools/decomp_cannon_asset_ray.py',FIXTURE.relative_to(ROOT).as_posix()]
    # 실제 loose 자산을 사용했다면 그 파일도 원문 SHA에 포함한다.
    if (ROOT/'originals/d/_shapes.shp').exists():files.append('originals/d/_shapes.shp')
    report=dict(unit=185,host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',rows=len(rows),normalReturns=oracle.returns,instructions=oracle.instructions,precisionDifferences=0,nativeCalls=dict(oracle.calls),shapeSha256=__import__('hashlib').sha256(oracle.shapeBytes).hexdigest(),sha256={name:digest(ROOT/name) for name in files},limits=['실제 자산은 보호SHP의 전체36바이트이며 TYPE128/range20/theme3/hotspot0,6/scale16,11은 명시 입력이다. 계산·TYPE/SHP getter·CRT를 실행하고 외부 계산 대체는 없다.','CD10.72/전체 전투/미션 자연 완주 증거는 아니다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('실제 자산 사정선 독립 원본',len(rows),oracle.returns,oracle.instructions)


if __name__=='__main__':main()
