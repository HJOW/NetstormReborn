"""점 명령 분배와 고정 Regexp를 보호 PE 메모리에서 관측한다. 게임·통신은 실행하지 않는다."""
import argparse
import csv
import json
import socket
from collections import Counter
from pathlib import Path
from decomp_chat_recipients_regex import NativeRegexOracle
from decomp_priestmenu import ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

BASE, SIZE, REGEX, DEFAULT = 0x531b30, 0xa08, 0x557df0, 0x557db4  # 원문 표·검색 객체·기본 명령 슬롯이다.
CALLBACK = STOP + 0x100  # 실제 원문 함수 대신 호출/변경을 명시하는 격리 장치 주소다.
PATTERN = b'^[\t\n\r ]*\\.([^\t\n\r ;$]+)([^;$]*)(;|$)'  # 00506a30의 실제 제어 문자와 역슬래시를 보존한다.
FIXTURE = ROOT/'cpppj/tests/fixtures/dot-command-dispatch-x86.tsv'  # 제품 구현 전에 동결하는 독립 관측이다.
REPORT = ROOT/'cpppj/dot-command-dispatch-native-20261030.json'  # 명령 범위와 명시 하위 장치를 기록한다.
SCHEMA = ('text', 'scene', 'mode', 'result', 'state', 'default', 'separator', 'input', 'trace')  # 최초 입력과 CPU 출력을 분리한다.


def cases():
    """명령/구분자/127BYTE 경계와 콜백 뒤 현재 전역 변경을 입력으로만 구성한다."""
    texts = [b'', b'hello', b' hello', b'.clear', b'.CLEAR', b'\t\r\n .echo abc', b'.echo', b'.echo ', b'.echo a b', b'.echo a\n\tb', b'.clear;.echo two', b'.clear;hello', b'.unknown;.clear', b'.clear;;.echo', b'.clear; .echo b; ', b'.clear;', b'.clear;\0ignored', b'.say x', b'.front 12', b'..clear', b'. clear', b'.clear$tail', b'.echo a$b', b'.echo a;b', b'.echo\vabc', b'.echo\fabc', b'.echo\xa0abc', b'.echo a\x80b', b'.echo '+b'a'*126, b'.echo '+b'a'*127, b'.echo '+b'a'*128, b'.echo '+b'a'*2048, b'.'+b'x'*31, b'.'+b'x'*32, b'.'+b'x'*80, b'.'+b'x'*81, b'\v.clear', b'\f.clear', b'.clear;\n.echo z', b'.clear;\n.echo z\n', b'.clear; .unknown z']
    for text in texts:  # 빈/한 개/전체 등록 및 양·음수 반환을 독립 교차한다.
        for scene in range(4):
            for mode in (0, 1, 2):
                yield text.hex() or '-', scene, mode
    for text in (b'.echo abc;.clear', b'.clear;.echo next', b'plain message', b'.echo abc;tail'):
        for mode in (3, 4, 5, 6, 7):  # 이름/함수표·기본 슬롯·입력·공유 캡처와 오류 장치 변경이다.
            yield text.hex(), 0, mode


def initial(scene):
    """부모 결과와 무관한 네 등록 표 입력을 전체 실제 폭으로 만든다."""
    state = bytearray(SIZE)
    names = [b'say', b'clear', b'echo', b'front', b'x'*31]
    if scene == 3:
        names[1] = b'echo'
    for index, name in enumerate(names):  # 최초 등록 이름/함수 주소와 표시 표는 독립 입력이다.
        state[index*32:index*32+len(name)] = name
        state[0x800+index*4:0x804+index*4] = (CALLBACK+index*16).to_bytes(4, 'little')
        state[0x900+index*4:0x904+index*4] = (BASE+index*32).to_bytes(4, 'little')
    count = (len(names), 0, 0xffffffff, len(names))[scene]
    state[0xa04:0xa08] = count.to_bytes(4, 'little')
    return state


