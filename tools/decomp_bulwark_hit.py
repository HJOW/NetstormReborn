#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""불워크48ef20의 비행체 관문/인수 전달/반환DWORD를 보호PE에서 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import pefile
from decomp_arc_spire import Oracle as BaseOracle,ROOT,STOP,POOL,TYPES,VTABLE
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

ENTRIES=(0x48ef20,0x4ac550,0x49a840)  # 실제관문과현재TYPE조회전체다.
FIXTURE=ROOT/'cpppj/tests/fixtures/bulwark-hit-x86.tsv'  # 기존일반피해기대값과분리한새원문이다.
REPORT=ROOT/'cpppj/bulwark-hit-20261023.json'  # 호스트/실행/정밀도/PE근거다.


def cases():
    """공격자0/현재genus/생존·extra무시와원래인수/반환폭을교차한다."""
    rows=[]
    for attacker,genus,state,extra in itertools.product((0,60),(0,1,0x20000,0x100000,0x100200,0xffffffff),(0,1,2,4,8),(0,1,9,32)):rows.append((attacker,144,genus,state,extra,41,2,1))  # 공중운반체와비행체비트를구분한다.
    for amount,owner,result,source in itertools.product((0,1,0xffffffff,0x80000000,0x7fffffff),(0,1,8,255,0xffffffff),(0,1,0xdeadbeef),(0,1,2)):rows.append((0 if source==0 else 60,133,0x100000 if source==2 else 0x50100,0,0,amount,owner,result))  # 일반호출의반환을bool로좁히지않는다.
    for kind,genus in itertools.product((0,73,79,111,133,138,141,144,146,187),(0,0x100000,0x20000)):rows.append((60,kind,genus,0,0,41,2,1))  # 이름이아닌현재TYPE메타데이터를검사한다.
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """원문몸체는실행하고이미별도검증된큰일반피해만명시경계로분리한다."""
    def __init__(self):
        """동일한보호이미지와분리풀/읽기·쓰기/호출가드를설치한다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES)

    def instruction(self,mu,address,size,data):
        """가상98의순서/분기를대체하지않고일반피해요청과부수효과만기록한다."""
        self.instructions+=1
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x44c8e0:
            sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);a=[self.get(sp+4+i*4) for i in range(3)];self.event('H',(ecx-POOL)//50,*a)
            self.put(ecx+26,(0x12345678-a[0])&0xffffffff);self.put(ecx+34,a[1]&255,1)
            if a[2]:self.put(POOL+a[2]*50+10,70,1)
            self.ret(self.result,12);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'불워크 미검토명령 {address:x}')

    def sample(self,values,precision):
        """명시입력으로두슬롯과현재TYPE을설치하고정상반환전체를기록한다."""
        attacker,kind,genus,state,extra,amount,owner,self.result=values;self.events=[];self.mu.mem_write(TYPES,bytes(0x20000))
        for sid in (50,60):  # 피해자와공격자의원래50바이트다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,0x50ea08 if sid==50 else VTABLE);raw[10]=79 if sid==50 else kind;raw[34]=2 if sid==50 else 1
            if sid==50:struct.pack_into('<I',raw,26,0x12345678)
            else:raw[11]=state;raw[40]=extra
            self.mu.mem_write(POOL+sid*50,bytes(raw))
        for address,value in ((0x5c8464,POOL),(0x5c847c,128),(0x59ab20,TYPES),(0x541348,188)):self.put(address,value)  # 같은현재원본전역이다.
        self.put(TYPES+kind*500+0xec,genus);self.call(0x48ef20,POOL+50*50,struct.pack('<3I',amount,owner,attacker),12,precision);self.returns+=1
        return ';'.join(self.events) or '-', ''.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in (50,60)),str(self.mu.reg_read(UC_X86_REG_EAX))


def main():
    """새관측을동결하거나원문/제품과고유행의SHA를확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**report['sha256'],**report.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 현재바이트만확인하며기대값을만들지않는다.
        rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==report['uniqueInputs'] and len({row[0] for row in rows})==len(rows);print('불워크 원문 SHA:',len(rows));return
    oracles=(Oracle(),Oracle());rows=[]
    for values in cases():  # 두x87정밀도의효과/슬롯/반환전체가같아야한다.
        first,second=[o.sample(values,p) for o,p in zip(oracles,(0x27f,0x37f))];assert first==second,values;rows.append((','.join(map(str,values)),*first))
    FIXTURE.write_text('# input effects raws2 result\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_bulwark_hit.py','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix());pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
    report=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),nativeInstructions=sum(o.instructions for o in oracles),precisionDifferentInputs=[],nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),vtableEntries={f'{b:08x}':{f'{off:02x}':f'{int.from_bytes(pe.get_data(b-0x400000+off,4),"little"):08x}' for off in (0x5c,0x98,0x9c)} for b in (0x50ea08,0x50ead0)},sha256={p:digest(ROOT/p) for p in paths},limitations=['48ef20/4ac550/49a840의전체몸체를실행한다.','일반44c8e0은기존원문대조몸체의명시경계이며여기서는인수전달/현재상태변경/반환폭을검사한다. 기존일반피해/83행fixture는바꾸지않는다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('불워크:',len(rows),'입력/두정밀도차이0')


if __name__=='__main__':main()
