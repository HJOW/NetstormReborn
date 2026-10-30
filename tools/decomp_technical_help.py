#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Technical Help의 두 부모와 외부 파일 선택 부모를 독립 관측한다."""
import argparse
import csv
import hashlib
import json
import socket
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EFLAGS


class ObservationStop(Exception):
    """효과 진입 전 원문 prefix 관측을 끝낸다."""


class Oracle(MemoryOracle):
    """부모 분기/복사는 실제 CPU이고 Config/파일/실행은 명시 응답이다."""
    def __init__(self):
        """같은 PC의 정밀 표 범위와 독립 SEH 첫 슬롯을 준비한다."""
        super().__init__(); self.mu.mem_map(0, 0x1000)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        for entry in (0x435e40, 0x435ef0, 0x435f10, 0x41ad10, 0x459c60):  # 실제 부모와 선택된 경로 조합 자식의 떨어진 범위를 그대로 사용한다.
            for part in facts[entry]['ranges'].split(';'):  # 함수 밖 명령으로 새지 않게 한다.
                low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high+1))

    def event(self, value):
        """효과 진입만 기록하고 선택된 중단 전까지의 순서를 보존한다."""
        self.trace.append(value)
        if self.stop and len(self.trace) == self.stop: raise ObservationStop()

    def instruction(self, mu, address, size, data):
        """실제 부모의 출력/복사/반환은 CPU가 수행하며 외부 경계만 공급한다."""
        args = self.words(mu.reg_read(UC_X86_REG_ESP)+4, 5); ecx = mu.reg_read(UC_X86_REG_ECX)
        if address == 0x441300:
            key = self.string(args[0]); self.event('R,'+key.hex()+','+self.string(args[1]).hex()); index = int(key == b'techHelpFile')
            mu.mem_write(args[1], self.config[index]+b'\0'); self.ret(self.returns[index])
        elif address == 0x41ad10 and self.mode != 'path':
            assert args[1] == 0
            self.event('Q,'+self.string(args[0]).hex()); value = self.resolved[min(self.resolve_index, 1)]; self.resolve_index += 1
            pointer = 0 if value is None else TEXT+0x4000+self.resolve_index*0x1000
            if pointer: mu.mem_write(pointer, value+b'\0')
            self.ret(pointer)
        elif address == 0x4dd6b0:
            assert args[2] == 0; self.event('L,'+self.string(args[0]).hex()+','+self.string(args[1]).hex()); self.ret()
        elif address == 0x45a220:
            self.spec_name = self.string(args[0]); self.ret(ecx, 4)
        elif address == 0x459900:
            self.event('D,'+self.spec_name.hex()); mu.mem_write(TEXT+0x9000, b'X\0' if self.flags[0] else b'\0'); self.ret(TEXT+0x9000)
        elif address == 0x459c60 and self.mode != 'join':
            assert args[0] == 0xfffffffe and args[2] == 0xfffffffd and args[4] == 0
            secondary = args[1] == 0x544e80; assert args[1] in (0x544d80, 0x544e80)
            self.event('P,'+str(int(secondary))+','+self.string(args[3]).hex()); self.current_disk = int(secondary)
            pointer = TEXT+0x6000+int(secondary)*0x1000; mu.mem_write(pointer, (b'D:\\disc\\' if secondary else b'C:\\base\\')+self.spec_name+b'\0'); self.ret(pointer)
        elif address == 0x4e545a:
            assert self.string(args[1]) == b'rb'; self.event('F,'+self.string(args[0]).hex()); self.ret(0x800 if self.flags[3 if self.current_disk else 1] else 0)
        elif address == 0x4e54b9:
            assert args[0] == 0x800; self.ret()
        elif address == 0x41aae0:
            self.event('F,'+self.string(args[0]).hex()); self.ret(self.flags[3])
        elif address == 0x41a340:
            secondary = ecx == 0x544f80; assert ecx in (0x5450b8, 0x544f80) and args[2] == 0
            self.event('A,'+str(int(secondary))+','+self.string(args[0]).hex()); self.ret(self.flags[4 if secondary else 2], 12)
        elif address == 0x4595f0: self.ret()
        elif address == 0x4e63e0:
            value=self.string(args[0]);byte=args[1]&255;index=len(value) if not byte else value.find(bytes([byte]));self.ret(args[0]+index if index>=0 else 0)
        elif not any(low <= address < high for low, high in self.allowed): raise RuntimeError(f'검토하지 않은 Technical Help 명령 {address:08x}')

    def prepare(self):
        """이전 관측의 레지스터/스택/SEH/호출 순서를 섞지 않는다."""
        self.trace = []; self.resolve_index = 0; self.put(0, 0); self.put(STACK, STOP)
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP): self.mu.reg_write(register, 0)  # 독립 caller 입력이다.
        self.mu.reg_write(UC_X86_REG_ESP, STACK); self.mu.reg_write(UC_X86_REG_EFLAGS, 2)

    def technical(self, name, mode, returns, config, resolved, stop):
        """가용성/실행/직접 출력 부모를 같은 명시 응답으로 관측한다."""
        self.prepare(); self.mode = mode; self.returns = returns; self.config = config; self.resolved = resolved; self.stop = stop
        self.mu.mem_write(TEXT+0x1000, b'OLD-EXE\0'); self.mu.mem_write(TEXT+0x2000, b'OLD-FILE\0')
        if mode == 'read': self.mu.reg_write(UC_X86_REG_EAX, TEXT+0x1000); self.mu.reg_write(UC_X86_REG_EBX, TEXT+0x2000)
        completed = True
        try: self.mu.emu_start({'read':0x435e40,'available':0x435ef0,'launch':0x435f10}[mode], STOP, count=10000)
        except ObservationStop: completed = False
        encoded = lambda value: 'NULL' if value is None else value.hex() or '-'
        return [name,mode,*map(str,returns),*map(encoded,config),*map(encoded,resolved),str(stop),str(self.mu.reg_read(UC_X86_REG_EAX) if completed else 0),str(int(completed)),encoded(self.string(TEXT+0x1000)) if mode=='read' else '-',encoded(self.string(TEXT+0x2000)) if mode=='read' else '-',';'.join(self.trace)]

    def path(self, name, flags, output):
        """파일 선택 부모의 다섯 출처와 실패 시 출력 보존을 관측한다."""
        self.prepare(); self.mode = 'path'; self.flags = flags; self.stop = 0; self.mu.mem_write(TEXT+0x1000, b'manual.hlp\0'); self.put(TEXT+0x2000, 77)
        self.put(STACK+4, TEXT+0x1000); self.put(STACK+8, TEXT+0x2000 if output else 0); self.mu.emu_start(0x41ad10, STOP, count=10000)
        pointer = self.mu.reg_read(UC_X86_REG_EAX)
        return [name,b'manual.hlp'.hex(),*map(str,flags),str(output),'NULL' if not pointer else self.string(pointer).hex() or '-',str(self.words(TEXT+0x2000,1)[0]),';'.join(self.trace)]

    def join(self, root, name):
        """실제459c60의 -2/root/-3/name/0 조합 결과를 관측한다."""
        self.prepare();self.mode='join';self.stop=0;self.put(0x55b160,0);self.mu.mem_write(TEXT+0x1000,root+b'\0');self.mu.mem_write(TEXT+0x2000,name+b'\0')
        for index,value in enumerate((0xfffffffe,TEXT+0x1000,0xfffffffd,TEXT+0x2000,0)): self.put(STACK+4+index*4,value)  # 실제 가변 인자의 호출 순서다.
        self.mu.emu_start(0x459c60,STOP,count=20000);result=self.string(self.mu.reg_read(UC_X86_REG_EAX));return [root.hex() or '-',name.hex() or '-',result.hex() or '-']


