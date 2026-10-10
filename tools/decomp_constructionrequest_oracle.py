#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""서버 건설 요청 전체를 세 PE의 제한 x86으로 실행한다. 게임/OS는 실행하지 않는다.

실제 명령: 요청 전체 분기, decoder, 타입 발자국/좌표 자르기, 절삭/거의 올림, 일반 finder, spot bit 검사와 표면 조회.
명시 경계: 전체 MayPlace, 기존 유닛 밀어내기, 서버 확정, 확정 실패 정리. 탐색기 Begin 인자는 대체 없이 관찰한다.
--smoke는 무저장 표본, --verify는 저장된 입력/출력/PE/내보내기의 SHA·ABI·사건 수를 감사한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, TYPES, POOL, digest
from decomp_constructionclear_oracle import ClearOracle, clear_case, node_text, MAP, OTHER
from decomp_constructionplace_oracle import PLAIN, BRIDGE, ISLAND, NOISLAND
from decomp_damageablepredestroy_oracle import CONTROLS, bits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX

# 요청 진입점·외부 효과 경계·편집/플레이어/genus·spot 포인터 전역이다.
SPEC = {
    'originals': dict(entry=0x444760, may=0x49b510, displace=0x443800, confirm=0x444590, cancel=0x46e6c0,
        editor=0x594fb8, local=0x540c70, genus=0x5325b4, spots=0x5c7c44),
    'originalCD': dict(entry=0x4d17d0, may=0x445200, displace=0x4d2450, confirm=0x4d0f10, cancel=0x48f960,
        editor=0x540a1c, local=0x50f6c8, genus=0x53fc28, spots=0x52fe48),
}
SPEC['original1037'] = dict(SPEC['originalCD'])
# 분석 전용 spot 배열과 표면 SID, 입력 순서, 원본 요청 인자 순서다.
SPOTS, SURFACE = 0x14500000, 80
KEYS = ('type','argument','direction','x','y','player','authority','footX','footY','frames','default','otherFootX','otherFootY',
    'editor','local','flags1','flags2','genus','may','displace','first','second','spot','surface','islandX','islandY','flags','timeLow','timeHigh','quality','profile')
REQUEST_KEYS = ('type','x','y','player','argument','direction','flags','timeLow','timeHigh','quality')
FIXTURE = ROOT / 'cpppj/tests/fixtures/constructionrequest-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-constructionrequest-evidence.json'


def request_case(**changes):
    """별도 기대 계산 없이 기본 요청 입력에 바꿀 칸만 덮어쓴다."""
    case = clear_case()
    case.update(editor=0, local=3, flags1=0, flags2=0, genus=0x20000000, may=1, displace=1,
        first=1, second=1, spot=2, surface=0, islandX=3, islandY=3, flags=0x12345678,
        timeLow=0x89abcdef, timeHigh=0x40012345, quality=0x1ff, profile=0)
    case.update(changes)
    return case


def inputs(edition):
    """소유자 폭·abstract 조건·모든 칸·받침/거부·판본별 밀어내기와 float 경계를 교차한다."""
    cases = []
    directions = range(8) if edition == 'originals' else (0,2,4,6)
    # 조각 후보 조건: 없음·일치·소유자/타입/x/y 불일치·noIsland·비abstract·buried·두 후보·서버 번호.
    for profile, player, editor in itertools.product(range(12), (3,0x103,0xffffff03), (0,1)):
        cases.append(request_case(profile=profile, player=player, editor=editor, local=3))
    # 효과 경계: MayPlace 실패, 밀어내기 genus의 일치/불일치, 실패 반환, 받침 필요 여부와 확정 결과.
    for may, flags1, flags2, displace, first, second, spot, surface in itertools.product(
            (0,1), (0,0x400), (0,0x20000000), (0,1), (0,1), (0,1), (0,2), (0,1)):
        cases.append(request_case(may=may, flags1=flags1, flags2=flags2, displace=displace, first=first, second=second, spot=spot, surface=surface))
    # 발자국/좌표 거의 올림과 spot 사각형의 큰 모서리 절삭을 구분하는 입력이다.
    positions = ((20.5,21.25),(20.0,21.0),(0.5,0.5),(1.0,1.0),(-3.5,-2.5),(256.0,300.0),
        (20.00001,21.00001),(20.00011,21.00011),(255.99998,254.5))
    for position, foot, spot, surface in itertools.product(positions, ((1,1),(3,2),(10,10)), (0,2,4,8), (0,1)):
        cases.append(request_case(x=bits(position[0]), y=bits(position[1]), footX=foot[0], footY=foot[1],
            flags1=0x400, spot=spot, surface=surface, islandX=4, islandY=2, first=0, second=1))
    # 다리의 모든 패턴·방향에서 뒤쪽 칸의 충돌도 검사한다.
    for index, (pattern,direction) in enumerate(itertools.product(range(26),directions)):
        cases.append(request_case(type=BRIDGE, argument=pattern, direction=direction, frames=index%2,
            profile=index%12, footX=1, footY=1, flags2=0x20000000, displace=index%2))
    # 아홉 칸·빈 decoder, 0x400과 다른 플래그/마스크, 편집 플레이어가 DWORD 범위인 입력이다.
    for direction, profile in itertools.product(directions, (0,1,2,9,10,11)):
        cases.append(request_case(type=NOISLAND, argument=0, direction=direction, profile=profile))
    # decoder에 칸이 없어도 뒤쪽 효과는 진행하며 genus는 마스크와 겹칠 때만 호출한다.
    for default, local, genus, flags2 in itertools.product((0xffffffff,3), (0x103,3), (0x10,0x20000000), (0x10,0x20000000)):
        cases.append(request_case(default=default, editor=1, local=local, genus=genus, flags2=flags2, flags1=0x1400, spot=4))
    return cases


