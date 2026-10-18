#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""훨리기그 전용 초기화/폭탄 프레임/통지/장식/홈파괴와 빈 표적을 실제10.78에서 관측한다."""
import argparse,csv,itertools,json,math,socket,struct
from collections import Counter
from decomp_flyer_frame import Oracle as FrameOracle,ROOT,POOL,FIELDS,TYPES,IMAGE,STACK,STOP,bits,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPTAG,UC_X86_REG_FPSW

# 현재 부모/표적/홈/종속과 폭탄 두 개를 독립 관측한다.
IDS=(50,51,60,61,62,63,70,80,81)
SHAPE,OUTPUT,SHIM=IMAGE+0x1000,IMAGE+0x8000,STOP+0x400
ENTRIES=(0x45d2c0,0x45bd00,0x45bd60,0x45be00,0x45beb0,0x45be40,0x45e170,0x45bc30,0x45b760,0x45b7a0,0x45b6f0,0x41bfb0,0x460de0,0x4ac550,0x49a840,0x419850,0x49ace0,0x4d4c60,0x43fa50,0x4e49c0,0x4e5e8a,0x4eded0,0x4edf9a,0x4ee1b7,0x46af80,0x4ac200,0x4adc60)
FIXTURE=ROOT/'cpppj/tests/fixtures/sun-flyer-frame-x86.tsv' # 새 제품 전 실제 명령의 결과를 동결한다.
REPORT=ROOT/'cpppj/sun-flyer-frame.json' # 계산과 기존 공통/장치 경계를 구별한다.


