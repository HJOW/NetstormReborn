#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건설 배치 실행(Construction.cpp 00442c80 / CD 004d0ca0)과 비용 부족 처리·환불/취소 통지·SP 조회를 세 실제 PE에서 대조한다.

실제 명령으로 실행하는 것: 배치 전체 몸체(비용 판정·조각 순회·abstract 비트·HP·프레임·단어 쓰기·다리 품질·Pop 플래그),
CanonDecoder 생성/진행과 프레임 검색, HP 유무/최대 HP/HP 쓰기, 단어 쓰기, 패치판의 비용 부족 처리(00442b50)·
환불(00442a40)·취소 통지(00442af0)·SP 조회(0040ef30)·SP 가산 분배(0041dff0)·좌표 유효성(0040e800)·Player 표시 비트.
대체하는 것(명시 경계): SID 수신(Take), SP 저장소 읽기/가산, 표면 알림, 가상 소유자 지정/Pop, 문구 조립/안내, 메시지 초기화/전송.
원본 assert 보고에 닿는 입력(서버 번호 9·void가 아닌 슬롯·개수 불일치)은 만들지 않는다. 원본 게임/OS는 실행하지 않는다.

python -X utf8 tools/decomp_constructionplace_oracle.py            # 기대값·근거 기록 생성
python -X utf8 tools/decomp_constructionplace_oracle.py --smoke    # 저장 없이 표본만 실행
python -X utf8 tools/decomp_constructionplace_oracle.py --verify   # 저장된 기록·입력 파일의 SHA/행 수/사건 수 확인
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,PLAYERS,POOL,STACK,STOP,digest
from decomp_priestgeometry_oracle import GeometryOracle,GEOMETRY,DEC,OUT,CODES
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPSW)

