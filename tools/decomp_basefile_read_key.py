#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""BaseFile 생성·디스크/부분 파일 열기·키 교체·부분 읽기를 실제 PE 명령으로 동결한다."""
import argparse
import hashlib
import itertools
import json
import platform
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, TYPES, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EAX

ENTRIES = dict(construct=0x41a5a0, disk=0x41a5b0, subfile=0x41b460, key=0x41a620, read=0x41a7a0)  # 보호10.78의 실제 다섯 몸체다.
RANGES = ((0x41a5a0,0x41a67f),(0x41a7a0,0x41a90d),(0x41b460,0x41b4bf),(0x4e4a40,0x4e4a7d))  # 실제 스택 probe도 포함한다.
SOURCE, NAME, NEWKEY = TEXT+0x3000, TEXT+0x4000, TYPES+0x4000  # 출력 버퍼·키 입력·파일 이름·새 할당은 겹치지 않는다.


class Oracle(MemoryOracle):
    """FILE/할당/진단만 경계로 두고 원문의 전체 버퍼·필드·호출을 기록한다."""
    def instruction(self, mu, address, size, data):
        """CRT에 공급한 명시 입력은 분기 기대값과 독립이며 실제 덜 읽힌 BYTE를 보존한다."""
        if address == STOP:
            self.completed = True
            mu.emu_stop()
            return
        args = self.words(mu.reg_read(UC_X86_REG_ESP)+4,4)
        if address == 0x4e545a:
            self.events.append('O:'+self.string(args[0]).hex()+':'+self.string(args[1]).decode('ascii'))
            if self.scenario == 1:
                for offset, value in ((4,255),(8,19),(12,23)):
                    self.put(POOL+offset,value)  # fopen 중 바뀐 필드와 함수 뒤 store를 구별한다.
            result = PLAYERS if self.mask & (1 << self.opens) else 0
            self.opens += 1
            self.ret(result)
            return
        if address == 0x4e573c:
            self.events.append(f'S:{args[0]}:{args[1]}:{args[2]}')
            self.position = args[1]
            if self.scenario == 2:
                self.put(POOL,0)
            self.ret(0xffffffff if self.scenario == 6 else 0)
            return
        if address == 0x4e566c:
            self.events.append(f'T:{args[0]}')
            if self.scenario == 2:
                self.put(POOL+4,self.words(POOL+4,1)[0] ^ 1)
                self.put(POOL+8,9)
            self.ret(self.position)
            return
        if address == 0x4e586e:
            pointer, length, count, file = args
            if self.operation == 'read' and self.initial_key and self.scratch is None:
                self.scratch = pointer
                mu.mem_write(pointer,bytes((index*11+165)&255 for index in range(4096)))
            read = min(length*count,self.available)
            mu.mem_write(pointer,bytes(((self.position+index)*29+7)&255 for index in range(read)))
            self.events.append(f'R:{file}:{length}:{count}:{read}')
            self.available -= read
            self.position = (self.position+read)&0xffffffff
            if self.scenario == 3:
                self.put(POOL,PLAYERS+64)
                self.put(POOL+16,TYPES+128)
                self.put(POOL+20,5)
            self.ret(count if length and read==length*count else 0)
            return
        if address == 0x4e4354:
            self.events.append(f'F:{args[0]}')
            if self.scenario == 1:
                self.put(POOL+16,TYPES+128)
                self.put(POOL+20,99)
                mu.mem_write(SOURCE,b'z\0')
            self.ret()
            return
        if address == 0x4e4391:
            self.events.append(f'N:{args[0]}')
            self.allocated = args[0]
            mu.mem_write(NEWKEY,bytes([205])*self.allocated)
            if self.scenario == 2:
                mu.mem_write(SOURCE+1,b'\0')
            self.ret(NEWKEY)
            return
        if address == 0x4e0620:
            self.events.append(f'A:{args[2]}')
            if self.scenario == 4 and args[2] == 0x38b:
                self.put(POOL+20,3)
            self.ret(1)
            return
        if not any(low<=address<high for low,high in RANGES):
            raise RuntimeError(f'원문 BaseFile 읽기/키 범위 밖 {address:08x}')

    def run(self, case):
        """고정된 물리와 입력을 설치한 뒤 원문이 실제로 작성한 전체 결과를 반환한다."""
        self.operation,file,flags,base,length,key,key_length,self.position,self.available,offset,size,source,mode,create,self.mask,self.scenario = case
        self.events,self.completed,self.opens,self.allocated,self.scratch = [],False,0,0,None
        self.initial_key = key
        self.mu.mem_write(STACK-0x10000,bytes(0x10100))
        for index,value in enumerate((file,flags,base,length,key,key_length)):
            self.put(POOL+index*4,value)  # 최종 상태는 계산하지 않고 입력 필드만 공급한다.
        self.mu.mem_write(TEXT,bytes((index*7+53)&255 for index in range(size)))
        self.mu.mem_write(SOURCE,bytes(128))
        if source!='null':
            self.mu.mem_write(SOURCE,bytes.fromhex(source))
        self.mu.mem_write(NAME,b'asset\0tail\0')
        self.mu.mem_write(TYPES,b'abc')
        self.mu.mem_write(TYPES+128,b'vwxyz')
        arguments = dict(construct=[],disk=[NAME,mode,create],subfile=[NAME,offset,33],key=[0 if source=='null' else SOURCE],read=[TEXT,size,offset])[self.operation]
        self.put(STACK,STOP)
        for index,value in enumerate(arguments):
            self.put(STACK+4+index*4,value)  # 원문 thiscall의 실제 인자 순서다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_ECX,POOL)
        self.mu.emu_start(ENTRIES[self.operation],STOP+1,count=2000000)
        assert self.completed,case
        result = (self.mu.reg_read(UC_X86_REG_EAX)&255) if self.operation in ('disk','subfile') else None
        return dict(result=result,final=list(self.words(POOL,6)),position=self.position,events=';'.join(self.events) or '-',
            destination=bytes(self.mu.mem_read(TEXT,size)).hex() or '-',scratch=bytes(self.mu.mem_read(self.scratch,4096)).hex() if self.scratch is not None else '-',
            allocation=bytes(self.mu.mem_read(NEWKEY,self.allocated)).hex() or '-',source=bytes(self.mu.mem_read(SOURCE,32)).hex())


