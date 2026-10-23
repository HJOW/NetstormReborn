#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Badges 저장004be7f0과 실제6160BYTE getter0047d7c0의 최초 독립 PE 관측이다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

HEADER, TABLE = 0x5c8a04, 0x593160  # Template1 this와28개 실제 Badge 기록의 원본 주소다.
STRIDE, RECORDS, COUNT = 220, 28, 216  # 각 기록의 실제 폭·기록 수·count DWORD 위치다.
NODES, BUFFER_A, BUFFER_B = POOL + 0x4000, POOL + 0x8000, POOL + 0x9000  # 두16BYTE stream과 수명이 겹치는 두256BYTE 물리 버퍼다.
MANAGERS = {0:0, 11:POOL + 0x100, 22:POOL + 0x200}  # 명시 Create 장치의 manager 식별자다.
PARENT = (0x4be7f0, 0x4be8da)  # 정상 RET까지의234BYTE 저장 부모다.
GETTER = ((0x47d7c0, 0x47d7cd), (0x47d7d0, 0x47d7f9))  # 정렬3BYTE를 제외한54BYTE 실제 getter다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-badges-save-x86.tsv'  # 제품 작성 전에 고정하고 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/fort-badges-save-native.json'  # 실제 부모/getter와 명시 외부 효과를 구별한다.


def cases():
    """raw장부·count DWORD·진단/stream변경 입력만 만들며 저장 기대값은 계산하지 않는다."""
    rows = []
    values = (0, 1, 255, 256, 0xffffffff, 0x80000000, 0x12345678)  # bool/저BYTE가 서로 다른 실제 DWORD 입력이다.
    # 부모는 type0/254/255와 첫 중복·없는type을 실제 getter로 모두 조회한다.
    for profile, query, count in itertools.product(range(5), (0, 70, 254, 255), values):
        rows.append((0, profile, query, count, 11, 0, 0, 128, 'none'))
    # 첫 count의 무진단 쓰기와 signed end/capacity 경계를 같은 물리 출력에 적용한다.
    states = ((0,0,128), (3,9,128), (0,0xffffffff,128), (0,0,0), (0,0,0xffffffff), (0,0x80000000,0x80000000))
    for manager, state in itertools.product((0,11), states):
        rows.append((0, 3, 0, 1, manager, *state, 'none'))
    changes = ('assert1-manager22', 'assert1-stop', 'create-dataB', 'create-pos5', 'create-manager22', 'create-current2', 'create-return2', 'create-stop',
               'count0-dataB', 'count0-pos0', 'count0-pos9', 'count0-end4294967295', 'count0-cap0', 'count0-counter250', 'count0-table999', 'count0-stop',
               'count254-dataB', 'count254-pos0', 'count254-stop',
               'capacity1-dataB', 'capacity1-pos9', 'capacity1-end4294967295', 'capacity1-cap128', 'capacity1-manager22', 'capacity1-counter250', 'capacity1-table999', 'capacity1-stop',
               'capacity2-dataA', 'capacity2-pos0', 'capacity2-end4294967295', 'capacity2-cap128', 'capacity2-counter255', 'capacity2-table999', 'capacity2-stop', 'capacity3-stop')
    # 정상 장부와 여러 항목·저BYTE0의세 입력에서 같은 외부효과를 적용한다.
    for profile, count, change in itertools.product((0,2,3), (1,256), changes):
        rows.append((0, profile, 0, count, 0 if change.startswith('assert1-') else 11, 0, 0, 0 if change.startswith('capacity') else 128, change))
    # 직접 getter는255/256/상위DWORD도BYTE절삭없이비교하고첫중복0도그대로돌려준다.
    for profile, query, count in itertools.product(range(5), (0,1,26,27,70,254,255,256,0xffffffff), values):
        rows.append((1, profile, query, count, 11, 0, 0, 128, 'none'))
    return list(dict.fromkeys(rows))


def badge_bytes(profile, query, count):
    """padding까지 명시한 입력 장부이며 출력 순서·개수의 기대값을 만들지 않는다."""
    raw = bytearray([0xa5]) * (STRIDE * RECORDS)
    # 기본 type/count와의미없는212BYTE까지원본입력으로유지한다.
    for index in range(RECORDS):
        typ, quantity = index, 0
        if profile == 0:
            typ = query if index == 0 else 70 + index
            quantity = count if index == 0 else 0
        elif profile == 1:
            typ = query if index in (0,13,27) else 100 + index
            quantity = (count + (0 if index == 0 else 1 if index == 13 else 2)) & 0xffffffff if index in (0,13,27) else 0
        elif profile == 2:
            typ = (0,254,255)[index] if index < 3 else 0 if index == 27 else 100 + index
            quantity = count if index < 3 else (count + 7) & 0xffffffff if index == 27 else 0
        elif profile == 3:
            typ = index if index < 27 else 254
            quantity = (count + index) & 0xffffffff
        elif profile == 4:
            typ, quantity = 1000 + index, count
        struct.pack_into('<I', raw, index * STRIDE, typ)
        struct.pack_into('<I', raw, index * STRIDE + COUNT, quantity)
    return bytes(raw)


