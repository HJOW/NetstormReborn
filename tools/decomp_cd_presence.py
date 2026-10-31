#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CD 루트004dde40과 캐시 확인00440c60을 실제 PE로 실행하며 OS/경로만 격리한다."""
import argparse
import hashlib
import itertools
import json
import platform
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX
import pefile

CACHE,OUTPUT,PATH=0x558dd4,POOL,TEXT  # 실제 전역 캐시와 독립 출력/경로 물리다.
MARKER=b'cd.netstorm.titanic.activision'  # 보호 PE의 고정 이름이다.


class Oracle(MemoryOracle):
    """루트 필터·길이·캐시·복사 판단은 원문 명령으로 실행한다."""
    def __init__(self):
        """실제 두 Win32 IAT만 격리 주소로 연결하고 다른 OS 진입은 허용하지 않는다."""
        super().__init__()
        self.imports={}
        pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
        for library in pe.DIRECTORY_ENTRY_IMPORT:
            for entry in library.imports:
                name=entry.name.decode() if entry.name else ''
                if name in ('GetLogicalDriveStringsA','GetDriveTypeA'):
                    target=STOP+0x100+len(self.imports)*0x10
                    self.imports[target]=name
                    self.put(entry.address,target)  # Native의 실제 IAT 호출 인자를 그대로 받는다.

    def instruction(self,mu,address,size,data):
        """각 실제 호출 위치에서 현재 BYTE/인자와 명시 콜백 변이를 관측한다."""
        if address==STOP:
            if self.operation=='roots' and self.logical is not None:
                self.logical_snapshot=bytes(mu.mem_read(self.logical,128)).hex()  # 직접 호출은 다음 부모의 stack 재사용 전에 끝난다.
            self.completed=True
            mu.emu_stop()
            return
        if address==0x440cb5 and self.logical is not None:
            self.logical_snapshot=bytes(mu.mem_read(self.logical,128)).hex()  # 반환 직후 관측한다. 뒤 경로 호출이 죽은 하위 stack을 재사용한 BYTE와 구별한다.
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,5)
        if address==0x4dde40 and self.operation=='check':
            assert args[0]==128
            self.output=args[1]
            mu.mem_write(self.output,bytes([205])*128)
        if address in self.imports:
            if self.imports[address]=='GetLogicalDriveStringsA':
                assert args[0]==128
                self.logical=args[1]
                mu.mem_write(self.logical,bytes([205])*128)
                mu.mem_write(self.logical,self.drives)
                self.events.append('L:128')
                self.ret(self.logical_result,8)
            else:
                root=self.string(args[0])
                self.events.append('T:'+root.hex())
                value=self.types[self.type_index]
                self.type_index+=1
                if self.mutation and self.type_index==1:
                    mu.mem_write(args[0],b'Z')
                self.ret(value,4)
            return
        if address==0x459c60:
            assert args[0]==0xfffffffe and args[2]==0xfffffffd and self.string(args[3])==MARKER and args[4]==0
            self.path_root=args[1]
            root=self.string(self.path_root)
            self.events.append('P:'+root.hex())
            path=root+(b'' if root.endswith(b'\\') else b'\\')+MARKER
            mu.mem_write(PATH,path+b'\0')
            self.ret(PATH)
            return
        if address==0x4dd560:
            path=self.string(args[0])
            self.events.append('E:'+path.hex())
            result=(self.mask>>self.probes)&1
            self.probes+=1
            if self.mutation and self.probes==1:
                mu.mem_write(self.path_root,b'Z')
            self.ret(result)
            return
        if not (0x440c60<=address<0x440d38 or 0x4dde40<=address<0x4ddf3c):
            raise RuntimeError(f'CD 판단 원문 범위 밖 {address:08x}')

    def run(self,case):
        """명시 캐시/OS 목록/반환/용량을 설치하고 실제 전체128BYTE를 읽는다."""
        self.operation,cached,drives,types,self.logical_result,capacity,self.mask,self.mutation=case
        self.drives=bytes.fromhex(drives)
        self.types=[] if types=='-' else list(map(int,types.split(',')))
        self.events,self.completed,self.type_index,self.probes=[],False,0,0
        self.output=OUTPUT if self.operation=='roots' else None
        self.logical=None
        self.logical_snapshot='-'  # 호출되지 않은 장치의 stack은 관측하지 않는다.
        mu=self.mu
        mu.mem_write(STACK-0x1000,bytes(0x1100))
        mu.mem_write(CACHE,bytes([205])*128)
        mu.mem_write(CACHE,cached.encode('ascii')+b'\0')
        mu.mem_write(OUTPUT,bytes([205])*128)
        self.put(STACK,STOP)
        if self.operation=='roots':
            self.put(STACK+4,capacity)
            self.put(STACK+8,OUTPUT)
        mu.reg_write(UC_X86_REG_ESP,STACK)
        mu.emu_start(0x4dde40 if self.operation=='roots' else 0x440c60,STOP+1,count=30000)
        assert self.completed,case
        return dict(result=mu.reg_read(UC_X86_REG_EAX),cache=bytes(mu.mem_read(CACHE,128)).hex(),
            output=bytes(mu.mem_read(self.output,128)).hex() if self.output is not None else '-',
            logical=self.logical_snapshot,events=';'.join(self.events) or '-')


