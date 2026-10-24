#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 @/# 전체 정상 본체의 등록·중복·정렬·행 효과를 독립 관측한다."""
import argparse
import itertools
import json
import socket
from decomp_game_menu_bar import Oracle as BaseOracle, ROOT, PE, BASE, HEAP, STACK, STOP, CONTROLS, digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP,UC_X86_REG_ESP,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

FIXTURE = ROOT/'cpppj/tests/fixtures/tell-file-menus-x86.tsv'  # 기존 자료와 구별한 새 원문 입력/출력이다.
SOURCE, ARGUMENT, PATH, NAME, VALUE = (HEAP+offset for offset in (0x1000,0x1100,0x1200,0x1400,0x1600))  # 실제 명시 하위 장치의 서로 다른 버퍼다.
COMMAND = 0x02000095  # 원본 Tell 명령이며 문자열 인자를 숫자로 바꾸지 않는다.


def basename(path):
    """기존에 분석한 FileSpec 장치의 마지막 구분자/첫 점 반환을 공급한다."""
    return path.replace(b'\\',b'/').split(b'/')[-1].split(b':')[-1].split(b'.')[0]


def title(name, profile):
    """기대 정렬/행과 무관한 파일 내용의 명시 제목만 공급한다."""
    if profile == 1 and name.lower() == b'a':
        return b''
    if profile == 1 and name.lower() == b'b':
        return b'--divider'
    return b'Title '+name


class Oracle(BaseOracle):
    """FileSpec/Config/할당/고리/메뉴는 명시 장치, 호출자의 전체 루프는 원문이다."""
    def new_config(self, address, name):
        """실제 Config+0xc의 고정 prefix 포인터와 첫 등록 순서를 공급한다."""
        prefix = HEAP+0x9000+len(self.prefixes)*256
        self.prefixes.append(prefix)
        self.mu.mem_write(prefix,name+b'.\0')
        self.word(address+12,prefix)
        self.configs[address] = {'name':name,'values':{}}
        self.registry.append(address)
        return address

    def instruction(self, mu, address, size, data):
        """정수/문자열 복사와 수집·중복 제거·정렬·@/# 분기는 원문으로 실행한다."""
        if address == STOP:
            self.completed=True
            mu.emu_stop()
            return
        if self.entry <= address < self.end:
            self.instructions+=1
            return
        esp=mu.reg_read(UC_X86_REG_ESP)
        arg=lambda index:self.word(esp+4+index*4)  # 실제 외부 CALL 인자만 읽는다.
        owner=mu.reg_read(UC_X86_REG_ECX)
        if address in (0x4437F0,0x4587C0,0x4587D0,0x4595F0):
            self.returned()
        elif address == 0x4588A0:
            if self.text(arg(0)) != b'input':
                raise RuntimeError('file pattern mismatch')
            self.cursor=0
            self.returned(0,4)
        elif address == 0x458B40:
            self.returned(int(self.cursor < len(self.paths)))
        elif address == 0x458B90:
            self.mu.mem_write(PATH,self.paths[self.cursor]+b'\0')
            self.returned(PATH)
        elif address == 0x458B50:
            self.cursor+=1
            self.returned()
        elif address == 0x45A220:
            self.file=self.text(arg(0))
            self.returned(owner,4)
        elif address == 0x459890:
            self.mu.mem_write(PATH,self.file+b'\0')
            self.returned(PATH,4)
        elif address == 0x459C40:
            self.returned(0,4)
        elif address == 0x459B20:
            self.mu.mem_write(NAME,basename(self.file)+b'\0')
            self.returned(NAME,4)
        elif address == 0x43FB90:
            name=self.text(arg(0)).lower()
            self.returned(next((value for value in self.registry if self.configs[value]['name'].lower()==name),0))
        elif address == 0x4E4391:
            if arg(0)!=24:
                raise RuntimeError('Config allocation mismatch')
            self.allocated=HEAP+0x4000+self.allocations*64
            self.allocations+=1
            self.returned(self.allocated)
        elif address == 0x43FC20:
            self.returned(self.new_config(owner,self.text(arg(0))),4)
        elif address == 0x4402F0:
            target=self.configs[arg(0)]
            form=self.text(arg(1))
            key=form.split(b'=')[0]
            target['values'][key]=(str(arg(2)).encode() if form.endswith(b'%d') else self.text(arg(2)))
            self.returned()
        elif address == 0x440380:
            target=self.configs[owner]
            path=self.text(arg(0))
            target['values'][b'title']=title(target['name'],self.profile)
            self.event('L',target['name'].hex(),path.hex(),target['values'][b'index'].decode())
            self.returned(0,4)
        elif address == 0x4E65ED:
            left,right=self.text(arg(0)).lower(),self.text(arg(1)).lower()
            self.returned((left>right)-(left<right))
        elif address == 0x441740:
            source=self.text(arg(0))
            if source == b'{{local.1}.title}':
                name=self.text(arg(2)).lower()
                target=next(self.configs[value] for value in self.registry if self.configs[value]['name'].lower()==name)
                value=target['values'][b'title']
            else:
                value=source.replace(b'{cur.file}',self.current_file())
            self.mu.mem_write(VALUE,value+b'\0')
            self.returned(VALUE)
        elif address == 0x4C7910:
            self.strings[self.next_string]=self.text(arg(0))
            self.returned(self.next_string)
            self.next_string+=1
        elif address == 0x477CB0:
            self.event('R',self.text(arg(0)).hex(),arg(1),self.strings[arg(2)].hex(),self.current_file().hex())
            self.returned()
        elif address == 0x478260:
            self.event('S',self.current_file().hex())
            self.returned()
        elif address == 0x43FCD0:
            self.registry.remove(owner)
            self.returned()
        else:
            raise RuntimeError(f'unmodeled instruction {address:08x}')

    def current_file(self):
        """현재 등록된 마지막 cur Config의 실제 file 필드만 공급한다."""
        return next(self.configs[value]['values'][b'file'] for value in reversed(self.registry) if self.configs[value]['name']==b'cur')

    def run(self, paths, profile, existing, argument, fallback, control):
        """파일 입력/제목/기존 객체만 공급하고 두 실제 부모의 효과를 읽는다."""
        self.entry,self.end=(0x4CE1B0,0x4CE580) if fallback else (0x4CE580,0x4CE940)
        self.paths,self.profile=paths,profile
        self.trace,self.registry,self.configs,self.prefixes,self.strings=[],[],{},[],{}
        self.instructions,self.allocations,self.next_string,self.completed=0,0,1,False
        if existing:
            seeded=self.new_config(HEAP+0x3c00,b'b')
            self.configs[seeded]['values']={b'title':b'Existing',b'index':b'99'}
        self.mu.mem_write(SOURCE,b'input\0')
        self.mu.mem_write(ARGUMENT,argument+b'\0')
        self.mu.mem_write(STACK-0x8000,bytes(0x10000))
        self.word(STACK,STOP)
        for index,value in enumerate((SOURCE,COMMAND,ARGUMENT)):  # 실제cdecl 인자 세 개를 독립 공급한다.
            self.word(STACK+4+index*4,value)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP):  # 일반 레지스터를 독립 초기화한다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(self.entry,STOP+1,count=500000)
        if not self.completed or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:
            raise RuntimeError('native return/stack mismatch')
        if bytes(self.mu.mem_read(self.entry,self.end-self.entry))!=self.image[self.entry-BASE:self.end-BASE]:
            raise RuntimeError('native body changed')
        if any(self.configs[value]['name']==b'cur' for value in self.registry):
            raise RuntimeError('cur Config not destroyed')
        totals=self.totals.setdefault(f'{control:03x}',{'runs':0,'instructions':0})
        totals['runs']+=1
        totals['instructions']+=self.instructions
        return ';'.join(self.trace)


