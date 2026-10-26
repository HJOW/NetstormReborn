# 원본 10.78의 전체 다리 품질 변경을 OS/게임 창 없이 독립 CPU로 관측한다.
import argparse, csv, hashlib, io, json, struct, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent  # 원본과 읽기 전용 정밀 내보내기를 찾는 저장소다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_oracle import Oracle, SCRATCH
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_ECX

POOL = 0x12000000  # 원본 24000개 슬롯을 담는 격리된 가상 주소다.
CAPACITY = 24000  # 실제 PATCH 풀 범위를 사용한다.
BRIDGE = 82  # 명시 입력의 현재 다리 TYPE이다.
HELPERS = [0x40eb40,0x421db0,0x421e60,0x421eb0,0x421bd0,0x4214a0,0x4ac550,0x4ad680,0x49a840,0x49a9a0,0x49a9e0,0x49aa90,0x4558c0]  # 조회/변경/후보/난수는 실제 원문 몸체다.
CODES = bytes([65,80,1,0,65,80,11,32,65,80,20,64,65,80,21,96,66,80,1,0,66,80,11,32,66,80,20,64,67,80,1,0])  # 하드 후보 중첩과 후보 부재를 포함하는 입력 표다.

class BridgeOracle(Oracle):
    """검토한 전체 다리 순회/품질/후보/난수의 실제 기계어와 세 외부 경계만 실행한다."""
    # 외부 할당·해제·가상 갱신만 기록하고 원문 전체 순회와 하위 품질 변경은 실제 실행한다.
    def __init__(self):
        super().__init__('originals',[dict(name='All',originals='00422a70')],HELPERS)
        self.mu.mem_map(0,0x1000)
        self.mu.mem_map(POOL, (CAPACITY*50+4095)&~4095)
        self.trace = []  # 실제 가상 +88 호출 때의 SID/WORD/프레임 관측이다.
        self.stubs = {0x4e4391:'bridge-new',0x4e4354:'bridge-free',0x4ad500:'bridge-redraw'}  # 이 세 경계 외 CRT/진단/OS를 암묵 대체하지 않는다.
        self.instructions = 0  # 한 호출의 실제 CPU 명령 hook 횟수다.
        self.entered = {}  # 허용 목록과 구별하는 실제 함수 진입 횟수다.
        self.allocations = 0  # 후보 버퍼1024BYTE를 실제 요청한 횟수다.
        self.frees = 0  # 같은 버퍼를 실제 반납한 횟수다.
        self.heap_live = False  # 단일 명시 후보 버퍼의 중복 할당/반납 오류를 검사한다.
        self.function_entries = {0x422a70,*HELPERS}  # 원문 함수 시작점만 실제 진입으로 센다.

    # 원문의 직접 가상/CRT 경계만 호스트 없는 메모리 장치로 반환한다.
    def on_instruction(self, mu, address, size, data):
        self.instructions += 1
        if address in self.function_entries:
            self.entered[address] = self.entered.get(address,0)+1
        kind = self.stubs.get(address)
        if kind not in ('bridge-new','bridge-free','bridge-redraw'):
            return super().on_instruction(mu,address,size,data)
        stack = mu.reg_read(UC_X86_REG_ESP)  # 현재 원본 호출 스택이다.
        ret, arg = struct.unpack('<II',mu.mem_read(stack,8))
        if kind == 'bridge-new':
            if arg != 1024: raise RuntimeError('예상 밖 후보 할당')
            if self.heap_live: raise RuntimeError('후보 버퍼 중복 할당')
            self.heap_live=True;self.allocations+=1
            mu.reg_write(UC_X86_REG_EAX,SCRATCH+0x18000)
        elif kind == 'bridge-free':
            if not self.heap_live or arg!=SCRATCH+0x18000: raise RuntimeError('실제 후보 버퍼 아닌 반납')
            self.heap_live=False;self.frees+=1
        elif kind == 'bridge-redraw':
            sid = (mu.reg_read(UC_X86_REG_ECX)-POOL)//50  # 실제 this를 SID로 관측한다.
            raw = mu.mem_read(POOL+sid*50,50)
            self.trace.append(f'{sid}:{struct.unpack_from("<H",raw,12)[0]}:{struct.unpack_from("<I",raw,36)[0]}')
        mu.reg_write(UC_X86_REG_ESP,stack+4)
        mu.reg_write(UC_X86_REG_EIP,ret)

    # 기대값을 계산하지 않고 명시 입력 바이트/seed를 새로 설치한다.
    def prepare(self, seed, pattern):
        slots = bytearray(CAPACITY*50)  # 입력 전체 풀의 free 비트를 명시한다.
        for sid in range(CAPACITY): slots[sid*50+11]=1  # 원문 iterator가 free만 건너뛰게 한다.
        for sid, frame, state, kind in [(1,0,4,BRIDGE),(5,0,4,BRIDGE),(6,1,6,BRIDGE),(7,2,4,BRIDGE),(8,4,4,BRIDGE),(9,7,4,BRIDGE),(10,0,4,70),(15000,3,4,BRIDGE)]:
            offset=sid*50
            struct.pack_into('<I',slots,offset,0x5034c8)
            slots[offset+10]=kind;slots[offset+11]=state
            struct.pack_into('<H',slots,offset+12,0xa5ff ^ pattern)
            struct.pack_into('<I',slots,offset+36,frame)
        self.mu.mem_write(POOL,bytes(slots))
        for address,value in [(0x5c8464,POOL),(0x5c847c,CAPACITY),(0x5411a0,BRIDGE),(0x59ab20,SCRATCH),(0x532710,seed),(0x5e4794,0),(0x52f960,5)]:
            self.mu.mem_write(address,struct.pack('<I',value))  # 실제 원문이 조회하는 같은 전역 주소다.
        table=SCRATCH+BRIDGE*500
        self.mu.mem_write(table+0x114,struct.pack('<I',len(CODES)//4))
        self.mu.mem_write(table+0x124,struct.pack('<I',SCRATCH+0x17000))
        self.mu.mem_write(SCRATCH+0x17000,CODES)
        self.trace=[];self.instructions=0;self.entered={};self.allocations=0;self.frees=0;self.heap_live=False

    # 전체 순회 뒤 관심 슬롯의 모든50BYTE와 현재 난수를 독립 출력한다.
    def observe(self):
        return ';'.join(f'{sid}:{bytes(self.mu.mem_read(POOL+sid*50,50)).hex()}' for sid in (1,5,6,7,8,9,10,15000))

    # 내보내기 범위와 실제 PE 코드 바이트를 함께 기록하며 기대값 모델을 추가하지 않는다.
    def code_evidence(self):
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as source:
            facts={int(row['entry'],16):row for row in csv.DictReader(source,delimiter='\t')}
        output=[]  # 같은 순서의 실제 몸체 범위/바이트 SHA다.
        for address in sorted(self.function_entries):  # 비연속 함수 조각도 내보내기 순서대로 읽는다.
            ranges=[];body=bytearray()
            for part in facts[address]['ranges'].split(';'):  # 범위 끝은 functions.tsv의 inclusive 끝이다.
                first,last=(int(value,16) for value in part.split('-'))
                ranges.append(f'{first:08x}-{last:08x}');body.extend(self.mu.mem_read(first,last-first+1))
            output.append(dict(entry=f'{address:08x}',ranges=ranges,bytes=len(body),sha256=hashlib.sha256(body).hexdigest()))
        return output

def main():
    # 제품 코드보다 먼저 만든 독립 기대값을 신규 fixture에만 쓴다. 원본/기존 fixture는 덮지 않는다.
    parser=argparse.ArgumentParser(description='게임/창 없는 원본 전체 다리 품질 CPU 관측')
    parser.add_argument('--verify',action='store_true',help='기존 fixture와 새 CPU 관측을 비교하며 fixture는 쓰지 않는다')
    options=parser.parse_args()
    target=ROOT/'cpppj/tests/fixtures/editor-bridges-x86.tsv'
    if target.exists() and not options.verify: raise RuntimeError('기존 독립 관측을 덮어쓸 수 없습니다')
    if options.verify and not target.exists(): raise RuntimeError('재현 비교할 기존 fixture가 없습니다')
    oracle=BridgeOracle();rows=[];runs=[];code_evidence=oracle.code_evidence()
    for mode in (0,1,2,3,0xffffffff):  # 원문 미지원 정수의 무효 동작도 보존한다.
        for seed in (1,0x12345678,0xffffffff):  # 후보 선택이 같은 실제 난수를 소비하는지 확인한다.
            for pattern in (0,0x5a78):  # 수명 비트 밖의 보존을 원시 WORD로 대조한다.
                oracle.prepare(seed,pattern);oracle.call('All',[mode])
                if oracle.heap_live or oracle.allocations!=oracle.frees: raise RuntimeError('후보 버퍼 반납 불일치')
                rows.append((mode,seed,pattern,oracle.observe(),';'.join(oracle.trace),struct.unpack('<I',oracle.mu.mem_read(0x532710,4))[0]))
                runs.append(dict(mode=mode,seed=seed,pattern=pattern,instructions=oracle.instructions,allocations=oracle.allocations,frees=oracle.frees,redraws=len(oracle.trace),normalReturn=True,stackRestored=True,functionEntries={f'{address:08x}':count for address,count in sorted(oracle.entered.items())}))
    output=io.StringIO(newline='')  # 기존 fixture와 생성 결과의 전체UTF-8 바이트를 비교한다.
    writer=csv.writer(output,delimiter='\t',lineterminator='\n');writer.writerow(('mode','seed','pattern','slots','trace','random'));writer.writerows(rows)
    encoded=output.getvalue().encode('utf-8')  # 파일에 쓰거나 비교할 동일 바이트다.
    if options.verify:
        if target.read_bytes()!=encoded: raise RuntimeError('기존 fixture와 새 원본 CPU 관측의 바이트 불일치')
    else:
        target.write_bytes(encoded)
    if oracle.code_evidence()!=code_evidence: raise RuntimeError('실행 중 허용된 원본 코드 바이트 변경')
    executed=sorted({address for run in runs for address in run['functionEntries']})  # 실제 호출되지 않은 허용 helper를 실행 근거에 합산하지 않는다.
    report=dict(host='HJOW-Athlon',lastFullDecompilation='HJOW-Athlon / 2026-10-23',originalGameExecuted=False,rows=len(rows),binarySha256=oracle.sha256,fixtureSha256=hashlib.sha256(encoded).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),functionsTsvSha256=hashlib.sha256((ROOT/'extracted/refined/originals/functions.tsv').read_bytes()).hexdigest(),codeEvidence=code_evidence,codeImageUnchanged=True,input=dict(pool=POOL,capacity=CAPACITY,stride=50,bridgeType=BRIDGE,codesHex=CODES.hex(),codesSha256=hashlib.sha256(CODES).hexdigest(),observedSids=[1,5,6,7,8,9,10,15000],nativeMaximumInstructions=200000,nativeTimeoutMicroseconds=500000),actualFunctions=executed,allowedFunctions=[f'{address:08x}' for address in sorted(oracle.function_entries)],replaced=['operator_new(1024)','free','virtual+88'],normalReturns=len(runs),verifiedExistingFixture=options.verify,runs=runs)
    (ROOT/'cpppj/editor-bridges-native-20261026.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({key:report[key] for key in ('host','rows','normalReturns','fixtureSha256','toolSha256','codeImageUnchanged','verifiedExistingFixture')},ensure_ascii=False))

if __name__=='__main__': main()  # 정적 PE 몸체만 실행하며 실제 게임이나 OS API는 실행하지 않는다.
