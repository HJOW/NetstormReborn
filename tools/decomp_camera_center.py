#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""497f20의 전체 Renderer 보존·목표 좌표·공유 카운터를 격리 PE에서 관측한다."""
import argparse,csv,hashlib,itertools,json,platform,struct
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,STACK,STOP
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPTAG,UC_X86_REG_FPSW

ENTRY=0x497f20  #10.78의 실제 world float 중심→pending 픽셀 목표 본체다.
FIELDS=('width','height','xBits','yBits','revision','rendererBefore','snapshotBefore','rendererAfter','snapshotAfter','revisionAfter')  # 기대 좌표식 없이 실제 전후 바이트를 보존한다.


def sha(path):
    """현재 원천/관측 파일의 바이트 해시를 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class Oracle(MemoryOracle):
    """원문과 FTOL만 실행하며 Renderer/전역 쓰기 범위를 매 명령에서 감사한다."""
    def __init__(self):
        """현재 호스트의 전체 정밀 범위 두 개만 허용한다."""
        super().__init__();self.allowed=[];self.done=False;self.writes=[]  # 허용 명령 구간·정상 반환 여부·스택 밖 실제 쓰기 순서를 각각 보관한다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as source:
            facts={int(row['entry'],16):row for row in csv.DictReader(source,delimiter='\t')}
        for entry in (ENTRY,0x4e49c0):  # 좌표 곱/절삭도 원문 명령을 사용한다.
            for part in facts[entry]['ranges'].split(';'):  # 분리된 실제 구간 사이의 명령은 실행하지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def instruction(self,mu,address,size,data):
        """정상 ret8과 미등록/외부 함수 진입을 구별한다."""
        if address==STOP: self.done=True;mu.emu_stop();return
        assert any(low<=address and address+size<=high for low,high in self.allowed),hex(address)
        self.instructions+=1

    def write(self,mu,access,address,size,value,data):
        """스택 외에는 두 목표 DWORD와 실제 revision만 쓰도록 제한한다."""
        if STACK-0x1f000<=address and address+size<=STACK+0x1000: return
        assert (POOL+0x50<=address and address+size<=POOL+0x58) or (0x5c85dc<=address and address+size<=0x5c85e0),(hex(address),size)
        self.writes.append((address,size,value&((1<<(size*8))-1)))

    def run(self,case,control):
        """입력 Renderer160BYTE/전역 snapshot64BYTE를 서로 다른 바이트로 채운다."""
        width,height,x,y,revision,seed=case;renderer=bytes((index*37+seed)&255 for index in range(160));snapshot=bytes((index*13+seed+71)&255 for index in range(64))
        self.mu.mem_write(POOL,renderer);self.mu.mem_write(0x59a900,snapshot)
        for address,value in ((0x5c78f0,width),(0x5c78f4,height),(0x5c85dc,revision)): self.put(address,value)  # 함수의 현재 signed 화면/카운터 입력이다.
        self.mu.mem_write(STACK,struct.pack('<3I',STOP,x,y));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,POOL)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.writes=[];self.done=False;self.instructions=0;self.mu.emu_start(ENTRY,STOP+1,count=500)
        assert self.done and self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+12 and self.mu.reg_read(UC_X86_REG_FPCW)==control
        after=bytes(self.mu.mem_read(POOL,160));current=bytes(self.mu.mem_read(0x59a900,64));out=self.words(0x5c85dc,1)[0]
        assert after[:0x50]==renderer[:0x50] and after[0x58:]==renderer[0x58:] and current==snapshot
        assert [(address,size) for address,size,value in self.writes]==[(POOL+0x50,4),(POOL+0x54,4),(0x5c85dc,4)]
        return [width,height,f'{x:08x}',f'{y:08x}',revision,renderer.hex(),snapshot.hex(),after.hex(),current.hex(),out]


def inputs():
    """좌표식과 무관한 정수/분수/부호0/NaN/무한/FTOL 경계 비트들을 조합한다."""
    coordinates=(0,0x80000000,0x3e000000,0xbe000000,0x42c88000,0xc2c88000,0x43800000,0x4effffff,0x5c800000,0x5d000000,0x7f800000,0xff800000,0x7fc12345,0x7f7fffff)
    return [(w,h,x,coordinates[(index*5+3)%len(coordinates)],revision,(index*19+w)&255)
        for (w,h),revision,(index,x) in itertools.product(((640,480),(801,601),(1024,768),(1280,720),(1280,800)),(0,0xffffffff),enumerate(coordinates))]  # 화면/카운터와 좌표 선택은 독립 입력이다.


def main():
    """서로 다른 x87 정밀도의 실제 결과가 같을 때만 새 파일로 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',required=True,type=Path);parser.add_argument('--report',required=True,type=Path);args=parser.parse_args();oracle=Oracle();rows=[]
    for case in inputs():  # 두 정밀도마다 정상 반환/전체 불변 바이트를 독립 관측한다.
        first=oracle.run(case,0x27f);second=oracle.run(case,0x37f);assert first==second;rows.append(first)
    assert len(rows)==140 and len({tuple(row[:7]) for row in rows})==140
    with args.fixture.open('x',encoding='utf-8',newline='\n') as output: output.write('# '+'\t'.join(FIELDS)+'\n');output.writelines('\t'.join(map(str,row))+'\n' for row in rows)
    report={'host':platform.node(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','entry':'00497f20','inputs':140,'runs':280,'differences':0,'externalCallbacks':0,'executedBodies':['00497f20','004e49c0'],'preserved':'Renderer160BYTE의50..57 외 전부와 별도snapshot64BYTE','writes':'목표X→목표Y→005c85dc++','sha256':{path.relative_to(ROOT).as_posix():sha(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'tools/decomp_camera_center.py',ROOT/'tools/decomp_priestmenu.py',args.fixture.resolve())},'scope':'격리 원문 함수이며 실제 게임/OS/커서/미션 진행/MP 실행은 없다.10.72는 정적458990→458800의 다른store 순서를 문서화하며 이번 fixture로 판본 동일성을 주장하지 않는다.'}
    with args.report.open('x',encoding='utf-8',newline='\n') as output: json.dump(report,output,ensure_ascii=False,indent=2);output.write('\n')
    print(json.dumps({'inputs':140,'runs':280,'differences':0}),flush=True)


if __name__=='__main__': main()  # 게임을 시작하지 않는 격리 분석이다.