def dbits(value):
    """double 입력을 문자열 반올림 없이 보존한다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def cases():
    """전용 제어·낙하·폭발·기한·일시정지와 현재 경계 뒤 재조회를 교차한다."""
    rows=[('I',(dbits(now),pattern)) for now,pattern in itertools.product((0,100.25,-5.25),(0,0x5a,0xa5))]
    rows += [('A',(dbits(now),form,count)) for now,form,count in itertools.product((0,.03,.07267799973487854,.14535599946975708,100.25,9999.75),(0,70,65535),(1,9,16))]
    rows += [('N',(server,message,info,target,received,mutation)) for server,message,info,target,received,mutation in itertools.product((0,1),(0,1,2,9),(0,1),(0,60),(0,60),(0,))]
    rows += [('P',(server,home,state,bomb)) for server,home,state,bomb in itertools.product((0,1),(0,61,65535),(0,1,2),(0,80))]
    rows += [('C',(bomb,mutation)) for bomb,mutation in itertools.product((0,80),(0,6))]
    rows.append(('R',()))
    rows += [('E',(server,mutation)) for server,mutation in itertools.product((0,1),(0,9))]
    # wrapper의 초기화·삭제 권한과 실제 일시정지 비교는 각각 독립 입력이다.
    rows += [('F',(init,extra,boss,pause,0,0,0,0,0,0,0,0,0)) for init,extra,boss,pause in itertools.product((0,1),(0,32),(0,1),(0,1,0xffffffff))]
    # 폭탄 낙하/폭발의 부호 프레임·void·기한과 부모 복귀를 교차한다.
    rows += [('F',(1,0,1,pause,80,frame,void,age,due,0,0,saved,0)) for pause,frame,void,age,due,saved in itertools.product((0,1),(0,2,3,5),(0,4),(0,1,2),(0,1),(0,1))]
    # 다음 발사0/이전/일치/미래와 두 표적 결과다. 폭탄 유무는 실제 생성 허용을 구별한다.
    rows += [('F',(1,0,1,0,bomb,0,0,0,0,may,fire,0,0)) for bomb,may,fire in itertools.product((0,80),(0,1),(0,1,2,3))]
    rows += [('F',(1,0,1,0,80,0,0,1,1,1,1,1,mutation)) for mutation in (1,4,5)]
    rows += [('F',(1,0,1,0,0,0,0,0,0,1,1,0,mutation)) for mutation in (2,3,7,8)]
    rows += [('Z',())]
    return list(dict.fromkeys(rows))


class Oracle(FrameOracle):
    """전용 함수·기하 getter·CRT는 실제 실행하며 기존 공통Frame/Host/공간/소리만 경계다."""
    def __init__(self,control=0x27f):
        """정확한 함수 구간과 반환 관찰용 FSTP만 설치한다."""
        super().__init__(control);facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')};self.allowed=[];self.calls=Counter();self.instructions=0;self.returns=0
        # 기존 전체 정밀 표의 불연속 구간을 빠짐없이 사용한다.
        for entry in ENTRIES:
            if entry==0x45e170:continue
            for part in facts[entry]['ranges'].split(';'):
                lo,hi=(int(n,16) for n in part.split('-'));self.allowed.append((lo,hi+1))
        assert bytes(self.mu.mem_read(0x45e170,7))==bytes.fromhex('d9053c9f5000c3')
        self.allowed += [(0x45e170,0x45e177),(0x4e5e94,0x4e5eb2),(0x4ee254,0x4ee25b)]
        code=b'\xe8'+struct.pack('<i',0x45e170-(SHIM+5))+b'\xdd\x1d'+struct.pack('<I',OUTPUT)+b'\xc3';self.mu.mem_write(SHIM,code);self.allowed.append((SHIM,SHIM+len(code)))
        self.put(0x5e4acc,0) # 실제 CRT의 native FPREM 경로를 선택하는 명시 전역 입력이다.

    def guard(self,mu,access,address,size,value,data):
        """선언한 필드/객체/관측 반환/스택/SEH 외 원본 CPU 쓰기를 거부한다."""
        spans=[(POOL+sid*50,POOL+(sid+1)*50) for sid in IDS]+[(FIELDS,FIELDS+136),(OUTPUT,OUTPUT+8),(STACK-8192,STACK+256),(0,4)]
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'훨리기그 Frame 선택 밖 쓰기 {address:x}/{size}')

    def instruction(self,mu,address,size,data):
        """새 계산은 모두 원본에 맡기고 외부 효과의 명시 인수/변경만 공급한다."""
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if any(lo<=address and address+size<=hi for lo,hi in self.allowed):self.instructions+=1
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,10);ecx=mu.reg_read(UC_X86_REG_ECX);sid=(ecx-POOL)//50
        if address==0x4e0620:self.event('Assert,'+str(args[2]));self.ret();return
        if address==0x4ac140:self.event('Sid,'+','.join(map(str,args[:2])));self.ret(0<args[0]<65535);return
        if address==0x45c9d0:self.event('Base');self.ret();return
        if address==0x4605f0:
            self.event('Pieces,'+str(args[0]))
            if self.mutation==9:self.put(FIELDS+56,51)
            self.ret();return
        if address==0x4af110:self.event('Count,'+str(args[0]));self.ret(self.count);return
        if address==0x4ae780:self.event('Remove,'+','.join(map(str,args[:3])));self.ret();return
        if address==0x41c010:self.event('Refresh');self.ret();return
        if address==0x4afe50:
            self.event(f'Unpop,{sid},{args[0]}')
            if self.mutation==1 and sid==50:self.put(POOL+50*50+14,bits(71.25));self.put(POOL+50*50+18,bits(72.5));self.put(FIELDS+56,51);self.now(200)
            self.ret(purge=4);return
        if address==0x4b02d0:
            self.event('Pop,'+str(sid)+','+','.join(map(str,args[:3])));self.put(ecx+14,args[0]);self.put(ecx+18,args[1])
            if self.mutation==4 and sid==80:self.put(POOL+50*50+18,bits(52.5))
            self.ret(purge=12);return
        if address==0x41c0d0:
            self.event(f'Repop,{sid},{args[0]}')
            if self.mutation==5:self.put(FIELDS+96,0)
            self.ret(purge=4);return
        if address==0x4af780:
            self.event(f'Destroy,{sid},{args[0]}')
            if self.mutation==6:self.put(FIELDS+96,81)
            self.ret(purge=4);return
        if address==0x45ae60:self.query=args[0];self.ret(IMAGE,purge=4);return
        if address==0x46af80 and self.kind!='Z':
            self.event(f'May,{self.query},{args[8]}')
            if self.mutation==3:self.put(FIELDS+56,51);self.put(FIELDS+12,61,2)
            self.ret(self.may);return
        if address==0x4af530:
            self.event('Create,'+','.join(map(str,args[:2])));self.put(POOL+80*50+10,args[0],1)
            if self.mutation==7:self.put(FIELDS+96,81)
            if self.mutation==8:self.put(POOL+80*50+10,164,1)
            self.ret(POOL+80*50);return
        if address==0x4a9c30:
            assert args[3:5]==(0,0);self.event('Sound,'+','.join(map(str,args[:3])))
            if self.mutation==2:self.now(200)
            self.ret();return
        if address==0x41c100:self.event(f'Home,{sid},'+','.join(map(str,args[:2])));self.ret(purge=8);return
        if any(lo<=address and address+size<=hi for lo,hi in self.allowed):return
        raise RuntimeError(f'훨리기그 Frame 미검토 명령 {address:x}')

    def sample(self,case):
        """매 호출의 raw/전역/타입/SHP 입력만 설치하고 결과는 정상 반환 뒤 읽는다."""
        self.kind,values=case;self.events=[];self.mutation=0;self.count=9;self.may=0;self.query=0;mu=self.mu
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x59ab20,TYPES);self.put(0x541348,188);self.put(0x5412e4,163);self.put(0x54117c,73);self.put(0x541280,138);self.put(0x540bc0,1);self.put(0x540bc4,1);self.put(0x55b4b0,0);self.put(0x5e4794,0);self.put(0,0x12340000);self.now(100)
        mu.mem_write(POOL,bytes(50)) # 빈 표적은 실제 SID0/TYPE0의 초기0 자료다.
        # 현재 실제 함수가 읽는 타입별 입력이며 SHP 월드 폭은 발자국과 구별한다.
        for typ in (0,73,80,163,164):
            record=TYPES+typ*500;mu.mem_write(record,bytes(500));self.put(record+0x114,20);self.put(record+0x1d4,2);self.put(record+0x1d8,2);self.put(record+0xdc,SHAPE);self.put(record+0x70,881 if typ==163 else 882);self.put(record+0x170,3);self.put(record+0x134,4);self.put(record+0x174,6)
        # getter가 실제 읽는 물리36바이트 헤더의 월드 폭/높이다. pixel stream은 이 관측에서 읽지 않는다.
        for index in range(20):
            header=SHAPE+0x1000+index*64;self.put(SHAPE+8+index*8,header-SHAPE+36);mu.mem_write(header,struct.pack('<ff',2.25+index*.125,3.75)+bytes(28))
        # 부모·표적·홈·Form과 두 폭탄은 서로 구별한50바이트 원문 입력이다.
        for sid in IDS:
            raw=bytearray(b'\xa5'*50);struct.pack_into('<IHHHBBHff',raw,0,0x50a1e8 if sid<60 else 0x509b98 if sid>=80 else 0x501dd0,0,70 if sid in (50,51) else 0,0,73 if sid<60 else 163 if sid>=80 else 72,0,50 if sid==61 else 0,50.25,50.5);struct.pack_into('<I',raw,36,0);raw[34]=1;raw[40]=0;mu.mem_write(POOL+sid*50,bytes(raw))
        fields=bytearray(b'\xa5'*136);struct.pack_into('<I',fields,0,0x509f24);fields[8]=0x10;struct.pack_into('<HH',fields,10,61,60);struct.pack_into('<III',fields,48,0x509f04,70,50);struct.pack_into('<IIII',fields,80,1,1,0,0);struct.pack_into('<I',fields,96,0);struct.pack_into('<dddff',fields,104,100,0,100,-10000,-10000)
        entry,receiver,args,purge=0x45d2c0,FIELDS+48,[],0
        if self.kind=='I':now,pattern=values;fields=bytearray([pattern]*136);self.now(struct.unpack('<d',struct.pack('<Q',now))[0]);entry=0x45bd00;receiver=FIELDS
        elif self.kind=='A':now,form,self.count=values;self.now(struct.unpack('<d',struct.pack('<Q',now))[0]);struct.pack_into('<I',fields,52,form);entry=0x45be40;receiver=FIELDS
        elif self.kind=='N':server,message,info,target,received,self.mutation=values;self.put(0x540bc0,server);struct.pack_into('<H',fields,12,target);entry=0x45bd60;args=[message,info,received];purge=12
        elif self.kind=='P':server,home,state,bomb=values;self.put(0x540bc0,server);struct.pack_into('<H',fields,10,home);self.put(POOL+61*50+11,state,1);struct.pack_into('<I',fields,96,bomb);entry=0x45beb0;args=[777];purge=4
        elif self.kind=='C':bomb,self.mutation=values;struct.pack_into('<I',fields,96,bomb);entry=0x45be00;receiver=FIELDS
        elif self.kind=='R':entry=SHIM;receiver=FIELDS
        elif self.kind=='E':server,self.mutation=values;self.put(0x540bc0,server);entry=0x45bc30;receiver=FIELDS
        elif self.kind=='Z':entry=0x46af80;receiver=0;args=[bits(50.25),bits(50.5),bits(49.25),bits(49.5),73,0,1,0,0]
        else:
            init,extra,boss,pause,bomb,frame,void,age,due,self.may,fire,saved,self.mutation=values;struct.pack_into('<I',fields,84,init);self.put(POOL+50*50+40,extra,1);self.put(0x540bc4,boss);self.put(0x55b4b0,pause);struct.pack_into('<I',fields,96,bomb);self.put(POOL+80*50+36,frame);self.put(POOL+80*50+11,void,1);struct.pack_into('<d',fields,104,100-(0,.25,1)[age]);struct.pack_into('<d',fields,120,99 if due else 101);struct.pack_into('<d',fields,112,(0,99,100,101)[fire]);struct.pack_into('<f',fields,132,49 if saved else 51)
        mu.mem_write(FIELDS,bytes(fields));mu.mem_write(OUTPUT,bytes(8));saved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        # 모든 비휘발/제어어/스택/SEH는 독립 호출의 입력이다.
        for register,value in saved.items():mu.reg_write(register,value)
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,receiver),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,self.control),(UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_FPSW,0)):mu.reg_write(register,value)
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(entry,STOP,count=300000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge and all(mu.reg_read(r)==v for r,v in saved.items()) and not mu.reg_read(UC_X86_REG_FPSW)&0x3800 and self.words(0)[0]==0x12340000
        self.returns+=1;result=str(mu.reg_read(UC_X86_REG_EAX)) if self.kind in ('A','Z') else str(struct.unpack('<Q',bytes(mu.mem_read(OUTPUT,8)))[0]) if self.kind=='R' else '-';objects=b''.join(bytes(mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex();globalsImage=b''.join(bytes(mu.mem_read(a,n)) for a,n in ((0x540bc0,4),(0x540bc4,4),(0x55b4b0,4),(0x55b4d0,8))).hex()
        return [self.kind,','.join(map(str,values)),bytes(mu.mem_read(FIELDS,136)).hex(),objects,globalsImage,result,';'.join(self.events) or '-']


def main():
    """제품 전 원문 생성과 현재 원문/입력 SHA 감사를 구분한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 감사 중 기대값을 계산하거나 다시 쓰지 않는다.
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert report['rows']==len(cases())==len(FIXTURE.read_text('utf-8').splitlines())-1;print('훨리기그 전용 비행 원문 감사',report['rows']);return
    first=Oracle();second=Oracle(0x37f);rows=[];differences=[]
    # 모든 입력을 두 정밀도에서 실행하고 차이는 삭제하지 않고 보존한다.
    for case in ([next(row for row in cases() if row[0]==kind) for kind in ('I','A','N','P','C','R','E','F','Z')]+[('A',(dbits(100.25),70,9)),('F',(1,0,1,0,80,0,0,1,1,0,0,1,0)),('F',(1,0,1,0,0,0,0,0,0,1,1,0,0))] if args.smoke else cases()):
        row=first.sample(case);alternate=second.sample(case)
        if row!=alternate:differences.append(case)
        rows.append(row)
    assert not differences,differences[:5]
    if args.smoke:print('훨리기그 전용 비행 정상 반환',first.returns+second.returns);return
    FIXTURE.write_text('# 종류/입력/전체136바이트/객체450바이트/현재전역20바이트/실제반환/효과순서\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_flyer_frame.py','tools/decomp_local_mission_transport.py','tools/decomp_sun_flyer_frame.py',FIXTURE.relative_to(ROOT).as_posix()]
    REPORT.write_text(json.dumps(dict(unit=188,host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',rows=len(rows),normalReturns=first.returns+second.returns,instructions=first.instructions+second.instructions,precisionDifferences=len(differences),nativeCalls=dict(first.calls+second.calls),boundaries='실제 전용 초기화/Frame/통지/폭탄해제/홈파괴/장식/반환/빈 표적과 TYPE·SHP/CRT fmod·정수화 전체.기존 공통Frame·표적 문맥/판정·생성/가상공간·삭제·의존·홈통지·장치소리는 명시 경계다. R의FSTP는 반환 관측 전용이며 실제 계산을 대체하지 않는다. CD/자연 비행/완주 증거는 아니다.',sha256={p:digest(ROOT/p) for p in paths}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('훨리기그 전용 비행 독립 원본',len(rows),first.returns+second.returns,first.instructions+second.instructions)


if __name__=='__main__':main()
