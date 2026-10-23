#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Kencloud 세 본체와 같은 GameRandom을 제품 전에 격리 실행해 최초 부분효과를 고정한다."""
import argparse
from collections import Counter
import hashlib
import itertools
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW

CACHE, END, TOP, BOTTOM = 0x569ffc, 0x569ff0, 0x569ff8, 0x569ff4  # 같은 할당의 현재 주소 전역이다.
PHASE, ALL, PART = 0x56b804, 0x56b7fc, 0x56b800  # 행 갱신 위상과 전체/부분 dirty 전역이다.
COLOR, SPEED, FURY, COUNT, SPACER = 0x540a34, 0x540a80, 0x540a84, 0x540a90, 0x540a88  # 같은 옵션 해석기가 쓰는 현재 DWORD다.
ROWS, DIRTY, SEEDS, RNG = 0x540a98, 0x56a000, 0x56a800, 0x532710  # signed 시작행·512dirty행·현재seed주소·실제게임난수다.
STATE, STATE_SIZE, SIZE = 0x5697f0, 0x2018, 0x40000  # 인접 alias까지 포함한 현재 상태와512×512BYTE 크기다.
BUFFERS = (POOL + 0x1000, POOL + 0x51000, POOL + 0xa1000)  # 원문 이미지와 겹치지 않는 세 실제 물리 블록이다.
TOKENS = {0: 0, 11: BUFFERS[0], 22: BUFFERS[1], 33: BUFFERS[2]}  # 파일/할당 장치의 주소만 논리 입력으로 분리한다.
RANGES = ((0x470b20, 0x470bad), (0x470bb0, 0x470c25), (0x471040, 0x471190), (0x4558c0, 0x4558e8))  # 세 계산 본체와 실제 GameRandom40BYTE를 실행한다.
FIXTURE, REPORT = ROOT / 'cpppj/tests/fixtures/kencloud-v2-x86.tsv', ROOT / 'cpppj/kencloud-v2-native.json'  # 최초 제품 전 자료이며 덮어쓰지 않는다.


