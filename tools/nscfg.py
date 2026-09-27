# -*- coding: utf-8 -*-
"""
원본 설정 파일(options.cfg, setup.cfg) 복호화 / 인코딩 / 값 변경 도구

포맷 명세: docs/formats/config.md

사용법:
    python tools/nscfg.py show originals/d/options.cfg
    python tools/nscfg.py set  originals/d/options.cfg startInFullScreen 0
    python tools/nscfg.py decode originals/d/options.cfg options.txt
    python tools/nscfg.py encode options.txt originals/d/options.cfg

set 은 원본 파일을 <파일>.bak 으로 백업한 뒤 수정한다.
"""
import os
import re
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from taff import xor_decode  # noqa: E402  (XOR 이므로 인코딩에도 같은 함수 사용)

# 복호화된 평문 앞에 붙는 5바이트 서명
CFG_SIGNATURE = b"mQdsT"

# 원본 텍스트 인코딩
TEXT_ENCODING = "latin-1"


def decode_file(path: str) -> str:
    """설정 파일을 복호화해 텍스트(서명 제외)로 돌려준다"""
    with open(path, "rb") as f:
        plain = xor_decode(f.read())
    if not plain.startswith(CFG_SIGNATURE):
        raise ValueError(f"설정 파일 서명이 맞지 않습니다: {path}")
    return plain[len(CFG_SIGNATURE):].decode(TEXT_ENCODING)


def encode_text(text: str) -> bytes:
    """텍스트에 서명을 붙여 인코딩한다"""
    return xor_decode(CFG_SIGNATURE + text.encode(TEXT_ENCODING))


def set_value(text: str, key: str, value: str) -> str:
    """'키 = "값"' 줄의 값을 바꾼다. 키가 없으면 끝에 추가한다"""
    pattern = re.compile(rf'^(\s*{re.escape(key)}\s*=\s*)"[^"]*"', re.M | re.I)
    if pattern.search(text):
        return pattern.sub(lambda m: f'{m.group(1)}"{value}"', text, count=1)
    newline = "\r\n" if "\r\n" in text else "\n"
    return text + f'{key} = "{value}"{newline}'


def main():
    """명령줄 인자 처리"""
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    cmd, path = sys.argv[1], sys.argv[2]
    if cmd == "show":
        print(decode_file(path))
    elif cmd == "decode" and len(sys.argv) == 4:
        with open(sys.argv[3], "w", encoding=TEXT_ENCODING, newline="") as f:
            f.write(decode_file(path))
    elif cmd == "encode" and len(sys.argv) == 4:
        with open(path, encoding=TEXT_ENCODING, newline="") as f:
            data = encode_text(f.read())
        with open(sys.argv[3], "wb") as f:
            f.write(data)
    elif cmd == "set" and len(sys.argv) == 5:
        text = set_value(decode_file(path), sys.argv[3], sys.argv[4])
        shutil.copy2(path, path + ".bak")
        with open(path, "wb") as f:
            f.write(encode_text(text))
        print(f"{sys.argv[3]} = \"{sys.argv[4]}\" 저장 (백업: {path}.bak)")
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
