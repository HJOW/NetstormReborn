#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문4a00c0/4a0030과 실제 토큰 helper의 링크·프로토콜 명령 순서를 동결한다."""
import argparse
import csv
import hashlib
import json
import socket
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EFLAGS


class ObservationStop(Exception):
    """선택한 외부 효과 진입에서 원래 부분순서를 보존한다."""


class Oracle(MemoryOracle):
    """부모와 토큰/프로토콜 helper는 CPU, Config·CRT/OS·UI 응답은 명시 경계다."""
    def __init__(self):
        """현재 같은 PC의 전체 정밀 범위를 사용한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        for entry in (0x4a00c0, 0x4a0030, 0x487710, 0x487620):  # 기존 문자열 helper를 실제 원문으로 실행한다.
            for part in facts[entry]['ranges'].split(';'):  # 떨어진 범위도 합치지 않는다.
                low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high + 1))

    def event(self, text):
        """효과 적용 전에 원래 진입을 관측한다."""
        self.trace.append(text)
        if self.stop and len(self.trace) == self.stop: raise ObservationStop()

    def instruction(self, mu, address, size, data):
        """실제 원문 바이트 복사/분기/포인터 조립을 유지하고 외부 장치만 반환한다."""
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 4)
        if address == 0x4e63e0:
            source = self.string(args[0]); byte = args[1] & 255; index = len(source) if not byte else source.find(bytes([byte])); self.ret(args[0] + index if index >= 0 else 0)
        elif address == 0x4e6770:
            index = self.string(args[0]).find(self.string(args[1])); self.ret(args[0] + index if index >= 0 else 0)
        elif address == 0x4e66ed:
            left, right = self.string(args[0])[:args[2]].lower(), self.string(args[1])[:args[2]].lower(); self.ret(((left > right) - (left < right)) & 0xffffffff)
        elif address == 0x4410f0:
            assert args[1:] == (0, 0, 0)
            self.event('P,' + self.string(args[0]).hex()); self.ret(TEXT + 0x3000)
        elif address == 0x441300:
            self.event('R,' + self.string(args[0]).hex() + ',' + self.string(args[1]).hex())
            returned, value = self.reads[min(self.read_index, len(self.reads) - 1)]; self.read_index += 1
            if value is not None: mu.mem_write(args[1], value + b'\0')
            self.ret(returned)
        elif address == 0x4dd640: self.event('O,' + self.string(args[0]).hex()); self.ret(self.shell)
        elif address == 0x4dd6b0:
            assert args[0] == 0 and args[2] == 0
            self.event('L,' + self.string(args[1]).hex()); self.ret()
        elif address == 0x4c9090: self.event('C,' + self.string(args[0]).hex()); self.ret()
        elif address == 0x4c63d0: self.event('H,' + self.string(args[0]).hex() + ',' + str(args[1]) + ',' + str(args[2])); self.ret()
        elif not any(low <= address < high for low, high in self.allowed): raise RuntimeError(f'검토하지 않은 Help 링크 명령 {address:08x}')

    def run(self, name, value, automatic, shell, reads, stop):
        """독립 입력/외부 응답만 설치하고 원문의 최종 호출 순서를 읽는다."""
        self.trace = []; self.stop = stop; self.shell = shell; self.reads = reads; self.read_index = 0  # 앞 사례의 현재 값과 효과를 섞지 않는다.
        self.mu.mem_write(TEXT + 0x1000, value + b'\0'); self.mu.mem_write(TEXT + 0x3000, automatic + b'\0')
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP): self.mu.reg_write(register, 0)  # caller 일반 레지스터를 고정한다.
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2); self.mu.reg_write(UC_X86_REG_ESP, STACK); self.put(STACK, STOP); self.put(STACK + 4, TEXT + 0x1000); completed = True
        try: self.mu.emu_start(0x4a00c0, STOP, count=10000)
        except ObservationStop: completed = False
        replies = ';'.join(str(returned) + ',' + ('-' if output is None else output.hex()) for returned, output in reads)
        return [name, value.hex() or '-', automatic.hex() or '-', str(shell), replies, str(stop), str(int(completed)), ';'.join(self.trace)]


def cases():
    """유효한 원래 버퍼 입력과 독립 외부 응답을 나열한다."""
    routes = [b'Topic', b'#frag', b'', b' cmd:Play()', b'CMD::Go()', b'"cmd:odd":Go()', b'http:#frag', b'::#here', b' "http" ://game/path', b'http:://one', b'http://a\0hidden', b'\t"mailto" :user@example.invalid', b'http: quoted? %1', b':body', b'http:', b'http:' + b'a' * 255]
    for index, value in enumerate(routes):  # 원문 파서의 공백/인용/첫4비교/연속 콜론/첫NUL을 분리한다.
        for shell in (0, 1): yield f'route-{index}-{shell}', value, b'http:mailto:', shell, [(1, b'helper %1')], 0
    for automatic in (b'', b'http:', b'HTTP:', b'prefixhttp:suffix'):  # autoOpen은 대소문자 구별 substring이다.
        for shell in (0, 1):
            for index, replies in enumerate([[(1, b'app %1')], [(0, None), (0, None)], [(1, b'{missing}'), (1, b'backup %1')], [(1, b'{missing}'), (1, b'')], [(1, b'{missing}'), (0, b'changed')]]):  # 반환과 출력 변경을 독립 입력으로 둔다.
                yield 'external-' + automatic.hex() + f'-{shell}-{index}', b'http://example.invalid/a?x=1', automatic, shell, replies, 0
    for index, command in enumerate([b'app %1', b'app %z %1', b'app %', b'%1', b'', b'"quoted helper" "%1"', b'{bad}', b'xx\0ignored']):  # 첫% 두바이트와 빈/첫NUL 명령을 분리한다.
        yield f'command-{index}', b'proto:payload', b'', 0, [(1, command), (1, b'fallback')], 0
    for automatic in (b'', b'http:'):  # 각 외부 효과 진입의 중단 prefix다.
        for stop in range(1, 6): yield f'stop-{automatic.hex()}-{stop}', b'http://target', automatic, 0, [(1, b'{bad}'), (1, b'fallback %1')], stop
    yield 'long-token-shell', b'a' * 90 + b':tail', b'a' * 78 + b':', 1, [(0, None)], 0


def sha(path):
    """보호 소스/최초 관측의 원시 바이트를 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """독립 두 emulation을 비교하고 최초 fixture를 한 번만 만든다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--output', type=Path, required=True); args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False); inputs = list(cases())
    first = Oracle(); rows = [first.run(*values) for values in inputs]; replay = Oracle(); repeated = [replay.run(*values) for values in inputs]
    for name, values in (('first.json', rows), ('replay.json', repeated)): (output / name).write_text(json.dumps(values, indent=2) + '\n', encoding='utf-8', newline='\n')  # 독립 두 관측을 보존한다.
    assert rows == repeated
    fixture = ROOT / 'cpppj/tests/fixtures/help-links-x86.tsv'
    with fixture.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# 이름/원문/autoOpen/Shell응답/설정응답/중단/완료/호출순서\n'); stream.writelines('\t'.join(row) + '\n' for row in rows)
    sources = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/Netstorm.c', 'tools/decomp_priestmenu.py', 'tools/decomp_help_links.py', 'cpppj/tests/fixtures/help-links-x86.tsv']
    report = {'host': socket.gethostname(), 'date': '2026-10-30', 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'cases': len(rows), 'observations': len(rows) * 2, 'differences': [], 'entries': ['004a00c0', '004a0030', '00487710', '00487620'], 'gameStarted': False,
              'scope': '부모/토큰·프로토콜 helper 실제 CPU. CRT 문자열/Config/Script·Help/Shell·실행은 명시 외부 경계. 실제 OS/외부 사이트/게임/MP 실행 없음.', 'sourceSha256': {name: sha(ROOT / name) for name in sources}, 'evidenceSha256': {path.name: sha(path) for path in output.iterdir() if path.is_file()}}
    (ROOT / 'cpppj/help-links-native-20261030.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n'); print(json.dumps({'cases': len(rows), 'observations': len(rows) * 2, 'differences': 0}))


if __name__ == '__main__':
    main()
