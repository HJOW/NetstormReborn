#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Graph Add/Flood/Rebuild의 진단·로그 복귀와 현재 표/슬롯/stack을 양판본에서 관측한다."""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path
from decomp_surface_oracle import SurfaceOracle, ROOT, POOL, MAP, node
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 원문 배열은 격리된 Native 메모리에만 둔다. 게임/파일/OS 장치를 열지 않는다.
TABLE, STACK = 0x11000000, 0x11002000
# 검토한 Graph 부모/하위의 실제 판본별 진입점이다. 로그와 assertion UI만 명시 반환 장치로 바꾼다.
SPECS = {
    'originals': {'entries': {'Add': 0x4633c0, 'Flood': 0x462f10, 'Rebuild': 0x463110},
        'helpers': [0x462ac0,0x462b20,0x462bc0,0x462b80,0x462c80,0x462e50,0x463330,0x4ad390,0x4ad3e0,0x40eb40],
        'table': 0x562c18, 'stack': 0x562c20, 'maximum': 0x54040c, 'invalid': 0x540410, 'log': 0x4c2630},
    'originalCD': {'entries': {'Add': 0x45b7d0, 'Flood': 0x45bb90, 'Rebuild': 0x45bd60},
        'helpers': [0x45b4d0,0x45b4e0,0x45b520,0x45b560,0x45b570,0x45b6f0,0x45b7a0,0x45c390,0x45c440,0x45c460,0x4adbc0,0x4adbd0],
        'table': 0x5207f0, 'stack': 0x520800, 'maximum': 0x5207ec, 'invalid': 0x5207f4, 'log': 0x48ca70},
}
# 북/서/동/남과 동쪽 꼬리를 둔다. 원문은 북/서/동/남 순서로 연결을 보관한다.
IDS = (5,6,7,8,9,10)  # 원본과 C++ 모두 예약1~4 밖의 실제 할당 범위를 사용한다.
SOURCE = IDS[0]  # 전역 iterator의 첫 Next가 건너뛰는 예약SID1과 구별한다.
OBJECTS = [node(5,20,20),node(6,19,20),node(7,21,20),node(8,22,20),node(9,20,21),node(10,20,19)]


