# -*- coding: utf-8 -*-
"""
PE(exe/dll) 리소스 추출 도구

추출 대상:
  - RT_BITMAP  → .bmp 파일 (+ Pillow 로 .png 변환)
  - RT_STRING  → strings.json (문자열 ID → 문자열)
  - RT_DIALOG  → dialogs.json (다이얼로그 제목·컨트롤 텍스트)
  - RT_CURSOR / RT_ICON → 원시 데이터(.bin)

사용법:
    python tools/peres.py originals/Netstorm.exe       extracted/res/Netstorm
    python tools/peres.py originals/NSENGLISHRES.DLL  extracted/res/NSENGLISHRES
"""
import json
import os
import struct
import sys

import pefile

try:
    from PIL import Image
except ImportError:  # Pillow 가 없으면 PNG 변환만 건너뛴다
    Image = None

# BMP 파일 헤더 크기 (BITMAPFILEHEADER)
BMP_FILE_HEADER_SIZE = 14

# RT_STRING 한 블록에 들어 있는 문자열 개수
STRINGS_PER_BLOCK = 16


def bitmap_to_bmp(dib: bytes) -> bytes:
    """리소스의 DIB 데이터(BITMAPINFOHEADER 부터 시작)에 BITMAPFILEHEADER 를 붙여 .bmp 파일로 만든다"""
    header_size, = struct.unpack_from("<I", dib, 0)
    bit_count, = struct.unpack_from("<H", dib, 14)
    clr_used, = struct.unpack_from("<I", dib, 32)
    # 8비트 이하 이미지는 팔레트가 붙는다 (clr_used 가 0 이면 2^bit_count 개)
    palette_entries = clr_used if clr_used else (1 << bit_count if bit_count <= 8 else 0)
    pixel_offset = BMP_FILE_HEADER_SIZE + header_size + palette_entries * 4
    file_header = struct.pack("<2sIHHI", b"BM", BMP_FILE_HEADER_SIZE + len(dib), 0, 0, pixel_offset)
    return file_header + dib


def parse_string_block(block_id: int, data: bytes) -> dict:
    """RT_STRING 블록 하나를 해석한다. 블록 N 에는 ID (N-1)*16 ~ (N-1)*16+15 가 [길이 u16][UTF-16 문자열] 형태로 들어 있다"""
    result = {}
    pos = 0
    # 블록 안의 16개 문자열 슬롯을 차례로 읽는다
    for i in range(STRINGS_PER_BLOCK):
        length, = struct.unpack_from("<H", data, pos)
        pos += 2
        if length:
            text = data[pos:pos + length * 2].decode("utf-16-le")
            result[(block_id - 1) * STRINGS_PER_BLOCK + i] = text
        pos += length * 2
    return result


def read_sz_or_ord(data: bytes, pos: int):
    """다이얼로그 템플릿의 sz_Or_Ord 필드를 읽는다. (값, 다음 위치) 를 돌려준다"""
    first, = struct.unpack_from("<H", data, pos)
    if first == 0x0000:
        return "", pos + 2
    if first == 0xFFFF:
        ordinal, = struct.unpack_from("<H", data, pos + 2)
        return ordinal, pos + 4
    end = pos
    # NUL(0x0000) 을 만날 때까지 UTF-16 문자를 읽는다
    while struct.unpack_from("<H", data, end)[0] != 0:
        end += 2
    return data[pos:end].decode("utf-16-le"), end + 2


def align4(pos: int) -> int:
    """4바이트 경계로 올림"""
    return (pos + 3) & ~3


def parse_dialog(data: bytes) -> dict:
    """일반 DLGTEMPLATE(확장형 아님)를 해석해 제목·크기·컨트롤 목록을 돌려준다"""
    style, ex_style, count, x, y, cx, cy = struct.unpack_from("<IIHhhhh", data, 0)
    if style == 0xFFFF0001:  # DLGTEMPLATEEX 는 현재 원본에서 사용하지 않으므로 표시만 한다
        return {"extended": True}
    pos = 18
    menu, pos = read_sz_or_ord(data, pos)
    wclass, pos = read_sz_or_ord(data, pos)
    title, pos = read_sz_or_ord(data, pos)
    font = None
    # DS_SETFONT(0x40) 스타일이면 글꼴 크기와 이름이 뒤따른다
    if style & 0x40:
        point, = struct.unpack_from("<H", data, pos)
        name, pos = read_sz_or_ord(data, pos + 2)
        font = f"{name} {point}pt"
    controls = []
    # 컨트롤 항목(DLGITEMTEMPLATE)을 개수만큼 읽는다
    for _ in range(count):
        pos = align4(pos)
        c_style, c_ex, cx_, cy_, cw, ch, cid = struct.unpack_from("<IIhhhhH", data, pos)
        pos += 18
        cls, pos = read_sz_or_ord(data, pos)
        text, pos = read_sz_or_ord(data, pos)
        extra, = struct.unpack_from("<H", data, pos)
        pos += 2 + extra
        controls.append({"id": cid, "class": cls, "text": text, "rect": [cx_, cy_, cw, ch]})
    return {"title": title, "font": font, "rect": [x, y, cx, cy], "controls": controls}


def main():
    """명령줄 인자 처리 및 전체 리소스 추출"""
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    src, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    pe = pefile.PE(src)
    strings, dialogs = {}, {}
    # 리소스 유형(타입) 단위로 순회
    for rtype in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        tname = pefile.RESOURCE_TYPE.get(rtype.id, str(rtype.name or rtype.id))
        # 같은 유형 안의 리소스 ID 별로 순회
        for rid in rtype.directory.entries:
            rid_name = str(rid.name) if rid.name else rid.id
            # 언어별 데이터(보통 1033 하나)를 순회
            for lang in rid.directory.entries:
                entry = lang.data.struct
                data = pe.get_memory_mapped_image()[entry.OffsetToData:entry.OffsetToData + entry.Size]
                if tname == "RT_BITMAP":
                    bmp_path = os.path.join(outdir, f"bitmap_{rid_name}.bmp")
                    with open(bmp_path, "wb") as f:
                        f.write(bitmap_to_bmp(data))
                    if Image:
                        Image.open(bmp_path).save(bmp_path[:-4] + ".png")
                elif tname == "RT_STRING":
                    strings.update(parse_string_block(rid.id, data))
                elif tname == "RT_DIALOG":
                    dialogs[rid_name] = parse_dialog(data)
                else:
                    with open(os.path.join(outdir, f"{tname}_{rid_name}.bin"), "wb") as f:
                        f.write(data)
    # 문자열·다이얼로그는 JSON 으로 저장 (있을 때만)
    for name, obj in (("strings.json", strings), ("dialogs.json", dialogs)):
        if obj:
            with open(os.path.join(outdir, name), "w", encoding="utf-8") as f:
                json.dump(obj, f, ensure_ascii=False, indent=2)
    print(f"추출 완료 → {outdir} (문자열 {len(strings)}개, 다이얼로그 {len(dialogs)}개)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
