#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 게임/OS 없이 State3 부모와 공통 ret를 독립 CPU로 관찰하고 최초 관측을 재검증한다."""
import argparse
import csv
import hashlib
import json
import socket
import struct
from collections import Counter
from pathlib import Path
from decomp_local_mission_transport import Oracle as Base, ROOT, TEXT, STACK, STOP
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG)

BLOCKS = (0x5c8630, 0x5c8758, 0x5c86c8, 0x5c87e8, 0x5c8878)  # 실제 old/phase/immediate/current/next 순서다.
WORDS = (0x55a420, 0x5c85ac, 0x542530, 0x54da8c, 0x53190c, 0x5c85a4)  # C++ 부모가 직접 사용하는 현재 DWORD의 관측 순서다.
SELECTED = (0x4b6dd0, 0x4b88c0, 0x4b2bc0, 0x4437f0, 0x4180d0, 0x463b20, 0x40aa60, 0x4df230)  # 부모/공통ret/빈Guide/기한/실제5BYTE종료패킷 생성자만 CPU로 실행한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/native-main-menu-state-x86.tsv'  # 기존 자료를 덮어쓰지 않는 새 독립 관측이다.
REPORT = ROOT / 'cpppj/native-main-menu-state-native-20261026.json'  # PE/도구/원시관측 해시와 검증 범위를 기록한다.


