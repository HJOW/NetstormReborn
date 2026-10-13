#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fort Money wrapper/시작SP/현재플레이어 지정과 분산저장소의 실제 양판본 몸체를 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_reward_oracle import RewardOracle, ROOT, ARENA, ARENA_SIZE, PLAYERS, CONTROLS, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

TEMPLATE, STREAM = ARENA + 0x30000, ARENA + 0x30020  # 에뮬레이터 내부의Template/실제3DWORD MemStream 입력이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fortmoney-x86.tsv'  # 원본기대값을재계산하지않는전수입력/출력이다.
REPORT = ROOT / 'cpppj/fortmoney.json'  # 현재PC·정상/assert·전체SP배열 출처를고정한다.
KEYS = ('kind','fort','mission','guard','client','special','local','difficulty','cdBase','excluded','depth','position','end','present','seed','fill','initial','owner','amount')  # 원본에공급하는독립입력필드순서다.
PLAN = {
    'originals': dict(load=0x4bf340, body=0x483300, setplayer=0x447810, lookup=0x44ceb0, assertion=0x4e0620,
                      depth=0x5c89b8, guard=0x5c85d4, client=0x595344, difficulty=0x52f7ee, checks=0x5c89c4),
    'originalCD': dict(load=0x4297f0, body=0x414720, lookup=0x44c4e0, assertion=0x490040,
                      depth=0x5178d4, guard=0x51894c, starting=0x5120d0)
}  # 실제wrapper/전역 주소이며호스트주소나게임실행에사용하지않는다.


def sample(**values):
    """기본입력만작성하며 SP/난수/반환의기대결과를계산하지않는다."""
    result = dict(zip(KEYS, (0,0,0,0,0,0,1,0,3000,0,0,0,4,1,0,0,0,1,7000)))
    result.update(values)
    return result


