#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 원본 분석 CLI의 현재 소유 세션을 조작하고 Graph/전체 Pool을 읽기 전용으로 보존한다."""
import argparse
import collections
import ctypes
from ctypes import wintypes
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]  # 원본과 기존 분석기를 가진 같은 저장소다.
MANAGER = ROOT / 'analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe'  # 기존 목적별 CLI를 사용하며 새 입력/캡처 프로토콜을 만들지 않는다.
POOL, CAPACITY, TYPES, GRAPH_TABLE, GRAPH_STACK = 0x5c8464, 0x5c847c, 0x59ab20, 0x562c18, 0x562c20  # 10.78의 실제 포인터/개수 전역이다.
SURFACE, TYPE_STRIDE, TYPE_COUNT, SLOT_STRIDE = 0x800, 500, 188, 50  # 실제 surface 플래그와 raw 자료 폭이다. 그림/장식 비트08000000과 구별한다.
FILETIME_TICKS = 504911232000000000  # Windows1601 기준을 .NET0001 기준100ns로 바꾸는 고정 차이다.


def sha(data):
    """원시 바이트만 식별하며 원문 코드나 기대값을 다시 만들지 않는다."""
    return hashlib.sha256(data).hexdigest()


def save(path, value):
    """검토 가능한 한국어 관측 JSON을 UTF-8로 저장한다."""
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')


def session_file(session):
    """명시 세션 ID만 저장소 안의 기존 분석 세션으로 해석한다."""
    if not re.fullmatch(r'[A-Za-z0-9-]{1,80}', session):
        raise ValueError('분석 세션 ID 형식 오류')
    return ROOT / 'extracted/analyzeManager' / session / 'session.json'


class Reader:
    """현재 소유 복사본의 경로/생성시각/PE를 검증하고 VM_READ 권한만 연다."""
    def __init__(self, session):
        """다른 게임/다른 프로세스에 대한 조회를 시작 전에 거부한다."""
        path = session_file(session)
        self.session = json.loads(path.read_text('utf-8'))  # 실제 분석기가 작성한 PID/생성시각 장부다.
        self.game = path.parent / 'game/Netstorm.exe'  # 기존 분석기와 같은 현재 소유 경로다.
        self.kernel = ctypes.WinDLL('kernel32', use_last_error=True)  # 입력·쓰기·중단·디버거 API는 사용하지 않는다.
        signatures = [(self.kernel.OpenProcess, [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD], wintypes.HANDLE),
                      (self.kernel.CloseHandle, [wintypes.HANDLE], wintypes.BOOL),
                      (self.kernel.QueryFullProcessImageNameW, [wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)], wintypes.BOOL),
                      (self.kernel.GetProcessTimes, [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4, wintypes.BOOL),
                      (self.kernel.GetExitCodeProcess, [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)], wintypes.BOOL),
                      (self.kernel.ReadProcessMemory, [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)], wintypes.BOOL)]
        for function, arguments, result in signatures:  # 포인터가64비트 호스트에서 잘리지 않게 ABI를 명시한다.
            function.argtypes, function.restype = arguments, result
        self.handle = self.kernel.OpenProcess(0x1010, False, self.session['processId'])  # QUERY_LIMITED_INFORMATION|VM_READ만 요청한다.
        if not self.handle:
            raise ctypes.WinError(ctypes.get_last_error())
        try:
            actual, size = ctypes.create_unicode_buffer(32768), wintypes.DWORD(32768)
            if not self.kernel.QueryFullProcessImageNameW(self.handle, 0, actual, ctypes.byref(size)):
                raise ctypes.WinError(ctypes.get_last_error())
            if str(Path(actual.value).resolve()).casefold() != str(self.game.resolve()).casefold():
                raise ValueError('소유 실행 경로 불일치')
            times = [wintypes.FILETIME() for _ in range(4)]
            if not self.kernel.GetProcessTimes(self.handle, *(ctypes.byref(value) for value in times)):
                raise ctypes.WinError(ctypes.get_last_error())
            created = (times[0].dwHighDateTime << 32) | times[0].dwLowDateTime
            if created + FILETIME_TICKS != self.session['processStartedUtcTicks']:
                raise ValueError('같은 PID의 다른 수명')
            source_sha = sha((ROOT / 'originals/Netstorm.exe').read_bytes())
            if not sha(self.game.read_bytes()) == source_sha == self.session['exeSha256']:
                raise ValueError('원문 코드 SHA 불일치')
            if self.read(0x400000, 2) != b'MZ':
                raise ValueError('실제 10.78 image base 불일치')
        except Exception:
            self.close()
            raise

    def close(self):
        """게임은 종료하지 않고 이 도구의 읽기 핸들만 반납한다."""
        if self.handle:
            self.kernel.CloseHandle(self.handle)
            self.handle = None

    def read(self, address, size):
        """생존 중인 소유 프로세스의 제한된 실제 바이트를 읽는다."""
        if not 0 < size <= 16 * 1024 * 1024 or not 0x10000 <= address < 0x80000000 or address + size > 0x80000000:
            raise ValueError('원문 읽기 물리 범위 오류')
        status = wintypes.DWORD()
        if not self.kernel.GetExitCodeProcess(self.handle, ctypes.byref(status)) or status.value != 259:
            raise RuntimeError('현재 원본 프로세스가 종료됨')
        buffer, count = ctypes.create_string_buffer(size), ctypes.c_size_t()
        if not self.kernel.ReadProcessMemory(self.handle, address, buffer, size, ctypes.byref(count)) or count.value != size:
            raise ctypes.WinError(ctypes.get_last_error())
        return buffer.raw

    def dword(self, address):
        """비정렬 DWORD의 현재 포인터/개수를 읽는다."""
        return struct.unpack('<I', self.read(address, 4))[0]


