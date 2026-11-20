"""PATCH4dece0의 실제 기계어를 격리 실행해 관찰1·현재길이·중첩 복제를 기록한다."""
from pathlib import Path
import hashlib
import argparse
import itertools
import json
import socket
import sys

# 기존 격리 오라클 의존성만 빌리며 설치나 게임/OS 실행은 하지 않는다.
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "extracted/oracle-python"))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBX, UC_X86_REG_EBP, UC_X86_REG_EFLAGS

# 원본 이미지와 겹치지 않는 실제 입력/관찰표/스택의 가상 주소다.
BUFFER, OBSERVERS, CALLBACK, STACK, STOP = 0x12000000, 0x12100000, 0x12200000, 0x30008000, 0x20000000
ENTRY, END, ALLOCATE, SOURCE = 0x4dece0, 0x4ded45, 0x4e4391, 64
# 새 관측은 제품 출력으로 덮지 않고 별도 준비 디렉터리에 쓴다.
OUTPUT = ROOT / "extracted/dotnet-0689"


def digest(path):
    """읽기 전용 근거의 전체SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cases():
    """길이 경계·signed 용량·양방향 중첩·관찰 뒤 변경의 명시 입력을 열거한다."""
    result = set()
    # DWORD 나머지와 헤더보다 짧은 선언도 실제 원본에 공급한다.
    for length, destination, mode in itertools.product((0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32), (-1, 60, 61, 62, 63, 64, 65, 66, 67, 68, 72, 128), range(5)):
        # 자동할당·정확한 용량·한 바이트 부족·음수/큰 signed 용량을 구별한다.
        for capacity in (0, length - 1, length, length + 1, -2147483648, 2147483647):
            actual_length = 9 if mode == 2 else 0 if mode == 3 else 31 if mode == 4 else length
            if destination == -1 and capacity != 0 and capacity >= actual_length and actual_length:
                continue  #NULL쓰기의 물리 오류는 별도 관리 경계 검사로 둔다.
            result.add((SOURCE, capacity, destination, length, mode))
    #NULL source는 관찰과 할당을 모두 생략해야 한다.
    for capacity, destination, mode in itertools.product((0, -1, 32), (-1, 64, 128), range(5)):
        result.add((-1, capacity, destination, 17, mode))
    return sorted(result)


class Oracle:
    """102바이트의 실제 원문만 실행하고 observer/할당 경계만 명시 장치로 둔다."""
    def __init__(self):
        """불변PE의 메모리 사본과 서로 분리된 입력 페이지를 만든다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        data = pefile.PE(str(ROOT / "originals/Netstorm.exe")).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(data) + 4095) & ~4095)
        self.mu.mem_write(0x400000, data)
        # 스택과 입력/관찰표는 원본PE와 별도로 매핑한다.
        for address, size in ((BUFFER, 4096), (OBSERVERS, 4096), (CALLBACK, 4096), (STACK - 0x8000, 0x10000), (STOP, 4096)):
            self.mu.mem_map(address, size)
        self.instructions = 0  #전체 실제 명령/명시 경계의 누적 관측이다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def put(self, address, value, width=4):
        """외부 입력 변경만 지정 폭의little endian으로 적용한다."""
        self.mu.mem_write(address, (value & ((1 << (width * 8)) - 1)).to_bytes(width, "little"))

    def word(self, address):
        """실제 현재DWORD를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address, 4), "little")

    def ret(self, value=0, purge=0):
        """실제cdecl/thiscall 복귀 폭을 적용한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_EIP, self.word(esp))
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)

    def write(self, mu, access, address, size, value, data):
        """원문 명령이 입력 창/스택 이외의 상태를 쓰면 즉시 실패한다."""
        assert BUFFER <= address and address + size <= BUFFER + 256 or STACK - 0x8000 <= address and address + size <= STACK + 16, (hex(address), size)

    def instruction(self, mu, address, size, data):
        """실제 계산은 실행하고 observer와 할당의 인자/입력 변경만 기록한다."""
        self.instructions += 1
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == CALLBACK:
            assert mu.reg_read(UC_X86_REG_ECX) == BUFFER + SOURCE and self.word(esp + 4) == 1
            self.effects.append("O:1")
            if self.case[4] == 2:
                self.put(BUFFER + SOURCE + 2, 9, 2)
                self.put(BUFFER + SOURCE + 1, 0, 1)
            elif self.case[4] == 3:
                self.put(BUFFER + SOURCE + 2, 0, 2)
            elif self.case[4] == 4:
                self.put(BUFFER + SOURCE + 2, 31, 2)
                self.put(BUFFER + SOURCE + 8, 0xee, 1)
            self.ret(purge=4)
            return
        if address == ALLOCATE:
            self.allocated = self.word(esp + 4)
            self.effects.append("A:" + str(self.allocated))
            self.ret(BUFFER + 192)
            return
        assert ENTRY <= address and address + size <= END + 1, hex(address)

    def sample(self, case):
        """명시 입력에서 원본 반환·전체 입력 창·효과 순서를 관측한다."""
        self.case = case  #source/capacity/destination/초기length/observer의 독립 입력이다.
        self.effects, self.allocated = [], -1
        self.mu.mem_write(BUFFER, bytes((index * 17 + 3) & 255 for index in range(256)))
        self.put(BUFFER + SOURCE + 1, 0x58, 1)
        self.put(BUFFER + SOURCE + 2, case[3], 2)
        self.mu.mem_write(OBSERVERS, b"\0" * 1024)
        self.put(0x5e477c, OBSERVERS)
        self.put(OBSERVERS + 0x58 * 4, CALLBACK if case[4] else 0)
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        # 독립 입력마다callee saved와 방향 플래그를 초기화한다.
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ECX, 0)
        self.mu.reg_write(UC_X86_REG_EDX, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.put(STACK, STOP)
        self.put(STACK + 4, BUFFER + case[0] if case[0] >= 0 else 0)
        self.put(STACK + 8, case[1])
        self.put(STACK + 12, BUFFER + case[2] if case[2] >= 0 else 0)
        self.mu.emu_start(ENTRY, STOP, count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        returned = self.mu.reg_read(UC_X86_REG_EAX)
        return (str(returned - BUFFER if returned else -1), str(self.allocated), ";".join(self.effects) or "-", bytes(self.mu.mem_read(BUFFER, 256)).hex())


def main():
    """원본 관측만 준비 디렉터리에 저장하며C# 결과/기존fixture는 읽거나 쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=OUTPUT, help="새 관측/근거의 준비 디렉터리")
    parser.add_argument("--verify", action="store_true", help="기존 근거SHA/고유 입력을 읽기 전용으로 감사")
    args = parser.parse_args()
    output = args.out.resolve()
    fixture = output / "packet-copy-x86.tsv"
    report_path = output / "packet-copy-native.json"
    if args.verify:
        facts = json.loads(report_path.read_text(encoding="utf-8"))
        assert digest(ROOT / "originals/Netstorm.exe") == facts["sourceSha256"]
        assert digest(Path(__file__).resolve()) == facts["toolSha256"]
        assert digest(fixture) == facts["fixtureSha256"]
        rows = [row.split("\t") for row in fixture.read_text(encoding="utf-8").splitlines() if row and not row.startswith("#")]
        assert len(rows) == len({row[0] for row in rows}) == facts["rows"] == facts["normalReturns"]
        assert [row[0] for row in rows] == [",".join(map(str, case)) for case in cases()]
        tracked = ROOT / "dotnetpj/tests/Netstorm.Core.Tests/Fixtures/packet-copy-x86.tsv"
        if tracked.exists():
            assert digest(tracked) == facts["fixtureSha256"]
        print("패킷 복제 원본SHA/고유 입력 감사:", len(rows))
        return
    output.mkdir(parents=True, exist_ok=True)
    oracle = Oracle()
    rows = []
    # 기대값 계산기 없이 실제 함수의 반환과 물리 메모리를 그대로 보존한다.
    for case in cases():
        rows.append((",".join(map(str, case)), *oracle.sample(case)))
    fixture.write_text("# input returnedOffset allocated effects buffer256\n" + "\n".join("\t".join(row) for row in rows) + "\n", encoding="utf-8", newline="\n")
    report = {"host": socket.gethostname(), "entry": "004dece0", "range": "004dece0-004ded45", "sourceSha256": digest(ROOT / "originals/Netstorm.exe"), "toolSha256": digest(Path(__file__).resolve()), "fixtureSha256": digest(fixture), "rows": len(rows), "normalReturns": len(rows), "instructionAndBoundaryObservations": oracle.instructions, "osCalls": 0, "limitations": ["observer와operator_new는 인자/입력변경/실제예약을 공급하는 명시 경계다.", "게임/OS 실행과Client 실제 접속 통합을 검증하지 않는다."]}
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(report, ensure_ascii=False))


if __name__ == "__main__":
    main()
