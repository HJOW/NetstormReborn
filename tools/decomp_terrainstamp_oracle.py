#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 초기 지형 프레임·99 CRTcache·stamp와 지역 직접 쓰기를 두 PE에서 독립 실행한다."""
import argparse
import collections
import csv
import functools
import hashlib
import itertools
import json
import struct
from pathlib import Path
from decomp_regionpaint_oracle import PaintOracle,ROOT,TYPES,POOL,CODES,HEAP,STOP,STACK_BASE,STACK,digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 전체 원본 지도·CRT thread·가상 호출 경계는 에뮬레이터 내부에만 존재한다.
MAP,THREAD,OWNER_EFFECT,POP_EFFECT,VTABLE=0x1b000000,0x1c000000,STOP+0x1800,STOP+0x1810,STOP+0x1f00
# 실제 보호 PE의 초기 몸체·현재 공유 전역과 OS/하위 객체 경계다.
SPECS={
 'originals':dict(frame=0x4c04b0,stamp=0x4c0640,offset=8,bounds=0x20820,context=0x20808,
    cache=0x5c8be4,rng=0x532710,allocate=0x4e4391,free=0x4e4354,thread=0x4ea61d,
    factory=0x4af530,isle=0x5411dc,edge=0x5411e0,fringe=0x5411f0,region=0x4acd20,
    assert_entry=0x4e0620,chunk=0x4322e0,helpers=()),
 'originalCD':dict(frame=0x49cf20,stamp=0x49d2c0,offset=4,bounds=0x2081c,context=0x20804,
    cache=0x537190,rng=0x5308ec,allocate=0x4f1650,free=0x4f1a50,thread=0x4f6010,
    factory=0x4ab390,isle=0x51cac8,edge=0x51cacc,fringe=0x51cadc,region=0x4aca80,
    assert_entry=0x490040,chunk=0x404350,helpers=(0x43f360,0x43f3b0,0x43f400,0x48cbd0,0x4446a8,0x407c40,0x407c50,0x407c91,0x4abf50,0x4abae0,0x4aca80))
}
# 현재 타입/코드 입력과 실제 지도 이웃 순서를 양 판본에 똑같이 준다.
ISLE,EDGE,FRINGE,OTHER=97,98,102,94
NEIGHBORS=((0,-1),(1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1))
CONTROLS=(0x027f,0x037f)
FIXTURE=ROOT/'cpppj/tests/fixtures/terrainstamp-x86.tsv'
REPORT=ROOT/'cpppj/terrainstamp.json'


def inputs():
    """기대 프레임을 계산하지 않고 원본 지도/두 난수/99cache/타입 코드를 교차한다."""
    cases=[]
    # 직선·대각선의 모든 mask는 code alias와 같은 지역BYTE를 실제 helper가 읽는다.
    for straight,diagonal in itertools.product(range(16),range(16)):
        cases.append(('Frame',(120,straight,diagonal,50,53,0,0,0,0,0)))
    # AA 좌표 잔여/99cache 순환·첫/기존 cache와 비AA 코드/지역 불일치를 별도 입력으로 둔다.
    for code,x,y,seed,cache in itertools.product((120,61), (0,2,50,255), (0,2,53,255), (1,0xffffffff), (0,1)):
        cases.append(('Frame',(code,15,15,x,y,seed,0x12345678,0,cache,0)))
    for code,straight,diagonal,dataset,cache,change in itertools.product((120,61),(0,5,15),(0,10,15),(1,2,3),(0,1),(0,1)):
        cases.append(('Frame',(code,straight,diagonal,50,53,7,0xffffffff,dataset,cache,change)))
    # stamp의 Y외부/X내부·빈alias/mapping·경계포함·fringe retry와 두 난수의 소비를 관찰한다.
    for scene,dataset,cache,rng,excluded in itertools.product(range(5),(0,3,4),(0,1),(0,0xffffffff),(0,2,0x80000000)):
        cases.append(('Stamp',(scene,dataset,cache,3,rng,excluded,1,0)))
    return list(dict.fromkeys(cases))


@functools.lru_cache(maxsize=None)
def frame_codes(dataset,fringe=False):
    """코드 저장순서/후보 수/실패·정규화·flags1와 부호확장 fringe 입력 표를 만든다."""
    rows=[]
    if dataset==1:return []
    if dataset==4 and fringe:return []
    if dataset==4:dataset=0
    if dataset==2:return [bytes((65,65,index&255,0)) for index in range(300)]
    pairs=itertools.product(range(65,81),range(65,81)) if dataset==0 else ((65,65),(67,65),(70,65),(74,65),(80,65))
    # 같은 side/variant의8개는 패치quartile과 CD전체 선택의 차이를 관찰한다.
    for side,variant in pairs:
        for index in range(8):
            flags=(0,2,0x80,1)[index%4] if fringe else 1
            rows.append(bytes((side,variant,index,flags)))
    return rows


