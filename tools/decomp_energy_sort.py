#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제474970의 거리 정렬과473570의 연속 소비를 같은 선택 메모리에서 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_energy_supply import Oracle as EnergyOracle, ROOT, STACK, STOP, BUFFER, LIST, POOL, bits, digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP

ENTRIES=(0x474970,0x41cfc0)  # 정렬과 실제제곱근 거리 몸체이며 대체하지 않는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/energy-sort-x86.tsv'  # 전체30칸/단계/반환·소비/진단의 두 정밀도 결과다.
REPORT=ROOT/'cpppj/energy-sort.json'  # 정상 반환/실제 실행/의존 바이트의 현재 기록이다.


def cases():
    """위치·용량·문자 길이·무효 슬롯만 입력하며 정렬 기대 순서는 계산하지 않는다."""
    result=[]
    # 초기 위치는 물리30슬롯 전체의 명시 float 입력이다.
    for capacity,count,demand,profile in itertools.product((0,1,3,30),(-1,0,1,3,12,30),('', 'x','wrt','abcdefghij','12345678901','x'*30,'ab\0c'),range(5)):
        if count>capacity:continue
        points=[]
        # 증가/감소/동점/999경계/float로 저장하는 거리 임계값을 방문한다.
        for index in range(30):
            x,y=((101+index,110),(130-index,110),(101,110),(1099+(index%3-1)*.0001,110),(100+index*.000003,110+index*.000001))[profile];points.extend((bits(x),bits(y)))
        row=[0,capacity,count,0,0,0,bits(100),bits(110),*points];result.append((row,demand))
    # 0/free/무효 SID의 보고 후 계속 처리와 원소를 다시 거르지 않는 동작이다.
    for operation,zeros,free,invalid in itertools.product((0,1,2),(0,0x15555555,0x3fffffff),(0,0x15),(0,0x2)):
        points=[value for index in range(30) for value in (bits(120-index),bits(110+index%2))];result.append(([operation,30,12,zeros,free,invalid,bits(100),bits(110),*points],'WrTs..xyzABCDEF'))
    # signed 음수 count의 Take는 원본처럼 빈 조회이며 목록을 고치지 않는다.
    for operation,count in itertools.product((0,1,2),(0x80000000,0xffffffff)):
        result.append(([operation,30,count,0,0,0,bits(100),bits(110),*([bits(101),bits(110)]*30)],'abc'))
    return list(dict.fromkeys((tuple(value&0xffffffff for value in row),demand) for row,demand in result))


class Oracle(EnergyOracle):
    """기존 격리 VM/ABI만 재사용하며 new/free/보고만 명시 외부 효과로 기록한다."""
    def __init__(self):
        super().__init__();self.diagnostics=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 정밀 원문의 불연속 범위를 그대로 추가한다.
        for entry in ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """할당/반납/원본 진단 외의 정렬·거리·소비는 실제 몸체다."""
        if address in (0x4e4391,0x4e4354,0x4e0620):
            esp=mu.reg_read(UC_X86_REG_ESP);ret=self.get(esp)
            if address==0x4e4391:assert self.get(esp+4)==40;mu.reg_write(UC_X86_REG_EAX,BUFFER+0x5000);self.calls['new40-boundary']+=1
            elif address==0x4e4354:assert self.get(esp+4)==BUFFER+0x5000;self.calls['free40-boundary']+=1
            else:
                line=self.get(esp+12);assert line in (0x141,0x150);self.diagnostics.append(str(line));self.calls['assertion-boundary']+=1
            mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        super().instruction(mu,address,size,data)

    def sample(self,case,precision):
        """같은 선택 상태에서 정렬 직후와 연속 소비 후 전체 슬롯을 읽는다."""
        row,demand=case;mu=self.mu;self.diagnostics=[]
        self.put(0,0);self.put(0x5c8464,POOL);self.put(0x5c847c,24000);self.put(0x592f4c,LIST+0x1000);self.put(0x592f50,row[1]);self.put(0x592f54,row[2]);mu.mem_write(BUFFER,demand.encode()+b'\0')
        # 물리 영역30칸은 용량과 무관하게 초기 입력을 보존한다.
        for index in range(30):
            sid=10+index;value=0 if row[3]>>index&1 else 24001 if row[5]>>index&1 else sid;self.put(LIST+0x1000+index*4,value)
            raw=bytearray(b'\xa5'*50);raw[11]=1 if row[4]>>index&1 else 0;struct.pack_into('<II',raw,14,*row[8+index*2:10+index*2]);mu.mem_write(POOL+sid*50,bytes(raw))
        snapshots=[];taken=[]
        if row[0]!=1:
            self.call(0x474970,BUFFER,struct.pack('<II',row[6],row[7]),8,precision);self.returns+=1;snapshots.append(str(self.get(0x592f54))+':'+bytes(mu.mem_read(LIST+0x1000,120)).hex())
        if row[0] in (1,2):
            # 호출 횟수도 입력 계약으로 고정하고0 반환 뒤 쓰기도 읽는다.
            for _ in range(32):
                self.put(BUFFER+0x3000,0xabcdef);self.call(0x473570,0,struct.pack('<I',BUFFER+0x3000),4,precision);self.returns+=1;taken.append(f'{mu.reg_read(UC_X86_REG_EAX)}:{self.get(BUFFER+0x3000)}')
        return (str(self.get(0x592f54)),bytes(mu.mem_read(LIST+0x1000,120)).hex(),','.join(self.diagnostics) or '-',','.join(taken) or '-',','.join(snapshots) or '-')


def main():
    """두 정밀도의 전체 실제 결과와 현재 제품/원문 의존을 별도 파일로 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs'];print('에너지 정렬 SHA 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 각 실제 ABI 정상 반환과 전체 저장소를 독립 실행에서 얻는다.
    for index,case in enumerate(cases()):
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))]
        if outputs[0]!=outputs[1]:different.append(index)
        row,demand=case;rows.append((','.join(map(str,row)),demand.encode().hex() or '-',*outputs[0],*(['-']*5 if outputs[0]==outputs[1] else outputs[1])))
    FIXTURE.write_text('# input68 demand count selected30 diagnostics taken snapshots; second precision\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_energy_supply.py','tools/decomp_energy_sort.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=['00474970','0041cfc0','00473570'],uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),instructions=sum(oracle.instructions for oracle in oracles),actualEntries=dict(sum((oracle.calls for oracle in oracles),Counter())),precisionDifferentInputs=different,sha256={path:digest(ROOT/path) for path in paths},limitations=['정렬/제곱근 거리/선택 소비는 실제 명령이며 메모리 확보40/반납/보고만 명시 경계다.','할당은 성공 입력이다. 할당 실패 뒤 원본의NULL 역참조는 정상 반환 근거가 아니다.','독립 입력 관찰이며 실제 GUI/사용자 플레이 증거가 아니다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('에너지 정렬:',len(rows),'입력, 정상 반환',facts['normalReturns'],'정밀도 차이',len(different))


if __name__=='__main__':main()
