#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 Cast/Drop 요청427070·메뉴42aa50·서버444bc0의 패킷과 효과 순서를 독립 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,VTABLE,PROCESS,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as InvokeOracle,POOL
from decomp_nugget_regular import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-drop-x86.tsv' # 최초 독립 원문이며 제품 결과로 기대값을 만들지 않는다.
REPORT=ROOT/'cpppj/carrier-drop-native.json' # 실제 원본 몸체/명시 하위 경계와 현재 PC를 기록한다.
ENTRIES=(0x427070,0x42aa50,0x444bc0,0x40a9e0,0x40ac60,0x4df230,0x49a840,0x40eb80,0x4b2270,0x4b1b20,0x44daa0,0x4e49c0,0x41df60,0x40e800,0x4ac200,0x4ac220) # 직접 실행할 원본 코드다.
PACKET,MENU,INPUT,THUNK=STOP+0x600,STOP+0x640,STOP+0x680,STOP+0x700 # 호출자 소유 패킷/메뉴 입력과 cdecl 호출 코드다.
FLOAT,FLOAT_VALUE=STOP+0x800,STOP+0x820 # 실제 fld/ret로 명시 경제 조회의 x87 반환을 공급한다.
DETACH,UNPOP,DESTROY,OWNER,POP,EVENT=(STOP+0x900+i*0x20 for i in range(6)) # 큰 월드 가상 효과의 선언 경계다.
SLOTS=(50,60,61,62,70,80,81) # 부모·세 내용물·새 주문·두 프로세스의 전체 raw를 보존한다.


def cases():
    """원본 관문/부모와 내용물 선택/비용/헤더/서버 정상·거부를 독립 입력으로 만든다."""
    base=dict(op=0,chosen=60,genus=0x100,extra=128,immobile=0,afford=1,cost=100,attached=1,
        owner=1,local=1,sender=1,server=1,checking=0,parentFlags=0,contentFlags=0,kind=168,contentType=6,
        count=188,point=1,create=1,carrierGenus=0x10000,standing=0,flag=0,chain=0,mark=1,
        money=bits(4000),x=bits(20.75),y=bits(21.5),protocol=1,message=0x50,length=9,mutation=0)
    rows=[]
    # 요청의 주문 전용 관문을 교차하고 일반 내려놓기에는 같은 관문을 임의로 추가하지 않는다.
    for genus,extra,immobile,afford in itertools.product((0,0x100,0x8000,0x8100),(0,128,160),(0,1),(0,1)):
        rows.append(dict(base,genus=genus,extra=extra,immobile=immobile,afford=afford))
    # 선택0은 첫 비주문을 찾으며 메뉴 Cast는 최초 항목을 보관한 채 플래그를 검색하는 실제 동작이다.
    for op,chosen,chain,mark in itertools.product((0,1),(0,60),(0,1,2,3),(0,1)):
        rows.append(dict(base,op=op,chosen=chosen,chain=chain,mark=mark))
    # 요청은 부모 상태에 새 free/dead 필터를 넣지 않으며 비용/로컬 소유자를 현재 값으로 읽는다.
    for flags,cost,local,attached in itertools.product((0,1,2,3),(0,100,1001),(1,2),(0,1)):
        rows.append(dict(base,parentFlags=flags,cost=cost,local=local,attached=attached))
    # 요청 헤더는 프로토콜 bit7만 제거하고 선언 길이를 최소4로 고치지 않는다.
    for protocol,length in itertools.product((0,1,128,255),(0,1,3,9,65535)):
        rows.append(dict(base,genus=0,protocol=protocol,length=length))
    # 수신의 실제 free/dead/void/kind/타입 범위를 교차한다.
    for parent,content,kind,ctype in itertools.product((0,1,2,4),(0,1,2,4,8),(0,168,188),(6,7)):
        rows.append(dict(base,op=2,parentFlags=parent,contentFlags=content,kind=kind,contentType=ctype))
    # 주문은 전체DWORD 송신자와 부모owner BYTE를 비교한다. 부족 비용 통지 뒤에도 생성 분기를 계속한다.
    for sender,owner,local,money,point in itertools.product((1,2,257),(1,2),(1,2),(bits(0),bits(99),bits(100),bits(4000)),(0,1)):
        rows.append(dict(base,op=2,sender=sender,owner=owner,local=local,money=money,point=point))
    # 일반 내려놓기의 생성 실패·BYTE 정규화·지상/공중 구별·현재 정지 여부다.
    for made,flag,genus,standing in itertools.product((0,1),(0,1,2,255),(0x10000,0x20000),(0,1)):
        rows.append(dict(base,op=2,genus=0,create=made,flag=flag,carrierGenus=genus,standing=standing))
    # 서버/강한 진단이 복귀해도 원본은 같은 뒤쪽 관문을 계속 실행한다.
    for server,checking,kind in itertools.product((0,1),(0,1),(0,168,188)):
        rows.append(dict(base,op=2,server=server,checking=checking,kind=kind))
    # 큰 하위 효과 뒤 현재 비용/타입/소유자/호출자 패킷을 다시 읽는 위치를 확인한다.
    for op,mutation in itertools.product((0,2),range(1,7)):
        rows.append(dict(base,op=op,mutation=mutation))
    return [json.loads(value) for value in dict.fromkeys(json.dumps(row,sort_keys=True,separators=(',',':')) for row in rows)]


