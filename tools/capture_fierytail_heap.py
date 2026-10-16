#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""분석기가 실행한 검증된10.78 복사본의 Fiery Tail 색 표/인접 메모리를 읽기만 한다."""
import argparse
import csv
import ctypes as ct
import hashlib
import json
import struct
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 분석 세션과 보호 PE의 기준 경로다.
PALETTE, TABLE, CAPACITY = 0x55a750, 0x595138, 0x59513c  #10.78 공유 색 표와 실제 표시 목록 전역이다.
FIERY_VTABLE = 0x509538  # TYPE11의 실제 파생 가상 표다.
REPORT = ROOT / 'cpppj/fierytail-live-heap.json'  # Git에 보존하는 실제 관찰 원문/범위와 전체 SHA다.


def verify():
    """실제 관찰 원문과 독립 생성자 fixture를 비교하며 인접 DWORD를 색으로 만들어 넣지 않는다."""
    facts = json.loads(REPORT.read_text(encoding='utf-8'))
    # 현재 보호 PE·관찰기·독립 생성자 입력의 바이트가 기록과 같아야 한다.
    for name, expected in facts['sha256'].items():
        assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == expected, name
    with (ROOT / 'cpppj/tests/fixtures/fierytail-lifetime-x86.tsv').open(encoding='utf-8') as stream:
        rows = [row for row in csv.reader(stream, delimiter='\t') if row and not row[0].startswith('#')]
    original = next(row for row in rows if int(row[1]) == 0x027f and row[2] == 'GIFCLOUD.COL,0,0,-1,50,127,0,1')
    count = 0
    # 원문60바이트 중44바이트만 실제 색 표다. 나머지는 인접 할당 메모리로 보존한다.
    for capture in facts['captures']:
        for palette in capture['palettes']:
            raw = bytes.fromhex(palette['raw'])
            assert palette['count'] == 11 and len(raw) == 60 and raw[:44] == bytes.fromhex(original[6])
            assert struct.unpack_from('<I', raw, 44)[0] == facts['observedAdjacentDwords'][0]
            count += palette['samples']
    assert count == facts['paletteObservations'] and facts['observedAdjacentDwords'] == [1]
    print('Fiery Tail 실제 heap 원문/SHA 감사:', count, '팔레트 읽기, 인접 DWORD1은 이 실행의 관찰값')


