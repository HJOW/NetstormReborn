# -*- coding: utf-8 -*-
"""10.78 즉시 중심 이동:433aa0→497f20→497960 전체 호출과 공유 전역/현재 장치를 격리 관찰한다."""
import argparse,csv,hashlib,itertools,json,platform,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 보호 PE와 독립 지원 도구의 기준 경로다.
sys.path.insert(0,str(ROOT/'tools'))
from decomp_camera_commit import Oracle as CommitOracle,FIELDS,GLOBAL_FIELDS,views,dirty,ROOT,POOL,STACK,STOP,bits
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ECX,UC_X86_REG_FPCW,UC_X86_REG_FPTAG,UC_X86_REG_FPSW


class ImmediateOracle(CommitOracle):
    """계산과 실제 commit은 모두 원문이며 기존 OS/Gump/dirty 추가 경계만 관찰한다."""
    def __init__(self):
        """기존 commit 허용 범위에 실제 부모/목표/FTOL 세 함수만 추가한다."""
        super().__init__();self.parentTrace=[]  # 목표 진입/실제 commit 진입 시 현재 전역의 독립 관측이다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as source:
            facts={int(row['entry'],16):row for row in csv.DictReader(source,delimiter='\t')}
        for entry in (0x433aa0,0x497f20,0x4e49c0):  # 기존 callback 경계 앞의 실제 기계어를 실행한다.
            for part in facts[entry]['ranges'].split(';'):  # 빈 구간이나 다음 함수를 허용하지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """부모가 목표/적용에 들어간 바로 그 시점의 전체 카운터를 읽는다."""
        if address in (0x497f20,0x497960):
            self.parentTrace.append([address,*self.words(POOL+0x50,2),self.words(0x59a8b0,1)[0],self.words(0x5c85dc,1)[0]])
        super().instruction(mu,address,size,data)

    def run(self,case,center,revision,control):
        """기존 독립 commit 입력과 두 float 비트를 실제 cdecl 부모에 공급한다."""
        self.case=case;self.events=[];self.mutated=set();self.instructions=0;self.completed=False;self.parentTrace=[]
        self.mu.mem_write(POOL,bytes(0x900));first,second=views(case)
        self.mu.mem_write(POOL,struct.pack('<16I',*(value&0xffffffff for value in first)));self.mu.mem_write(0x59a900,struct.pack('<16I',*(value&0xffffffff for value in second)))
        for offset,key in ((0x50,'targetX'),(0x54,'targetY'),(0x58,'targetZoom'),(0x5c,'redraw')): self.put(POOL+offset,case[key])  # 기존 목표/배율/전체 dirty를 현재뷰와 구별한다.
        self.put(POOL+0x48,480);self.put(POOL+0x4c,490);self.put(0x59a8b0,case['worldRedraw']);self.put(0x5c85dc,revision);self.put(0x59a8bc,POOL)
        for address,key in GLOBAL_FIELDS: self.put(address,case[key])  # 입력 전역의 현재 주소를 그대로 사용한다.
        self.mu.mem_write(0x5ca9cc,struct.pack('<4I',3,4,997,701));entries=dirty(case['dirtyProfile']);self.put(POOL+0x64,len(entries))
        for index,entry in enumerate(entries):  # 실제 사각형/flags의 분리된 배열 입력이다.
            for field,value in enumerate(entry[:4]): self.put(POOL+0x68+index*16+field*4,value)  # 좌표 산술 결과를 미리 계산하지 않는다.
            self.put(POOL+0x6a8+index*4,entry[4])
        self.mu.mem_write(STACK-256,bytes(320));self.mu.mem_write(STACK,struct.pack('<3I',STOP,*center))
        for register,value in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_ECX,0),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_FPSW,0)): self.mu.reg_write(register,value)  # 부모는 ECX가 아닌 현재 Renderer 전역을 사용한다.
        self.mu.emu_start(0x433aa0,STOP+1,count=100000);assert self.completed and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4 and self.mu.reg_read(UC_X86_REG_FPCW)==control
        count=self.words(POOL+0x64,1)[0];current=[(*self.words(POOL+0x68+index*16,4),self.words(POOL+0x6a8+index*4,1)[0]) for index in range(count)]  # 실제 원문이 갱신한 표만 읽는다.
        assert len(self.parentTrace)==2 and self.parentTrace[0][0]==0x497f20 and self.parentTrace[1][0]==0x497960
        return {'renderer':self.words(POOL,16),'snapshot':self.words(0x59a900,16),'target':self.words(POOL+0x48,6),'dirty':current,'worldRedraw':self.words(0x59a8b0,1)[0],'globals':tuple(self.words(address,1)[0] for address,_ in GLOBAL_FIELDS)+(self.words(0x59a8b0,1)[0],),'revision':self.words(0x5c85dc,1)[0],'parentTrace':self.parentTrace,'events':self.events,'instructions':self.instructions}


