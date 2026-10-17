#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""커서 마나의 실제 생성·프레임·통지·소멸과 현재 전역/가상 효과를 격리 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_particle_gradient import Oracle as CallOracle, ROOT, STACK, STOP, digest
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

POOL, TYPES, PROCESS, VTABLE, POINT = 0x12000000, 0x12100000, 0x12200000, 0x12300000, 0x12400000  # raw/TYPE/원본 필드/가상 경계/중심점의 격리 영역이다.
DETACH, UNPOP, DESTROY, POSITION, KILL_RETURN = (STOP+0x100+32*index for index in range(5))  # 서로 다른 실제 가상 효과 경계다.
ENTRIES = (0x4741d0, 0x4737f0, 0x473d00, 0x473c30, 0x473b90, 0x474b60, 0x472e60, 0x49a840, 0x4ac220, 0x41bfb0, 0x41d150, 0x40e800, 0x446040, 0x41bd00, 0x4e49c0)  # 대체하지 않고 실행하는 실제 몸체다.
FIXTURE = ROOT/'cpppj/tests/fixtures/cursor-mana-x86.tsv'  # 수명/사정거리 원문과 별개인 새 관측이다.
REPORT = ROOT/'cpppj/cursor-mana.json'  # 현재 PC·정밀도·범위·정상 반환과 SHA 근거다.


