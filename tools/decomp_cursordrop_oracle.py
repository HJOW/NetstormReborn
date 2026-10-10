#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""커서 내려놓기 전체·반올림/CRT·CD 잠금을 세 실제 PE에서 실행한다. 게임/OS/창은 실행하지 않는다.

Form/Gump 가상 효과, 위치 판정/준비, 강조/소리/기록/로컬 배치/SP/화면 갱신은 명시 경계다.
--smoke는 무저장 표본, --verify는 저장 입력/SHA/실제 진입·반환/사건 수를 감사한다.
"""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, TYPES, STOP, digest
from decomp_constructionplace_oracle import PlaceOracle, VTABLE
from decomp_damageablepredestroy_oracle import CONTROLS, bits
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 원본 전체 진입점과 명시 경계, 실제 전역 주소다. 분석용 가상 표/GUI 주소는 원본 파일에 쓰지 않는다.
GUI, SOURCE, DESTROY, POP, DETACH, ATTACH, DISPATCH, CLOSE = 0x14600000,*[STOP+0x400+i*0x20 for i in range(7)]
PREVIEW, FIRST, SECOND, TYPE = 50,77,88,100
SPEC = {
 'originals': dict(entry=0x4473e0,round=0x43fa60,query=0x445440,prepare=0x4458e0,highlight=0x4d5bb0,sound=0x4a9cb0,
    record=0x495460,local=0x4433b0,add=0x40eec0,show=0x43dd20,battleRefresh=0x4b54e0,invalid=0x446e70,
    piece=0x4468b0,activate=0x445aa0,cursor=0x4a4410,refresh=0x4d5c90,quality=30,
    globals=dict(lock=0x5325c8,legal=0x55a28c,preview=0x55a2a8,x=0x55a300,y=0x55a304,inputFlags=0x55a2ac,
      costFlags=0x55a2e0,legalGump=0x55a27c,cursorGump=0x55a274,type=0x55a280,argument=0x55a288,direction=0x55a2a4,
      recording=0x59a7dc,recorder=0x59a7e8,battle=0x594fbc,blocked=0x594fa4,screenX=0x55a2f8,screenY=0x55a2fc,width=0x55a270,
      queryMessage=0x542874,receiveMessage=0x542870)),
 'originalCD': dict(entry=0x43d660,round=0x48ee20,query=0x43c8c0,prepare=0x43cd70,highlight=0x41ad10,sound=0x438b50,
    record=0x49f080,local=0x4d14a0,add=0x414680,show=0x4eea70,battleRefresh=0x432770,invalid=0x43e4e0,
    piece=0x43e930,activate=0x43cf40,cursor=0x4266c0,refresh=0x41eda0,quality=26,
    globals=dict(lock=0x51b654,legal=0x51b638,preview=0x51b658,x=0x565218,y=0x56521c,inputFlags=0x51b65c,
      costFlags=0x51b6a8,legalGump=0x51b628,cursorGump=0x51b620,type=0x51b62c,argument=0x51b634,direction=0x51b650,
      recording=0x53720c,recorder=0x537218,battle=0x540a20,blocked=0x52e170,screenX=0x565228,screenY=0x56522c,width=0x565230,
      queryMessage=0x5129fc,receiveMessage=0x5129f8)),
}
SPEC['original1037'] = dict(SPEC['originalCD'])
# 입력 열과 관찰할 현재 전역/raw 필드다. 기대 결과는 입력 생성기에서 계산하지 않는다.
KEYS = ('lock','legal','x','y','inputFlags','costFlags','legalGump','cursorGump','recording','argument','direction',
    'flags','quality','allowed','first','second','cost','footX','screenX','screenY','width','battle','blocked',
    'queryChange','prepareChange','sourceChange','detachChange','receiveChange','localChange','addChange')
SNAPSHOT = ('lock','legal','preview','x','y','inputFlags','costFlags','legalGump','cursorGump','argument','direction')
FIXTURE = ROOT/'cpppj/tests/fixtures/cursordrop-x86.tsv'
REPORT = ROOT/'cpppj/recovery-cursordrop-evidence.json'


def drop_case(**changes):
    """합성 상태/타입/Form 입력만 만들고 관찰 출력은 원본 실행에서 얻는다."""
    case = dict(lock=0,legal=1,x=bits(20.5),y=bits(-0.5),inputFlags=0,costFlags=0,legalGump=0,cursorGump=1,
        recording=0,argument=0x12345678,direction=0x100,flags=8,quality=3,allowed=0x100,first=FIRST,second=SECOND,
        cost=bits(250.75),footX=3,screenX=100,screenY=200,width=17,battle=1,blocked=0,
        queryChange=0,prepareChange=0,sourceChange=0,detachChange=0,receiveChange=0,localChange=0,addChange=0)
    case.update(changes);return {key:value&0xffffffff for key,value in case.items()}


def inputs():
    """조기 반환·world/GUI·새 건설/이동·두 조회·캡처 시점·좌표/비용/화면 정수 경계를 교차한다."""
    cases = [drop_case()]
    # 잠금이 있으면 조회도 하지 않는다. 조회 효과는 legal/현재 깊이를 바꿀 수 있다.
    for lock,legal,query in itertools.product((0,1,0xffffffff),(0,1),(0,1,2)):
        cases.append(drop_case(lock=lock,legal=legal,queryChange=query))
    # 월드 분기: BYTE 비트 0x80/0x40만 사용하며 음수/절반 경계와 두 번째 준비 뒤 좌표를 확인한다.
    positions = ((20.5,-0.5),(20.499998,-0.500001),(20.500002,-0.499999),(-1.5,-1.500001),(0.0,-0.0),(255.99998,300.25))
    for flags,cost,record,position,prepare in itertools.product((0,0x80,0x8000),(0,0x40,0x4000),(0,1),positions,(0,1,2)):
        cases.append(drop_case(inputFlags=flags,costFlags=cost,recording=record,x=bits(position[0]),y=bits(position[1]),prepareChange=prepare))
    # GUI: 전체 반환 DWORD로 검사한 뒤 WORD만 쓰고, old flags/quality는 detach 전에 캡처한다.
    for flags,allowed,moving,quality,mutate in itertools.product((0,8,0x100,0x108,0xffff),(0,8,0x100,0x10000),
            (0,0x80),(0,3,0xff),(0,1)):
        cases.append(drop_case(legalGump=1,costFlags=0x40,flags=flags,allowed=allowed,inputFlags=moving,quality=quality,
            sourceChange=mutate,detachChange=1,recording=1))
    # 두 번째 연관 번호는 반환 helper에 DWORD 그대로 전달한다. 첫 번호만 이동 삭제에 쓰인다.
    for second,receive in itertools.product((0,0x12345678,0xffffffff),(0,1)):
        cases.append(drop_case(legalGump=1,second=second,receiveChange=receive))
    # 현재 배치/가산 경계 뒤 깊이·cost flag·단가·화면을 다시 읽으며 전투 갱신 조건을 확인한다.
    for local,add,battle,blocked in itertools.product((0,1,2),(0,1),(0,1),(0,1)):
        cases.append(drop_case(costFlags=0x40,localChange=local,addChange=add,battle=battle,blocked=blocked))
    # SP 표시의 signed /2와 DWORD 넘침, CRT 비용 절삭(음수·NaN·범위 밖)을 실제 명령으로 관찰한다.
    for cost,foot,width,screen in itertools.product((0,bits(-3.75),bits(0.75),bits(2147483520.0),bits(3e9),0x7fc01234),
            (3,-3,0x10000001),(17,-17),(-100,0x7fffffff)):
        cases.append(drop_case(costFlags=0x40,cost=cost,footX=foot,width=width,screenX=screen))
    cases.append(drop_case(cursorGump=0));return cases


class DropOracle(PlaceOracle):
    """새 함수 범위/허용 쓰기만 실행하며 미복원 효과의 인자와 순서를 기록한다."""
    def __init__(self,edition):
        """실제 PE와 합성 Form/Gump의 가상 표를 준비한다."""
        super().__init__(edition);self.d=SPEC[edition];self.gd=self.d['globals'];mu=self.mu;mu.mem_map(GUI,0x1000)
        self.exports=[ROOT/f'extracted/cursordrop/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra의 실제 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 포함 끝 주소를 반열린 범위로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        # Form 가상 함수와 GUI 가상 함수를 구분한다.
        for offset,target in ((0,SOURCE),(0x10,DESTROY),(0x48,DETACH),(0x50,ATTACH),(0x90,POP)):
            mu.mem_write(VTABLE+offset,struct.pack('<I',target))
        mu.mem_write(VTABLE+0x100+0x28,struct.pack('<I',DISPATCH));mu.mem_write(VTABLE+0x100+0x1c,struct.pack('<I',CLOSE))
        self.stubs={self.d[key]:(letter,key) for key,letter in (
            ('query','Q'),('prepare','P'),('highlight','H'),('sound','S'),('record','R'),('local','L'),('add','A'),
            ('show','V'),('battleRefresh','B'),('invalid','I'),('piece','T'),('activate','E'),('cursor','C'),('refresh','F'))}
        self.virtual={SOURCE:('U','source'),DESTROY:('D','destroy'),POP:('O','pop'),DETACH:('N','detach'),
            ATTACH:('J','attach'),DISPATCH:('G','dispatch'),CLOSE:('X','close')}
        self.stubs.update(self.virtual)

    def get(self,key):
        """실제 현재 전역 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(self.gd[key],4))[0]

    def put(self,key,value):
        """명시 경계가 공급하는 전역 변경이다. 원본 명령 쓰기는 별도로 감사한다."""
        self.mu.mem_write(self.gd[key],struct.pack('<I',value&0xffffffff))

    def event(self,letter,*values):
        """명시 경계 사건의 DWORD 값을 십진수로 기록한다."""
        self.events.append(letter+':'+':'.join(map(str,values)))

    def sid_this(self):
        """가상 호출 this의 슬롯 번호를 검사한다."""
        address=self.mu.reg_read(UC_X86_REG_ECX);sid=(address-self.slot(0))//self.stride
        if self.slot(sid)!=address or sid not in (PREVIEW,FIRST,SECOND):raise RuntimeError('커서 this 오류')
        return sid

    def stub(self,letter,key):
        """명시 경계의 ABI/인자를 관찰하고 지정된 입력 변경만 공급한다."""
        mu=self.mu;c=self.case;result=0;purge=0
        if key=='query':
            self.event(letter,0)
            if c['queryChange']==1:self.put('legal',self.get('legal')^1)
            if c['queryChange']==2:self.put('lock',7)
        elif key=='prepare':
            self.prepared+=1;self.event(letter,self.prepared,self.get('x'),self.get('y'),self.get('preview'))
            if c['prepareChange']==1 and self.prepared==1:self.put('x',bits(-0.500001));self.put('y',bits(0.499999))
            if c['prepareChange']==2 and self.prepared==2:
                self.put('x',bits(20.25));self.put('y',bits(21.75));self.put('preview',51)
        elif key in ('highlight','activate','cursor'):self.event(letter,*self.args(1))
        elif key=='sound':
            pointer,*args=self.args(6);name=bytes(mu.mem_read(pointer,14))
            if name!=b'droppiece.wav\0':raise RuntimeError('커서 내려놓기 소리 이름 오류')
            self.event(letter,'droppiece.wav',*args)
        elif key=='record':
            name,x,y,argument,direction=self.args(5)
            if name!=TYPES+TYPE*self.type_stride+4 or mu.reg_read(UC_X86_REG_ECX)!=27:raise RuntimeError('기록 이름/this 오류')
            self.event(letter,27,'cursorType',x,y,argument,direction);purge=20
        elif key=='local':
            self.event(letter,*self.args(5))
            if c['localChange']==1:self.put('lock',0)
            if c['localChange']==2:self.put('costFlags',self.get('costFlags')^0x40)
        elif key=='add':
            self.event(letter,*self.args(1))
            if c['addChange']:
                mu.mem_write(TYPES+TYPE*self.type_stride+0xc4,struct.pack('<I',bits(9.75)));self.put('screenX',-17);self.put('width',-3)
            if self.edition!='originals':
                # CD 가산은 ST0 값을 반환한다. 도구의 fldz 하나만 허용해 반환 ABI를 공급한다.
                self.ret();address=STOP+0x600;mu.mem_write(address,b'\xd9\xee');esp=mu.reg_read(UC_X86_REG_ESP)
                self.fldz_resume=mu.reg_read(UC_X86_REG_EIP);mu.reg_write(UC_X86_REG_EIP,address);return
        elif key=='show':self.event(letter,*self.args(4))
        elif key in ('battleRefresh','invalid'):self.event(letter,0)
        elif key in ('piece','refresh'):self.event(letter,*self.args(3))
        elif key=='source':
            sid=self.sid_this();self.sources+=1;result=c['first'] if self.sources==1 else c['second'];self.event(letter,sid,result)
            if c['sourceChange'] and self.sources==2:
                mu.mem_write(self.slot(PREVIEW)+20,struct.pack('<H',c['flags']^0x108));mu.mem_write(self.slot(PREVIEW)+self.d['quality'],bytes([c['quality']^3]))
        elif key in ('destroy','detach'):
            sid=self.sid_this();self.event(letter,sid,*self.args(1));purge=4
            if key=='detach' and c['detachChange']:
                mu.mem_write(self.slot(PREVIEW)+20,b'\xff\xff');mu.mem_write(self.slot(PREVIEW)+self.d['quality'],b'\xfe')
        elif key in ('pop','attach'):self.event(letter,self.sid_this(),*self.args(3 if key=='pop' else 2));purge=12 if key=='pop' else 8
        elif key=='dispatch':
            if mu.reg_read(UC_X86_REG_ECX)!=GUI:raise RuntimeError('GUI dispatch this 오류')
            args=self.args(3);self.event(letter,1,*args);purge=12
            if args[0]==901:result=c['allowed']
            elif args[0]==902:
                if c['receiveChange']:self.put('inputFlags',self.get('inputFlags')^0x80)
            else:raise RuntimeError('GUI 메시지 오류')
        elif key=='close':
            if mu.reg_read(UC_X86_REG_ECX)!=GUI+0x80:raise RuntimeError('GUI close this 오류')
            self.event(letter,2,*self.args(3));purge=12
        else:raise RuntimeError('알 수 없는 커서 경계')
        self.ret(result,purge)

    def on_instruction(self,mu,address,size,data):
        """명시 경계와 CD 반환 ABI용 fldz 이외에는 실제 원본 몸체만 실행한다."""
        if address==STOP+0x600:return
        if address==STOP+0x602:
            mu.reg_write(UC_X86_REG_EIP,self.fldz_resume);return
        stub=self.stubs.get(address)
        if stub is None:OwnerOracle.on_instruction(self,mu,address,size,data);return
        letter,key=stub;self.stub(letter,key);self.stub_calls[letter]+=1

    def drop(self,case,control):
        """같은 합성 상태에서 전체 함수의 정상 반환·최종 전역/raw와 효과를 관찰한다."""
        self.case=case;self.events=[];self.prepared=0;self.sources=0;self.instructions=0;mu=self.mu
        self.write_ranges=[];self.messages={}
        # 실제 루트가 읽을 전역과 합성 GUI의 유효 부모를 공급한다.
        for key in self.gd:self.put(key,case.get(key,0))
        self.put('preview',PREVIEW);self.put('legalGump',GUI if case['legalGump'] else 0)
        self.put('cursorGump',GUI+0x80 if case['cursorGump'] else 0);self.put('type',TYPES+TYPE*self.type_stride)
        self.put('recorder',27);self.put('queryMessage',901);self.put('receiveMessage',902)
        mu.mem_write(TYPES+TYPE*self.type_stride+4,b'cursorType\0');mu.mem_write(TYPES+TYPE*self.type_stride+0xc4,struct.pack('<I',case['cost']))
        mu.mem_write(TYPES+TYPE*self.type_stride+self.s['foot'],struct.pack('<I',case['footX']))
        # 미리보기와 두 연관 슬롯은 함수들이 같은 가상 표를 사용하는 실제 raw 크기로 공급한다.
        for sid in (PREVIEW,FIRST,SECOND):
            mu.mem_write(self.slot(sid),bytes(self.stride));mu.mem_write(self.slot(sid),struct.pack('<I',VTABLE))
        mu.mem_write(self.slot(PREVIEW)+20,struct.pack('<H',case['flags']));mu.mem_write(self.slot(PREVIEW)+self.d['quality'],bytes([case['quality']]))
        mu.mem_write(GUI,struct.pack('<I',VTABLE+0x100));mu.mem_write(GUI+36,struct.pack('<I',123));mu.mem_write(GUI+0x80,struct.pack('<I',VTABLE+0x100))
        self.write_ranges=[(self.gd[key],self.gd[key]+4) for key in ('lock','x','y','preview','cursorGump')]
        self.write_ranges.append((self.slot(PREVIEW)+20,self.slot(PREVIEW)+22))
        self.invoke(self.d['entry'],[],control)
        final=[self.get(key) for key in SNAPSHOT]
        final[7]=1 if final[7] else 0;final[8]=1 if final[8] else 0
        final.extend((struct.unpack('<I',mu.mem_read(TYPES+TYPE*self.type_stride+0xc4,4))[0],
            struct.unpack('<H',mu.mem_read(self.slot(PREVIEW)+20,2))[0],mu.mem_read(self.slot(PREVIEW)+self.d['quality'],1)[0]))
        return [';'.join(self.events) or '-',','.join(map(str,final))]


