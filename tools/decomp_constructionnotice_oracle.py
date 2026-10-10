#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건설 배치 통지 처리기 전체 분기·배치 이력·SID/genus 조회·좌표 거의 올림을 세 실제 PE에서 대조한다.

정리/배치/사제 조회·이동/발자국·접근점/contained 제거/환불/가상 삭제/갱신은 명시 경계다.
실제 게임·OS·창을 실행하지 않으며 허용한 함수 몸체 밖의 실행과 이력/스택 밖의 쓰기를 거부한다.
--smoke는 저장 없는 표본, --verify는 저장한 입력·출력·PE·내보내기의 SHA/ABI/사건 수 감사다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,STACK,STOP,digest
from decomp_priestgeometry_oracle import GeometryOracle,OUT
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 판본별 처리기·실제 helper·외부 효과·이력 전역 주소다.
SPEC={
 'originals':dict(entry=0x4441b0,clear=0x441fd0,place=0x442c80,builder=0x443000,priest=0x4921f0,
    contained=0x488fe0,remove=0x4b1a90,immobile=0x427030,foot=0x4ade70,approach=0x41db70,
    move=0x489080,refund=0x442a40,start=0x443fe0,refresh=0x4da650,item=0x40a840,genus=0x4ac200,
    snap=0x41d7a0,ftol=0x4e49c0,dais=0x5412ec,priest_type=0x5412d0,local=0x540c70,
    authority=0x540bc4,move_local=0x544d10,history=0x558ea4,counter=0x558e78),
 'originalCD':dict(entry=0x4d1d00,clear=0x4cff80,place=0x4d0ca0,builder=0x4d1120,priest=0x40d940,
    immobile=0x4e5150,foot=0x4aeec0,approach=0x440400,move=0x47d150,refund=0x4d1bc0,
    start=0x4d06a0,refresh=0x41eac0,item=0x40a540,genus=0x4abae0,snap=0x440a80,ftol=0x4f161c,
    dais=0x51cbd8,local=0x50f6c8,authority=0x540a2c,move_local=0x540c4c,history=0x589150,counter=0x53fc40),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 도구 전용 메시지·가상 표·삭제 경계 주소와 타입 번호다. 서로 겹치지 않는다.
MESSAGE,VTABLE,KILL=OUT+0x200,OUT+0x400,STOP+0x100
PLAIN,DAIS,PRIEST,OTHER=100,165,158,102
# 초기 이력 DWORD는 서로 다른 비트로 채운다. 기대 이력은 실제 몸체만 만든다.
HISTORY=tuple((0x3f800000+i*0x01020304)&0xffffffff for i in range(10))
FIXTURE=ROOT/'cpppj/tests/fixtures/constructionnotice-x86.tsv'
REPORT=ROOT/'cpppj/recovery-constructionnotice-evidence.json'
# 입력 칸 순서다. drift는 첫 이동 뒤 사제 좌표를 바꿔 재시도의 현재 좌표 읽기를 검사한다.
KEYS=('type','flags1','group','x','y','argument','direction','player','flags','quality','timeLow','timeHigh','count',
    'local','authority','moveLocal','dais','objectType','owner','genus','priestSid','secondPriest','priestState','priestGenus',
    'immobile','px','py','p1x','p1y','p2x','p2y','success1','success2','drift','counter','contained')


def case(**changes):
    """판정 결과 없이 기본 입력과 바꿀 칸만 조립한다."""
    value=dict(type=PLAIN,flags1=0x10,group=1,x=bits(20.5),y=bits(21.25),argument=7,direction=6,player=3,
        flags=0,quality=3,timeLow=0x12345678,timeHigh=0x40081234,count=1,local=3,authority=1,moveLocal=0,
        dais=DAIS,objectType=PLAIN,owner=8,genus=0x4000,priestSid=70,secondPriest=71,priestState=0,
        priestGenus=0x200000,immobile=0,px=bits(12.5),py=bits(13.75),p1x=bits(19.000002),p1y=bits(20.999998),
        p2x=bits(-1.25),p2y=bits(-0.75),success1=0,success2=1,drift=0,counter=0x87654321,contained=144)
    value.update(changes);return value


def inputs():
    """abstract·건설 사제 필요·권한·이력·소유 사제 상태·이동 결과·스냅 경계를 교차한다."""
    result=[]
    # HP/사제 필요/무관 비트 × 통지 플래그 × 로컬 여부 × 권한 × 로컬 사제 재이동.
    for flags1,flags,local,authority,move in itertools.product((0,0x10,0x8000,0x8010,0x10000010),(0,1,2,3,0xff),(3,5,0x103),(0,1),(0,1)):
        result.append(case(flags1=flags1,flags=flags,local=local,authority=authority,moveLocal=move))
    # 타입 번호 일치만 dais 판정이다. genus의 dais 비트만으로 abstract가 되지 않는다.
    for typ,dais,group,flags in itertools.product((PLAIN,DAIS),(PLAIN,DAIS),(0,9,10,11,0xffffffff),(0,1,2)):
        result.append(case(type=typ,dais=dais,group=group,flags=flags,flags1=0,genus=0x400000))
    # 첫 조각의 현재 genus: 관련 비트 각각과 무관 비트, 통지와 다른 타입/소유자를 섞는다.
    for genus,object_type,owner in itertools.product((0,0x200,0x4000,0x400000,0x404200,0x200000,0x80000,0xffffffff),(PLAIN,OTHER),(0,8)):
        result.append(case(genus=genus,objectType=object_type,owner=owner))
    # 사제 상태의 low 2bit만 검사한다. void(4)와 높은 비트는 거부 조건이 아니다.
    for state,genus,immobile,success in itertools.product((0,1,2,3,4,0x80,0xfc),(0,0x200000,0xffffffff),(0,1),((0,0),(0,1),(1,0))):
        result.append(case(priestState=state,priestGenus=genus,immobile=immobile,success1=success[0],success2=success[1],drift=1))
    # 첫/두 번째 조회 번호, 첫 SID 사용, raw double 비트 전달, 모든 바이트 입력의 경계다.
    for changes in (dict(priestSid=0,priestState=1),dict(secondPriest=0),dict(secondPriest=70),dict(count=2),dict(count=19),
        dict(player=0,local=0),dict(player=255,local=255),dict(x=0x80000000,y=0x7fc01234),
        dict(flags1=0x8010,local=5,timeLow=0xffffffff,timeHigh=0x7ff81234,argument=255,direction=255,quality=255)):
        result.append(case(**changes))
    # 단정도 거의 올림의 양/음수·경계·NaN·무한대·64비트 범위 밖 결과를 실제 CRT에 맡긴다.
    for x,y in ((0,0x80000000),(bits(20),bits(21)),(bits(20.25),bits(21.25)),(bits(-2.75),bits(-3.75)),
        (bits(20.000002),bits(20.999998)),(bits(-1.25),bits(-0.75)),
        (bits(255.99998),bits(0.000001)),(0x7fc01234,0x7f800000),(bits(3e9),bits(-3e9)),(bits(1e20),bits(-1e20))):
        result.append(case(p1x=x,p1y=y,p2x=y,p2y=x,success1=0,success2=0,drift=1))
    return result


class NoticeOracle(GeometryOracle):
    """처리기 몸체와 실제 helper만 실행하며 효과 경계의 인자와 순서를 기록한다."""
    def __init__(self,edition):
        """읽기 전용 PE와 새 허용 범위·가상 삭제 표를 준비한다."""
        super().__init__(edition);self.x=SPEC[edition];self.patch=edition=='originals'
        self.exports=[ROOT/f'extracted/constructionnotice/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 내보낸 불연속 함수 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 범위 끝은 포함 주소이므로 반열린 구간으로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_write(VTABLE+0x10,struct.pack('<I',KILL));self.stub_calls=collections.Counter();self.returns=0
        self.stubs={self.x[key]:key for key in ('clear','place','builder','priest','immobile','foot','approach','move','refund','start','refresh')}
        if self.patch:self.stubs.update({self.x[key]:key for key in ('contained','remove')})
        self.stubs[KILL]='destroy'

    def args(self,count):
        """현재 스택의 호출 인자 DWORD를 읽는다."""
        return struct.unpack(f'<{count}I',self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,4*count))

    def ret(self,value=0,purge=0):
        """명시 경계에서 원본 호출 규약대로 돌아간다. 보존 레지스터는 건드리지 않는다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);address=struct.unpack('<I',self.mu.mem_read(esp,4))[0]
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,address)

    def sid(self,address):
        """this 포인터를 실제 풀의 SID로 검증해 바꾼다."""
        offset=address-POOL
        if offset<0 or offset%self.stride or offset//self.stride>=128:raise RuntimeError('통지 경계 this 오류')
        return offset//self.stride

    def emit(self,letter,*values):
        """관찰 사건 하나를 문자열로 기록한다."""
        self.events.append(letter+':'+':'.join(map(str,values)))

    def on_instruction(self,mu,address,size,data):
        """명시 경계만 대체하며 그 밖의 명령은 원본 몸체 안에서만 허용한다."""
        key=self.stubs.get(address)
        if key is None:OwnerOracle.on_instruction(self,mu,address,size,data);return
        self.stub_calls[key]+=1;c=self.case;purge=0;value=0
        if key=='clear':self.emit('Q',*self.args(6))
        elif key=='place':
            typ,x,y,arg,direction,player,count,sids,abstract,quality=self.args(10)
            numbers=struct.unpack(f'<{count}H',mu.mem_read(sids,count*2))
            self.emit('P',typ,x,y,arg,direction,player,count,','.join(map(str,numbers)),abstract,quality)
            self.emit('H',bytes(mu.mem_read(self.x['history'],40)).hex(),struct.unpack('<I',mu.mem_read(self.x['counter'],4))[0])
            # 배치 경계가 타입/소유자 바이트를 바꾼 뒤 처리기는 현재 슬롯을 조회해야 한다.
            mu.mem_write(self.slot(numbers[0])+10,bytes([c['objectType']]))
            mu.mem_write(self.slot(numbers[0])+self.o['owner'],bytes([c['owner']]))
        elif key=='builder':
            _out,player,sid,low,high=self.args(5);self.emit('B',player,sid,low,high)
        elif key=='priest':
            player,=self.args(1);value=c['priestSid'] if self.priest_calls==0 else c['secondPriest'];self.priest_calls+=1;self.emit('R',player,value)
        elif key=='contained':
            typ,=self.args(1);value=c['contained'];self.emit('T',typ,value)
        elif key=='remove':self.emit('X',*self.args(4))
        elif key=='immobile':value=c['immobile'];self.emit('I',self.sid(mu.reg_read(UC_X86_REG_ECX)),value)
        elif key=='foot':
            out,flags=self.args(2)
            if flags or self.sid(mu.reg_read(UC_X86_REG_ECX))!=50:raise RuntimeError('발자국 경계 인자 오류')
            mu.mem_write(out,bytes(16));self.rectangle=out;value=out;purge=8
        elif key=='approach':
            out,x,y,flags=self.args(4)
            if mu.reg_read(UC_X86_REG_ECX)!=self.rectangle:raise RuntimeError('접근점의 발자국 this 오류')
            prefix='p1' if flags==7 else 'p2' if flags==3 else None
            if prefix is None:raise RuntimeError('접근 플래그 오류')
            point=(c[prefix+'x'],c[prefix+'y']);mu.mem_write(out,struct.pack('<2I',*point));value=out;purge=16
            self.emit('A',50,x,y,flags,*point)
        elif key=='move':
            words=self.args(8 if self.patch else 7);sid,x,y,zero,object_sid,*tail=words
            if zero or any(tail):raise RuntimeError('사제 이동의 0 인자 오류')
            value=c['success1'] if self.move_calls==0 else c['success2'];self.emit('M',sid,x,y,object_sid,value)
            if self.move_calls==0 and c['drift']:mu.mem_write(self.slot(c['priestSid'])+14,struct.pack('<2I',bits(15.25),bits(16.5)))
            self.move_calls+=1
        elif key=='refund':self.emit('F',*self.args(2))
        elif key=='destroy':self.emit('D',self.sid(mu.reg_read(UC_X86_REG_ECX)),*self.args(1));purge=4
        elif key=='start':self.emit('S',*self.args(2))
        elif key=='refresh':self.emit('W')
        else:raise RuntimeError('알 수 없는 통지 경계')
        self.ret(value,purge)

    def run(self,case,control):
        """새 입력으로 처리기 전체를 실행하고 사건·이력·정상 ABI 반환을 관찰한다."""
        self.case=case;self.events=[];self.priest_calls=0;self.move_calls=0;mu=self.mu;x=self.x
        # 처리기 전역만 합성 입력으로 쓴다.
        for key,value in (('dais',case['dais']),('local',case['local']),('authority',case['authority']),('move_local',case['moveLocal']),('counter',case['counter'])):
            mu.mem_write(x[key],struct.pack('<I',value))
        if self.patch:mu.mem_write(x['priest_type'],struct.pack('<I',PRIEST))
        mu.mem_write(x['history'],struct.pack('<10I',*HISTORY))
        # 통지 타입과 실제 배치 뒤 객체 타입의 표를 별도로 준비한다.
        for typ in set((case['type'],case['objectType'],PRIEST)):
            mu.mem_write(TYPES+typ*self.type_stride,bytes(self.type_stride))
        mu.mem_write(TYPES+case['type']*self.type_stride+0xe8,struct.pack('<I',case['flags1']))
        mu.mem_write(TYPES+case['type']*self.type_stride+0x9c,struct.pack('<I',case['group']))
        mu.mem_write(TYPES+case['objectType']*self.type_stride+0xec,struct.pack('<I',case['genus']))
        mu.mem_write(TYPES+PRIEST*self.type_stride+0xec,struct.pack('<I',case['priestGenus']))
        # 첫 조각과 첫 조회 사제 raw 입력이다. 두 번째 조회는 삭제 경계에만 쓰인다.
        for sid,typ,state in ((50,case['type'],0),(case['priestSid'],PRIEST,case['priestState'])):
            raw=bytearray(self.stride);struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;raw[11]=state
            struct.pack_into('<2I',raw,14,case['px'],case['py']);mu.mem_write(self.slot(sid),bytes(raw))
        sids=[50+i for i in range(case['count'])];message=bytearray(27+2*len(sids))
        message[4]=case['type'];struct.pack_into('<2I',message,5,case['x'],case['y'])
        message[13:18]=bytes(case[k] for k in ('argument','direction','player','flags','quality'))
        struct.pack_into('<2I',message,18,case['timeLow'],case['timeHigh']);message[26]=len(sids);struct.pack_into(f'<{len(sids)}H',message,27,*sids)
        mu.mem_write(MESSAGE,bytes(message));self.write_ranges=[(x['history'],x['history']+40),(x['counter'],x['counter']+4)]
        self.prepare_registers(control);mu.mem_write(STACK,struct.pack('<4I',STOP,9,0,MESSAGE));mu.emu_start(x['entry'],STOP,count=10000)
        self.check(control,STACK+4);self.returns+=1
        return ';'.join(self.events),bytes(mu.mem_read(x['history'],40)).hex(),struct.unpack('<I',mu.mem_read(x['counter'],4))[0]


def generate(smoke=False):
    """두 x87 제어 워드에서 일치한 독립 원본 출력만 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestgeometry_oracle.py',
        ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/constructionnotice-functions.json',FIXTURE}
    # 세 판본은 각각의 PE 기계어를 실행한다.
    for edition in SPECS:
        oracle=NoticeOracle(edition);cases=inputs()
        if smoke:cases=cases[::31]
        # 같은 입력을 두 정밀도로 새로 실행해 순서와 이력의 일치를 확인한다.
        for item in cases:
            observed=[oracle.run(item,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'통지 처리 x87 차이: {edition} {item}')
            rows.append([edition,','.join(str(item[k]) for k in KEYS),*observed[0]])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: constructionnotice {len(cases)}개 통과',flush=True)
    if smoke:return
    header=['# 실제 건설 통지 처리기 전체 분기·이력·SID/genus 조회·좌표 거의 올림. 외부 효과는 명시 경계.',
        '# 판본 입력('+ ' '.join(KEYS)+') 사건(Q 정리/P 배치/H 배치 시 이력/B 건설 사제/R 소유 사제/T contained 타입/X contained 삭제/I 이동 불가/A 접근점/M 이동/F 환불/D 삭제/S 건설 시작/W 갱신) 최종이력 최종카운터']
    FIXTURE.write_text('\n'.join(header)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',decompile_date='2026-10-10',controls=list(CONTROLS),
        os_calls=0,editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=[
        '전체 통지 처리 분기·이력 쓰기·SID 항목/genus 조회·거의 올림/CRT는 실제 명령; 정상 반환/보존 레지스터/스택/x87 검사',
        '정리·배치·사제 조회/이동·contained 타입/삭제·발자국/접근점·이동 불가·환불·가상 삭제·건설 시작·갱신은 명시 효과 경계',
        '배치 경계가 조각 타입/소유자를 바꾸고 첫 이동 경계가 사제 좌표를 바꾸는 합성 입력 포함; 실제 공간/경로 탐색/GUI 미실행',
        'C++ 어댑터의 실제 Clear/Place/Refund 연결은 별도 통합 검사에서 검증; 통지 wire 해독·서버 요청·건설 진행은 후속 작업']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'total: {len(rows)}')


def verify():
    """SHA·입력 순서·정상 반환·사건별 경계와 실제 helper 호출 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 저장 파일과 실제 PE/내보내기 변경을 검출한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('통지 근거 오류')
    letters=dict(clear='Q',place='P',builder='B',priest='R',contained='T',remove='X',immobile='I',approach='A',move='M',refund='F',destroy='D',start='S',refresh='W')
    # 각 판본의 원본 실행 및 경계 수를 저장 사건과 대조한다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];cases=inputs();x=SPEC[edition]
        if len(selected)!=len(cases) or item['cases']!=len(cases) or item['returns']!=2*len(cases) or item['assertions']:raise RuntimeError('통지 행/반환/assert 오류')
        events=collections.Counter();priest_genus=0
        # 입력 순서를 다시 확인하고 출력 사건만 센다. 기대 판정을 새로 계산하지 않는다.
        for row,c in zip(selected,cases,strict=True):
            if len(row)!=5 or row[1]!=','.join(str(c[k]) for k in KEYS) or len(row[3])!=80:raise RuntimeError('통지 입력/열 오류')
            events.update(e.split(':',1)[0] for e in row[2].split(';'))
            if ';R:' in row[2] and (c['priestState']&3)==0:priest_genus+=1
        for key,letter in letters.items():
            if item['substitutions'].get(key,0)!=2*events[letter]:raise RuntimeError('통지 경계 횟수 오류: '+key)
        calls=item['native_calls'];attempts=events['A']
        priest_branches=events['R']//2 if edition=='originals' else events['R']
        genus_calls=priest_branches+events['S']+priest_genus
        item_calls=len(cases)+events['B']+priest_branches+2*events['S']
        if calls.get(f'{x["item"]:08x}',0)!=2*item_calls or calls.get(f'{x["genus"]:08x}',0)!=2*genus_calls:raise RuntimeError('통지 실제 SID/genus 조회 누락')
        if edition=='originals' and calls.get('0049a840',0)!=2*(len(cases)+genus_calls):raise RuntimeError('통지 실제 타입 조회 누락')
        if item['substitutions'].get('foot',0)!=2*attempts or calls.get(f'{x["snap"]:08x}',0)!=2*attempts or calls.get(f'{x["ftol"]:08x}',0)!=4*attempts:raise RuntimeError('통지 발자국/스냅/CRT 오류')
        if calls.get(f'{x["entry"]:08x}',0)!=2*len(cases) or events['Q']!=len(cases) or events['P']!=len(cases) or events['W']!=len(cases):raise RuntimeError('통지 전체 몸체/정리/배치/갱신 누락')
    print(f'constructionnotice 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 감사 가운데 하나를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