def cases():
    """유효 파일/실패 할당/현재 옵션/행 경계/하위 callback을 독립 입력한다."""
    rows = []
    # kind/cache/image,width,height,exists,allocate,color,speed,fury,count,phase,state,endOffset,mutation,pathCase다.
    base = ('build', 0, 11, 512, 512, 1, 11, 228, 1, 10000, 3, 0, 0, SIZE, 'none', 0)
    def add(**changes):
        values = list(base)
        names = ('kind','cache','image','width','height','exists','allocate','color','speed','fury','count','phase','state','end','mutation','path')
        for key, value in changes.items(): values[names.index(key)] = value
        rows.append(tuple(values))
    for image, width, height, exists in ((11,512,512,1),(11,511,512,1),(11,512,511,1),(0,512,512,1),(11,512,512,0)):
        for speed, fury in ((0,10000),(1,10000),(2,10000),(1,20000),(1,-1),(1,0)):
            add(image=image,width=width,height=height,exists=exists,speed=speed,fury=fury)
    for cache, count, phase in itertools.product((11,22),(-1,0,3),(0,0xffffffff)): add(cache=cache,count=count,phase=phase,state=0x12345678)
    for count in (-1,0,1,400): add(count=count,state=0x89abcdef)
    add(exists=0,allocate=0);add(exists=0,color=0x123456a5)
    for phase, end in itertools.product((0,7,15,0xffffffff),(SIZE,17,1,0)): add(kind='step',cache=11,phase=phase,end=end)
    add(kind='step',cache=0)
    for width,height,image,exists,path in itertools.product((511,512),(511,512),(0,11),(0,1),(0,1)):
        add(kind='file',width=width,height=height,image=image,exists=exists,path=path)
    add(kind='file',path=2)
    for mutation in ('path-source','path-stop','exists-source','exists-name','exists-stop','decode-cache33','decode-stop','free-cache33','free-stop','alloc-color17','alloc-cache33','alloc-stop','random1-cache22','random1-count0','random2-count4','random1-state17','random1-stop','random3-fury0','move1-cache22','move1-end17','move1-state17','move1-stop','move32-cache0','move32-fury20000','move65-stop'):
        add(image=11 if mutation.startswith(('path','exists','decode','free','random')) else 0,width=511 if mutation.startswith('free') else 512,fury=4000 if mutation=='move32-cache0' else 10000,speed=0xffffffff if mutation=='move65-stop' else 1,mutation=mutation)
    # 같은 입력 중복은 최초 자료의 중복 관측으로 숨기지 않는다.
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """계산/쓰기/실제GameRandom은 원문이며 파일·할당·memmove의 OS 밖 물리만 명시 장치다."""
    def __init__(self):
        super().__init__()
        self.total, self.completions = 0, Counter()

    def current(self, address):
        """현재 실제 DWORD를 읽는다."""
        return self.words(address, 1)[0]

    def signed(self, address):
        """같은 DWORD의 signed view를 원문 비교/IDIV에 쓴다."""
        return struct.unpack('<i', self.mu.mem_read(address, 4))[0]

    def region(self, address, length):
        """소유한 실제 물리만 허용하며 길이0의one-past주소는 접근하지 않는다."""
        for base in BUFFERS:
            if base <= address <= base + SIZE and length <= base + SIZE - address: return True
        return False

    def stop(self, cause):
        """명시/물리 중단은 정상 반환과 구별하고 마지막 파일 scratch도 보존한다."""
        self.cause = cause
        self.capture_file()
        self.mu.emu_stop()

    def capture_file(self):
        """활성 파일 부모의 caller명시 stack입력/현재출력을 해시로 보존한다."""
        if self.file_base is not None:
            data = bytes(self.mu.mem_read(self.file_base, 0x108))
            self.file_snapshot = hashlib.sha256(data).hexdigest()

    def mutate(self, boundary):
        """실제 하위가 정상 복귀하기 직전의 명시 현재값 변화만 적용한다."""
        mutation = self.case[14]
        if mutation == boundary + '-stop': self.stop(boundary)
        elif mutation == boundary + '-cache33': self.put(CACHE, TOKENS[33])
        elif mutation == boundary + '-cache22': self.put(CACHE, TOKENS[22])
        elif mutation == boundary + '-cache0': self.put(CACHE, 0)
        elif mutation == boundary + '-count0': self.put(COUNT, 0)
        elif mutation == boundary + '-count4': self.put(COUNT, 4)
        elif mutation == boundary + '-state17': self.put(RNG, 17)
        elif mutation == boundary + '-fury0': self.put(FURY, 0)
        elif mutation == boundary + '-fury20000': self.put(FURY, 20000)
        elif mutation == boundary + '-color17': self.put(COLOR, 17)
        elif mutation == boundary + '-end17': self.put(END, self.current(CACHE) + 17)
        elif mutation == boundary + '-source': self.mu.mem_write(TEXT, b'ChangedSource\0')
        elif mutation == boundary + '-name' and self.file_base is not None: self.mu.mem_write(self.file_base + 8, b'ChangedName\0')

    def instruction(self, mu, address, size, data):
        """세 부모와GameRandom만 실행하고 명시 하위/원시 접근 경계를 추적한다."""
        self.total += 1
        self.instructions += 1
        if self.instructions > 500000: raise RuntimeError('Kencloud 독립명령상한 ' + str(self.case))
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp + 4, 5)
        if address == 0x470bb0:
            self.calls['file'] += 1
            self.events.append('FILE:' + str(self.calls['file']))
            self.file_base = esp - 0x108
            self.mu.mem_write(self.file_base, b'\xcd' * 0x108)
        elif address == 0x470c24:
            self.capture_file(); self.file_base = None
        elif address in (0x471126,0x47115e) and self.current(FURY) == 0:
            self.stop('divide-zero'); return
        elif address == 0x470b2b and not self.region(mu.reg_read(UC_X86_REG_ECX),1): self.stop('step-read'); return
        elif address == 0x470b3c and not self.region((mu.reg_read(UC_X86_REG_ECX)-1)&0xffffffff,1): self.stop('step-tail'); return
        elif address == 0x471093 and mu.reg_read(UC_X86_REG_ECX) != 0 and not self.region(mu.reg_read(UC_X86_REG_EDI),4): self.stop('fill-physical'); return
        elif address == 0x470bd0:
            source = mu.reg_read(UC_X86_REG_EAX)
            if not TEXT <= source < TEXT + 1024: self.stop('file-path-read'); return
        elif address == 0x470bd3 and mu.reg_read(UC_X86_REG_EDX) >= self.file_base + 0x108:
            self.stop('file-path-write'); return
        elif address == 0x4410f0:
            assert self.string(args[0]) == b'CloudFileSpec' and args[1:4] == (0,0,0)
            self.events.append('PATH:CloudFileSpec:0,0,0');self.mutate('path')
            if not self.cause: self.ret(TEXT)
            return
        elif address == 0x41ab10:
            self.events.append('EXISTS:' + self.string(args[0]).hex());self.mutate('exists')
            if not self.cause: self.ret(self.case[5])
            return
        elif address == 0x4dae60:
            self.events.append('DECODE:' + self.string(args[0]).hex())
            self.put(args[1],self.case[3]);self.put(args[2],self.case[4]);self.mutate('decode')
            if not self.cause: self.ret(TOKENS[self.case[2]])
            return
        elif address == 0x4e4354:
            self.calls['free'] += 1;self.events.append('FREE:' + str(args[0]));self.mutate('free')
            if not self.cause: self.ret()
            return
        elif address == 0x4e4391:
            assert args[0] == SIZE
            self.calls['alloc'] += 1
            result = 0 if self.case[6] == 0 else BUFFERS[min(self.calls['alloc']-1,2)]
            self.events.append('ALLOC:' + str(result));self.mutate('alloc')
            if not self.cause: self.ret(result)
            return
        elif address == 0x4558c0:
            self.calls['random'] += 1
            self.random_before = self.current(RNG)
            self.random_limit = args[0]
        elif address in (0x4710da,0x4710ea,0x471111):
            index = self.calls['random'];self.events.append('RANDOM:' + ','.join(map(str,(index,self.random_limit,self.random_before,mu.reg_read(UC_X86_REG_EAX),self.current(RNG)))))
            self.mutate('random' + str(index))
            if self.cause: return
        elif address == 0x4e5b40:
            dst,src,length = args[:3]
            self.calls['move'] += 1;index = self.calls['move']
            self.events.append('MOVE:' + ','.join(map(str,(index,dst,src,length))))
            if not self.region(dst,length) or not self.region(src,length): self.stop('move-physical');return
            if length: self.mu.mem_write(dst, bytes(self.mu.mem_read(src,length)))
            self.mutate('move' + str(index))
            if not self.cause: self.ret(dst)
            return
        if not any(lo <= address < hi for lo,hi in RANGES): raise RuntimeError('Kencloud 허용밖 ' + hex(address))

    def sample(self, case, control):
        """같은 물리 상태·옵션·난수를독립초기화하고 전체 버퍼/상태 해시를 비교한다."""
        self.case,self.calls,self.events,self.cause,self.instructions = case,Counter(),[],'',0
        self.file_base,self.file_snapshot = None,'-'
        kind,cache,image,width,height,exists,allocate,color,speed,fury,count,phase,state,end,mutation,path = case
        self.mu.mem_write(STATE,b'\xcd'*STATE_SIZE)
        for index,base in enumerate(BUFFERS): self.mu.mem_write(base,bytes((byte*13+index*31)&255 for byte in range(SIZE)))
        self.mu.mem_write(TEXT,b'\xde'*1024)
        self.mu.mem_write(TEXT,b'' if path == 2 else b'Cloud/File.gif\0' if path == 0 else b'\0')
        if path == 2: self.mu.mem_write(TEXT,b'Q'*256+b'\0')
        token = TOKENS[cache]
        for address,value in ((CACHE,token),(END,token+end),(TOP,token+512),(BOTTOM,token+end-512),(PHASE,phase),(ALL,17),(PART,19),(COLOR,color),(SPEED,speed),(FURY,fury),(COUNT,count),(SPACER,29),(RNG,state)):
            self.put(address,value)
        for index in range(512): self.put(STATE+index*4,0x80000000+index);self.put(DIRTY+index*4,0x90000000+index)
        for index in range(1000): self.put(SEEDS+index*4,0x70000000+index)
        self.mu.mem_write(STACK-0x1000,b'\xcd'*0x1100);self.put(STACK,STOP)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.emu_start({'build':0x471040,'step':0x470b20,'file':0x470bb0}[kind],STOP)
        if not self.cause: self.capture_file()
        self.completions[self.cause or 'return'] += 1
        values = ','.join(map(str,(mu_eax:=self.mu.reg_read(UC_X86_REG_EAX) if kind=='file' and not self.cause else 0,self.current(RNG),self.current(COLOR),self.current(SPEED),self.current(FURY),self.current(COUNT),self.current(SPACER),self.calls['file'],self.calls['alloc'],self.calls['free'],self.calls['random'],self.calls['move'])))
        raw = ':'.join(hashlib.sha256(bytes(self.mu.mem_read(base,length))).hexdigest() for base,length in ((STATE,STATE_SIZE),*((base,SIZE) for base in BUFFERS))) + ':' + self.file_snapshot
        return '\t'.join((str(control),','.join(map(str,case)),self.cause or 'return',values,raw,';'.join(self.events)))


