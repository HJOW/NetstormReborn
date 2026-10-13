#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""표시를 켠 공통 update88/update8c·경계·raw Pop/Unpop을 실제 두 PE의 x86으로 검증한다."""
import argparse
import collections
import csv
import hashlib
import json
import struct
from decomp_pop_oracle import PopOracle, HEADS, SHAPE, bits
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL
from decomp_oracle import ROOT, STOP, STACK
from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 격리 지도 뒤의 Renderer 객체/경계 출력이다. SID 풀과 겹치지 않는다.
DISPLAY_OBJECT=HEADS+0x60000
RECT=HEADS+0x70000
# 실제 표시 함수와 카메라/Q16/viewport/Renderer/화면 크기 전역 주소다.
DISPLAY={
 'originals':dict(update=0x4ad500,alternate=0x4ad670,mark=0x4990d0,bounds=0x498ff0,dirty=0x497580,
     camera=0x59a91c,zoom=0x59a918,viewport=0x5ca9cc,renderer=0x59a8bc,width=0x5c78f0,height=0x5c78f4,merge=0x59a8d0),
 'originalCD':dict(update=0x4ae380,alternate=0x4ae3e0,mark=0x456a00,bounds=0x456930,dirty=0x457320,
     camera=0x565c34,zoom=0x565c30,viewport=0x583e20,renderer=0x5203a8,width=0x516b10,height=0x516b14,merge=0x5203bc),
}


def packed(entries):
    """원본 변경 표의 사각형/플래그를 TSV 열 하나로 보존한다."""
    return ';'.join(','.join(map(str,entry)) for entry in entries) or '-'