def candidates(case, cells, edition):
    """실제 decoder 좌표에 후보 입력을 놓는다. 어떤 후보가 요청을 거부시키는지는 계산하지 않는다."""
    result = []; profile = case['profile']; owner = case['local'] if case['editor'] else case['player']
    if not profile or not cells: return result
    # 칸마다 같은 후보를 만들거나 마지막 칸에만 충돌을 놓아 전체 순회를 확인한다.
    for index, (_,xb,yb) in enumerate(cells):
        x,y = (struct.unpack('<f',struct.pack('<I',value))[0] for value in (xb,yb))
        if not (1 <= x <= 255 and 1 <= y <= 255): continue
        typ,extra,node_owner = case['type'],1,owner & 0xff
        if profile == 2 or (profile == 9 and index == len(cells)-1): node_owner = (owner+1)&0xff
        if profile == 3: typ = OTHER
        if profile == 4: x += 0.125
        if profile == 5: y += 0.125
        if profile == 6: typ = NOISLAND; node_owner = (owner+1)&0xff
        if profile == 7: extra = 0; typ = OTHER
        if profile == 8: extra = 9; typ = OTHER
        sid = (15000 if edition == 'originals' else 6000) + index if profile == 11 else 50+index
        result.append(dict(sid=sid, type=typ, extra=extra, x=bits(x), y=bits(y), owner=node_owner, frame=0, level=index%4))
        if profile == 10:
            result.append(dict(result[-1], sid=100+index, owner=(owner+1)&0xff))
    return result


def spot_bytes(profile):
    """spot 입력: 0/2 전체 채움, 4는 bit 2 외의 비트, 8은 큰 모서리 다음 칸 하나만 bit 2다."""
    if profile != 8: return bytes([profile])*65536
    raw = bytearray(65536); raw[22*256+21] = 2
    return bytes(raw)


