#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""표시/postPop 억제·비전투 raw Pop→Unpop→반납을 실제 원본 x86과 대조한다. 대체 함수는 없다."""
import argparse
import collections
import csv
import hashlib
import json
import struct
import zlib
from decomp_creation_oracle import CreationOracle, TYPES, CREATION
from decomp_sid_oracle import SidOracle, POOL
from decomp_oracle import ROOT, SCRATCH, STOP, STACK
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_FPCW, UC_X86_REG_FPSW

# SID raw 풀과 겹치지 않는 지도와 합성 SHP/빈 grid다.
HEADS=0x11000000
SPOTS=HEADS+0x30000
GRID=HEADS+0x50000
SHAPE=SCRATCH+0x360000
# 원본 보드 크기·네 해시 단계의 총 머리 수와 정밀도 입력이다.
HEAD_COUNT=86272
CONTROLS=(0x027f,0x037f)
# 실제 Pop/가상 후처리/표시·postPop 억제 전역의 판본별 주소다.
SPACE={
 'originals':dict(pop=0x4b02d0,post=0x4b0d30,first=0x4ad470,extra=40,level=33,frame=36,
     hash=0x542508,surface=0x5c84bc,spots=0x5c7c44,board=0x531928,grid=0x542514,
     dirty=0x5949d0,debug=0x5e4794,display=0x59a8b0,log=0x594fc8,depth=0x5c8488,foot=0x1d4),
 'originalCD':dict(pop=0x4ad490,post=0x4ae180,first=0x4ae0f0,extra=35,level=31,frame=34,
     hash=0x5670c0,surface=0x52d590,spots=0x52fe48,board=0x52e9a8,grid=0x5670cc,
     dirty=0x51c54c,debug=0x540a24,display=0x52039c,log=0x540a24,depth=0x539600,foot=0x1b4),
}