def cases():
    """읽기 길이/실제 공급량·열기 모드·키 소유와 콜백 변이를 출력과 독립적으로 구성한다."""
    def case(operation,flags=0x81,key=TYPES,key_length=3,available=10000,offset=0xffffffff,size=0,source='6162636400',mode=0,create=0,mask=0,scenario=0):
        """원문 필드와 장치 입력 한 행이며 파일/부분 시작/길이는 고정된 canary다."""
        return [operation,PLAYERS,flags,7,9,key,key_length,15,available,offset,size,source,mode,create,mask,scenario]
    result = [case('construct')]
    result += [case('disk',mode=m,create=c,mask=a,scenario=s) for m,c,a,s in itertools.product((0,1,2),(0,1),(0,1,2),(0,1))]
    result += [case('subfile',offset=o,mask=m,scenario=s) for o,m,s in itertools.product((0,7,0xfffffffb),(0,1),(0,2,6))]
    result += [case('key',key=k,source=t,scenario=s) for k,t,s in itertools.product((0,TYPES),('null','00','6100','6162636400','6d79646f67686173666c65617300'),(0,1,2))]
    result += [case('read',flags=f,key=k,size=n,available=a,offset=o) for f,k,n,a,o in itertools.product((0x80,0x81),(0,TYPES),(0,1,4095,4096,4097,8193),(0,3,10000),(0xffffffff,2))]
    result += [case('read',flags=f,size=4097,available=3,scenario=s,key_length=0 if s==4 else 3) for f,s in itertools.product((0x80,0x81),(2,3,4))]
    return result


def main():
    """실제 원문을 두 번 관측하고 새로운 디렉터리에만 결과를 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    out=parser.parse_args().output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    inputs=cases()
    oracle=Oracle()
    results=[oracle.run(case) for case in inputs]
    assert results==[oracle.run(case) for case in inputs]
    with (out/'basefile-read-key-x86.jsonl').open('x',encoding='utf-8',newline='\n') as stream:
        for case,result in zip(inputs,results):
            stream.write(json.dumps(dict(input=case,**result),separators=(',',':'))+'\n')  # 부분 읽기 뒤 버퍼 전체도 원시 그대로 기록한다.
    with (out/'basefile-read-key-x86.tsv').open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# op/file/flags/base/length/key/keyLength/position/available/offset/size/source/mode/create/mask/scenario / result/final6/position/events/destination/scratch/allocation/source32\n')
        for case,result in zip(inputs,results):
            stream.write('\t'.join(map(str,(*case,'-' if result['result'] is None else result['result'],','.join(map(str,result['final'])),result['position'],result['events'],result['destination'],result['scratch'],result['allocation'],result['source'])))+'\n')
    report=dict(unit=652,host=platform.node(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',cases=len(inputs),observations=len(inputs)*2,differences=0,executableSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest(),scope='5개 원문/실제 스택 probe. CRT/할당/진단은 명시 경계. 게임/GUI/통신 실행 없음.')
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