# 판본별 진입점·명시 경계·전역 주소와 객체 필드 배치다. CD/10.37에는 비용 처리와 취소 통지가 없다.
SPEC={
 'originals':dict(place=0x442c80,charge=0x442b50,reject=0x442af0,refund=0x442a40,money=0x40ef30,add=0x41dff0,
    take=0x4af610,notify=0x4214a0,spget=0x40ec50,addlocal=0x40eec0,addother=0x41df80,format=0x4a89c0,
    others=0x4c9200,player=0x4c9150,tell=0x4cf960,init_money=0x40af90,init_reject=0x40a620,send=0x4e2dc0,
    id_money=0x52ee6a,id_reject=0x52edca,local=0x540c70,blocked=0x594fa4,server=0x540bc0,authority=0x540bc4,
    shown=0x595344,life=0x52f960,nugget=0x5412c0,altar=0x5412e8,quarter=0x5ca8f0,
    frame=0x24,frame_width=4,hp_width=4,server_first=15000),
 'originalCD':dict(place=0x4d0ca0,refund=0x4d1bc0,take=0x4ab440,notify=0x448c10,init_money=0x40b180,send=0x4c0380,
    id_money=0x510148,server=0x540a28,life=0x51f05c,altar=0x51cbd4,quarter=0x511c90,
    frame=0x22,frame_width=1,hp_width=2,server_first=6000),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 가상 표 기록값과 두 가상 경계 주소, SID 배열 입력 주소다. 호스트/원본 주소가 아니다.
VTABLE,OWNER,POP,SIDS=0x11010000,STOP+0x100,STOP+0x200,OUT+0x100
# x87 반환값을 단정도로 꺼내는 도구 명령(fstp dword [OUT]) 한 개를 두는 주소다. 원본 코드가 아니다.
HARVEST=STOP+0x300
# CanonDecoder가 패턴으로 분기하는 여덟 타입 번호(원본 전역의 실제 초기값)와 이 도구가 쓰는 타입 번호다.
PATTERN_TYPES=(107,82,94,157,131,129,140,142)
PLAIN,DAIS,BRIDGE,ISLAND,NOISLAND,NUGGET,ALTAR=100,101,82,94,157,154,164
# 타입 플래그: flags1 0x10은 HP가 있는 타입, flags2 0x400000은 HP 유무가 abstract 비트인 dais 부류다.
HAS_HP,DAIS_KIND=0x10,0x400000
# 문구 조립 경계가 돌려주는 고정 토큰과 메시지 본문 길이(식별 번호 → 머리 4바이트를 뺀 길이)다.
TOKEN=b'cheat-token\0'
PAYLOAD={'money':7,'reject':11}
FIXTURE=ROOT/'cpppj/tests/fixtures/constructionplace-x86.tsv'
REPORT=ROOT/'cpppj/recovery-constructionplace-evidence.json'
# 연산별 입력 칸 순서다. count는 실제 decoder가 센 조각 수이며 배치의 SID 개수 인자로 공급한다.
PLACE_KEYS=('type','argument','direction','x','y','player','local','blocked','cost','money','server','authority','shown',
    'flagged','abstract','quality','sids','slots','frames','flags1','flags2','maxHp','altarHp','default','life','count')
CHARGE_KEYS=('type','player','local','authority','flagged','shown','cost','x','y','a4','a5')
REFUND_KEYS=('type','player','local','shown','cost')
REJECT_KEYS=('type','player','local','shown','cost','x','y','a5','a6')
MONEY_KEYS=('player','local','shown','money')
ADD_KEYS=('player','local','delta')
KEYS=dict(Place=PLACE_KEYS,Charge=CHARGE_KEYS,Refund=REFUND_KEYS,Reject=REJECT_KEYS,Money=MONEY_KEYS,Add=ADD_KEYS)
# 단정도 NaN·무한대 입력 비트다.
NAN,INF=0x7fc01234,0x7f800000


def frame_codes(profile):
    """타입의 프레임 코드 표 입력을 만든다. 0=기본, 1=역순, 2=부호 있는 플래그(0x80), 3=품질 프레임 없음."""
    codes=[];base=0x80 if profile==2 else 0
    # 방향 글자 A~P마다 번호 0~10과 품질 플래그(0x20, 0x40) 프레임을 둔다. 변형 글자는 P다.
    for side in range(65,81):
        for number in range(11):codes.append(bytes((side,80,number,base)))
        if profile!=3:codes.extend((bytes((side,80,11,base|0x20)),bytes((side,80,12,base|0x40))))
    if profile==1:codes.reverse()
    codes.append(codes[0])
    return b''.join(codes)


def slot_bytes(stride,profile,typ):
    """배치 전 void 슬롯의 raw 입력이다. 0=0으로 채움, 1=0xff로 채움, 2=위치마다 다른 값. 상태 바이트는 void를 켜고 free를 끈다."""
    raw=bytearray(stride) if profile==0 else bytearray(b'\xff'*stride) if profile==1 else bytearray((i*37+11)&0xff for i in range(stride))
    struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;raw[11]=(raw[11]|4)&0xfe
    return bytes(raw)


def sid_of(edition,profile,index):
    """조각 순서의 SID 입력이다. 0=클라이언트 영역(직접 사용), 1=서버 영역(서버가 아니면 수신), 2=번갈아 섞음."""
    first=SPEC[edition]['server_first']
    return 50+index if profile==0 or (profile==2 and index%2==0) else first+index


def player_flags(player,flagged):
    """Player 표시 필드(+0x54)의 입력이다. 홀수 번호는 다른 비트를 모두 켜서 0x400 비트만 바뀌는지 본다."""
    return (0xfffffbff if player%2 else 0)|(0x400 if flagged else 0)


def place_case(**changes):
    """배치 입력의 기본값에 바꿀 칸만 덮어쓴다. 기본은 비용 판정을 건너뛰는(로컬 플레이어) 한 칸 타입이다."""
    case=dict(type=PLAIN,argument=7,direction=0,x=bits(20.5),y=bits(21.25),player=3,local=3,blocked=0,cost=bits(250.0),
        money=bits(1000.0),server=1,authority=1,shown=0,flagged=0,abstract=0,quality=0,sids=0,slots=0,frames=0,
        flags1=HAS_HP,flags2=0,maxHp=120,altarHp=777,default=3,life=5,count=0)
    case.update(changes)
    return case


def place_inputs(edition):
    """비용 판정·조각별 처리·다리 품질 분기를 교차한 배치 입력이다. 기대 결과는 원본 기계어가 정한다."""
    patch=edition=='originals';cases=[]
    directions=(0,1,2,3,4,5,6,7) if patch else (0,2,4,6)
    if patch:
        # 비용 판정을 건너뛰는 조건: 차단 전역, 범위 밖 플레이어, 비용 0/-0, nugget 타입.
        for changes in (dict(blocked=1),dict(player=0),dict(player=10),dict(player=0xffffffff),dict(cost=0),dict(cost=0x80000000),
                        dict(type=NUGGET),dict(local=3)):
            cases.append(place_case(**dict(dict(local=5,money=bits(10.0)),**changes)))
        # 잔액이 충분하면 그 플레이어의 SP를 차감한다(같음·넉넉함·무한대·NaN 비용).
        for player,money,cost in itertools.product((1,8),(bits(250.0),bits(1000.0),INF),(bits(250.0),bits(0.5),NAN)):
            cases.append(place_case(player=player,local=4,money=money,cost=cost))
        # 잔액이 모자라면 권한·기존 표시·좌표 유효성·전투 표시에 따라 안내/환불/취소 통지가 갈린다.
        positions=((20.5,21.25),(0.0,21.25),(256.0,21.25),(20.5,-1.0),(20.5,300.0),(255.99998,0.000001))
        for money,authority,flagged,position,shown in itertools.product((bits(249.99),NAN,bits(-1.0),0),(0,1),(0,1),positions,(0,1)):
            cases.append(place_case(player=6,local=2,money=money,authority=authority,flagged=flagged,shown=shown,
                x=bits(position[0]),y=bits(position[1]),argument=0x1234,direction=6 if position[0]==20.5 else 0))
    # 조각별 처리: 서버 플래그 × SID 범위 × abstract 인자 × 슬롯 초기 내용 × 타입 부류.
    kinds=((PLAIN,HAS_HP,0),(PLAIN,0,0),(DAIS,0,DAIS_KIND),(DAIS,HAS_HP,DAIS_KIND),(NOISLAND,HAS_HP,0),(ISLAND,0,0),(NUGGET,HAS_HP,0))
    for (typ,flags1,flags2),server,sids,abstract,slots in itertools.product(kinds,(0,1),(0,1,2),(0,1,2,3),(0,1,2)):
        argument=(sids+abstract)%2 if typ==ISLAND else 0 if typ==NOISLAND else 0x4321+slots
        cases.append(place_case(type=typ,flags1=flags1,flags2=flags2,server=server,sids=sids,abstract=abstract,slots=slots,
            argument=argument,direction=directions[(sids+slots)%len(directions)] if typ in (ISLAND,NOISLAND) else 0,player=1+(abstract+slots)%8))
    # 좌표·기본 프레임·방향 입력: 소수 좌표와 지도 가장자리, 없는 기본 프레임(-1).
    for (x,y),default,direction in itertools.product(((0.0,0.0),(255.99998,254.5),(20.000002,21.999998),(-1.25,-0.75)),(0,3,-1),directions[:2]):
        cases.append(place_case(x=bits(x),y=bits(y),default=default&0xffffffff,direction=direction,type=PLAIN))
        cases.append(place_case(x=bits(x),y=bits(y),type=NOISLAND,argument=0,direction=direction))
    # 다리: 모든 패턴 × 방향에 품질·프레임 표·abstract를 돌려 가며 넣는다.
    # 품질 프레임이 없는 표(3)에서 품질 1·3을 주면 원본 프레임 검색이 assert를 보고하므로 0·2로 낮춘다.
    index=0
    for pattern,direction in itertools.product(range(26),directions):
        cases.append(place_case(type=BRIDGE,argument=pattern,direction=direction,quality=index%4&2 if (index//4)%4==3 else index%4,frames=(index//4)%4,
            abstract=(index//16)%2,flags1=HAS_HP if index%3 else 0,slots=index%3,sids=index%3,server=(index//3)%2,player=1+index%8,local=1+index%8))
        index+=1
    # 다리 품질의 촘촘한 격자: 품질 인자 × 프레임 표 × abstract × 수명 상수.
    for pattern,quality,frames,abstract,life in itertools.product((0,7,25),(0,1,2,3),(0,1,2,3),(0,1),(5,1,16)):
        if frames==3 and quality in (1,3):continue
        cases.append(place_case(type=BRIDGE,argument=pattern,direction=directions[pattern%len(directions)],quality=quality,
            frames=frames,abstract=abstract,life=life,slots=1+(quality+frames)%2))
    # 수명 상수가 17 이상이면 수명 값이 단어의 bit 7 위로 넘친다. 원본은 bit 3~6만 바꾸고 나머지 비트는 그대로 둔다.
    for pattern,life,abstract in itertools.product((0,7),(17,33),(0,1)):
        cases.append(place_case(type=BRIDGE,argument=pattern,direction=0,quality=1,life=life,abstract=abstract,slots=abstract))
    if patch:
        # 비용 판정과 여러 조각 배치가 함께 일어나는 경우(다리·섬).
        for typ,money,authority in itertools.product((BRIDGE,ISLAND),(bits(1000.0),bits(1.0)),(0,1)):
            cases.append(place_case(type=typ,argument=1,player=7,local=1,money=money,authority=authority,quality=1))
    return cases


def direct_inputs(edition):
    """배치를 거치지 않는 직접 호출 입력이다: 비용 부족 처리·환불·취소 통지·SP 조회·SP 가산."""
    patch=edition=='originals';cases=[]
    costs=(0,bits(0.4),bits(1.0),bits(250.7),bits(-3.0),NAN,bits(3e9),bits(2147483520.0),INF)
    # 환불: nugget 여부 × 로컬 여부 × 전투 표시 × 비용(절삭·음수·NaN·정수 범위 밖).
    for typ,player,local,shown,cost in itertools.product((PLAIN,NUGGET),(2,8),(2,5),(0,1),costs):
        if patch or (local==2 and shown==0):cases.append(('Refund',dict(type=typ,player=player,local=local,shown=shown,cost=cost)))
    if not patch:return cases
    positions=((20.5,21.25),(0.0,21.25),(256.0,21.25),(20.5,0.0),(20.5,256.0),(float('nan'),21.25),(20.5,float('nan')),(255.99998,0.000001))
    # 비용 부족 처리: 로컬 여부 × 권한 × 기존 표시 × 전투 표시 × 좌표.
    for player,local,authority,flagged,shown,position in itertools.product((2,8),(2,5),(0,1),(0,1),(0,1),positions):
        cases.append(('Charge',dict(type=PLAIN,player=player,local=local,authority=authority,flagged=flagged,shown=shown,
            cost=bits(250.7),x=bits(position[0]),y=bits(position[1]),a4=0x1234,a5=0x5678)))
    # 비용 값(절삭·음수·NaN·범위 밖)과 로컬 플레이어/nugget 타입의 비권한·권한 처리.
    for cost,authority in itertools.product(costs,(0,1)):
        cases.append(('Charge',dict(type=PLAIN,player=4,local=4,authority=authority,flagged=0,shown=0,cost=cost,x=bits(20.5),y=bits(21.25),a4=1,a5=2)))
        cases.append(('Charge',dict(type=NUGGET,player=4,local=1,authority=authority,flagged=1,shown=1,cost=cost,x=bits(20.5),y=bits(21.25),a4=3,a5=4)))
    # 취소 통지: 환불 뒤 좌표·타입·두 바이트 인자를 그대로 싣는다.
    for typ,player,local,shown,cost in itertools.product((PLAIN,NUGGET),(3,),(3,6),(0,1),(bits(250.7),0,NAN)):
        cases.append(('Reject',dict(type=typ,player=player,local=local,shown=shown,cost=cost,x=bits(-2.5),y=NAN,a5=0x1ff,a6=0x2fe)))
    # SP 조회: 전투 표시 × 로컬 여부 × 범위 안팎의 플레이어 번호.
    for player,local,shown,money in itertools.product((0,1,8,9,300,0xffffffff),(1,8),(0,1),(bits(123.5),NAN)):
        cases.append(('Money',dict(player=player,local=local,shown=shown,money=money)))
    # SP 가산 분배: 로컬이면 로컬 가산, 아니면 플레이어 가산.
    for player,local,delta in itertools.product((0,1,8,9),(1,8),(bits(-250.0),bits(12.5),NAN)):
        cases.append(('Add',dict(player=player,local=local,delta=delta)))
    return cases


class PlaceOracle(GeometryOracle):
    """배치·비용 처리 몸체와 실제 helper를 실행하고 명시 경계의 인자와 순서를 기록한다."""
    def __init__(self,edition):
        """새 내보내기 범위, 가상 표, 서버 영역 SID까지 넓힌 풀, 경계 주소 표를 준비한다."""
        super().__init__(edition);self.p=p=SPEC[edition];mu=self.mu;self.patch=edition=='originals'
        self.exports=[ROOT/f'extracted/constructionplace/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 내보낸 함수의 불연속 몸체만 실행을 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 범위 끝 주소는 포함이므로 반열린 구간으로 바꿔 둔다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        mu.mem_map(VTABLE,0x1000);mu.mem_write(VTABLE+0x74,struct.pack('<I',OWNER));mu.mem_write(VTABLE+0x90,struct.pack('<I',POP))
        mu.mem_write(HARVEST,b'\xd9\x1d'+struct.pack('<I',OUT))
        low=(128*self.stride+4095)&~4095;high=((p['server_first']+32)*self.stride+4095)&~4095
        mu.mem_map(POOL+low,high-low)
        # 경계 주소 → (사건 글자, 처리기). 판본에 없는 경계는 넣지 않는다.
        self.stubs={OWNER:('O',self.stub_owner),POP:('P',self.stub_pop),p['take']:('T',self.stub_take),p['notify']:('N',self.stub_notify),
            p['init_money']:('I',self.stub_init),p['send']:('M',self.stub_send)}
        if self.patch:
            self.stubs.update({p['spget']:('G',self.stub_spget),p['addlocal']:('L',self.stub_addlocal),p['addother']:('A',self.stub_addother),
                p['format']:('F',self.stub_format),p['others']:('X',self.stub_others),p['player']:('Y',self.stub_player),
                p['tell']:('D',self.stub_tell),p['init_reject']:('I',self.stub_init)})
        self.stub_calls=collections.Counter();self.returns=collections.Counter();self.harvest=False;self.messages={}

    def args(self,count):
        """경계 진입 시점의 스택 인자 DWORD들이다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);return struct.unpack(f'<{count}I',self.mu.mem_read(sp+4,4*count))

    def ret(self,result=0,purge=0):
        """경계의 반환값과 ABI 스택 정리만 수행한다."""
        mu=self.mu;sp=mu.reg_read(UC_X86_REG_ESP);mu.reg_write(UC_X86_REG_EAX,result&0xffffffff)
        mu.reg_write(UC_X86_REG_EIP,struct.unpack('<I',mu.mem_read(sp,4))[0]);mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def text(self,address):
        """원본이 넘긴 ASCII 문자열을 읽는다."""
        return bytes(self.mu.mem_read(address,96)).split(b'\0',1)[0].decode('ascii')

    def sid(self,pointer):
        """슬롯 주소를 SID로 바꾼다. 풀 밖이나 어긋난 주소는 원본 인자 오류다."""
        index,rest=divmod(pointer-POOL,self.stride)
        if rest or not 0<=index<self.p['server_first']+32:raise RuntimeError(f'슬롯 주소 오류: {pointer:08x}')
        return index

    def stub_owner(self):
        """가상 +0x74: 소유자 바이트만 쓰는 경계다."""
        this=self.mu.reg_read(UC_X86_REG_ECX);player,=self.args(1);sid=self.sid(this)
        self.mu.mem_write(this+self.o['owner'],bytes((player&0xff,)));self.events.append(f'O:{sid}:{player}');self.ret(purge=4)

    def stub_pop(self):
        """가상 +0x90: 좌표 비트와 플래그를 기록하는 경계다."""
        x,y,flags=self.args(3);self.events.append(f'P:{self.sid(self.mu.reg_read(UC_X86_REG_ECX))}:{x}:{y}:{flags}');self.ret(purge=12)

    def stub_take(self):
        """SID 수신: 요청한 번호의 슬롯 주소를 돌려준다. 슬롯 내용은 입력이 미리 준비한다."""
        typ,sid=self.args(2);self.events.append(f'T:{typ}:{sid}');self.ret(self.slot(sid))

    def stub_notify(self):
        """표면 알림: 호출 사실과 번호만 기록한다."""
        sid,=self.args(1);self.events.append(f'N:{sid}');self.ret()

    def stub_spget(self):
        """SP 저장소 읽기: 입력 잔액 비트를 출력 포인터에 쓴다."""
        owner,out=self.args(2);self.mu.mem_write(out,struct.pack('<I',self.case['money']));self.events.append(f'G:{owner}');self.ret()

    def stub_addlocal(self):
        """로컬 SP 가산 요청의 가산액 비트를 기록한다."""
        delta,=self.args(1);self.events.append(f'L:{delta}');self.ret()

    def stub_addother(self):
        """플레이어 SP 가산 요청의 번호와 가산액 비트를 기록한다."""
        owner,delta=self.args(2);self.events.append(f'A:{owner}:{delta}');self.ret()

    def stub_format(self):
        """문구 조립: 서식 key와 이름 인자를 기록하고 고정 토큰을 버퍼에 쓴다."""
        buffer,key,name=self.args(3);self.mu.mem_write(buffer,TOKEN)
        self.events.append(f'F:{self.text(key)}:{self.text(name)}');self.ret(buffer)

    def stub_others(self):
        """그 플레이어를 뺀 모두에게 보내는 안내다."""
        player,text=self.args(2);self.events.append(f'X:{player}:{self.text(text)}');self.ret()

    def stub_player(self):
        """그 플레이어에게 보내는 안내다."""
        player,text=self.args(2);self.events.append(f'Y:{player}:{self.text(text)}');self.ret()

    def stub_tell(self):
        """로컬 안내 창 요청이다."""
        text,=self.args(1);self.events.append(f'D:{self.text(text)}');self.ret()

    def stub_init(self):
        """메시지 머리 초기화: 버퍼가 어떤 메시지인지만 기억한다. 사건으로는 남기지 않는다."""
        address=self.mu.reg_read(UC_X86_REG_EIP);kind='reject' if self.patch and address==self.p['init_reject'] else 'money'
        self.messages[self.mu.reg_read(UC_X86_REG_ECX)]=kind;self.ret()

    def stub_send(self):
        """메시지 전송: 대상, 메시지 종류, 머리 뒤 본문 바이트를 기록한다."""
        target,message=self.args(2);kind=self.messages.pop(message)
        payload=bytes(self.mu.mem_read(message+4,PAYLOAD[kind])).hex();self.events.append(f'M:{target}:{kind}:{payload}');self.ret()

    def on_instruction(self,mu,address,size,data):
        """경계 주소는 대체 처리하고 그 밖에는 내보낸 몸체 안의 명령만 허용한다."""
        if self.harvest:return
        stub=self.stubs.get(address)
        if stub is None:OwnerOracle.on_instruction(self,mu,address,size,data);return
        letter,handler=stub;handler()
        if letter!='I':self.stub_calls[letter]+=1

    def setup(self,case):
        """전역·타입 표·프레임 코드·Player 이름/표시 비트 입력을 쓴다."""
        mu=self.mu;p=self.p;self.case=case;self.events=[];self.messages={}
        # CanonDecoder가 비교하는 여덟 패턴 타입 전역을 실제 초기값으로 채운다.
        for address,value in zip(self.s['patterns'],PATTERN_TYPES,strict=True):mu.mem_write(address,struct.pack('<I',value))
        mu.mem_write(p['altar'],struct.pack('<I',ALTAR));mu.mem_write(p['quarter'],bytes(4))
        mu.mem_write(p['server'],struct.pack('<I',case.get('server',1)));mu.mem_write(p['life'],struct.pack('<I',case.get('life',5)))
        if self.patch:
            mu.mem_write(p['nugget'],struct.pack('<I',NUGGET))
            # 비용 판정과 비용 부족 처리가 읽는 네 전역이다. 입력에 없으면 0이다.
            for name in ('local','blocked','authority','shown'):mu.mem_write(p[name],struct.pack('<I',case.get(name,0)))
            # Player 구조체의 이름(+0xc)과 표시 비트(+0x54)를 플레이어마다 준비한다.
            for player in range(9):
                base=PLAYERS+player*self.player_stride;mu.mem_write(base,bytes(self.player_stride))
                mu.mem_write(base+0xc,f'P{player}'.encode('ascii')+b'\0')
                mu.mem_write(base+0x54,struct.pack('<I',player_flags(player,case.get('flagged',0))))
        typ=case.get('type',PLAIN);record=TYPES+typ*self.type_stride;codes=frame_codes(case.get('frames',0))
        mu.mem_write(record,bytes(self.type_stride));mu.mem_write(record,struct.pack('<i',case.get('maxHp',0)))
        mu.mem_write(record+0xc4,struct.pack('<I',case.get('cost',0)));mu.mem_write(record+0xe8,struct.pack('<2I',case.get('flags1',0),case.get('flags2',0)))
        mu.mem_write(record+0x114,struct.pack('<Ii',len(codes)//4,struct.unpack('<i',struct.pack('<I',case.get('default',0)))[0]))
        mu.mem_write(record+0x124,struct.pack('<I',CODES));mu.mem_write(record+self.s['foot'],struct.pack('<2i',1,1))
        mu.mem_write(CODES,bytes(0x1000));mu.mem_write(CODES,codes)
        altar=TYPES+ALTAR*self.type_stride;mu.mem_write(altar,bytes(self.type_stride));mu.mem_write(altar,struct.pack('<i',case.get('altarHp',0)))

    def flags(self,player):
        """Player 표시 비트 필드의 주소다. 범위 밖 번호는 없다."""
        return PLAYERS+player*self.player_stride+0x54

    def invoke(self,entry,arguments,control,value=False):
        """cdecl 진입점을 정상 반환까지 실행하고 스택·보존 레지스터·x87 균형을 검사한다. value이면 ST0 결과 비트를 돌려준다."""
        mu=self.mu;self.prepare_registers(control)
        mu.mem_write(STACK,struct.pack(f'<{1+len(arguments)}I',STOP,*[a&0xffffffff for a in arguments]))
        mu.emu_start(entry,STOP,count=400000)
        result=None
        if value:
            # 반환된 x87 값을 단정도로 꺼내는 한 명령(fstp dword [OUT])만 도구 주소에서 실행한다.
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or (mu.reg_read(UC_X86_REG_FPSW)>>11)&7!=7:raise RuntimeError('SP 조회 반환/x87 깊이 오류')
            self.harvest=True
            try:mu.emu_start(HARVEST,HARVEST+6,count=1)
            finally:self.harvest=False
            if mu.reg_read(UC_X86_REG_EIP)!=HARVEST+6:raise RuntimeError('SP 조회 결과 추출 명령 미실행')
            mu.reg_write(UC_X86_REG_EIP,STOP)
            result=struct.unpack('<I',mu.mem_read(OUT,4))[0]
        self.check(control,STACK+4);self.returns[f'{entry:08x}']+=1
        if self.messages:raise RuntimeError('보내지 않은 메시지 버퍼가 남음')
        return result

    def measure(self,case):
        """실제 decoder 생성/진행으로 조각 수를 센다. 배치의 SID 개수 입력을 정하는 데만 쓰며 호출 집계에서 뺀다."""
        saved=collections.Counter(self.native_calls);self.setup(case);self.mu.mem_write(DEC,bytes(84));self.write_ranges=[]
        self.call(self.s['begin'],[case[k] for k in ('type','argument','direction','x','y')]+[0],CONTROLS[0]);count=0
        # 유효 칸이 남아 있는 동안 진행한다. 패턴은 최대 15칸이다.
        while struct.unpack('<I',self.mu.mem_read(DEC+20,4))[0]:
            count+=1;self.call(self.s['next'],[],CONTROLS[0])
            if count>15:raise RuntimeError('패턴 칸 수 초과')
        self.native_calls=saved
        return count

    def place(self,case,control):
        """배치 전체를 실행하고 (사건, 조각 슬롯들, Player 표시 비트)를 돌려준다."""
        mu=self.mu;self.setup(case);sids=[sid_of(self.edition,case['sids'],i) for i in range(case['count'])]
        # 조각마다 void 슬롯 입력을 준비한다.
        for sid in sids:mu.mem_write(self.slot(sid),slot_bytes(self.stride,case['slots'],case['type']))
        mu.mem_write(SIDS,struct.pack(f'<{len(sids)+1}H',*sids,0xeeee))
        self.write_ranges=[(self.slot(sid),self.slot(sid)+self.stride) for sid in sids]
        player=case['player'];tracked=self.patch and 0<player<9
        if tracked:self.write_ranges.append((self.flags(player),self.flags(player)+4))
        self.invoke(self.p['place'],[case[k] for k in ('type','x','y','argument','direction','player','count')]+[SIDS,case['abstract'],case['quality']],control)
        slots=','.join(bytes(mu.mem_read(self.slot(sid),self.stride)).hex() for sid in sids) or '-'
        return [';'.join(self.events) or '-',slots,struct.unpack('<I',mu.mem_read(self.flags(player),4))[0] if tracked else '-']

    def direct(self,op,case,control):
        """직접 호출 하나를 실행하고 (사건, '-', 부가 결과)를 돌려준다."""
        mu=self.mu;p=self.p;self.setup(case);self.write_ranges=[];extra='-'
        if op=='Charge':
            self.write_ranges=[(self.flags(case['player']),self.flags(case['player'])+4)]
            self.invoke(p['charge'],[case[k] for k in ('type','player','x','y','a4','a5')],control)
            extra=struct.unpack('<I',mu.mem_read(self.flags(case['player']),4))[0]
        elif op=='Refund':self.invoke(p['refund'],[case['type'],case['player']],control)
        elif op=='Reject':self.invoke(p['reject'],[case[k] for k in ('type','player','x','y','a5','a6')],control)
        elif op=='Money':extra=self.invoke(p['money'],[case['player']],control,True)
        else:self.invoke(p['add'],[case['player'],case['delta']],control)
        return [';'.join(self.events) or '-','-',extra]


def generate(smoke=False):
    """세 실제 PE를 두 x87 정밀도로 실행해 같은 관찰만 fixture와 SHA 근거로 저장한다."""
    rows=[];editions={}
    paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestgeometry_oracle.py',
        ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/constructionplace-functions.json',FIXTURE}
    # 판본마다 배치 입력과 직접 호출 입력을 독립 실행한다.
    for edition in SPECS:
        oracle=PlaceOracle(edition);counts=collections.Counter();pieces=0
        places=place_inputs(edition);directs=direct_inputs(edition)
        if smoke:places=places[::17];directs=directs[::11]
        # 배치: 실제 decoder로 조각 수를 센 뒤 두 x87 정밀도로 전체 몸체를 실행한다.
        for case in places:
            case['count']=oracle.measure(case);observed=[oracle.place(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'배치 x87 정밀도 차이: {edition} {case}')
            rows.append(['Place',edition,','.join(str(case[k]) for k in PLACE_KEYS),*observed[0]]);counts['Place']+=1;pieces+=case['count']
        # 직접 호출: 비용 부족 처리·환불·취소 통지·SP 조회·SP 가산 분배를 따로 진입한다.
        for op,case in directs:
            observed=[oracle.direct(op,case,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'직접 호출 x87 정밀도 차이: {edition} {op} {case}')
            rows.append([op,edition,','.join(str(case[k]) for k in KEYS[op]),*observed[0]]);counts[op]+=1
        editions[edition]=dict(cases=dict(counts),pieces=pieces,returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: constructionplace {sum(counts.values())}개 통과',flush=True)
    if smoke:return
    header=['# 실제 건설 배치 실행·비용 부족 처리·환불/취소 통지·SP 조회. SID 수신·SP 저장소·표면 알림·가상 소유자/Pop·문구/메시지는 명시 경계.',
        '# 연산 판본 입력(쉼표) 사건 조각슬롯(쉼표) 부가결과. 입력 칸: '+' | '.join(f'{op}='+' '.join(keys) for op,keys in KEYS.items())]
    FIXTURE.write_text('\n'.join(header)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-10',decompile_date='2026-10-10',controls=list(CONTROLS),
        os_calls=0,editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['배치 전체 몸체/비용 판정/조각 순회/abstract·HP·프레임·단어·다리 품질/Pop 플래그와 CanonDecoder·프레임 검색·HP helper 정상 반환/ABI/x87',
            '패치판 비용 부족 처리·환불·취소 통지·SP 조회/가산 분배·좌표 유효성·Player 표시 비트 실제 실행; CD/10.37은 환불 통지만 있음',
            'SID 수신(Take)·SP 저장소 읽기/가산·표면 알림·가상 소유자/Pop·문구 조립/안내·메시지 초기화/전송은 명시 경계',
            '합성 타입/프레임 표/슬롯 입력, 유한 좌표; 서버 번호 9·void 아님·개수 불일치 assert와 번호 5 미만 SID는 입력에서 제외',
            '로컬 배치 004433b0·서버 확정 00444590/00444760·사제 이동 00443000·메시지 수신 처리·실제 Pop/GUI 미실행']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(total=len(rows)),ensure_ascii=False))


def verify():
    """SHA·입력 순서·정상 반환 수·실제 진입 수·경계 사건 수를 감사한다. 기대 결과를 다시 계산하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 저장된 원본 출력과 도구/내보내기의 변경을 검출한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('건설 배치 근거 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[1]==edition];p=SPEC[edition];places=place_inputs(edition);directs=direct_inputs(edition)
        expected=[('Place',case) for case in places]+directs
        if len(selected)!=len(expected) or item['assertions']:raise RuntimeError('건설 배치 행/assert 오류')
        counts=collections.Counter();pieces=0
        # 입력 칸이 생성 순서와 같은지 본다. 배치의 마지막 칸(조각 수)은 원본 decoder가 센 값이라 비교에서 뺀다.
        for row,(op,case) in zip(selected,expected,strict=True):
            keys=KEYS[op];values=row[2].split(',')
            if len(row)!=6 or row[0]!=op or len(values)!=len(keys):raise RuntimeError('건설 배치 열 오류')
            compared=keys[:-1] if op=='Place' else keys
            if values[:len(compared)]!=[str(case[k]) for k in compared]:raise RuntimeError('건설 배치 입력 순서 오류')
            counts[op]+=1
            if op=='Place':pieces+=int(values[-1])
        if dict(counts)!=item['cases'] or pieces!=item['pieces']:raise RuntimeError('건설 배치 연산/조각 수 오류')
        entry={'Place':'place','Charge':'charge','Refund':'refund','Reject':'reject','Money':'money','Add':'add'}
        # 직접 진입한 몸체는 연산 수의 두 배(두 x87 정밀도)만큼 정상 반환했어야 한다.
        for op,name in entry.items():
            if name not in p:
                if counts[op]:raise RuntimeError(f'판본에 없는 연산의 행: {op}')
            elif item['returns'].get(f'{p[name]:08x}',0)!=2*counts[op]:raise RuntimeError(f'정상 반환 수 오류: {op}')
        if item['native_calls'].get(f'{p["place"]:08x}',0)!=2*counts['Place']:raise RuntimeError('실제 배치 진입 수 오류')
        events=[event for row in selected for event in row[3].split(';') if event!='-']
        # 사건 글자별 수가 경계 처리 수와 같아야 한다. 경계 몸체는 한 번도 실행되지 않았어야 한다.
        for letter in 'OPTNGLAFXYDM':
            if item['substitutions'].get(letter,0)!=2*sum(event.startswith(letter+':') for event in events):raise RuntimeError(f'경계 사건 수 오류: {letter}')
        if sum(event.startswith('P:') for event in events)!=pieces or sum(event.startswith('O:') for event in events)!=pieces:raise RuntimeError('조각별 소유자/Pop 수 오류')
        boundary=[p[name] for name in ('take','notify','spget','addlocal','addother','format','others','player','tell','init_money','init_reject','send') if name in p]
        if any(item['native_calls'].get(f'{address:08x}',0) for address in boundary):raise RuntimeError('경계 몸체 실행/대체 혼합')
    print(f'constructionplace 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사 중 하나를 실행한다. 게임/창은 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
