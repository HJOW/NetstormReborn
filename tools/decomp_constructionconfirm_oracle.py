#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""서버의 건설 확정(Construction.cpp 00444590 / CD 004d0f10)을 세 실제 PE에서 대조한다.

실제 명령으로 실행하는 것: 확정 전체 몸체(사제 조회 조건·스택 확보·조각 순회·SID 목록/메시지 필드 쓰기·기록기 조건),
CanonDecoder 생성/진행과 프레임 검색, 메시지 항목 주소 계산.
대체하는 것(명시 경계): 지을 사제 조회, 메시지 생성/전송, SID 생성, 통지 처리기, 기록기 활성 조회/기록.
원본 assert 보고에 닿는 입력(조각 20개 이상)은 만들 수 없다(패턴은 최대 15칸). 원본 게임/OS는 실행하지 않는다.

python -X utf8 tools/decomp_constructionconfirm_oracle.py            # 기대값·근거 기록 생성
python -X utf8 tools/decomp_constructionconfirm_oracle.py --smoke    # 저장 없이 표본만 실행
python -X utf8 tools/decomp_constructionconfirm_oracle.py --verify   # 저장된 기록·입력 파일의 SHA/행 수/사건 수 확인
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,digest
from decomp_constructionplace_oracle import PlaceOracle,PLAIN,BRIDGE,ISLAND,NOISLAND
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX

# 판본별 진입점·명시 경계·전역 주소다.
SPEC={
 'originals':dict(entry=0x444590,builder=0x48fdb0,message=0x4df590,create=0x4af530,send=0x4e2e30,handle=0x4441b0,
    active=0x495710,record=0x495730,skip=0x54db14,enabled=0x59a7e4,recorder=0x59a7e0,message_id=0x52edea),
 'originalCD':dict(entry=0x4d0f10,builder=0x406f80,message=0x418b60,create=0x4ab390,send=0x4c0420,handle=0x4d1d00,
    active=0x49f2b0,record=0x49f330,skip=0x52e8a8,enabled=0x537214,recorder=0x537210,message_id=0x50ffc8),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 기록기 객체의 입력 주소, 새 SID가 서버 영역 첫 번호에서 떨어진 거리, 타입 flags1의 "사제가 지어야 함" 비트다.
RECORDER,BORN,NEEDS_BUILDER=0x1234abcd,8,0x8000
FIXTURE=ROOT/'cpppj/tests/fixtures/constructionconfirm-x86.tsv'
REPORT=ROOT/'cpppj/recovery-constructionconfirm-evidence.json'
# 입력 칸 순서다. count는 실제 decoder가 센 조각 수이며 비교용 정보다(확정 함수의 인자가 아니다).
KEYS=('type','x','y','player','argument','direction','flags','timeLow','timeHigh','quality','flags1','skip','builder',
    'enabled','active','frames','default','count')


def confirm_case(**changes):
    """확정 입력의 기본값에 바꿀 칸만 덮어쓴다. 기본은 사제가 필요 없는 한 칸 타입이다."""
    case=dict(type=PLAIN,x=bits(20.5),y=bits(21.25),player=3,argument=7,direction=0,flags=0,timeLow=0,timeHigh=0,quality=0,
        flags1=0,skip=0,builder=77,enabled=0,active=0,frames=0,default=3,count=0)
    case.update(changes)
    return case


def inputs(edition):
    """사제 조회 조건·조각 수·메시지 필드의 폭·기록기 조건을 교차한다. 기대 결과는 원본 기계어가 정한다."""
    cases=[];directions=(0,1,2,3,4,5,6,7) if edition=='originals' else (0,2,4,6)
    # 사제 조회: 타입 비트 × 호출 플래그 bit 0 × 건너뛰기 전역 × 조회 결과.
    for flags1,flags,skip,builder in itertools.product((0,NEEDS_BUILDER,NEEDS_BUILDER|0x10,0x7fff,0xffffffff),(0,1,2,3,0xfe,0xff),(0,1),(0,77)):
        cases.append(confirm_case(flags1=flags1,flags=flags,skip=skip,builder=builder,player=1+(flags+skip)%8))
    # 메시지 필드의 폭: 바이트로 잘리는 인자와 그대로 실리는 좌표/시각 비트.
    for player,argument,quality,time in itertools.product((1,8,0x1ff,0xffffff09),(0,0x1234,0xffffffff),(0,1,3,0x1ff),
            ((0,0),(0x89abcdef,0x40234567),(0xffffffff,0x7ff80000))):
        cases.append(confirm_case(player=player,argument=argument,quality=quality,timeLow=time[0],timeHigh=time[1],
            x=bits(-2.5) if quality else bits(255.99998),y=bits(0.0) if argument else bits(300.25)))
    # 조각 수: 조각 없음(기본 프레임 -1), noIsland 9칸, island, 모든 다리 패턴 × 방향.
    cases.append(confirm_case(default=0xffffffff));cases.append(confirm_case(default=0xffffffff,flags1=NEEDS_BUILDER,builder=0))
    for direction in directions:
        cases.append(confirm_case(type=NOISLAND,argument=0,direction=direction,enabled=1,active=1))
        cases.append(confirm_case(type=ISLAND,argument=direction%2,direction=direction,enabled=direction%2,active=1))
    for pattern,direction in itertools.product(range(26),directions):
        cases.append(confirm_case(type=BRIDGE,argument=pattern,direction=direction,quality=(pattern+direction)%4,
            frames=(pattern+direction)%3,enabled=pattern%2,active=direction%2,player=1+pattern%8))
    # 기록기: 켜짐 × 활성 × 조각이 있는 타입.
    for enabled,active,typ in itertools.product((0,1,2),(0,1,0x100),(PLAIN,NOISLAND)):
        cases.append(confirm_case(type=typ,argument=0 if typ==NOISLAND else 9,enabled=enabled,active=active))
    return cases


class ConfirmOracle(PlaceOracle):
    """확정 전체 몸체와 실제 decoder를 실행하고 명시 경계의 인자와 순서를 기록한다."""
    def __init__(self,edition):
        """새 내보내기 범위와 이 함수의 경계 주소 표로 바꾼다."""
        super().__init__(edition);self.c=c=SPEC[edition]
        self.exports=[ROOT/f'extracted/constructionconfirm/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 내보낸 함수의 불연속 몸체만 실행을 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 범위 끝 주소는 포함이므로 반열린 구간으로 바꿔 둔다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        # 경계 주소 → (사건 글자, 처리기). 메시지 생성은 사건으로 남기지 않는다.
        self.stubs={c['builder']:('B',self.stub_builder),c['message']:('I',self.stub_message),c['create']:('C',self.stub_create),
            c['send']:('S',self.stub_send_notice),c['handle']:('H',self.stub_handle),c['active']:('Q',self.stub_active),c['record']:('R',self.stub_record)}
        self.stub_calls=collections.Counter();self.returns=collections.Counter();self.native_calls=collections.Counter()
        self.message_ids=set()

    def stub_builder(self):
        """지을 사제 조회: 인자를 기록하고 입력 결과를 돌려준다."""
        player,typ,x,y=self.args(4);self.events.append(f'B:{player}:{typ}:{x}:{y}');self.ret(self.case['builder'])

    def stub_message(self):
        """메시지 생성: 크기와 버퍼 인자를 확인하고 버퍼를 그대로 돌려준다."""
        number,_,_,size,buffer=self.args(5)
        if size!=0x200 or self.buffer is not None:raise RuntimeError('메시지 생성 인자/횟수 오류')
        self.message_ids.add(number);self.buffer=buffer;self.ret(buffer)

    def stub_create(self):
        """SID 생성: 타입과 플래그를 기록하고 서버 영역의 다음 번호 슬롯을 돌려준다."""
        typ,flags=self.args(2);sid=self.p['server_first']+BORN+self.born;self.born+=1
        self.mu.mem_write(self.slot(sid),bytes(self.stride));self.events.append(f'C:{typ}:{flags}');self.ret(self.slot(sid))

    def notice(self,message):
        """메시지 본문의 필드를 원본 위치에서 읽어 한 줄로 만든다."""
        raw=bytes(self.mu.mem_read(message,0x1b+2*20));count=raw[0x1a]
        if count>19:raise RuntimeError('SID 개수 범위 오류')
        sids=struct.unpack_from(f'<{count}H',raw,0x1b)
        fields=(raw[4],*struct.unpack_from('<2I',raw,5),raw[0xd],raw[0xe],raw[0xf],raw[0x10],raw[0x11],*struct.unpack_from('<2I',raw,0x12),count)
        return ':'.join(map(str,fields))+':'+(','.join(map(str,sids)) or '-')

    def stub_send_notice(self):
        """메시지 전송: 버퍼가 생성 경계의 것인지 확인하고 본문 필드를 기록한다."""
        message,=self.args(1)
        if message!=self.buffer:raise RuntimeError('전송 메시지 주소 오류')
        self.events.append('S:'+self.notice(message));self.ret()

    def stub_handle(self):
        """통지 처리기 직접 호출: 앞 두 인자와 같은 메시지인지를 기록한다."""
        first,second,message=self.args(3)
        if message!=self.buffer:raise RuntimeError('처리기 메시지 주소 오류')
        self.events.append(f'H:{first}:{second}:'+self.notice(message));self.ret()

    def stub_active(self):
        """기록기 활성 조회: this가 기록기 전역 값인지 확인하고 입력 결과를 돌려준다."""
        if self.mu.reg_read(UC_X86_REG_ECX)!=RECORDER:raise RuntimeError('기록기 this 오류')
        self.events.append('Q');self.ret(self.case['active'])

    def stub_record(self):
        """기록: 첫 SID 인자를 기록한다."""
        sid,=self.args(1)
        if self.mu.reg_read(UC_X86_REG_ECX)!=RECORDER:raise RuntimeError('기록기 this 오류')
        self.events.append(f'R:{sid}');self.ret(purge=4)

    def on_instruction(self,mu,address,size,data):
        """경계 주소는 대체 처리하고 그 밖에는 내보낸 몸체 안의 명령만 허용한다."""
        stub=self.stubs.get(address)
        if stub is None:OwnerOracle.on_instruction(self,mu,address,size,data);return
        letter,handler=stub;handler()
        if letter!='I':self.stub_calls[letter]+=1

    def confirm(self,case,control):
        """확정 전체를 실행하고 (사건, 반환값)을 돌려준다. 풀과 전역은 쓰지 않아야 한다."""
        mu=self.mu;c=self.c;self.setup(dict(case,flags2=0,maxHp=0,cost=0));self.case=case;self.born=0;self.buffer=None
        mu.mem_write(c['skip'],struct.pack('<I',case['skip']));mu.mem_write(c['enabled'],struct.pack('<I',case['enabled']))
        mu.mem_write(c['recorder'],struct.pack('<I',RECORDER));self.write_ranges=[]
        arguments=[case[k] for k in ('type','x','y','player','argument','direction','flags','timeLow','timeHigh','quality')]
        self.invoke(c['entry'],arguments,control)
        return [';'.join(self.events) or '-',mu.reg_read(UC_X86_REG_EAX)]


def generate(smoke=False):
    """세 실제 PE를 두 x87 정밀도로 실행해 같은 관찰만 fixture와 SHA 근거로 저장한다."""
    rows=[];editions={}
    paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestgeometry_oracle.py',ROOT/'tools/decomp_constructionplace_oracle.py',
        ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/constructionconfirm-functions.json',FIXTURE}
    # 판본마다 입력을 독립 실행한다.
    for edition in SPECS:
        oracle=ConfirmOracle(edition);cases=inputs(edition);pieces=0
        if smoke:cases=cases[::13]
        # 조각 수는 실제 decoder로 세어 입력 칸에 적어 둔다. 확정 함수에는 넘기지 않는다.
        for case in cases:
            case['count']=oracle.measure(dict(case,flags2=0,maxHp=0,cost=0));observed=[oracle.confirm(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'확정 x87 정밀도 차이: {edition} {case}')
            rows.append([edition,','.join(str(case[k]) for k in KEYS),*observed[0]]);pieces+=case['count']
        editions[edition]=dict(cases=len(cases),pieces=pieces,returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.stub_calls),assertions=oracle.assertions,message_ids=sorted(oracle.message_ids))
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: constructionconfirm {len(cases)}개 통과',flush=True)
    if smoke:return
    header=['# 실제 서버 건설 확정 전체 몸체. 사제 조회·메시지 생성/전송·SID 생성·통지 처리기·기록기는 명시 경계.',
        '# 판본 입력(쉼표: '+' '.join(KEYS)+') 사건 반환값. 통지 필드: 타입:x:y:인자:방향:플레이어:플래그:품질:시각하위:시각상위:개수:SID목록']
    FIXTURE.write_text('\n'.join(header)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-10',decompile_date='2026-10-10',controls=list(CONTROLS),
        os_calls=0,editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['확정 전체 몸체/사제 조회 조건/스택 확보/조각 순회/SID 목록·메시지 필드/기록기 조건과 CanonDecoder·프레임 검색 정상 반환/ABI/x87',
            '지을 사제 조회·메시지 생성/전송·SID 생성·통지 처리기·기록기 활성 조회/기록은 명시 경계',
            '합성 타입/프레임 표, 유한 좌표; 조각 20개 이상 assert는 패턴 크기상 입력 불가',
            '조각이 없을 때 기록기에 넘기는 SID는 원본에서 초기화되지 않은 지역 값이라 그 조합은 입력에서 제외',
            '요청 00444760·통지 처리기 004441b0·로컬 배치 004433b0·실제 메시지 계층/GUI 미실행']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(total=len(rows)),ensure_ascii=False))


def verify():
    """SHA·입력 순서·정상 반환 수·경계 사건 수를 감사한다. 기대 결과를 다시 계산하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 저장된 원본 출력과 도구/내보내기의 변경을 검출한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('건설 확정 근거 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];c=SPEC[edition];cases=inputs(edition)
        if len(selected)!=len(cases) or item['cases']!=len(cases) or item['assertions']:raise RuntimeError('건설 확정 행/assert 오류')
        pieces=0
        # 입력 칸이 생성 순서와 같은지 본다. 마지막 칸(조각 수)은 원본 decoder가 센 값이라 비교에서 뺀다.
        for row,case in zip(selected,cases,strict=True):
            values=row[1].split(',')
            if len(row)!=4 or len(values)!=len(KEYS) or values[:-1]!=[str(case[k]) for k in KEYS[:-1]]:raise RuntimeError('건설 확정 입력/열 오류')
            pieces+=int(values[-1])
        if pieces!=item['pieces'] or item['returns'].get(f'{c["entry"]:08x}',0)!=2*len(selected):raise RuntimeError('건설 확정 조각/반환 수 오류')
        if item['native_calls'].get(f'{c["entry"]:08x}',0)!=2*len(selected):raise RuntimeError('실제 확정 진입 수 오류')
        events=[event for row in selected for event in row[2].split(';') if event!='-']
        # 사건 글자별 수가 경계 처리 수와 같아야 한다. 경계 몸체는 한 번도 실행되지 않았어야 한다.
        for letter in 'BCSHQR':
            if item['substitutions'].get(letter,0)!=2*sum(event==letter or event.startswith(letter+':') for event in events):raise RuntimeError(f'경계 사건 수 오류: {letter}')
        if any(item['native_calls'].get(f'{c[name]:08x}',0) for name in ('builder','message','create','send','handle','active','record')):raise RuntimeError('경계 몸체 실행/대체 혼합')
    print(f'constructionconfirm 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사 중 하나를 실행한다. 게임/창은 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
