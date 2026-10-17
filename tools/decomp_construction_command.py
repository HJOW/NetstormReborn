#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 전체 로컬 배치 요청25바이트와 서버444af0을 CPU 메모리에서 독립 관찰한다."""
import argparse,itertools,json,struct,hashlib,bisect
from pathlib import Path
from unicorn.x86_const import *
from decomp_mana_bolt_arrival import Oracle as MemoryOracle,ROOT,TYPES,POOL,STOP,STACK
from decomp_constructionlocal_oracle import LocalOracle,local_case,PLAIN,CONTROLS

# 새 관측은 기존 로컬 배치/Builder fixture와 구별한다.
WRITE=ROOT/'cpppj/tests/fixtures/construction-command-write-x86.tsv'
RECEIVE=ROOT/'cpppj/tests/fixtures/construction-command-receive-x86.tsv'
REPORT=ROOT/'cpppj/construction-command.json'
PACKET=POOL+0x180000
KEYS=('blocked','charge','sender','local','money','cost','result','mutation')

def sha(p):
    """현재 파일의 원문 SHA256이다."""
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

class Writer(LocalOracle):
    """실제4433b0/40a7c0/4df230이 만든 머리와21바이트 본문을 함께 관찰한다."""
    def stub_send(self):
        """네트워크 경계에서25바이트 전체를 복사한다."""
        target,message=self.args(2);assert target==9
        self.packet=bytes(self.mu.mem_read(message,25));super().stub_send()