class RequestOracle(ClearOracle):
    """실제 요청/decoder/finder/spot/표면 몸체를 실행하고 네 외부 효과의 인자·순서를 기록한다."""
    def __init__(self, edition):
        """요청용 내보내기 범위와 spot 지도를 준비한다. 원본 PE는 가상 메모리에서만 읽는다."""
        super().__init__(edition); self.r = SPEC[edition]; self.mu.mem_map(SPOTS,65536)
        self.exports = [ROOT / f'extracted/constructionrequest/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed = []; self.entries = set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra 몸체의 불연속 범위만 실제 실행을 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 포함 끝 주소를 반열린 실행 범위로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high = (int(value,16) for value in part.split('-')); self.allowed.append((low,high+1))
        self.mu.mem_write(self.r['spots'],struct.pack('<I',SPOTS))

    def on_instruction(self, mu, address, size, data):
        """명시 경계만 대체하고 탐색기 Begin은 실제 실행한다. 요청의 조건은 재계산하지 않는다."""
        r = self.r
        if address == r['may']:
            typ = (mu.reg_read(UC_X86_REG_ECX)-TYPES)//self.type_stride
            self.events.append('M:'+':'.join(map(str,[typ,*self.args(6),self.case['may']])))
            self.stub_calls['M'] += 1; self.ret(self.case['may'],24); return
        if address == r['displace']:
            letter = 'D' if self.patch else 'V'; values = list(self.args(4 if self.patch else 3))
            if self.patch: values.append(self.case['displace'])
            self.events.append(letter+':'+':'.join(map(str,values))); self.stub_calls[letter] += 1
            self.ret(self.case['displace']); return
        if address == r['confirm']:
            result = self.case['first'] if self.confirm_index == 0 else self.case['second']; self.confirm_index += 1
            self.events.append('C:'+':'.join(map(str,[*self.args(10),result]))); self.stub_calls['C'] += 1; self.ret(result); return
        if address == r['cancel']:
            self.events.append('K:'+':'.join(map(str,self.args(4)))); self.stub_calls['K'] += 1; self.ret(); return
        if address == self.x['begin'] and self.observing:
            self.events.append('A:'+':'.join(map(str,struct.unpack('<4i',struct.pack('<4I',*self.args(4)))))); self.areas += 1
        OwnerOracle.on_instruction(self,mu,address,size,data)

    def prepare(self, case, nodes):
        """입력 전역·타입/프레임·후보/해시·spot과 buried 표면 객체를 쓴다. 기록 결과는 아직 없다."""
        super().prepare(case,nodes); mu = self.mu; self.confirm_index = 0
        # 세 현재 전역 값을 DWORD로 공급한다.
        for key in ('editor','local','genus'): mu.mem_write(self.r[key],struct.pack('<I',case[key]))
        mu.mem_write(TYPES+case['type']*self.type_stride+0xe8,struct.pack('<2I',case['flags1'],case['flags2']))
        mu.mem_write(TYPES+ISLAND*self.type_stride+self.s['foot'],struct.pack('<2i',case['islandX'],case['islandY']))
        if case['type'] != NOISLAND: mu.mem_write(TYPES+NOISLAND*self.type_stride+self.s['foot'],struct.pack('<2i',1,1))
        mu.mem_write(SPOTS,spot_bytes(case['spot'])); mu.mem_write(POOL,bytes(self.stride))
        # 표면 조회의 입력 버킷은 +0.9999 뒤 절삭한다. 후보 등록 체인을 잃지 않게 buried 객체를 머리에 잇는다.
        x,y = (struct.unpack('<f',struct.pack('<I',case[k]))[0] for k in ('x','y'))
        cx,cy = int(x+0.9998999834060669),int(y+0.9998999834060669)
        if case['surface'] and 0<=cx<256 and 0<=cy<256:
            address = self.bases[0]+(cy*256+cx)*2; raw = bytearray(self.stride); raw[10] = NOISLAND
            raw[self.o['extra']] = 9; struct.pack_into('<2f',raw,14,float(cx),float(cy))
            struct.pack_into('<H',raw,4,struct.unpack('<H',mu.mem_read(address,2))[0])
            mu.mem_write(self.slot(SURFACE),bytes(raw)); mu.mem_write(address,struct.pack('<H',SURFACE))
        elif case['surface']: mu.mem_write(POOL+self.o['extra'],b'\x01')

    def request(self, case, nodes, control):
        """전체 몸체의 반환/사건을 기록하고 풀·해시·spot 불변과 정상 반환 ABI/x87을 확인한다."""
        self.prepare(case,nodes); mu = self.mu; self.write_ranges = []; self.observing = True
        before = [bytes(mu.mem_read(address,length)) for address,length in ((MAP,self.map_end-MAP),(SPOTS,65536),(POOL,(self.p['server_first']+16)*self.stride))]
        try: self.invoke(self.r['entry'],[case[k] for k in REQUEST_KEYS],control)
        finally: self.observing = False
        after = [bytes(mu.mem_read(address,length)) for address,length in ((MAP,self.map_end-MAP),(SPOTS,65536),(POOL,(self.p['server_first']+16)*self.stride))]
        if before != after: raise RuntimeError('요청 몸체가 풀/공간 입력을 수정함')
        return [mu.reg_read(UC_X86_REG_EAX), ';'.join(self.events)]


def generate(smoke=False):
    """세 PE·두 x87 정밀도로 요청 전체 관찰을 만들고 입력/도구/원본의 SHA 근거를 저장한다."""
    rows = []; editions = {}
    paths = {Path(__file__), FIXTURE, ROOT/'tools/ghidra/constructionrequest-functions.json',
        *[ROOT/f'tools/decomp_{name}_oracle.py' for name in ('owner','priestgeometry','constructionplace','constructionclear','damageablepredestroy')]}
    # 각 PE를 별도 에뮬레이터에서 독립 실행한다.
    for edition in SPECS:
        oracle = RequestOracle(edition); cases = inputs(edition); node_count = 0
        if smoke: cases = cases[::31]
        # 실제 decoder로 후보의 자리만 읽은 뒤 전체 요청의 두 관찰을 비교한다.
        for case in cases:
            cells = oracle.cells(case); nodes = candidates(case,cells,edition)
            observed = [oracle.request(case,nodes,control) for control in CONTROLS]
            if observed[0] != observed[1]: raise RuntimeError(f'요청 x87 정밀도 차이: {edition} {case}')
            rows.append([edition,','.join(str(case[k]) for k in KEYS),node_text(nodes),*observed[0]]); node_count += len(nodes)
        editions[edition] = dict(cases=len(cases),nodes=node_count,areas=oracle.areas,returns=dict(oracle.returns),
            native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports); paths.add(ROOT/oracle.spec['binary']); print(f'{edition}: constructionrequest {len(cases)}개 통과',flush=True)
    if smoke: return
    header = ['# 실제 서버 요청 전체와 decoder/발자국/finder/spot/표면. MayPlace·밀어내기·확정·실패 정리만 명시 경계.',
        '# 판본 입력('+ ' '.join(KEYS)+') 후보(번호,타입,extra,x,y,소유자,프레임,해시단계) 반환 사건(M:MayPlace,A:범위,D/V:밀어내기,C:확정,K:실패 정리)']
    FIXTURE.write_text('\n'.join(header)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',decompile_date='2026-10-11',controls=list(CONTROLS),
        os_calls=0,editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['요청 전체·decoder·발자국/좌표 자르기·절삭/거의 올림·일반 탐색·spot/표면 실제 몸체와 정상 반환 ABI/x87',
            'MayPlace·유닛 밀어내기·확정·확정 실패 정리는 명시 경계이며 이 도구에서는 실제 하위 효과 미실행',
            '합성 타입/프레임/풀/4단계 해시/spot 입력; 유한 좌표와 1~10 발자국, 후보 1~255; 원본 assert 경로 제외',
            '로컬 요청/커서·실제 유닛 밀어내기/실패 정리·게임/GUI/메시지 미실행']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(total=len(rows)),ensure_ascii=False))


