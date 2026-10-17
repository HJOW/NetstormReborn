#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""에너지 공급의 실제 수집/문자 대응/배치 허용값과 공급 가상 몸체를 격리 실행한다."""
import argparse
import csv
import itertools
import json
import re
import socket
import struct
from collections import Counter
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP
from decomp_particle_gradient import Oracle as CallOracle, ROOT, STACK, STOP, digest

POOL,TYPES,BUFFER,LIST=0x12000000,0x12100000,0x12200000,0x12300000  # raw/TYPE/문자열/공유 목록의 격리 영역이다.
ENTRIES=(0x4730c0,0x473330,0x4734d0,0x473570,0x41c0f0,0x41c170,0x49a840,0x49ae80,0x49a900,0x4ac550,0x4ac970,0x40e800,0x40e880,0x40e8f0,0x41d0d0,0x4e5f7b,0x4e5eb2)  # 모두 대체 없이 실제 몸체를 실행한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/energy-supply-x86.tsv'  # 숫자 입력/문자 입력과 두 정밀도의 실제 결과다.
REPORT=ROOT/'cpppj/energy-supply.json'  # 실행 범위/정상 반환/의존 바이트를 보존한다.
VTABLES=ROOT/'cpppj/tests/fixtures/energy-provider-vtables.tsv'  # 실제83VT의+a0 DWORD이며 계산 기대값과 별개다.


