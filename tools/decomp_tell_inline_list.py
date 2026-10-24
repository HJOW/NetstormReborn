#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본4cdd40의 % 목록 전체 정상 본체와 공유 꼬리를 독립 관측한다."""
import argparse
import itertools
import json
import socket
from decomp_tell_file_menus import Oracle as FileOracle
from decomp_game_menu_bar import ROOT,PE,BASE,HEAP,STACK,STOP,CONTROLS,digest
from decomp_tell_file_menus import SOURCE,ARGUMENT
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP,UC_X86_REG_ESP,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

OUTPUT,COMMAND,VALUE=HEAP+0x2500,HEAP+0x2400,HEAP+0x1600  # 원문 String 반환 객체와 두 번 치환 장치의 서로 다른 영역이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/tell-inline-list-x86.tsv'  # % 전용 새 관측이며 @/# 자료를 쓰지 않는다.


class Oracle(FileOracle):
    """같은 파일 장치에 실제 String 경계만 추가하고 모든 원문 정상 분기를 실행한다."""
    def instruction(self, mu, address, size, data):
        """실제 두 번 치환/링크 조각 작성과4ce14d 공유 꼬리로의 점프를 유지한다."""
        esp=mu.reg_read(UC_X86_REG_ESP)
        arg=lambda index:self.word(esp+4+index*4)  # 실제 외부 CALL 인자다.
        owner=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4E3180:
            self.text_objects[owner]=self.text(arg(0))
            self.returned(owner,4)
        elif address==0x4E3590:
            self.text_objects[owner]+=self.text(arg(0))
            self.returned(owner,4)
        elif address==0x4E3130:
            self.text_objects[owner]=self.text_objects[arg(0)]
            self.returned(owner,4)
        elif address==0x4E31A0:
            del self.text_objects[owner]
            self.returned()
        elif address==0x441740 and self.text(arg(0))==b'{@probe.value}':
            self.probe+=1
            self.event('Q',self.current_file().hex(),self.probe)
            self.mu.mem_write(VALUE,str(self.probe).encode()+b'\0')
            self.returned(VALUE)
        else:
            super().instruction(mu,address,size,data)

    def run_inline(self, paths, profile, existing, argument, control):
        """출력 문자열/동적 조회 값은 원문 실행 뒤에만 읽는다."""
        self.entry,self.end=0x4CDD40,0x4CE1B0
        self.paths,self.profile=paths,profile
        self.trace,self.registry,self.configs,self.prefixes,self.strings=[],[],{},[],{}
        self.text_objects={}
        self.instructions,self.allocations,self.next_string,self.completed,self.probe=0,0,1,False,0
        if existing:
            seeded=self.new_config(HEAP+0x3c00,b'b')
            self.configs[seeded]['values']={b'title':b'Existing',b'index':b'99'}
        self.mu.mem_write(SOURCE,b'input\0')
        self.mu.mem_write(COMMAND,b'TeLl\0')
        self.mu.mem_write(ARGUMENT,argument+b'\0')
        self.mu.mem_write(STACK-0x8000,bytes(0x10000))
        self.word(STACK,STOP)
        for index,value in enumerate((OUTPUT,SOURCE,COMMAND,ARGUMENT)):  # String 숨은 반환 포인터를 포함한 실제 네cdecl 인자다.
            self.word(STACK+4+index*4,value)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP):  # 원문 레지스터를 독립 초기화한다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(self.entry,STOP+1,count=500000)
        if not self.completed or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or self.mu.reg_read(UC_X86_REG_EAX)!=OUTPUT or OUTPUT not in self.text_objects:
            raise RuntimeError('native String return/stack mismatch')
        if any(self.configs[value]['name']==b'cur' for value in self.registry):
            raise RuntimeError('cur Config not destroyed')
        if bytes(self.mu.mem_read(self.entry,self.end-self.entry))!=self.image[self.entry-BASE:self.end-BASE]:
            raise RuntimeError('native body changed')
        totals=self.totals.setdefault(f'{control:03x}',{'runs':0,'instructions':0})
        totals['runs']+=1
        totals['instructions']+=self.instructions
        return ';'.join(self.trace)+'|'+self.text_objects[OUTPUT].hex()


def main():
    """빈 목록/중복/정렬/구분선/두 번 동적 조회의 원문 결과를 기록한다."""
    parser=argparse.ArgumentParser()
    parser.add_argument('--check',action='store_true')
    options=parser.parse_args()
    if digest(PE)!='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236':
        raise RuntimeError('protected PE mismatch')
    cases=[[],[b'd/z.english'],[b'd/b.extra.english',b'd/a.english',b'd/z.english'],[b'd/A.english',b'e/a.german',b'd/b.english'],[b'd/a.english',b'd/a.english'],[b'plain']]
    cases.extend(list(value) for value in itertools.permutations((b'd/c.english',b'd/a.english',b'd/b.english')))
    oracle,rows=Oracle(),[]
    for index,(paths,profile,existing,argument) in enumerate(itertools.product(cases,range(2),range(2),(b'{cur.file}',b'123',b'{@probe.value}'))):  # 기대 HTML을 계산하지 않는 입력 조합이다.
        previous=None
        for control in CONTROLS:  # 두 정밀도의 현재 효과와 최종 String 바이트를 비교한다.
            observed=oracle.run_inline(paths,profile,existing,argument,control)
            if previous is not None and observed!=previous:
                raise RuntimeError('precision difference')
            previous=observed
            rows.append(f'{index}-cw{control:03x}\t'+','.join(path.hex() for path in paths)+f'\t{profile}\t{existing}\t{argument.hex()}\t{observed}')
    value='# name\tpaths\tprofile\texisting\targument\tobserved\n'+'\n'.join(rows)+'\n'
    if options.check:
        if FIXTURE.read_text(encoding='utf-8')!=value:
            raise RuntimeError('fixture mismatch')
    else:
        FIXTURE.write_text(value,encoding='utf-8',newline='\n')
    report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'precisionDifferences':0,'precisions':oracle.totals,'gameExecuted':False,'scope':'실제4cdd40 전체 정상 본체/4ce14d 공유 꼬리의 고유 prefix·정렬·빈 제목/구분선·인자 두 번 치환·링크 조각/최종 String을 관측한다.','boundaries':['FileSpec·Config·할당·CRT 비교·String 생성/추가/복사/소멸은 명시 장치다. Config의 동적 두 번 조회 장치는 각 호출에 서로 다른 값을 돌려준다.','행 작성 중 C++ 예외/Catch funclet 경로와 실제 GUI/전체 Tell/미션 완주는 검증하지 않는다.'],'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE,FIXTURE,ROOT/'tools/decomp_tell_inline_list.py',ROOT/'tools/decomp_tell_file_menus.py',ROOT/'extracted/refined/originals/Netstorm.c')}}
    (ROOT/'extracted/cpp-tell-inline-410-native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'precisionDifferences':0,'check':options.check}))


if __name__=='__main__':
    main()
