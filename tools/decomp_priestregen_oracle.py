#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 0x25a 전체 분기·실제 HP setter/중립 조건/난수를 세 PE에서 실행한다. 게임/OS 실행은 없다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_prieststate_oracle import StateOracle,STATE,CONTROLS,bits
from decomp_owner_oracle import ROOT,SPECS,TYPES,PLAYERS,TARGET,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 원본 회복 진입/외부 경계와 중립 조건의 두 전역이다.
REGEN={
    'originals':dict(entry=0x494580,damageable=0x44c060,audit=0x491680,shape=0x488fe0,occupied=0x4b1a90,
        action=0x491db0,measure=0x40ee90,blocked=0x5c89b8,mode=0x594fa4),
    'originalCD':dict(entry=0x40ccd0,damageable=0x462480,shape=0x47d020,occupied=0x4eb310,
        action=0x40d030,blocked=0x5178d4,mode=0x52e170),
}
REGEN['original1037']=REGEN['originalCD']
# 패치 전용 추적 필드의 순서: sample, sequence, observed, changed, remaining, gate, signed WORD sentinel.
TRACK=(0x540cb8,0x54d4e0,0x568af8,0x5318ec,0x5c89c4,0x594fd0,0x531890)
RNG=0x532710
# x87 반환값 주입/읽기 전용 합성 호출부. 원본 함수로 집계하지 않는다.
TEMP=0x13010000
CALLER=STOP+0x100
FIXTURE=ROOT/'cpppj/tests/fixtures/priestregen-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestregen-evidence.json'
# 유한/음수/0/quiet NaN payload는 반환 비트와 재예약 계약의 입력이다.
PAYLOAD=(bits(8.25),bits(0.0),bits(-0.0),bits(-1.0),bits(2.0),0x7fc12345)


