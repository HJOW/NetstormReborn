#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제004c8ea0 Tell파라미터부모와기존토큰/CRT를독립관측한다."""
import itertools
import json
import socket
from pathlib import Path

from decomp_game_menu_bar import Oracle as BarOracle, ROOT, PE, digest, BASE, HEAP, STACK, STOP, CONTROLS
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP,UC_X86_REG_ESP,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

FIXTURE=ROOT/'cpppj/tests/fixtures/tell-parameters-x86.tsv'  # 새실제원문관측이다.
REPORT=ROOT/'extracted/cpp-tell-parameters-20261023-native.json'  # 범위/한계/해시다.
SOURCE,VALUE,NAME,CONFIG=HEAP+0x1000,HEAP+0x4000,HEAP+0x5000,HEAP+0x6000  # 서로다른명시장치영역이다.
ENTRY,END=0x4C8EA0,0x4C8F60  # 전체부모의실제명령범위다.
RANGES=((ENTRY,END),(0x487620,0x487740),(0x4E63D0,0x4E649E))  # 토큰/지정CRT strchr도실제실행한다.


class Oracle(BarOracle):
    """부모/토큰의정수·문자열명령을실행하고Config치환/Append만명시한다."""
    def instruction(self,mu,address,size,data):
        """분기/다음포인터/80BYTE제한은원문명령으로만관측한다."""
        if address == STOP:
            self.completed=True;mu.emu_stop();return
        if any(low<=address<high for low,high in RANGES):
            self.instructions+=1;return
        esp=mu.reg_read(UC_X86_REG_ESP);arg=lambda index:self.word(esp+4+index*4)  # 실제cdecl경계인자다.
        if address == 0x441740:
            text=self.text(arg(0));self.event('S',text.hex())
            if tuple(arg(index) for index in range(1,5)) != (1024,0,0,0):
                raise RuntimeError('원본SubstituteLocal현재인자불일치')
            if self.profile == 1:
                value=b'['+text+b']'
            elif self.profile == 2:
                value=self.values.get(1,b'') if text == b'{param.1}' else text
            elif self.profile == 3:
                value=b'' if text == b'EMPTY' else b'Q";=\n'+text
            elif self.profile == 4:
                value=b'N\0TAIL'
            else:
                value=text
            self.mu.mem_write(VALUE,value+b'\0');self.returned(VALUE)
        elif address == 0x4402F0:
            if arg(0) != CONFIG or self.text(arg(1)) != b'%d="%s"':
                raise RuntimeError('원본param.Append현재객체/서식불일치')
            index=arg(2);value=self.text(arg(3));line=str(index).encode('ascii')+b'="'+value+b'"';self.values[index]=value
            self.event('A',line.hex());self.returned()
        elif address == 0x4B9390:
            if arg(0) != 256:
                raise RuntimeError('원본이름문자열고리요청크기불일치')
            self.name=self.text(arg(1));self.mu.mem_write(NAME,self.name+b'\0');self.returned(NAME)
        else:
            raise RuntimeError(f'미명시원문실행 {address:08x}')

    def run(self,source,profile,control):
        """입력문자열과명시Config프로필만제공하고전체결과순서를읽는다."""
        self.profile,self.trace,self.values,self.instructions,self.completed,self.name=profile,[],{},0,False,None
        self.mu.mem_write(SOURCE,source+b'\0');self.mu.mem_write(STACK-0x8000,bytes(0x10000));self.word(STACK,STOP);self.word(STACK+4,CONFIG);self.word(STACK+8,SOURCE)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP):  # 원문일반레지스터를독립초기화한다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(ENTRY,STOP+1,count=300000)
        if not self.completed or self.mu.reg_read(UC_X86_REG_ESP) != STACK+4 or self.name is None:
            raise RuntimeError('원문정상반환/이름/스택누락')
        for low,high in RANGES:  # 실제사용한보호본체가변경되지않았는지확인한다.
            if bytes(self.mu.mem_read(low,high-low)) != self.image[low-BASE:high-BASE]:
                raise RuntimeError('보호원문변경')
        total=self.totals.setdefault(f'{control:03x}',{'runs':0,'instructions':0});total['runs']+=1;total['instructions']+=self.instructions
        return self.name.hex()+'|'+';'.join(self.trace)


def sources():
    """정상메뉴·빈/인용/구분자·토큰78BYTE·NUL의실제입력만열거한다."""
    values=[b'',b'About',b'SETSIZE(640,480)',b'SETSIZE(800,600)',b'SetIslands(1)',b'Foo()',b'Foo(,1)',b'Foo(1,,2)',b'Foo(1,)tail',b'Foo("",2)',b'Foo(" ",2)',b'Foo(EMPTY,{param.1})',b'Foo(first,{param.1})',b'Foo(1,2)3',b' Foo ( a , b ) ',b'Foo((1,2))',b'Foo("a,b)",c)',b'Foo(a;=b,c)',b'Foo(\x80,\xff)',b'Foo(1\0,2)',b'Foo\0(1,2)']  # 기대이름/인자는계산하지않는다.
    for size in (1,2,76,77,78,79,80,155,156,157,255,1023):  # 원본토큰상한과지역입력상한을각각교차한다.
        values.append(b'A'*size)
        if size+7<1024:
            values.append(b'Foo('+b'A'*size+b',B)')
    for space,first,second in itertools.product((b'',b' ',b'\t',b'\r\n'),(b'A',b'"A"',b'"A B"',b'"A,B"'),(b'B',b'""',b'" "',b'{param.1}')):  # 뒤공백첫바이트보존과인용을원문이판정한다.
        values.append(space+b'Foo('+first+b','+second+b')'+space)
    return list(dict.fromkeys(values))


def main():
    """제품기대없이같은원문을두정밀도로실행해새fixture만저장한다."""
    if digest(PE) != 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236':
        raise RuntimeError('보호PE식별자불일치')
    oracle,rows=Oracle(),[];inputs=list(itertools.product(sources(),range(5)))  # 명시입력문자열과Config경계프로필이다.
    for index,(source,profile) in enumerate(inputs):  # 각입력의두정밀도전체출력을비교한다.
        previous=None
        for control in CONTROLS:
            try:
                observed=oracle.run(source,profile,control)
            except Exception as error:
                raise RuntimeError(f'{index}/{source!r}/{profile}: {error}') from error
            if previous is not None and previous != observed:
                raise RuntimeError('정밀도차이')
            previous=observed;rows.append(f'{index}-cw{control:03x}\t'+source.hex()+f'\t{profile}\t'+observed)
    FIXTURE.write_text('# name\tsource\tprofile\toutput\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report={'host':socket.gethostname(),'lastDecompileHost':'VM-W11-Codex / 2026-10-23','inputs':len(inputs),'observations':len(rows),'precisionDifferences':0,'precisions':oracle.totals,'scope':'실제004c8ea0전체부모와487710/487620토큰helper/CRT strchr를실행한다. 첫구분자(·뒤구분자,)·첫빈토큰종료·토큰78BYTE·원문printf순서/이름반환은실제명령이다.','limits':['441740의Config치환·4402f0의Config.Append·4b9390의문자열고리는명시경계다. 실제Registry/본문표시/해상도변경/GUI검증이아니다.','parent원문을넘는1024BYTE무효입력은관측하지않는다.'],'gameExecuted':False,'guiExecuted':False,'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE,Path(__file__),ROOT/'tools/decomp_game_menu_bar.py',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE)}}  # 범위/한계/식별자다.
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('inputs','observations','precisionDifferences')}))


if __name__ == '__main__':
    main()
