#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 실제 WAVE 열기/헤더 보정/표본 읽기/닫기를 정상 반환까지 실행한다.

Winmm mmio 경계만 메모리 RIFF로 대체한다. 장치·게임·OS를 실행하지 않는다.
fixture는 C++가 실제 mmio로 읽을 WAV 바이트와 원본이 반환한 형식/위치/표본이다.
DirectSound 초기화·파일 검색·COM 버퍼는 이 독립 기대값의 범위에 포함하지 않는다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, STOP, STACK, digest, UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS
import pefile

# 파일명·반환 포인터·표본을 두는 독립 작업 메모리와 Winmm 대체 진입점이다.
WORK, STUBS = 0x15200000, STOP+0x800
# 세 판본의 실제 WAVE 열기/읽기/닫기 함수다.
BODY = {'originals': (0x4a8b50,0x4a8cf0,0x4a8d30), 'originalCD': (0x4375c0,0x437790,0x437800)}
BODY['original1037'] = BODY['originalCD']
# Winmm stdcall 인자 수다. 이 경계 밖의 외부 명령은 허용하지 않는다.
IMPORTS = {'mmioOpenA':3,'mmioDescend':4,'mmioRead':3,'mmioAscend':3,'mmioClose':2}
# fixture와 근거 기록은 기존 감사 파일과 분리한다.
FIXTURE = ROOT/'cpppj/tests/fixtures/sounddevice-x86.tsv'
REPORT = ROOT/'cpppj/recovery-sounddevice-evidence.json'


def chunk(name, data):
    """청크와 홀수 크기의 RIFF 패딩을 만든다. 기대 결과는 계산하지 않는다."""
    return name+struct.pack('<I',len(data))+data+(b'\0' if len(data)%2 else b'')


def inputs():
    """16/17/18/확장 fmt·다양한 PCM 수치·청크 패딩·빈 data·누락 fmt/data 입력이다."""
    cases=[]
    # 평균 바이트율과 bits가 불일치하는 짧은 fmt도 넣어 원본 보정을 관찰한다.
    for channels,rate,bits,length,size,junk in itertools.product((1,2),(11025,22050,44100),(8,16),(16,17,18,24),(0,1,8),(0,1)):
        align=channels*bits//8
        # 실제 PCM 자산 네 개의 불필요한 cbSize=20도 헤더 바이트 그대로 관찰한다.
        for extra in ((0,20) if length>=18 else (0,)):
            fmt=struct.pack('<HHIIHHH',1,channels,rate,rate*align,align,99 if length<18 else bits,extra)
            fmt=fmt[:length] if length<=18 else fmt+bytes(length-18)
            body=b'WAVE'+(chunk(b'JUNK',b'X') if junk else b'')+chunk(b'fmt ',fmt)+chunk(b'data',bytes(range(size)))
            cases.append(b'RIFF'+struct.pack('<I',len(body))+body)
    # 명시적으로 실패하는 청크 누락과 비WAVE 입력도 정상 반환까지 실행한다.
    for body in (b'WAVE'+chunk(b'data',b'ab'),b'WAVE'+chunk(b'fmt ',struct.pack('<HHIIHH',1,1,22050,22050,1,8)),b'AVI '):
        cases.append(b'RIFF'+struct.pack('<I',len(body))+body)
    return cases


