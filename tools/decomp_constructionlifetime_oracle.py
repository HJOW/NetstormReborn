#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""표시 등록·건설 소멸자의 전체 몸체를 실제 세 PE에서 격리 실행한다. 게임/OS 실행은 없다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_process_oracle import (ProcessOracle,ROOT,BINARIES,CONTROLS,POOL,digest,
    UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,
    UC_X86_REG_ESP,UC_X86_REG_EIP)

# 격리 객체/목록 두 배열과 반납할 입력 버퍼다. 원본/호스트 주소를 역참조하지 않는다.
DATA=0x14000000
OBJECT,TABLE,NEXT,SOURCE,OUTPUT,PARTICLES=(DATA+offset for offset in (0,0x1000,0x3000,0x5000,0x6000,0x7000))
# 실제 원본 진입점·목록 전역·CRT 확보/반납 경계다.
SPECS={'originals':dict(ctor=0x487fd0,render=0x487880,build=0x422d00,
    deleting=(0x423540,0x423820,0x423820),table=0x595138,count=0x59513c,
    new=0x4e4391,heap=0x4437f0,base_vtable=0x5019ec,z=35),
    'originalCD':dict(ctor=0x45c6c0,render=0x45c840,build=0x4ea360,
    deleting=(0x4ead20,0x4ead40,0x4eadb0),table=0x5209c0,count=0x5209c4,
    new=0x4f1650,heap=0x48ef00,base_vtable=0x500d68,z=33)}
SPECS['original1037']=dict(SPECS['originalCD'])
# 관찰 열 순서·fixture·근거 기록이다. 필드 값은 전부 실제 메모리에서 읽는다.
KEYS=('capacity','pattern','parent','z0','z','mutate','buffers','flags','slot')
OFFSETS=(4,8,0xc,0x10,0x14,0x18,0x1c,0x20)
FIXTURE=ROOT/'cpppj/tests/fixtures/constructionlifetime-x86.tsv'
REPORT=ROOT/'cpppj/constructionlifetime.json'


def case(**changes):
    """등록/반납의 입력만 구성한다. 기대 슬롯이나 삭제 결과는 계산하지 않는다."""
    value=dict(capacity=40,pattern=0,parent=50,z0=128,z=255,mutate=0,buffers=7,flags=0,slot=19)
    value.update(changes);return value


def plan():
    """빈/가득 찬 목록·40칸 확장·부호 있는 깊이·부착 중 변경·버퍼 조합·삭제 flags를 포함한다."""
    rows=[]
    # capacity 0에는 빈 칸 위치를 나누는 의미가 없어 같은 입력을 중복하지 않는다.
    for capacity in (0,1,39,40,80):
        # 첫/중간/마지막 빈 칸과 가득 찬 목록을 독립 입력으로 만든다.
        for pattern,parent,z in itertools.product(range(4) if capacity else (0,),(0,50),(0,127,128,255)):
            rows.append(('Construct',case(capacity=capacity,pattern=pattern,parent=parent,z=z)))
    # 부착 경계에서 현재 부모/깊이를 바꾸면 final 깊이는 진입 부모로 다시 조회한다.
    for mutate in (1,2):rows.append(('Construct',case(mutate=mutate)))
    # 세 버퍼의 null 조합과 scalar deleting destructor의 BYTE bit 0을 모두 관찰한다.
    for operation in ('Render','Build','DeleteBase','DeleteInstant','DeleteNormal'):
        # 홀짝·상위 BYTE·상위 bit·signed DWORD 경계를 포함한다.
        for buffers,flags in itertools.product(range(8),(0,1,2,3,256,257,0x80000000,0xffffffff)):
            rows.append((operation,case(buffers=buffers,flags=flags)))
    rows.append(('Sequence',case(capacity=1)))
    return rows


