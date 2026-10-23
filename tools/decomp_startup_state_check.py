#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004b5ba0만 실제 진입/ret까지 실행하고 저장상태 reader·UI는 명시 하위로 관측한다."""
import argparse
from collections import Counter
import itertools
import json
import math
from pathlib import Path
import socket
import struct

import decomp_existing_editor_install_outer as parent
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG,
)

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE/정밀 C와 이전 명시 장치 oracle의 기준 경로다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/startup-state-check-x86.tsv'  # 제품 작성 전 고정하는 독립 부모 관측이다.
REPORT = ROOT / 'cpppj/startup-state-check-native.json'  # 최초 도구·입력·관측 SHA와 범위를 함께 보존한다.


def bits(value):
    """입력 double의 NaN/경계 비트를 손실 없이 fixture에 표시한다."""
    return struct.pack('<d', value).hex()


def cases():
    """설정·signed 저장시각·벽시각과 float32 경과분 경계를 독립 입력한다."""
    result = []  # 실제 부모에 전달할 입력이며 제품 계산 결과를 사용하지 않는다.
    times = (0.0, -0.0, 100.0, 700.0, 2800.0, -700.0, float('inf'), float('-inf'), float('nan'))
    for option, saved, now in itertools.product((0, 1, 0xffffffff), (-2147483648, -1, 0, 1, 100, 2147483647), times):  # GetInt의 영/비영과 fild signedDWORD를 교차한다.
        result.append((option, saved, bits(now), 'none'))
    for saved in (1, 100, 2147483647):  # 각 정수부에서0/45분 경계의 double 이웃을 따로 확인한다.
        for minutes in (0.0, 2.0**-149, 10.0, 45.0):
            at = saved + minutes * 60.0  # 경계 입력 생성일 뿐 기대 분기는 PE가 결정한다.
            for now in (math.nextafter(at, -math.inf), at, math.nextafter(at, math.inf)):
                result.append((1, saved, bits(now), 'none'))
    for mutation in ('startup-option-stop', 'saved-stop', 'time-stop', 'translate-stop', 'translate2-stop', 'format-stop', 'format2-stop', 'tell-stop', 'saved-pending'):  # 실제 선택 하위의 부분 중단을 정상ret와 구별한다.
        result.append((1, 100, bits(700.0), mutation))
    return list(dict.fromkeys(result))  # 겹치는 명시 경계 입력은 한 번만 실행한다.


class Oracle(parent.Oracle):
    """기존 외곽의 명시 reader/번역/format/Tell 장치만 빌리고 부모 진입은 독립 실행한다."""
    def mutate(self, boundary):
        """두 번째 번역/format의 중단도 각 호출 효과 뒤에만 적용한다."""
        if self.case[-1] == boundary + '2-stop' and self.calls[boundary] == 2:
            self.finish(boundary + '2')
        else:
            super().mutate(boundary)

    def sample_parent(self, case, control):
        """정확한 cdecl 진입부터 실제 ret 또는 선택 하위 중단까지 결과와 호출을 기록한다."""
        option, saved, now, mutation = case
        self.case = (1, 0, 1, 0, 0, 0, 1, 1, mutation)  # 이전 oracle의 명시 하위 입력 형태만 재사용한다.
        self.calls, self.events, self.config, self.cause, self.instructions = Counter(), [], {}, '', 0
        self.startup_option, self.saved_stamp, self.now, self.other = option, saved, struct.unpack('<d', bytes.fromhex(now))[0], 8
        self.mu.mem_write(parent.STACK - 0x1800, b'\xcd' * 0x1808)
        self.mu.mem_write(parent.PENDING, b'BeforeSavedState\0')
        self.put(0x5c85a4, 1)
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):  # 이전 관측의 레지스터·x87 상태를 이어받지 않는다.
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_ESP, parent.STACK)
        self.put(parent.STACK, parent.STOP)
        self.mu.emu_start(0x4b5ba0, parent.STOP)
        if not self.cause:
            if self.mu.reg_read(UC_X86_REG_EIP) != parent.STOP:
                raise RuntimeError('startupStateCheck가 실제ret 없이 종료됨')
            self.cause = 'return'
        self.completions[self.cause] += 1
        return '\t'.join((str(control), ','.join(map(str, case)), self.cause, str(self.mu.reg_read(UC_X86_REG_EAX)), self.string(parent.PENDING).hex(), ';'.join(self.events)))


def main():
    """최초 자료의 덮어쓰기를 거부하고 후속에는 기존 SHA만 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, expected in report['sha256'].items():  # 최초 원문·도구·기대 자료의 모든 SHA를 확인한다.
            if parent.digest(ROOT / name) != expected:
                raise RuntimeError('startupStateCheck 최초 SHA 불일치: ' + name)
        print(json.dumps({'verified': True, 'observations': report['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('startupStateCheck 최초 자료 덮어쓰기 금지')
    oracle, inputs, differences = Oracle(), cases(), 0
    rows = ['# control / option,savedSignedDWORD,wallDoubleLE,mutation / completion / eax / currentPending / trace']
    for case in inputs:  # 같은 입력을 두 x87 정밀도로 각각 초기화해 실제 부모를 실행한다.
        pair = [oracle.sample_parent(case, control) for control in (0x27f, 0x37f)]
        differences += pair[0].split('\t', 1)[1] != pair[1].split('\t', 1)[1]
        rows.extend(pair)
    FIXTURE.write_text('\n'.join(rows) + '\n', encoding='utf-8')
    names = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_existing_editor_install_outer.py', 'tools/decomp_startup_state_check.py', FIXTURE.relative_to(ROOT).as_posix())
    report = {'status': '단위385 독립 부모 선관측·공용 미등록', 'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entry': '004b5ba0', 'range': '004b5ba0..004b5c98', 'uniqueInputs': len(inputs), 'observations': len(inputs)*2, 'instructions': oracle.total_instructions, 'precisionDifferences': differences, 'completions': dict(oracle.completions), 'scope': '실제 GetInt관문/reader호출뒤벽시각/x87 signedDWORD→double차·1/60곱·float32저장/엄격0<분<45/UI호출순서/ret. saved-state772BYTE reader·Config/번역/format/Tell은 기존 oracle의 명시 하위이며 실제 파일·OS·GUI·재접속 실행이 아니다.', 'sha256': {name: parent.digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({key:report[key] for key in ('uniqueInputs', 'observations', 'instructions', 'precisionDifferences', 'completions')}))


if __name__ == '__main__':
    main()
