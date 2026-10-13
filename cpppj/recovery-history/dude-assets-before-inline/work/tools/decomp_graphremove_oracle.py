#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""삭제 준비 분할→일반 다리/섬 Unpop→void 반납을 세 실제 PE에서 격리 실행한다."""
import argparse
import collections
import csv
import hashlib
import json
import struct
from pathlib import Path
from decomp_rawgraph_oracle import RawGraphOracle, TABLE, FLOOD
from decomp_graph_oracle import SPECS, records_for
from decomp_creation_oracle import CreationOracle
from decomp_sid_oracle import SidOracle, POOL
from decomp_pop_oracle import SPACE, HEADS, bits
from decomp_display_oracle import DISPLAY
from decomp_postpop_oracle import POST, ARENA
from decomp_surface_oracle import packed
from decomp_oracle import ROOT, pefile

# 실제 PE 경로와 함수/동결 시계/분할 정책/특수 9 감소 타입의 판본별 전역이다.
BINARIES = {'originals': 'originals/Netstorm.exe', 'originalCD': 'originalCD/NETSTORM.EXE',
            'original1037': 'original1037/netstorm.exe'}
DELETE = {'originals': (0x4637b0, 0x562c1c, 0x5412e0, 0x55b4b0),
          'originalCD': (0x45bfd0, 0x5207fc, 0x51cbcc, 0x50f250)}
# 새 기대값은 고정 부모 도구의 준비/관찰만 공유한다. 기존 SHA 기록을 수정하지 않는다.
DEPENDENCIES = ('decomp_rawgraph_oracle.py', 'decomp_graph_oracle.py', 'decomp_creation_oracle.py',
                'decomp_sid_oracle.py', 'decomp_pop_oracle.py', 'decomp_display_oracle.py',
                'decomp_postpop_oracle.py', 'decomp_surface_oracle.py', 'decomp_oracle.py')


