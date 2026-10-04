#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 프로세스를 시작하지 않고, 허용한 순수 함수의 x86 기계어를 격리 에뮬레이션한다.

두 판본의 반환값·출력 버퍼·스택 복구를 비교해 C++ 검증용 TSV를 만든다.
게임 진입점·OS API·파일·창·소켓을 실행하는 경로는 허용하지 않는다.
ASCII CRT toupper/strnicmp와 assert 보고만 명시적으로 대체한다.

python -m pip install --target extracted/oracle-python -r tools/requirements-decomp-oracle.txt
python tools/decomp_oracle.py
"""
import argparse
import csv
import hashlib
import importlib.metadata
import json
from pathlib import Path
import random
import struct
import sys

# 이 도구가 쓰는 저장소 루트와 격리된 선택적 Python 의존성 경로.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 모든 메모리는 에뮬레이터 내부 주소다. 호스트 메모리·원본 파일은 수정하지 않는다.
SCRATCH = 0x10000000
# 정상 함수 반환을 확인하는 가상 주소.
STOP = 0x20000000
# 인자·반환 주소가 놓이는 가상 스택.
STACK = 0x3000f000
# type +0x124가 가리키는 프레임 코드 배열.
FRAME_ARRAY = SCRATCH + 0x1000
# 설정 텍스트·키·출력 버퍼의 격리된 주소.
CONFIG_TEXT = SCRATCH + 0x2000
CONFIG_KEY = SCRATCH + 0x8000
CONFIG_OUT = SCRATCH + 0x10000
# 재현 가능한 생성 입력 시드. 기계어 반환값이 기대값이며 Python은 구현 모델을 만들지 않는다.
INPUT_SEED = 0x10781072
# 서로 다른 반복 입력에서 읽기 전용 코드의 실행을 제한한다.
MAX_INSTRUCTIONS = 200000


class Oracle:
    """PE를 가상 메모리로 읽고 검토된 함수 몸체·CRT 대체만 실행한다."""
    def __init__(self, edition, pairs):
        """판본별 허용 함수·실제 함수 몸체 범위·호스트 없는 가상 메모리를 준비한다."""
        self.edition = edition
        binary = ROOT / ('originals/Netstorm.exe' if edition == 'originals' else 'originalCD/NETSTORM.EXE')
        self.sha256 = hashlib.sha256(binary.read_bytes()).hexdigest()
        pe = pefile.PE(str(binary))
        base = pe.OPTIONAL_HEADER.ImageBase
        image = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(base, (len(image) + 4095) & ~4095)
        self.mu.mem_write(base, image)
        self.mu.mem_map(SCRATCH, 0x20000)
        self.mu.mem_map(STOP, 0x1000)
        self.mu.mem_map(0x30000000, 0x10000)
        with (ROOT / f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(fp, delimiter='\t')}
        self.entries = {p['name']: int(p[edition], 16) for p in pairs}
        self.allowed = []
        # 큐레이션된 함수 몸체만 허용한다. CD판의 ConfigFindKey 줄 이동 보조 함수도 읽기 함수다.
        for address in list(self.entries.values()) + ([0x42a240] if edition == 'originalCD' else []):
            for part in facts[address]['ranges'].split(';'):
                lo, hi = (int(x, 16) for x in part.split('-'))
                self.allowed.append((lo, hi + 1))
        self.stubs = ({0x4e5f7b: 'toupper', 0x4e66ed: 'strnicmp', 0x4e0620: 'assert'}
                      if edition == 'originals' else
                      {0x4f24c0: 'toupper', 0x4f3fb0: 'strnicmp', 0x490040: 'assert'})
        self.assertions = 0
        self.mu.hook_add(UC_HOOK_CODE, self.on_instruction)

    def read_string(self, address):
        """지정된 가상 버퍼의 NUL 종료 문자열만 최대 8KB 읽는다."""
        result = bytearray()
        # 복사된 입력·출력의 최대 길이를 제한한다.
        for i in range(8192):
            byte = self.mu.mem_read(address + i, 1)[0]
            if byte == 0:
                return bytes(result)
            result.append(byte)
        raise RuntimeError('NUL 종료 없는 oracle 문자열')

    def on_instruction(self, mu, address, size, _data):
        """CRT·assert 대체 또는 허용 몸체인지 확인한다. 나머지는 즉시 실패한다."""
        stub = self.stubs.get(address)
        if stub:
            esp = mu.reg_read(UC_X86_REG_ESP)
            ret, a, b, n = struct.unpack('<IIII', mu.mem_read(esp, 16))
            if stub == 'toupper':
                value = a - 32 if 97 <= a <= 122 else a
            elif stub == 'strnicmp':
                left, right = self.read_string(a)[:n].lower(), self.read_string(b)[:n].lower()
                value = (left > right) - (left < right)
            else:
                self.assertions += 1
                value = 0 # 오류 보고 UI만 대체한다. 검색 실패의 -1은 원본 기계어가 정한다.
            mu.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
            mu.reg_write(UC_X86_REG_ESP, esp + 4)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        if not any(lo <= address and address + size <= hi for lo, hi in self.allowed):
            raise RuntimeError(f'허용하지 않은 실행 주소: {self.edition} {address:08x}')

    def call(self, name, args, this=0, purge=0):
        """원본 x86 호출 규약으로 실행하고 정상 반환·ret N을 별도로 검사한다."""
        # 일반 레지스터를 매번 초기화해 이전 호출의 흔적이 결과에 영향을 주지 않게 한다.
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(reg, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, this)
        words = [STOP] + [arg & 0xffffffff for arg in args]
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * len(words), *words))
        self.mu.emu_start(self.entries[name], STOP, timeout=500000, count=MAX_INSTRUCTIONS)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError(f'{name}: 제한 내에 반환하지 않음')
        if self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError(f'{name}: 스택 복구 크기 불일치')
        return self.mu.reg_read(UC_X86_REG_EAX)

    def frame(self, name, codes, args, purge):
        """32비트 type 배치 +0x114/+0x124에 입력 배열을 주고 반환 인덱스를 읽는다."""
        self.mu.mem_write(SCRATCH, bytes(500))
        self.mu.mem_write(SCRATCH + 0x114, struct.pack('<I', len(codes) // 4))
        self.mu.mem_write(SCRATCH + 0x124, struct.pack('<I', FRAME_ARRAY))
        if codes:
            self.mu.mem_write(FRAME_ARRAY, codes)
        value = self.call(name, args, SCRATCH, purge)
        return value if value < 0x80000000 else value - 0x100000000

    def config(self, text, key):
        """실제 FindKey·GetRaw 기계어의 위치와 출력 버퍼를 함께 확인한다."""
        self.mu.mem_write(CONFIG_TEXT, text + b'\0')
        self.mu.mem_write(CONFIG_KEY, key + b'\0')
        self.mu.mem_write(CONFIG_OUT, bytes(8192))
        self.mu.mem_write(SCRATCH, struct.pack('<I', CONFIG_TEXT))
        offset = self.call('ConfigFindKey', [CONFIG_KEY, CONFIG_TEXT])
        found = self.call('ConfigGetRaw', [CONFIG_KEY, CONFIG_OUT], SCRATCH, 8)
        if bool(offset) != bool(found):
            raise RuntimeError('FindKey/GetRaw의 발견 여부 불일치')
        return (offset - CONFIG_TEXT, self.read_string(CONFIG_OUT)) if found else None


def inputs():
    """경계·중복·부호 확장·빈 배열과 시드 고정 임의 입력을 생성한다."""
    rng = random.Random(INPUT_SEED)
    # 프레임 순서, 중복, 플래그 bit 7의 부호 확장 경계를 명시적으로 포함한다.
    for index in range(512):
        count = rng.randrange(0, 33)
        codes = bytes(rng.randrange(256) for _ in range(count * 4))
        if count and index % 3 != 0:
            target = codes[rng.randrange(count) * 4:][:4]
            if index % 5 == 0:
                codes += target # 중복 코드에서 첫 일치를 선택하는지 확인한다.
            side, variant, number, flag = target
        else:
            side, variant, number, flag = [rng.randrange(256) for _ in range(4)]
        flags = flag if flag < 128 else flag - 256
        mask = [0, 1, 0x7f, 0x80, 0x100, 0x80000000, 0xffffffff][index % 7]
        yield 'FrameFindNumber', codes, [side, variant, number]
        yield 'FrameFindMasked', codes, [side, variant, number, mask]
        yield 'FrameFindFlags', codes, [side, variant, flags]


def config_inputs():
    """원본 값 파서의 LF/CR·주석·백틱·공백·중복·접두어 경계 입력을 만든다."""
    cases = [
        (b'A="first"\r\nA="last"', b'a'), (b'\tQuick Help Spec = "help" // comment', b'quick help spec'),
        (b'A="//inside" // outside', b'A'), (b'A="left`"right"', b'A'),
        (b'A==== value  \n', b'A'), (b'AB=x\nA=y', b'A'), (b'A=\n', b'A'),
        (b'//A=x\n[section]\nA="ok"', b'A'), (b'A=x\rB=y', b'B'),
        (b'\rA=x', b'A'), (b'A="unterminated // quoted', b'A'), (b'A=x\0\nB=y', b'B'),
        (b'A="x"\n\r \tB="y"', b'b'), (b'', b'A')]
    # 재현 가능한 ASCII 버퍼를 생성한다. CRT 스텁이 로케일 규칙을 다루지 않는 범위를 유지한다.
    rng = random.Random(INPUT_SEED)
    alphabet = 'abAB= /\t`"\r\n012'
    for _ in range(256):
        text = ''.join(rng.choice(alphabet) for _ in range(rng.randrange(1, 100)))
        cases.append((text.encode('ascii'), rng.choice([b'A', b'B', b'AB', b'A B'])))
    return cases


def main():
    """두 판본이 같은 반환값을 낼 때만 C++용 기대값과 실행 근거를 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'cpppj/tests/fixtures/original-x86.tsv')
    parser.add_argument('--report', type=Path, default=ROOT / 'cpppj/recovery-evidence.json')
    args = parser.parse_args()
    manifest = json.loads((ROOT / 'cpppj/recovery-manifest.json').read_text(encoding='utf-8'))
    pairs = manifest['reviewed_pairs']
    patch, cd = [Oracle(key, pairs) for key in ('originals', 'originalCD')]
    purge = {p['name']: p['purge'] for p in pairs}
    rows = ['# 원본 x86 기계어에서 생성. 두 판본 동일 결과·스택 복구 확인. CRT ASCII 비교·assert 보고만 대체.']
    counts = {}
    # Python 계산 모델이 아니라 실제 원본 기계어의 반환값을 기대값으로 쓴다.
    for name, codes, params in inputs():
        a = patch.frame(name, codes, params, purge[name])
        b = cd.frame(name, codes, params, purge[name])
        if a != b:
            raise RuntimeError(f'두 판본 불일치: {name}, {codes.hex()}, {params}, {a}/{b}')
        rows.append('\t'.join([name, codes.hex() or '-', *map(str, params), str(a)]))
        counts[name] = counts.get(name, 0) + 1
    # 키 위치와 원시 값은 별도 열로 저장해 C++ 양쪽 API를 검증한다.
    for text, key in config_inputs():
        a, b = patch.config(text, key), cd.config(text, key)
        if a != b:
            raise RuntimeError(f'설정 두 판본 불일치: {text!r}/{key!r}: {a!r}/{b!r}')
        offset, value = (-1, 'missing') if a is None else (a[0], 'hex:' + a[1].hex())
        rows.append('\t'.join(['Config', text.hex() or '-', key.hex(), str(offset), value]))
        counts['Config'] = counts.get('Config', 0) + 1
    payload = '\n'.join(rows) + '\n'
    report = {'schema': 1, 'method': 'Unicorn x86 32-bit; OS/API execution forbidden',
              'manifest_sha256': hashlib.sha256((ROOT / 'cpppj/recovery-manifest.json').read_bytes()).hexdigest(),
              'unicorn_version': importlib.metadata.version('unicorn'),
              'binary_sha256': {'originals': patch.sha256, 'originalCD': cd.sha256},
              'seed': INPUT_SEED, 'cases': counts, 'total_cases': sum(counts.values()),
              'stubbed': ['ASCII toupper', 'ASCII strnicmp', 'assert reporting'],
              'assert_report_calls': {'originals': patch.assertions, 'originalCD': cd.assertions},
              'fixture_sha256': hashlib.sha256(payload.encode('utf-8')).hexdigest(),
              'limitations': ['게임 전체 실행 검증 아님', 'Config CRT는 ASCII 로케일만 검증',
                             '원본 assert UI 미검증', 'C++ 검증은 ctest에서 별도로 실행']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(payload, encoding='utf-8', newline='\n')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'두 판본 x86 동일 결과 {sum(counts.values())}개 → {args.output}')


if __name__ == '__main__':
    main()
