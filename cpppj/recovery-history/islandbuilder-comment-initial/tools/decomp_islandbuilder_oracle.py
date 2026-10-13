#!/usr/bin/env python3
"""실제 IslandBuilder Place/PlaceAll·ChunkMap·Runtime.Add/Bounds·CanonDecoder를 두 PE에서 독립 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT, SPECS as PE_SPECS, TYPES, STOP, STACK_BASE, STACK, digest, pefile
from decomp_priestgeometry_oracle import GeometryOracle, GEOMETRY
from decomp_canontype_oracle import frame_codes
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 분석 입력의 별도주소이며 실제World ChunkMap/IslandList는 원본 전역의inline 저장소를 그대로 쓴다.
BUILDER, CUSTOM, RECORDS, CODES = 0x11600000, 0x11700000, 0x11800000, 0x11900000
CHUNK_BYTES, ISLAND_BYTES = 12 + 256 * 12, 4 + 100 * 32
FIXTURE = ROOT / "cpppj/tests/fixtures/islandbuilder-x86.tsv"
REPORT = ROOT / "cpppj/recovery-islandbuilder-evidence.json"
# 실제primary/helper 주소와 원본global inline 지도/목록을 판본별로 분리한다.
SPECS = {
    "originals": dict(place=0x46dba0, all=0x46dd70, constructor=0x46d9b0, attach=0x46da20,
                      world=0x54c8a0, islands=0x568b60, assert_=0x4e0620,
                      lines={99:"B", 0x96:"E", 0xb8:"R", 0xc2:"M",299:"F"}),
    "originalCD": dict(place=0x490750, all=0x490980, constructor=0x490430, attach=0x4904d0,
                       world=0x5484c0, islands=0x583188, assert_=0x490040,
                       lines={0x5f:"B", 0x92:"E", 0x2ae:"O", 0xbe:"M", 0xce:"N",0x123:"F"})}
# 고정입력열은 world여부/owner/Terr/count/bounds/위치무시/프레임profile/지도크기/범위다.
KEYS = ("world", "owner", "territory", "count", "left", "top", "right", "bottom", "ignore", "profile", "xlen", "ylen", "start", "end")


def inputs():
    """기대 출력 없이 전체6비트모양/원본high2방향/세계·임시지도와선택flag/경계를 교차한다."""
    result = []
    # 모든64저장모양은방향0..3을그대로전달하고원래owner/전달지역의좁은폭을 독립 입력한다.
    for shape, direction, world in itertools.product(range(64), range(4), (0, 1)):
        values = (world, (0, 1, 255, 0xffffffff)[shape%4], (0, 3, 127, 255, 0xffffffff)[shape%5],
                  2, 1, 1, 15, 15, (shape+direction)&1, shape&1, 16, 16, 0, 20)
        record = bytes((shape | direction<<6, 1, 0x32, (shape*13)&255, 0xab, 0xcd))
        result.append(("Place", values, record))
    # 빈/부분누락프레임·edgeclamp·invalidbounds진단·실제99번째Add의물리끝과원래기록/padding을 관찰한다.
    for profile, bounds, count, world in itertools.product((0, 2, 3), ((1, 1, 15, 15), (15, 15, 16, 16), (-1, -1, 15, 15)), (0, 99), (0, 1)):
        result.append(("Place", (world, 0xffffffff, 0xffffffff, count, *bounds, 0, profile, 16, 16, 0, 20), bytes((3, 1, 0, 255, 0xaa, 0xbb))))
    # 각record의flag/위치/high2는파일순서그대로이며start=end와역순범위도 정상 반환까지 실행한다.
    for world, flag_profile, interval in itertools.product((0, 1), range(3), ((0, 20), (1, 3), (4, 4), (5, 3), (0, 0))):
        records = bytearray()
        for index in range(20):
            flag = (1, 0, 2, 3, 5, 255)[(index+flag_profile)%6]
            records.extend((index | ((index%4)<<6), flag, (index%4)|((index%3)<<4), (index*17)&255, 0xaa, 0xbb))
        result.append(("All", (world, 255, 0, 2, 1, 1, 15, 15, 0, flag_profile%2, 16, 16, *interval), bytes(records)))
    return list(dict.fromkeys(result))


def block_rle(data, width):
    """기계어가쓴전체저장소를동일길이block run으로보존하며C++에서예상필드를재계산하지않는다."""
    runs = []
    # 원본memory순서의같은block만합쳐생략된padding/바뀌지않은항목도 전부 보존한다.
    for offset in range(0, len(data), width):
        block = data[offset:offset+width].hex()
        if runs and runs[-1][0] == block:
            runs[-1][1] += 1
        else:
            runs.append([block, 1])
    return ";".join(f"{block}:{count}" for block, count in runs)


class IslandBuilderOracle:
    """현재20/20좁은기계어만허용하며assert보고만명시경계로관찰하고다른body를stub하지않는다."""
    prepare_registers = GeometryOracle.prepare_registers
    check = GeometryOracle.check

    def __init__(self, edition):
        """타호스트의이전export초기화에의존하지않고현재PE와원본inline 저장소를직접준비한다."""
        self.edition, self.spec, self.s = edition, PE_SPECS[edition], SPECS[edition]
        pe = pefile.PE(str(ROOT / self.spec["binary"])); image = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(image)+4095)&~4095)
        self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase, image)
        self.type_stride = self.spec["type_stride"]
        # 분석buffer는서로다른page이며원본파일의바이트를변경하지않는다.
        for address, size in ((TYPES, 0x20000), (BUILDER, 4096), (CUSTOM, 4096), (RECORDS, 4096),
                              (CODES, 4096), (STOP, 4096), (STACK_BASE, 0x40000)):
            self.mu.mem_map(address, size)
        self.exports = [ROOT/f"extracted/islandbuilder/{edition}/{name}" for name in ("creation.c", "functions.tsv")]
        self.native = collections.Counter(); self.primary_calls = collections.Counter(); self.returns = 0
        self.allowed = bytearray(0x200000); self.entries = set()
        ranges = []
        with self.exports[1].open(encoding="utf-8") as stream:
            # Ghidra의실제불연속body만실행을허용한다.
            for row in csv.DictReader(stream, delimiter="\t"):
                ranges.append(row)
        self.reused_ranges = {}
        if edition == "originalCD":
            # CD validId가호출하는기존완료Count만이전현재호스트export에서선택한다.
            helper = ROOT/f"extracted/island-runtime/{edition}/functions.tsv"
            self.exports.extend([ROOT/f"extracted/island-runtime/{edition}/creation.c",helper])
            with helper.open(encoding="utf-8") as stream:
                selected = [row for row in csv.DictReader(stream,delimiter="\t") if int(row["entry"],16)==0x4bd4f0]
            if len(selected)!=1:
                raise RuntimeError("IslandBuilder 기존완료CD Count helper범위누락")
            ranges.extend(selected);self.reused_ranges = {row["entry"]:row["ranges"] for row in selected}
        # 새좁은범위와명시선택한기존완료helper의전체불연속범위를등록한다.
        for row in ranges:
                self.entries.add(int(row["entry"],16))
                for span in row["ranges"].split(";"):
                    low, high = (int(value,16) for value in span.split("-"))
                    self.allowed[low-0x400000:high+1-0x400000] = b"\x01"*(high+1-low)
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def put(self, address, value, width=4):
        """명시입력/경계효과의little endian 좁은필드를 쓴다."""
        self.mu.mem_write(address, (int(value)&((1<<(8*width))-1)).to_bytes(width,"little"))

    def read(self, address, width=4):
        """실제현재저장소의BYTE/WORD/DWORD를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),"little")

    def instruction(self, mu, address, size, data):
        """진단외부경계와실제body진입만계수하고세계지도와섬효과는원본명령이 실행한다."""
        if address == self.s["assert_"]:
            sp = mu.reg_read(UC_X86_REG_ESP); line = self.read(sp+12)
            token = self.s["lines"].get(line)
            if token is None:
                raise RuntimeError(f"미분류 IslandBuilder 원본assert {self.edition} line={line} kind={self.kind} input={self.values}")
            self.events.append(token); self.assertions += 1
            mu.reg_write(UC_X86_REG_ESP,sp+4); mu.reg_write(UC_X86_REG_EIP,self.read(sp)); return
        if not 0x400000 <= address < 0x600000 or not self.allowed[address-0x400000]:
            raise RuntimeError(f"IslandBuilder 허용밖native {self.edition} {address:08x}")
        if address in self.entries:
            self.native[f"{address:08x}"] += 1
        if address in (self.s["place"], self.s["all"]):
            self.primary_calls[f"{address:08x}"] += 1

    def write(self, mu, access, address, size, value, data):
        """원본inline 지도/3204BYTE 기록·builder·스택밖의실제쓰기를 거부한다."""
        ranges = ((STACK_BASE,0x40000), (BUILDER,24), (CUSTOM,CHUNK_BYTES),
                  (self.s["world"],CHUNK_BYTES), (self.s["islands"],ISLAND_BYTES))
        if not any(low <= address and address+size <= low+length for low,length in ranges):
            raise RuntimeError(f"IslandBuilder 허용밖쓰기 {self.edition} {address:08x}+{size}")

    def call(self, address, args, control):
        """실제thiscall/반환·보존레지스터·x87를입력마다확인한다."""
        self.prepare_registers(control); self.mu.reg_write(UC_X86_REG_ECX,BUILDER)
        self.mu.mem_write(STACK,struct.pack("<"+"I"*(len(args)+1),STOP,*[value&0xffffffff for value in args]))
        self.mu.emu_start(address,STOP,count=600000); self.check(control,STACK+4+4*len(args))
        return self.mu.reg_read(UC_X86_REG_EAX)

    def run(self, kind, values, records, control):
        """명시원본입력만준비하고목록/지도전체바이트와진단순서를관찰한다."""
        self.kind, self.values, self.events, self.assertions = kind, values, [], 0
        world, owner, territory, count, left, top, right, bottom, ignore, profile, xlen, ylen, start, end = values
        mu = self.mu; s = self.s
        # 첫DWORD의isValid nonzero와inline X열우선256record를그대로구성한다.
        chunk = struct.pack("<III",1,xlen,ylen) + (bytes((127,ord('.')))+b"\xa5"*6+struct.pack("<I",0x12345678))*256
        mu.mem_write(s["world"],chunk); mu.mem_write(CUSTOM,chunk)
        mu.mem_write(s["islands"],b"\xa5"*ISLAND_BYTES); self.put(s["islands"],count)
        target = s["world"] if world else CUSTOM
        self.call(s["constructor"],[],control); self.call(s["attach"],[target],control)
        mu.mem_write(BUILDER+4,struct.pack("<4I",*[value&0xffffffff for value in (left,top,right,bottom)]))
        self.put(BUILDER+20,owner); mu.mem_write(RECORDS,bytes(120)); mu.mem_write(RECORDS,records)
        self.put(self.spec["globals"]["types"],TYPES)
        # 원본현재puzzlePiece번호와다른패턴타입을별칭없이공급한다.
        for address, value in zip(GEOMETRY[self.edition]["patterns"],(107,82,94,157,131,129,140,142),strict=True):
            self.put(address,value)
        codes = frame_codes(profile)
        if profile==3:
            # 부분누락은명시입력G방향만빼며원본FindNumber가어떤칸을건너뛸지미리계산하지않는다.
            codes = b"".join(codes[offset:offset+4] for offset in range(0,len(codes),4) if codes[offset]!=ord('G'))
        typ = TYPES + 107*self.type_stride
        mu.mem_write(typ,bytes(self.type_stride)); self.put(typ+0x114,len(codes)//4); self.put(typ+0x124,CODES)
        mu.mem_write(CODES,bytes(4096)); mu.mem_write(CODES,codes)
        if self.edition == "originals":
            self.put(0x5e4794,0)
        result = self.call(s["place"] if kind=="Place" else s["all"],
                           [RECORDS,territory,ignore] if kind=="Place" else [RECORDS,start,end],control)
        self.returns += 1
        target_bytes = bytes(mu.mem_read(target+12,256*12)); world_bytes = bytes(mu.mem_read(s["world"]+12,256*12))
        islands = bytes(mu.mem_read(s["islands"],ISLAND_BYTES))
        return [result if kind=="Place" else 0,block_rle(target_bytes,12),"-" if world else block_rle(world_bytes,12),
                str(int.from_bytes(islands[:4],"little"))+","+block_rle(islands[4:],32),";".join(self.events) or "-",self.assertions]


def generate(smoke=False):
    """같은고유입력의두x87정밀도반환을대조하고반복/기존helper관찰을새고유수에더하지않는다."""
    rows, editions = [], {}
    sources = {Path(__file__),ROOT/"tools/ghidra/islandbuilder-functions.json",ROOT/"tools/decomp_owner_oracle.py",
               ROOT/"tools/decomp_priestgeometry_oracle.py",ROOT/"tools/decomp_canontype_oracle.py",
               ROOT/"tools/decomp_damageablepredestroy_oracle.py",ROOT/"cpppj/src/o/RawIslandBuilder.h",ROOT/"cpppj/src/o/RawIslandBuilder.cpp",ROOT/"cpppj/tests/IslandBuilderTests.cpp",
               ROOT/"cpppj/src/o/CanonDecoder.h",ROOT/"cpppj/src/o/CanonDecoder.cpp",ROOT/"cpppj/src/o/CanonDecoderTables.inc",
               ROOT/"cpppj/src/o/RiftType.h",ROOT/"cpppj/src/o/RiftType.cpp",ROOT/"cpppj/src/o/ChunkMap.h",ROOT/"cpppj/src/o/ChunkMap.cpp",
               ROOT/"cpppj/src/o/RawIslandRuntime.h",ROOT/"cpppj/src/o/RawIslandRuntime.cpp",
               ROOT/"cpppj/recovery-islandbuilder-decoder-lineage-evidence.json",ROOT/"tools/analyze_islandbuilder_decoder_lineage.py"}
    cases = inputs()
    if smoke:
        cases = cases[:8]+cases[512:520]+cases[-8:]
    # 두판본은같은입력집합을서로다른원본body에서실행한다.
    for edition in SPECS:
        oracle = IslandBuilderOracle(edition); normal = assertions = 0
        for kind, values, record in cases:
            observations = [oracle.run(kind,values,record,control) for control in CONTROLS]
            if observations[0] != observations[1]:
                raise RuntimeError(f"IslandBuilder x87 정밀도차이 {edition} {kind} {values} {record.hex()}")
            rows.append([edition,kind,",".join(map(str,values)),record.hex(),*observations[0]])
            if observations[0][-1]:
                assertions += 1
            else:
                normal += 1
        editions[edition] = {"unique":len(cases),"normal_unique":normal,"assertion_unique":assertions,
                             "repeated_returns":oracle.returns,"native_calls":dict(oracle.native),"primary_calls":dict(oracle.primary_calls),
                             "reused_ranges":oracle.reused_ranges}
        sources.update(oracle.exports); sources.add(ROOT/oracle.spec["binary"])
        print(f"{edition}: IslandBuilder {len(cases)} 고유입력 정상{normal}/assert{assertions}, 두정밀도반환{oracle.returns}",flush=True)
    if smoke:
        return
    FIXTURE.write_bytes(("# 실제 IslandBuilder Place/PlaceAll·전체ChunkMap/3204BYTE·진단. 이전helper관찰 신규합산0.\n"+
                         "\n".join("\t".join(map(str,row)) for row in rows)+"\n").encode("utf-8"))
    sources.add(FIXTURE)
    report = {"host":"HJOW-Athlon","date":"2026-10-14","total":len(rows),"normal_total":sum(item["normal_unique"] for item in editions.values()),
              "assertion_total":sum(item["assertion_unique"] for item in editions.values()),"controls":list(CONTROLS),"editions":editions,
              "game_runs":0,"os_calls":0,"existing_helper_new_observations":0,"files":{path.relative_to(ROOT).as_posix():digest(path) for path in sorted(sources)},
              "limits":["같은actual inline ChunkMap/3204BYTE에Place/PlaceAll이정상반환/ABI/x87·전체저장소·진단을실행한다.",
                        "assert보고만명시경계로되돌려원본계속흐름을관찰한다. null지도/물리100기록초과/손상20Terr메모리는정상격자밖이다.",
                        "프레임표는합성정상/역순/빈/부분누락입력이며패턴/회전표는실제PE바이트다. 전체게임/Fort부모·지형/경제/GUI완료아님."]}
    REPORT.write_bytes((json.dumps(report,ensure_ascii=False,indent=2)+"\n").encode("utf-8"))


def verify():
    """현재기록의전체파일SHA와고유정상/assert입력·두정밀도복귀/primary진입을 감사한다."""
    report = json.loads(REPORT.read_text(encoding="utf-8"))
    rows = [line.split("\t") for line in FIXTURE.read_text(encoding="utf-8").splitlines() if line and not line.startswith("#")]
    assert len(rows)==report["total"] and report["game_runs"]==report["os_calls"]==report["existing_helper_new_observations"]==0
    assert report["controls"]==list(CONTROLS)
    for path, expected in report["files"].items():
        assert digest(ROOT/path)==expected, path
    # 원본입력은독립generator와같으며각row의마지막assert횟수로정상/진단을분류한다.
    expected = {(kind,tuple(values),record.hex()) for kind,values,record in inputs()}
    for edition, counts in report["editions"].items():
        selected = [row for row in rows if row[0]==edition]
        actual = {(row[1],tuple(int(value) for value in row[2].split(",")),row[3]) for row in selected}
        assert actual==expected and len(actual)==len(selected)==counts["unique"]
        assert sum(int(row[-1])==0 for row in selected)==counts["normal_unique"]
        assert sum(int(row[-1])!=0 for row in selected)==counts["assertion_unique"]
        assert counts["repeated_returns"]==2*len(selected)
        assert counts["primary_calls"][f"{SPECS[edition]['all']:08x}"]==2*sum(row[1]=="All" for row in selected)
    print(f"IslandBuilder 출처감사성공: {report['total']} 고유입력 정상{report['normal_total']}/assert{report['assertion_total']}")


def main():
    """--smoke는무저장표본,기본은전체기록생성,--verify는현재출처감사다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--smoke",action="store_true")
    parser.add_argument("--verify",action="store_true")
    args = parser.parse_args()
    if args.verify:
        verify()
    else:
        generate(args.smoke)


if __name__ == "__main__":
    main()
