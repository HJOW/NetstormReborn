#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""훨리기그45ef30의 홈/136바이트 생성/소리/공통 그림자 전체를 실제10.78에서 관측한다."""
import argparse,csv,itertools,json,socket,struct
from collections import Counter
from decomp_flyer_postpop import Oracle as FlyerOracle,ROOT,POOL,FIELDS,PACKET,TYPES,IDS,bits,digest
from decomp_local_mission_transport import STACK,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPTAG,UC_X86_REG_FPSW

# 별도 소리 객체와 원본 전용/공통 함수의 현재 진입 주소다.
SOUND=FIELDS+0x2000
ENTRIES=(0x45ef30,0x45ef10,0x45e6b0,0x45c340,0x4ac550,0x49a840)
FIXTURE=ROOT/'cpppj/tests/fixtures/sun-flyer-postpop-x86.tsv' # 제품 수정 전에 동결할 실제 결과다.
REPORT=ROOT/'cpppj/sun-flyer-postpop.json' # 기존 맨 오 워 관측과 별도 범위를 기록한다.


def cases():
    """서버·최초·두 할당 실패·signed 그림자 프레임·기존 그림자와 경계 뒤 재조회를 교차한다."""
    base=[(*row,0) for row in itertools.product((0,1),(0,1,2,0xffffffff),(0,1),(0,1),(0xffffffff,0,12),(0,1),(0,1))]
    return base+[(1,1,1,1,2,1,1,mutation) for mutation in range(2,9)]


