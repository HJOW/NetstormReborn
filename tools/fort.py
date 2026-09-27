# -*- coding: utf-8 -*-
"""
.fort (요새 / 미션 맵) 파일 해석 도구

포맷 명세: docs/formats/fort.md

사용법:
    python tools/fort.py dump   originals/d/b0.fort            # 섹션 요약 + 오브젝트 목록
    python tools/fort.py verify [--orig originals]              # 전체 .fort 파싱 검증 (섹션 소비 길이 확인)
"""
import argparse
import collections
import glob
import os
import struct
import sys
from dataclasses import dataclass, field

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from shp import TYPE_LOAD_ORDER, load_type_texts  # noqa: E402
from taff import TaffArchive  # noqa: E402
from typefile import parse_type  # noqa: E402

# 파일 첫 바이트 (Template.cpp: 첫 바이트가 0x46 이어야 유효)
FORT_MAGIC = 0x46

# 둘째 바이트가 이 값이면 별도 플래그가 켜진다 (의미 미확정)
FORT_FLAG_FE = 0xFE

# 섹션 이름 (Netstorm.exe 0x5146B0 문자열 + "Terr%02d" 20개). 파일에는 이 순서로 길이 접두 섹션이 저장된다
SECTION_NAMES = (
    "Subscriber State Mission CoreData Chaff Badges Territory TypeNames CompressedData "
    "Technology Money Deck Reserved2 Reserved3 Reserved4"
).split() + [f"Terr{i:02d}" for i in range(20)]

# 청크 레코드 시작 표지 ('c')
CHUNK_MARK = 99

# 타입 바이트가 이 값 이상이면 "다리 조각" 약식 표기 (소유자 = 0xFF - 값)
BRIDGE_SHORTHAND_MIN = 0xF6

# 현재 실행 파일 기준 .type 타입 번호 = 이 값 + 로딩 순서 (타입 번호 전역 변수 0x541178~ 의 초기값에서 확인)
TYPE_INDEX_BASE = 70

# 특수 처리되는 타입 (로딩 순서 이름)
TYPE_BRIDGE = "bridge"      # 약식 표기 대상 (DAT_005411a0)
TYPE_ISLAND = "island"      # 뒤따르는 1바이트만 건너뜀 (DAT_005411d0)

# typeflags → 플래그 비트 (Rifttype.cpp 해석 코드, 타입 구조체 +0xE8 = 플래그1, +0xEC = 플래그2)
FLAG1_BITS = {
    "default_hotspot": 0x1, "defaulthotspot": 0x1, "mayDropOnIsle": 0x2, "mayDropOnRim": 0x4,
    "saveFrame": 0x20, "createsisland": 0x400, "surface": 0x800, "saveQA": 0x1000, "saveQB": 0x2000,
    "carribleInVehicle": 0x10000, "container": 0x20000, "shadow": 0x40000, "randframe": 0x100000,
    "matchframe": 0x200000, "flyershadow": 0x400000, "opaqueCollide": 0x800000,
    "not_selectable": 0x8000000, "notselectable": 0x8000000, "dontSave": 0x20000000,
    "not_real": 0x28000000, "notreal": 0x28000000,
}
FLAG2_BITS = {
    "fencePost": 0x1, "island": 0x2, "bridge": 0x4, "dropblocking": 0x10, "blocking": 0x18,
    "shotblocking": 0x20, "bomb": 0x100, "vortex": 0x200, "guy": 0x400, "fringe": 0x1000,
    "buried": 0x2000, "factory": 0x4000, "nugget": 0x8000, "walker": 0x10000, "balloon": 0x20000,
    "emplacement": 0x40000, "edgefarm": 0x80000, "flyer": 0x100000, "priest": 0x200000,
    "dais": 0x400000, "islandThreeByThree": 0x1000000, "not_real": 0x2000000, "notreal": 0x2000000,
    "geyser": 0x10000000, "fence": 0x20000000, "residence": 0x40000000, "altar": 0x80000000,
}

