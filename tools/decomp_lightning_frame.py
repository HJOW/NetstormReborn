#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 공유 빔의 실제 시간 진행·전체 입자·선 출력·소리/소진 요청을 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_lightning_path import Oracle as PathOracle, ROOT, PATH, OWNER, PARTICLES, POOL, STACK, CAPACITY, ENTRIES as PATH_ENTRIES, bits
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

ENTRIES = PATH_ENTRIES | {0x472310, 0x4724f0, 0x41bfb0, 0x418400, 0x418470, 0x4239f0, 0x487ba0, 0x4e49c0}  # 실제로 실행할 몸체 전체다.
SOUND = OWNER + 0x1000  # 입력 파일명은 실제 포인터로 두고 출력에서만 토큰1로 바꾼다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/lightning-frame-x86.tsv'  # 입력/출력 전체 필드와 용량 전체 입자다.
REPORT = ROOT / 'cpppj/lightning-frame.json'  # 실제 함수 반환 수·외부 경계·해시와 제한이다.


def semantic(raw):
    """포인터/미사용 필드만 제외한64BYTE 의미 상태를 보존한다."""
    value = raw[0xc:0x1c] + raw[0x24:0x2c] + raw[0x30:0x4c] + raw[0x54:0x58] + raw[0x5c:0x64]
    value = bytearray(value); sound = struct.unpack_from('<I', value, 60)[0]
    assert sound in (0, SOUND); struct.pack_into('<I', value, 60, int(bool(sound)))
    return bytes(value)