def initial(c):
    """등록 전의 토큰 배열만 만들며 원본의 슬롯 검색/확장은 구현하지 않는다."""
    slots=[100+i for i in range(c['capacity'])]
    if slots and c['pattern']:
        slots[(0,len(slots)//2,len(slots)-1)[c['pattern']-1]]=0
    return slots


class LifetimeOracle(ProcessOracle):
    """전체 등록/소멸 몸체와 기존 base 소멸자를 실행한다. 부착·new/free만 응답한다."""
    def __init__(self,edition):
        """읽기 전용 내보내기의 실제 명령 범위와 명시 전역 쓰기만 허용한다."""
        super().__init__(edition);self.d=SPECS[edition];self.edition=edition
        self.extra=[ROOT/f'extracted/constructionlifetime/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.extra[1].open(encoding='utf-8') as fp:
            # 불연속 함수 범위만 허용하며 함수 사이 코드는 실행하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                # 내보낸 실제 명령 범위의 양끝을 포함한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(DATA,0x10000);self.writes.append((DATA,DATA+0x10000))
        self.writes.extend((self.d[key],self.d[key]+4) for key in ('table','count'))
        self.boundaries=collections.Counter();self.returns=collections.Counter()

    def args(self,count):
        """현재 호출의 스택 인자만 읽는다."""
        return struct.unpack('<'+'I'*count,self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def ret(self,value=0,purge=0):
        """명시 경계를 원본 규약으로 반환시킨다. 원본 몸체 계산은 대체하지 않는다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);address=self.u32(esp)
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,address)

    def on_instruction(self,mu,address,size,data):
        """목록 확보/반납과 BaseProcess 부착의 this/인자/시점을 검사한다."""
        d=self.d;c=getattr(self,'case',None)
        if address==d['new']:
            self.boundaries['new']+=1;amount=self.args(1)[0]
            if amount!=self.u32(d['count'])*4 or amount>4096:raise RuntimeError('목록 확보량 오류')
            self.events.append(f'A:{amount}');self.ret(NEXT);return
        if address==d['heap']:
            self.boundaries['heap']+=1;self.ret();return
        if address==self.s['stubs']['free']:
            self.boundaries['free']+=1;pointer=self.args(1)[0]
            labels={TABLE:'T',NEXT:'T',SOURCE:'S',OUTPUT:'O',PARTICLES:'P',OBJECT:'B'}
            if pointer not in labels:raise RuntimeError('표시 반납 포인터 오류')
            label=labels[pointer];self.events.append('F:'+label)
            if label in 'SOP' and self.u32(self.u32(d['table'])+self.u32(OBJECT+0x1c)*4)!=OBJECT:
                raise RuntimeError('버퍼 반납 전 등록 제거')
            if label=='B' and self.u32(self.u32(d['table'])+self.u32(OBJECT+0x1c)*4):raise RuntimeError('객체 반납 전 등록 유지')
            self.ret();return
        if address==self.s['base_ctor']:
            self.boundaries['attach']+=1;kind,parent,form,flags=self.args(4)
            slot=self.u32(OBJECT+0x1c);depth=self.u32(OBJECT+0x20)
            if mu.reg_read(UC_X86_REG_ECX)!=OBJECT or (kind,parent,form,flags)!=(17,c['parent'],0,16):raise RuntimeError('표시 부착 인자 오류')
            if self.u32(self.u32(d['table'])+slot*4)!=OBJECT:raise RuntimeError('부착 전 목록 미등록')
            self.events.append(f'B:{kind}:{parent}:{form}:{flags}:{slot}:{depth}')
            self.put(OBJECT+4,7);self.put(OBJECT+8,51 if c['mutate']==2 else parent)
            if c['mutate']==1:self.put(POOL+parent*self.s['stride']+d['z'],128,1)
            self.ret(OBJECT,16);return
        ProcessOracle.on_instruction(self,mu,address,size,data)

    def invoke(self,entry,args=(),purge=0):
        """원본 ABI·레지스터·x87·SEH 복구와 루트 정상 반환 수를 확인한다."""
        result=self.call(entry,args,this=OBJECT,purge=purge)
        if any(self.mu.reg_read(reg) for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)):
            raise RuntimeError('표시 수명 보존 레지스터 오류')
        self.returns[f'{entry:08x}']+=1;return result

    def snapshot(self):
        """목록/공통 필드의 관찰 값만 직렬화하며 객체 포인터를 토큰 1로 정규화한다."""
        pointer=self.u32(self.d['table']);count=self.u32(self.d['count'])
        slots=[self.u32(pointer+i*4) for i in range(count)]
        return [':'.join(str(1 if v==OBJECT else v) for v in slots) or '-',
            ':'.join(str(self.u32(OBJECT+off)) for off in OFFSETS)]

    def run(self,operation,c,control):
        """유한 입력 목록·버퍼를 넣고 원본 전체 등록/소멸 몸체와 연속 재사용을 실행한다."""
        self.begin(dict(server=1,boss=1,rotation=0,now=0),control);self.case=c;d=self.d;mu=self.mu
        mu.mem_write(DATA,bytes(0x10000));self.put(d['count'],c['capacity']);self.put(d['table'],TABLE if c['capacity'] else 0)
        # 목록 토큰은 원본 ctor가 역참조하지 않는 외부 객체 포인터의 정규화 값이다.
        for i,token in enumerate(initial(c)):self.put(TABLE+i*4,token)
        self.put(POOL+d['z'],c['z0'],1);self.put(POOL+50*self.s['stride']+d['z'],c['z'],1)
        self.put(POOL+51*self.s['stride']+d['z'],127,1)
        mu.mem_write(OBJECT,b'\xa5'*136);history=[];result=0
        if operation in ('Construct','Sequence'):
            result=self.invoke(d['ctor'],(c['parent'],17),8)
            if result!=OBJECT:raise RuntimeError('표시 생성 반환 포인터 오류')
            if operation=='Sequence':
                history.append('|'.join(self.snapshot()));self.invoke(d['render']);history.append('|'.join(self.snapshot()))
                result=self.invoke(d['ctor'],(c['parent'],17),8);history.append('|'.join(self.snapshot()))
        else:
            slot=c['slot'];self.put(TABLE+slot*4,OBJECT)
            # 원본 소멸자는 포인터를 지우지 않으므로 입력 필드가 그대로 남는 것도 확인한다.
            for off,value in ((4,7),(8,50),(0x1c,slot),(0x20,0xffffff80),(0x4c,SOURCE if c['buffers']&1 else 0),
                    (0x50,OUTPUT if c['buffers']&2 else 0),(0x2c,PARTICLES if c['buffers']&4 else 0)):
                self.put(OBJECT+off,value)
            if operation=='Render':self.invoke(d['render'])
            elif operation=='Build':self.invoke(d['build'])
            else:
                result=self.invoke(d['deleting'][('DeleteBase','DeleteInstant','DeleteNormal').index(operation)],(c['flags'],),4)
                if result!=OBJECT:raise RuntimeError('표시 삭제 반환 포인터 오류')
            if self.u32(OBJECT)!=d['base_vtable']:raise RuntimeError('base 소멸자 미실행')
        return [';'.join(self.events) or '-',*self.snapshot(),'~'.join(history) or '-',str(1 if result==OBJECT else result)]


def generate(smoke):
    """두 x87 정밀도의 관찰을 비교한 뒤 실제 출력과 SHA/진입/반환/외부 사건을 저장한다."""
    rows=[];editions={};files={Path(__file__),ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/ghidra/constructionlifetime-functions.json',FIXTURE}
    # 세 PE마다 독립 격리 메모리를 사용한다.
    for edition in BINARIES:
        oracle=LifetimeOracle(edition);cases=plan()[::47]+plan()[-3:] if smoke else plan()
        # 기대식을 만들지 않고 실제 명령 관찰 두 개를 비교한다.
        for operation,c in cases:
            observed=[oracle.run(operation,c,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('표시 수명 x87 정밀도 차이')
            rows.append([edition,operation,','.join(str(c[key]) for key in KEYS),*observed[0]])
        editions[edition]=dict(cases=dict(collections.Counter(op for op,_ in cases)),entries=dict(oracle.hits),returns=dict(oracle.returns),boundaries=dict(oracle.boundaries),assertions=oracle.asserts)
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: constructionlifetime {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 표시 등록/삭제·건설 버퍼 소멸 관찰. 부착/new/free만 외부 경계.\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),controls=list(CONTROLS),host='HJOW-Athlon',date='2026-10-11',os_calls=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},limits=[
            'RenderProcess ctor/소멸·Build 공통/파생 scalar 삭제·base 소멸자 전체 몸체, CD 내부 연속 helper 포함',
            '유한 정상 목록·부모 0/50·signed 깊이·부착 중 변경·null 버퍼 조합·flags bit 0·삭제 뒤 첫 구멍 재사용',
            'BaseProcess 부착/메모리 확보/반납만 경계, 실제 Kernel/form 부착 및 게임/장치/OS 실행 없음',
            '잘못된 등록/assert/new 실패/정수 감김/버퍼 alias/경계 중 재진입·임의 메모리 손상은 제외',
            '원본 free 뒤 잔여 포인터는 호스트 vector 해제와 같은 바이트 배치를 요구하지 않으며 반납 순서를 비교']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """저장된 SHA·입력 순서/폭·루트 정상 반환·실제 진입·외부 사건 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 원본 PE/도구/내보내기/fixture의 바이트를 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('표시 수명 SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or set(report['editions'])!=set(BINARIES):raise RuntimeError('표시 수명 총계 오류')
    # 알고리즘을 재계산하지 않고 각 저장 관찰의 폭과 실제 진입/반환을 감사한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];d=SPECS[edition];returns=collections.Counter();boundaries=collections.Counter()
        if len(chosen)!=len(plan()) or item['assertions']:raise RuntimeError('표시 수명 행/assert 오류')
        for row,(operation,c) in zip(chosen,plan(),strict=True):
            if len(row)!=8 or row[1]!=operation or row[2]!=','.join(str(c[key]) for key in KEYS):raise RuntimeError('표시 수명 입력 순서 오류')
            values=[int(v) for v in row[5].split(':')]
            if len(values)!=len(OFFSETS) or any(not 0<=v<=0xffffffff for v in values):raise RuntimeError('표시 수명 필드 폭 오류')
            if row[4]!='-' and any(not 0<=int(v)<=0xffffffff for v in row[4].split(':')):raise RuntimeError('표시 슬롯 폭 오류')
            entries=[d['ctor'],d['render'],d['ctor']] if operation=='Sequence' else [d['ctor'] if operation=='Construct' else d['render'] if operation=='Render' else d['build'] if operation=='Build' else d['deleting'][('DeleteBase','DeleteInstant','DeleteNormal').index(operation)]]
            # 루트별 정상 반환 집계를 두 정밀도로 비교한다.
            for entry in entries:returns[f'{entry:08x}']+=2
            # 외부 사건은 기존 배열/각 버퍼/객체 free를 함께 집계한다.
            for event in (() if row[3]=='-' else row[3].split(';')):boundaries[{'A':'new','F':'free','B':'attach'}[event[0]]]+=2
        boundaries['heap']=boundaries['new']
        if not boundaries['heap']:del boundaries['heap']
        if dict(returns)!=item['returns'] or dict(boundaries)!=item['boundaries'] or item['cases']!=dict(collections.Counter(op for op,_ in plan())):raise RuntimeError('표시 수명 사건/반환 집계 오류')
        for entry,count in returns.items():
            if item['entries'].get(entry,0)<count:raise RuntimeError('표시 수명 실제 루트 진입 부족')
    print(f'constructionlifetime 검증 통과: {len(rows)}개')


def main():
    """전체 관찰 저장·무저장 표본·저장 근거 감사 가운데 하나를 실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
