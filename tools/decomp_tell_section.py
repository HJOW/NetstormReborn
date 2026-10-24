#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제4cf730을 격리CPU에서 실행해 이름 기록과 세 단계 섹션 선택을 관측한다."""
import argparse
import itertools
import json
import socket
from pathlib import Path
from decomp_game_menu_bar import Oracle as BaseOracle, ROOT, PE, BASE, HEAP, STACK, STOP, CONTROLS, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_FPCW, UC_X86_REG_FPSW)

ENTRY, END = 0x4CF730, 0x4CF960  # 파라미터 변환은 명시 장치이며 호출자의 전체 정상 본체는 실제 명령이다.
SOURCE, TARGET, OBJECT, MISSION, FALLBACK, BODY, PATH = (HEAP+offset for offset in (0x1000,0x2000,0x4000,0x5000,0x6000,0x7000,0x8000))  # 겹치지 않는 외부 장치 영역이다.
CURRENT, CURRENT_MISSION = 0x5CAB40, 0x5634A0  # 원문이 읽고 쓰는 현재 이름과 미션이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/tell-section-x86.tsv'  # 기존 자료를 바꾸지 않는 새 관측이다.


def fingerprint(data):
    """NUL 뒤를 포함한256BYTE 전체를 독립 기록한다."""
    value = 2166136261
    for byte in data:  # 물리 바이트 순서를 보존한다.
        value = ((value ^ byte)*16777619) & 0xffffffff
    return f'{value:08x}'


class Oracle(BaseOracle):
    """Config/파서/HTML 등의 하위 장치만 명시한 원문 실행기다."""
    def instruction(self, mu, address, size, data):
        """이름 비교/복사·점 분기·현재 미션 재조회는 원문 명령으로 실행한다."""
        if address == STOP:
            self.completed = True
            mu.emu_stop()
            return
        if ENTRY <= address < END:
            self.instructions += 1
            return
        esp = mu.reg_read(UC_X86_REG_ESP)
        arg = lambda index: self.word(esp+4+index*4)  # 선택한 외부CALL의 실제 인자다.
        if address == 0x4E65ED:
            self.returned(0 if self.text(arg(0)).lower() == self.text(arg(1)).lower() else 1)
        elif address == 0x43FC20:
            self.returned(mu.reg_read(UC_X86_REG_ECX), 4)
        elif address == 0x4C8EA0:
            self.returned(TARGET)
        elif address == 0x4414D0:
            if self.text(arg(0)) != b'global.priorTell':
                raise RuntimeError('priorTell key mismatch')
            self.event('P', self.text(arg(1)).hex())
            if self.mask & 64:
                self.available = not self.available
                self.word(CURRENT_MISSION, MISSION if self.available else 0)
                mu.mem_write(CURRENT, b'changed\0')
            self.returned()
        elif address == 0x4E63E0:
            text = self.text(arg(0))
            at = text.find(bytes([arg(1) & 255]))
            self.returned(arg(0)+at if at >= 0 else 0)
        elif address == 0x43FB90:
            self.named_prefix = self.text(arg(0))
            # 이름 뒤의 섹션은 원문이 쓰는 현재 지역 입력에서 읽는다.
            whole = self.target.split(b'\0',1)[0]
            self.event('N', self.named_prefix.hex(), whole.split(b'.',1)[1].hex())
            self.returned(OBJECT if self.mask & 1 else 0)
        elif address == 0x440700:
            owner, name = mu.reg_read(UC_X86_REG_ECX), self.text(arg(0))
            if owner == OBJECT:
                present, value = bool(self.mask & 2), b'named'
            elif owner == MISSION:
                self.event('M', name.hex())
                present, value = bool(self.mask & 8), b'mission'
            elif owner == FALLBACK:
                self.event('F', name.hex())
                present, value = bool(self.mask & 16), b'fallback'
            else:
                raise RuntimeError(f'unknown config {owner:x}')
            if present:
                self.selected = b'' if self.mask & 32 else value
                mu.mem_write(BODY, self.selected+b'\0')
            self.returned(BODY if present else 0, 4)
        elif address == 0x4E4391:
            if arg(0) != 24:
                raise RuntimeError('fallback allocation mismatch')
            self.returned(FALLBACK)
        elif address == 0x4410F0:
            if self.text(arg(0)) != b'missionSpec' or self.text(arg(1)) != b'tell':
                raise RuntimeError('fallback path mismatch')
            self.returned(PATH)
        elif address == 0x440380:
            if mu.reg_read(UC_X86_REG_ECX) != FALLBACK or arg(0) != PATH:
                raise RuntimeError('fallback load mismatch')
            self.returned(0, 4)
        elif address in (0x43FCD0, 0x43FD40, 0x4E4354, 0x4C2630):
            self.returned()
        elif address == 0x4CF270:
            if tuple(arg(index) for index in range(4)) != (BODY,self.suppress,0,0):
                raise RuntimeError('HTML arguments mismatch')
            self.returned(0x7654)
        else:
            raise RuntimeError(f'unmodeled instruction {address:08x}')

    def run(self, target, previous, mask, suppress, control):
        """기대값을 계산하지 않고 입력과 하위 장치 존재 상태만 공급한다."""
        self.target, self.mask, self.suppress = target, mask, suppress
        self.trace, self.instructions, self.completed, self.selected = [], 0, False, None
        self.available = bool(mask & 4)
        self.mu.mem_write(SOURCE,b'request()\0')  # BLANK 조기 분기와 구별한 정규화 전의 명시 입력이다.
        self.mu.mem_write(TARGET,target+b'\0')
        self.mu.mem_write(PATH,b'd/tell.english\0')
        self.mu.mem_write(CURRENT,bytes([0x5a])*256)
        self.mu.mem_write(CURRENT,previous+b'\0')
        self.word(CURRENT_MISSION,MISSION if self.available else 0)
        self.word(MISSION+0x5c,MISSION)
        self.mu.mem_write(STACK-0x8000,bytes(0x10000))
        self.word(STACK,STOP)
        self.word(STACK+4,SOURCE)
        self.word(STACK+8,suppress)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP):  # 일반 레지스터를 독립 초기화한다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(ENTRY,STOP+1,count=100000)
        if not self.completed or self.mu.reg_read(UC_X86_REG_ESP) != STACK+4:
            raise RuntimeError('native return/stack mismatch')
        returned = self.mu.reg_read(UC_X86_REG_EAX)
        if returned != (0x7654 if self.selected is not None else 0):
            raise RuntimeError('native result mismatch')
        if bytes(self.mu.mem_read(ENTRY,END-ENTRY)) != self.image[ENTRY-BASE:END-BASE]:
            raise RuntimeError('native code changed')
        total = self.totals.setdefault(f'{control:03x}',{'runs':0,'instructions':0})
        total['runs'] += 1
        total['instructions'] += self.instructions
        return self.text(CURRENT).hex()+'|'+fingerprint(self.mu.mem_read(CURRENT,256))+'|'+('-' if self.selected is None else self.selected.hex())+'|'+';'.join(self.trace)