def rle(raw):
    """장부6160BYTE전체를동일BYTE반복의길이/값으로무손실표현한다."""
    parts = []
    for value, group in itertools.groupby(raw):  # type/count외모든padding도비교자료에포함한다.
        parts.append(f'{sum(1 for _ in group)}:{value:02x}')
    return ','.join(parts)


class Oracle(MemoryOracle):
    """저장부모와getter는실제명령으로실행하고Create/진단만외부장치로공급한다."""
    def __init__(self):
        """보호 PE 가상 복사와 실행/쓰기 제한·최초 전체 계수기를 준비한다."""
        super().__init__()
        self.total_instructions = 0  # 실제 실행한 네이티브 명령의 전체 수다.
        self.normal_returns = 0  # 정상 RET 관측 횟수다.
        self.stops = 0  # 지정 경계에서 중단한 횟수다.
        self.boundaries = Counter()  # 진단/Create/getter 반환의 전체 횟수다.
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """CPU쓰기만제한한다.입력callback변경은별도로지정한host효과다."""
        ranges = ((STACK-0x1000,STACK+8), (NODES,NODES+32), (BUFFER_A,BUFFER_A+256), (BUFFER_B,BUFFER_B+256))
        if not any(low <= address and address + size <= high for low, high in ranges):
            raise RuntimeError(f'Badges 저장 CPU 쓰기 범위 밖 {address:08x}/{size}')

    def event(self, kind, text):
        """실제외부진입과getter반환을순서대로기록한다."""
        self.events.append(text)
        self.calls[kind] += 1
        self.boundaries[kind] += 1
        return self.calls[kind]

    def counter(self):
        """원본 EDI가 보관한 처음 포인터의 현재 BYTE만 읽는다."""
        return self.mu.mem_read(self.count_pointer,1)[0] if self.count_pointer else -1

    def mutate(self, boundary):
        """명시하위응답의callback변경만공급하며부모의후속분기를계산하지않는다."""
        trigger, action = self.case[8].rsplit('-',1) if '-' in self.case[8] else ('','')
        if trigger != boundary:
            return
        if action.startswith('manager'):
            self.put(HEADER,MANAGERS[int(action[7:])])
        elif action == 'dataA':
            self.put(self.active_node,BUFFER_A)
        elif action == 'dataB':
            self.put(self.active_node,BUFFER_B)
        elif action.startswith('pos'):
            self.put(self.active_node+4,int(action[3:]))
        elif action.startswith('end'):
            self.put(self.active_node+8,int(action[3:]))
        elif action.startswith('cap'):
            self.put(self.active_node+12,int(action[3:]))
        elif action.startswith('counter'):
            self.mu.mem_write(self.count_pointer,bytes((int(action[7:]),)))
        elif action == 'table999':
            self.put(TABLE+COUNT,999)
        elif action == 'current2':
            self.currents = {key:2 for key in self.currents}
        elif action == 'stop':
            self.stopped = True
            self.mu.emu_stop()

    def instruction(self, mu, address, size, data):
        """장부linear검색/인라인쓰기는실제명령이며원문밖경계만명시복귀한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 100000:
            raise RuntimeError('Badges 저장 명령 상한 초과')
        # getter내모든분기·DWORD읽기는그대로실행한다.자주방문하는범위에서는호스트인자를읽지않는다.
        if any(low <= address < high for low,high in GETTER):
            return
        if address == 0x4be835:
            self.count_pointer = mu.reg_read(UC_X86_REG_EDI)
        elif address == 0x4be846:
            result, typ = mu.reg_read(UC_X86_REG_EAX), mu.reg_read(UC_X86_REG_EBX)
            self.event('count',f'count:{typ}:{result}:{self.counter()}')
            self.mutate('count'+str(typ))
        elif address == 0x44d1b0:
            args = self.words(mu.reg_read(UC_X86_REG_ESP)+4,1)
            manager = next(key for key,pointer in MANAGERS.items() if pointer == mu.reg_read(UC_X86_REG_ECX))
            if self.string(args[0]) != b'Badges':
                raise RuntimeError('Badges 저장 Create 이름 오류')
            self.created += 1
            self.token = 2 if self.case[8] == 'create-return2' else 1
            self.active_node = NODES+(self.token-1)*16
            self.event('create',f'create:{manager}:Badges')
            self.currents[manager] = self.token
            self.mutate('create')
            if not self.stopped:
                self.ret(self.active_node,4)
        elif address == 0x4e0620:
            line = self.words(mu.reg_read(UC_X86_REG_ESP)+4,3)[2]
            pointer,position,end,capacity = self.words(self.active_node,4)
            index = self.event('assert',f'assert:{line}:{pointer}:{position}:{end}:{capacity}:{self.counter()}')
            if line == 36:
                self.capacity_calls += 1
                self.mutate('capacity'+str(self.capacity_calls))
            else:
                self.mutate('assert'+str(index))
            if not self.stopped:
                self.ret()
        elif not PARENT[0] <= address < PARENT[1]:
            raise RuntimeError(f'검토하지 않은 Badges 명령 {address:08x}')

    def sample(self, case, control):
        """전체raw입력을준비하고실제정상/부분중단의모든메모리를읽는다."""
        self.case = case  # 최초 입력 kind/profile/query/count/manager/pos/end/capacity/mutation다.
        self.events, self.calls = [], Counter()  # 한 관측의 실제 경계 순서·횟수다.
        self.currents = {0:90,11:91,22:92}  # 외부 manager의 초기 current 입력이다.
        self.created, self.token = 0, 0  # 명시 Create 횟수와 반환 stream 식별자다.
        self.instructions, self.capacity_calls = 0, 0  # 한 관측의 명령/용량 진단 수다.
        self.stopped, self.count_pointer = False, 0  # 중단 여부와 부모가 직접 저장할 EDI 포인터다.
        self.active_node = NODES  # 첫 진입 진단이 볼 준비된 stream이며 Create가 바꾸면 그대로 따른다.
        self.mu.mem_write(HEADER,bytes(range(20)))
        self.put(HEADER,MANAGERS[case[4]])
        self.mu.mem_write(TABLE,badge_bytes(case[1],case[2],case[3]))
        self.mu.mem_write(NODES,struct.pack('<4I',BUFFER_A,*case[5:8])+struct.pack('<4I',BUFFER_B,*case[5:8]))
        self.mu.mem_write(BUFFER_A,b'\xcc'*256)
        self.mu.mem_write(BUFFER_B,b'\xdd'*256)
        # 관측마다CPU상태와스택인자를완전히새입력으로지정한다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ECX,HEADER)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.mem_write(STACK,struct.pack('<2I',STOP,case[2]))
        self.mu.emu_start(0x4be7f0 if case[0] == 0 else 0x47d7c0,STOP)
        if self.stopped:
            self.stops += 1
        else:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK+4
            self.normal_returns += 1
        result = self.mu.reg_read(UC_X86_REG_EAX) if case[0] == 1 else 0
        manager = next(key for key,pointer in MANAGERS.items() if pointer == self.words(HEADER,1)[0])
        state = [manager,self.created,self.token,result,*self.currents.values()]
        raw = bytes(self.mu.mem_read(HEADER,20)).hex()+bytes(self.mu.mem_read(NODES,32)).hex()+bytes(self.mu.mem_read(BUFFER_A,256)).hex()+bytes(self.mu.mem_read(BUFFER_B,256)).hex()
        table = rle(bytes(self.mu.mem_read(TABLE,STRIDE*RECORDS)))
        return '\t'.join((str(control),','.join(map(str,case)),str(int(not self.stopped)),','.join(map(str,state)),raw,table,';'.join(self.events)))


def main():
    """최초원문결과를제품작성전에고정하거나보존된모든SHA만확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args = parser.parse_args()
    if args.verify:
        record = json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in record['sha256'].items():  # 제품에맞춰최초자료를바꾸지못하게한다.
            if digest(ROOT/name) != expected:
                raise RuntimeError('Badges 저장 최초 SHA 불일치: '+name)
        print('Badges 저장 최초 SHA 보존:',record['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('Badges 저장 최초 fixture/보고서 덮어쓰기 금지')
    oracle, inputs = Oracle(), cases()
    lines = ['# control / kind,profile,query,count,manager,pos,end,capacity,mutation / completed / 현재장치7DWORD / Template20+stream32+출력512BYTE / 장부6160BYTE_RLE / 실제경계trace다.']
    for index,case in enumerate(inputs):  # 두정밀도도각각실제별도실행한다.
        first,second = oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        assert first.split('\t')[1:] == second.split('\t')[1:]
        lines.extend((first,second))
        if index%100 == 0:
            print(f'원문관측 {index}/{len(inputs)}',flush=True)
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths = ('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_fort_badges_save.py','cpppj/tests/fixtures/fort-badges-save-x86.tsv')
    record = {'status':'최초 독립 원문 고정·제품 미작성','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','parent':'004be7f0','getter':'0047d7c0','bodyBytes':234+sum(high-low for low,high in GETTER),'uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalReturns':oracle.normal_returns,'explicitStops':oracle.stops,'instructions':oracle.total_instructions,'precisionDifferences':0,'boundaries':dict(oracle.boundaries),'scope':'실제저장부모와28x220BYTE첫type일치DWORDgetter. 모든255type순회/처음count포인터/인라인BYTE·pos/end/선택진단/부분중단. Create·진단보고만명시외부장치이며getter복귀/진단의입력callback변경은독립명시효과다. Client장부초기화·UI갱신·원시파일load·OS·게임실행없음.','gameExecuted':False,'osExecuted':False,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:record[key] for key in ('uniqueInputs','observations','normalReturns','explicitStops','instructions','bodyBytes')}))


if __name__ == '__main__':
    main()