class WaveOracle(OwnerOracle):
    """원본 WAVE 처리 몸체만 실행하며 mmio의 파일 위치/청크 동작을 독립 메모리로 제공한다."""
    def __init__(self, edition):
        """PE·내보낸 범위·실제 Winmm IAT 대체를 준비한다."""
        super().__init__(edition)
        self.mu.mem_map(WORK,0x10000)
        self.exports=[ROOT/f'extracted/sounddevice/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        # 실제 불연속 함수 범위만 허용한다.
        for row in csv.DictReader(self.exports[1].open(encoding='utf-8'),delimiter='\t'):
            self.entries.add(int(row['entry'],16))
            # 함수 사이의 임의 코드 진입은 거부한다.
            for part in row['ranges'].split(';'):
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        pe=pefile.PE(str(ROOT/self.spec['binary']));self.stubs={}
        # 실제 IAT의 Winmm 주소만 가상 진입점으로 치환한다.
        for directory in pe.DIRECTORY_ENTRY_IMPORT:
            # 이 oracle이 허용한 다섯 API 외에는 치환하지 않는다.
            for item in directory.imports:
                name=item.name.decode() if item.name else ''
                if name in IMPORTS:
                    stub=STUBS+len(self.stubs)*16;self.stubs[stub]=name
                    self.mu.mem_write(item.address,struct.pack('<I',stub))
        self.substitutions=collections.Counter();self.returns=collections.Counter()

    def args(self, count):
        """현재 stdcall의 DWORD 인자를 읽는다."""
        return struct.unpack(f'<{count}I',self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def leave(self, value, count):
        """API를 stdcall 정상 반환시킨다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);target=struct.unpack('<I',self.mu.mem_read(esp,4))[0]
        self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.mu.reg_write(UC_X86_REG_ESP,esp+4+count*4);self.mu.reg_write(UC_X86_REG_EIP,target)

    def mmio(self, name):
        """파일 시스템/Winmm 대신 RIFF 청크와 읽기 위치를 공급한다. 헤더 보정은 실제 PE가 수행한다."""
        args=self.args(IMPORTS[name]);result=0
        if name=='mmioOpenA':
            if args[2]!=0x10000: raise RuntimeError('mmio 열기 플래그 불일치')
            self.position=0;self.opened=True;result=7
        elif name=='mmioClose':
            if not self.opened: raise RuntimeError('mmio 이중 닫기')
            self.opened=False
        elif name=='mmioRead':
            handle,pointer,size=args
            if handle!=7 or not self.opened: raise RuntimeError('읽기 핸들 불일치')
            data=self.file[self.position:self.position+size];self.mu.mem_write(pointer,data);self.position+=len(data);result=len(data)
        elif name=='mmioDescend':
            handle,pointer,parent,flags=args
            if handle!=7 or not self.opened: raise RuntimeError('청크 핸들 불일치')
            found=None
            if flags==0x20:
                if self.file[:4]==b'RIFF' and self.file[8:12]==b'WAVE': found=(b'RIFF',len(self.file)-8,b'WAVE',8,12)
            elif flags==0x10:
                wanted=bytes(self.mu.mem_read(pointer,4));offset=max(self.position,12)
                # fmt/data 찾기의 청크 순서와 홀수 패딩은 Winmm 경계에서 공급한다.
                while offset+8<=len(self.file):
                    size=struct.unpack_from('<I',self.file,offset+4)[0]
                    if self.file[offset:offset+4]==wanted: found=(wanted,size,b'\0'*4,offset+8,offset+8);break
                    offset+=8+size+(size%2)
            else: raise RuntimeError('청크 탐색 플래그 오류')
            if found:
                kind,size,form,data,pos=found;self.mu.mem_write(pointer,kind+struct.pack('<I',size)+form+struct.pack('<II',data,0));self.position=pos
            else: result=1
        elif name=='mmioAscend':
            _,pointer,_=args
            _,size,_,offset,_=struct.unpack('<IIIII',self.mu.mem_read(pointer,20));self.position=offset+size+(size%2)
        self.leave(result,IMPORTS[name])

    def on_instruction(self, mu, address, size, data):
        """허용한 Winmm 경계만 대체하고 나머지는 실제 코드 범위/원본 assert 0을 검사한다."""
        if address in self.stubs:
            name=self.stubs[address];self.substitutions[name]+=1;self.mmio(name)
        else: super().on_instruction(mu,address,size,data)

    def call(self, entry, args):
        """보존 레지스터·cdecl 스택·정상 반환을 확인하며 실제 함수를 호출한다."""
        preserved={UC_X86_REG_EBX:0x12340001,UC_X86_REG_ESI:0x12340002,UC_X86_REG_EDI:0x12340003,UC_X86_REG_EBP:0x12340004}
        # 호출 사이에 임의 레지스터를 보존한다고 가정하지 않는다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX): self.mu.reg_write(register,0)
        # 원본 ABI의 callee-saved 값을 검사한다.
        for register,value in preserved.items(): self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.mem_write(STACK,struct.pack(f'<{len(args)+1}I',STOP,*args))
        self.mu.emu_start(entry,STOP,count=20000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4: raise RuntimeError('정상 cdecl 반환 실패')
        if any(self.mu.reg_read(r)!=v for r,v in preserved.items()): raise RuntimeError('보존 레지스터 오류')
        self.returns[f'{entry:08x}']+=1;return self.mu.reg_read(UC_X86_REG_EAX)

    def run(self, file):
        """열기 성공 시 실제 표본 읽기/닫기도 실행하고 반환 바이트를 관찰한다."""
        self.file=file;self.opened=False;self.position=0;self.mu.mem_write(WORK,bytes(0x1000));self.mu.mem_write(WORK,b'case.wav\0')
        self.write_ranges=[(WORK,WORK+0x1000)]
        open_body,read_body,close_body=BODY[self.edition]
        result=self.call(open_body,(WORK,WORK+0x100,WORK+0x104,WORK+0x108,WORK+0x120))
        if not result:
            if self.opened: raise RuntimeError('실패 경로의 파일 미반납')
            return (0,'-',0,'-')
        size=struct.unpack('<I',self.mu.mem_read(WORK+0x100,4))[0]
        if self.call(read_body,(WORK+0x200,size,7))!=1: raise RuntimeError('실제 표본 읽기 실패')
        fmt=bytes(self.mu.mem_read(WORK+0x108,18)).hex();offset=struct.unpack('<I',self.mu.mem_read(WORK+0x120,4))[0]
        samples=bytes(self.mu.mem_read(WORK+0x200,size)).hex()
        self.call(close_body,(WORK+0x104,))
        if self.opened or bytes(self.mu.mem_read(WORK+0x104,4))!=bytes(4): raise RuntimeError('실제 닫기 핸들 초기화 오류')
        return (1,fmt,offset,samples)


def generate():
    """독립 원본 관찰과 SHA/입력/진입/반환 근거를 저장한다."""
    rows=[];editions={};paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/sounddevice-functions.json',ROOT/'tools/decomp_owner_oracle.py'}
    # 세 판본을 별도로 실행하여 입력마다 원본 출력만 기록한다.
    for edition in SPECS:
        oracle=WaveOracle(edition)
        # C++와 같은 기대 결과 계산식을 사용하지 않는다.
        for file in inputs(): rows.append([edition,file.hex(),*oracle.run(file)])
        editions[edition]=dict(cases=len(inputs()),native_calls=dict(oracle.native_calls),returns=dict(oracle.returns),substitutions=dict(oracle.substitutions),assertions=oracle.assertions,instructions=oracle.instructions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary'])
        print(f'{edition}: 실제 WAVE 입력 {len(inputs())}개 정상 반환',flush=True)
    FIXTURE.write_text('# 세 실제 PE의 WAVE 열기/읽기/닫기. Winmm 파일 경계만 메모리 RIFF로 대체한다.\n# edition wavHex success formatHex dataOffset samplesHex\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-10',total=len(rows),os_calls=0,editions=editions,
        stubbed=list(IMPORTS),limitations=['DirectSound 초기화/버퍼/파일 검색은 독립 대조 범위 밖','온전한 PCM fmt 16/17/18/24바이트·누락 청크·빈 data','잘린/코덱 확장/제로 분모 입력은 C++ 별도 안전 검사'],
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """실제 입력·원본 SHA·모든 함수 진입/정상 반환·assert/OS 0과 성공 시 읽기/닫기 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));cases=inputs()
    if report['schema']!=1 or report['os_calls'] or report['total']!=len(cases)*3 or set(report['editions'])!=set(SPECS): raise RuntimeError('감사 스키마/판본/개수 오류')
    # 실행기/PE/내보내기/fixture가 기록 당시와 같은지 검사한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError(f'SHA 불일치: {name}')
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total']: raise RuntimeError('fixture 행 개수 오류')
    # 성공한 열기 뒤에는 실제 읽기/닫기가 각각 한 번이어야 한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];success=sum(int(row[2]) for row in selected)
        if [row[1] for row in selected]!=[case.hex() for case in cases] or item['cases']!=len(cases) or item['assertions']: raise RuntimeError('입력/원본 assert 오류')
        expected={f'{BODY[edition][0]:08x}':len(cases),f'{BODY[edition][1]:08x}':success,f'{BODY[edition][2]:08x}':success}
        if item['native_calls']!=expected or item['returns']!=expected: raise RuntimeError('실제 진입/반환 개수 오류')
        expected_api={'mmioOpenA':len(cases),'mmioClose':len(cases),'mmioRead':len(cases)-2+success,'mmioAscend':len(cases)-2,'mmioDescend':3*(len(cases)-3)+6}
        if item['substitutions']!=expected_api: raise RuntimeError(f'Winmm 경계 개수 오류: {item["substitutions"]} vs {expected_api}')
    print(f'WAVE 원본 감사 통과: {report["total"]}개, 세 몸체 정상 반환·원본 assert/OS 0')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()