def generate(smoke=False):
    """세 실제 PE × 두 x87 제어값의 독립 결과와 SHA/실행 집계를 기록한다."""
    rows=[];editions={};paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/cursordrop-functions.json',
        *[ROOT/f'tools/decomp_{name}_oracle.py' for name in ('owner','priestgeometry','constructionplace','damageablepredestroy')]}
    # 판본마다 독립된 원본 메모리에서 실행한다.
    for edition in SPECS:
        oracle=DropOracle(edition);cases=inputs()
        if smoke:cases=cases[::47]
        # 두 실제 실행의 사건/최종 상태가 같아야 한다.
        for case in cases:
            results=[oracle.drop(case,control) for control in CONTROLS]
            if results[0]!=results[1]:raise RuntimeError(f'커서 x87 정밀도 차이: {edition} {case}')
            rows.append([edition,','.join(str(case[key]) for key in KEYS),*results[0]])
        editions[edition]=dict(cases=len(cases),returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: cursordrop {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 커서 내려놓기 전체 실제 PE 관찰. 입력: '+' '.join(KEYS)+'\n# 최종 전역: '+' '.join(SNAPSHOT)+' cost flags quality\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',decompile_date='2026-10-11',controls=list(CONTROLS),
        os_calls=0,editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},
        limits=['커서 전체/반올림·스냅/CRT floor·ftol/CD 잠금 조회·증가는 실제 원본 명령',
            'Form/Gump 가상 효과, 위치 판정/준비, 강조/소리/기록/로컬 배치/SP/화면 갱신은 명시 경계',
            'CD SP 가산 ST0 반환은 도구 fldz 1개로 ABI 공급; GUI 메시지 번호/부모·타입/Form은 합성 입력',
            '유한 좌표·유효 미리보기/이동 연관 SID·부모 123; 원본 assert/게임/OS/실제 GUI 미실행']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'cursordrop 전체 {len(rows)}개')