def digest(path):
    """현재 바이트의 SHA256만 계산한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def cases():
    """분기 결과를 계산하지 않고 의미가 다른 최소 입력과 명시 외부 변경/중단만 선택한다."""
    base = (1, 1, 0, 1, 0, 1, 4, 99, 0, 0, 1, 'none')
    rows = [('default', base)]
    variants = {
        'welcome': {1: 0}, 'already-seen': {3: 0}, 'updated': {4: 1}, 'tips-off': {5: 0},
        'tips-high': {5: 0x80000000}, 'first-high': {3: 0xffffffff}, 'startup-question': {2: 1},
        'startup-high': {2: 0xffffffff}, 'welcome-high': {1: 0xffffffff},
        'source-present': {6: 7, 7: 1}, 'source-missing': {6: 7, 7: 1, 10: 0},
        'source-phase-eleven': {6: 11, 7: 1}, 'source-signed-phase': {6: 0x80000000, 7: 1},
        'victory': {8: 1}, 'defeat': {8: 2}, 'other-outcome': {8: 4},
        'source-and-victory': {6: 7, 7: 1, 8: 1}, 'battle-fourteen': {6: 14, 7: 0, 9: 1},
        'battle-other-phase': {6: 12, 7: 0, 9: 1}, 'intro-zero': {0: 0},
        'tell-current-tips0': {11: 'tips0'}, 'blank-current-updated1': {11: 'updated1'},
        'startup-current-first0': {11: 'first0'}, 'startup-current-tips1': {5: 0, 11: 'tips1'},
        'music-current-source2': {6: 7, 7: 1, 11: 'source2'},
        'music-current-outcome2': {8: 1, 11: 'outcome2'},
        'prefix-current-outcome4': {11: 'prefix-outcome4'},
        'title-current-state2': {11: 'state2'}, 'startup-current-editor1': {11: 'editor1'},
        'title-pending-null': {11: 'pending-null'},
    }
    for label, updates in variants.items():  # 입력 한두 필드만 바꾸고 나머지 외부 조건은 고정한다.
        row = list(base)
        for index, value in updates.items():  # 사전에 명시한 입력값만 설치한다.
            row[index] = value
        rows.append((label, tuple(row)))
    for boundary in ('intro', 'prefix', 'hud1', 'title', 'close', 'startup', 'tell:Tips', 'palette'):
        row = list(base); row[-1] = 'stop:' + boundary; rows.append(('stop-' + boundary.replace(':', '-'), tuple(row)))
    row = list(base); row[8] = 1; row[-1] = 'stop:delete'; rows.append(('stop-delete', tuple(row)))
    row = list(base); row[-1] = 'editor1-stop'; rows.append(('stop-editorPlayers', tuple(row)))
    return rows


class Oracle(Base):
    """검토한 실제 PE 명령만 실행하고 큰 자식의 명시 장치 효과를 분기 계산과 구별한다."""
    def __init__(self):
        """격리된 PE/스택/이름 외 호스트 게임·파일·창·네트워크를 실행하지 않는다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        for address in SELECTED:  # 불연속 함수표의 실제 범위만 허용한다.
            for part in facts[address]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)
        self.instructions = 0; self.boundaries = Counter(); self.active = False  # CPU 실행 중의 원시 관측만 누적한다.

    def guard(self, mu, access, address, size, value, data):
        """부모가 쓰는 Native/이름/정확한 SEH·스택 범위 밖 쓰기를 거부한다."""
        ranges = [(0, 4), (STACK - 0x7f000, STACK + 0x1000), (0x5c8630, 0x5c8908),
                  (0x565e70, 0x565ec0), (TEXT, TEXT + 0x400), (0x544d58, 0x544d60)]
        ranges += [(field, field + 4) for field in (0x55a420, 0x5c85ac, 0x594fb8, 0x540bc0, 0x540bc4, 0x54c8a0, 0x594fc4, 0x542530)]
        if not any(low <= address and address + size <= high for low, high in ranges):
            raise RuntimeError(f'State3 CPU 쓰기 범위 밖 {address:08x}/{size}')

    def read(self, mu, access, address, size, value, data):
        """현재 userUpdated/tips의 실제 CPU 재읽기를 callback 조회와 같은 위치에서 기록한다."""
        if self.active and address in (0x54da8c, 0x53190c):
            self.events.append('updated' if address == 0x54da8c else 'tips')

    def event(self, value, boundary=None):
        """도달한 외부 경계/순서와 명시 중단만 기록한다."""
        self.events.append(value); self.boundaries[boundary or value] += 1
        if self.case[-1] == 'stop:' + (boundary or value):
            self.cause = 'stop:' + (boundary or value); self.mu.emu_stop(); return False
        return True

    def instruction(self, mu, address, size, data):
        """실제 분기/직접 쓰기/전체 복사/기한 산술 외 하위만 명시 장치로 반환한다."""
        self.instructions += 1
        esp = mu.reg_read(UC_X86_REG_ESP); args = self.words(esp + 4, 6)
        mutation = self.case[-1]
        if address == 0x45ad00:
            state = self.words(0x5c87e8)[0]; self.events.append('state:' + str(state))
            if state == 39:
                self.cause = 'intro39'; mu.emu_stop(); return
            self.ret(); return
        if address == 0x441090:
            key = self.string(args[0]).decode('ascii')
            if not self.event(key, 'intro' if key == 'intro' else 'welcome'): return
            self.ret(self.case[0] if key == 'intro' else self.case[1] if key == 'WelcomeGiven' else 0); return
        if address == 0x4b5470:
            if self.event('delete', 'delete'): self.ret(TEXT + 0x700)
            return
        if address == 0x4e58e4:
            if self.string(args[0]) != b'd/!state.dat': raise RuntimeError('저장 상태 이외 삭제 요청')
            self.ret(); return
        if address == 0x469db0:
            name = self.string(args[0]).decode('ascii')
            if not self.event('music:' + name): return
            if mutation == 'source2' and name == 'fanfare.mus': self.put(0x55a420, 2)
            if mutation == 'outcome2' and name == 'fanfare.mus': self.put(0x5c85ac, 2)
            self.ret(); return
        if address == 0x49a840:
            number = args[0]
            if number not in (1, 2): raise RuntimeError('명시 입력 밖 source TYPE')
            self.event('source:' + str(number)); self.ret(TEXT + 0xa00 + (number - 1) * 0x100); return
        if address == 0x459c60:
            expression = self.words(esp + 4, 9)
            if expression[:5] != (0xfffffffe, self.words(0x540cec)[0], 0xfffffffe, self.words(0x540cf8)[0], 0xfffffffd) or expression[6:] != (0xfffffffc, 0x5142a0, 0):
                raise RuntimeError('State3 source 배경 경로의 실제 인자 변경')
            if self.string(expression[5]) not in (b'SourceOne', b'SourceTwo') or self.string(expression[7]) != b'.gif': raise RuntimeError('State3 source 이름/확장자 변경')
            self.ret(TEXT + 0xc00); return
        if address == 0x41ab10: self.ret(self.case[10]); return
        if address == 0x4e2dc0:
            if not self.event('battleLeave', 'battleLeave'): return
            if args[0] != 9 or bytes(mu.mem_read(args[1], 5))[4] != 0: raise RuntimeError('원문 Battle14 종료 패킷 변경')
            self.ret(); return
        if address == 0x4b2df0:
            if args[0] != 0: raise RuntimeError('State3 cleanup 인자')
            if not self.event('prefix', 'prefix'): return
            for field in (0x5c85a4, 0x594fbc, 0x594fc8, 0x594fa4): self.put(field, 0)
            if mutation == 'prefix-outcome4': self.put(0x5c85ac, 4)
            self.ret(); return
        if address in (0x42e440, 0x4e22c0, 0x48f3b0, 0x432360, 0x41cc10):
            self.prefixCalls.append(f'{address:08x}:' + (':'.join(map(str, args[:2])) if address == 0x48f3b0 else ''))
            self.ret(); return
        if address == 0x4993c0: self.prefixCalls.append('004993c0:' + str(args[0])); self.ret(purge=4); return
        if address == 0x466280:
            self.huds += 1
            if args[:4] != (0x0200003d, 1, 0, 1): raise RuntimeError('State3 HUD 실제 인자')
            if self.event('hud1' if self.huds == 1 else 'hud2', 'hud1' if self.huds == 1 else 'hud2'): self.ret()
            return
        if address == 0x460730:
            if not self.event('title', 'title'): return
            if mutation == 'state2': self.put(0x5c87e8, 2)
            self.ret(); return
        if address == 0x4b72eb:
            self.events.append('changedState'); self.cause = 'changedState'; mu.emu_stop(); return
        if address == 0x463b20:
            if not self.event('close', 'close'): return
            # Guide의 실제 NULL 자연 반환 몸체는 아래 허용범위에서 그대로 실행한다.
        elif address == 0x4cf960:
            name = self.string(args[0]).decode('ascii')
            if not self.event('tell:' + name, 'tell:' + name): return
            if mutation == 'tips0' and name == 'UserNotUpdated': self.put(0x53190c, 0)
            if mutation == 'updated1' and name == 'Blank': self.put(0x54da8c, 1)
            self.ret(); return
        elif address == 0x4b5ba0:
            if not self.event('startup', 'startup'): return
            if mutation == 'first0': self.put(0x542530, 0)
            if mutation == 'tips1': self.put(0x53190c, 1)
            if mutation in ('editor1', 'editor1-stop'): self.put(0x5c85a4, 1)
            self.ret(self.case[2]); return
        elif address == 0x4414d0:
            if self.string(args[0]) != b'lastEditFort' or self.string(args[1]): raise RuntimeError('State3 lastEditFort 실제 인자')
            self.events.append('editorPlayers')
            if mutation == 'editor1-stop': self.cause = 'stop:editorPlayers'; mu.emu_stop(); return
            self.ret(); return
        elif address == 0x4904b0: self.ret(); return
        elif address == 0x4180d0: pass  #원문 기한 helper의 실제 x87 산술과 store를 실행한다.
        elif address == 0x460d70:
            self.events.append('wall'); self.ret(TEXT + 0x900); return
        elif address == 0x4410f0:
            if self.string(args[0]) != b'fortPal': raise RuntimeError('State3 팔레트 원문 키')
            self.ret(TEXT + 0x800); return
        elif address == 0x4b6100:
            if self.event('palette', 'palette'): self.ret()
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'State3 미검토 실행 주소 {address:08x}')

    def sample(self, label, case, control):
        """기대 모델 없이 독립 입력만 배치하고 실제 ret 또는 선택 경계 중단 뒤 원시 바이트를 수집한다."""
        self.case = case; self.events = []; self.prefixCalls = []; self.cause = 'normal'; self.huds = 0; self.active = False
        for index, base in enumerate(BLOCKS):  #다섯 블록의 부가 인자도 각각 구별한다.
            self.mu.mem_write(base, struct.pack('<36I', *[index * 1000 + word for word in range(36)]))
        fields = ((0x5c87e8, 3), (0x5c8758, case[6]), (0x55a420, case[7]), (0x5c85ac, case[8]),
                  (0x594fbc, case[9]), (0x594fc8, 0), (0x594fa4, 1), (0x5c85a4, 1), (0x594fb8, 0), (0x54db14, 0),
                  (0x594fa0, 0), (0x542530, case[3]), (0x54da8c, case[4]), (0x53190c, case[5]),
                  (0x5427ec, 0x0200003d), (0x562ca0 + 0x800, 0), (0x55b490, TEXT),
                  (0x55b494, 0 if case[-1] == 'pending-null' else TEXT + 0x200))
        for address, value in fields: self.put(address, value)  #명시 현재값만 새로 설치한다.
        for base, text in ((TEXT, b'Old'), (TEXT + 0x200, b'Next')):  #NUL 뒤 흔적의 불필요한 초기화를 검출한다.
            self.mu.mem_write(base, b'\xa5' * 256); self.mu.mem_write(base, text + b'\0')
        self.mu.mem_write(TEXT + 0x700, b'd/!state.dat\0'); self.mu.mem_write(TEXT + 0x800, b'fortpal\0')
        self.mu.mem_write(TEXT + 0x900, struct.pack('<d', 100.5)); self.mu.mem_write(TEXT + 0xa00, b'\0' * 4 + b'SourceOne\0')
        self.mu.mem_write(TEXT + 0xb00, b'\0' * 4 + b'SourceTwo\0'); self.mu.mem_write(TEXT + 0xc00, b'd/Source.gif\0')
        self.mu.mem_write(0x565e70, b'Previous\0' + b'?' * 71); self.mu.mem_write(0x544d58, b'\0' * 8)
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP): self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_FPCW, control); self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff); self.mu.reg_write(UC_X86_REG_ESP, STACK); self.put(STACK, STOP)
        self.active = True; self.mu.emu_start(0x4b6dd0, STOP, count=100000); self.active = False
        if self.cause == 'normal' and (self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4): raise RuntimeError('State3 정상 ret/스택 검증 실패')
        if self.cause == 'normal' and self.mu.reg_read(UC_X86_REG_FPTAG) != 0xffff: raise RuntimeError('State3 x87 스택 잔류')
        raw = ':'.join(bytes(self.mu.mem_read(base, 144)).hex() for base in BLOCKS)
        names = ':'.join(bytes(self.mu.mem_read(base, length)).hex() for base, length in ((TEXT, 256), (TEXT + 0x200, 256), (0x565e70, 80)))
        return '\t'.join((label, str(control), ','.join(map(str, case)), self.cause, raw,
                          ','.join(str(self.words(field)[0]) for field in WORDS), names,
                          bytes(self.mu.mem_read(0x544d58, 8)).hex(), ';'.join(self.events)))