def digest(path):
    """원본/도구/내보내기/고정 fixture의 정확한 바이트 SHA를 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class DeleteOracle(RawGraphOracle):
    """누락된 과거 추출물 대신 이번 새 몸체를 사용하고 실제 명령만 허용한다."""
    def __init__(self, binary):
        """기존 SID/생성 기반과 새 전체 허용 몸체를 구성한다. 대체 함수는 없다."""
        layout = 'originalCD' if binary == 'original1037' else binary
        CreationOracle.__init__(self, layout)
        self.binary = binary
        self.space, self.display, self.post, self.graph = SPACE[layout], DISPLAY[layout], POST[layout], SPECS[layout]
        self.actual_calls = collections.Counter()
        self.display_calls, self.effects, self.graph_calls = collections.Counter(), collections.Counter(), collections.Counter()
        self.observed = collections.Counter()
        self.encodings = 0
        self.delete = DELETE[layout]
        self.bases = []
        cursor = HEADS
        # 네 해시 단계의 실제 배열 크기와 순서를 유지한다.
        for scale in (1, 2, 4, 16):
            self.bases.append(cursor)
            cursor += (256 // scale) ** 2 * 2
        # 기존 raw 준비/관찰의 서로 겹치지 않는 지도·표시·후처리·그래프 영역이다.
        for address, size in ((HEADS, 0x80000), (ARENA, 0x10000), (TABLE, 0x10000)):
            self.mu.mem_map(address, size)
        self.sha256 = digest(ROOT / BINARIES[binary])
        if binary == 'original1037':
            old, new = (ROOT / BINARIES['originalCD']).read_bytes(), (ROOT / BINARIES[binary]).read_bytes()
            assert len(old) == len(new) and [(i, a, b) for i, (a, b) in enumerate(zip(old, new)) if a != b] == [(0x3314c, 0x75, 0xeb)]
            pe = pefile.PE(data=new)
            self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase, pe.get_memory_mapped_image())
        # 이번 읽기 전용 내보내기의 불연속 몸체만 추가한다.
        for suffix in (('', '/geometry') if layout=='originals' else ('', '/geometry', '/lookup')):
            with (ROOT / f'extracted/graphremove/{binary}{suffix}/functions.tsv').open(encoding='utf-8') as fp:
                # 함수 사실을 내보내기 순서로 누적하여 허용한 몸체의 근거를 보존한다.
                for row in csv.DictReader(fp, delimiter='\t'):
                    self.facts.append(row)
                    # 몸체 사이의 다른 함수/OS 호출은 허용하지 않는다.
                    for part in row['ranges'].split(';'):
                        lo, hi = (int(v, 16) for v in part.split('-'))
                        self.allowed.append((lo, hi + 1))
        assert not any(lo <= 0x433d4c < hi for lo, hi in self.allowed)

    def on_instruction(self, mu, address, size, data):
        """실제 분할/표면 통지/표시/반납 도달을 공개 호출 수와 별도로 관찰한다."""
        if hasattr(self, 'delete'):
            entries = {'Detach': self.delete[0], 'Unpop': self.creation['unpop'],
                       'Release': self.spec['entries']['Release'], 'Flood': self.graph['entries']['Flood'],
                       'Allocate': self.graph['entries']['Allocate'],
                       'Notify': 0x481510 if self.edition == 'originals' else 0x442f30}
            # 대체/계산 없이 실제 진입점 도달만 집계한다.
            for name, entry in entries.items():
                if address == entry:
                    self.observed[name] += 1
        SidOracle.on_instruction(self, mu, address, size, data)

    def prepare_delete(self, nodes, spots, records, seed, rebuild, removed):
        """합성 기존 배치/번호/표를 넣는다. 분할 정답은 계산하지 않는다."""
        self.prepare_graph(nodes, spots, records, struct.pack('<4096I', *(seed + i for i in range(4096))))
        self.nodes = nodes
        self.mu.mem_write(HEADS, bytes(86272 * 2))
        # 각 활성 기준점을 원본 4단계 체인에 넣는다. 위치 조회는 0단계 머리를 읽는다.
        for i, n in enumerate(nodes):
            x, y, width, height = n[:4]
            if n[9] & 4:
                continue
            level = n[12]
            scale = (1, 2, 4, 16)[level]
            head = self.bases[level] + ((y // scale) * (256 // scale) + x // scale) * 2
            self.mu.mem_write(POOL + self.ids[i] * self.stride + 4, bytes(self.mu.mem_read(head, 2)))
            self.mu.mem_write(head, struct.pack('<H', self.ids[i]))
        # 동결 시계의 실제 조기 반환을 사용한다. timeGetTime/OS를 대체하지 않는다.
        for address, value in ((self.delete[1], rebuild), (self.delete[2], 74 if removed == 9 else 255),
                               (self.delete[3], 1)):
            self.mu.mem_write(address, struct.pack('<I', value))

    def delete_step(self, name, args):
        """실제 cdecl/thiscall 반환·x87/쓰기 제한 뒤 전체 raw 결과를 관찰한다."""
        if name == 'Detach':
            n = self.nodes[0]
            # 원본은 8 DWORD 위치/타입 정보 중 x/y, 다섯째 type, 여덟째 frame index를 읽는다.
            self.invoke(self.delete[0], [bits(n[0]), bits(n[1]), 0, 0, 74, 0, 0, n[8]])
        elif name == 'Unpop':
            self.invoke(self.creation['unpop'], args, POOL + self.ids[0] * self.stride, 4)
        elif name == 'Release':
            self.invoke(self.spec['entries']['Release'], [self.ids[0]])
        else:
            raise ValueError(name)
        return self.graph_output('-')


def generate():
    """분리/지도 가장자리/별도 그래프/무효·미사용/특수 9 감소와 해제→반납을 실행한다."""
    rows = ['# 실제 삭제 준비/일반 섬·다리 Unpop/반납; dead 원천·정수·소진 전·비전투 null 큐·대체 없음.']
    counts, reports = collections.Counter(), {}
    # CD/10.37은 같은 코드 배치지만 실제 각 PE를 따로 실행한다.
    for binary in BINARIES:
        oracle = DeleteOracle(binary)
        # x87 53/64비트 두 제어값에서 동일한 입력 시퀀스를 각각 실행한다.
        for control in (0x027f, 0x037f):
            oracle.start_graph(control)
            rows.append(f'Begin\t{binary}\t{control}\t' + ','.join(map(str, oracle.ids)))
            # 유형마다 초기 표/공간을 다시 입력하여 앞선 반납의 상태를 누적하지 않는다.
            for case in range(64):
                mode = case % 8
                coords = ([(20,20),(19,20),(21,20),(20,19),(20,21),(18,20),(30,30)] if mode != 6 else
                          [(20,20),(18,19),(23,19),(19,18),(19,23),(17,19),(30,30)])
                # 일반 발자국 helper의 최소 1 보정과 초기 점 0 재설정을 실제 후보에서 검사한다.
                if case>=32 and mode!=6:
                    # CD는 위치 0 후보에서 무조건 assert한다. 패치만 0 후보, CD는 최소 1 후보를 입력한다.
                    edge=1 if binary=='originals' else 2
                    coords=[(edge,edge),(edge-1,edge),(edge+1,edge),(edge,edge-1),
                            (edge,edge+1),(edge+2,edge),(30,30)]
                nodes = []
                # 원천과 여섯 후보의 타입/프레임/상태/번호를 초기 입력으로만 구성한다.
                for i, (x, y) in enumerate(coords):
                    graph = 0 if i < 6 else 2
                    if mode == 1 and i in (2,4): graph = 1
                    if mode == 2: graph = 254
                    state = 2 if i == 0 else 0
                    if mode == 3 and i > 0: state = 2
                    f1 = 0 if i == 6 else 0x800
                    f2 = (2, 4, 8)[case % 3] if i == 0 else 4
                    width = 3 if mode == 6 and i < 6 else 1
                    nodes.append([x,y,width,width,f1,f2,65,80,case % 2,state,
                                  8 if i == 3 and mode == 5 else 0,graph,(case // 8) % 4 if i == 0 else 0])
                # 표면 membership과 내부 spot 준비도 입력일 뿐 분할 결과를 계산하지 않는다.
                members = [(oracle.ids[i],n[11],n[9]) for i,n in enumerate(nodes) if n[4]&0x800]
                records = bytearray(records_for(members,case*11))
                if mode == 4: struct.pack_into('<2h',records,0,-3,0)
                spots = [(n[0],n[1],8) for i,n in enumerate(nodes) if i == 4 and mode == 5]
                rebuild, removed, seed = int((case // 8) % 2 == 0), 9 if mode == 6 else 1, case*17
                oracle.prepare_delete(nodes,spots,bytes(records),seed,rebuild,removed)
                rows.append('\t'.join(['Setup',packed(nodes),packed(spots),records.hex(),str(seed)]))
                # 공개 호출 수에는 준비 Reset/Create와 내부 flood/할당을 더하지 않는다.
                steps=[('Detach',[rebuild,removed]),('Unpop',[0x2000 if case % 2 else 0]),('Unpop',[0])]
                if case==63: steps.append(('Release',[]))
                # 분할→해제→void 조기 반환과 마지막 반납을 실제 호출 순서로 관찰한다.
                for name,args in steps:
                    rows.append('\t'.join(map(str,[name,','.join(map(str,args)) or '-',*oracle.delete_step(name,args)])))
                    counts[name] += 1
        exports = ROOT / f'extracted/graphremove/{binary}'
        export_names=['creation.c','functions.tsv','geometry/creation.c','geometry/functions.tsv']
        if oracle.edition!='originals': export_names.extend(['lookup/creation.c','lookup/functions.tsv'])
        reports[binary] = dict(binary_sha256=oracle.sha256,layout=oracle.edition,function_ranges=oracle.facts,
                               assert_calls=oracle.assertions,actual_calls=dict(oracle.observed),
                               preparation_calls=dict(Reset=2,Create=14),exports={name:digest(exports/name) for name in export_names})
        print(f'{binary}: 삭제 준비/해제 실제 x86 완료',flush=True)
    fixture = ROOT / 'cpppj/tests/fixtures/graphremove-x86.tsv'
    fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    full = ROOT/'extracted/original1037/decomp'
    with (full/'functions.tsv').open(encoding='utf-8') as fp:
        function_count=sum(1 for _ in csv.DictReader(fp,delimiter='\t'))
    assert function_count==3711
    report = dict(schema=1,public_calls=dict(counts),total_calls=sum(counts.values()),sequences=6,editions=reports,
                  original1037_full_decompile=dict(function_count=function_count,
                      exports={name:digest(full/name) for name in ('netstorm.c','functions.tsv')}),
                  original1037_cd_difference=dict(file_offset=0x3314c,virtual_address=0x433d4c,cd=0x75,original1037=0xeb),
                  stubbed=[],tool_sha256=digest(Path(__file__)),fixture_sha256=digest(fixture),
                  dependencies={name:digest(ROOT/'tools'/name) for name in DEPENDENCIES},
                  limitations=['합성 타입/FrameCode/SHP·client 32768·dead 원천·정수·소진 전·동결 시계·비전투 null 큐',
                               '위치 조회는 해시 객체 +12의 0단계 머리; 실제 grid 갱신/상위 삭제/파생 destructor/raw GameWorld는 미연결',
                               'rebuild 분할 정책만 검증; 전역 소진 복구/할당/OS 실행 없음',
                               '7개 슬롯/dirty 항목 직접 비교, 전체 풀/해시/spot/표/스택/통계는 Adler-32'])
    (ROOT/'cpppj/recovery-graphremove-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'삭제 준비 x86 fixture {report["total_calls"]}회: {dict(counts)}')


def verify():
    """기계어를 실행하지 않고 원본/도구/새 내보내기/fixture SHA와 공개 호출 수를 확인한다."""
    r = json.loads((ROOT/'cpppj/recovery-graphremove-evidence.json').read_text(encoding='utf-8'))
    assert r['tool_sha256']==digest(Path(__file__)) and r['stubbed']==[]
    # 이번 전체 디컴파일의 바이트와 구버전 실행 파일의 유일한 분기 차이를 다시 확인한다.
    for name,sha in r['original1037_full_decompile']['exports'].items():
        assert sha==digest(ROOT/'extracted/original1037/decomp'/name)
    old,new=(ROOT/BINARIES['originalCD']).read_bytes(),(ROOT/BINARIES['original1037']).read_bytes()
    assert len(old)==len(new) and [(i,a,b) for i,(a,b) in enumerate(zip(old,new)) if a!=b]==[(0x3314c,0x75,0xeb)]
    # 실제 세 PE와 새 본체/기하 내보내기의 SHA를 확인한다.
    for binary,data in r['editions'].items():
        assert data['binary_sha256']==digest(ROOT/BINARIES[binary]) and data['assert_calls']==0
        # 각 출력 파일의 전체 바이트를 검증한다.
        for name,sha in data['exports'].items(): assert sha==digest(ROOT/f'extracted/graphremove/{binary}'/name)
    # 부모의 준비/관찰 코드도 당시 바이트여야 한다.
    for name,sha in r['dependencies'].items(): assert sha==digest(ROOT/'tools'/name)
    path=ROOT/'cpppj/tests/fixtures/graphremove-x86.tsv'
    assert r['fixture_sha256']==digest(path)
    counts=collections.Counter(line.split('\t')[0] for line in path.read_text(encoding='utf-8').splitlines())
    assert all(counts[name]==count for name,count in r['public_calls'].items())
    assert sum(r['public_calls'].values())==r['total_calls'] and counts['Begin']==r['sequences']
    print(f'삭제 준비 SHA/호출 수 확인: {r["total_calls"]}회')


if __name__=='__main__':
    # 검증 모드는 결과의 바이트/호출 수만 읽고 게임/GUI/OS를 실행하지 않는다.
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    verify() if parser.parse_args().verify else generate()