def verify():
    """저장 입력/관찰·SHA·전체 정상 진입/반환·명시 경계 사건 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 몸체 내보내기/실행기/원본/fixture 모두 변경을 검출한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('커서 근거 오류')
    # 판본별 입력 순서와 실제 루트/경계 집계를 독립 저장값과 대조한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];cases=inputs();spec=SPEC[edition]
        if len(selected)!=len(cases) or item['cases']!=len(cases) or item['assertions']:raise RuntimeError('커서 행/assert 오류')
        # 결과를 재계산하지 않고 입력 열과 관찰 상태의 폭을 검증한다.
        for row,case in zip(selected,cases,strict=True):
            if len(row)!=4 or row[1]!=','.join(str(case[key]) for key in KEYS) or len(row[3].split(','))!=14:raise RuntimeError('커서 입력/열 오류')
        if item['returns'].get(f'{spec["entry"]:08x}',0)!=2*len(selected) or item['native_calls'].get(f'{spec["entry"]:08x}',0)!=2*len(selected):raise RuntimeError('커서 정상 반환 수 오류')
        events=[event for row in selected for event in row[2].split(';') if event!='-']
        # 명시 경계마다 실제 관찰 사건과 호출 수가 일대일이다.
        for letter in 'QPHSLRAVBITECDONJGXU':
            if item['substitutions'].get(letter,0)!=2*sum(event.startswith(letter+':') for event in events):raise RuntimeError('커서 경계 수 오류: '+letter)
        if any(item['native_calls'].get(f'{spec[key]:08x}',0) for key in ('query','prepare','sound','local','add','show','piece')):raise RuntimeError('커서 몸체/경계 혼합')
        if not item['native_calls'].get(f'{spec["round"]:08x}',0):raise RuntimeError('실제 반올림 미실행')
    print(f'cursordrop 검증 통과: {len(rows)}개')


def main():
    """전체 생성/무저장 표본/저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