def main():
    """원문 관측을 생성하거나 --check로 같은 자료를 쓰지 않고 재생한다."""
    parser=argparse.ArgumentParser()
    parser.add_argument('--check',action='store_true')
    options=parser.parse_args()
    if digest(PE)!='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236':
        raise RuntimeError('protected PE mismatch')
    cases=[[],[b'd/z.english'],[b'd/b.extra.english',b'd/a.english',b'd/z.english'],[b'd/A.english',b'e/a.german',b'd/b.english'],[b'd/a.english',b'd/a.english'],[b'plain']]
    cases.extend(list(value) for value in itertools.permutations((b'd/c.english',b'd/a.english',b'd/b.english')))
    oracle,rows=Oracle(),[]
    for index,(paths,profile,existing,argument,fallback) in enumerate(itertools.product(cases,range(2),range(2),(b'{cur.file}',b'123'),range(2))):  # 기대 정렬을 계산하지 않는 입력 조합이다.
        previous=None
        for control in CONTROLS:  # 원문의 두 정밀도 전체 효과를 독립 관측한다.
            observed=oracle.run(paths,profile,existing,argument,fallback,control)
            if previous is not None and observed!=previous:
                raise RuntimeError('precision difference')
            previous=observed
            rows.append(f'{index}-cw{control:03x}\t'+','.join(path.hex() for path in paths)+f'\t{profile}\t{existing}\t{argument.hex()}\t{fallback}\t{observed}')
    value='# name\tpaths\tprofile\texisting\targument\tfallback\tobserved\n'+'\n'.join(rows)+'\n'
    if options.check:
        if FIXTURE.read_text(encoding='utf-8')!=value:
            raise RuntimeError('fixture mismatch')
    else:
        FIXTURE.write_text(value,encoding='utf-8',newline='\n')
    report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'precisionDifferences':0,'precisions':oracle.totals,'gameExecuted':False,'scope':'실제4ce1b0/4ce580의 전체 정상 본체에서 수집/고유 prefix·비교교환·@의 빈 제목 폴백·구분선/행·cur 소멸을 실행한다.','boundaries':['FileSpec 열거/경로·이름 반환, Config 생성/조회/Append/적재/치환/소멸, 할당·CRT 비교·문자열 고리·메뉴 작성은 명시 장치다.','실제 OS 열거와 GUI 픽셀/전체 Tell 공장·미션 완주 검증이 아니다.'],'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE,FIXTURE,ROOT/'tools/decomp_tell_file_menus.py',ROOT/'extracted/refined/originals/Netstorm.c')}}
    (ROOT/'extracted/cpp-tell-files-409-native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'precisionDifferences':0,'check':options.check}))


if __name__=='__main__':
    main()