def inputs():
    """모드/난이도/현재플레이어flags와스트림경계·직접지정입력을방문한다."""
    rows = []
    # 주요모드8종·난이도3종·제외마스크3종·로컬1/8을독립교차한다.
    for index, values in enumerate(itertools.product(range(2), range(2), range(2), range(3), (0,0x1fe,4), (1,8))):
        fort, mission, guard, difficulty, excluded, local = values
        rows.append(sample(fort=fort, mission=mission, guard=guard, difficulty=difficulty, excluded=excluded, local=local,
                           client=index%2, special=(index//2)%2, initial=(index//3)%2,
                           seed=(0,0x12345678,0xffffffff)[index%3], fill=(0,0x55555555)[index%2], cdBase=(-10,2500,3000)[difficulty]))
    # 원본4바이트감김/끝signed비교 및missingTemplate진단뒤계속하는입력을보존한다.
    for position, end, present in itertools.product((0,0xfffffffc,0x7ffffffe), (0,4,0x80000000), (0,1)):
        rows.append(sample(kind=2,position=position,end=end,present=present,mission=1,excluded=0x1fe,depth=0xffffffff))
    # wrapper의missingTemplate보고뒤Money조회/현재pos초기화와추가끝진단을직접확인한다.
    for end in (0,4):
        rows.append(sample(present=0,end=end,position=8,depth=3))
    # 직접SetPlayerSp의로컬/비로컬/음수·범위밖owner와float반올림값을독립교차한다.
    for owner, amount, special, client in itertools.product((0,1,8,9,0xffffffff), (0,7000,0xffffffff,16777217), range(2), range(2)):
        rows.append(sample(kind=1,owner=owner,amount=amount,special=special,client=client,seed=0xabc123))
    return rows


class MoneyOracle(RewardOracle):
    """기존PE/분산SP 로더를재사용하고실행목록을Money8/4 몸체로좁힌다."""
    def __init__(self, edition):
        """보호PE 원문·현재PC 정밀범위·필수스트림조회/표시/진단경계를준비한다."""
        super().__init__(edition)
        self.p = PLAN[edition]
        self.s = dict(self.s, stubs=dict(self.s['stubs'], assert_=self.p['assertion']))
        self.stub_addresses = set(self.s['stubs'].values())
        self.exports = [ROOT / f'extracted/fortmoney/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed, self.entry_names = [], {}
        # Ghidra의실제분리범위만허용하며다른게임몸체는즉시거부한다.
        with self.exports[1].open(encoding='utf-8-sig') as handle:
            for row in csv.DictReader(handle, delimiter='\t'):
                self.entry_names[int(row['entry'],16)] = row['entry']
                for part in row['ranges'].split(';'):
                    low, high = (int(value,16) for value in part.split('-'))
                    self.allowed.append((low,high+1))
        self.mu.mem_map(0,0x1000)
        self.writes.append((0,4))
        # 실제wrapper의깊이/검사횟수쓰기와CD 현재SP만추가허용한다.
        for name in ('depth','checks'):
            if name in self.p:
                self.writes.append((self.p[name],self.p[name]+4))
        self.events = []
        self.boundaries = collections.Counter()

    def on_instruction(self, mu, address, size, data):
        """Money조회만공급하고실제SP설정/난수/SEH깊이몸체는그대로실행한다."""
        if address == self.p['lookup']:
            esp = mu.reg_read(UC_X86_REG_ESP)
            assert self.u32(esp+8) == 0
            assert bytes(mu.mem_read(self.u32(esp+4),6)) == b'Money\0'
            self.boundaries['section_lookup'] += 1
            self.put(STREAM+4,0)
            mu.reg_write(UC_X86_REG_EAX,STREAM)
            mu.reg_write(UC_X86_REG_ESP,esp+12)
            mu.reg_write(UC_X86_REG_EIP,self.u32(esp))
            return
        if address == self.s['stubs'].get('ui'):
            self.events.append('U')
        if address == self.p['assertion']:
            line = self.u32(mu.reg_read(UC_X86_REG_ESP)+12)
            self.events.append('A:P' if line == 0x2e else 'A:T')
        super().on_instruction(mu,address,size,data)

    def run_money(self, case, control):
        """같은원본입력을초기화하고전체현재SP/배열/난수/깊이·진단을관찰한다."""
        s, g, p = self.s, self.s['g'], self.p
        self.mu.mem_write(ARENA,bytes(ARENA_SIZE))
        self.mu.mem_write(0,bytes(4))
        self.seed, self.fill, self.malloc_next = case['seed'], case['fill'], 0
        self.hits.clear();self.stub_hits.clear();self.asserts=0;self.events=[];self.boundaries.clear()
        self.put(g['fixed_a'],case['fort']);self.put(g['half'],case['mission']);self.put(g['fixed_b'],case['special'])
        self.put(g['local'],case['local']);self.put(g['players'],PLAYERS)
        self.put(p['guard'],case['guard']);self.put(p['depth'],case['depth'])
        self.put(TEMPLATE,1 if case['present'] else 0)
        self.put(STREAM,ARENA+0x31000);self.put(STREAM+4,case['position']);self.put(STREAM+8,case['end'])
        # 실제Player9기록의flagsBYTE만원본배치에준비한다.
        for owner in range(9):
            self.put(PLAYERS+owner*s['players_stride']+0x50,0x20 if case['excluded']&(1<<owner) else 0,1)
        if self.patch:
            self.put(p['client'],case['client']);self.put(p['difficulty'],case['difficulty'],1);self.put(p['checks'],17)
            # 초기화전/후의게임난수·분산배열입력을같은기존몸체로구성한다.
            for name in ('rng','inited','offset','init_copy'):
                self.put(g[name],0)
            self.mu.mem_write(g['perm'],bytes(64));self.mu.mem_write(g['pools'],bytes(128))
            if case['initial']:
                self.call(0x40eba0,[],control)
        else:
            self.put(p['starting'],case['cdBase']);self.put(g['sp'],struct.unpack('<I',struct.pack('<f',123.5))[0])
        if case['kind']==1:
            self.call(p['setplayer'],[case['owner'],case['amount']],control)
        elif case['kind']==2:
            self.call(p['body'],[STREAM],control)
        else:
            self.call(p['load'],[],control,TEMPLATE)
        initialized = self.u32(g['inited']) if self.patch else 0
        pools = '-'
        values = '-'
        # 이미초기화된경우에만Get을실행한다. 관찰때문에lazy상태/난수를바꾸지않는다.
        if initialized:
            values = ','.join(map(str,self.read_sp(control)))
            pools = b''.join(bytes(self.mu.mem_read(self.u32(g['pools']+bit*4),36)) for bit in range(32)).hex()
        if not self.patch:
            values = str(self.u32(g['sp']))
        return [str(self.u32(p['depth'])),str(self.u32(STREAM+4)),str(self.u32(p['checks'])) if self.patch else '17',
                str(initialized),str(self.u32(g['offset'])) if self.patch else '0',str(self.u32(g['rng'])) if self.patch else '0',
                values,pools,';'.join(self.events) or '-',str(self.asserts)]


def generate(limit=None):
    """각고유입력의두x87 결과가같을때만정상/assert 분리fixture와현재SHA를기록한다."""
    rows, editions, paths = [], {}, [Path(__file__).resolve(),ROOT/'tools/decomp_reward_oracle.py',ROOT/'tools/ghidra/fortmoney-functions.json',
                                    ROOT/'cpppj/src/o/RawFortMoneyLoad.h',ROOT/'cpppj/src/o/RawFortMoneyLoad.cpp',ROOT/'cpppj/src/o/SpStore.h',ROOT/'cpppj/src/o/SpStore.cpp']
    # CD에없는patch 직접지정입력은그판본에추가하지않는다.
    for edition in PLAN:
        oracle = MoneyOracle(edition)
        cases = [case for case in inputs() if edition=='originals' or case['kind']!=1]
        if limit:
            cases = cases[:limit]
        calls, boundaries, normal, assertions = collections.Counter(), collections.Counter(), 0, 0
        # 고유입력한개를두정밀도로반복하지만관찰수는한개로센다.
        for case in cases:
            outputs = []
            for control in CONTROLS:
                outputs.append(oracle.run_money(case,control))
            assert outputs[0] == outputs[1],(edition,case)
            result = outputs[0]
            rows.append([edition,','.join(str(case[name]) for name in KEYS),*result])
            calls.update(oracle.hits);boundaries.update(oracle.boundaries);boundaries.update(oracle.stub_hits)
            assertions += result[-1]!='0';normal += result[-1]=='0'
        editions[edition] = dict(normal_cases=normal,assert_cases=assertions,native_calls=dict(calls),boundaries=dict(boundaries),x87_returns=len(cases)*2)
        paths.extend([ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'),*oracle.exports])
    FIXTURE.write_text('# 판본\t'+','.join(KEYS)+'\tdepth\tposition\tspCheckCount\tinitialized\toffset\trng\tSPbits\tpools\tevents\tasserts\n'+
                       '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths.append(FIXTURE)
    data = dict(host='HJOW-Athlon',last_decompile_host='HJOW-Athlon',last_decompile_date='2026-10-14',total=len(rows),editions=editions,
                files={path.relative_to(ROOT).as_posix():digest(path) for path in paths},game_runs=0,
                limitations=['실제파일섹션조회/시각재시드/힙확보/진단/SP표시는명시경계다.', '기존SP/난수body 재사용은새입력에더하지않는다.',
                             'Money4바이트값은원본몸체에서사용하지않는다. Badges/Technology/전체부모·GameWorld설치는후속이다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(total=len(rows),editions=editions),ensure_ascii=False))


def verify():
    """현재PE/소스/전용범위/fixture 전체바이트를감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    # 기록한원본과현재구현바이트를하나씩확인한다.
    for name, expected in data['files'].items():
        assert digest(ROOT/name)==expected,name
    print(f"FortMoney 현재SHA감사: {data['total']} 입력")


# 기본실행은전수이며 --limit은초기진단용부분입력이고 --verify는현재출처감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--limit',type=int)
    parser.add_argument('--verify',action='store_true')
    arguments=parser.parse_args()
    verify() if arguments.verify else generate(arguments.limit)