def inputs():
    """HP 경계/폭과 추적 분기 입력을 만들며 기대 HP·반환·사건은 계산하지 않는다."""
    cases=[]
    # 큰 최대 HP에서는 signed ADD 감기와 CD WORD 저장도 실제 기계어가 판단한다.
    for hp,quarter,current,boss,allow,owner in itertools.product((200,201,5,65536,2147483647),(0,1),
        (-2147483648,-32769,-1,0,24,25,99,100,199,200,32768,2147483646),(0,1),(0,1),(0,1,8)):
        cases.append(dict(hp=hp,quarter=quarter,current=current&0xffffffff,boss=boss,allow=allow,owner=owner))
    # 외부 표시 효과 뒤 HP/소유자, 패치 audit 뒤 추적 재조회를 따로 교차한다.
    for after,owner in itertools.product((20,150,250),(0,1,8)):
        cases.append(dict(hp=200,quarter=0,current=90,boss=1,allow=1,owner=1,after=after,after_owner=owner,
            audit_change=1,blocked=0,mode=0,occupied=0))
    # 중립 경로는 차단 조건을 독립 교차하여 모든 게이트/점유/동작 분기를 실제로 실행한다.
    for current,boss,blocked,mode,occupied in itertools.product((20,99,100,200),(0,1),(0,1),(0,1),(0,1)):
        cases.append(dict(hp=200,quarter=0,current=current,boss=boss,allow=1,owner=0,
            blocked=blocked,mode=mode,occupied=occupied))
    # 조건 입력의 변형을 순환해 기본 격자에 고정 분포로 공급한다.
    for index,case in enumerate(cases):
        case.update(kind='Set',genus=0x210000,extra=32 if index%7==0 else 0,spot=6,new=0,
            after=case.get('after','-'),after_owner=case.get('after_owner',case['owner']),
            x=bits(20.000099182128906 if index%2 else 20.75),y=bits(21.9),
            blocked=case.get('blocked',(index//2)%2),mode=case.get('mode',(index//3)%2),
            occupied=case.get('occupied',(index//5)%2),count=index%5,
            payload=PAYLOAD[index%len(PAYLOAD)],rng=(0,1,0xffffffff,0x12345678)[index%4],
            sequence=(0,9597,9598,9600,2147483646,-4)[index%6],
            observed=(3,0,12345)[index%3],changed=19,remaining=(0,1,0xffffffff)[(index//2)%3],
            gate=(index//7)%2,sentinel=(-1,0,7)[(index//5)%3],
            measurement=(6999.0,7000.0,7000.0001,float('nan'))[(index//11)%4],
            audit_change=case.get('audit_change',0))
    return cases


class RegenOracle(StateOracle):
    """실제 회복/HP/중립/난수는 실행하고 미복원 공간·표시·추적/측정만 기록 대체한다."""
    def __init__(self,edition):
        """새 함수 범위와 x87 호출부를 추가한다. 기존 상태 조회 자료는 변경하지 않는다."""
        super().__init__(edition);self.r=REGEN[edition]
        self.exports.extend(ROOT/f'extracted/priestregen/{edition}/{name}' for name in ('creation.c','functions.tsv'))
        with self.exports[-1].open(encoding='utf-8') as fp:
            # 허용 몸체 밖 실제 명령은 기존 훅에서 거부한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.refresh=struct.unpack('<I',self.mu.mem_read(self.p['vtable']+0x88,4))[0]
        self.mu.mem_map(TEMP,0x1000)
        # 측정 대체는 fld qword [TEMP]; ret, 원본 반환 관찰은 fstp dword [TEMP+16]; jmp STOP이다.
        self.mu.mem_write(TEMP+0x100,b'\xdd\x05'+struct.pack('<I',TEMP)+b'\xc3')
        self.mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',TEMP+16)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)))

    def event(self,marker):
        """외부 경계 진입의 현재 HP/소유자/좌표 비트를 기록한다."""
        x,y=struct.unpack('<II',self.mu.mem_read(self.slot(TARGET)+14,8))
        owner=self.mu.mem_read(self.slot(TARGET)+self.o['owner'],1)[0]
        self.events.append(f'{marker}:{TARGET}:{self.hp()}:{owner}:{x}:{y}')

    def on_instruction(self,mu,address,size,data):
        """외부 몸체만 가로채고 실제 setter/중립 조건/난수에는 손대지 않는다."""
        if address in (TEMP+0x100,TEMP+0x106,CALLER,CALLER+6):return
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==self.refresh:
            marker='V';args=0;self.event(marker)
            if self.case['after']!='-':
                self.mu.mem_write(self.slot(TARGET)+26,struct.pack('<I' if self.stride==50 else '<H',int(self.case['after'])))
                self.mu.mem_write(self.slot(TARGET)+self.o['owner'],bytes([self.case['after_owner']]))
        elif address==self.r['damageable']:marker='D';args=0;self.event(marker)
        elif address==self.r['shape']:
            ret,type_number=struct.unpack('<II',mu.mem_read(esp,8));marker='F';args=0
            self.events.append(f'F:{type_number}');mu.reg_write(UC_X86_REG_EAX,0x12345678)
        elif address==self.r['occupied']:
            ret,sid,shape,zero1,zero2=struct.unpack('<IIIII',mu.mem_read(esp,20));marker='O';args=0
            self.events.append(f'O:{sid}:{shape}:{zero1}:{zero2}');mu.reg_write(UC_X86_REG_EAX,self.case['occupied'])
        elif address==self.r['action']:marker='N';args=0;self.event(marker)
        elif self.stride==50 and address==self.r['audit']:
            marker='A';args=0;self.events.append('A')
            if self.case['audit_change']:
                mu.mem_write(TRACK[2],bytes(mu.mem_read(TRACK[1],4)));mu.mem_write(TRACK[4],struct.pack('<I',1))
        elif self.stride==50 and address==self.r['measure']:
            self.stub_calls['M']+=1;self.events.append('M');mu.reg_write(UC_X86_REG_EIP,TEMP+0x100);return
        else:
            # StateOracle 기록은 U/R만 담당한다. afterHP 변경은 여기서는 V 경계에서 공급한다.
            after=self.case['after'];self.case['after']='-'
            try:super().on_instruction(mu,address,size,data)
            finally:self.case['after']=after
            return
        if marker in ('V','D','N') and mu.reg_read(UC_X86_REG_ECX)!=self.slot(TARGET):raise RuntimeError('외부 this 오류')
        self.stub_calls[marker]+=1;ret=struct.unpack('<I',mu.mem_read(esp,4))[0]
        mu.reg_write(UC_X86_REG_ESP,esp+4+args);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,case,control):
        """전체 0x25a 반환과 thiscall/x87 스택·raw/추적 쓰기 범위를 검증한다."""
        self.case=case;self.events=[];slot=self.slot(TARGET)
        raw=bytearray([0xab]*self.stride);struct.pack_into('<I',raw,0,self.p['vtable'])
        raw[10]=158;raw[11]=0;raw[self.o['extra']]=case['extra'];raw[self.o['owner']]=case['owner']
        struct.pack_into('<II',raw,14,case['x'],case['y'])
        struct.pack_into('<I' if self.stride==50 else '<H',raw,26,case['current'] if self.stride==50 else case['current']&0xffff)
        self.mu.mem_write(slot,bytes(raw))
        base=TYPES+158*self.type_stride;self.mu.mem_write(base,bytes(self.type_stride))
        self.mu.mem_write(base,struct.pack('<i',case['hp']));self.mu.mem_write(base+0xe8,struct.pack('<II',0x69012,case['genus']))
        self.mu.mem_write(self.p['quarter'],struct.pack('<I',case['quarter']));self.mu.mem_write(self.p['priest'],struct.pack('<I',158))
        self.mu.mem_write(self.p['boss'],struct.pack('<I',case['boss']))
        self.mu.mem_write(PLAYERS+case['owner']*self.player_stride+self.p['player_flag'],struct.pack('<I',case['allow']))
        self.mu.mem_write(0x5e4794 if self.stride==50 else 0x546778,bytes(4))
        self.mu.mem_write(self.r['blocked'],struct.pack('<I',case['blocked']));self.mu.mem_write(self.r['mode'],struct.pack('<I',case['mode']))
        self.write_ranges=[(slot+14,slot+22),(slot+26,slot+26+(4 if self.stride==50 else 2)),(TEMP+16,TEMP+20)]
        if self.stride==50:
            # 추적 전역은 원본 폭 그대로 초기화하고 그 필드 밖 쓰기를 거부한다.
            for address,name in zip(TRACK,('sample','sequence','observed','changed','remaining','gate','sentinel')):
                self.mu.mem_write(address,struct.pack('<H' if name=='sentinel' else '<I',case.get(name,0)&(0xffff if name=='sentinel' else 0xffffffff)))
                self.write_ranges.append((address,address+(2 if name=='sentinel' else 4)))
            self.mu.mem_write(RNG,struct.pack('<I',case['rng']));self.write_ranges.append((RNG,RNG+4))
            self.mu.mem_write(TEMP,struct.pack('<d',case['measurement']))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,slot)
        self.mu.mem_write(STACK,struct.pack('<IIII',CALLER,0x25a,case['count'],case['payload']))
        self.mu.emu_start(self.r['entry'],STOP,count=30000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+16:raise RuntimeError('회복 반환/스택 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('회복 x87 복구 오류')
        tracking='-';rng=case['rng']
        if self.stride==50:
            tracking=','.join(str(struct.unpack('<h' if address==TRACK[-1] else '<I',self.mu.mem_read(address,2 if address==TRACK[-1] else 4))[0]) for address in TRACK)
            rng=struct.unpack('<I',self.mu.mem_read(RNG,4))[0]
        return [struct.unpack('<I',self.mu.mem_read(TEMP+16,4))[0],';'.join(self.events) or '-',tracking,rng,bytes(self.mu.mem_read(slot,self.stride)).hex()]


def generate(smoke=False):
    """두 정밀도의 관찰이 일치한 입력만 UTF-8 자료로 저장한다."""
    cases=inputs();rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_prieststate_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/priestregen-functions.json',FIXTURE}
    if smoke:cases=cases[::101]
    columns=('hp','quarter','current','boss','allow','owner','extra','x','y','blocked','mode','occupied','count','payload','rng','sequence','observed','changed','remaining','gate','sentinel','measurement','after','after_owner','audit_change')
    for edition in SPECS:
        oracle=RegenOracle(edition)
        # 각 입력의 기대값은 실제 PE 명령에서만 얻는다.
        for case in cases:
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError(f'회복 x87 차이: {edition}')
            rows.append([edition,*[case[name] for name in columns],*values[0]])
        editions[edition]=dict(cases=len(cases),native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 회복 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 0x25a 전체 분기. 공간/표시/추적/측정 경계만 대체.\n# edition '+ ' '.join(columns)+' result events tracking rngOut slot\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-08',normal_return=True,os_calls=0,controls=list(CONTROLS),editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['회복 이벤트 분기만 실행. 다른 사제 이벤트/GUI/전체 Pop은 미복원',
            'Unpop/Repop·+0x88·damageable·shape/occupied·중립 동작·패치 audit/측정은 외부 대체',
            '난수/HP getter/setter/중립 조건/분기와 전역 필드 쓰기는 실제 명령']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """자료 SHA·행 수·실제 회복 진입·외부 대체 기록을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    if len(rows)!=report['total'] or not report['normal_return'] or report['os_calls']:raise RuntimeError('회복 실행 경계 오류')
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition]
        if item['cases']!=len(selected) or item['assertions'] or item['native_calls'].get(f'{REGEN[edition]["entry"]:08x}')!=2*len(selected):raise RuntimeError('회복 실제 진입/행 수 오류')
        if any(item['substitutions'].get(marker,0)==0 for marker in ('F','O','N')):raise RuntimeError('중립 후처리 분기 관찰 누락')
        for marker,count in item['substitutions'].items():
            if count!=2*sum(sum(part==marker or part.startswith(marker+':') for part in r[-4].split(';')) for r in selected):raise RuntimeError('회복 대체 수 오류')
    print(f'priestregen 검증 통과: {len(rows)}개')


def main():
    """전체 생성/무저장 smoke/저장 증거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