def cases():
    """정상 bool/원시 AND·복사/NUL/효과 중단을 독립 입력으로 나열한다."""
    for mode in ('read','available','launch'):  # 래퍼는 실제 본문을 실행하고 호출 차이를 구별한다.
        for first, second in ((0,0),(0,1),(1,0),(1,1),(2,4),(2,3),(0xffffffff,2)):  # bitwise AND를 논리 AND로 바꾸지 않는다.
            for resolved in ((None,b'doc'),(b'C:\\help.exe',None),(b'C:\\help.exe',b'D:\\manual.hlp')):  # 첫/둘째 실패와 정상 복사를 분리한다.
                yield f'{mode}-{first}-{second}-{resolved[0] is None}-{resolved[1] is None}',mode,(first,second),(b'help.exe',b'manual.hlp'),resolved,0
        for value in (b'',b'new\0hidden'):  # 비null 빈 결과와 첫NUL 복사다.
            yield f'{mode}-value-{value.hex()}',mode,(1,1),(b'help.exe',b'manual.hlp'),(value,value),0
        for stop in range(1,6): yield f'{mode}-stop-{stop}',mode,(1,1),(b'help.exe',b'manual.hlp'),(b'program',b'document'),stop  # 실제 효과 진입 prefix다.


def sha(path):
    """최초 관측과 보호 소스를 원시 바이트로 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """독립 두 관측의 일치 뒤 최초 기대 파일을 새로 한 번만 만든다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify-existing',action='store_true');args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    rows=[]
    for replay in range(2):  # 독립 emulator 메모리에서 동일 입력을 다시 관측한다.
        oracle=Oracle();technical=[oracle.technical(*case) for case in cases()];paths=[oracle.path(f'path-{mask}-{output}',tuple((mask>>bit)&1 for bit in range(5)),output) for mask in range(32) for output in (0,1)]
        joins=[oracle.join(root,name) for root in (b'',b'base',b'C:\\base',b'C:\\base\\',b'C:\\base\\\\',b'\\',b'\\\\server\\share') for name in (b'',b'file.hlp',b'\\child\\',b'child//')]
        rows.append((technical,paths,joins));(out/f'{replay}.json').write_text(json.dumps(rows[-1],indent=2)+'\n',encoding='utf-8',newline='\n')
    assert rows[0]==rows[1];fixtures=[]
    for name, table in zip(('technical-help-x86.tsv','external-file-x86.tsv','external-file-join-x86.tsv'),rows[0]):  # 처음 관측을 기존 파일 위에 덮지 않는다.
        fixture=ROOT/'cpppj/tests/fixtures'/name
        content=''.join('\t'.join(row)+'\n' for row in table).encode('utf-8')
        if args.verify_existing and fixture.exists(): assert fixture.read_bytes()==content,name
        else:
            with fixture.open('xb') as stream: stream.write(content)
        fixtures.append(fixture.relative_to(ROOT).as_posix())
    sources=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','extracted/refined/originals/Netstorm.c','tools/decomp_priestmenu.py','tools/decomp_technical_help.py',*fixtures]
    report={'unit':602,'host':socket.gethostname(),'date':'2026-10-30','lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','technicalCases':len(rows[0][0]),'pathCases':len(rows[0][1]),'joinCases':len(rows[0][2]),'observations':sum(map(len,rows[0]))*2,'differences':[],'gameStarted':False,'scope':'435e40/435ef0/435f10/41ad10 및 별도459c60 실제 CPU. 부모 내 Config/경로/Filespec·조합/CRT파일/TAFF/실행은 명시 응답 경계. 파일 부모와 자식의 전체 통합/OS 동일성 증거는 아니다.','verifiedExistingFixtures':args.verify_existing,'sourceSha256':{p:sha(ROOT/p) for p in sources},'evidenceSha256':{p.name:sha(p) for p in out.iterdir() if p.is_file()}}
    (ROOT/'cpppj/technical-help-native-20261030.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('technicalCases','pathCases','observations','differences')}))


if __name__=='__main__':
    main()