def main():
    """최초원문만생성하고이후에는고정SHA를검증한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in report['sha256'].items():
            if digest(ROOT/name)!=expected: raise RuntimeError('Kencloud 최초SHA불일치 '+name)
        print('Kencloud 최초원문보존',report['observations']);return
    if FIXTURE.exists() or REPORT.exists(): raise RuntimeError('Kencloud 최초fixture/report덮어쓰기금지')
    oracle,inputs=Oracle(),cases();lines=['# control / kind,cache,image,width,height,exists,allocate,color,speed,fury,count,phase,state,end,mutation,pathCase / stop / values12 / raw5hash / trace']
    # 실제 GameRandom도양정밀도에서동일상태를소비함을확인한다.
    for case in inputs:
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        assert first.split('\t')[1:]==second.split('\t')[1:],case
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_kencloud_v2.py','cpppj/tests/fixtures/kencloud-v2-x86.tsv','cpppj/src/o/SpStore.h','cpppj/src/o/SpStore.cpp')
    report={'status':'후속384독립준비·제품작성전최초고정','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','uniqueInputs':len(inputs),'observations':len(inputs)*2,'completions':dict(oracle.completions),'instructions':oracle.total,'precisionDifferences':0,'scope':'실제471040/470b20/470bb0와4558c0 GameRandom. PathSpec/exists/GIF/allocate/free/CRTmemmove는명시물리장치이며관측callback후현재값재조회. 정확한256BYTE파일scratch입력/출력보존. 실제OS/GUI/원본게임실행아님. 물리할당범위/Divide0/명시하위중단을정상반환과구분. ColorSort와별개Kencloud.','gameExecuted':False,'guiExecuted':False,'sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs','observations','completions','instructions')}))


if __name__=='__main__':main()