def pool_graph_identity(pool, types):
    """전체 Pool의 Graph에 쓰이는 상태·좌표·frame·번호만 현재 일관성 검사에 포함한다."""
    result = bytearray()
    for offset in range(0, len(pool), SLOT_STRIDE):  # 애니메이션 시각이나 미정 꼬리를 Graph 변화로 오인하지 않는다.
        raw = pool[offset:offset + SLOT_STRIDE]
        if raw[11] & 1:
            continue
        number = raw[10]
        if number >= TYPE_COUNT or not struct.unpack_from('<I', types, number * TYPE_STRIDE + 0xe8)[0] & SURFACE:
            continue
        result += struct.pack('<I', offset) + raw[10:22] + raw[30:41]
    return sha(result)


def snapshot(session, out):
    """실제 전체 Pool과 Graph 표의 같은 일관된 관측을 최대3번 안에서 저장한다."""
    reader = Reader(session)
    try:
        for attempt in range(3):  # 게임 상태를 멈추거나 쓰지 않고 두 읽기의 관련 바이트만 대조한다.
            addresses = {name: reader.dword(address) for name, address in [('pool', POOL), ('capacity', CAPACITY), ('types', TYPES), ('table', GRAPH_TABLE), ('stack', GRAPH_STACK)]}
            if not 5 <= addresses['capacity'] <= 200000:  # 실제 원본 설정의120000슬롯도 읽으며 물리 Pool을16비트 SID 범위로 잘라 관측하지 않는다.
                raise ValueError('실제 Pool 개수가 관측 범위를 벗어남')
            types = reader.read(addresses['types'], TYPE_COUNT * TYPE_STRIDE)
            table = reader.read(addresses['table'], 255 * 6)
            pool = reader.read(addresses['pool'], addresses['capacity'] * SLOT_STRIDE)
            again = reader.read(addresses['pool'], len(pool))
            if table == reader.read(addresses['table'], len(table)) and pool_graph_identity(pool, types) == pool_graph_identity(again, types):
                break
        else:
            raise RuntimeError('변하는 Graph/Pool을 일관된 관측으로 저장하지 않음')
        histogram, type_histogram = collections.Counter(), collections.Counter()
        for offset in range(0, len(pool), SLOT_STRIDE):  # 전체 실제 슬롯의 surface를 가상 표면 개수로 대체하지 않는다.
            raw = pool[offset:offset + SLOT_STRIDE]
            if raw[11] & 1 or raw[10] >= TYPE_COUNT or not struct.unpack_from('<I', types, raw[10] * TYPE_STRIDE + 0xe8)[0] & SURFACE:
                continue
            histogram[raw[30]] += 1
            type_histogram[raw[10]] += 1
        records = [struct.unpack_from('<HHH', table, index * 6) for index in range(255)]
        active = {index: list(record) for index, record in enumerate(records[:251]) if index and record[1]}
        blobs = dict(pool=pool, types=types, graph=table)
        for name, data in blobs.items():  # 큰 원시 입력은 현재 출력 폴더의 Git 제외 파일이다.
            (out / (name + '.bin')).write_bytes(data)
        record = dict(sessionId=session, pid=reader.session['processId'], originalExeSha256=reader.session['exeSha256'], readOnly=True, coherentReads=attempt + 1,
                      capacity=addresses['capacity'], editor=reader.dword(0x5c85a4), mission=reader.dword(0x594fa4), activeGraphRecords=active,
                      graphs=len(active), connectedSurfaces=sum(value[0] for value in active.values()), allocatedSurfaceGraphHistogram=dict(sorted(histogram.items())), surfaceTypes=dict(sorted(type_histogram.items())),
                      nativeAddresses=addresses, sha256={name: sha(value) for name, value in blobs.items()}, scope='현재 원본 프로세스의 전체 Pool/TYPE/Graph를 읽은 상태이며 코드/게임 메모리/시계는 쓰지 않는다. 원격/멀티플레이/공격·승리 재현이 아니다.')
        save(out / 'snapshot.json', record)
        return record
    finally:
        reader.close()