# 레코드 선택 필드를 결정하는 비트
F1_SAVE_FRAME = 0x20
F1_SAVE_QA = 0x1000
F1_SAVE_QB = 0x2000
F1_CONTAINER = 0x20000
F2_BRIDGE = 0x4
F2_FACTORY = 0x4000

# 파생 규칙: vortex·factory·walker·balloon 타입은 container 플래그가 켜진다 (내용물 목록 저장)
F2_CONTAINER_SOURCES = 0x34200

# 파생 규칙: buried 타입은 saveQA 플래그가 켜진다
F2_BURIED = 0x2000

# 소유자 바이트를 저장하는 플래그2 마스크 (Netstorm.exe 0x542644 의 값)
F2_OWNER_MASK = 0x5D77CF00


@dataclass
class FortObject:
    """청크 안의 오브젝트 하나"""
    chunk: tuple            # (청크 x, 청크 y)
    cell: tuple             # 청크 안 위치 (위치 바이트 상위 4비트, 하위 4비트)
    type_index: int         # 파일에 저장된 타입 번호 (TypeNames 기준)
    type_name: str          # 해석한 타입 이름
    fields: dict = field(default_factory=dict)  # 선택 필드 (frame, qa, qb, bridge, factory, owner)


class Reader:
    """리틀 엔디언 바이트 읽기 도우미"""

    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0

    def u8(self) -> int:
        """1바이트 읽기"""
        v = self.data[self.pos]
        self.pos += 1
        return v

    def u16(self) -> int:
        """2바이트 읽기"""
        v, = struct.unpack_from("<H", self.data, self.pos)
        self.pos += 2
        return v

    def remaining(self) -> int:
        """남은 바이트 수"""
        return len(self.data) - self.pos


def type_hash(name: str) -> int:
    """TypeNames 섹션의 이름 해시: 글자(부호 있는 8비트)를 4바이트 주기로 자리를 바꿔 더한다. 원본은 이름을 20바이트로 자른다"""
    v = 0
    # 이름 글자마다 (i % 4) * 8 비트 왼쪽으로 밀어 더한다
    for i, c in enumerate(name[:20].encode("latin-1")):
        c = c - 256 if c > 127 else c
        v += c << ((i & 3) * 8)
    return v & 0xFFFFFFFF


def split_sections(data: bytes) -> tuple:
    """머리 2바이트와 길이 접두(u16, 길이 필드 포함) 섹션 목록을 나눈다. (섹션 목록, 마지막 섹션 뒤 위치)"""
    sections = []
    pos = 2
    # 파일 끝까지 섹션을 차례로 자른다
    while pos + 2 <= len(data):
        n, = struct.unpack_from("<H", data, pos)
        if n < 2:
            break
        sections.append(data[pos + 2:pos + n])
        pos += n
    return sections, pos


def derive_flags(f1: int, f2: int) -> tuple:
    """타입 로드 후 후처리(Rifttype.cpp FUN_0049adc0 부근)가 켜는 파생 플래그 중 .fort 해석에 영향을 주는 것만 반영한다.
    내부 class 값에 따른 플래그2 비트(0x800, 0x4000000, 0x8000000)는 class 값의 출처를 아직 몰라 반영하지 못했다."""
    if f2 & F2_CONTAINER_SOURCES:
        f1 |= F1_CONTAINER
    if f2 & F2_BURIED:
        f1 |= F1_SAVE_QA
    return f1, f2


class TypeCatalog:
    """타입 번호 → 이름·플래그 해석 (파일의 TypeNames 해시와 현재 .type 정의를 대응시킨다)"""

    def __init__(self, orig: str):
        texts = load_type_texts(orig)
        self.flags = {}
        # 로딩 목록의 타입마다 typeflags 를 비트로 바꿔 둔다
        for name in TYPE_LOAD_ORDER:
            td = parse_type(texts[name.lower()])
            f1 = f2 = 0
            # 플래그 단어마다 대응 비트를 더한다 (대소문자 무시)
            for word in td.flags:
                f1 |= next((b for k, b in FLAG1_BITS.items() if k.lower() == word.lower()), 0)
                f2 |= next((b for k, b in FLAG2_BITS.items() if k.lower() == word.lower()), 0)
            self.flags[name] = derive_flags(f1, f2)
        self.by_hash = {type_hash(n): n for n in TYPE_LOAD_ORDER}

    def conversion(self, typenames: bytes) -> dict:
        """TypeNames 섹션으로 "파일 타입 번호 → 이름" 표를 만든다. 섹션이 비어 있으면 현재 번호 체계를 그대로 쓴다"""
        if len(typenames) == 0:
            return {TYPE_INDEX_BASE + i: n for i, n in enumerate(TYPE_LOAD_ORDER)}
        count = typenames[0]
        hashes = struct.unpack_from(f"<{count}I", typenames, 1)
        return {i: self.by_hash[h] for i, h in enumerate(hashes) if h in self.by_hash}


