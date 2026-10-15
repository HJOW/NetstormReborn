# 원본 TYPE 후처리의 mana 기본값을 정적으로 준비한 입력으로 격리 관찰한다.
import csv
import itertools
import json
import socket
import struct
import sys
from pathlib import Path

# 기존 격리 메모리 도구를 읽기 전용으로 사용한다.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TYPES, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP


class TypeProbe(MemoryOracle):
    """TYPE 후처리는 실제 명령이며 CRT ASCII 소문자화와 프레임 후처리는 명시 외부 경계다."""

    def __init__(self):
        """같은 PC의 정밀 함수 범위와 격리 TYPE 한 개를 준비한다."""
        super().__init__()
        self.allowed = []  # TYPE 후처리 본체 밖의 OS/미검토 명령을 거부한다.
        self.lower_calls = 0  # 대체한 CRT ASCII 소문자화 횟수다.
        self.frame_calls = 0  # 이번 관찰 범위 밖 프레임 후처리 진입 횟수다.
        self.native_calls = 0  # 실제 원본 후처리 본체 진입 횟수다.
        self.target = TYPES + 180 * 500  # 실제 PE와 겹치지 않는 500바이트 TYPE 입력 주소다.
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            # 동일 PC의 정밀 본체 범위를 그대로 사용한다.
            for row in csv.DictReader(stream, delimiter='\t'):
                if int(row['entry'], 16) == 0x49b0d0:
                    # 분리된 함수 범위도 명시적으로 허용한다.
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'))
                        self.allowed.append((low, high + 1))
        self.put(0x59ab20, TYPES)
        # 이름/간헐천 전용 분기를 피할 명시 타입 전역 입력이다.
        for address in (0x541074, 0x541078, 0x54107c, 0x541080, 0x541088, 0x541240, 0x5412d0):
            self.put(address, 0xffffffff)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write_guard)

    def write_guard(self, mu, access, address, size, value, data):
        """실제 TYPE와 함수 스택 밖의 쓰기를 거부하여 다른 전역 효과를 숨기지 않는다."""
        if not (self.target <= address and address + size <= self.target + 500 or
                STACK - 4096 <= address and address + size <= STACK + 16):
            raise RuntimeError(f'TYPE 관찰 범위 밖 쓰기 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """CRT/프레임 경계를 대체하며 나머지 계산과 문자열 쓰기는 실제 PE에서 수행한다."""
        if address == 0x49b0d0:
            self.native_calls += 1
        elif address == 0x4e53dc:
            value = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 1)[0]
            if value >= 128:
                raise RuntimeError('비ASCII CRT 입력은 관찰 범위 밖이다')
            self.lower_calls += 1
            self.ret(value + 32 if 65 <= value <= 90 else value)
        elif address == 0x49b060:
            self.frame_calls += 1
            self.ret()
        elif not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError(f'미검토 TYPE 명령 {address:08x}')

    def run(self, group, flags, level, theme, mana):
        """명시 속성 입력을 공급하고 원본이 쓴 NUL 종료 문자열을 읽는다."""
        self.mu.mem_write(self.target, bytes(500))
        self.put(self.target + 0x9c, group)
        self.put(self.target + 0xec, flags)
        self.put(self.target + 0x94, level)
        self.put(self.target + 0x98, theme)
        self.put(self.target + 0xc4, 0x3f800000)
        self.mu.mem_write(self.target + 0xa0, mana.encode('ascii') + b'\0')
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, self.target)
        self.mu.mem_write(STACK, struct.pack('<I', STOP))
        self.mu.emu_start(0x49b0d0, STOP, count=10000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('TYPE 반환/스택 오류')
        return self.string(self.target + 0xa0).hex()


# Git에 보존하는 독립 관찰과 실행 근거 보고서다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/type-required-energy-x86.tsv'
REPORT = ROOT / 'cpppj/type-required-energy.json'
# 실제 PE와 함수 범위/대체 경계/제품 구현의 현재 바이트를 함께 감사한다.
SOURCES = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv',
           'tools/decomp_type_required_energy.py', 'tools/decomp_priestmenu.py',
           'cpppj/src/o/RawTypeRequiredEnergy.h', 'cpppj/src/o/RawTypeRequiredEnergy.cpp',
           'cpppj/src/client/AiTypeRequiredEnergy.h', 'cpppj/src/client/AiTypeRequiredEnergy.cpp',
           'cpppj/tests/fixtures/type-required-energy-x86.tsv')