def main():
    """명시 세션에 대한 기존 CLI 호출 한 번 또는 현재 메모리 읽기 한 번만 수행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--operation', choices=['capture_state', 'game_input', 'game_status', 'end_session', 'restart_session', 'snapshot'], required=True)
    parser.add_argument('--request', default='{}')
    args = parser.parse_args()
    out = args.output.resolve()
    if not out.is_relative_to((ROOT / 'extracted').resolve()):
        raise ValueError('분석 출력은 저장소 extracted 내부만 허용')
    out.mkdir(parents=True, exist_ok=False)
    if not session_file(args.session).is_file():
        raise ValueError('기존 분석 세션 파일이 없음')
    if args.operation == 'snapshot':
        print(json.dumps(snapshot(args.session, out), ensure_ascii=False))
        return
    request = json.loads(args.request)
    if not isinstance(request, dict) or 'sessionId' in request:
        raise ValueError('추가 요청은 sessionId 없는 JSON 객체여야 함')
    request['sessionId'] = args.session
    save(out / 'request.json', request)
    # 새 GUI 자식이 CLI 파이프를 유지할 수 있으므로 재실행의 EOF 대기로 살아 있는 게임을 종료하지 않는다.
    process = subprocess.run([str(MANAGER), '--repo', str(ROOT), 'call', args.operation, '--args-file', str(out / 'request.json')], capture_output=True, timeout=None if args.operation == 'restart_session' else 45)
    (out / 'response.json').write_bytes(process.stdout)
    (out / 'stderr.log').write_bytes(process.stderr)
    result = json.loads(process.stdout)
    if process.returncode or result.get('isError'):
        raise RuntimeError(result)
    print(json.dumps(dict(exit=process.returncode, imagePath=result.get('imagePath'), changedRatio=result.get('data', {}).get('changedRatio')), ensure_ascii=False))


if __name__ == '__main__':
    main()