class LiveGraphOracle(SurfaceOracle):
    """실제 표면 탐색과 Graph 명령을 실행하고 현재 진단/로그 경계만 관찰한다."""
    def __init__(self, edition):
        """최신 전체 정밀 함수 범위에서 검토한 부모와 하위만 추가한다."""
        super().__init__(edition);self.spec = SPECS[edition]
        self.entries.update(self.spec['entries']);self.stubs[self.spec['log']] = 'graph-log'
        with (ROOT / f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'],16): row for row in csv.DictReader(stream,delimiter='\t')}
        # 미검토 함수나 범위 사이의 바이트는 실행 허용 범위에 넣지 않는다.
        for entry in list(self.spec['entries'].values()) + self.spec['helpers']:
            # 한 함수의 불연속 실제 명령 범위를 각각 허용한다.
            for part in facts[entry]['ranges'].split(';'):
                lo,hi = (int(value,16) for value in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.mem_map(TABLE,0x10000)
        self.write_ranges.extend([(TABLE,TABLE+255*6),(STACK,STACK+4096*4),(POOL,POOL+32*self.squid_stride)])
        self.checking = 0x5e4794 if edition == 'originals' else TABLE + 0x7000  # CD는 checking을 읽지 않는다.
        self.action = 0  # 콜백이 수행하는 명시 입력 변경 종류다.
        self.events = []  # 진단/로그의 실제 식/파일/행/수준/순서를 기록한다.
        self.traces = 0  # 첫 flood 로그에서만 입력을 바꾼다.

    def on_instruction(self, mu, address, size, data):
        """원문의 assertion/log 스택을 읽고 지정한 변경 후 원래 호출 위치로 복귀한다."""
        kind = self.stubs.get(address)
        if kind not in ('assert','graph-log'):
            return super().on_instruction(mu,address,size,data)
        esp = mu.reg_read(UC_X86_REG_ESP)  # cdecl 인자와 반환 주소가 놓인 실제 Native 스택이다.
        ret,a,b,c = struct.unpack('<IIII',mu.mem_read(esp,16))
        if kind == 'assert':
            self.events.append(f'a:{c}:{self.read_string(a).decode()}:{self.read_string(b).decode()}')
            if self.action == 1:
                mu.mem_write(self.checking,struct.pack('<I',0));mu.mem_write(TABLE+3*6,struct.pack('<hhh',9,0,-310))
                mu.mem_write(POOL+SOURCE*self.squid_stride+11,b'\0');mu.mem_write(POOL+SOURCE*self.squid_stride+14,struct.pack('<ff',20,20))
            elif self.action == 2:
                mu.mem_write(POOL+SOURCE*self.squid_stride+11,b'\0')
                if c in (0x100,0xc2):
                    mu.mem_write(POOL+IDS[2]*self.squid_stride+self.graph_offset,b'\x05');mu.mem_write(TABLE+5*6,struct.pack('<h',3))
                elif c == 0x113:
                    mu.mem_write(TABLE+5*6,struct.pack('<h',1));mu.mem_write(TABLE+4*6,struct.pack('<h',77))
            elif self.action == 4 and c == 0x68:
                mu.mem_write(self.checking,struct.pack('<I',0))
        else:
            assert self.read_string(b) == b'%d graphs in use\n', '원문 Graph 로그 형식 변경'
            self.events.append(f'l:{a}:{c}');self.traces += 1
            if self.traces == 1 and self.action == 3:
                graph = mu.mem_read(POOL+SOURCE*self.squid_stride+self.graph_offset,1)[0]
                # 뒤 후보 번호와 현재 행/미사용 stack은 로그 장치의 명시 입력으로만 변경한다.
                for sid in (IDS[2],IDS[4]):
                    mu.mem_write(POOL+sid*self.squid_stride+self.graph_offset,bytes([graph]))
                if graph < 251:
                    mu.mem_write(TABLE+graph*6,struct.pack('<h',91))
                mu.mem_write(STACK+4095*4,struct.pack('<I',0xdeadbeef))
            elif self.traces == 1 and self.action == 4:
                mu.mem_write(TABLE+254*6,struct.pack('<hh',0,0))
            elif self.traces == 1 and self.action == 5:
                mu.mem_write(MAP+(20*256+22)*2,struct.pack('<H',0))
            elif self.traces == 1 and self.action == 6:
                mu.mem_write(POOL+IDS[2]*self.squid_stride+11,b'\x01')
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)

    def observe(self, operation, checking, action):
        """입력만 초기화하며 결과 표/전체 stack/슬롯과 이벤트는 원문 기계어에서 읽는다."""
        objects = list(OBJECTS)
        if operation == 'AddVoid':
            objects[0] = node(SOURCE,20,20,dead=0)
        elif operation == 'AddZero':
            objects[0] = node(SOURCE,0,20)
        self.setup(objects);self.action = action;self.events = [];self.traces = 0
        self.graph_offset = 30 if self.edition == 'originals' else 28
        # 전역 iterator가 준비하지 않은 슬롯을 읽지 않게 실제 free 상태를 입력한다.
        for sid in range(1,32):
            if sid not in IDS:
                self.mu.mem_write(POOL+sid*self.squid_stride+11,b'\x01')
        records = bytearray().join(struct.pack('<hhh',0,0,100+index) for index in range(255))
        # 현재 count는 일부 원문 장부처럼 실제 지도 수와 다를 수 있다. 결과를 정규화하지 않는다.
        for graph,count,used in ((3,2,2),(4,2,1),(5,1,1),(6,1,1),(7,0,1),(254,0x7dfd,0x7dfd)):
            struct.pack_into('<hh',records,graph*6,count,used)
        self.mu.mem_write(TABLE,bytes(records))
        self.mu.mem_write(STACK,struct.pack('<4096I',*(0xa0000000+index for index in range(4096))))
        # 각 부모는 동일한 현재 그래프 입력에서 시작한다. Flood만 기존3번에서7번으로 옮긴다.
        for sid,graph in zip(IDS,(3 if operation == 'Flood' else 254,3,4,4,5,6)):
            self.mu.mem_write(POOL+sid*self.squid_stride+self.graph_offset,bytes([graph]))
        if operation == 'AddVoid':
            self.mu.mem_write(POOL+SOURCE*self.squid_stride+11,b'\x04')
        self.mu.mem_write(self.checking,struct.pack('<I',checking))
        # 할당/로그 I/O에 들어가지 않도록 기존 표와 stack을 원문 전역에 연결한다.
        for key,value in (('table',TABLE),('stack',STACK),('maximum',251),('invalid',254)):
            self.mu.mem_write(self.spec[key],struct.pack('<I',value))
        if self.edition == 'originals':
            self.mu.mem_write(0x5411c0,struct.pack('<I',90));self.mu.mem_write(0x541288,struct.pack('<I',140))
        result = self.call('Add' if operation.startswith('Add') else 'Rebuild' if operation.startswith('Rebuild') else 'Flood',
            [SOURCE] if operation.startswith('Add') else [SOURCE,7] if operation == 'Flood' else [int(operation[-1])])
        slots = b''.join(bytes(self.mu.mem_read(POOL+sid*self.squid_stride,self.squid_stride)) for sid in IDS)
        return [str(result) if operation == 'Flood' else '-', bytes(self.mu.mem_read(TABLE,255*6)).hex(),
            bytes(self.mu.mem_read(STACK,4096*4)).hex(),slots.hex(),str(struct.unpack('<I',self.mu.mem_read(self.checking,4))[0]),'|'.join(self.events) or '-']


def main():
    """새 관측만 생성하거나 기존 증거를 읽기 비교하며 기존 파일을 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--fixture',type=Path,default=ROOT/'cpppj/tests/fixtures/graph-live-checking-x86.tsv')
    parser.add_argument('--report',type=Path,default=ROOT/'cpppj/graph-live-checking-native-20261103.json')
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--action',type=int,choices=range(7))  # 6은 뒤 후보를 free로 바꾸는 추가 Native 관측이다.
    args = parser.parse_args()
    rows,binaries = [], {}  # 결과 모델을 두지 않는 독립 Native 관측과 PE 해시다.
    # 두 판본에서 고위 비트 checking과 진단/로그 중 변경을 같은 입력으로 비교한다.
    for edition in SPECS:
        oracle = LiveGraphOracle(edition);binaries[edition] = oracle.sha256
        # 비영/0 관문과 여섯 부모 입력을 관측한다.
        for operation in ('Add','AddVoid','AddZero','Flood','Rebuild0','Rebuild1'):
            # checking은 bool로 좁히지 않고 원래 DWORD로 기록한다.
            for checking in (0,0x80000000):
                # 콜백 변경은 별도 입력이며 기대 결과를 Python으로 계산하지 않는다.
                for action in (range(6) if args.action is None else (args.action,)):
                    rows.append([edition,operation,str(checking),str(action)]+oracle.observe(operation,checking,action))
    output = ('\n'.join('\t'.join(row) for row in rows)+'\n').encode()
    if args.verify:
        assert args.fixture.read_bytes() == output, 'Graph 현재 실행의 원문 재관측 불일치'
    else:
        if args.fixture.exists() or args.report.exists():
            raise FileExistsError('기존 산출물에는 --verify를 사용하십시오')
        args.fixture.write_bytes(output)
        args.report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03',
            'binaries':binaries,'rows':len(rows),'fixtureSha256':hashlib.sha256(output).hexdigest(),
            'boundary':'실제 PE Graph/탐색/표/stack·진단/로그만 명시 반환·게임/GUI/OS/소켓 미실행'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'verify':args.verify,'sha256':hashlib.sha256(output).hexdigest()}))


if __name__ == '__main__':
    main()