class Oracle(PathOracle):
    """경로 하위는 실제 코드로 실행하고 dirty/선/파일명 음향/월드 삭제만 요청 경계로 관찰한다."""
    def __init__(self):
        """기존 실제 경로 격리/호출 균형 검사를 재사용하고 프레임 함수 범위를 추가한다."""
        super().__init__(); self.events = []; self.killed = False
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 전체 함수 범위만 허용하며 무관한 실행으로 넓히지 않는다.
        for entry in ENTRIES - PATH_ENTRIES:
            # 함수의 불연속 범위를 각각 보존한다.
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """선·dirty·소리·월드 수명 외부 요청만 기록한다. 경로/난수/시간 계산은 대체하지 않는다."""
        if address in (0x499310, 0x4dade0, 0x4a9d70, 0x41bf10):
            esp = mu.reg_read(UC_X86_REG_ESP); ret = struct.unpack('<I', mu.mem_read(esp, 4))[0]; purge = 0
            if address in (0x499310, 0x4dade0):
                arguments = bytes(mu.mem_read(esp + 4, 20))
                if address == 0x499310:
                    assert arguments[16:] == bytes(4)
                self.events.append((b'D' if address == 0x499310 else b'L') + arguments)
            elif address == 0x4a9d70:
                x, y, filename, flags = struct.unpack('<4I', mu.mem_read(esp + 4, 16))
                assert filename == SOUND and flags == 0 and bytes(mu.mem_read(filename, 13)) == b'shortZap.wav\0'
                self.events.append(b'S' + struct.pack('<4I', x, y, 1, flags))
            else:
                assert mu.reg_read(UC_X86_REG_ECX) == OWNER and bytes(mu.mem_read(esp + 4, 4)) == bytes(4)
                self.events.append(b'K' + bytes(4)); self.killed = True; purge = 4
            self.calls[f'boundary-{address:08x}'] += 1
            mu.reg_write(UC_X86_REG_EAX, 0); mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge); mu.reg_write(UC_X86_REG_EIP, ret); return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'허용하지 않은 빔 프레임 실행 {address:08x}')
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1

    def write(self, mu, access, address, size, value, data):
        """빔 객체·경로·입자·난수·스택 밖의 쓰기는 거부한다."""
        regions = ((PATH, PATH + 64), (OWNER, OWNER + 0x68), (PARTICLES, PARTICLES + CAPACITY * 24), (0x532710, 0x532714), (STACK - 0x7f000, STACK + 0x1000))
        if not any(low <= address and address + size <= high for low, high in regions):
            raise RuntimeError(f'허용하지 않은 빔 프레임 쓰기 {address:08x}+{size}')

    def particles(self):
        """용량 전체의24BYTE에서 이전 입자 포인터만 index+1로 바꾼다."""
        count = struct.unpack('<I', self.mu.mem_read(OWNER + 0x24, 4))[0]; raw = bytearray(self.mu.mem_read(PARTICLES, count * 24))
        # 죽은/미사용 입자까지 모두 비교한다.
        for index in range(count):
            offset = index * 24 + 20; struct.pack_into('<I', raw, offset, self.token(struct.unpack_from('<I', raw, offset)[0]))
        return bytes(raw)

    def observation(self):
        """실제 필드/경로/용량 전체/난수를 읽으며 기대값을 계산하지 않는다."""
        return (semantic(bytes(self.mu.mem_read(OWNER, 0x68))).hex(), self.snapshot()[:64].hex(), self.particles().hex() or '-', str(struct.unpack('<I', self.mu.mem_read(0x532710, 4))[0]))

    def setup(self, seed, sound, variant):
        """실제 경로 생성자로 경로를 준비한 뒤 독립 빔 객체 입력을 구성한다."""
        points = (20.5, 21.25, 40.125, 35.375) if not variant else (-7.75, 8.25, 12.5, -9.75)
        scale = (16, 11) if not variant else (.75, 1.25)
        super().sample((0, 148, 70, *map(bits, points), *map(bits, scale), seed, '-'), 0x027f)
        mu = self.mu; raw = bytearray(b'\xa5' * 0x68)
        struct.pack_into('<3I4f2i', raw, 0, 0x50c268, 1234, 50, 12.25, 13.75, 30.5, 35.5, 0, -25)
        struct.pack_into('<3I2d2f2I', raw, 0x24, CAPACITY, 0, PARTICLES, 1, .123456789, 123.25, -456.5, 0, 0)
        distance = struct.unpack('<f', mu.mem_read(PATH + 0x18, 4))[0]
        struct.pack_into('<2IIfI', raw, 0x50, 0x55a758, 0, PATH, distance * struct.unpack('<f', struct.pack('<I', bits(.025)))[0], SOUND if sound else 0)
        mu.mem_write(OWNER, bytes(raw)); mu.mem_write(SOUND, b'shortZap.wav\0'); mu.mem_write(POOL + 50 * 50 + 6, b'\x01\0')
        return scale

    def run(self, kind, now, scale, rate, camera, precision):
        """현재 입력을 기록한 다음 실제 Frame/Draw를 호출하여 전체 후상태와 요청 순서를 보존한다."""
        mu = self.mu; state = struct.pack('<d5f', now, *scale, rate, *camera)
        mu.mem_write(0x55b4d0, state[:8]); mu.mem_write(0x59a924, state[8:16]); mu.mem_write(0x59a934, state[16:])
        fields, path, particles, seed = self.observation(); parent = bytes(mu.mem_read(POOL + 2500, 50)).hex()
        before = bytes(mu.mem_read(OWNER, 0x68)); before_parent = bytes(mu.mem_read(POOL + 2500, 100))
        self.events = []; self.killed = False
        if kind == 0:
            self.call(0x472310, OWNER, b'', 0, precision); self.returns += 1
        if not self.killed:
            self.call(0x4724f0, OWNER, struct.pack('<2i', 123, -456), 8, precision); self.returns += 1
        after = bytes(mu.mem_read(OWNER, 0x68))
        assert before[:0xc] == after[:0xc] and before[0x1c:0x24] == after[0x1c:0x24] and before[0x2c:0x30] == after[0x2c:0x30] and before[0x4c:0x54] == after[0x4c:0x54] and before[0x58:0x60] == after[0x58:0x60] and before[0x64:] == after[0x64:]
        assert bytes(mu.mem_read(POOL + 2500, 100)) == before_parent
        return (str(kind), fields, path, particles, parent, state.hex(), seed, *self.observation(), b''.join(self.events).hex() or '-')


