"""실제 10.78 SP 초기화의 32개 할당 입력과 부분 중단을 제품 작성 전에 고정한다."""
import csv
import hashlib
import json
import struct
from pathlib import Path
from decomp_reward_oracle import RewardOracle, ROOT, ARENA, ARENA_SIZE, MALLOC, STACK, UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP

FIXTURE=ROOT/'cpppj/tests/fixtures/sp-initialization-x86.tsv'  # 이전 회전 fixture를 덮어쓰지 않는 새 원문 자료다.
REPORT=ROOT/'cpppj/sp-initialization-native-20261025.json'  # 실행한 원문과 입력/관측을 식별한다.


class AllocationStop(Exception):
    """선택한 operator_new가 정상 반환하기 전 명시적으로 중단한다."""


class InitializationOracle(RewardOracle):
    """기존 PE 로더/허용 쓰기만 재사용하고 계산은 실제 초기화와 GameRandom이 수행한다."""
    def __init__(self):
        """현재 호스트의 정밀 함수표에서 두 실제 몸체의 분리 범위만 허용한다."""
        super().__init__('originals');self.allowed=[];self.entry_names={}
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as handle:
            for row in csv.DictReader(handle,delimiter='\t'):  # 전체 재디컴파일이나 누락된 과거 개별 export에 의존하지 않는다.
                entry=int(row['entry'],16)
                if entry not in (0x40eba0,0x4558c0):continue
                self.entry_names[entry]=f'{entry:08x}'
                for part in row['ranges'].split(';'):  # 중간 정렬 BYTE를 실행 범위로 넓히지 않는다.
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        assert len(self.entry_names)==2
        self.stub_addresses={self.s['stubs']['reseed'],self.s['stubs']['malloc']};g=self.s['g']
        self.writes=[(ARENA,ARENA+ARENA_SIZE),(STACK-0xf000,STACK+0x1000),(g['perm'],g['perm']+64),(g['pools'],g['pools']+128)]
        for name in ('rng','inited','offset','init_copy'):self.writes.append((g[name],g[name]+4))  # 실제 두 몸체의 네 DWORD 쓰기만 허용한다.
    def on_instruction(self,mu,address,size,data):
        """서로 다른 36BYTE 입력을 제공하며 포인터 저장은 원문이 실행하게 한다."""
        if address==self.s['stubs']['malloc']:
            esp=mu.reg_read(UC_X86_REG_ESP)
            assert self.u32(esp+4)==36
            self.allocations+=1
            target=MALLOC+(self.allocations-1)*64
            mu.mem_write(target,bytes((self.allocations*13+byte*7)&255 for byte in range(36)))
            if self.stop and self.allocations==self.before*32+self.stop:raise AllocationStop()
            mu.reg_write(UC_X86_REG_EAX,target);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));mu.reg_write(UC_X86_REG_ESP,esp+4)
            return
        super().on_instruction(mu,address,size,data)

    def observe(self,seed,before,stop,control):
        """정상/중단 뒤 현재 포인터가 가리키는 실제 32배열과 전역을 읽기만 한다."""
        self.seed=seed;self.before=before;self.stop=stop;self.allocations=0;g=self.s['g']
        self.mu.mem_write(ARENA,bytes(ARENA_SIZE))
        for name in ('rng','inited','offset','init_copy'):self.put(g[name],0)  # 명시 입력이며 원문 파일은 바꾸지 않는다.
        self.mu.mem_write(g['perm'],bytes(64));self.mu.mem_write(g['pools'],bytes(128));status='return'
        try:
            for _ in range(before+1):self.call(0x40eba0,[],control)  # 각 명시 재초기화도 원래 진입점을 사용한다.
        except AllocationStop:status='stop'
        pointers=[self.u32(g['pools']+bit*4) for bit in range(32)]
        memory=b''.join(bytes(self.mu.mem_read(pointer,36)) if pointer else bytes(36) for pointer in pointers)
        return [seed,before,stop,control,status,self.allocations,self.u32(g['inited']),self.u32(g['offset']),self.u32(g['init_copy']),self.u32(g['rng']),int(pointers[0]!=0),memory.hex()]


def main():
    """불변 새 fixture만 만들며 두 x87 모드가 같은 입력에서 같은 상태를 돌려주는지 감사한다."""
    assert not FIXTURE.exists() and not REPORT.exists()
    oracle=InitializationOracle();rows=[]
    for seed in (0,42,0xffffffff):  # 초기 상태의 별도 분기와 DWORD 감김을 원문에서 실행한다.
        for before in (0,1):  # 첫 생성과 이미 초기화된 저장소의 명시 재생성을 구별한다.
            for stop in (0,1,16,32):  # 첫/중간/마지막 할당의 미반환 부분 효과다.
                pair=[oracle.observe(seed,before,stop,control) for control in (0x27f,0x37f)]
                assert pair[0][:3]+pair[0][4:]==pair[1][:3]+pair[1][4:]
                rows.extend(pair)
    header=['seed','before','stop','control','status','allocations','initialized','offset','tracking','rng','firstPool','pools']
    with FIXTURE.open('w',encoding='utf-8',newline='') as handle:
        writer=csv.writer(handle,delimiter='\t',lineterminator='\n');writer.writerow(header);writer.writerows(rows)
    references=['originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','extracted/reward/originals/functions.tsv','tools/decomp_reward_oracle.py','tools/decomp_sp_initialization.py',FIXTURE.relative_to(ROOT).as_posix()]
    value=dict(observations=len(rows),normal=sum(row[4]=='return' for row in rows),stopped=sum(row[4]=='stop' for row in rows),precisionDifferences=0,actualEntries=['0040eba0','004558c0'],boundaries=['operator_new 36BYTE','reseed input'],gameExecuted=False,lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',hashes={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in references})
    REPORT.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:value[key] for key in ('observations','normal','stopped','precisionDifferences')}))


if __name__=='__main__':
    main()
