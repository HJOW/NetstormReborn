#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 낙하 요청·SharedRegular 생성·0x25b 전체 반환을 세 PE에서 대조한다. 게임/OS 실행은 없다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, TYPES, LISTS, TARGET, STACK, STOP, CAPACITY, digest
from decomp_prieststate_oracle import STATE, SPOTS, CONTROLS, bits
from decomp_priestpostpop_oracle import POST, MEMORY, PRIEST_TYPE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 원본 몸체와 명시적인 미복원 효과 경계다. SharedRegular는 타입 61이다.
FALL = {
 'originals': dict(begin=0x4941f0, dispatch=0x494580, ctor=0x496f00, attach=0x41bd20,
    shield=0x493d30, clear=0x491980, find=0x4918e0, set=0x4acee0, advance=0x4afc90,
    sound=0x4a9d70, surfaces=0x542514, shared=0x541164, now=0x55b4d0, frame=36),
 'originalCD': dict(begin=0x40c880, dispatch=0x40ccd0, ctor=0x48f1d0, attach=0x48f590,
    shield=0x40bfb0, clear=0x40c0d0, set=0x4acbc0, advance=0x4acce0,
    sound=0x438c00, surfaces=0x5670cc, shared=0x51ca54, now=0x5484a8, frame=34),
}
FALL['original1037'] = FALL['originalCD']
# 가상 코드 표/WORD 지도/반환 관찰 메모리와 저장 경로다.
CODES, SURFACES, TEMP, CALLER = LISTS + 0x2000, 0x13020000, 0x13010000, STOP + 0x100
FIXTURE = ROOT / 'cpppj/tests/fixtures/priestfall-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-priestfall-evidence.json'
KEYS = ('kind', 'authority', 'allocated', 'extra', 'hp', 'spot', 'side', 'surface', 'change', 'x', 'y')
# 좌표는 입력만 만들며 어느 칸인지/반환값은 원본 기계어가 결정한다.
COORDS = ((20.75,21.9),(20.000099182128906,21.000099182128906),(-2.0,21.0),
    (255.000099182128906,255.000099182128906),(256.0,1.0),(-0.0,0.0))


def inputs():
    """요청의 권한/확보와 이벤트의 상태/방향/지면 및 외부 효과 변이를 교차한다."""
    rows = []
    # 요청은 현재 상태와 관계없이 보호막부터 실행한다. change는 권한/최종 좌표 변이다.
    for authority, allocated, change, coordinate in itertools.product((0,1),(0,1),range(4),COORDS):
        rows.append(dict(kind='Begin',authority=authority,allocated=allocated,extra=32,hp=20,spot=0,
            side=65,surface=0,change=change,x=bits(coordinate[0]),y=bits(coordinate[1])))
    # 효과 뒤 현재 extra/프레임/좌표/HP를 다시 읽는지 검사한다.
    for extra,hp,spot,side,surface,change in itertools.product((0,32),(20,150),(0,6),
            (65,74,255),(0,1,65535),range(5)):
        coordinate=COORDS[len(rows)%len(COORDS)]
        # 기존 지면 helper는 지도 안의 정상 자산 좌표를 받는다. 강제 이동 불가 경로만 지도 밖을 공급한다.
        if extra==0 and hp==150:coordinate=COORDS[len(rows)%2]
        rows.append(dict(kind='Handle',authority=1,allocated=1,extra=extra,hp=hp,spot=spot,
            side=side,surface=surface,change=change,x=bits(coordinate[0]),y=bits(coordinate[1])))
    return rows