def bits(value):
    """정밀도를 잃지 않고 float 입력을 실제 32비트 값으로 저장한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


class PopOracle(CreationOracle):
    """실제 base 생성과 SID 코드를 공유하되 모든 공간/가상 호출을 실제 몸체로 실행한다."""
    def __init__(self,edition):
        """새 읽기 전용 디컴파일의 범위만 추가하고 지도 메모리를 별도로 확보한다."""
        super().__init__(edition)
        self.space=SPACE[edition]
        self.mu.mem_map(HEADS,0x80000)
        self.bases=[]
        cursor=HEADS
        # 원본 네 단계 배열을 같은 크기와 순서로 놓는다.
        for scale in (1,2,4,16):
            self.bases.append(cursor); cursor+=(256//scale)**2*2
        paths=[ROOT/f'extracted/pop/{edition}/functions.tsv']
        if edition=='originals': paths.append(ROOT/'extracted/pop/helpers-originals/functions.tsv')
        # 새 Ghidra 몸체 밖 함수나 OS API 호출은 기존 엄격한 hook이 거부한다.
        for path in paths:
            with path.open(encoding='utf-8') as fp:
                # 함수마다 실제 몸체 범위를 기존 허용 목록에 더한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.facts.append(row)
                    # 불연속 몸체를 각각 유지하며 사이의 코드를 허용하지 않는다.
                    for part in row['ranges'].split(';'):
                        lo,hi=(int(v,16) for v in part.split('-')); self.allowed.append((lo,hi+1))
        self.actual_calls=collections.Counter()

    def on_instruction(self,mu,address,size,data):
        """assert/대체 없이 실제 firstPop/postPop/Activate/표시 함수를 관찰한다."""
        observed={self.space['first']:'firstPop',self.space['post']:'postPop',
                  self.space['pop']:'Pop',self.creation['unpop']:'Unpop'}
        if address in observed: self.actual_calls[observed[address]]+=1
        SidOracle.on_instruction(self,mu,address,size,data)

    def setup_space(self,capacity,control):
        """비전투/클라이언트 단계에서 실제 Reset을 수행하고 억제 전역을 입력한다."""
        self.setup(capacity,False,False)
        self.control=control; self.mu.reg_write(UC_X86_REG_FPCW,control)
        s=self.space
        # log는 실제 공통 postPop의 효과 억제이고 display는 Renderer의 표시 억제다.
        for address,value in [(s['surface'],self.bases[0]),(s['spots'],SPOTS),(s['board'],256),
                              (s['grid'],GRID),(s['dirty'],0),(s['debug'],0),(s['display'],1),
                              (s['log'],1),(s['depth'],0)]:
            self.mu.mem_write(address,struct.pack('<I',value))
        self.mu.mem_write(s['hash'],struct.pack('<7I',0,1,256,*self.bases))
        self.mu.mem_write(HEADS,bytes(0x80000))
        self.write_ranges.extend([(HEADS,HEADS+HEAD_COUNT*2),(SPOTS,SPOTS+65536),(s['depth'],s['depth']+4)])
        # CD는 debug와 postPop 억제가 같은 전역이다. 정상 frame 입력만 사용한다.

    def type_space(self,number,f1,f2,width,height,fw,fh):
        """파일 로더/파생 ctor가 아닌 명시적인 합성 타입·현재 SHP 크기 입력이다."""
        self.type_input(number,f1,f2,91,7,0)
        typ=TYPES+number*self.creation['type_stride']
        self.mu.mem_write(typ+0xdc,struct.pack('<I',SHAPE))
        self.mu.mem_write(typ+0x114,struct.pack('<I',1))
        self.mu.mem_write(typ+self.space['foot'],struct.pack('<2I',width,height))
        self.mu.mem_write(SHAPE+8,struct.pack('<I',0x100))
        self.mu.mem_write(SHAPE+0x100-0x24,struct.pack('<2f',fw,fh))

    def fill_space(self,sid,seed,state,number,vtable,extra):
        """실제 할당 슬롯의 payload를 오염시키되 유효한 raw 공간 입력만 지정한다."""
        self.fill(sid,seed,state,number)
        address=POOL+sid*self.stride
        self.mu.mem_write(address,struct.pack('<IH',vtable,0))
        self.mu.mem_write(address+8,struct.pack('<H',23))
        self.mu.mem_write(address+14,struct.pack('<2f',20,20))
        self.mu.mem_write(address+self.space['extra'],bytes([extra]))
        self.mu.mem_write(address+self.space['level'],bytes([3]))
        self.mu.mem_write(address+self.space['frame'],bytes(4 if self.edition=='originals' else 1))

    def snapshot(self,sid,result):
        """슬롯 모든 바이트·풀·전체 해시·spot을 관찰한다. 체크섬은 바이트별 증명과 구별한다."""
        return [result,zlib.adler32(self.mu.mem_read(POOL,self.capacity*self.stride)),
                zlib.adler32(self.mu.mem_read(HEADS,HEAD_COUNT*2)),zlib.adler32(self.mu.mem_read(SPOTS,65536)),
                bytes(self.mu.mem_read(POOL+sid*self.stride,self.stride)).hex()]

    def step_space(self,name,sid,arg,x,y):
        """실제 thiscall 반환/EIP/ESP와 x87 스택·postPop 깊이를 검사한다."""
        self.mu.reg_write(UC_X86_REG_FPCW,self.control)
        top=self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        result=0
        if name=='Create':
            sid=self.creation_step(name,arg,2)[0]; result=sid
        elif name=='Release':
            SidOracle.step(self,name,sid)
        else:
            before=self.mu.mem_read(POOL+sid*self.stride+11,1)[0]
            self.registers(POOL+sid*self.stride)
            args=[STOP,bits(x),bits(y),arg] if name=='Pop' else [STOP,arg]
            self.mu.mem_write(STACK,struct.pack('<'+'I'*len(args),*args))
            self.execute(self.space['pop'] if name=='Pop' else self.creation['unpop'],STOP,STACK+4*len(args))
            after=self.mu.mem_read(POOL+sid*self.stride+11,1)[0]
            if name=='Pop': result=0 if not before&4 else (1 if not after&4 else 2)
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control
        assert self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==top
        assert self.mu.mem_read(self.space['depth'],4)==bytes(4)
        return sid,self.snapshot(sid,result)


def generate():
    """두 판본의 실제 vtable과 Pop/충돌/재등록/Unpop/반납 시퀀스를 fixture로 저장한다."""
    rows=['# 실제 raw Pop: 표시/postPop 억제·비전투; 합성 타입/SHP, 대체 함수 없음.']
    tables=['# PE firstPop/postPop 주소. 지원은 공통 두 메서드와 표시 경로가 모두 필요하다.']
    code=['// PE에서 공통 firstPop/postPop을 확인한 주소 표. 호스트 포인터가 아니다.']
    counts=collections.Counter(); reports={}; begins=0
    # 판본마다 별도의 실제 PE/격리 메모리에서 호출한다.
    for edition in CREATION:
        oracle=PopOracle(edition); s=oracle.space; metadata=[]; supported=[]
        # 기존 생성자 감사에서 확인한 vtable만 다시 PE 메서드 포인터와 대조한다.
        for line in (ROOT/'cpppj/tests/fixtures/unpop-vtables.tsv').read_text(encoding='utf-8').splitlines():
            if not line.startswith(edition+'\t'): continue
            cells=line.split('\t'); vtable=int(cells[1]); mask=int(cells[5])
            first,post=[struct.unpack('<I',oracle.mu.mem_read(vtable+offset,4))[0] for offset in (0x30,0x20)]
            ok=first==s['first'] and post==s['post'] and mask==3
            metadata.append([vtable,first,post,int(ok)])
            tables.append('\t'.join(map(str,[edition,*metadata[-1]])))
            if ok: supported.append(vtable)
        code.append('// '+edition+'의 공통 firstPop/postPop·표시 경로다.')
        code.append(f'constexpr std::array<std::uint32_t,{len(supported)}> k{"Patch" if edition=="originals" else "Cd"}PopVtables{{{{')
        code.extend(f'    0x{v:08x}U,' for v in supported); code.append('}};')

        def step(name,sid=0,arg=0,x=30.25,y=45.5):
            """호출 직후 독립 기계어 결과를 저장한다."""
            actual,out=oracle.step_space(name,sid,arg,x,y)
            rows.append('\t'.join(map(str,['Step',name,actual,arg,bits(x),bits(y),*out])))
            counts[name]+=1
            return actual

        # 큰 SID와 두 x87 정밀도, 다양한 extra 비트를 반복 등록/해제로 검사한다.
        for capacity in (32768,65535):
            # 같은 시퀀스를 두 정상 x87 정밀도에서 독립 실행한다.
            for control in CONTROLS:
                begins+=1; rows.append(f'Begin\t{edition}\t{capacity}\t{control}')
                oracle.setup_space(capacity,control)
                # 타입/좌표/상태/부가 비트를 바꿔 등록·재등록·해제를 반복한다.
                for case in range(80):
                    f1=0x800 if case%11==0 else 0
                    f2=(0,0x11,0x80,0x10000,0x20000,6|0x50444200)[case%6]
                    extra=(case*13)&255
                    if f2&(6|0x50444200): extra|=8
                    width,height=1+case%3,1+(case//3)%3
                    fw,fh=((1.,2.),(2.00001,1.),(4.,3.),(4.00001,8.))[case%4]
                    rows.append(f'Type\t74\t{f1}\t{f2}\t{width}\t{height}\t{bits(fw)}\t{bits(fh)}')
                    oracle.type_space(74,f1,f2,width,height,fw,fh)
                    sid=step('Create',arg=74)
                        # 고위 SID는 아래 명시적인 확보 슬롯 입력으로 검사한다.
                    if capacity==65535 and case%2==0:
                        sid=65534 if case==78 else oracle.spec['predictable']+30000+case
                        # Take 목록 흐름을 완료했다고 주장하지 않는 명시적인 기존 확보 슬롯 입력이다.
                        rows.append(f'Claim\t{sid}'); oracle.mu.mem_write(POOL+sid*oracle.stride+11,b'\x04')
                    vtable=supported[case%len(supported)]
                    seed=(case*17)&255; state=(4,6,0x84)[case%3]
                    rows.append(f'Fill\t{sid}\t{seed}\t{state}\t74\t{vtable}\t{extra}')
                    oracle.fill_space(sid,seed,state,74,vtable,extra)
                    flags=(0,8,0x2000,0x2008,1,0x10)[case%6]
                    x,y=30.25+(case%10)*10,45.5+(case//10)*10
                    if case%13==0: x,y=0,256
                    # 일부 walker 입력은 spot을 건드리지 않아 양 판본의 0<x<1 보존을 확인한다.
                    if case%6==3 and case%2:
                        x,y=0.25,20.5
                    step('Pop',sid,flags,x,y)
                    if not oracle.mu.mem_read(POOL+sid*oracle.stride+11,1)[0]&4:
                        if edition=='originals': step('Pop',sid,flags,x+1,y)
                        step('Unpop',sid,flags)
                        step('Pop',sid,flags,150.99995,160.00005)
                        step('Unpop',sid,flags)
                    step('Release',sid)
                # 충돌은 첫 칸/중간 칸을 명시적으로 채워 부분 OR의 판본 차이를 검사한다.
                for cx,cy in ((28,44),(29,45)):
                    rows.append(f'Type\t74\t0\t17\t3\t2\t{bits(3)}\t{bits(2)}')
                    oracle.type_space(74,0,17,3,2,3,2)
                    sid=step('Create',arg=74)
                    rows.append(f'Fill\t{sid}\t7\t4\t74\t{oracle.creation["vtable"]}\t0')
                    oracle.fill_space(sid,7,4,74,oracle.creation['vtable'],0)
                    rows.append(f'Seed\t{cx}\t{cy}\t17'); oracle.mu.mem_write(SPOTS+cy*256+cx,bytes([17]))
                    step('Pop',sid,0,30,45)
                    if not oracle.mu.mem_read(POOL+sid*oracle.stride+11,1)[0]&4: step('Unpop',sid)
                    step('Release',sid)
                    rows.append('Clear'); oracle.mu.mem_write(HEADS,bytes(HEAD_COUNT*2)); oracle.mu.mem_write(SPOTS,bytes(65536))
                # 실제 Pop으로 같은 버킷에 세 객체를 넣고 중간/머리/꼬리를 각각 해제한다.
                for level,(f2,extra,fw,fh) in enumerate(((2,8,1.,1.),(0x10000,0,1.,1.),(0x10000,0,3.,3.),(0x10000,0,8.,8.))):
                    rows.append(f'Type\t74\t0\t{f2}\t1\t1\t{bits(fw)}\t{bits(fh)}')
                    oracle.type_space(74,0,f2,1,1,fw,fh); peers=[]
                    # 각 next는 앞선 Pop이 쓴 실제 머리를 사용한다. 합성 Head 입력은 없다.
                    for seed in (7,19,31):
                        sid=step('Create',arg=74); peers.append(sid)
                        rows.append(f'Fill\t{sid}\t{seed}\t4\t74\t{oracle.creation["vtable"]}\t{extra}')
                        oracle.fill_space(sid,seed,4,74,oracle.creation['vtable'],extra)
                        step('Pop',sid,0,170.5,180.25)
                    # 가운데를 제거해 이전 next 갱신을 확인하고 남은 머리/꼬리를 제거한다.
                    for sid in (peers[1],peers[2],peers[0]):
                        step('Unpop',sid); step('Release',sid)
        reports[edition]=dict(binary_sha256=oracle.sha256,function_ranges=oracle.facts,vtable_rows=len(metadata),
            supported_vtables=len(supported),actual_calls=dict(oracle.actual_calls),assert_reports=oracle.assertions,
            max_instructions_observed=oracle.max_instructions)
        print(f'{edition}: 실제 raw Pop/Unpop 대조 완료',flush=True)
    artifacts={'cpppj/tests/fixtures/pop-x86.tsv':'\n'.join(rows)+'\n',
               'cpppj/tests/fixtures/pop-vtables.tsv':'\n'.join(tables)+'\n','cpppj/src/o/PopVtables.inc':'\n'.join(code)+'\n'}
    # UTF-8/LF와 도구/원본/기대값의 해시로 재현 근거를 보관한다.
    for name,body in artifacts.items(): (ROOT/name).write_text(body,encoding='utf-8',newline='\n')
    sources=['tools/decomp_pop_oracle.py','tools/decomp_creation_oracle.py','tools/decomp_sid_oracle.py',
             'tools/decomp_oracle.py','tools/ghidra/ExportCreation.java','tools/ghidra/ExportSid.java',
             'cpppj/tests/fixtures/unpop-vtables.tsv']
    report=dict(schema=1,classification='scoped-raw-pop-display-postpop-disabled-nonbattle',cases=dict(counts),
        total_cases=sum(counts.values()),begins=begins,setup_resets=begins,editions=reports,
        artifact_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in artifacts},
        source_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in sources},
        limits=['실제 Pop/firstPop/Activate/공통 postPop/표시 억제/Unpop/void 반납; 대체 함수/assert 없음',
                'base fallback 생성·합성 타입/SHP/파생 vtable payload·고위 슬롯 Claim 입력; 파생 ctor/월드 로딩·고위 Allocate 검증 아님',
                '표시/postPop 효과 억제·비전투·빈 grid/dirty; 영역/부착/파생 효과·소유자/생산 효과는 후속',
                'SID/풀/공간 지도 직접 연동; GameWorld 연결과 미션 완주 증명은 아님',
                '슬롯 모든 바이트 직접 대조; 전체 풀/머리/spot은 Adler-32; 게임/OS/GUI 진입점 없음'])
    (ROOT/'cpppj/recovery-pop-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(cases=report['cases'],total_cases=report['total_cases'],begins=begins)),flush=True)


def verify():
    """기록의 원본/도구/주소 표/fixture 해시와 호출 행 수를 검사한다."""
    report=json.loads((ROOT/'cpppj/recovery-pop-evidence.json').read_text(encoding='utf-8'))
    # 각 기록 그룹에 들어 있는 원본 도구/산출물을 빠짐없이 대조한다.
    for group in ('source_sha256','artifact_sha256'):
        # 상대 경로의 파일 바이트가 생성 당시 SHA-256과 같은지 확인한다.
        for name,digest in report[group].items(): assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==digest,name
    # 양 판본의 바이너리와 assert 미도달 기록을 별도로 확인한다.
    for edition,data in report['editions'].items():
        binary=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        assert hashlib.sha256(binary.read_bytes()).hexdigest()==data['binary_sha256'] and data['assert_reports']==0
    counts=collections.Counter(line.split('\t')[1] for line in (ROOT/'cpppj/tests/fixtures/pop-x86.tsv').read_text(encoding='utf-8').splitlines() if line.startswith('Step\t'))
    assert dict(counts)==report['cases'] and sum(counts.values())==report['total_cases']
    print(json.dumps(dict(verified=True,cases=dict(counts),total_cases=sum(counts.values()))),flush=True)


# 직접 실행할 때만 격리 분석 또는 기록 확인을 수행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify',action='store_true')
    args=parser.parse_args(); verify() if args.verify else generate()