def bits(value):
    """명시 float 입력을 손실 없는 DWORD로 기록한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def time_words(value):
    """명시 double 입력을 두 DWORD로 기록한다."""
    return list(struct.unpack('<II', struct.pack('<d', value)))


def cases():
    """입력만 조합하며 이동/판정/소멸 기대값은 계산하지 않는다."""
    base = [0, 0, 3, 4, 8, 5, 0, bits(4), bits(22), 0, 0, 1, 0, 0, 0, 0, 0, 1, 256, 0, 1, 3, *time_words(5.25), *time_words(5), *time_words(0), bits(100), bits(110), bits(99), bits(108), bits(101), bits(111), bits(22), bits(1)]  # 입력36열이며 전역 반경과 객체 반경을 분리한다.
    result = []  # 중복은 마지막에 제거한다.
    # 생성은 현재 theme/발자국과 가상 효과·Attach 뒤 시각 변경을 교차한다.
    for theme, foot, mutation in itertools.product(range(11), ((1, 1), (3, 4), (-1, 0)), (0, 2, 6)):
        row = base.copy(); row[1:4] = theme, *foot; row[19] = mutation; result.append(row)
    # 원형 프레임은 개수·반경·시간 차·종료·숨김을 독립적으로 바꾼다.
    for count, radius, delta, stopping, hidden in itertools.product((0, 1, 8, 30), (0, 4, 22), (0, .125, 2), (0, 1), (0, 1)):
        row = base.copy(); row[0] = 1; row[4] = count; row[7] = bits(radius); row[9:11] = stopping, hidden; row[24:26] = time_words(5.25-delta); result.append(row)
    # 실제 _ftol EAX와 엄격한 expiry 비교·클램프를 방문한다.
    for now, mutation in itertools.product((-.125, 0, .5, 2.999999, 3, 1000000.25, 2147483648.125, 4294967296.25, 9.223372036854776e18), (0, 1, 5)):
        row = base.copy(); row[0] = 1; row[19] = mutation; row[22:24] = time_words(now); row[26:28] = time_words(3); result.append(row)
    # 사각형은 일부/전체 SID0·범위 밖 삭제·현재 위상/변별 개수와 자기 삭제 뒤 쓰기다.
    for edges, delta, zeros, limit, flags in itertools.product((0, 3, 5), (0, .125, 2), (0, 0x3fffffff), (0, 22), (0, 4)):
        row = base.copy(); row[0] = 1; row[4:7] = 20, edges, 1; row[8] = bits(limit); row[13] = flags; row[14] = zeros; row[24:26] = time_words(5.25-delta); result.append(row)
    # 통지의 event/info DWORD·현재 void와 개수 경계다.
    for event, info, count, flags in itertools.product((0, 1, 3, 9), (0, 1, 0xffffffff), (0, 1, 8, 30), (0, 4, 3)):
        row = base.copy(); row[0] = 2; row[4] = count; row[11:14] = event, info, flags; result.append(row)
    # 소멸은 현재 토큰·SID0·dead/free와 콜백 뒤 개수 변경을 검사한다.
    for count, zeros, dead, token, mutation in itertools.product((0, 1, 8, 30), (0, 1, 0x3fffffff), (0, 0x55555555), (0, 1, 2), (0, 3)):
        row = base.copy(); row[0] = 3; row[4] = count; row[14] = zeros; row[16] = dead; row[17] = token; row[19] = mutation; result.append(row)
    # 원본 new 성공/실패와 실제 생성 후 여러 프레임·종료 통지·소멸의 연속 호출이다.
    for operation, mutation, success in itertools.product((4, 5), (0, 2, 6), (0, 1)):
        row = base.copy(); row[0] = operation; row[19:21] = mutation, success; result.append(row)
    return list(dict.fromkeys(tuple(value&0xffffffff for value in row) for row in result))


class Oracle(CallOracle):
    """실제 마나 몸체·보조 조회/클램프/절삭을 쓰고 할당·Host·객체 가상 효과만 기록한다."""
    def __init__(self):
        """PE·스택·입력 풀을 격리하고 정밀표 범위/읽기/쓰기를 제한한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32); image = pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image()
        self.image_end = 0x400000+((len(image)+4095)&~4095); self.mu.mem_map(0x400000, self.image_end-0x400000); self.mu.mem_write(0x400000, image)
        # SEH는 FS:0의 DWORD 하나만 허용한다.
        for address, size in ((0, 0x1000), (POOL, 0x10000), (TYPES, 0x20000), (PROCESS, 0x1000), (VTABLE, 0x1000), (POINT, 0x1000), (STACK-0x7f000, 0x80000), (STOP, 0x1000)):
            self.mu.mem_map(address, size)
        self.put(VTABLE+0x48, DETACH); self.put(VTABLE+0x4c, UNPOP); self.put(VTABLE+0x10, DESTROY); self.put(VTABLE+0x90, POSITION)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []  # 명령 마지막 바이트까지 함수 구간에 포함돼야 한다.
        # 실제 실행 본문만 허용하며 알려지지 않은 호출은 중단한다.
        for entry in ENTRIES:
            for part in rows[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high+1))
        self.instructions = self.returns = 0; self.calls = Counter(); self.events = []; self.case = None
        self.mu.hook_add(UC_HOOK_CODE, self.instruction); self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write); self.mu.hook_add(UC_HOOK_MEM_READ, self.read)
        self.call(0x472e60, 0, b'', 0, 0x027f); self.table = [self.get(0x592d60+letter*4) for letter in b'wrtsWRTS.?>']

    def put(self, address, value, width=4):
        """명시 입력과 효과 경계의 원본 폭만 쓴다."""
        self.mu.mem_write(address, (value&((1 << (8*width))-1)).to_bytes(width, 'little'))

    def get(self, address):
        """현재 비정렬 DWORD를 읽는다."""
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def ret(self, value=0, purge=0):
        """cdecl/thiscall 경계의 실제 스택 정리 규약으로 복귀한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP); target = self.get(esp); self.mu.reg_write(UC_X86_REG_EAX, value&0xffffffff); self.mu.reg_write(UC_X86_REG_ESP, esp+4+purge); self.mu.reg_write(UC_X86_REG_EIP, target)

    def event(self, kind, *values):
        """호출 인수의 전체 DWORD 비트를 순서대로 저장한다."""
        self.events.append(':'.join(map(str, (kind, *values))))

    def mutate(self, kind):
        """명시 입력의 첫 효과만 현재 시간/개수에 영향을 주며 계산 결과를 공급하지 않는다."""
        if self.mutated: return
        mode = self.case[19]
        if (mode, kind) not in ((1, 'D'), (2, 'V'), (3, 'X'), (4, 'D'), (5, 'O'), (6, 'A')): return
        self.mutated = True
        if mode in (1, 2, 3, 4, 5): self.put(PROCESS+0x84, 1)
        if mode == 4: self.put(PROCESS+0xd8, 0)
        now = struct.unpack('<d', self.mu.mem_read(0x55b4d0, 8))[0]; self.mu.mem_write(0x55b4d0, struct.pack('<d', now+({1:.125, 2:.25, 5:.5, 6:1.25}.get(mode, 0))))

    def instruction(self, mu, address, size, data):
        """계산 몸체는 대체하지 않고 명시한 실제 외부 효과만 처리한다."""
        self.instructions += 1
        if address in ENTRIES: self.calls[f'{address:08x}'] += 1
        esp = mu.reg_read(UC_X86_REG_ESP); args = struct.unpack('<5I', mu.mem_read(esp+4, 20)); ecx = mu.reg_read(UC_X86_REG_ECX); sid = (ecx-POOL)//50
        if address == 0x4e5f7b:
            value = args[0]; assert 0 <= value < 128; self.ret(value-32 if 97 <= value <= 122 else value); return
        if address == 0x4e4391: self.event('N', args[0]); assert args[0] == 0xe0; self.ret(PROCESS if self.case[20] else 0); return
        if address == 0x4e4354: assert args[0] == PROCESS; self.event('F', 1); self.ret(); return
        if address == 0x4af530:
            assert args[:2] == (103, 2); result = 50+self.created; self.created += 1; self.event('C', *args[:2], result); self.ret(POOL+result*50); return
        if address == 0x41bd20:
            assert ecx == PROCESS; self.event('A', *args[:4]); self.put(PROCESS+4, args[2] or 90); self.put(PROCESS+8, args[1]); self.mutate('A'); self.ret(PROCESS, 16); return
        if address in (DETACH, UNPOP, DESTROY):
            kind = {DETACH:'D', UNPOP:'U', DESTROY:'X'}[address]; self.event(kind, sid, args[0]); flags = mu.mem_read(ecx+11, 1)[0]; self.put(ecx+11, flags|4 if kind == 'D' else flags&~4 if kind == 'U' else flags|2, 1); self.mutate(kind); self.ret(0, 4); return
        if address == POSITION:
            self.event('V', sid, *args[:3]); self.put(ecx+14, args[0]); self.put(ecx+18, args[1]); self.put(ecx+11, mu.mem_read(ecx+11, 1)[0]&~4, 1); self.mutate('V'); self.ret(0, 12); return
        if address == 0x4adea0:
            self.event('O', sid); self.put(POINT, self.case[32]); self.put(POINT+4, self.case[33]); self.mutate('O'); self.ret(POINT, 4); return
        if address == 0x41bf10:
            assert ecx == PROCESS; self.event('K', args[0]); self.kill_return = self.get(esp); self.kill_esp = esp; self.put(esp-4, KILL_RETURN); mu.reg_write(UC_X86_REG_ESP, esp-4); mu.reg_write(UC_X86_REG_EIP, 0x473b90); return
        if address == KILL_RETURN:
            assert esp == self.kill_esp; mu.reg_write(UC_X86_REG_ESP, esp+8); mu.reg_write(UC_X86_REG_EIP, self.kill_return); return
        if address == 0x4e0620: raise RuntimeError('마나 원본 진단')
        if not any(low <= address and address+size <= high for low, high in self.allowed): raise RuntimeError(f'마나 범위 밖 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """알려진 필드·현재 시간/커서·초기 theme 표·raw/스택 밖 쓰기를 거부한다."""
        if not any(low <= address and address+size <= high for low, high in ((0,4), (POOL,POOL+0x10000), (PROCESS,PROCESS+0xe0), (STACK-0x7f000,STACK+0x1000), (0x55a274,0x55a278), (0x592990,0x592f60))): raise RuntimeError(f'마나 범위 밖 쓰기 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """SEH DWORD 이외의NULL 읽기를 막고 실제 격리 영역만 읽는다."""
        if not any(low <= address and address+size <= high for low, high in ((0,4), (0x400000,self.image_end), (POOL,POOL+0x10000), (TYPES,TYPES+0x20000), (PROCESS,PROCESS+0xe0), (VTABLE,VTABLE+0x1000), (POINT,POINT+8), (STACK-0x7f000,STACK+0x1000))): raise RuntimeError(f'마나 범위 밖 읽기 {address:08x}+{size}')

    def sample(self, case, precision):
        """새 입력으로 한 동작/연속 호출을 실행하고 전체 필드·raw31개·전역·효과를 읽는다."""
        self.case = case; self.events = []; self.created = 0; self.mutated = False; mu = self.mu
        mu.mem_write(PROCESS, b'\xa5'*0xe0); self.put(0, 0); self.put(0x5c8464, POOL); self.put(0x59ab20, TYPES); self.put(0x55a274, 0 if not case[17] else PROCESS if case[17] == 1 else PROCESS+4)
        self.put(0x531928, case[18]); self.put(0x52f488, case[34]); self.put(0x52f838, case[35]); mu.mem_write(0x55b4d0, struct.pack('<2I', *case[22:24]))
        self.put(TYPES+100*500+0x98, case[1]); self.put(TYPES+100*500+0x1d4, case[2]); self.put(TYPES+100*500+0x1d8, case[3])
        # 실제 효과 대상의 raw50바이트는 원본 몸체 실행 전에만 준비한다.
        for sid in (*range(50,80), 150):
            raw = bytearray(b'\xa5'*50); struct.pack_into('<I', raw, 0, VTABLE); raw[10] = 100 if sid == 150 else 103; raw[11] = 0 if sid == 150 else case[13]|(4 if case[15] >> (sid-50)&1 else 0)|(2 if case[16] >> (sid-50)&1 else 0); struct.pack_into('<H', raw, 6, 90 if sid == 150 else 0); struct.pack_into('<2I', raw, 14, *(case[28:30] if sid == 150 else case[30:32])); mu.mem_write(POOL+sid*50, bytes(raw))
        self.put(PROCESS, 0x50c468); self.put(PROCESS+4, 90); self.put(PROCESS+8, 150)
        # 원본 필드의 독립 입력이며 기대값 계산은 아니다.
        for index in range(30): self.put(PROCESS+12+4*index, 0 if case[14] >> index&1 else 50+index)
        for offset, value in ((0x84,case[4]),(0x88,case[5]),(0x98,case[6]),(0x90,case[7]),(0x8c,case[8]),(0x94,case[9]),(0xd8,case[10])): self.put(PROCESS+offset,value)
        for index in range(7): self.put(PROCESS+0x9c+4*index,bits(.25*(index+1)))
        self.put(PROCESS+0xbc,bits(float(struct.unpack('<i',struct.pack('<I',case[2]))[0]-1)));self.put(PROCESS+0xb8,bits(float(struct.unpack('<i',struct.pack('<I',case[3]))[0]-1)))
        mu.mem_write(PROCESS+0xc0,struct.pack('<2I',*case[24:26]));mu.mem_write(PROCESS+0xc8,struct.pack('<d',3.25));mu.mem_write(PROCESS+0xd0,struct.pack('<2I',*case[26:28]))
        snapshots=[]; returned=0
        # 각 호출에서 실제 ABI·정상 반환·x87을 검사한다.
        def execute(entry, arguments=b'', purge=0, this=PROCESS):
            self.call(entry,this,arguments,purge,precision);self.returns+=1;snapshots.append(bytes(mu.mem_read(PROCESS,0xe0)).hex())
        if case[0] in (0,5):
            execute(0x4737f0,struct.pack('<II',150,0),8)
            if case[0]==5:
                # 실제 생성 상태를 이어 세 프레임·종료 통지·소멸까지 관찰한다.
                for _ in range(case[21]):
                    now=struct.unpack('<d',mu.mem_read(0x55b4d0,8))[0];mu.mem_write(0x55b4d0,struct.pack('<d',now+.125));execute(0x473d00)
                execute(0x473c30,struct.pack('<III',3,0,0),12);execute(0x473b90)
        elif case[0]==1:execute(0x473d00)
        elif case[0]==2:execute(0x473c30,struct.pack('<III',case[11],case[12],0x87654321),12)
        elif case[0]==3:execute(0x473b90)
        elif case[0]==4:execute(0x4741d0,struct.pack('<II',150,0),0,0);returned=int(mu.reg_read(UC_X86_REG_EAX)==PROCESS)
        raw=[]
        # 가상 표의 합성 주소만 토큰1로 정규화하고 다른46바이트를 모두 보존한다.
        for sid in (*range(50,80),150):
            data=bytearray(mu.mem_read(POOL+sid*50,50));struct.pack_into('<I',data,0,1);raw.append(data.hex())
        token=self.get(0x55a274);globals_=(0 if not token else 1 if token==PROCESS else 2,*struct.unpack('<2I',mu.mem_read(0x55b4d0,8)),returned)
        return (';'.join(self.events) or '-',bytes(mu.mem_read(PROCESS,0xe0)).hex(),','.join(map(str,globals_)),''.join(raw),'|'.join(snapshots))


def main():
    """전체 입력을 두 정밀도로 실행해 차이를 보존하거나 기존 SHA만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 저장된 기대값을 재계산하지 않고 전체 바이트를 감사한다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs'];print('커서 마나 SHA 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];assert oracles[0].table==oracles[1].table;rows=[];different=[]
    # 두 실제 명령 실행의 결과를 나란히 보존하고 어느 쪽의 차이도 덮어쓰지 않는다.
    for case in cases():
        results=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))]
        if results[0]!=results[1]:different.append(len(rows))
        rows.append((','.join(map(str,case)),*results[0],*results[1]))
    FIXTURE.write_text('# input36 events53 fields53 globals53 raw53 steps53 events64 fields64 globals64 raw64 steps64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_cursor_mana.py',FIXTURE.relative_to(ROOT).as_posix())  # 현재 관측의 바이트 의존이다.
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=[f'{entry:08x}' for entry in ENTRIES],uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),instructions=sum(oracle.instructions for oracle in oracles),actualEntries=dict(sum((oracle.calls for oracle in oracles),Counter())),precisionDifferentInputs=different,themeFrames=oracles[0].table,sha256={path:digest(ROOT/path) for path in paths},limitations=['실제 생성·원형/사각형 프레임·통지·소멸·클램프·절삭을 실행한다.','할당/Attach/부모 중심과 객체 가상 효과는 명시 경계다. 실제 World/Kernel 연결은 제품 검사로 구별한다.','프레임 자기 삭제는 실제 소멸 본문을 실행하고 사각형 꼬리 쓰기도 관측한다. 실제 GUI/플레이 증거는 아니다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('커서 마나:',len(rows),'고유 입력, 정상 반환',facts['normalReturns'],'정밀도 차이',len(different))


if __name__=='__main__':main()