class Oracle(NativeRegexOracle):
    """실제 분배·Regexp 컴파일/검색/캡처를 실행하고 명령·오류·CRT만 장치로 둔다."""
    def __init__(self):
        """기존 토큰 Regexp와 별개인 실제 점 명령 패턴을 원문 생성자로 컴파일한다."""
        super().__init__()
        self.allowed.append((0x43f6f0, 0x43f817))
        assert self.string(0x506a30) == PATTERN
        self.put(STACK, STOP); self.put(STACK+4, 0x506a30)
        self.mu.reg_write(UC_X86_REG_ECX, REGEX); self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.emu_start(0x496c90, STOP, count=1000000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP

    def mutate(self, argument):
        """첫 명령 장치에서만 같은 표/기본 슬롯/입력/공유 캡처를 명시 변경한다."""
        if self.calls != 1:
            return
        if self.mode == 3:
            self.mu.mem_write(BASE+32, b'changed\0'); self.put(BASE+0x808, CALLBACK+48)
        elif self.mode == 4:
            self.put(DEFAULT, 1); self.mu.mem_write(BASE+32, b'new-default\0')
        elif self.mode == 5 and self.string(argument):
            self.mu.mem_write(argument, b'Z')
        elif self.mode == 6:
            compiled = self.words(REGEX, 1)[0]
            self.put(compiled+12, self.words(compiled, 1)[0]+2)

    def instruction(self, mu, address, size, data):
        """선언한 하위 외 실행을 거부하며 분배 순서와 실제 현재 이름은 PE에서 얻는다."""
        if CALLBACK <= address < CALLBACK+80 and (address-CALLBACK) % 16 == 0:
            argument = self.words(mu.reg_read(UC_X86_REG_ESP)+4, 1)[0]
            self.calls += 1; self.trace.append(f'C:{address-CALLBACK}:{self.string(argument).hex()}')
            self.mutate(argument)
            self.ret(0x80000001 if self.mode == 2 else 0x1234 if self.mode in (1, 3, 4, 7) else 0)
            return
        if address == 0x42f100:
            fmt, name, code = self.words(mu.reg_read(UC_X86_REG_ESP)+4, 3)
            assert self.string(fmt) == b'Error in command ".%s": code == %x\n'
            self.trace.append(f'E:{self.string(name).hex()}:{code:x}')
            if self.mode == 7:
                compiled = self.words(REGEX, 1)[0]
                self.put(compiled+12, self.words(compiled, 1)[0]+2)
            self.ret(0); return
        super().instruction(mu, address, size, data)

    def sample(self, case, control):
        """매번 표·입력·스택을 독립 초기화하고 콜백 순서/전체 물리를 보존한다."""
        text, scene, self.mode = case
        source = bytes.fromhex('' if text == '-' else text)+b'\0'+bytes([0xd7])*32
        self.mu.mem_write(BASE, bytes(initial(scene))); self.put(DEFAULT, 0)
        self.mu.mem_write(TEXT, source); self.mu.mem_write(STACK-0x1000, bytes([0xa5])*0x1100)
        self.put(STACK, STOP); self.put(STACK+4, TEXT)
        self.mu.reg_write(UC_X86_REG_ESP, STACK); self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.calls = 0; self.trace = []; self.instructions = 0
        self.mu.emu_start(0x43f6f0, STOP, count=1000000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK+4
        compiled = self.words(REGEX, 1)[0]
        if not self.words(REGEX+4, 1)[0]:
            separator = -1
        else:
            first, group = self.words(compiled, 1)[0], self.words(compiled+12, 1)[0]
            separator = group-first if group else -1
        return dict(zip(SCHEMA, (*case, self.mu.reg_read(UC_X86_REG_EAX), bytes(self.mu.mem_read(BASE, SIZE)).hex(), self.words(DEFAULT, 1)[0], separator, bytes(self.mu.mem_read(TEXT, len(source))).hex(), ';'.join(self.trace) or '-')))


def main():
    """두 정밀도 관측을 먼저 일치시키고 최초 자료는 새 파일에만 저장한다."""
    parser = argparse.ArgumentParser(); parser.add_argument('--verify', action='store_true'); args = parser.parse_args()
    oracle = Oracle(); rows = []
    for case in cases():  # 정수 부모와 실제 엔진의 두 독립 실행을 비교한다.
        pair = [oracle.sample(case, control) for control in (0x27f, 0x37f)]
        assert pair[0] == pair[1], case
        rows.append(pair[0])
    sources = {path.relative_to(ROOT).as_posix(): digest(path) for path in (ROOT/'originals/Netstorm.exe', ROOT/'extracted/refined/originals/NETSTORM.c', ROOT/'extracted/refined/originals/functions.tsv', Path(__file__), ROOT/'tools/decomp_chat_recipients_regex.py', ROOT/'tools/decomp_chat_recipients.py', ROOT/'tools/decomp_priestmenu.py')}
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        assert report['sources'] == sources and report['fixtureSha256'] == digest(FIXTURE)
        with FIXTURE.open(encoding='utf-8', newline='') as stream:
            assert list(csv.DictReader(stream, delimiter='\t')) == [{key: str(value) for key, value in row.items()} for row in rows]
        print(json.dumps({'verified': len(rows), 'differences': 0})); return
    if FIXTURE.exists() or REPORT.exists():
        raise FileExistsError('최초 원문 관측 덮어쓰기 금지')
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.DictWriter(stream, SCHEMA, delimiter='\t', lineterminator='\n'); writer.writeheader(); writer.writerows(rows)
    report = {'host': socket.gethostname(), 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'rows': len(rows), 'observations': len(rows)*2, 'pattern': PATTERN.hex(), 'native': ['0043f6f0', '00496c90', '004964e0 및 엔진', '00495a10', '00495a80'], 'explicitDevices': ['CRT malloc/strchr/isalnum/strncmp/ASCII stricmp', '명령 callback 반환·물리 변경', '오류 출력 callback'], 'sources': sources, 'fixtureSha256': digest(FIXTURE), 'gameOrNetworkExecuted': False}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'rows': len(rows), 'observations': len(rows)*2}))


if __name__ == '__main__':
    main()