def read_object(r: Reader, chunk: tuple, version: int, conv: dict, catalog: TypeCatalog):
    """오브젝트 레코드 하나를 읽는다 (Template.cpp FUN_004bdc60). 끝 표지이면 None"""
    pos_byte = r.u8()
    t = r.u8()
    fields = {}
    if t >= BRIDGE_SHORTHAND_MIN:
        fields["owner"] = 0xFF - t
        name = TYPE_BRIDGE
    else:
        name = conv.get(t)
    if t == 0:
        r.u8()  # 원본은 이 값이 0 인지 확인만 한다
        return None
    if name is None:
        raise ValueError(f"알 수 없는 타입 번호 {t} (위치 {r.pos - 1})")
    if name == TYPE_ISLAND:
        fields["skip"] = r.u8()
        return FortObject(chunk, (pos_byte >> 4, pos_byte & 0xF), t, name, fields)
    f1, f2 = catalog.flags[name]
    if f1 & F1_SAVE_FRAME:
        fields["frame"] = r.u8()
    if f1 & F1_SAVE_QA:
        fields["qa"] = r.u8()
    if f1 & F1_SAVE_QB:
        fields["qb"] = r.u16()
    if f2 & F2_BRIDGE:
        fields["bridge"] = r.u8()
    if version > 1 and f2 & F2_FACTORY:
        fields["factory"] = r.u8()
    if version > 0 and f2 & F2_OWNER_MASK:
        fields["owner"] = r.u8()
    if f1 & F1_CONTAINER:
        fields["contents"] = read_contents(r, conv, catalog)
    return FortObject(chunk, (pos_byte >> 4, pos_byte & 0xF), t, name, fields)


def read_contents(r: Reader, conv: dict, catalog: TypeCatalog) -> list:
    """container 의 내용물 목록 (Template.cpp FUN_004bd130): [개수 u8] + 항목([타입 u8] [saveQA u8] [saveQB u16] [중첩 내용물])"""
    items = []
    count = r.u8()
    # 개수만큼 내용물 항목을 읽는다
    for _ in range(count):
        t = r.u8()
        name = conv.get(t)
        if t == 0:
            break
        if name is None:
            raise ValueError(f"알 수 없는 내용물 타입 번호 {t} (위치 {r.pos - 1})")
        f1, _f2 = catalog.flags[name]
        item = {"type": name}
        if f1 & F1_SAVE_QA:
            item["qa"] = r.u8()
        if f1 & F1_SAVE_QB:
            item["qb"] = r.u16()
        if f1 & F1_CONTAINER:
            item["contents"] = read_contents(r, conv, catalog)
        items.append(item)
    return items


def read_chunks(section: bytes, conv: dict, catalog: TypeCatalog) -> tuple:
    """Chaff / TerrNN 섹션: [버전 u8] + 청크 레코드('c' + u16 개수 + 오브젝트) 반복. (버전, 청크 목록)"""
    r = Reader(section)
    version = r.u8()
    chunks = []
    # 섹션이 끝날 때까지 청크 레코드를 읽는다
    while r.remaining() > 0:
        mark = r.u8()
        if mark != CHUNK_MARK:
            raise ValueError(f"청크 표지 오류 0x{mark:02X} (위치 {r.pos - 1})")
        count = r.u16()
        objects = []
        # 청크 안의 오브젝트를 개수만큼 읽는다
        for _ in range(count):
            obj = read_object(r, (len(chunks), 0), version, conv, catalog)
            if obj:
                objects.append(obj)
        chunks.append(objects)
    return version, chunks


