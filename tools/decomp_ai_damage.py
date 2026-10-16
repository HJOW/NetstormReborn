#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AI 피해 반응의 실제 세 목록 helper와 전체 본체를 두 x87 정밀도로 관찰한다."""
import csv
import itertools
import json
import socket
import struct
import sys
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TYPES, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)

# 실제 AI 등록부와 같은 opaque32 블록 주소·크기 및 네 목록 위치다.
BASE, SIZE, OFFSETS = 0x13000000, 0xb08, (0x360,0x36c,0x378,0x384)
KEYS = ('genus','altar','tracked','guard','mode','hcap','hcount','hdup','vcap','vcount','vdup','flyer','home','homefree','homegroup','seed','attacker')
FIXTURE = ROOT/'cpppj/tests/fixtures/ai-damage-x86.tsv'
REPORT = ROOT/'cpppj/ai-damage.json'


def sample(**values):
    """독립 입력만 만들며 기대 목록 결과는 계산하지 않는다."""
    result = dict(zip(KEYS,(0x200,0,0,0,2,4,0,0,4,0,0,1,100,0,3,19,60)))
    result.update(values)
    return result


def inputs():
    """genus·추적·설정·용량·중복과 홈 범위/free/group을 독립 교차한다."""
    rows=[]
    for genus,tracked,mode,flyer in itertools.product((0,0x200,0x4000,0x200000,0x400000,0x80000000,0x4000000,0x10000,0x20000),(0,1),(0,2),(0,1)):
        rows.append(sample(genus=genus,tracked=tracked,guard=1,mode=mode,flyer=flyer))
    options=((0,0,0),(1,0,0),(1,1,0),(1,1,1),(4,4,0),(4,4,1))
    for hate,very in itertools.product(options,options):
        rows.append(sample(hcap=hate[0],hcount=hate[1],hdup=hate[2],vcap=very[0],vcount=very[1],vdup=very[2]))
    for home,free,group in itertools.product((0,1,100,23999,24000,65535),(0,1,2),(2,3)):
        rows.append(sample(home=home,homefree=free,homegroup=group))
    rows.extend((sample(genus=0,altar=1),sample(attacker=0),sample(genus=0x4000000,guard=0)))
    # CPP 통과147행 뒤 실제 썬더 캐논 genus의 초기 무반응/명시 tracked 반응을 독립 추가한다.
    rows.extend((sample(genus=0x08040010,tracked=0,flyer=0),sample(genus=0x08040010,tracked=1,flyer=0)))
    return list({tuple(row[key] for key in KEYS):row for row in rows}.values())