class Oracle(FlyerOracle):
    """기본 생성·그림자/offset은 실제 명령이며 Host/장부/공간/음향만 명시 경계다."""
    def __init__(self,control=0x27f):
        """이전 실제 helper 범위에 훨리기그의 정확한 두 진입만 추가한다."""
        super().__init__(control);self.nativeCalls=Counter();self.instructions=0;self.returns=0
        rows={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        # 새로 실행하는 원본 함수의 불연속 구간을 그대로 허용한다.
        for entry in (0x45ef30,0x45ef10):
            for part in rows[entry]['ranges'].split(';'):
                lo,hi=(int(n,16) for n in part.split('-'));self.allowed.append((lo,hi+1))

    def guard(self,mu,access,address,size,value,data):
        """136바이트 생성 이미지와 관측 객체/스택/SEH 외 원본 쓰기를 거부한다."""
        ranges=[(POOL+sid*50,POOL+(sid+1)*50) for sid in IDS]+[(FIELDS,FIELDS+136),(STACK-4096,STACK+128),(0,4)]
        if not any(lo<=address and address+size<=hi for lo,hi in ranges):raise RuntimeError(f'훨리기그 선택 밖 쓰기 {address:x}/{size}')

    def instruction(self,mu,address,size,data):
        """원본 큰 경계의 인수/반환만 공급하며 전용 분기/상태 쓰기/그림자 계산은 대체하지 않는다."""
        if address in ENTRIES:self.nativeCalls[f'{address:08x}']+=1
        if any(lo<=address and address+size<=hi for lo,hi in self.allowed):self.instructions+=1
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,5);ecx=mu.reg_read(UC_X86_REG_ECX);parent=POOL+50*50
        if address==0x4e4391:
            requested=args[0];assert requested in (136,40);self.events.append('Allocate,'+str(requested))
            if requested==136:
                if self.mutation==7:self.put(parent+10,143,1);self.put(parent+12,13,2)
                if self.mutation==8:self.put(0x540bc0,0);self.put(0x5410ec,30)
                self.ret(FIELDS if self.allocated else 0)
            else:self.ret(SOUND if self.soundAllocated else 0)
            return
        if address==0x41c0d0:
            self.events.append(f'Repop,{(ecx-POOL)//50},{args[0]}');self.ret(purge=4);return
        if address==0x41bd20:
            assert ecx==FIELDS+48 and args[2]==0;self.attach_image=bytes(mu.mem_read(FIELDS,136)).hex();self.events.append(f'Attach,{args[0]},{args[1]},{args[3]}')
            if self.mutation==2:self.put(FIELDS+84,77);self.put(parent+10,143,1);self.put(parent+12,13,2);self.now(200)
            self.ret(purge=16);return
        if address==0x4ab240:
            assert ecx==SOUND;self.events.append('Sound,'+','.join(map(str,args[:4])));self.ret(SOUND,16);return
        super().instruction(mu,address,size,data)

    def sample(self,case):
        """명시 현재 입력을 다시 설치하고 실제 전체 이미지/객체/경계 순서를 읽는다."""
        server,flags,self.allocated,self.soundAllocated,frame,shadow,existing,self.mutation=case;mu=self.mu;self.events=[];self.pop_count=0;self.attach_image='-'
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x59ab20,TYPES);self.put(0x541348,188);self.put(0x5410ec,31);self.put(0x540bc0,server);self.now(100.25);self.put(0,0x12340000)
        # 부모·현재 홈·생성 그림자·기존 종속은 각50바이트를 구별한 동일 입력이다.
        for sid in IDS:
            raw=bytearray(b'\xa5'*50);struct.pack_into('<IHHHBBHff',raw,0,0x50a1e8,0,0,0,73,0,60,50.25,50.5);struct.pack_into('<I',raw,36,2);raw[40]=0;mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(POOL+60*50,0x509930);self.put(POOL+60*50+10,72,1);self.put(POOL+50*50+36,frame);self.put(POOL+50*50+8,60 if existing else 0,2)
        # 현재 TYPE 포인터 snapshot을 구별하도록 대체143의 speed/프레임/소리도 다르게 준다.
        for typ in (73,143):
            record=TYPES+typ*500;mu.mem_write(record,bytes(500));self.put(record+0xe0,bits(.75 if typ==73 else 1.25));self.put(record+0x114,12 if typ==73 else 18);self.put(record+0xe8,0x400000 if shadow else 0);self.put(record+0x74,901 if typ==73 else 911);self.put(record+0x6c,902 if typ==73 else 912)
        mu.mem_write(FIELDS,b'\xa5'*136);saved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        # 비휘발/SEH/FPU와 스택을 독립 관측마다 같은 입력으로 되돌린다.
        for register,value in saved.items():mu.reg_write(register,value)
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,POOL+50*50),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,self.control),(UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_FPSW,0)):mu.reg_write(register,value)
        mu.mem_write(STACK,struct.pack('<II',STOP,flags));mu.emu_start(0x45ef30,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+8 and self.words(0,1)[0]==0x12340000 and all(mu.reg_read(r)==v for r,v in saved.items()) and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        self.returns+=1;fields=bytes(mu.mem_read(FIELDS,136)).hex();objects=b''.join(bytes(mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex();globalsImage=(bytes(mu.mem_read(0x540bc0,4))+bytes(mu.mem_read(0x55b4d0,8))+bytes(mu.mem_read(0x5410ec,4))).hex()
        return [','.join(map(str,case)),fields,objects,globalsImage,self.attach_image,';'.join(self.events) or '-']


def main():
    """원문 관측 생성과 읽기 전용 SHA 감사를 분리한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 검사 중 원본 결과를 재생성하지 않는다.
        for name,wanted in report['sha256'].items():assert digest(ROOT/name)==wanted,name
        assert report['rows']==len(cases())==len(FIXTURE.read_text('utf-8').splitlines())-1;print('훨리기그 후처리 원문 감사',report['rows']);return
    first=Oracle();second=Oracle(0x37f);rows=[]
    # 두 정밀도의 바이트/순서 차이는 사례 삭제 없이 즉시 실패시킨다.
    for case in (cases()[::90] if args.smoke else cases()):
        row=first.sample(case);assert row==second.sample(case),case;rows.append(row)
    if args.smoke:print('훨리기그 후처리 정상 반환',first.returns+second.returns);return
    FIXTURE.write_text('# 입력/전체136바이트/객체250바이트/전역16바이트/부착 전136바이트/효과 순서\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_flyer_postpop.py','tools/decomp_local_mission_transport.py','tools/decomp_sun_flyer_postpop.py',FIXTURE.relative_to(ROOT).as_posix()]
    REPORT.write_text(json.dumps(dict(unit=187,host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',rows=len(rows),normalReturns=first.returns+second.returns,instructions=first.instructions+second.instructions,precisionDifferences=0,nativeCalls=dict(first.nativeCalls+second.nativeCalls),boundaries='45ef30/기본 비행45c340/45e6b0/그림자45ef10/TYPE getter 전체.힙/Host부착/음향생성/홈·그림자Pop·Unpop·Repop/시각/공통장부는 명시 경계다. 전체 전용 비행 Frame/통지·홈파괴/음향장치/미션 완주는 포함하지 않는다.',sha256={p:digest(ROOT/p) for p in paths}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('훨리기그 후처리 독립 원본',len(rows),first.returns+second.returns,first.instructions+second.instructions)


if __name__=='__main__':main()