def verify():
    """기대값을 재계산하지 않고 SHA·입력 순서·사건/반환 수와 경계 몸체 미실행을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 도구와 모든 입력/기록 파일이 생성 당시 바이트 그대로인지 확인한다.
    for name, expected in report['files'].items():
        if digest(ROOT/name) != expected: raise RuntimeError('SHA 불일치: '+name)
    if report['total'] != len(rows) or report['controls'] != list(CONTROLS) or report['os_calls']: raise RuntimeError('요청 근거 오류')
    # 판본별로 행과 원본/경계 호출 수를 교차 감사한다.
    for edition, item in report['editions'].items():
        selected = [row for row in rows if row[0] == edition]; cases = inputs(edition); r = SPEC[edition]
        if len(selected) != len(cases) or item['cases'] != len(cases) or item['assertions']: raise RuntimeError('요청 행/assert 오류')
        nodes = 0
        # 입력 순서는 생성 계획과 같아야 하며 후보 텍스트는 입력 개수만 센다.
        for row, case in zip(selected,cases,strict=True):
            if len(row)!=5 or row[1].split(',') != [str(case[k]) for k in KEYS] or row[3] not in ('0','1'): raise RuntimeError('요청 입력/반환 오류')
            nodes += 0 if row[2] == '-' else len(row[2].split(';'))
        events = [event for row in selected for event in row[4].split(';')]
        if nodes != item['nodes'] or item['areas'] != 2*sum(event.startswith('A:') for event in events): raise RuntimeError('요청 후보/탐색 수 오류')
        if item['returns'].get(f'{r["entry"]:08x}',0) != 2*len(selected) or item['native_calls'].get(f'{r["entry"]:08x}',0) != 2*len(selected): raise RuntimeError('요청 진입/반환 오류')
        # 경계 사건은 두 정밀도의 대체 호출 수와 정확히 대응해야 한다.
        for letter in 'MDVCK':
            if item['substitutions'].get(letter,0) != 2*sum(event.startswith(letter+':') for event in events): raise RuntimeError('요청 경계 수 오류: '+letter)
        if any(item['native_calls'].get(f'{r[key]:08x}',0) for key in ('may','displace','confirm','cancel')): raise RuntimeError('요청 경계 몸체 실행')
    print(f'constructionrequest 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--smoke',action='store_true'); parser.add_argument('--verify',action='store_true'); args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__': main()