def encode(values):
    """실제 관측 DWORD를 계산 없이 콜론으로 직렬화한다."""
    return ':'.join(str(value&0xffffffff) for value in values)


def sha(path):
    """실제 원천/도구/fixture의 바이트 SHA를 고정한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """같은 원문 부모/하위를 두 x87 정밀도로 관측하고 새 파일에만 쓴다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',required=True,type=Path);parser.add_argument('--report',required=True,type=Path);args=parser.parse_args()
    base=dict(zip(FIELDS,(100,200,701,702,65536,65536,65536,16,11,32,22,2,15,0,28,1024,740,1024,768,99,88,17,19,1,0,317,419,32768)));oracle=ImmediateOracle();rows=[]
    for center,mode,zoom,mutation,counter in itertools.product(((bits(100.25),bits(101.75)),(bits(0),bits(0)),(0x7fc12345,0x7f800000)),(2,18),(65536,98304),(0,1,2,3,4,5,6),(0,0xffffffff)):  # 정상/음수목표/NaN·DIB/배율/현재callback 입력을 독립 조합한다.
        case=dict(base,flags=mode,targetZoom=zoom,mutation=mutation,worldRedraw=counter);first=oracle.run(case,center,counter,0x27f);second=oracle.run(case,center,counter,0x37f);assert first==second
        value=first;currentGlobals=value['globals']  # callback 뒤 실제 전역11개까지 두 정밀도에서 함께 대조한 관측이다.
        rows.append([*(case[key] for key in FIELDS),*center,counter,encode(value['renderer']),encode(value['snapshot']),encode(value['target']), '|'.join(encode(entry) for entry in value['dirty']) or '-',encode(currentGlobals),';'.join(value['events']) or '-',value['revision'],encode(value['parentTrace'][1][1:])])
    assert len(rows)==168
    with args.fixture.open('x',encoding='utf-8',newline='\n') as output:
        output.write('# '+'\t'.join((*FIELDS,'centerXBits','centerYBits','revisionBefore','renderer16','snapshot16','target6','dirty5','globals11','events','revisionAfter','commitEntry4'))+'\n');output.writelines('\t'.join(map(str,row))+'\n' for row in rows)
    report={'host':platform.node(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','entry':'00433aa0','inputs':168,'observations':336,'differences':0,'originalGameExecutions':0,'executedBodies':['433aa0','497f20','4e49c0','497960','497490','4834c0'],'boundaries':'기존 CommitOracle의 커서 잠금/해제·Gump redraw·Screen/배경 scroll·dirty 추가·마우스/진단만 외부 입력이다. 부모/목표/카운터/두 setter/dirty확장은 실제 원문이다.','scope':'격리10.78 함수/합성장치 입력이며 실제 게임·승패·MP 실행은 없다.10.72는 별도 부모/store순서 분석 전 공통 완료로 세지 않는다.','sha256':{path.relative_to(ROOT).as_posix():sha(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'tools/decomp_camera_immediate.py',ROOT/'tools/decomp_camera_commit.py',ROOT/'tools/decomp_priestmenu.py',args.fixture.resolve())}}
    with args.report.open('x',encoding='utf-8',newline='\n') as output: json.dump(report,output,ensure_ascii=False,indent=2);output.write('\n')
    print(json.dumps({'inputs':168,'observations':336,'differences':0}),flush=True)


if __name__=='__main__': main()  # 원본 게임을 시작하지 않는 격리 함수 관측이다.