def generate(oracle, precision):
    """자연 수명 전체·시간 후퇴/긴 간격·죽은 꼬리·표시 임계값과 저장 폭을 별도로 방문한다."""
    rows = []
    # 서로 다른 난수/음향/배율에서 최초부터 소진까지 실제 연속 상태를 재사용한다.
    for seed, sound, variant, rate in itertools.product((0, 0x12345678), (False, True), (0, 1), (0, .3, 1)):
        scale = oracle.setup(seed, sound, variant)
        # 마지막 시간은 남은 수명이 모두 소진될 만큼 길다. 삭제 뒤에는 호출하지 않는다.
        for now in (1, 1, .99, 1.025, 1.05, 1.13, 1.2, 1.3, 1.6):
            rows.append(oracle.run(0, now, scale, rate, (.25, -.5), precision))
            if oracle.killed:
                break
    # 사용 꼬리 밖의 양수 입자도 검사하며0/음수 수명 역시 감소하는지 관찰한다.
    for capacity, now, pattern in itertools.product((0, 2, 12, 120), (1, 1.05, 1.051, 1.3), (0, 1)):
        scale = oracle.setup(0xdeadbeef, True, 0); mu = oracle.mu
        mu.mem_write(OWNER + 0x24, struct.pack('<2I', capacity, min(capacity, 2))); mu.mem_write(OWNER + 0x48, struct.pack('<I', 1)); mu.mem_write(PATH + 0x38, struct.pack('<I', 1))
        # 이전 토큰은같은 저장소만 가리킨다. 살아 있는 꼬리 밖 입자도 표시 영역에는 영향을 준다.
        for index in range(capacity):
            life = (2, .1, 0, -1)[index % 4] if pattern else 0
            mu.mem_write(PARTICLES + index * 24, struct.pack('<5fI', 25.125 + index, 30.375 - index, .5, .75, life, PARTICLES))
        rows.append(oracle.run(0, now, scale, .3, (.25, -.5), precision))
    # 순수 Draw는 세 굵기 경계·음수/큰 좌표에서 float 저장 후 절삭/offset을 분리한다.
    for strength, first, camera in itertools.product((.125, .25, .5, .5000000596046448, 1), (0, 1, 4), ((.25, -.5), (16777216, -16777216))):
        scale = oracle.setup(1, False, 0); mu = oracle.mu
        mu.mem_write(OWNER + 0x24, struct.pack('<2I', 4, 4)); mu.mem_write(OWNER + 0x54, struct.pack('<i', first))
        # 이전 점과 현재 점의 순서, 죽은 점 건너뛰기와 이전 점 참조 보존을 함께 방문한다.
        for index, (x, y, life) in enumerate(((2.75, -3.25, 1), (-2.75, 3.25, 1), (16777218, -16777218, 0), (16777220, -16777220, 1))):
            mu.mem_write(PARTICLES + index * 24, struct.pack('<5fI', x, y, strength, 1, life, PARTICLES + max(0, index - 1) * 24))
        rows.append(oracle.run(1, 1, scale, 1, camera, precision))
    return rows


def main():
    """독립 원문 기대값을 저장하거나 재계산 없이 SHA·입력 고유성을 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); parser.add_argument('--smoke', action='store_true'); args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 원본/도구/기대값의 전체 바이트를 감사한다.
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        rows = [row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
        assert all(len(row) == 12 for row in rows) and len(rows) == report['uniqueInputs'] and len({tuple(row[:7]) for row in rows}) == len(rows)
        print('빔 프레임 SHA/고유 입력 감사:', len(rows)); return
    oracles = [Oracle(), Oracle()]
    if args.smoke:
        scale = oracles[0].setup(1, True, 0)
        # 최초·진행·소진까지 원문 정상 반환/경계를 확인한다.
        for now in (1, 1.025, 1.6):
            row = oracles[0].run(0, now, scale, 1, (.25, -.5), 0x027f); print(now, row[-1][:100], '정상 반환')
        return
    observations = [generate(oracle, precision) for oracle, precision in zip(oracles, (0x027f, 0x037f))]
    assert len(observations[0]) == len(observations[1])
    different = [index for index, pair in enumerate(zip(*observations)) if pair[0] != pair[1]]
    rows = list(dict.fromkeys(observations[0])); assert len({row[:7] for row in rows}) == len(rows)
    FIXTURE.write_text('# kind fields64 path64 particles parent state seed fieldsAfter pathAfter particlesAfter seedAfter events; 전후 포인터만 토큰으로 정규화\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_lightning_path.py', 'tools/decomp_lightning_frame.py', FIXTURE.relative_to(ROOT).as_posix())
    report = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', uniqueInputs=len(rows), observationsPerPrecision=len(observations[0]), normalReturns=sum(oracle.returns for oracle in oracles), precisionDifferentInputs=different, nativeCalls=dict(oracles[0].calls), sha256={path:digest(ROOT / path) for path in paths}, limitations=['실제 Frame/Draw·경로·시간·난수·영역·ftol 몸체를 실행했다.', 'dirty 큐 소비499310·선 래스터4dade0·파일명 음향4a9d70·월드 삭제41bf10은 요청 관찰 경계다. 실제 픽셀/재생/삭제 구현 증거와 구별한다.', '독립 빔 필드와 경로 생성자로 준비한 입력이다. 빔 생성/Host/등록/공유 팔레트/캠페인 발사는 다음 연결 대상이다.'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n'); print('빔 프레임:', len(rows), '고유 입력, 정밀도 차이', len(different))


if __name__ == '__main__':
    main()