class Oracle(MemoryOracle):
    """홈 조회와 로그 요청만 경계이며 목록·genus·그룹·좌표 절삭은 실제 명령이다."""
    def __init__(self,control):
        """현재 PC 전체 정련에서 필요한 실제 함수 구간만 허용한다."""
        super().__init__();self.control=control;self.mu.mem_map(BASE,0x100000);self.mu.mem_map(0,0x1000);self.mu.mem_map(POOL+0x100000,0x100000)
        self.allowed=[];self.events=[];self.instructions=0;self.writes=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for entry in (0x415460,0x411200,0x414450,0x40e9d0,0x40ea40,0x4ac200,0x4ac550,0x4ac950,0x49a840,0x4e49c0):
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write_guard)

    def write_guard(self,mu,access,address,size,value,data):
        """실행 중에는 본체·네 목록·현재 스택 이외의 쓰기를 거부한다."""
        if not self.writes:return
        assert any(low<=address and address+size<=high for low,high in self.writes),(hex(address),size)

    def instruction(self,mu,address,size,data):
        """두 실제 외부 경계의 인수·순서만 기록한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,7)
        if address==0x45c100:
            self.events.append('H:'+str(args[0]));self.ret(self.case['home']);return
        if address==0x4105b0:
            assert args[0]==BASE and args[1]==29
            more=int(b'MORE' in self.string(args[2]));typeid=(args[3]-TYPES-4)//500
            sid=self.case['home'] if typeid==73 else 60
            signed=lambda value:value if value<0x80000000 else value-0x100000000
            self.events.append('L:'+':'.join(map(str,(more,sid,typeid,signed(args[4]),signed(args[5])))));self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError(f'검토하지 않은 AI 피해 명령 {address:08x}')
        self.instructions+=1
        if self.instructions>30000:raise RuntimeError('AI 피해 명령 상한 초과')

    def run(self,case):
        """기대값 없이 독립 메모리를 준비한 뒤 전체 출력과 ABI를 수집한다."""
        self.case=case;self.events=[];self.instructions=0;self.writes=[];mu=self.mu
        mu.mem_write(0,bytes(4));mu.mem_write(BASE,bytes((i*13+case['seed'])&255 for i in range(SIZE)))
        for index,offset in enumerate(OFFSETS):
            pointer=BASE+SIZE+index*32;mu.mem_write(pointer,bytes((i*7+case['seed']+index*17)&255 for i in range(32)))
            cap=case['hcap'] if index==1 else case['vcap'] if index==2 else 4
            count=case['hcount'] if index==1 else case['vcount'] if index==2 else case['guard'] if index==0 else case['tracked']
            self.put(BASE+offset,pointer);self.put(BASE+offset+4,cap);self.put(BASE+offset+8,count)
            for item in range(count):self.put(pointer+item*4,50 if index in (0,3) else 60 if item==0 and case['hdup' if index==1 else 'vdup'] else 200+index*10+item)
        self.put(BASE+0x248,case['mode']);self.put(BASE+0x404,0x11223344)
        for address,value in ((0x5c8464,POOL),(0x5c847c,24000),(0x59ab20,TYPES),(0x5412e8,164)):
            self.put(address,value)
        victim_type=164 if case['altar'] else 71
        for typeid,genus,group in ((victim_type,case['genus'],0),(140,0x100000 if case['flyer'] else 0,0),(73,0,case['homegroup'])):
            mu.mem_write(TYPES+typeid*500,bytes(500));self.put(TYPES+typeid*500+0xec,genus);self.put(TYPES+typeid*500+0x9c,group)
        slots={50:(victim_type,0,5.5,7.5),60:(140,0,-4.75,9.25)}
        if 0<case['home']<24000:slots[case['home']]=(73,case['homefree'],12.75,-3.5)
        self.slots=sorted(slots)
        for sid,(typeid,state,x,y) in slots.items():
            raw=bytearray((i*3+sid)&255 for i in range(50));raw[10]=typeid;raw[11]=state;struct.pack_into('<ff',raw,14,x,y);mu.mem_write(POOL+sid*50,bytes(raw))
        saved={register:0x12340000+index for index,register in enumerate((UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP))}
        for register,value in saved.items():mu.reg_write(register,value)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EDX):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_FPCW,self.control);mu.reg_write(UC_X86_REG_FPTAG,0xffff);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,BASE);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<III',STOP,50,case['attacker']))
        self.writes=[(BASE,BASE+SIZE+128),(STACK-0x10000,STACK+0x100)]
        mu.emu_start(0x415460,STOP,count=30000);self.writes=[]
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+12
        assert all(mu.reg_read(register)==value for register,value in saved.items())
        assert not(mu.reg_read(UC_X86_REG_FPSW)&0x3800) and mu.reg_read(UC_X86_REG_FPCW)==self.control and self.words(0,1)[0]==0
        return [','.join(str(case[key]) for key in KEYS),bytes(mu.mem_read(BASE,SIZE)).hex(),bytes(mu.mem_read(BASE+SIZE,128)).hex(),';'.join(self.events) or '-',b''.join(bytes(mu.mem_read(POOL+sid*50,50)) for sid in self.slots).hex()]


def main():
    """기대 fixture와 실행 범위를 기록하거나 동결 SHA를 감사한다."""
    if '--verify' in sys.argv:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in facts['sha256'].items():assert digest(ROOT/name)==expected,name
        with FIXTURE.open(encoding='utf-8') as stream:rows=list(csv.DictReader(stream,delimiter='\t'))
        assert len(rows)==facts['observations'] and len({row['input'] for row in rows})==len(rows)
        print('AI 피해 원본 고유 입력/SHA 감사 통과:',len(rows));return
    cases=inputs();oracles=[Oracle(0x027f),Oracle(0x037f)];rows=[]
    for case in cases:
        first,second=(oracle.run(case) for oracle in oracles);assert first==second,case;rows.append(first)
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(('input','brain','lists','events','slots'));writer.writerows(rows)
    sources=['originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_ai_damage.py','cpppj/tests/fixtures/ai-damage-x86.tsv','cpppj/src/o/RawAiDamage.h','cpppj/src/o/RawAiDamage.cpp']
    facts={'host':socket.gethostname(),'date':'2026-10-16','observations':len(rows),'normal_returns':len(rows)*2,'x87_control_words':['027f','037f'],'precision_differences':0,'entries':['00415460','00411200','00414450'],'boundaries':['0045c100 actual FlyerProcess home response','004105b0 actual log request; output filtering/formatting is outside boundary'],'checks':['whole0xb08brain/four32bytebackinglists/wholeinputslots/events','stack/nonvolatile/x87/SEH/writeguard'],'scope':'PATCH only; no GUI/game process/new decompile; CD and whole AI strategy not observed','sha256':{name:digest(ROOT/name) for name in sources}}
    # 첫 원문 관찰은 보존하고 실제 Session의 입력 영역으로 다시 관찰한 이유를 구별한다.
    history=ROOT/'extracted/goal-campaign-damage-20261016-ai-capacity20000'
    initial=json.loads(history.with_suffix('.json').read_text(encoding='utf-8'))
    product=['cpppj/src/o/RawAiDamage.h','cpppj/src/o/RawAiDamage.cpp']
    assert all(initial['sha256'][name]==digest(ROOT/name) for name in product)
    facts['input_domain_correction']={'initial_capacity':20000,'current_capacity':24000,'reason':'The first native synthetic capacity20000 is a valid native input, but the restored C++ SidPool requires capacity>predictableFirst23001+1; the actual Session uses24000. Re-observe native boundary23999/24000 in the same domain; no C++ expected values calculated.','initial_observations':initial['observations'],'initial_fixture_sha256':digest(history.with_suffix('.tsv')),'initial_report_sha256':digest(history.with_suffix('.json')),'initial_fixture_path':str(history.with_suffix('.tsv').relative_to(ROOT)),'initial_report_path':str(history.with_suffix('.json').relative_to(ROOT)),'product_algorithm_unchanged':True,'product_sha256':{name:digest(ROOT/name) for name in product}}
    # 실제 자원 경계 두 행을 추가해도 앞서 CPP 통과한 기대값은 키별 전체 출력이 그대로다.
    passed=ROOT/'extracted/goal-campaign-damage-20261016-ai-147-passed'
    with passed.with_suffix('.tsv').open(encoding='utf-8') as stream:prior_rows={row['input']:row for row in csv.DictReader(stream,delimiter='\t')}
    with FIXTURE.open(encoding='utf-8') as stream:current_rows={row['input']:row for row in csv.DictReader(stream,delimiter='\t')}
    assert len(prior_rows)==147 and all(current_rows[key]==value for key,value in prior_rows.items())
    facts['actual_genus_extension']={'prior_fixture_sha256':digest(passed.with_suffix('.tsv')),'prior_report_sha256':digest(passed.with_suffix('.json')),'preserved_cpp_passed_rows':147,'all_prior_keys_and_output_fields_unchanged':True,'new_observations':2,'genus':'08040010','scope':'The native victim metadata remains non-Altar TYPE71 with independently supplied actual TYPE131 genus.00415460 only tests victim type==Altar164, then genus and tracked/guard lists. Attacker TYPE140 has Flyer bit clear. These two inputs distinguish initial tracked0 natural no-response and explicit tracked1 reaction; they do not synthesize AI strategy population.'}
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('AI 피해 원본 고유 입력:',len(rows),'두 정밀도 정상 반환:',len(rows)*2)


if __name__=='__main__':main()
