#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제State1/2의기존Fort Editor설치외곽을전체전환함수완료와구별해최초고정한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

HEADERS, OWNER, BATTLE_OWNER = 0x5c89f0, 0x540cac, 0x540c70  # 현재Template9와각호출전에다시읽는두owner다.
CURRENT, PENDING, HANDLE, PATH = TEXT, TEXT + 0x200, TEXT + 0x400, TEXT + 0x600  # 서로다른256BYTE이름/handle/명시작업본경로다.
STATES, CLOCK, FORMAT = POOL + 0x1000, POOL + 0x2000, POOL + 0x3000  #9×28State/실제double시각/명시format결과다.
FIELDS = (0x5c87e8, 0x5c8878, 0x5c86c8, 0x5c85a4, 0x594fb8, 0x594fa0, 0x594fa4, 0x54db14, OWNER, BATTLE_OWNER, 0x540c74, 0x542640, 0x540bc0, 0x540bc4, 0x54c8a0, 0x594fc4, 0x5c85ac, 0x55a420, 0x55b490, 0x55b494, 0x540ca8, 0x565e70)  # 현재DWORD원시관측순서다.
RANGES = ((0x4b6dd0, 0x4b6e62), (0x4b6fa9, 0x4b7028), (0x4b7169, 0x4b717f), (0x4b71a5, 0x4b7244), (0x4b72eb, 0x4b743b), (0x4b5ca0, 0x4b5ced), (0x4b5d57, 0x4b5d7d), (0x4b5e0c, 0x4b5e2a), (0x4b5e80, 0x4b5eb8), (0x4bd340, 0x4bd35f), (0x4b5ba0, 0x4b5c99))  # 실제진입부터선택하위·startup분기까지만허용한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/existing-editor-install-outer-x86.tsv'  # 최초독립분석관측이며제품전환모듈은작성하지않는다.
REPORT = ROOT / 'cpppj/existing-editor-install-outer-native.json'  # 고정context/명시하위/도달checkpoint/원문SHA를보존한다.


