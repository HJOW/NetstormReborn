"""시간 동기화의 동률 정렬 검사용 입력과 Windows UCRT 결과를 기록한다.

사용: python -X utf8 webpj/tools/capture-ucrt-clock-sort.py <출력 JSON 경로>
게임 실행 없이 현재 Windows의 ucrtbase.dll qsort만 합성 double 배열에 호출한다.
"""
import ctypes
import json
import random
import sys
from pathlib import Path


# C++ Poll의 offset·왕복 시각 double 쌍과 같은 물리 배치다.
class ClockPair(ctypes.Structure):
    _fields_ = [("offset", ctypes.c_double), ("rtt", ctypes.c_double)]


# 원본의 두 ordered 비교와 같은 반환값을 갖는 native 콜백 형식이다.
COMPARE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)


# 실제 정렬기가 전달한 두 쌍의 왕복 시간을 비교한다.
@COMPARE
def compare_clock_pairs(left, right):
    first = ctypes.cast(left, ctypes.POINTER(ClockPair)).contents.rtt
    second = ctypes.cast(right, ctypes.POINTER(ClockPair)).contents.rtt
    return -1 if first < second else 1 if second < first else 0


# 합성 배열을 정렬해 결과의 원래 인덱스 순서를 반환한다.
def capture_order(qsort, values):
    pairs = (ClockPair * len(values))(*(ClockPair(index, value) for index, value in enumerate(values)))
    qsort(pairs, len(pairs), ctypes.sizeof(ClockPair), compare_clock_pairs)
    return [int(pair.offset) for pair in pairs]


# 길이 1~70과 다양한 동률·역순·중복 입력을 재현 가능하게 수집한다.
def main():
    if len(sys.argv) != 2:
        raise SystemExit("사용: capture-ucrt-clock-sort.py <출력 JSON 경로>")
    library = ctypes.CDLL("ucrtbase.dll")
    qsort = library.qsort
    qsort.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t, COMPARE]
    qsort.restype = None
    randomizer = random.Random(0x4C2070)
    cases = []
    # 한 표본부터 원문의 최대 70표본까지 각각 다섯 종류의 배열을 만든다.
    for length in range(1, 71):
        inputs = [
            [0] * length,
            list(range(length)),
            list(range(length - 1, -1, -1)),
            [randomizer.randrange(4) for _ in range(length)],
            [randomizer.randrange(13) - 6 for _ in range(length)],
        ]
        # 각 합성 입력의 실제 native 순열을 결과로 저장한다.
        for values in inputs:
            cases.append({"values": values, "order": capture_order(qsort, values)})
    output = {"format": "netstorm-ucrt-clock-sort-v1", "cases": cases}
    Path(sys.argv[1]).write_text(json.dumps(output, ensure_ascii=False, separators=(",", ":")) + "\n", encoding="utf-8")


# 명시 실행할 때만 시스템 DLL을 읽고 출력 파일을 생성한다.
if __name__ == "__main__":
    main()