def cases():
    """용량/OS 반환/종류/캐시와 명시 변이를 출력과 독립인 교차 입력으로 만든다."""
    specs=((),(('A:\\',3),),(('D:\\',5),),(('A:\\',3),('D:\\',5)),(('D:\\',5),('E:\\',5)),(('A:\\',1),('D:\\',5),('E:\\',5)))
    def fields(spec):
        """실제 OS MULTI_SZ 입력과 종류 순서를 직렬화한다."""
        raw=('\0'.join(root for root,kind in spec)+'\0\0').encode('ascii')
        return raw.hex(),','.join(str(kind) for root,kind in spec) or '-',len(raw)-1
    result=[]
    for spec,capacity,ret,mutation in itertools.product(specs,(0,1,4,5,8,9,128),('normal',0,128,129),(0,1)):
        drives,types,length=fields(spec)
        result.append(['roots','Q:\\',drives,types,length if ret=='normal' else ret,capacity,0,mutation])  # 루트 결과의 엄격 용량 경계를 확인한다.
    for cached,spec,mask,mutation in itertools.product(('','Q:\\','D:\\'),specs[1:],(0,1,2,4,8),(0,1)):
        drives,types,length=fields(spec)
        result.append(['check',cached,drives,types,length,128,mask,mutation])  # 캐시 성공/실패 뒤 실제 선택과 source 재조회를 관측한다.
    for cached,ret in itertools.product(('','Q:\\'),(0,128,129)):
        drives,types,length=fields(specs[4])
        result.append(['check',cached,drives,types,ret,128,0,0])
    return result


def main():
    """한 프로세스 안에서도 전체 재관측을 대조하고 새 폴더에만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    out=parser.parse_args().output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    inputs=cases()
    oracle=Oracle()
    observed=[oracle.run(case) for case in inputs]
    assert observed==[oracle.run(case) for case in inputs]
    with (out/'cd-presence-x86.jsonl').open('x',encoding='utf-8',newline='\n') as stream:
        for case,result in zip(inputs,observed):
            stream.write(json.dumps(dict(input=case,**result),separators=(',',':'))+'\n')  # 원문의 실제 세 물리/호출 전체다.
    with (out/'cd-presence-x86.tsv').open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# op/cache/drives/types/osResult/capacity/existsMask/mutation / result/cache128/output128/logical128/events\n')
        for case,result in zip(inputs,observed):
            encoded=[case[0],case[1].encode('ascii').hex() or '-',*case[2:]]
            stream.write('\t'.join(map(str,(*encoded,result['result'],result['cache'],result['output'],result['logical'],result['events'])))+'\n')
    report=dict(unit=654,host=platform.node(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',cases=len(inputs),observations=len(inputs)*2,differences=0,executableSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest(),scope='실제440c60/4dde40. 두 Win32 IAT·경로·존재만 명시 장치. 논리 목록은 하위 반환 직후에만 관측한다. 명시 stack 초기값이며 게임/GUI/실제 CD/통신 실행 없음.')
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
