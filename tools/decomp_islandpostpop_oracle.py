#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""noIsland postPop·3×3 소유자 전파를 세 실제 PE의 제한 x86으로 대조한다.

프레임 선택·서/북 지도 조회·F 받침 생성 인자·H 소유자 조회·연결 순회·3×3 소유자 전파 몸체와
일반 탐색/프레임 검색은 실제 명령이다. 생성·가상 소유자·Pop·표시 갱신·공통 postPop은 기록 대체다.
진단 문자열 작성/표시/assert도 기록 대체이며 손상된 받침의 효과 이후 진단 발생 여부를 비교한다.
원본 게임·OS·업데이터는 실행하지 않는다. 마지막 디컴파일 PC: VM-W11-CODEX, 2026-10-08.
"""
import argparse
import collections
import csv
import importlib.metadata
import itertools
import json
import random
import struct
from pathlib import Path

from decomp_bridgeevent_oracle import ROOT, SPECS, CONTROLS, BRIDGE, BRIDGE_TYPE, VTABLE, TYPES, CODES, STOP, digest, float_bits
from decomp_bridgeconnect_oracle import ConnectOracle, ISLE, ISLAND, STALAG, NO_ISLAND, node, scene_of, scene_columns
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 합성 가상 표시 갱신(+0x88)의 진입점과 입력 생성용 시드다.
UPDATE = STOP + 0x180
SEED = 0x4423b0
# 실제 noIsland 첫 프레임의 3×3 방향 글자 순서. 입력일 뿐 기대값은 기계어로 계산한다.
LETTERS = 'FCG BAD IEH'.replace(' ', '')
# 판본별 새 몸체 진입점·지도/중첩/억제 전역·진단 경계다.
EXTRA = {
    'originals': dict(entries=dict(SurfacePostPop=0x4423b0, SupportOwner=0x44bd20, SurfaceLetter=0x442350),
        surface=0x5c84bc, owner_surface=0x5c84bc, depth=0x5c89b8, suppress=0x5c85d4,
        stubs={0x4a8a00:'format',0x436d20:'diagnostic'}),
    'originalCD': dict(entries=dict(SurfacePostPop=0x4d2790, SupportOwner=0x461f00, SurfaceLetter=0x4d03a0),
        surface=0x5670cc, owner_surface=0x52d590, depth=0x5178d4, suppress=0x51894c,
        stubs={0x4205b0:'format',0x4875b0:'diagnostic'}),
}
EXTRA['original1037'] = EXTRA['originalCD']
# 독립 기대값과 SHA 감사 기록의 출력 위치다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/islandpostpop-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-islandpostpop-evidence.json'


class SurfaceOracle(ConnectOracle):
    """연결 실행기에 noIsland 접두/소유자 몸체와 진단·표시 효과 대체를 더한다."""
    def __init__(self, edition):
        """새 내보내기의 불연속 범위를 허용하며 합성 가상 표에 표시 갱신을 연결한다."""
        super().__init__(edition)
        self.extra = EXTRA[edition]
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], **self.extra['entries']))
        self.stubs.update(self.extra['stubs'])
        self.stubs[UPDATE] = 'update'
        self.mu.mem_write(VTABLE + 0x88, struct.pack('<I', UPDATE))
        paths = [ROOT / f'extracted/islandpostpop/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.exports.extend(paths)
        # 내보낸 몸체 이외의 명령은 부모 실행기가 거부한다.
        with paths[1].open(encoding='utf-8') as fp:
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = (int(value,16) for value in part.split('-'))
                    self.allowed.append((low,high+1))

    def on_instruction(self, mu, address, size, data):
        """표시/진단/문자열 작성 경계만 대체하며 프레임 검색 실패 등 예상하지 못한 assert는 거부한다."""
        name = self.stubs.get(address)
        if address == self.spec['assert_report'] and self.diagnostic_pending:
            name = 'expected_assert'
        if name not in ('update','format','diagnostic','expected_assert'):
            super().on_instruction(mu,address,size,data)
            return
        self.instructions += 1
        self.stub_calls[name] += 1
        esp = mu.reg_read(UC_X86_REG_ESP)
        ret = int.from_bytes(mu.mem_read(esp,4),'little')
        if name == 'update':
            self.events.append(f'U:{self.sid_of(mu.reg_read(UC_X86_REG_ECX))}')
        elif name == 'diagnostic':
            self.events.append('D')
            self.diagnostic_pending = True
        elif name == 'expected_assert':
            self.diagnostic_pending = False
        mu.reg_write(UC_X86_REG_ESP,esp+4)
        mu.reg_write(UC_X86_REG_EIP,ret)

    def prepare_surface(self, scene, depth, suppress):
        """실제 해시 머리 지도와 프레임 코드·중첩 전역을 넣고 소유자 효과에 필요한 입력만 준비한다."""
        self.prepare_scene(scene)
        # 별도 표면 지도 전역은 0단계 해시 배열을 가리킨다. 기대 결과는 계산하지 않는다.
        for name in ('surface','owner_surface'):
            self.mu.mem_write(self.extra[name],struct.pack('<I',self.bases[0]))
        self.mu.mem_write(self.extra['depth'],struct.pack('<I',depth))
        self.mu.mem_write(self.extra['suppress'],struct.pack('<I',suppress))
        codes = bytes(value for letter in LETTERS for value in (ord(letter),ord('P'),1,0))
        address = CODES + (NO_ISLAND-BRIDGE_TYPE)*0x400
        self.mu.mem_write(address,codes)
        self.mu.mem_write(TYPES+NO_ISLAND*self.type_stride+0x114,struct.pack('<I',len(LETTERS)))
        frame_address, width = self.frame_field(BRIDGE)
        self.write_ranges.append((frame_address,frame_address+width))
        self.diagnostic_pending = False

    def run_case(self, kind, scene, flags, depth, suppress, control):
        """몸체 호출 뒤 효과 순서/생성 슬롯/풀 전체를 관찰한다. 두 정밀도는 입력부터 다시 실행한다."""
        self.prepare_surface(scene,depth,suppress)
        source = next(n for n in scene['nodes'] if n[0]==BRIDGE)
        try:
            if kind == 'SurfacePostPop':
                self.call(kind,[flags],self.slot(BRIDGE),4,control,returns_float=False)
            else:
                self.call(kind,[source[4],source[5],flags],0,0,control,returns_float=False)
        except RuntimeError as error:
            raise RuntimeError(f'{kind} {self.edition} flags={flags} depth={depth} suppress={suppress} '
                               f'source={source} events={self.events}') from error
        observed = self.observed(False)
        return observed[:3]


def post_inputs():
    """서/북의 모든 글자, flags·중첩/억제·소수 좌표·프레임 유지·실제 연결 순회를 포함한다."""
    result=[]
    # 서쪽/북쪽 칸은 없는 경우·다른 타입인 경우와 9개 글자를 모두 넣는다.
    for west,north,depth in itertools.product(range(-2,9),range(-2,9),(1,2)):
        nodes=[node(BRIDGE,NO_ISLAND,30.0,30.0,frame=4,level=0)]
        # 서/북 칸 프레임은 결과가 아니라 원본 몸체의 입력이다.
        for sid,letter,x,y in ((60,west,29.0,30.0),(61,north,30.0,29.0)):
            if letter==-2:continue
            nodes.append(node(sid,ISLE if letter==-1 else NO_ISLAND,x,y,frame=max(0,letter),level=0))
        # H가 선택될 때 왼쪽 위 소유자 조회가 유효하도록 전체 3×3과 받침/종유석을 입력한다.
        used={(n[4],n[5]) for n in nodes}
        used.update(((float_bits(29.0),float_bits(30.0)),(float_bits(30.0),float_bits(29.0))))
        sid=62
        for y in (28.0,29.0,30.0):
            for x in (28.0,29.0,30.0):
                item=node(sid,NO_ISLAND,x,y,frame=4,level=0)
                if (item[4],item[5]) not in used:nodes.append(item)
                sid+=1
        nodes.extend((node(80,ISLAND,30,30,frame=8,level=2),node(81,STALAG,30,30,frame=8,level=2)))
        result.append((scene_of(nodes,[],{BRIDGE:3,62:5}),1,depth,0))
    rng=random.Random(SEED)
    # 프레임 유지와 flag 비트 없는 경로, 양수/음수 방향의 바깥 후보를 함께 넣는다.
    for index in range(160):
        x,y=rng.choice(((30.0,30.0),(30.75,41.5),(1.25,1.75),(254.5,254.75)))
        frame=rng.randrange(9)
        nodes=[node(BRIDGE,NO_ISLAND,x,y,frame=frame,level=0,extra=rng.choice((0,1,8,0x80)))]
        # H의 왼쪽 위가 지도에 있도록 정수 칸을 넣으며 다른 타입의 머리도 조회한다.
        for i,(dx,dy) in enumerate(((-2,-2),(-1,0),(0,-1),(1,0),(0,1))):
            if x+dx<0 or y+dy<0:continue
            # H 조회가 올림에 가까운 표면 칸을 읽도록 그 머리만 정수 좌표로 입력한다.
            ox,oy=(int(x+0.9998999834060669)-2,int(y+0.9998999834060669)-2) if i==0 else (x+dx,y+dy)
            nodes.append(node(60+i,NO_ISLAND if i==0 else BRIDGE_TYPE,ox,oy,frame=4 if i==0 else 2,level=0))
        flags=rng.choice((0,2,4,0x10,1,0x11,0xffffffff))
        # 지도 가장자리 H는 소유자 전파를 억제해 원본 assert를 피한다.
        suppress=int(frame==8 and (x<2 or y<2)) or rng.randrange(2)
        result.append((scene_of(nodes,[],{BRIDGE:rng.randrange(9),60:4}),flags,0,suppress))
    return result


def owner_inputs():
    """3×3 순회·소수/가장자리 좌표·타입 제한 없음·받침 누락 진단·대상 타입 불일치 입력이다."""
    rng=random.Random(SEED^0x44bd20)
    result=[]
    for index in range(150):
        x,y=rng.choice(((30.0,30.0),(30.75,41.5),(1.25,1.75),(254.5,254.75)))
        nodes=[node(BRIDGE,NO_ISLAND if index%13 else ISLE,x,y,frame=4,level=0)]
        sid=60
        # 오른쪽 아래 대상 외의 칸을 x/y 순서로 입력하며 일부를 비워 진단 경로도 확인한다.
        for cx in range(int(x)-2,int(x)+1):
            for cy in range(int(y)-2,int(y)+1):
                if (cx,cy)==(int(x),int(y)) or cx<0 or cy<0 or (index%4==0 and rng.randrange(3)==0):continue
                nodes.append(node(sid,ISLE if sid%3==0 else NO_ISLAND,cx,cy,frame=4,level=0));sid+=1
        if index%5:nodes.append(node(80,ISLAND,x,y,frame=8,level=2))
        if index%7:nodes.append(node(81,STALAG,x,y,frame=8,level=2))
        # 소수 좌표의 진입 조건은 보정 지도 조회다. 일부 입력에만 그 칸의 표면 머리를 추가한다.
        if x!=int(x) and index%2:nodes.append(node(89,NO_ISLAND,int(x)+1,int(y)+1,frame=4,level=0))
        result.append((scene_of(nodes,[],{}),rng.choice((0,1,3,8,255)),0,0))
    return result


def generate(smoke=False):
    """세 PE와 두 정밀도에서 같은 관찰을 얻은 입력만 저장하고 도구/PE/내보내기 SHA를 기록한다."""
    groups=[('SurfacePostPop',post_inputs()),('SupportOwner',owner_inputs())]
    if smoke:groups=[(name,cases[:4]+cases[-4:]) for name,cases in groups]
    rows=[];scenes={};reports={}
    paths={Path(__file__),ROOT/'tools/ghidra/islandpostpop-functions.json',FIXTURE}
    # 부모 실행기의 도구 SHA도 감사 대상에 포함한다.
    for name in ('bridgeevent','neighbor','bridgeconnect'):paths.add(ROOT/f'tools/decomp_{name}_oracle.py')
    for edition in SPECS:
        oracle=SurfaceOracle(edition)
        for kind,cases in groups:
            for scene,flags,depth,suppress in cases:
                values=[oracle.run_case(kind,scene,flags,depth,suppress,c) for c in CONTROLS]
                if values[0]!=values[1]:raise RuntimeError(f'x87 정밀도 관찰 불일치: {kind} {edition}')
                columns=tuple(map(str,scene_columns(scene)))
                number=scenes.setdefault(columns,len(scenes))
                rows.append([kind,edition,number,flags,depth,suppress,*values[0]])
        reports[edition]=dict(binary=oracle.spec['binary'],sha256=oracle.sha256,
            native_calls=dict(oracle.native_calls),stub_calls=dict(oracle.stub_calls),assertions=oracle.assertions,
            entries={k:f'{v:08x}' for k,v in oracle.spec['entries'].items()},
            replaced_entry_points={f'{k:08x}':v for k,v in oracle.stubs.items()},instructions=oracle.instructions)
        paths.add(ROOT/oracle.spec['binary']);paths.update(oracle.exports)
        print(f'{edition}: {dict(collections.Counter(r[0] for r in rows if r[1]==edition))}',flush=True)
    if smoke:
        for row in rows[:4]:print('\t'.join(map(str,row)))
        return
    scene_rows=[['Scene',number,*columns] for columns,number in scenes.items()]
    header='# 제한 x86 noIsland postPop/받침 소유자 기대값. 생성/가상 소유자/Pop/표시/공통 postPop/진단은 기록 대체.\n# kind edition scene flags-or-owner depth suppress events born pool-adler\n'
    FIXTURE.write_text(header+'\n'.join('\t'.join(map(str,row)) for row in scene_rows+rows)+'\n',encoding='utf-8',newline='\n')
    counts=collections.Counter(r[0] for r in rows)
    report=dict(schema=1,primary_target='10.78',last_decompile_host='VM-W11-CODEX',
        method='Unicorn x86 32-bit; OS/API/file/window execution forbidden',unicorn_version=importlib.metadata.version('unicorn'),
        seed=SEED,x87_control_words=[hex(c) for c in CONTROLS],cases=dict(counts),total=len(rows),scenes=len(scenes),
        editions=reports,os_calls=0,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},
        limitations=['create/virtual owner/Pop/update/common postPop are recorded substitutes',
                     'diagnostic string formatting/display/assert are recorded substitutes; occurrence and effect order compared',
                     'synthetic types/frames and hash registration; no GUI/raw GameWorld/Graph activation',
                     'CD current and owner surface arrays point to the same input map here'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'noIsland postPop 검증 입력: {len(rows)}개')


def verify():
    """기계어 재실행 없이 SHA·행 수·원본 몸체와 예상 진단 assert만 사용했는지 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError(f'SHA 불일치: {name}')
    counts=collections.Counter(line.split('\t')[0] for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#'))
    if counts.pop('Scene',0)!=report['scenes'] or dict(counts)!=report['cases']:raise RuntimeError('행 수 불일치')
    for edition,data in report['editions'].items():
        if data['assertions'] or not all(data['native_calls'].get(name) for name in ('SurfacePostPop','SupportOwner')):
            raise RuntimeError(f'원본 몸체/예상 밖 assert 오류: {edition}')
        if data['stub_calls'].get('diagnostic',0)!=data['stub_calls'].get('expected_assert',0):raise RuntimeError('진단 경계 오류')
    print(f'islandpostpop 감사 통과: {report["total"]}개')


def main():
    """전체 생성·파일 없는 소규모 점검·SHA 감사 가운데 하나를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':
    main()
