#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초CoreRefresh논리span정지와별도로실제미매핑guard경계store를제품전에관측한다."""
import argparse
import json
import socket
from collections import Counter
import decomp_player_core_refresh as native
from decomp_priestmenu import ROOT, STACK, STOP, PLAYERS, digest
from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_FPCW

# 한page뒤를미매핑으로두며두Core의존재하는prefix만실제메모리로만든다.
PAGES = (0x18000000,0x18002000)
FIXTURE = ROOT/'cpppj/tests/fixtures/player-core-refresh-guard-x86.tsv'
REPORT = ROOT/'extracted/cpp-player-core-refresh-guard-20261024-native.json'


class GuardOracle(native.Oracle):
    """메모리hook논리정지가아닌실제미매핑접근에서CPU오류를기록한다."""
    def __init__(self):
        """원문같은몸체와각한page의실제Core물리만추가한다."""
        super().__init__();self.guard_fault=False
        for base in PAGES:self.mu.mem_map(base,4096)

    def read(self,*args):
        """짧은길이의논리hook중단을하지않고실제페이지보호에맡긴다."""
        return

    def write(self,*args):
        """store도논리hook에서멈추지않아실제미매핑오류만관측한다."""
        return

    def invalid(self,mu,access,address,size,value,data):
        """허용한두guard page밖접근만fault로기록한다."""
        if any(base+4096<=address<base+8192 for base in PAGES):
            self.guard_fault=True;self.status='fault-guard';mu.emu_stop();return False
        return super().invalid(mu,access,address,size,value,data)

    def sample_guard(self,length,kind,control):
        """같은초기Coreprefix와실제페이지끝배치를독립입력으로사용한다."""
        native.CORE_A=PAGES[0]+4096-length;native.CORE_B=PAGES[1]+4096-length
        self.case=(0,0,0,31,-1,0,0,1,152,0,0,kind,length,'none');self.trace=[];self.calls=Counter();self.level_calls=Counter();self.reentry=[];self.status='normal';self.instructions=0;self.running=False;self.guard_fault=False
        for block,base in enumerate(PAGES):
            self.mu.mem_write(base,bytes([0x5a+block])*4096)
            if length:self.mu.mem_write(base+4096-length,bytes((0x31+block*29+index*11)&255 for index in range(length)))
        self.mu.mem_write(native.CACHED,bytes([0x6b])*16)
        for address,value in ((native.CURRENT,(native.CORE_A,native.CORE_B)[kind]),(native.METAMAP,0),(0x595344,0),(0x59534c,PLAYERS),(native.OWNER,1),(native.POOL_GLOBAL,native.POOLS[0]),(native.CAPACITY,0),(0x5e4794,0),(0x540bc4,1)):
            self.put(address,value)
        self.mu.mem_write(native.POOLS[0],bytes(400));self.mu.mem_write(STACK-8192,bytes(8196));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.put(STACK,STOP);self.running=True
        try:self.mu.emu_start(0x44fc00,STOP)
        except UcError:
            if not self.guard_fault:raise
        self.running=False
        if self.status=='normal' and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('guard원문스택복귀')
        prefixes=b''.join(bytes(self.mu.mem_read(base+4096-16,16)) for base in PAGES)
        return '\t'.join((str(control),f'{length},{kind}',self.status,prefixes.hex(),';'.join(self.trace),str(self.instructions)))


def main():
    """최초논리중단자료를보존하고추가guard입력만최초한번고정한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,expected in report['sha256'].items():assert digest(ROOT/path)==expected,path
        print(json.dumps({'verified':True,'observations':report['observations']}));return
    assert not FIXTURE.exists() and not REPORT.exists(),'guard최초자료덮어쓰기금지'
    assert not (ROOT/'cpppj/src/o/RawPlayerCoreRefresh.cpp').exists(),'제품작성전guard관측요건'
    oracle=GuardOracle();lines=['# control / coreLength,currentKind / status / twoPageTail16 / trace / instructions'];statuses=Counter()
    for length in (0,5,6,9,10):
        for kind in (0,1):
            first,second=oracle.sample_guard(length,kind,0x27f),oracle.sample_guard(length,kind,0x37f)
            assert first.split('\t')[1:]==second.split('\t')[1:]
            for line in (first,second):lines.append(line);statuses[line.split('\t')[2]]+=1
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','tools/decomp_priestmenu.py','tools/decomp_player_core_refresh.py','tools/decomp_player_core_refresh_guard.py','cpppj/tests/fixtures/player-core-refresh-x86.tsv','cpppj/tests/fixtures/player-core-refresh-guard-x86.tsv')
    report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','sourceAbsentBeforeObservation':True,'observations':len(lines)-1,'statuses':dict(statuses),'instructions':oracle.total,'scope':'초기642중논리길이mem-write정지24행은Unicorn store커밋때문에진짜물리fault동일대조에서제외한다. 이별도20행은A/B의실제미매핑page끝입력이다. 고정캐시C의임의짧은span은별도host계약이며Nativepagefault라고하지않는다.','sha256':{path:digest(ROOT/path) for path in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({'observations':report['observations'],'statuses':dict(statuses),'instructions':oracle.total}))


if __name__=='__main__':main()
