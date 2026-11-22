"""원본 게임 없이 Windows UCRT의 개발자 명령 scanf 입력을 독립 관측한다."""
from pathlib import Path
import ctypes
import hashlib
import platform
import struct

# 제품 관리 파서를 import하지 않고 OS의 실제 scanf만 호출한다.
ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / 'dotnetpj/tests/Netstorm.Core.Tests/Fixtures/developer-scanf-windows.tsv'
assert platform.system() == 'Windows' and ctypes.sizeof(ctypes.c_void_p) == 8
CRT = ctypes.CDLL('ucrtbase')
SCAN = CRT.__stdio_common_vsscanf
SCAN.argtypes = [ctypes.c_ulonglong, ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_void_p]
SCAN.restype = ctypes.c_int
# float32의 double 승격 뒤 원문 %f 출력도 별도로 관측한다.
PRINT = CRT.__stdio_common_vsprintf
PRINT.argtypes = [ctypes.c_ulonglong, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_void_p]
PRINT.restype = ctypes.c_int
ROWS = []


def observe(kind, value):
    """고정 포맷과 독립 쓰기 저장소를 사용하고 결과 원시 DWORD를 보존한다."""
    raw = b'cmd ' + value
    first = ctypes.c_uint32(0x12345678)
    second = ctypes.c_uint32(0x23456789)
    pointers = (ctypes.c_void_p * 2)(ctypes.addressof(first), ctypes.addressof(second))
    format_bytes = {'one': b'%*s %d', 'two': b'%*s %d %d', 'float': b'%*s %f'}[kind]
    count = SCAN(0, raw, len(raw), format_bytes, None, ctypes.cast(pointers, ctypes.c_void_p))
    printed = '-'
    if kind == 'float' and count == 1:
        value = struct.unpack('<f', struct.pack('<I', first.value))[0]
        arguments = (ctypes.c_uint64 * 1)(struct.unpack('<Q', struct.pack('<d', value))[0])
        buffer = ctypes.create_string_buffer(512)
        result = PRINT(0, buffer, 512, b'%f', None, arguments)
        assert 0 <= result < 512
        printed = buffer.value.hex()
    ROWS.append('\t'.join([kind, raw.hex(), str(count), f'{first.value:08x}', f'{second.value:08x}', printed]))


# 부호·부분 변환·DWORD/64비트 넘침·공백·추가토큰의 실제 scanf 관측이다.
INTEGER_CASES = [b'', b' ', b'+', b'-', b'0', b'-0', b'+17x', b'12x 9', b'0x10', b'12 13tail', b'12\t-13 extra', b'1 +', b'\x85 1', b'\xa0 1', b'2147483647', b'2147483648', b'-2147483648', b'-2147483649', b'4294967295', b'4294967296', b'9223372036854775807', b'9223372036854775808', b'18446744073709551615', b'18446744073709551616', b'-18446744073709551615', b'-18446744073709551616', b'9' * 300, b'-' + b'9' * 300]
# 원문 루프 없이도 경계 양쪽을 같은 독립 OS 변환으로 관측한다.
for number in range(-40, 41):
    INTEGER_CASES.extend([str(number).encode(), (str(number) + ' ' + str(17 - number)).encode()])
for value in INTEGER_CASES:
    observe('one', value)
    observe('two', value)

# float32 반올림·서브노멀·16진·미완성 지수·NaN/Infinity의 원시 비트다.
FLOAT_CASES = [b'', b'+', b'-', b'0', b'-0', b'+0', b'.5x', b'1.', b'1e', b'1e+', b'1e- 2', b'.', b'1.#INF', b'0x', b'0xg', b'0x1', b'0x1p', b'0x1p+', b'0x1.8p2', b'-0x1.8p-2', b'0x.8', b'0x.p1', b'0x1.fffffep127', b'0x1.ffffffp127', b'0x1p-149', b'0x1p-150', b'0x1.000000000000000000001p-150', b'0x1.fffffep-127', b'0x1.000001p0', b'0x1.000001000000000000001p0', b'1.000000059604644775390625', b'1.000000059604644775390626', b'1.23456789', b'3.4028235e38', b'3.4028236e38', b'1e-45', b'1e-46', b'1e99999', b'-1e99999', b'inf', b'-inf', b'infinity', b'infi', b'infinitytail', b'nan', b'-nan', b'+nan', b'nan(foo)', b'nan(ind)', b'nan(snan)', b'nan(', b'NaN(123)', b'INF', b' 2.5 tail']
for value in FLOAT_CASES:
    observe('float', value)
OUTPUT.write_text('# Windows x64 UCRT __stdio_common_vsscanf/vsprintf options=0; 원본 PE 관측과 구별한다.\n# kind\tinput_hex\treturn\tfirst_bits\tsecond_bits\tprintf_hex\n' + '\n'.join(ROWS) + '\n', encoding='utf-8')
print(len(ROWS), hashlib.sha256(OUTPUT.read_bytes()).hexdigest())