class FallOracle(OwnerOracle):
    """전체 원본 요청/분배/상태/지도/공유 생성자는 실행하고 미복원 효과만 대체한다."""
    def __init__(self, edition):
        """허용 몸체를 최신 내보내기로 제한하고 FS/코드/지도/반환 관찰 주소를 연결한다."""
        super().__init__(edition)
        self.f,self.s,self.p = FALL[edition],STATE[edition],POST[edition]
        self.exports = [ROOT / f'extracted/priestfall/{edition}/{n}' for n in ('creation.c','functions.tsv')]
        self.allowed,self.entries = [],set()
        with self.exports[-1].open(encoding='utf-8') as fp:
            # Ghidra가 보고한 불연속 함수 몸체 밖은 거부한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(0,0x1000);self.mu.mem_map(SPOTS,0x10000)
        self.mu.mem_map(TEMP,0x1000);self.mu.mem_map(SURFACES,0x20000)
        self.mu.mem_write(self.s['spot'],struct.pack('<I',SPOTS))
        self.mu.mem_write(self.f['surfaces'],struct.pack('<I',SURFACES))
        self.mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',TEMP+16)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)))
        self.stub_calls=collections.Counter();self.returns=0
        self.unpop=self.u32(self.s['vtable']+0x48);self.repop=self.u32(self.s['vtable']+0x4c)
        self.destroy=self.u32(self.s['vtable']+0x10)
        # 조회 결과 객체의 vtable만 합성한다. 실제 보호막 삭제 몸체는 실행하지 않는다.
        self.mu.mem_write(self.slot(60),struct.pack('<I',self.s['vtable']))

    def u32(self,address):
        """격리 주소에서 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def ret(self,purge=0,result=0):
        """대체 함수의 callee 스택 정리와 반환 주소를 원본 ABI로 유지한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);address=self.u32(sp)
        self.mu.reg_write(UC_X86_REG_EAX,result);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)
        self.mu.reg_write(UC_X86_REG_EIP,address)

    def change(self,stage):
        """외부 효과에 공급한 입력 변이만 쓴다. 이후 분기는 원본이 판단한다."""
        raw=self.slot(TARGET);c=self.case['change'];f=self.f
        if self.case['kind']=='Begin':
            if stage=='shield' and c==1: self.mu.mem_write(self.s['boss'],struct.pack('<I',1-self.case['authority']))
            if stage=='new' and c==2: self.mu.mem_write(self.s['boss'],struct.pack('<I',1-self.case['authority']))
            if stage=='repop' and c==3: self.mu.mem_write(raw+14,struct.pack('<ff',30.75,31.9))
            return
        if stage=='shield' and c==1:
            self.mu.mem_write(raw+f['frame'],struct.pack('<I' if self.stride==50 else '<B',1))
        if stage in ('set','advance') and c in (2,4):
            self.mu.mem_write(raw+14,struct.pack('<ff',30.75,31.9))
        if stage=='land' and c in (3,4):
            self.mu.mem_write(raw+self.o['extra'],b'\x00')
            self.mu.mem_write(raw+26,struct.pack('<I' if self.stride==50 else '<H',150))

    def on_instruction(self,mu,address,size,data):
        """실제 공유 생성과 명시 경계 인자/순서를 기록하며 다른 효과/OS 호출은 거부한다."""
        f=self.f;sp=mu.reg_read(UC_X86_REG_ESP);raw=self.slot(TARGET)
        if address==CALLER: return
        if CALLER<=address<CALLER+11: return
        if address==f['ctor']:
            event,parent,payload=struct.unpack('<III',mu.mem_read(sp+4,12))
            if mu.reg_read(UC_X86_REG_ECX)!=MEMORY or (event,parent,payload)!=(0x25b,TARGET,bits(0.01)):
                raise RuntimeError('실제 공유 생성 인자 오류')
            self.events.append(f'R:{parent}:{event}:{payload}')
        elif address==f['attach']:
            args=struct.unpack('<IIII',mu.mem_read(sp+4,16))
            if args!=(61,TARGET,0,0x50) or mu.reg_read(UC_X86_REG_ECX)!=MEMORY:
                raise RuntimeError('공유 프로세스 부착 인자 오류')
            self.stub_calls['attach']+=1;self.ret(16);return
        elif address==self.p['new']:
            if self.u32(sp+4)!=0x28: raise RuntimeError('확보 크기 오류')
            self.events.append('A:40');self.change('new');self.stub_calls['new']+=1
            self.ret(result=MEMORY if self.case['allocated'] else 0);return
        elif address in (f['shield'],self.unpop,self.repop,f['sound'],f['set'],f['advance'],f['clear']):
            if address!=f['sound'] and mu.reg_read(UC_X86_REG_ECX)!=raw: raise RuntimeError('낙하 효과 this 오류')
            purge=0
            if address==f['shield']: marker='S';self.events.append(f'S:{TARGET}');self.change('shield')
            elif address in (self.unpop,self.repop):
                marker='U' if address==self.unpop else 'P';flags=self.u32(sp+4)
                if flags!=(0 if marker=='U' else 0x800): raise RuntimeError('공간 flags 오류')
                self.events.append(f'{marker}:{TARGET}:{flags}');purge=4
                if marker=='P':self.change('repop')
            elif address==f['sound']:
                marker='W';x,y,name,flags=struct.unpack('<IIII',mu.mem_read(sp+4,16))
                if bytes(mu.mem_read(name,15))!=b'priestFall.wav\x00' or flags: raise RuntimeError('소리 인자 오류')
                self.events.append(f'W:{TARGET}:{x}:{y}')
            elif address==f['set']:
                marker='F';frame,flags=struct.unpack('<II',mu.mem_read(sp+4,8))
                self.events.append(f'F:{TARGET}:{frame}:{flags}');purge=8
                self.change('land' if frame==4 else 'set')
            elif address==f['advance']:
                marker='N';steps,flags=struct.unpack('<II',mu.mem_read(sp+4,8))
                self.events.append(f'N:{TARGET}:{steps}:{flags}');purge=8;self.change('advance')
            else:
                marker='X';self.events.append(f'X:{TARGET}')
            self.stub_calls[marker]+=1;self.ret(purge);return
        elif self.stride==50 and address==f['find']:
            self.events.append(f'X:{TARGET}');self.stub_calls['find']+=1;self.ret(result=60);return
        elif self.stride==50 and address==self.destroy:
            if mu.reg_read(UC_X86_REG_ECX)!=self.slot(60) or self.u32(sp+4):raise RuntimeError('보호막 삭제 인자 오류')
            self.stub_calls['destroy']+=1;self.ret(4);return
        OwnerOracle.on_instruction(self,mu,address,size,data)

    def run(self,case,control):
        """정상 반환/SEH/보존 레지스터/x87와 전체 슬롯·공유 생성 필드를 관찰한다."""
        self.case,self.events=case,[];raw=self.slot(TARGET);f,s=self.f,self.s
        data=bytearray(self.stride);struct.pack_into('<I',data,0,s['vtable'])
        data[10],data[self.o['owner']],data[self.o['extra']]=PRIEST_TYPE,1,case['extra']
        struct.pack_into('<II',data,14,case['x'],case['y'])
        struct.pack_into('<I' if self.stride==50 else '<H',data,26,case['hp'])
        self.mu.mem_write(raw,bytes(data));self.mu.mem_write(MEMORY,bytes(0x28))
        base=TYPES+PRIEST_TYPE*self.type_stride;self.mu.mem_write(base,bytes(self.type_stride))
        self.mu.mem_write(base,struct.pack('<I',200));self.mu.mem_write(base+0xe8,struct.pack('<II',0x69012,0x210000))
        self.mu.mem_write(base+0x124,struct.pack('<I',CODES));self.mu.mem_write(CODES,bytes([case['side'],80,1,0,74,80,1,0]))
        self.mu.mem_write(base+0x140,struct.pack('<I',4));self.mu.mem_write(base+0x154,struct.pack('<I',3))
        self.mu.mem_write(SPOTS,bytes([case['spot']])*65536)
        self.mu.mem_write(SURFACES,struct.pack('<H',case['surface'])*65536)
        self.mu.mem_write(s['quarter'],bytes(4));self.mu.mem_write(s['priest'],struct.pack('<I',PRIEST_TYPE))
        self.mu.mem_write(s['boss'],struct.pack('<I',case['authority']))
        # 패치의 보호막 유효 주소 범위 검사도 실행한다. SID 60은 실제 에뮬레이터 풀 안에 있다.
        if self.stride==50:self.mu.mem_write(0x5c847c,struct.pack('<I',CAPACITY))
        self.mu.mem_write(f['shared'],struct.pack('<I',61));self.mu.mem_write(f['now'],struct.pack('<d',12.5))
        self.mu.mem_write(0x5c8488 if self.stride==50 else 0x539600,bytes(4))
        self.mu.mem_write(0x5e4794 if self.stride==50 else 0x546778,bytes(4));self.mu.mem_write(0,struct.pack('<I',0xffffffff))
        self.write_ranges=[(0,4),(raw,raw+self.stride),(MEMORY,MEMORY+0x28),(TEMP+16,TEMP+20)]
        saved=(0x1234,0x2345,0x3456,0x4567)
        for reg,value in zip((UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP),saved):self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,raw)
        begin=case['kind']=='Begin'
        self.mu.mem_write(STACK,struct.pack('<I',STOP) if begin else struct.pack('<IIII',CALLER,0x25b,0xabcdef01,0x7fc12345))
        self.mu.emu_start(f['begin'] if begin else f['dispatch'],STOP,count=30000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+(4 if begin else 16):raise RuntimeError('정상 반환/스택 오류')
        if tuple(self.mu.reg_read(r) for r in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP))!=saved:raise RuntimeError('보존 레지스터 오류')
        if self.u32(0)!=0xffffffff or self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('SEH/x87 복구 오류')
        if begin and case['allocated']:
            if self.u32(MEMORY+24)!=0x25b or self.u32(MEMORY+28)!=bits(0.01) or self.u32(MEMORY+32)!=0 or struct.unpack('<d',self.mu.mem_read(MEMORY+16,8))[0]!=12.5:raise RuntimeError('실제 공유 생성 필드 오류')
            if self.u32(self.u32(MEMORY)+0x18)!=(0x496cb0 if self.stride==50 else 0x48f0e0):raise RuntimeError('공유/일반 RunFrame 연결 오류')
        self.returns+=1
        return [';'.join(self.events) or '-',bytes(self.mu.mem_read(raw,self.stride)).hex(),'-' if begin else self.u32(TEMP+16),self.u32(s['boss'])]