def bits(value):
    """명시 float 입력을 손실 없는 DWORD로 저장한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """기대값 없이 요구/공급 문자열과 소유자/기하/용량 입력을 조합한다."""
    base=[2,0,0,0,0,0,1,30,8,1,0,0,3,4,bits(100),bits(110),bits(484),256,0,0xffffffff,0xffffffff,0,0,3,0,bits(100),bits(110),1,1,0]
    result=[]
    # 실제 네 요구 문자와 특수 대체 원소/우회 전역을 교차한다.
    for available,required,theme in itertools.product(('', 'w','r','t','s','wrt','wrts','ssw','..rT','wwrrttss','WR.?'),('','w','r','t','s','ss','ws','sw','wrt','sss','rrs'),(-1,0,1,2,3)):
        row=base.copy();row[0]=0;row[19]=theme;result.append((row,available,required))
    # 우회는 실패한 실제 문자 결과를 유지하면서 반환값만1로 바꾸는 경우도 포함한다.
    for flags in itertools.product((0,1),repeat=4):
        row=base.copy();row[0]=0;row[1:5]=flags;result.append((row,'r','ww'))
        row=row.copy();row[0]=2;result.append((row,'sentinel','ww'))
    # 공급 범위/소유권/비대칭 동맹/에디터/가상 base0/필드 상태를 독립 입력으로 바꾼다.
    for operation,owners,allied,editor,entry in itertools.product((1,2),(0,1),(0,1,2),(0,1),(0,0x55,0xffffffff)):
        row=base.copy();row[0]=operation;row[21]=owners;row[10]=allied;row[5]=editor;row[20]=entry;result.append((row,'old','wrs'))
    # 실제 크기 항/저장된 dx/저장하지 않은 dy/거리의 엄격한 경계를 방문한다.
    for operation,foot,offset in itertools.product((1,2,3),((1,1),(3,4),(-1,0),(30,1)),(0,21.5,22,22.000001,30,60)):
        row=base.copy();row[0]=operation;row[12:14]=foot;row[25]=bits(99.5+offset);row[26]=bits(109.5);result.append((row,'old','s'))
    # 고정255 클램프와 가변 유효 범위를 별개로 검사한다.
    for point,limit in itertools.product(((-1,-1),(0,0),(1,1),(255,255),(256,256),(300,-5)),(22,256,300)):
        row=base.copy();row[0]=1;row[14:16]=map(bits,point);row[17]=limit;result.append((row,'old','s'))
    # 선택 용량은 문자 집계를 제한하지 않으며 문자열은254바이트에서만 끝난다.
    for count,capacity,theme,pattern in itertools.product((0,1,8,31,260),(0,1,3,30),(-1,0,3,6),(0,1,2)):
        row=base.copy();row[0]=1;row[8]=count;row[7]=capacity;row[19]=theme;row[29]=pattern;row[23]=min(capacity,3);result.append((row,'old','s'))
    # dead/free/void 원본 슬롯은 수집에서 별도 필터로 생략하지 않는다.
    for flags in (0,1,2,3,4,8,0xff):
        row=base.copy();row[24]=flags;result.append((row,'old','w'))
    # 빈 요구/부족 공급/반환된 공급 개수와 첫 비영 슬롯 소비를 구분한다.
    for operation,count,required in itertools.product((2,4),(0,1,8),('','s','wr','sss')):
        row=base.copy();row[0]=operation;row[8]=count;row[23]=min(count,3);result.append((row,'old',required))
    return list(dict.fromkeys((tuple(value&0xffffffff for value in row),available,required) for row,available,required in result))


class Oracle(CallOracle):
    """할당이나 월드 효과를 대체하지 않고 공급의 실제 PE와 CRT만 실행한다."""
    def __init__(self):
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));mapped=pe.get_memory_mapped_image();self.image_end=0x400000+len(mapped)
        self.mu.mem_map(0x400000,(len(mapped)+4095)&~4095);self.mu.mem_write(0x400000,mapped)
        # 원본 주소와 겹치지 않는 독립 입력/출력 영역이다.
        for address,size in ((POOL,0x10000),(TYPES,0x20000),(BUFFER,0x10000),(LIST,0x10000),(STACK-0x7f000,0x80000),(STOP,0x1000),(0,0x1000)):self.mu.mem_map(address,size)
        self.allowed=[];self.calls=Counter();self.returns=0;self.instructions=0
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 실제 내보낸 함수의 불연속 범위 밖으로 실행이 새지 않게 한다.
        for entry in ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        assert mapped[0x4fcb80-0x400000:0x4fcb87-0x400000]==bytes.fromhex('6a1eb94c2f5900')
        assert mapped[0x4fcb60-0x400000:0x4fcb6a-0x400000]==bytes.fromhex('6800010000b9802f5900')
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def put(self,address,value):
        """실행 전 독립 DWORD 입력만 기록한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address):
        """실제 실행 결과 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def instruction(self,mu,address,size,data):
        """공급/조회/CRT 몸체를 대체하지 않고 범위·실제 진입 수만 검사한다."""
        if address==0x4ea61d:
            # OS의 현재 스레드 저장소 조회만 C locale 입력으로 공급한다. toupper 계산은 실제 CRT다.
            esp=mu.reg_read(UC_X86_REG_ESP);ret=self.get(esp);mu.reg_write(UC_X86_REG_EAX,BUFFER+0x4000);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);self.calls['thread-locale-boundary']+=1;return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'에너지 범위 밖 실행 {address:08x}')
        self.instructions+=1
        if address in ENTRIES:self.calls[f'{address:08x}']+=1

    def read(self,mu,access,address,size,value,data):
        """실제 이미지와 명시 raw/TYPE/문자열/목록/스택만 읽게 한다."""
        if not any(low<=address and address+size<=high for low,high in ((0,4),(0x400000,self.image_end),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(BUFFER,BUFFER+0x10000),(LIST,LIST+0x10000),(STACK-0x7f000,STACK+0x1000))):raise RuntimeError(f'에너지 범위 밖 읽기 {address:08x}+{size}')

    def write(self,mu,access,address,size,value,data):
        """문자열/선택 목록/실제 선택 count와 스택 이외 쓰기는 거부한다."""
        if not any(low<=address and address+size<=high for low,high in ((0,4),(BUFFER,BUFFER+0x10000),(LIST+0x1000,LIST+0x2000),(0x592f54,0x592f58),(STACK-0x7f000,STACK+0x1000))):raise RuntimeError(f'에너지 범위 밖 쓰기 {address:08x}+{size}')

    def sample(self,case,precision):
        """한 독립 입력의 실제 반환·문자열·전체30칸 공유 목록을 읽는다."""
        row,available,required=case;mu=self.mu
        mu.mem_write(BUFFER,b'\xa5'*0x10000);mu.mem_write(BUFFER,available.encode()+b'\0');mu.mem_write(BUFFER+0x1000,b'sentinel\0');mu.mem_write(BUFFER+0x2000,required.encode()+b'\0')
        self.put(BUFFER+0x4064,self.get(0x543efc))
        self.put(0,0);self.put(0x5c8464,POOL);self.put(0x5c847c,1000);self.put(0x59ab20,TYPES)
        # 실제 원본 mutable 전역의 독립 입력이다.
        for address,value in zip((0x55a480,0x594fa4,0x5cab30,0x592f48,0x5c85a4,0x540cb0),row[1:7]):self.put(address,value)
        self.put(0x52f48c,row[16]);self.put(0x531928,row[17]);self.put(0x592f84,LIST);self.put(0x592f8c,row[8]);self.put(0x592f4c,LIST+0x1000);self.put(0x592f50,row[7]);self.put(0x592f54,row[23])
        mu.mem_write(LIST+0x1000,struct.pack('<30I',*(0 if index%2==0 else 10+index for index in range(30))))
        mu.mem_write(0x595200,b'\0'*(81*4))
        # 9×9 동맹은 sourceOwner 행/요청owner 열 방향이다.
        for owner in range(9):self.put(0x595200+(owner*9+row[9])*4,1 if row[10]>>owner&1 else 0)
        self.put(TYPES+100*500+0x1d4,row[12]);self.put(TYPES+100*500+0x1d8,row[13]);self.put(TYPES+100*500+0xec,row[22]);mu.mem_write(TYPES+100*500+0xa0,required.encode()+b'\0')
        # 공급 raw 슬롯은 free/dead까지 입력 그대로 준비한다. 기대 판정은 계산하지 않는다.
        for index in range(max(1,row[8])):
            sid=10+index;kind=101+index%8;self.put(LIST+index*4,sid)
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,0x501ea0 if row[20]>>(index%32)&1 else 0x501dd0);raw[10]=kind;raw[11]=row[24]&255;raw[34]=row[9] if not row[21] else 1+index%3
            struct.pack_into('<II',raw,14,row[25],row[26]);mu.mem_write(POOL+sid*50,bytes(raw))
            self.put(TYPES+kind*500+0x1d4,row[27]);self.put(TYPES+kind*500+0x1d8,row[28]);self.put(TYPES+kind*500+0x98,index%4 if row[29]==0 else 1 if row[29]==1 else 6+index%2)
        operation=row[0]
        if operation==0:entry,this,args,purge=0x473330,BUFFER,struct.pack('<III',BUFFER+0x2000,BUFFER+0x1000,row[19]),12
        elif operation in (1,2):entry,this,args,purge=(0x4730c0 if operation==1 else 0x4734d0),BUFFER,struct.pack('<IIII',row[14],row[15],100,row[9])+(struct.pack('<I',row[19]) if operation==1 else b''),(20 if operation==1 else 16)
        elif operation==3:entry,this,args,purge=(0x41c170 if row[20]&1 else 0x41c0f0),POOL+10*50,struct.pack('<8I',bits(1),bits(2),bits(3),bits(4),row[14],row[15],row[16],row[22]),32
        else:entry,this,args,purge=0x473570,0,struct.pack('<I',BUFFER+0x3000),4;self.put(BUFFER+0x3000,0xabcdef)
        self.call(entry,this,args,purge,precision);self.returns+=1
        returned=mu.reg_read(UC_X86_REG_EAX) if operation!=1 else 0
        # C 문자열은 첫NUL까지이며 쓰지 않은 꼬리는 결과로 합성하지 않는다.
        def string(address):return bytes(mu.mem_read(address,512)).split(b'\0',1)[0].hex() or '-'
        return (str(returned),string(BUFFER),string(BUFFER+0x1000),str(self.get(0x592f54)),bytes(mu.mem_read(LIST+0x1000,120)).hex(),str(self.get(BUFFER+0x3000)) if operation==4 else '-')


def main():
    """독립 두 정밀도 결과와 현재 소스/원문/관측 파일의 정확한 바이트를 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs'];print('에너지 공급 SHA 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 기대 결과를 재구현하지 않고 실제 정상 반환한 두 실행만 저장한다.
    for ordinal,case in enumerate(cases()):
        result=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))]
        if result[0]!=result[1]:different.append(ordinal)
        row,available,required=case;rows.append((','.join(map(str,row)),available.encode().hex() or '-',required.encode().hex() or '-',*result[0],*(['-']*6 if result[0]==result[1] else result[1])))
    FIXTURE.write_text('# input30 available required return output remainder selectedCount selected30 take; second precision\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    tables=[]
    # 기존83VT 목록의 각+a0를 실제 보호 PE에서 읽어 새 분배 표와 정확히 비교한다.
    for line in (ROOT/'cpppj/tests/fixtures/unpop-vtables.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('originals\t'):
            table=int(line.split('\t')[1]);entry=oracles[0].get(table+0xa0);assert entry in (0x41c0f0,0x41c170);tables.append((table,entry))
    assert len(tables)==83
    metadata=(ROOT/'cpppj/src/o/EnergyProviderVtables.inc').read_text(encoding='utf-8')
    assert [(int(table,16),int(entry,16)) for table,entry in re.findall(r'\{(0x[0-9a-f]+)U,(0x[0-9a-f]+)U\}',metadata)]==tables
    VTABLES.write_text('# 실제 PATCH VT +a0 DWORD\n'+''.join(f'{table}\t{entry}\n' for table,entry in tables),encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_energy_supply.py','cpppj/tests/fixtures/unpop-vtables.tsv','cpppj/src/o/EnergyProviderVtables.inc',VTABLES.relative_to(ROOT).as_posix(),FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=[f'{entry:08x}' for entry in ENTRIES],uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),instructions=sum(oracle.instructions for oracle in oracles),actualEntries=dict(sum((oracle.calls for oracle in oracles),Counter())),precisionDifferentInputs=different,sha256={path:digest(ROOT/path) for path in paths})
    facts['limitations']=['수집/문자 대응/배치/소비/공급 두 가상/TYPE 조회/발자국/클램프/toupper는 실제 명령이다.','OS 스레드 locale 조회만 보호 PE의 현재 C locale 포인터를 공급하는 명시 경계다.','독립 명시 raw/TYPE/문자열/목록/전역 입력이며 실제 GUI 또는 사용자 플레이 증거는 아니다.']
    facts['providerVtables']=[{'vtable':f'{table:08x}','entry':f'{entry:08x}'} for table,entry in tables]
    facts['originalListCapacities']={'providers':256,'lastSources':30,'providerInitializer':'004fcb60','lastSourceInitializer':'004fcb80'}
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('에너지 공급:',len(rows),'고유 입력, 정상 반환',facts['normalReturns'],'정밀도 차이',len(different))


if __name__=='__main__':main()
