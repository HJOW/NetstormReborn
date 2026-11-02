#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""OS/글꼴/설정을 바꾸지 않고 명시 문자열의 Windows NLS 변환 결과만 최초 보관한다."""
from pathlib import Path
import ctypes
import hashlib
import json
import socket

WORK = Path(__file__).resolve().parent  #새 참고값은 준비 폴더에만 쓴다.
KERNEL = ctypes.WinDLL('kernel32', use_last_error=True)  #코드 페이지 조회/순수 변환 API만 사용한다.
KERNEL.MultiByteToWideChar.argtypes = [ctypes.c_uint, ctypes.c_uint, ctypes.c_char_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_int]
KERNEL.WideCharToMultiByte.argtypes = [ctypes.c_uint, ctypes.c_uint, ctypes.c_wchar_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p]


def convert(kind, page, value):
    """명시된 실제 NLS의 strict/no-best-fit 결과만 읽는다. 글꼴이나 파일은 열지 않는다."""
    value = value.split(b'\0', 1)[0]
    if not value:
        return b''
    source_page, target_page = (page, 65001) if kind == 'to' else (65001, page)
    size = KERNEL.MultiByteToWideChar(source_page, 8, value, len(value), None, 0)
    if not size:
        return None
    wide = ctypes.create_unicode_buffer(size)
    assert KERNEL.MultiByteToWideChar(source_page, 8, value, len(value), wide, size) == size
    flags = 0x80 if target_page in (65001, 54936) else 0x400
    used = ctypes.c_int()
    used_pointer = None if target_page in (65001, 54936) else ctypes.byref(used)
    count = KERNEL.WideCharToMultiByte(target_page, flags, wide, size, None, 0, None, used_pointer)
    if not count or used.value:
        return None
    output = ctypes.create_string_buffer(count)
    assert KERNEL.WideCharToMultiByte(target_page, flags, wide, size, output, count, None, used_pointer) == count
    return None if used.value else output.raw


def main():
    """입력만 명시하며 예상 변환은 OS NLS에서 읽고 기존 fixture는 만지지 않는다."""
    global WORK  # 명시 새 출력 디렉터리에만 순수 NLS 참고값을 반복한다.
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)  # OS/설정/글꼴은 바꾸지 않는다.
    parser.add_argument('--output',type=Path,required=True)
    WORK=parser.parse_args().output.resolve()
    WORK.mkdir(parents=True,exist_ok=True)
    fixture=WORK/'font-face-encoding-win32.tsv'
    assert not fixture.exists()
    cases = [('to',1252,b'Caf\xe9'),('to',1252,b'Face\0ignored'),('to',1252,b''),('to',1252,b'\x80Font'),('to',1252,b'\x81'),('to',949,'글꼴'.encode('cp949')),('to',949,b'A'*30+b'\xb0'),('to',932,'明朝'.encode('cp932')),('to',65001,'éFace'.encode()),('to',65001,b'\xc0\xaf'),('to',65001,b'\xf0'),('from',1252,'Café'.encode()),('from',1252,'😀'.encode()),('from',1252,'∞'.encode()),('from',1252,b''),('from',1252,b'Arial\0ignored'),('from',949,'글꼴'.encode()),('from',949,'😀'.encode()),('from',932,'明朝'.encode()),('from',65001,'😀'.encode()),('from',65001,b'\xed\xa0\x80'),('from',65001,b'\xf4\x90\x80\x80'),('to',65001,'😀'.encode()),('from',1252,b'\xe9')]
    rows = ['# direction\tcodepage\tsourceHex\toutcome\toutputHex; actual Windows NLS only, no font/window/config changes']
    failed = 0
    for kind,page,value in cases:  #명시 원문 바이트/Unicode/잘린DBCS/표현불가 입력을 각각 구별한다.
        output = convert(kind,page,value)
        failed += output is None
        rows.append('\t'.join((kind,str(page),value.hex() or '-', 'throw' if output is None else 'ok', '-' if output is None or not output else output.hex())))
    fixture.parent.mkdir(parents=True,exist_ok=True)
    fixture.write_text('\n'.join(rows)+'\n','utf-8',newline='\n')
    record = dict(unit=774,host=socket.gethostname(),systemAcp=KERNEL.GetACP(),inputs=len(cases),observations=len(cases),rejected=failed,actual='Windows MultiByteToWideChar strict + WideCharToMultiByte no-best-fit/used-default check; current ACP queried only',boundary='No EnumFontFamilies/GDI font creation/OS font change/game/MP; explicit codepages1252/949/932/65001, current installed fonts are not asserted',sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),builds=0,game=0,multiplayer=0)
    (WORK/'nls.json').write_text(json.dumps(record,indent=2)+'\n','utf-8')
    print(json.dumps(dict(inputs=len(cases),rejected=failed,systemAcp=KERNEL.GetACP())))


if __name__ == '__main__':
    main()