def generate():
    """유효한 명시 입력마다 원본 문자열을 얻고 대체 경계를 구분해 보존한다."""
    oracle = TypeProbe()
    rows = []  # 다음 단위의 입력과 실제 출력이며 기대 문자열 계산은 포함하지 않는다.
    # 기본/제외/group10/음수 및 범위 밖 level/문자표 NUL/명시 문자열을 교차한다.
    for values in itertools.product((0, 10), (0, 0x200000), (-2, -1, 0, 1, 2, 3, 98), range(12), ('', 's', 'S', 'WRTS', 'ws', 'Sss', '012345678')):
        rows.append([*values, oracle.run(*values)])
    if oracle.native_calls != len(rows) or oracle.frame_calls != len(rows):
        raise RuntimeError('TYPE 실제 진입/프레임 경계 수 오류')
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(('group', 'flags', 'level', 'theme', 'mana', 'nativeHex'))
        writer.writerows(rows)
    facts = dict(host=socket.gethostname(), date='2026-10-15', observations=len(rows),
                 status='native-postprocess-observed-product-unbuilt', normal_return=True, os_calls=0,
                 native_entry='0049b0d0', native_calls=oracle.native_calls,
                 substitutions={'004e53dc-ASCII-tolower': oracle.lower_calls, '0049b060-frame-postprocess': oracle.frame_calls},
                 limits=['mana AST parsing and zero constructor are supplied inputs, not native parser observations',
                         'ASCII CRT conversion and frame postprocess are explicit external substitutions',
                         'Only TYPE500 and stack writes allowed; no original game or OS execution',
                         'Product comparison and actual188TYPE bindings require separate C++ tests'],
                 sha256={name: digest(ROOT / name) for name in SOURCES})
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'원본 mana 후처리 관찰 {len(rows)}개, CRT 경계 {oracle.lower_calls}회, 프레임 경계 {oracle.frame_calls}회')


def verify():
    """보존 자료의 SHA·고유 입력·정상 반환·대체 경계 수를 확인한다."""
    facts = json.loads(REPORT.read_text(encoding='utf-8'))
    # 원본 및 현재 관찰기/제품 바이트가 보고서와 같은지 확인한다.
    for name, expected in facts['sha256'].items():
        if digest(ROOT / name) != expected:
            raise RuntimeError('mana 근거 SHA 불일치: ' + name)
    with FIXTURE.open(encoding='utf-8') as stream:
        rows = list(csv.reader(stream, delimiter='\t'))[1:]
    if len(rows) != facts['observations'] or len({tuple(row[:5]) for row in rows}) != len(rows):
        raise RuntimeError('mana 관찰 수/고유 입력 오류')
    if not facts['normal_return'] or facts['os_calls'] or facts['native_calls'] != len(rows):
        raise RuntimeError('mana 정상 반환/원본 진입 오류')
    if facts['substitutions']['0049b060-frame-postprocess'] != len(rows):
        raise RuntimeError('mana 프레임 대체 경계 오류')
    print(f'mana 원본 관찰/SHA 감사 통과: {len(rows)}개')


if __name__ == '__main__':
    if sys.argv[1:] == ['--verify']:
        verify()
    elif not sys.argv[1:]:
        generate()
    else:
        raise SystemExit('사용법: decomp_type_required_energy.py [--verify]')