def main():
    """두 정밀도 관측을 기록하며 --check는 기존fixture를 쓰지 않는다."""
    args = argparse.ArgumentParser()
    args.add_argument('--check',action='store_true')
    options = args.parse_args()
    if digest(PE) != 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236':
        raise RuntimeError('protected PE mismatch')
    targets = (b'About',b'about',b'file.Section',b'file.',b'.Section',b'file.a.b',b'',b'BLANK',b'A'*78,b'\x80.section',b'file.Section\0ignored')
    priors = (b'',b'About',b'file.Section')
    masks = (0,1,2,3,4,12,16,31,63,64,68,79,95)
    oracle, rows = Oracle(), []
    for index,(target,prior,mask) in enumerate(itertools.product(targets,priors,masks)):  # 각 장면은 입력으로만 구성한다.
        previous = None
        for control in CONTROLS:  # 두x87 정밀도에서 전체 효과와256BYTE를 독립 비교한다.
            observed = oracle.run(target,prior,mask,index%2,control)
            if previous is not None and previous != observed:
                raise RuntimeError('precision difference')
            previous = observed
            rows.append(f'{index}-cw{control:03x}\t{target.hex()}\t{prior.hex()}\t{mask}\t{observed}')
    value = '# name\ttarget\tprior\tmask\tobserved\n'+'\n'.join(rows)+'\n'
    if options.check:
        if FIXTURE.read_text(encoding='utf-8') != value:
            raise RuntimeError('fixture mismatch')
    else:
        FIXTURE.write_text(value,encoding='utf-8',newline='\n')
    report = {'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'precisionDifferences':0,'precisions':oracle.totals,'gameExecuted':False,'scope':'실제4cf730 정상 본체에서 이름 비교/복사와 세 단계 분기를 실행한다. fixture는 파라미터 처리 뒤 이름 선택 부분의 투영이며 BLANK 조기 분기는 포함하지 않는다.','boundaries':['Config 생성/등록/적재/섹션/소멸, 파라미터 파서, stricmp/strchr, 할당/free, 경고, HTML 공장은 명시 장치다.','GUI/전체Tell생성/입력/배경/미션 완주 검증이 아니다.'],'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE,Path(__file__),FIXTURE,ROOT/'extracted/refined/originals/Netstorm.c')}}
    (ROOT/'extracted/cpp-tell-section-408-native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'precisionDifferences':0,'check':options.check}))


if __name__ == '__main__':
    main()
