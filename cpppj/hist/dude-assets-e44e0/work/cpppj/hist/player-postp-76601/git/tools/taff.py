# -*- coding: utf-8 -*-
"""
TAFF 아카이브(netstorm.tarc) 목록 조회 / 추출 도구

포맷 명세: docs/formats/taff.md

사용법:
    python tools/taff.py list    originals/netstorm.tarc
    python tools/taff.py extract originals/netstorm.tarc extracted/tarc
"""
import argparse
import os
import struct
import sys
from dataclasses import dataclass

# 파일 시작 매직 문자열 ("TAFF v0.2" + 0x1A)
TAFF_MAGIC = b"TAFF v"

# 헤더 필드 오프셋: 엔트리 개수 (u32)
OFS_ENTRY_COUNT = 0x14

# 헤더 필드 오프셋: 이름 인덱스 테이블 시작 위치 (u32)
OFS_INDEX_TABLE = 0x1C

# 헤더 필드 오프셋: 디렉터리 시작 위치 (u32)
OFS_DIRECTORY = 0x20

# 헤더 필드 오프셋: 디렉터리 크기 (u32)
OFS_DIRECTORY_SIZE = 0x24

# 헤더 필드 오프셋: 데이터 영역 시작 위치 (u32)
OFS_DATA = 0x28

# 엔트리 데이터 XOR 키 (Netstorm.exe 의 "TAFF v%d.%d" 문자열 바로 뒤에 저장되어 있음)
XOR_KEY = b"mydoghasfleas"


@dataclass
class TaffEntry:
    """아카이브 내 파일 하나의 정보"""
    name: str    # 원본 경로 (예: "\\d\\altar.type")
    offset: int  # 데이터 영역 기준 오프셋
    size: int    # 바이트 크기


def xor_decode(data: bytes) -> bytes:
    """엔트리 데이터를 복호화한다. 키 위치는 엔트리 시작 기준으로 0 부터 순환한다 (XOR 이므로 암호화에도 동일하게 사용)."""
    key = XOR_KEY
    key_len = len(key)
    # 각 바이트를 키의 (위치 % 키 길이) 번째 바이트와 XOR
    return bytes(b ^ key[i % key_len] for i, b in enumerate(data))


class TaffArchive:
    """TAFF 아카이브 읽기 클래스"""

    def __init__(self, path: str):
        with open(path, "rb") as f:
            self.raw = f.read()
        if not self.raw.startswith(TAFF_MAGIC):
            raise ValueError(f"TAFF 파일이 아닙니다: {path}")
        # 매직 문자열은 0x1A(EOF 문자)로 끝난다
        self.version = self.raw[: self.raw.index(b"\x1a")].decode("ascii")
        count, = struct.unpack_from("<I", self.raw, OFS_ENTRY_COUNT)
        dir_ofs, = struct.unpack_from("<I", self.raw, OFS_DIRECTORY)
        self.data_ofs, = struct.unpack_from("<I", self.raw, OFS_DATA)
        self.entries: list[TaffEntry] = []
        pos = dir_ofs
        # 디렉터리 레코드 [offset u32][size u32][이름 NUL 종료 문자열] 을 엔트리 개수만큼 읽는다
        for _ in range(count):
            ofs, size = struct.unpack_from("<II", self.raw, pos)
            end = self.raw.index(b"\x00", pos + 8)
            name = self.raw[pos + 8:end].decode("latin-1")
            self.entries.append(TaffEntry(name, ofs, size))
            pos = end + 1

    def read(self, entry: TaffEntry) -> bytes:
        """엔트리의 복호화된 데이터를 돌려준다"""
        start = self.data_ofs + entry.offset
        return xor_decode(self.raw[start:start + entry.size])


def cmd_list(args):
    """아카이브 내 파일 목록 출력"""
    arc = TaffArchive(args.archive)
    print(f"{arc.version}, 엔트리 {len(arc.entries)}개")
    # 엔트리마다 오프셋·크기·이름 출력
    for e in arc.entries:
        print(f"{e.offset:10d} {e.size:8d}  {e.name}")


def cmd_extract(args):
    """아카이브 내 모든 파일을 출력 폴더에 추출 (내부 경로의 '\\' 는 폴더 구분자로 변환)"""
    arc = TaffArchive(args.archive)
    # 엔트리를 하나씩 복호화해 파일로 저장
    for e in arc.entries:
        rel = e.name.replace("\\", "/").lstrip("/")
        out = os.path.join(args.outdir, rel)
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "wb") as f:
            f.write(arc.read(e))
    print(f"{len(arc.entries)}개 파일 추출 → {args.outdir}")


def main():
    """명령줄 인자 처리"""
    parser = argparse.ArgumentParser(description="TAFF 아카이브(.tarc) 도구")
    sub = parser.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("list", help="파일 목록")
    p.add_argument("archive")
    p.set_defaults(func=cmd_list)
    p = sub.add_parser("extract", help="전체 추출")
    p.add_argument("archive")
    p.add_argument("outdir")
    p.set_defaults(func=cmd_extract)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    sys.exit(main())