def cases():
    """실제선택State1과동일case2·현재이름·State·startup관문을독립입력한다."""
    rows = []
    # case2는메뉴mode2와같은숫자라는이유로추정하지않고동일switch의대조입력으로둔다.
    for state, names, owner, flags, startup in itertools.product((1, 2), (0, 1, 2), (1, 8), (0, 255), (0, 1, 2)):
        rows.append((state, names, owner, flags, startup, 0, 1, 1, 'none'))
    # 기존범위를벗어난NewFort/무효파일/미션우회는성공이아닌별도도달점으로고정한다.
    rows.extend(((1, 3, 1, 255, 0, 0, 1, 1, 'none'), (1, 4, 1, 255, 0, 0, 1, 1, 'none'), (1, 0, 1, 255, 0, 0, 0, 1, 'none'), (1, 0, 1, 255, 0, 0, 1, 0, 'none'), (1, 0, 1, 255, 0, 1, 1, 1, 'none'), (1, 2, 1, 255, 0, 1, 1, 1, 'guide-clear')))
    mutations = ('poll-stop', 'cleanup-stop', 'name-stop', 'load-stop', 'state1-stop', 'notify1-stop', 'subscriber-stop', 'player-stop', 'recover-stop', 'world-stop', 'deck-stop', 'camera-stop', 'production-stop', 'corewrite-stop', 'snapshot-stop', 'state2-stop', 'notify2-stop', 'startup-stop', 'lastedit-stop', 'editorplayers-stop', 'load-owner', 'state1-owner', 'notify1-owner', 'subscriber-owner', 'player-owner', 'coreread-owner', 'recover-owner', 'world-owner', 'deck-owner', 'camera-owner', 'production-owner', 'corewrite-owner', 'snapshot-owner', 'state2-owner', 'notify2-owner', 'startup-owner', 'snapshot-name', 'startup-name', 'lastedit-name', 'state2-returnother', 'state2-zero', 'saved-pending', 'saved-owner', 'saved-stop', 'startup-option-stop', 'startup-option0', 'production-editor0', 'snapshot-editor0', 'startup-editor0')
    # callback한개에서현재값을바꾸고그뒤재조회와분기는PE본문에맡긴다.
    for mutation in mutations:
        rows.append((1, 2, 1, 255, 2 if mutation.startswith('saved') else 0, 0, 1, 1, mutation))
    rows.extend(((1, 0, 1, 255, 3, 0, 1, 1, 'none'), (1, 0, 1, 255, 4, 0, 1, 1, 'none')))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """원문외곽·실제기존파일선택/State bit·startup분기만실행하고거대한child는명시입력경계로둔다."""
    def __init__(self):
        """FS한페이지만추가하고보호PE/기존가상주소배치를그대로빌린다."""
        super().__init__()
        self.mu.mem_map(0, 4096)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.total_instructions = 0
        self.boundaries, self.completions = Counter(), Counter()

    def guard(self, mu, access, address, size, value, data):
        """허용범위내실제CPU쓰기만통과시키며전체World/OS로확장하지않는다."""
        ranges = ((0, 4), (STACK - 0x1800, STACK + 8), (0x5c86c8, 0x5c8908), (STATES, STATES + 9 * 28), *((field, field + 4) for field in FIELDS))
        if not any(low <= address and address + size <= high for low, high in ranges):
            raise RuntimeError(f'Editor외곽CPU쓰기범위밖 {address:08x}/{size}')

    def record(self, kind, text):
        """실제호출인자와1기반경계순번을원문순서로기록한다."""
        self.calls[kind] += 1
        self.boundaries[kind] += 1
        self.events.append(text)
        return self.calls[kind]

    def finish(self, cause):
        """부분checkpoint/명시중단을전체함수의정상ret와구별한다."""
        self.cause = cause
        self.mu.emu_stop()

    def mutate(self, boundary):
        """하위의명시현재값효과만입력하며외곽기대결과는계산하지않는다."""
        mutation = self.case[-1]
        if mutation == boundary + '-owner':
            self.put(OWNER, self.other)
        elif mutation == boundary + '-name':
            self.mu.mem_write(CURRENT, b'ChangedDuringInstall\0')
        elif mutation == boundary + '-editor0':
            self.put(0x5c85a4, 0)
        elif mutation == 'state2-returnother' and boundary == 'state2':
            self.return_state = self.other
        elif mutation == 'state2-zero' and boundary == 'state2':
            self.return_state = None
        elif mutation == 'saved-pending' and boundary == 'saved':
            self.mu.mem_write(PENDING, b'RestoredPending\0')
        elif mutation == 'startup-option0' and boundary == 'startup-option':
            self.startup_option = 0
        elif mutation == boundary + '-stop':
            self.finish(boundary)

    def effect(self, name, text, purge=0, value=0):
        """명시하위는고정효과뒤실제cdecl/thiscall규약으로만복귀한다."""
        self.record(name, text)
        self.mutate(name)
        if not self.cause:
            self.ret(value, purge)

    def slot(self, ecx):
        """현재this의실제20BYTE주소에서슬롯인자를관측한다."""
        if (ecx - HEADERS) % 20 or not 0 <= (ecx - HEADERS) // 20 <= 8:
            raise RuntimeError('Editor외곽현재Template슬롯범위')
        return (ecx - HEADERS) // 20

    def instruction(self, mu, address, size, data):
        """실제State1/2진입과선택된기존Editor본문만CPU로실행한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 8000:
            raise RuntimeError('Editor외곽명령상한초과')
        ecx = mu.reg_read(UC_X86_REG_ECX)
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 6)
        if address == 0x4b743b:
            self.finish('checkpoint-editor-children-returned')
            return
        if address in (0x4b734b, 0x4b5e2a, 0x4b5ced, 0x4b8708):
            self.finish({0x4b734b: 'outside-new-name', 0x4b5e2a: 'outside-new-fort', 0x4b5ced: 'outside-invalid-file', 0x4b8708: 'outside-mission'}[address])
            return
        if address == 0x4b73fe and not mu.reg_read(UC_X86_REG_EAX):
            self.finish('state0')
            return
        if address == 0x4b7355:
            self.record('install', 'INSTALL:alternate-before=' + str(self.words(args[0], 1)[0]))
        if address == 0x4b5ba0:
            self.record('startup', 'STARTUP:editor=' + str(self.words(0x5c85a4, 1)[0]))
            self.mutate('startup')
            if self.cause:
                return
        if address == 0x4437f0:
            self.effect('poll', 'POLL')
        elif address == 0x45ad00:
            self.effect('trace', 'TRACE:' + self.string(args[0]).hex())
        elif address == 0x4b2df0:
            assert args[0] == 0
            self.effect('cleanup', 'CLEANUP:0')
        elif address in (0x42e440, 0x4e22c0, 0x41cc10):
            self.effect(f'pre{address:08x}', f'PRE:{address:08x}')
        elif address == 0x48f3b0:
            assert args[:2] == (1, 1)
            self.effect('owners', 'OWNERS:1:1')
        elif address == 0x4993c0:
            assert ecx == POOL + 0x6000 and args[0] == 1
            self.effect('clockreset', 'CLOCKRESET:1', 4)
        elif address == 0x432360:
            assert ecx == 0x54c8a0
            self.effect('preserve', 'PRESERVE:54c8a0')
        elif address == 0x466280:
            assert args[:4] == (POOL + 0x6100, 1, 0, 1)
            self.effect('screen', 'SCREEN:1:0:1')
        elif address == 0x4e65ed:
            first, second = self.string(args[0]), self.string(args[1])
            self.effect('compare', 'COMPARE:' + first.hex() + ':' + second.hex(), value=int(first.lower() != second.lower()))
        elif address == 0x460c40:
            self.effect('missionname', 'MISSIONNAME:' + self.string(args[0]).hex(), value=0)
        elif address == 0x463b20:
            assert ecx == 0x562ca0
            if self.case[-1] == 'guide-clear':
                self.put(0x594fa0, 0)
            self.effect('guide', 'GUIDECLOSE')
        elif address == 0x460980:
            name = self.string(args[0])
            self.mu.mem_write(CURRENT, name + b'\0')
            self.effect('name', 'NAME:' + name.hex())
        elif address == 0x460680:
            self.effect('path', 'PATH:' + self.string(PATH).hex(), value=PATH)
        elif address == 0x41ab10:
            assert self.string(args[0]) == self.string(PATH)
            self.effect('exists', 'EXISTS:' + str(self.case[6]), value=self.case[6])
        elif address == 0x4bf550:
            self.effect('load', 'LOAD:' + str(self.slot(ecx)) + ':' + self.string(args[0]).hex(), 4)
        elif address == 0x41aad0:
            self.effect('valid', 'VALID:' + str(self.slot(ecx)) + ':' + str(self.case[7]), value=self.case[7])
        elif address == 0x4bd550:
            slot = self.slot(ecx)
            self.return_state = slot
            index = self.record('state', 'STATE:' + str(slot))
            self.mutate('state' + str(index))
            if not self.cause:
                self.ret(0 if self.return_state is None else STATES + self.return_state * 28)
        elif address == 0x4bd540:
            slot = self.slot(ecx)
            index = self.record('notify', 'NOTIFY:' + str(slot))
            self.mutate('notify' + str(index))
            if not self.cause:
                self.ret()
        elif address == 0x4bd450:
            slot = self.slot(ecx)
            assert args[:2] == (0x540c74, HANDLE)
            self.put(args[0], 0x100 + slot)
            self.mu.mem_write(args[1], f'Subscriber{slot}'.encode('ascii') + b'\0')
            self.effect('subscriber', 'SUBSCRIBER:' + str(slot) + ':540c74:handle', 8)
        elif address == 0x4414d0:
            key, value = self.string(args[0]), self.string(args[1])
            assert key in (b'fort.mySubHandle', b'lastEditFort')
            self.config[key] = value
            self.effect('lastedit' if key == b'lastEditFort' else 'handleconfig', 'CONFIG:' + key.hex() + ':' + value.hex())
        elif address == 0x460730:
            self.effect('title', 'TITLE:' + self.string(CURRENT).hex() + ':' + self.string(HANDLE).hex())
        elif address == 0x490250:
            self.effect('player', 'PLAYER:' + str(args[0]))
        elif address == 0x4beca0:
            assert args[0] == 0
            self.effect('coreread', 'COREREAD:' + str(self.slot(ecx)) + ':0', 4)
        elif address == 0x4b5ec0:
            self.effect('recover', 'RECOVER:owner=' + str(self.words(OWNER, 1)[0]))
        elif address == 0x4b5f70:
            self.effect('world', 'WORLD:' + str(args[0]))
        elif address == 0x4bf190:
            self.effect('deck', 'DECK:' + str(self.slot(ecx)))
        elif address in (0x4923e0, 0x4340d0, 0x47d890, 0x4beb30, 0x4bf9a0):
            name = {0x4923e0: 'priests', 0x4340d0: 'camera', 0x47d890: 'production', 0x4beb30: 'corewrite', 0x4bf9a0: 'snapshot'}[address]
            self.effect(name, name.upper() + (':' + str(self.slot(ecx)) if address in (0x4beb30, 0x4bf9a0) else ''))
        elif address == 0x441090:
            assert self.string(args[0]) == b'startupStateCheck'
            self.record('startup-option', 'STARTUP-OPTION:' + str(self.startup_option))
            self.mutate('startup-option')
            if not self.cause:
                self.ret(self.startup_option)
        elif address == 0x4b5890:
            self.effect('saved', 'SAVED-STATE:' + str(self.saved_stamp), value=self.saved_stamp)
        elif address == 0x460d70:
            self.mu.mem_write(args[0], struct.pack('<d', self.now))
            self.effect('time', 'TIME:' + struct.pack('<d', self.now).hex(), value=args[0])
        elif address == 0x4de9a0:
            assert ecx == 0x5e22b8
            self.effect('translate', 'TRANSLATE:' + self.string(args[0]).hex(), 4, args[0])
        elif address == 0x4a89c0:
            index = self.calls['format']
            text = ('ExplicitButtons' if index == 0 else 'ExplicitWarning').encode('ascii')
            self.mu.mem_write(args[0], text + b'\0')
            tail = ':elapsed64=' + bytes(mu.mem_read(mu.reg_read(UC_X86_REG_ESP) + 12, 8)).hex() if index else ''
            self.effect('format', 'FORMAT:' + self.string(args[1]).hex() + tail, value=args[0])
        elif address == 0x4cf550:
            assert args[1:3] == (0, 0)
            self.effect('tell', 'TELL:' + self.string(args[0]).hex() + ':0:0')
        elif address == 0x4904b0:
            self.effect('editorplayers', 'EDITORPLAYERS:handle=' + self.string(HANDLE).hex())
        elif address == 0x4e0620:
            self.effect('assert', 'ASSERT:' + str(args[2]))
        elif not any(low <= address < high for low, high in RANGES):
            raise RuntimeError('Editor외곽원문범위밖 ' + hex(address))

    def sample(self, case, control):
        """전체함수첫진입의명시State1/2context로시작해부분checkpoint까지관측한다."""
        self.case = case
        state, names, owner, flags, startup, mission, exists, valid, mutation = case
        self.other = 8 if owner == 1 else 1
        self.calls, self.events, self.config, self.cause, self.instructions = Counter(), [], {}, '', 0
        self.startup_option = int(startup != 0)
        self.saved_stamp = 0 if startup in (0, 1) else 100
        self.now = {0: 700.0, 1: 700.0, 2: 700.0, 3: 100.0, 4: 2800.0}[startup]
        self.put(0, 0x12345678)
        self.mu.mem_write(STACK - 0x1800, b'\xcd' * 0x1808)
        # 원래요청144BYTE와별개next/work영역을입력해실제두번복사를검사한다.
        for base, marker in ((0x5c87e8, 0x31), (0x5c8878, 0x61), (0x5c86c8, 0x91)):
            self.mu.mem_write(base, bytes((marker + byte) & 255 for byte in range(144)))
        self.put(0x5c87e8, state)
        # 이전Battle/승패/UI이력분기는이기존Editor범위의명시context에서비활성이다.
        for field in FIELDS[3:]:
            self.put(field, 0)
        self.put(0x5c8758, 4)
        self.put(0x594fbc, 0)
        self.put(0x594fa0, mission)
        self.put(OWNER, owner)
        self.put(BATTLE_OWNER, 7)
        self.put(0x540c74, 17)
        self.put(0x55b490, CURRENT)
        self.put(0x55b494, PENDING)
        self.put(0x540ca8, HANDLE)
        self.put(0x59a8bc, POOL + 0x6000)
        self.put(0x5427ec, POOL + 0x6100)
        self.mu.mem_write(HEADERS, bytes((0x50 + byte) & 255 for byte in range(180)))
        # 각현재Template의manager는명시비영이며실제4bd340은진단없이항상0을반환한다.
        for slot in range(9):
            self.put(HEADERS + slot * 20, POOL + 0x7000 + slot * 0x20)
            self.mu.mem_write(STATES + slot * 28, bytes((0x80 + slot + byte * 7) & 255 for byte in range(28)))
            self.put(STATES + slot * 28, flags, 1)
        pair = ((b'Existing', b'Existing'), (b'Existing', b'eXiStInG'), (b'Old', b'Existing'), (b'', b''), (b'.temp', b'.TEMP'))[names]
        #256BYTE주소를보존하고NUL뒤의주변BYTE도독립초기화한다.
        for base, text in ((CURRENT, pair[0]), (PENDING, pair[1]), (HANDLE, b'BeforeSubscriber'), (PATH, b'EditorFiles/Existing.fort')):
            self.mu.mem_write(base, b'\xee' * 256)
            self.mu.mem_write(base, text + b'\0')
        self.mu.mem_write(FORMAT, b'\xee' * 256)
        # 이전관측의레지스터와x87태그/정밀도에기대지않는다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.put(STACK, STOP)
        self.mu.emu_start(0x4b6dd0, STOP)
        if not self.cause:
            raise RuntimeError('Editor외곽이명시checkpoint없이전체ret에도달함')
        self.completions[self.cause] += 1
        values = ','.join(str(self.words(field, 1)[0]) for field in FIELDS)
        raw = ':'.join((bytes(self.mu.mem_read(HEADERS, 180)).hex(), bytes(self.mu.mem_read(STATES, 9 * 28)).hex(), *(bytes(self.mu.mem_read(base, 256)).hex() for base in (CURRENT, PENDING, HANDLE)), *(bytes(self.mu.mem_read(base, 144)).hex() for base in (0x5c87e8, 0x5c8878, 0x5c86c8))))
        configs = ';'.join(key.hex() + '=' + value.hex() for key, value in sorted(self.config.items()))
        return '\t'.join((str(control), ','.join(map(str, case)), self.cause, values, raw, configs, ';'.join(self.events)))


def main():
    """제품외곽제안전에최초관측을고정하고그뒤에는SHA만확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 원문과도구/관측을함께잠가후속코드에맞춘fixture수정을거부한다.
        for name, expected in report['sha256'].items():
            if digest(ROOT / name) != expected:
                raise RuntimeError('Editor외곽최초SHA불일치 ' + name)
        print('기존Editor외곽최초원문보존:', report['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('Editor외곽최초fixture/report덮어쓰기금지')
    oracle, inputs = Oracle(), cases()
    lines = ['# control / state,nameCase,owner,stateFirst,startupMode,inMission,exists,valid,mutation / checkpoint-or-stop / currentDWORDs / raw / configs / trace']
    # x87경과분비교도두정밀도에서실제PE로각각실행한다.
    for case in inputs:
        first, second = oracle.sample(case, 0x27f), oracle.sample(case, 0x37f)
        if first.split('\t')[1:] != second.split('\t')[1:]:
            raise RuntimeError('Editor외곽정밀도차이 ' + str(case))
        lines.extend((first, second))
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_existing_editor_install_outer.py', 'cpppj/tests/fixtures/existing-editor-install-outer-x86.tsv')
    report = {'status': '단위383후속외곽·분석전용·제품미적용', 'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entry': '004b6dd0', 'checkpoint': '004b743b(004904b0명시하위복귀뒤·후속UI전)', 'context': 'currentState1/동일case2,previousState4,battle0,명시공통cleanup경계,실제비영헤더/기존파일명/manager,기존파일유효경로. mode2는AskEditFort목록모드이며실제선택nextState는1.', 'ranges': [[f'{low:08x}', f'{high:08x}'] for low, high in RANGES], 'uniqueInputs': len(inputs), 'observations': len(inputs) * 2, 'completions': dict(oracle.completions), 'instructions': oracle.total_instructions, 'precisionDifferences': 0, 'boundaries': dict(oracle.boundaries), 'currentFields': [f'{field:08x}' for field in FIELDS], 'scope': '실제전체함수진입prologue/State1·2switch/144BYTE복사/Editor모드설정/기존파일선택4b5ca0·4bd340/State flag1설정·해제/현재owner·이름재조회/Subscriber·config·제목호출/실제startup4b5ba0의관문·x87경과분·Tell분기. cleanup/CRT비교/파일·Guide·게임하위·Subscriber·State·Notify·시계/저장파일읽기5890/번역·format·Tell/EditorPlayers는명시하위다. 004b743b부분checkpoint는전체4b6dd0정상ret/loaded완료/후속UI/OSSEH실행이아니다.', 'gameExecuted': False, 'guiExecuted': False, 'osExecuted': False, 'filesMutated': False, 'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'completions', 'instructions')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