class Oracle(BaseOracle):
    """원본 제어 흐름/패킷/finder/정수변환을 실행하고 큰 월드·경제·장치 효과를 기록한다."""
    def __init__(self):
        """현재 보호PE와 격리 호출자/가상 경계를 연결한다. 게임/OS를 실행하지 않는다."""
        super().__init__()
        self.mu.mem_map(POOL,0x10000)
        self.allowed=[]
        ranges(self,ENTRIES)
        self.calls=collections.Counter()
        self.instructions=0
        # cdecl 수신의 세 인자를 실제 스택에 공급하고 호출자 쪽에서 정리한다.
        code=b'\x68'+struct.pack('<I',PACKET)+b'\x6a\x00\xff\x35'+struct.pack('<I',INPUT)+b'\xe8'+struct.pack('<i',0x444bc0-(THUNK+18))+b'\x83\xc4\x0c\xc3'
        assert len(code)==22
        self.mu.mem_write(THUNK,code)
        self.mu.mem_write(FLOAT,b'\xd9\x05'+struct.pack('<I',FLOAT_VALUE)+b'\xc3')
        # 직접 효과를 원본 객체의 실제 호출 위치에서만 관측한다.
        for offset,address in ((0x48,DETACH),(0x4c,UNPOP),(0x10,DESTROY),(0x74,OWNER),(0x90,POP),(0x1c,EVENT)):
            self.put(VTABLE+offset,address)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def instruction(self,mu,address,size,data):
        """원본이 선택한 인자/순서/패킷만 기록하며 분기 결과를 기대값으로 합성하지 않는다."""
        self.instructions+=1
        sp=mu.reg_read(UC_X86_REG_ESP)
        args=struct.unpack('<10I',mu.mem_read(sp+4,40))
        this=mu.reg_read(UC_X86_REG_ECX)
        sid=(this-POOL)//50
        c=self.case
        if address in ENTRIES:
            self.calls[f'{address:08x}']+=1
        if address==0x492090:
            self.event('immobile',args[0]);self.ret(c['immobile']);return
        if address==0x425e90:
            self.event('afford',args[0])
            if c['mutation']==1:
                self.put(0x59ab50+c['kind']*4,77+c['kind']*23)
            self.ret(c['afford']);return
        if address==0x41dff0:
            self.event('subtract',*args[:2])
            if c['mutation']==2:
                self.put(POOL+60*50+18,169,2)
            self.ret();return
        if address==0x497220:
            self.event('project',args[1],args[2]);self.put(this,321);self.put(this+4,234);self.ret(this,12);return
        if address==0x43dd20:
            self.event('popup',*args[:4]);self.ret();return
        if address==0x425ef0:
            self.event('regular',*args[:2]);self.ret();return
        if address==0x425e60:
            self.event('attached',args[0]);self.put(this+8,80 if c['attached'] else 0);self.ret(this,4);return
        if address==0x4b2340:
            self.ret(POOL+self.get(this+8)*50);return
        if address==0x4ae2d0:
            self.event('process',(this-POOL)//50);self.ret(PROCESS);return
        if address==0x4b1b80:
            self.put(this+8,81 if self.get(this+8)==80 else 0);self.ret();return
        if address==EVENT:
            self.event('notify',*args[:3]);self.ret(0,12);return
        if address==0x4d5c90:
            self.event('select',*args[:3]);self.ret();return
        if address==0x4e2dc0:
            self.packet=bytes(mu.mem_read(args[1],9)).hex();self.events.append('send:'+str(args[0])+':'+self.packet);self.ret();return
        if address==0x4ae010:
            self.event('base-menu',sid);self.ret(7,4);return
        if address==0x428e00:
            self.event('pickup',sid);self.ret();return
        if address==0x4e0620:
            self.event('assert',args[2]);self.ret();return
        if address==0x426a70:
            self.event('point',sid,args[1]);self.put(args[0],c['x'] if c['point'] else bits(-10000));self.put(args[0]+4,c['y'] if c['point'] else bits(-10000));self.ret(0,8);return
        if address==0x452fe0:
            self.event('queue',*args[:3])
            if c['mutation']==3:
                self.put(POOL+50*50+34,2,1)
            if c['mutation']==4:
                self.put(PACKET+4,51,2)
            self.ret();return
        if address==0x442a40:
            self.event('bad-owner',*args[:2]);self.ret();return
        if address==0x40ef30:
            self.event('money',args[0]);self.put(FLOAT_VALUE,c['money']);mu.reg_write(UC_X86_REG_EIP,FLOAT);return
        if address==0x442b50:
            self.event('refund',*args[:6]);self.ret();return
        if address==0x488fe0:
            self.event('movement',args[0]);self.ret(10);return
        if address==0x4b1a90:
            self.event('remove-path',*args[:4])
            if c['mutation']==5:
                self.put(POOL+50*50+34,2,1)
            self.ret();return
        if address in (DETACH,UNPOP):
            self.event('detach' if address==DETACH else 'unpop',sid,args[0]);self.ret(0,4);return
        if address==0x4af530:
            self.event('create',*args[:2]);self.put(POOL+70*50+10,args[0],1)
            if c['mutation']==6:
                self.put(PACKET+4,51,2)
            self.ret(POOL+70*50);return
        if address==OWNER:
            self.event('owner',sid,args[0]);self.put(this+34,args[0],1);self.ret(0,4);return
        if address==POP:
            self.event('pop',sid,*args[:3]);self.ret(0,12);return
        if address==0x41e860:
            length=struct.unpack('<H',mu.mem_read(args[2]+2,2))[0] # 실제 헤더의6바이트만 관측하며 뒤의 스택 잔여 바이트를 패킷에 넣지 않는다.
            self.events.append('local-send:'+str(args[0])+':'+bytes(mu.mem_read(args[2],length)).hex());self.ret();return
        if address==0x4e2e30:
            length=struct.unpack('<H',mu.mem_read(args[0]+2,2))[0] # 같은 실제 전송 헤더의 물리 길이다.
            self.events.append('broadcast:'+bytes(mu.mem_read(args[0],length)).hex());self.ret();return
        if address==0x444760:
            self.event('place',*args[:10]);self.ret(c['create']);return
        if address==DESTROY:
            self.event('destroy',sid,args[0],mu.mem_read(this+22,1)[0]);self.ret(0,4);return
        if address==0x40f100:
            self.event('standing',sid);self.ret(c['standing']);return
        if address==0x429010:
            self.event('escape',sid,*args[:2]);self.ret(0,8);return
        if THUNK<=address<THUNK+22 or FLOAT<=address<FLOAT+7:
            return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'운반물 요청/수신 미허용 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """원본 몸체는 스택/SEH와 내용물22·새 주문의 QA WORD12만 직접 변경할 수 있다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==0 and size==4:
            return
        if size==1 and address in (POOL+60*50+22,POOL+61*50+22,POOL+62*50+22) or address==POOL+70*50+12 and size==2:
            return
        raise RuntimeError(f'운반물 요청/수신 미허용 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호 이미지·격리 슬롯/TYPE/프로세스·호출자 영역 밖 읽기를 거부한다."""
        spans=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(VTABLE,VTABLE+0x1000),(PROCESS,PROCESS+0x1000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(low<=address and address+size<=high for low,high in spans):
            raise RuntimeError(f'운반물 요청/수신 미허용 읽기 {address:08x}+{size}')

    def sample(self,c,precision):
        """현재 입력만 공급하고 두 정밀도의 전체 raw/효과/패킷을 독립적으로 관측한다."""
        self.case=c;self.events=[];self.packet='-'
        # 모든 명시 슬롯을 매 입력 새로 초기화해 앞선 실행 상태가 다음 실행에 섞이지 않게 한다.
        for sid in SLOTS:
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=133 if sid==50 else c['contentType'] if sid in (60,61,62) else 51 if sid==80 else 52
            raw[11]=c['parentFlags'] if sid==50 else c['contentFlags'] if sid==60 else 0;raw[34]=c['owner'];raw[40]=c['extra']
            struct.pack_into('<II',raw,14,c['x'],c['y'])
            if sid in (60,61,62):
                struct.pack_into('<H',raw,18,c['kind'] if sid==60 else 169);raw[22]=c['flag'];raw[20]=c['mark'] if sid==60 else 1
            self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.mu.mem_write(POOL+51*50,bytes(self.mu.mem_read(POOL+50*50,50))) # 수신 콜백이 호출자 parent WORD를 바꿀 때의 두 번째 정상 부모 입력이다.
        chain=((),(60,),(60,61),(60,61,62))[c['chain']]
        self.put(POOL+50*50+6,chain[0] if chain else 0,2)
        # 실제 contained finder가 링크를 직접 따라가도록 next와contained상태만 입력한다.
        for index,sid in enumerate(chain):
            self.put(POOL+sid*50+4,chain[index+1] if index+1<len(chain) else 0,2)
        self.mu.mem_write(TYPES,bytes(0x20000))
        self.put(TYPES+133*500+0xec,c['carrierGenus']);self.put(TYPES+168*500+0xec,c['genus']);self.put(TYPES+169*500+0xec,0)
        # 실제 비용 인코딩은 TYPE별 정수에23×타입을 더한 전역 표다.
        for kind in (168,169,188,0):
            self.put(0x59ab50+kind*4,c['cost']+kind*23)
        for address,value in ((0x5c8464,POOL),(0x59ab20,TYPES),(0x540c70,c['local']),(0x540bc0,c['server']),(0x5e4794,c['checking']),(0x541348,c['count']),(0x541088,6),(0x54113c,51),(0x5429bc,0x030000b2),(0x542b38,0x0300011a),(0x5428d0,0x03000076),(0x5428cc,0x03000075)):
            self.put(address,value)
        self.put(0x52ee0e,c['message'],1);self.put(0x52ee0f,c['protocol'],1);self.put(0x52ee10,c['length'],2)
        self.mu.mem_write(PACKET,struct.pack('<BB3HB',1,0x50,9,50,c['chosen'],c['flag']))
        self.mu.mem_write(MENU,struct.pack('<4I',0x0300011a,0,50,0));self.put(INPUT,c['sender']);self.put(0,0x87654321)
        entry=(0x427070,0x42aa50,THUNK)[c['op']]
        args=b'' if c['op']==2 else struct.pack('<I',MENU if c['op']==1 else c['chosen'])
        InvokeOracle.invoke(self,entry,args,precision,False)
        self.returns+=1
        assert self.get(0)==0x87654321
        return ';'.join(self.events) or '-',self.packet,','.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in SLOTS)


def main():
    """독립 기대를 처음 동결하거나 이미 동결된 원문과 출처만 읽기 전용 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--smoke',action='store_true')
    options=parser.parse_args()
    if options.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 현재 도구/보호 원본/동결 기대의 전체 바이트를 대조한다.
        for path,wanted in report['sha256'].items():
            assert digest(ROOT/path)==wanted,path
        rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==report['uniqueInputs'] and len(rows)*2==report['normalReturns']
        assert [json.loads(row[0]) for row in rows]==cases()
        print('Cast/Drop 원본 요청·메뉴·서버 감사',len(rows),report['normalReturns'])
        return
    oracles=[Oracle(),Oracle()]
    inputs=cases()
    if options.smoke:
        inputs=[next(c for c in inputs if c['op']==op and c['genus']==genus and c['chosen']==60 and c['parentFlags']==c['contentFlags']==0 and c['kind']==168 and c['point']==1) for op,genus in ((0,0x100),(0,0),(1,0x100),(2,0x100),(2,0))]
    else:
        assert not FIXTURE.exists() and not REPORT.exists(),'동결 원문을 덮어쓰지 않는다'
    rows=[];different=[]
    # 제품 C++를 읽지 않고 동일 입력을 두 원본 x87 제어어에서 독립 실행한다.
    for index,c in enumerate(inputs):
        try:
            results=[oracle.sample(c,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))]
        except Exception as error:
            raise RuntimeError(json.dumps(c,sort_keys=True)) from error
        if results[0]!=results[1]:
            different.append(index)
        rows.append((json.dumps(c,sort_keys=True,separators=(',',':')),*results[0],*results[1]))
    if options.smoke:
        print(json.dumps(rows,indent=2));return
    FIXTURE.write_text('# input effects53 packet53 raws53 effects64 packet64 raws64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n','utf-8',newline='\n')
    paths=('originals/Netstorm.exe','tools/decomp_carrier_drop.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_carrier_falling.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-drop-x86.tsv')
    record=dict(unit=228,host=socket.gethostname(),date='2026-10-19',lastPreciseDecompile=dict(host='VM-W11-Codex',date='2026-10-19'),uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),instructions=sum(oracle.instructions for oracle in oracles),nativeCalls=dict(sum((oracle.calls for oracle in oracles),collections.Counter())),precisionDifferentInputs=different,
        boundaries='원본427070/42aa50/444bc0 및 헤더/contained finder/TYPE/getCost/ftol/point 관문을 직접 실행한다. 지면불가/비용승인·돈차감·투영·팝업·Regular·부착프로세스조회/통지·선택·송신·큐·하위경제·이동삭제·공간분리·생성/가상Owner/Pop·배포·일반건설/삭제·회피는 명시 큰 하위 경계다. GP/retN/스택canary/SEH/x87/실행·읽기·쓰기 범위와 전체7슬롯을 검사한다. 실제 주문 폭발·사제 포획·미션 완주의 증거가 아니다.',sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n')
    print(json.dumps({key:record[key] for key in ('uniqueInputs','normalReturns','instructions','nativeCalls','precisionDifferentInputs')}))


if __name__=='__main__':
    main()