class StampOracle(PaintOracle):
    """실제 프레임/mask/CRT·게임PRNG/지역setter를 실행하고 객체 생성과 가상 효과를 관찰한다."""
    def __init__(self,edition):
        """현재 호스트의 정밀 범위와 보호 PE를 읽고 분리된 입력 저장소를 연결한다."""
        super().__init__(edition);self.s=SPECS[edition];self.native=collections.Counter();self.external=collections.Counter();self.returns=0
        self.mu.mem_map(MAP,0x21000);self.mu.mem_map(THREAD,4096)
        paths=[ROOT/f'extracted/terrainstamp/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as stream:
            # 좁은 정밀 내보내기가 선언한 불연속 몸체를 실제 허용 범위에 더한다.
            for row in csv.DictReader(stream,delimiter='\t'):
                for span in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in span.split('-'));self.allowed.append((low,high+1))
        helpers=set(self.s['helpers'])|({0x4acd20} if edition=='originals' else set())
        full=ROOT/f'extracted/refined/{edition}/functions.tsv';self.exports.append(full)
        with full.open(encoding='utf-8') as stream:
            rows=[row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16) in helpers]
        if len(rows)!=len(helpers):raise RuntimeError('초기stamp 현재 helper 범위 누락')
        for row in rows:
            for span in row['ranges'].split(';'):
                low,high=(int(value,16) for value in span.split('-'));self.allowed.append((low,high+1))
        self.allowed_bytes=bytearray(0x200000)
        for low,high in self.allowed:self.allowed_bytes[low-0x400000:high-0x400000]=b'\x01'*(high-low)
        self.counted_entries={self.s['frame'],self.s['stamp'],self.s['region'],0x4e64ab,0x4f23b0,0x49aa90,0x4444c0,0x4437f0,0x48ef00}
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.put(VTABLE+0x74,OWNER_EFFECT);self.put(VTABLE+0x90,POP_EFFECT)
        for name,number in (('isle',ISLE),('edge',EDGE),('fringe',FRINGE)):self.put(self.s[name],number)
        self.put(0x50c038 if edition=='originals' else 0x505eb0,127)

    def put(self,address,value,width=4):
        """명시 입력이나 격리된 효과를 little endian 실제 폭으로 기록한다."""
        self.mu.mem_write(address,(int(value)&((1<<(width*8))-1)).to_bytes(width,'little'))

    def read(self,address,width=4):
        """현재 raw 필드를 BYTE/WORD/DWORD 지정 폭으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def instruction(self,mu,address,size,data):
        """기계어 몸체를 제한하고 OS thread·할당·factory/가상 호출의 입력과 현재 raw를 관찰한다."""
        s=self.s;p=self.p;sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        if address==STOP:mu.emu_stop();return
        if address==s['thread']:
            self.external['crt_thread']+=1;self.return_from(value=THREAD);return
        if address==s['allocate']:
            count=self.read(sp+4);result=self.heap;self.heap+=(count+15)&~15
            if self.heap>HEAP+0x10000:raise RuntimeError('stamp 분석heap 범위 초과')
            self.external['allocate']+=1;self.return_from(value=result);return
        if address==s['free']:
            self.external['free']+=1;self.return_from();return
        if address==s['factory']:
            number=self.read(sp+4);predictable=self.read(sp+8);sid=self.next_sid;self.next_sid+=1
            raw=POOL+sid*p['stride'];self.mu.mem_write(raw,bytes(p['stride']));self.put(raw,VTABLE);self.put(raw+10,number,1)
            self.external['factory']+=1;self.events.append(f'C:{sid}:{number}:{predictable}');self.return_from(value=raw);return
        if address==OWNER_EFFECT:
            sid=(this-POOL)//p['stride'];owner=self.read(sp+4);self.put(this+p['owner'],owner,1)
            self.external['owner']+=1;self.events.append(f'O:{sid}:{owner}');self.return_from(4);return
        if address==POP_EFFECT:
            sid=(this-POOL)//p['stride'];x,y,flags=struct.unpack('<3I',mu.mem_read(sp+4,12))
            self.put(this+14,x);self.put(this+18,y);self.external['pop']+=1;self.events.append(f'P:{sid}:{x}:{y}:{flags}');self.return_from(12);return
        if address==s['chunk']:
            self.external['chunk']+=1;self.events.append(f'H:{self.read(sp+4)}:{self.read(sp+8)}');self.return_from(8,6);return
        if address==s['assert_entry']:
            self.assertions+=1;self.events.append('A');self.external['assert']+=1;self.return_from();return
        if not(0x400000<=address<0x600000 and self.allowed_bytes[address-0x400000]):raise RuntimeError(f'초기stamp 허용 밖 native {address:08x}, {self.case}')
        if address in self.counted_entries:self.native[f'{address:08x}']+=1
        if address==s['region']:self.events.append(f'R:{(this-POOL)//p["stride"]}:{self.read(sp+4)}')

    def write(self,mu,access,address,size,value,data):
        """원본의 스택/FS·지도mapping/context·슬롯frame/region·CRT/게임난수·heap 쓰기만 허용한다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if address==0 and size==4:return
        if MAP<=address and address+size<=MAP+0x21000:return
        if HEAP<=address and address+size<=HEAP+0x10000:return
        if THREAD+0x14<=address and address+size<=THREAD+0x18:return
        if address in (self.s['cache'],self.s['rng']) and size==4:return
        if POOL<=address and address+size<=POOL+0x10000:
            offset=(address-POOL)%self.p['stride'];frame=self.p['frame'];width=4 if self.edition=='originals' else 1
            if (offset==8 and size==2) or (frame<=offset and offset+size<=frame+width):return
        raise RuntimeError(f'초기stamp 허용 밖 쓰기 {address:08x}+{size}, {self.case}')

    def cell(self,x,y,code,region=5):
        """원본은 X열 우선 code와 지역BYTE를 읽으므로 전치하지 않고 입력한다."""
        if not(0<=x<256 and 0<=y<256):return
        index=x*256+y;self.put(MAP+self.s['offset']+index,code,1);self.put(MAP+self.s['offset']+0x10000+index,region,1)

    def run(self,kind,case,control):
        """매 독립 입력을 초기화하고 실제 반환·두 난수·99cache·외부 순서와 생성 슬롯을 수집한다."""
        self.case=(kind,case);s=self.s;p=self.p;mu=self.mu;self.events=[];self.heap=HEAP;self.next_sid=40;self.assertions=0
        mu.mem_write(MAP,bytes(0x21000));mu.mem_write(POOL,bytes(0x10000));mu.mem_write(CODES,bytes(0x10000));self.put(VTABLE+0x74,OWNER_EFFECT);self.put(VTABLE+0x90,POP_EFFECT)
        if kind=='Frame':code,straight,diagonal,x,y,seed,rng,dataset,cache,change=case
        else:scene,dataset,cache,seed,rng,excluded,predictable,change=case;x,y=50,53;code=120
        self.put(THREAD+0x14,seed);self.put(s['rng'],rng);self.put(s['cache'],HEAP+0xe000 if cache else 0)
        if cache:
            # 기존cache는 명시 입력이며 새 CRT 소비로 계수하지 않는다.
            for index in range(99):self.put(HEAP+0xe000+index*4,(index*17+3)%32768)
        for number in (ISLE,EDGE,FRINGE,OTHER):
            address=TYPES+number*p['type_stride'];mu.mem_write(address,bytes(p['type_stride']));codes=frame_codes(dataset,number==FRINGE)
            base=CODES+(0x6000 if number==FRINGE else 0x1000);mu.mem_write(base,b''.join(codes));self.put(address+0x114,len(codes));self.put(address+0x124,base)
            self.put(address+0x154,next((index for index,c in enumerate(codes) if c[0]==74),0xffffffff));self.put(address+0xec,2 if number==ISLE else 0)
        offset=s['offset'];mapping=MAP+offset+0x20000;aliases=MAP+offset+0x20400
        for char,number in ((120,ISLE),(61,EDGE),(63,OTHER),(104,ISLE)):
            self.put(mapping+char*4,number);self.put(aliases+char*4,120 if char==104 else char)
        bounds=(0,0,255,255) if kind=='Frame' else (50,53,52,55)
        for index,value in enumerate(bounds):self.put(MAP+s['bounds']+index*4,value)
        self.put(MAP+s['context'],1 if kind=='Frame' else predictable);self.put(MAP+s['context']+8,0 if kind=='Frame' else excluded)
        self.cell(x,y,code)
        if kind=='Frame':
            # 여덟 방향 입력은 실제 mask 검사에서 다시 읽으며 같은 alias h도 일치한다.
            for index,(dx,dy) in enumerate(NEIGHBORS):
                mask=diagonal if index%2 else straight;bit=index//2
                self.cell(x+dx,y+dy,104 if mask&(1<<bit) else 63,6 if change else 5)
        else:
            for dx,dy in ((1,0),(0,1),(1,1)):self.cell(x+dx,y+dy,120)
            if scene==1:self.cell(51,53,63)
            if scene==2:self.cell(50,53,104);self.put(mapping+104*4,0)
            if scene==3:self.cell(51,54,104,6)
            if scene==4:self.put(MAP+s['bounds']+8,50)
        # 실제 register/x87·복귀주소를 초기화하고 두 정밀도에서 동일 입력을 실행한다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        args=(x,y) if kind=='Frame' else (120,);entry=s['frame'] if kind=='Frame' else s['stamp']
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_ECX,MAP)
        try:mu.emu_start(entry,STOP,count=2000000)
        except Exception as error:raise RuntimeError(f'초기stamp 실행 오류 {mu.reg_read(UC_X86_REG_EIP):08x}, {self.case}') from error
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+len(args)*4:raise RuntimeError('초기stamp 정상 복귀/스택 오류')
        self.returns+=1;pointer=self.read(s['cache']);values=','.join(str(self.read(pointer+index*4)) for index in range(99)) if pointer else '-'
        raw=[]
        for sid in range(40,self.next_sid):
            address=POOL+sid*p['stride'];raw.append(':'.join(map(str,(sid,self.read(address+10,1),self.read(address+p['frame'],4 if self.edition=='originals' else 1),self.read(address+p['owner'],1),self.read(address+8,2),self.read(address+14),self.read(address+18)))))
        mappings=','.join(str(self.read(mapping+char*4)) for char in (120,88,61))
        return [mu.reg_read(UC_X86_REG_EAX) if kind=='Frame' else 0,self.read(s['rng']),self.read(THREAD+0x14),values,';'.join(self.events) or '-',';'.join(raw) or '-',self.read(MAP+s['context']+4),mappings,self.assertions]