def observe():
    """같은 의미 입력을 x87 두 제어워드에서 실행하고 매번 실제 정상/중단 결과를 수집한다."""
    oracle = Oracle(); lines = ['# label / control / intro,welcome,startup,first,updated,tips,phase,source,outcome,battle,sourceExists,mutation / status / 5x144 raw / source,outcome,first,updated,tips,editor / current:pending:background raw / activity double / actual trace']
    counts = Counter()
    for label, case in cases():  #변경/중단 입력을 무작정 전 조합으로 늘리지 않는다.
        first = oracle.sample(label, case, 0x27f); second = oracle.sample(label, case, 0x37f)
        if first.split('\t')[2:] != second.split('\t')[2:]: raise RuntimeError('State3 x87 제어워드 차이 ' + label)
        lines.extend((first, second)); counts[first.split('\t')[3]] += 2
    return '\n'.join(lines) + '\n', oracle, counts


def main():
    """최초 fixture/report를 덮어쓰지 않고 verify는 원시 CPU 관측까지 다시 실행해 대조한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); args = parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()): raise RuntimeError('State3 최초 관측 덮어쓰기 금지')
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, expected in report['sha256'].items():  #제품 코드가 아니라 원문/관측/도구 바이트를 고정한다.
            if digest(ROOT / name) != expected: raise RuntimeError('State3 독립 관측 SHA 불일치 ' + name)
    content, oracle, completions = observe()
    if args.verify:
        if content != FIXTURE.read_text(encoding='utf-8'): raise RuntimeError('State3 실제 CPU 재관측 차이')
        print(json.dumps({'verified': True, 'observations': sum(completions.values()), 'differences': 0, 'completions': dict(completions)}, ensure_ascii=False)); return
    FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_local_mission_transport.py', 'tools/decomp_native_main_menu_state.py', str(FIXTURE.relative_to(ROOT)).replace('\\', '/'))
    report = {'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entry': '004b6dd0',
              'uniqueInputs': len(cases()), 'observations': sum(completions.values()), 'completions': dict(completions),
              'instructions': oracle.instructions, 'boundaries': dict(oracle.boundaries), 'precisionDifferences': 0,
              'scope': 'State3 선택/직접쓰기/이름 첫BYTE/현재 재읽기/5블록 공통ret와 Activity의 실제 x87 산술. 빈 Guide와 Battle14 5BYTE패킷 생성은 실제 body. cleanup/Chat/listener/Core/Renderer/Chunk/options/HUD/큰Tell/startup/read/음향/TYPE조회/경로/파일존재/삭제/팔레트/EditorPlayers는 명시 장치. State39는 도달점에서 멈추며 전체 성공이 아니다.',
              'fixtureTiming': '제품 초안 작성 후의 독립 PE 재검증이며 제품의 예상값으로 fixture를 만들지 않았다.',
              'gameExecuted': False, 'guiExecuted': False, 'osExecuted': False, 'originalFilesMutated': False,
              'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'completions', 'instructions')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
