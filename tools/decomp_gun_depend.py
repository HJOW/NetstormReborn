#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Gun 감시 wrapper와 ProcessForm34/PID getter의 인자 순서를 독립 실행한다."""
import csv
import itertools
import json
import struct
from decomp_owner_oracle import OwnerOracle, ROOT, PLAYERS, TARGET, STOP, STACK, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

SPEC = {  # 두 PE의 실제 wrapper/notify/Kernel 표와 server 전역이다.
    'originals': dict(add=0x4b15f0,pair=0x4b1510,notify=0x4ae280,table=0x56b808,server=0x540bc0),
    'originalCD': dict(add=0x434aa0,pair=0x4af940,notify=0x4af490,table=0x5b7bb0,server=0x540a28),
}
OBSERVER = STOP+0x320  # 실제 가상notify 호출의 명시 외부 관찰 경계다.
VTABLE = PLAYERS+0x200  # 명시 관찰 프로세스의 가상 표다.
FIELDS = PLAYERS+0x1000  # 실제 Gun outer/inner 필드 입력 저장소다.
PROCESS = PLAYERS+0x5000  # PID별 명시 프로세스 입력 주소의 시작이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/gun-depend-x86.tsv'  # 새 독립 입력과 실제 요청이다.
REPORT = ROOT/'cpppj/gun-depend.json'  # 기존 Form 수명 입력은 재합산하지 않는다.


class GunDependOracle(OwnerOracle):
    """wrapper/가상34/실제 PID 조회는 실행하고 pair 및 내장notify 요청만 관찰한다."""

    def __init__(self, edition):
        """같은 PC의 실제 불연속 함수 범위를 추가한다."""
        super().__init__(edition)
        self.paths=[]  # 이번 추가 export 전체 원문이다.
        # wrapper와 현재 PID getter는 내보낸 몸체 범위 안에서만 실행한다.
        for folder in ('gun-depend','gun-notify'):
            self.paths += [ROOT/f'extracted/{folder}/{edition}/{name}' for name in ('creation.c','functions.tsv')]
            with self.paths[-1].open(encoding='utf-8') as fp:
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'))
                        self.allowed.append((low,high+1))
        self.put(VTABLE+0x1c,OBSERVER)

    def put(self, address, value, width=4):
        """합성 입력의 지정 폭만 격리 메모리에 쓴다."""
        self.mu.mem_write(address,int(value).to_bytes(width,'little'))

    def read(self, address):
        """현재 실제 DWORD를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,4),'little')

    def on_instruction(self, mu, address, size, data):
        """큰 pair 생성/파생notify의 실제 요청만 ABI를 보존하여 기록한다."""
        if address in (SPEC[self.edition]['pair'],OBSERVER):
            sp=mu.reg_read(UC_X86_REG_ESP)
            ret=self.read(sp)
            if address==OBSERVER:
                pid=(mu.reg_read(UC_X86_REG_ECX)-PROCESS)//32
                self.events.append('N:'+','.join(map(str,(pid,self.read(sp+4),self.read(sp+8),self.read(sp+12)))))
                purge=12
            else:
                self.events.append('A:'+','.join(map(str,(self.read(sp+4),self.read(sp+8)))))
                purge=0
            mu.reg_write(UC_X86_REG_EAX,0)
            mu.reg_write(UC_X86_REG_ESP,sp+4+purge)
            mu.reg_write(UC_X86_REG_EIP,ret)
            return
        super().on_instruction(mu,address,size,data)

    def sample(self, kind, server, pid, watched, form):
        """입력만 만들고 실제 normal return 및 요청 순서를 얻는다."""
        self.events=[]
        spec=SPEC[self.edition]
        self.put(spec['server'],server)
        self.mu.mem_write(self.slot(TARGET),bytes(self.stride))
        self.put(self.slot(TARGET)+18,pid)
        if kind=='N' and pid!=0xffffffff:
            self.put(spec['table']+pid*4,PROCESS+pid*32)
            self.put(PROCESS+pid*32,VTABLE)
        self.mu.mem_write(FIELDS,bytes(0x38))
        self.put(FIELDS+0x18,watched,2)
        self.put(FIELDS+0x2c,form)
        args=(watched,form) if kind=='N' else ((watched,FIELDS+0x28) if self.edition=='originals' else ())
        this=self.slot(TARGET) if kind=='N' else FIELDS
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_ECX,this)
        self.mu.emu_start(spec['notify' if kind=='N' else 'add'],STOP,count=2000)
        purge=8 if kind=='N' else 0
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        return ';'.join(self.events) or '-'


def main():
    """최종 판본별 입력/반환/assert와 전체 export SHA를 별도 fixture에 고정한다."""
    cases=[('A',1,1,w,f) for w,f in itertools.product((5,511,32760),(5,511,32760))]
    cases += [('N',s,p,w,f) for s,p,w,f in itertools.product((0,1,0x80000000),(1,5,255),(5,511,32760),(5,32760))]
    cases += [('N',0,0xffffffff,w,f) for w,f in itertools.product((5,511,32760),(5,32760))]
    rows,editions=[],{}  # 새 독립 입력과 현재 정상 반환 보고다.
    # 양 보호 PE를 각각 독립 매핑하여 실제 명령을 실행한다.
    for edition in SPEC:
        oracle=GunDependOracle(edition)
        for values in cases:
            rows.append((edition,*values,oracle.sample(*values)))
        editions[edition]=dict(samples=len(cases),returns=len(cases),assertions=oracle.assertions,
            exports={p.relative_to(ROOT).as_posix():digest(p) for p in oracle.paths},nativeCalls=dict(oracle.native_calls))
        print(edition,len(cases),'정상 반환',flush=True)
    FIXTURE.write_text('# 실제 감시 wrapper/ProcessForm34/PID getter. pair와 파생notify는 명시 요청 경계.\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',uniqueInputs=len(rows),editions=editions,
        fixtureSha=digest(FIXTURE),toolSha=digest(ROOT/'tools/decomp_gun_depend.py'),
        limitations=['합성 Gun/Form/PID 입력. 실제 wrapper/34/현재PID getter를 실행한다.',
            'pair 생성과 파생notify는 명시 외부 요청이다. 기존 Form 수명 관찰은 재합산하지 않는다.',
            'GUI/실제 전투 발사/완주 동일성은 별도 미완료다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':
    main()