def generate(smoke=False):
    """독립 실행 행과 PE/실제 허용 범위/실행기 SHA를 기록한다. 두 정밀도는 입력 수를 늘리지 않는다."""
    cases=inputs();cases=cases[::67] if smoke else cases;rows=[];editions={};files=[Path(__file__),ROOT/'tools/ghidra/terrainstamp-functions.json',FIXTURE]
    for edition in SPECS:
        oracle=StampOracle(edition)
        for kind,case in cases:
            output=oracle.run(kind,case,CONTROLS[0]);repeat=oracle.run(kind,case,CONTROLS[1])
            if output!=repeat:raise RuntimeError(f'초기stamp x87 정밀도 차이 {edition} {kind} {case}')
            if output[-1]:raise RuntimeError(f'초기stamp 정상 입력 assert {edition} {kind} {case}')
            rows.append([edition,kind,','.join(map(str,case)),*output])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native=dict(oracle.native),external=dict(oracle.external));files.extend(oracle.exports+[ROOT/oracle.p['binary']])
        print(f'{edition}: {len(cases)}개 정상 반환',flush=True)
    if smoke:return
    FIXTURE.write_text('# edition kind input result gameRandom crtRandom cache effects raw lastSid mappings assertions\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files.append(ROOT/'tools/decomp_regionpaint_oracle.py')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in dict.fromkeys(files)},os_calls=0,limits=[
        '현재 두 보호PE의 초기프레임/mask/99CRTcache/stamp/Choose/region setter를 실제 실행한다.',
        'CRT thread 주소·할당/free·factory·가상 owner/Pop·region127 chunk 조회는 격리된 외부 경계다.',
        '생성자/Pop/PostPop 전체 월드 결합이나 GUI/캠페인 완주 검증이 아니다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'total':len(rows),'cases':len(cases)}))


def verify():
    """현재 출처SHA·고유입력과 정상 복귀/x87 반복 수·assert/OS 경계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError('초기stamp SHA 불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or len(rows)!=len(inputs())*2 or report['os_calls']:raise RuntimeError('초기stamp 고유 행/OS 계수 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if item['returns']!=len(selected)*2 or len({(row[1],row[2]) for row in selected})!=len(selected) or any(int(row[-1]) for row in selected):raise RuntimeError('초기stamp 정상 반환/입력/assert 오류')
    print(f'초기stamp {len(rows)}개 SHA/고유입력/두정밀도 정상반환 감사 통과')


# 명시한 생성·smoke·출처 감사만 실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