def parse_fort(data: bytes, catalog: TypeCatalog) -> dict:
    """.fort 전체를 해석해 요약 사전으로 돌려준다"""
    if data[0] != FORT_MAGIC:
        raise ValueError(f"첫 바이트가 0x46 이 아님: 0x{data[0]:02X}")
    sections, end = split_sections(data)
    named = {SECTION_NAMES[i]: s for i, s in enumerate(sections[:len(SECTION_NAMES)])}
    result = {"flagFE": data[1] == FORT_FLAG_FE, "sections": len(sections), "extraBytes": len(data) - end,
              "sizes": {k: len(v) for k, v in named.items()}}
    sub = Reader(named.get("Subscriber", b""))
    if sub.remaining() >= 6:
        result["subscriberId"] = struct.unpack_from("<I", sub.data, 0)[0]
        length, = struct.unpack_from("<H", sub.data, 4)
        result["name"] = sub.data[6:6 + length].decode("latin-1")
    money = named.get("Money", b"")
    if len(money) == 4:
        result["money"] = struct.unpack("<f", money)[0]
    conv = catalog.conversion(named.get("TypeNames", b""))
    objects = collections.Counter()
    # 오브젝트가 들어 있는 섹션(Chaff, TerrNN)을 해석한다
    for name, section in named.items():
        if (name == "Chaff" or (name.startswith("Terr") and name[4:].isdigit())) and len(section) > 1:
            version, chunks = read_chunks(section, conv, catalog)
            # 청크의 오브젝트 타입 개수를 센다
            for chunk in chunks:
                objects.update(o.type_name for o in chunk)
            result.setdefault("chunkCounts", {})[name] = len(chunks)
    result["objects"] = dict(objects)
    return result


def cmd_dump(args):
    """파일 하나의 해석 결과 출력"""
    catalog = TypeCatalog(args.orig)
    data = open(args.file, "rb").read()
    info = parse_fort(data, catalog)
    # 요약 항목을 한 줄씩 출력
    for k, v in info.items():
        print(f"{k}: {v}")


def iter_forts(orig: str):
    """원본의 모든 .fort (느슨한 파일 + 아카이브) 를 (이름, 바이트) 로 돌려준다"""
    # 느슨한 d/*.fort
    for path in sorted(glob.glob(os.path.join(orig, "d", "*.fort"))):
        yield os.path.basename(path), open(path, "rb").read()
    arc = TaffArchive(os.path.join(orig, "netstorm.tarc"))
    # 아카이브 안의 .fort
    for e in arc.entries:
        if e.name.lower().endswith(".fort"):
            yield "tarc:" + e.name, arc.read(e)


def cmd_verify(args):
    """전체 .fort 파싱 검증"""
    catalog = TypeCatalog(args.orig)
    ok, errors = 0, collections.Counter()
    samples = {}
    # 파일마다 해석을 시도하고 실패 원인을 분류
    for name, data in iter_forts(args.orig):
        try:
            parse_fort(data, catalog)
            ok += 1
        except Exception as ex:  # noqa: BLE001 — 원인별 통계를 내기 위해 모두 받는다
            key = str(ex).split(" (")[0]
            errors[key] += 1
            samples.setdefault(key, name)
    print(f"성공 {ok}, 실패 {sum(errors.values())}")
    # 실패 원인별 개수와 예시 파일
    for key, n in errors.most_common():
        print(f"  {n:4d}  {key}   예: {samples[key]}")


def main():
    """명령줄 인자 처리"""
    parser = argparse.ArgumentParser(description=".fort 도구")
    sub = parser.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("dump")
    p.add_argument("file")
    p.add_argument("--orig", default="originals")
    p.set_defaults(func=cmd_dump)
    p = sub.add_parser("verify")
    p.add_argument("--orig", default="originals")
    p.set_defaults(func=cmd_verify)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    sys.exit(main())