def main():
    """세션 PID/경로/PE SHA를 검증한 뒤 같은 게임의 메모리만 읽고 원문 증거를 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session')
    parser.add_argument('--seconds', type=float, default=45)
    parser.add_argument('--output')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        verify()
        return
    if not args.session or not args.output:
        parser.error('--session과 --output이 필요합니다')
    if not 0 < args.seconds <= 60 or '/' in args.session or '\\' in args.session:
        raise ValueError('관찰 시간/세션 ID 오류')
    directory = ROOT / 'extracted/analyzeManager' / args.session
    session = json.loads((directory / 'session.json').read_text(encoding='utf-8'))
    expected = (directory / 'game/Netstorm.exe').resolve()
    digest = hashlib.sha256(expected.read_bytes()).hexdigest()
    assert digest == session['exeSha256'] == hashlib.sha256((ROOT / 'originals/Netstorm.exe').read_bytes()).hexdigest()
    kernel = ct.WinDLL('kernel32', use_last_error=True)  # GUI/프로세스 상태 변경 권한 없이 조회/메모리 읽기만 한다.
    kernel.OpenProcess.argtypes = [ct.c_uint32, ct.c_int, ct.c_uint32]
    kernel.OpenProcess.restype = ct.c_void_p
    kernel.QueryFullProcessImageNameW.argtypes = [ct.c_void_p, ct.c_uint32, ct.c_wchar_p, ct.POINTER(ct.c_uint32)]
    kernel.QueryFullProcessImageNameW.restype = ct.c_int
    kernel.ReadProcessMemory.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_void_p, ct.c_size_t, ct.POINTER(ct.c_size_t)]
    kernel.ReadProcessMemory.restype = ct.c_int
    kernel.CloseHandle.argtypes = [ct.c_void_p]
    process = kernel.OpenProcess(0x1010, False, session['processId'])  # QUERY_LIMITED_INFORMATION|VM_READ만 요청한다.
    if not process:
        raise OSError(ct.get_last_error(), '분석 게임 읽기 핸들을 열지 못했습니다')
    try:
        name, length = ct.create_unicode_buffer(32768), ct.c_uint32(32768)
        assert kernel.QueryFullProcessImageNameW(process, 0, name, ct.byref(length))
        assert Path(name.value).resolve() == expected

        def read(address, amount):
            """지정한 격리 게임 메모리를 모두 읽지 못하면 실패로 보고한다."""
            data, received = ct.create_string_buffer(amount), ct.c_size_t()
            if not kernel.ReadProcessMemory(process, ct.c_void_p(address), data, amount, ct.byref(received)):
                raise OSError(ct.get_last_error(), f'{address:08x}+{amount}')
            assert received.value == amount
            return data.raw

        def word(address):
            """실제32비트 게임 포인터/개수를 little endian으로 읽는다."""
            return struct.unpack('<I', read(address, 4))[0]

        start, samples, errors, unique, effects = time.monotonic(), 0, 0, {}, []
        # 동일 게임의 현재 할당을 관찰하며 타임아웃을 프로세스 종료로 해석하지 않는다.
        while time.monotonic() - start < args.seconds:
            try:
                samples += 1
                palette = word(PALETTE)
                if palette:
                    header = read(palette, 8)
                    count, pointer = struct.unpack('<II', header)
                    if count == 11 and pointer:
                        content = read(pointer, 60)
                        if read(palette, 8) == header and word(PALETTE) == palette:
                            key = f'{palette:08x}:{pointer:08x}:{content.hex()}'
                            current = unique.setdefault(key, dict(palette=palette, pointer=pointer, count=count, prefix=read(pointer-16,16).hex(), raw=content.hex(), firstSeconds=time.monotonic()-start, samples=0))
                            current['samples'] += 1
                table, capacity = word(TABLE), word(CAPACITY)
                if table and 0 < capacity <= 10000 and len(effects) < 200:
                    pointers = struct.unpack('<' + 'I' * capacity, read(table, capacity * 4))
                    # 실제 목록의 TYPE11 객체만 읽으며 다른 앱이나 게임의 임의 영역은 검색하지 않는다.
                    for pointer in pointers:
                        if not pointer or word(pointer) != FIERY_VTABLE:
                            continue
                        raw = read(pointer, 0x68)
                        capacity, total, particles = struct.unpack_from('<III', raw, 0x24)
                        if capacity == 120 and total <= capacity and particles:
                            particle_bytes = read(particles, total * 24) if total else b''
                            effects.append(dict(seconds=time.monotonic()-start, pointer=pointer, raw=raw.hex(), particles=particle_bytes.hex()))
            except OSError:
                errors += 1  # 실행 중인 게임을 정지시키지 않아 객체 생성/삭제 경합은 관찰 오류로 남긴다.
            time.sleep(.005)
        result = dict(session=args.session, pid=session['processId'], executable=str(expected), exeSha256=digest,
                      heapMode=word(0x5e5370), smallBlockThreshold=word(0x5e535c), heapHandle=word(0x5e536c),
                      samples=samples, readErrors=errors, elapsedSeconds=time.monotonic()-start, palettes=list(unique.values()), effects=effects,
                      limitations=['읽기 전용 실제 게임 관찰이며 디버거 중단/WriteProcessMemory/게임 파일 수정은 없다.', '프로세스를 정지시키지 않아 각 객체/입자의 전체 원자적 snapshot은 아니다. 읽기 경합 수를 함께 보존한다.', '색수 뒤16바이트는 관찰한 이 할당의 실제 인접 메모리다. 다른 실행/allocator에서 같은 값이라는 증거는 아니다.', '표시 목록의 객체/입자를 읽은 것은 실제 Draw의 명령 실행 또는 색 조회 주소 추적 증거가 아니다.'])
        output = Path(args.output).resolve()
        assert output.is_relative_to(ROOT / 'extracted')
        output.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
        print(json.dumps(dict(samples=samples, readErrors=errors, palettes=len(unique), effectSnapshots=len(effects), output=str(output)), ensure_ascii=False))
    finally:
        kernel.CloseHandle(process)


if __name__ == '__main__':
    main()