def generate(smoke=False):
    """독립 원본 결과와 SHA/실제 실행 및 대체 경계를 저장한다. 두 x87 정밀도를 비교한다."""
    cases=inputs();cases=cases[::97] if smoke else cases;rows=[];reports={}
    paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_prieststate_oracle.py',
        ROOT/'tools/decomp_priestpostpop_oracle.py',ROOT/'tools/ghidra/priestfall-functions.json',FIXTURE}
    for edition in SPECS:
        oracle=FallOracle(edition)
        # 동일 입력의 두 정밀도 관찰이 일치해야 저장한다. fixture에는 중복 없이 한 행만 넣는다.
        for case in cases:
            outputs=[oracle.run(case,c) for c in CONTROLS]
            if outputs[0]!=outputs[1]:raise RuntimeError('정밀도별 결과 차이')
            rows.append([edition,*[case[k] for k in KEYS],*outputs[0]])
        reports[edition]=dict(binary=oracle.spec['binary'],cases=len(cases),executions=oracle.returns,
            native_calls=dict(oracle.native_calls),stub_calls=dict(oracle.stub_calls),assertions=oracle.assertions,instructions=oracle.instructions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary'])
        print(f'{edition}: {len(cases)}개 입력 / {oracle.returns}회 정상 반환',flush=True)
    if smoke:return
    FIXTURE.write_text('# 사제 낙하 요청/0x25b 원본 관찰. 공유 생성자는 실제 실행, 부착/공간/소리/보호막은 명시 대체.\n'
        '# edition '+ ' '.join(KEYS)+' events raw result authority_after\n'+ '\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(schema=1,primary_target='10.78',last_decompiled_host='HJOW-Athlon',date='2026-10-10',total=len(rows),
        controls=list(CONTROLS),editions=reports,os_calls=0,stopped_before=None,
        stubbed=['new(0x28)','BaseProcess Attach','ensure/clear shield','Unpop/Repop','set/advance frame','positional sound','patch shield find/destroy'],
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA·입력 순서/폭·행 수·정상 반환 수와 실제 공유 생성 실행을 검사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for path,value in report['files'].items():
        if digest(ROOT/path)!=value:raise RuntimeError(f'SHA 불일치: {path}')
    rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    expected=[[e,*map(str,(c[k] for k in KEYS))] for e in SPECS for c in inputs()]
    if len(rows)!=report['total'] or [r[:12] for r in rows]!=expected or any(len(r)!=16 for r in rows):raise RuntimeError('fixture 입력/폭 오류')
    for edition,r in report['editions'].items():
        if r['executions']!=len(inputs())*2 or r['assertions'] or not r['native_calls'].get(f'{FALL[edition]["ctor"]:08x}'):raise RuntimeError('실제 생성/정상 반환 근거 오류')
        if edition=='originals' and r['stub_calls'].get('destroy')!=r['stub_calls'].get('find'):raise RuntimeError('유효 보호막 가상 삭제 근거 오류')
    print(f'사제 낙하 감사: {len(rows)}개 입력, 세 PE·두 정밀도·SHA 통과')


def main():
    """일반 생성·읽기 전용 감사·축소 기계어 확인을 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