class Receiver(MemoryOracle):
    """실제444af0을 실행하며 타입/Money/Request/부족/환불만 입력 경계로 둔다."""
    def instruction(self,mu,address,size,data):
        """경계 인자는 실제 원본 스택에서 읽고 원본의 cdecl 반환을 유지한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if STOP+0x800<=address<STOP+0x807:self.instructions+=1;return
        if address in (0x49a840,0x40ef30,0x442b50,0x444760,0x442af0):
            self.instructions+=1;self.calls[hex(address)]+=1
            if address==0x49a840:
                typ=struct.unpack('<I',mu.mem_read(sp+4,4))[0];self.events.append(f'T:{typ}');mu.reg_write(UC_X86_REG_EAX,TYPES+typ*500)
            elif address==0x40ef30:
                owner=struct.unpack('<I',mu.mem_read(sp+4,4))[0];self.events.append(f'M:{owner}');self.put(POOL+0x190000,self.case['money'])
                if self.case['mutation']==1:
                    self.put(TYPES+100*500+0xc4,0x3f800000);self.put(PACKET+4,101,1);self.put(PACKET+5,0xc0500000)
                mu.reg_write(UC_X86_REG_EIP,STOP+0x800);return
            else:
                count=10 if address==0x444760 else 6;values=struct.unpack('<'+'I'*count,mu.mem_read(sp+4,count*4));label={0x442b50:'S',0x444760:'Q',0x442af0:'R'}[address]
                self.events.append(label+':'+':'.join(map(str,values)))
                if address==0x444760:
                    if self.case['mutation']==2:self.put(PACKET+4,101,1);self.put(PACKET+9,0xc0200000);self.put(PACKET+13,0x1234,2);self.put(PACKET+15,9,1)
                    mu.reg_write(UC_X86_REG_EAX,self.case['result'])
            self.return_stub();return
        # 전체 정밀 함수 표의 범위 검사만 재사용하고 마나 볼트 효과 stubs는 사용하지 않는다.
        self.instructions+=1;index=bisect.bisect_right(self.starts,address)-1
        if index<0 or address+size>self.ranges[index][1]:raise RuntimeError(f'미허용 실행 {address:08x}')

    def run(self,case,control):
        """세 cdecl 입력과 x87 제어/빈 스택 및 보존 레지스터를 두 정밀도로 확인한다."""
        self.case=case;self.events=[];mu=self.mu
        mu.mem_write(STOP+0x800,b'\xd9\x05'+struct.pack('<I',POOL+0x190000)+b'\xc3')
        packet=bytearray.fromhex('013e1900640000a4410000aa41efbefa80efcdab890000f87f')
        assert len(packet)==25
        mu.mem_write(PACKET,bytes(packet));self.put(TYPES+100*500+0xc4,case['cost']);self.put(0x558e74,case['blocked']);self.put(0x594fa4,case['charge']);self.put(0x540c70,case['local']);self.put(0x595344,1)
        preserved={UC_X86_REG_EBX:0x11223344,UC_X86_REG_ESI:0x22334455,UC_X86_REG_EDI:0x33445566,UC_X86_REG_EBP:0x44556677}
        # cdecl이 보존해야 하는 레지스터에 서로 다른 입력을 넣는다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_FPTAG,0xffff);mu.reg_write(UC_X86_REG_ESP,STACK)
        mu.mem_write(STACK,struct.pack('<4I',STOP,case['sender'],0xaabbccdd,PACKET));mu.emu_start(0x444af0,STOP,count=20000)
        assert mu.reg_read(UC_X86_REG_ESP)==STACK+4 and mu.reg_read(UC_X86_REG_FPCW)==control
        assert ((mu.reg_read(UC_X86_REG_FPSW)>>11)&7)==0
        assert all(mu.reg_read(reg)==value for reg,value in preserved.items())
        return ';'.join(self.events) or '-'

def inputs():
    """동일 소유자/차단/부족/NaN/실패/경계 뒤 재조회의 입력 조합이다."""
    cases=[]
    # 로컬/다른 DWORD sender와 비용 선검사 경계를 교차한다.
    for blocked,charge,sender,money,result in itertools.product((0,1),(0,1),(1,2,0xffffffff),(0x43790000,0x437a0000,0x7fc12345),(0,1)):
        cases.append(dict(blocked=blocked,charge=charge,sender=sender,local=1,money=money,cost=0x437a0000,result=result,mutation=0))
    # NaN/음수와 Money·Request 뒤의 현재 원문 변경도 관찰한다.
    for money,cost,result,mutation in itertools.product((0,0x43790000,0x7fc12345),(0,0x437a0000,0x7fc54321,0xbf800000),(0,1),(0,1,2)):
        cases.append(dict(blocked=0,charge=0,sender=2,local=1,money=money,cost=cost,result=result,mutation=mutation))
    return cases

def generate():
    """기대 분기를 계산하지 않고 두 실제 실행 결과가 같을 때만 새 자료에 기록한다."""
    writer=Writer('originals');written=[]
    # 실제 원본 로컬 배치가 각 필드를 기록하도록 입력을 교차한다.
    for typ,x,y,arg,direction,quality in itertools.product((100,101),(0x41a40000,0x80000000),(0x41aa0000,0x3f800000),(0,0x12345678),(0,0x1ff),(0,255)):
        case=local_case(type=typ,x=x,y=y,argument=arg,direction=direction,quality=quality)
        observed=[]
        # 같은 입력을 두 실제 x87 정밀도로 실행한다.
        for control in CONTROLS:writer.local(case,control);observed.append(writer.packet)
        assert observed[0]==observed[1]
        written.append([typ,x,y,arg,direction,quality,observed[0].hex()])
    oracle=Receiver();received=[]
    # 수신 분기의 기대값은 두 원본 실행에서만 얻는다.
    for case in inputs():
        observed=[oracle.run(case,control) for control in CONTROLS];assert observed[0]==observed[1]
        received.append([*[case[k] for k in KEYS],observed[0]])
    # 새 관측은 UTF-8/LF의 별도 두 fixture로 저장한다.
    for file,header,rows in ((WRITE,'type\tx\ty\targument\tdirection\tquality\tpacket',written),(RECEIVE,'\t'.join(KEYS)+'\tevents',received)):
        file.write_text(header+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    names=['originals/Netstorm.exe','tools/decomp_construction_command.py','tools/decomp_constructionlocal_oracle.py','tools/decomp_constructionplace_oracle.py','tools/decomp_damageablepredestroy_oracle.py','tools/decomp_priestgeometry_oracle.py','tools/decomp_owner_oracle.py','tools/decomp_mana_bolt_arrival.py','tools/decomp_constructionarrival_oracle.py','extracted/refined/originals/functions.tsv','extracted/constructionlocal/originals/functions.tsv','extracted/constructionlocal/originals/creation.c','extracted/constructionplace/originals/functions.tsv','extracted/constructionplace/originals/creation.c','cpppj/tests/fixtures/construction-command-write-x86.tsv','cpppj/tests/fixtures/construction-command-receive-x86.tsv']
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-17',unit=133,lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),controls=list(CONTROLS),writeRows=len(written),receiveRows=len(received),precisionDifferences=0,normalReturns=2*(len(written)+len(received)),receiveInstructions=oracle.instructions,receiveBoundaries=dict(oracle.calls),source_sha256={n:sha(ROOT/n) for n in names},boundaries=['실제4433b0/40a7c0/4df230의 전체25바이트','실제444af0; 타입/Money/Request/부족/환불은 명시 경계','두 정밀도 ABI/보존 레지스터/x87 균형; OS/GUI/게임 호출0']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('native',len(written),len(received),'returns',2*(len(written)+len(received)),'precision differences 0')

def verify():
    """저장 관측/원본/실행기 SHA와 행 수를 기대 동작 재계산 없이 확인한다."""
    report=json.loads(REPORT.read_text('utf-8'))
    # 실제 원본/관측/실행기와 현재 제품의 SHA를 각각 검증한다.
    for group in ('source_sha256','currentSources'):
        # 생성 후 제품 연결이 있으면 그 바이트도 같은 기준으로 검사한다.
        for n,wanted in report.get(group,{}).items():assert sha(ROOT/n)==wanted,n
    assert len(WRITE.read_text('utf-8').splitlines())-1==report['writeRows']
    assert len(RECEIVE.read_text('utf-8').splitlines())-1==report['receiveRows']
    assert report['normalReturns']==2*(report['writeRows']+report['receiveRows']) and report['precisionDifferences']==0
    print('construction command verified',report['writeRows'],report['receiveRows'])

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