class DisplayOracle(PopOracle):
    """기존 실제 생성/공간 함수와 새 표시 몸체만 실행한다. assert/OS/대체는 금지한다."""
    def __init__(self,edition):
        """읽기 전용 내보내기로만 허용 범위를 확장한다."""
        super().__init__(edition)
        self.display=DISPLAY[edition]
        self.display_calls=collections.Counter()
        with (ROOT/f'extracted/display/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            # 불연속 함수 범위 사이의 명령은 허용하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.facts.append(row)
                for part in row['ranges'].split(';'):
                    lo,hi=(int(v,16) for v in part.split('-')); self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """공통 가상 표시와 경계/dirty의 실제 도달을 별도 집계한다."""
        if hasattr(self,'display'):
            # 내부 표시 호출 수를 상위 fixture 호출 수에 더하지 않는다.
            for name in ('update','alternate','mark','bounds','dirty'):
                if address==self.display[name]: self.display_calls[name]+=1
        super().on_instruction(mu,address,size,data)

    def start(self,control):
        """기존 확보 풀의 실제 Reset/Create를 사용하고 표시 객체 쓰기만 추가 허용한다."""
        self.setup_space(32768,control)
        self.write_ranges.extend([(DISPLAY_OBJECT,DISPLAY_OBJECT+0x900),(RECT,RECT+16),
                                  (self.space['display'],self.space['display']+4)])
        self.sid=self.creation_step('Create',74,2)[0]

    def shape(self,flags,count,loaded,first,frames):
        """파일 로더를 대신하는 명시적 합성 타입/SHP 입력이다. 실제 표시 기계어는 대체하지 않는다."""
        self.type_input(74,flags,0x10000,first,7,0)
        typ=TYPES+74*self.creation['type_stride']
        self.mu.mem_write(typ+0xdc,struct.pack('<I',SHAPE if loaded else 0))
        self.mu.mem_write(typ+0x114,struct.pack('<I',count))
        self.mu.mem_write(typ+self.space['foot'],struct.pack('<2I',1,1))
        # VFX table 주소 앞 36바이트에 cell 크기와 네 signed short를 넣는다.
        for i,frame in enumerate(frames):
            offset=0x100+i*0x80
            self.mu.mem_write(SHAPE+8+i*8,struct.pack('<I',offset))
            self.mu.mem_write(SHAPE+offset-36,struct.pack('<2f',1,1))
            self.mu.mem_write(SHAPE+offset-12,struct.pack('<4h',*frame))

    def view(self,width,height,cameraX,cameraY,zoom,viewport,suppressed):
        """원본 카메라·viewport와 빈 Renderer 변경 표를 입력한다."""
        d=self.display
        self.mu.mem_write(DISPLAY_OBJECT,bytes(0x900))
        self.mu.mem_write(d['camera'],struct.pack('<2i',cameraX,cameraY))
        self.mu.mem_write(d['viewport'],struct.pack('<4i',*viewport))
        # 화면 크기/표시 억제/병합 설정은 파일이 아닌 격리 전역이다.
        for a,v in [(d['zoom'],zoom),(d['renderer'],DISPLAY_OBJECT),(d['width'],width),(d['height'],height),
                    (d['merge'],0),(self.space['display'],suppressed)]:
            self.mu.mem_write(a,struct.pack('<I',v))

    def slot(self,frame,extra,state,x,y):
        """실제 확보한 base 슬롯에 통제된 입력만 넣고 기존 공간 머리를 비운다."""
        self.mu.mem_write(HEADS,bytes(86272*2))
        address=POOL+self.sid*self.stride
        self.mu.mem_write(address,bytes(self.stride))
        self.mu.mem_write(address,struct.pack('<I',self.creation['vtable']))
        self.mu.mem_write(address+10,bytes([74,state]))
        self.mu.mem_write(address+14,struct.pack('<2f',x,y))
        self.mu.mem_write(address+self.space['frame'],struct.pack('<I',frame&0xffffffff) if self.edition=='originals' else bytes([frame]))
        self.mu.mem_write(address+self.space['extra'],bytes([extra]))

    def invoke(self,address,args=(),this=0,purge=0):
        """실제 호출 규약을 유지하고 반환 주소·ESP·x87 설정/스택을 검사한다."""
        self.registers(this); self.mu.reg_write(UC_X86_REG_FPCW,self.control)
        top=self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        self.execute(address,STOP,STACK+4+purge)
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control
        assert self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==top

    def dirty_output(self):
        """원본 순서의 전체 변경 표와 full 전역을 읽는다."""
        count=struct.unpack('<I',self.mu.mem_read(DISPLAY_OBJECT+100,4))[0]
        result=[]
        # 무시된 항목도 플래그와 함께 보존한다.
        for i in range(count):
            rect=struct.unpack('<4i',self.mu.mem_read(DISPLAY_OBJECT+0x68+i*16,16))
            flag=struct.unpack('<I',self.mu.mem_read(DISPLAY_OBJECT+0x6a8+i*4,4))[0]
            result.append((*rect,flag))
        full=struct.unpack('<I',self.mu.mem_read(self.space['display'],4))[0]
        return [full,packed(result)]

    def raw(self):
        """표시가 raw 객체를 변경하지 않는지 슬롯 전체를 검사한다."""
        return bytes(self.mu.mem_read(POOL+self.sid*self.stride,self.stride)).hex()


def generate():
    """두 판본/두 x87 정밀도의 단독 표시와 실제 raw 등록/제거 시퀀스를 생성한다."""
    rows=['# 실제 표시 활성 x86. 합성 타입/SHP, 대체 함수·게임 프로세스 실행 없음.']
    counts=collections.Counter(); reports={}
    # 각 판본은 독립 원본 PE와 Ghidra 몸체 범위를 사용한다.
    for edition in DISPLAY:
        oracle=DisplayOracle(edition)
        # 같은 float 입력을 두 정상 x87 정밀도에서 실행한다.
        for control in (0x027f,0x037f):
            oracle.start(control); rows.append(f'Begin\t{edition}\t{control}\t{oracle.sid}')
            # 크기 18/19 경계·음수 hotspot·축소·viewport 밖·그림자·null을 섞는다.
            for case in range(100):
                flags=(0,0x40000,0x400000,0x440000)[case%4]
                count=2; loaded=case%17!=0; first=91
                if edition=='originals' and case%23==0: first=-0x22222223
                frames=[(w,5+i*11,(case%9)-4,i*5-2) for i,w in enumerate((17,18,3,31,9,40))]
                rows.append(f'Type\t{flags}\t{count}\t{int(loaded)}\t{first}\t'+packed(frames))
                oracle.shape(flags,count,loaded,first,frames)
                cameraX,cameraY=(case%5)*7,(case%7)*9
                zoom=(32768,65536,98304,131072)[(case//4)%4]
                viewport=(12,9,620,450); suppressed=int(case%19==0)
                rows.append(f'View\t640\t480\t{cameraX}\t{cameraY}\t{zoom}\t'+','.join(map(str,viewport))+f'\t{suppressed}')
                oracle.view(640,480,cameraX,cameraY,zoom,viewport,suppressed)
                frame=case%2; extra=(0,4,8,12)[(case//2)%4]
                x,y=((-2.,2.),(0.25,0.5),(39.99995,40.00005),(30.25,25.5))[case%4]
                if edition=='originals' and case%13==0: frame=-1
                rows.append(f'Slot\t{frame}\t{extra}\t4\t{bits(x)}\t{bits(y)}')
                oracle.slot(frame,extra,4,x,y)
                callflags=0x2000 if case%2 else 0
                before=oracle.raw()
                oracle.invoke(oracle.display['alternate' if callflags else 'update'],this=POOL+oracle.sid*oracle.stride)
                assert before==oracle.raw()
                rows.append('\t'.join(map(str,['Update',callflags,*oracle.dirty_output()]))); counts['Update']+=1
                # 경계 계산은 표시 억제와 무관하다. frameCheck assert를 요구하는 입력은 제외한다.
                if loaded and first==91:
                    oracle.invoke(oracle.display['bounds'],[RECT,74,0,bits(x),bits(y)])
                    actual=struct.unpack('<4i',oracle.mu.mem_read(RECT,16))
                    rows.append('\t'.join(map(str,['Bounds',0,bits(x),bits(y),*actual]))); counts['Bounds']+=1
            # 실제 Pop/Unpop→이동 재등록은 같은 dirty 표에서 old/new 위치를 갱신한다.
            frames=[(11,17,12,16),(19,23,-3,5)]
            rows.append('Type\t262144\t1\t1\t91\t'+packed(frames)); oracle.shape(0x40000,1,True,91,frames)
            rows.append('View\t4096\t3072\t0\t0\t65536\t0,0,4096,3072\t0')
            oracle.view(4096,3072,0,0,65536,(0,0,4096,3072),0)
            rows.append(f'Slot\t0\t4\t4\t{bits(20)}\t{bits(20)}'); oracle.slot(0,4,4,20,20)
            # 120개 이상의 분리 영역은 100항목 넘침과 이후 표시 생략을 검사한다.
            for case in range(120):
                x,y=5.+(case%40)*6.,8.+(case//40)*40.
                callflags=0x2000 if case%2 else 0
                oracle.invoke(oracle.space['pop'],[bits(x),bits(y),callflags],POOL+oracle.sid*oracle.stride,12)
                rows.append('\t'.join(map(str,['Pop',callflags,bits(x),bits(y),oracle.raw(),*oracle.dirty_output()]))); counts['Pop']+=1
                oracle.invoke(oracle.creation['unpop'],[callflags],POOL+oracle.sid*oracle.stride,4)
                rows.append('\t'.join(map(str,['Unpop',callflags,oracle.raw(),*oracle.dirty_output()]))); counts['Unpop']+=1
            assert oracle.dirty_output()[0]==1
        reports[edition]=dict(binary_sha256=oracle.sha256,function_ranges=oracle.facts,
            actual_display_calls=dict(oracle.display_calls),actual_lifecycle_calls=dict(oracle.actual_calls),assert_calls=oracle.assertions)
        print(f'{edition}: 실제 표시/경계/Pop/Unpop 완료',flush=True)
    payload='\n'.join(rows)+'\n'
    fixture=ROOT/'cpppj/tests/fixtures/display-x86.tsv'; fixture.write_text(payload,encoding='utf-8',newline='\n')
    report=dict(schema=1,method='격리 실제 x86; 공통 표시 활성, 공통 postPop 효과 억제, 비전투',
        public_calls=dict(counts),total_calls=sum(counts.values()),sequences=4,editions=reports,stubbed=[],
        fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(__file_bytes()).hexdigest(),
        dependencies={name:hashlib.sha256((ROOT/'tools'/name).read_bytes()).hexdigest() for name in
            ('decomp_oracle.py','decomp_sid_oracle.py','decomp_creation_oracle.py','decomp_pop_oracle.py')},
        limitations=['합성 타입/SHP와 기존 확보 풀; 실제 자산/Win32/월드 플레이 검증은 별도',
            '공통 update88/update8c만 지원; debug assert/파생 표시/공통 postPop 영역·소유자 효과 미복원',
            '유한 float·정상 viewport; CD 손상된 물리 프레임 접근은 C++ 보호 경로로 거부'])
    (ROOT/'cpppj/recovery-display-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'표시 x86 fixture: {sum(counts.values())}회, {dict(counts)}',flush=True)


def __file_bytes():
    """실행 도구 자체의 정확한 SHA를 고정한다."""
    return (ROOT/'tools/decomp_display_oracle.py').read_bytes()


def verify():
    """원본 PE·도구·의존성·fixture와 보고서 호출 수를 게임 실행 없이 재검사한다."""
    report=json.loads((ROOT/'cpppj/recovery-display-evidence.json').read_text(encoding='utf-8'))
    assert report['tool_sha256']==hashlib.sha256(__file_bytes()).hexdigest()
    # 원본 파일은 읽기만 하며 판본별 해시를 확인한다.
    for edition,data in report['editions'].items():
        binary=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        assert data['binary_sha256']==hashlib.sha256(binary.read_bytes()).hexdigest()
        assert data['assert_calls']==0
    # 부모 도구의 변화가 fixture의 근거를 무효화하면 검증을 실패시킨다.
    for name,digest in report['dependencies'].items(): assert digest==hashlib.sha256((ROOT/'tools'/name).read_bytes()).hexdigest()
    fixture=ROOT/'cpppj/tests/fixtures/display-x86.tsv'
    assert report['fixture_sha256']==hashlib.sha256(fixture.read_bytes()).hexdigest()
    counts=collections.Counter(line.split('\t')[0] for line in fixture.read_text(encoding='utf-8').splitlines())
    assert {name:counts[name] for name in report['public_calls']}==report['public_calls']
    assert sum(report['public_calls'].values())==report['total_calls']
    assert report['stubbed']==[]
    print(f'표시 근거 SHA/호출 수 확인: {report["total_calls"]}회')


if __name__=='__main__':
    # --verify는 Unicorn 기계어 실행도 생략한다.
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    verify() if args.verify else generate()
