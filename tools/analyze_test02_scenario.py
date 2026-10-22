#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""TEST02 원문과 원본 Test Battle 오류의 저장 증거를 대조한다. 게임이나 창을 실행하지 않는다."""
import argparse
from dataclasses import asdict
import hashlib
import json
from pathlib import Path
import socket
import struct
import subprocess
import xml.etree.ElementTree as ET

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import pefile
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

import cpp_fort_smoke
import fort

ROOT = Path(__file__).resolve().parents[1]  # 보호 원본과 기존 검사 실행 파일을 읽는 저장소 루트다.
WORK = ROOT / 'extracted/test02-review-20261022'  # analyzeManager로 얻은 이번 실제 실행의 증거 폴더다.
REPORT = ROOT / 'cpppj/test02-scenario-20261022.json'  # 게임 실행 없이 다시 생성하는 대조 보고서다.
GUIDE = 0x562ca0  # 원본 Test Battle 명령이 사용하는 Guide 전역 주소다.
SET_FLAG = 0x4639d0  # 현재 미션의 +64에 호출 인자를 쓰는 원본 함수다.
STACK = 0x2100000  # 원본 OS 스택을 대신하는 격리된 호출 입력 주소다.
MISSION = 0x1100000  # 정상 입력에서만 사용하는 불투명 미션 객체 주소다.
STOP = 0x3100000  # 원본 ret 4 뒤의 정상 복귀를 구분하는 주소다.


def digest(path):
    """원문을 변환하지 않고 SHA-256을 구한다. 보호 파일과 저장 증거 모두 같은 방식으로 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def native_set_flag(image, pointer):
    """하위 대체 함수 없이 원본 네 명령을 실행한다. NULL 입력에서는 저주소 페이지를 매핑하지 않는다."""
    mu = Uc(UC_ARCH_X86, UC_MODE_32)
    mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
    mu.mem_write(0x400000, image)
    mu.mem_map(STACK, 4096)
    mu.mem_map(MISSION, 4096)
    mu.mem_map(STOP, 4096)
    mu.mem_write(GUIDE + 0x800, struct.pack('<I', pointer))
    mu.mem_write(STACK, struct.pack('<II', STOP, 1))
    mu.reg_write(UC_X86_REG_ECX, GUIDE)
    mu.reg_write(UC_X86_REG_ESP, STACK)
    invalid = []  # 실제 기계어가 요청한 잘못된 메모리 접근을 보존한다.

    def fault(uc, access, address, size, value, _):
        """잘못된 접근의 명령 위치/주소/폭/값만 기록하고 처리하지 않은 상태로 돌려준다."""
        invalid.append(dict(eip=hex(uc.reg_read(UC_X86_REG_EIP)), access=access,
                            address=hex(address), size=size, value=value))
        return False

    mu.hook_add(UC_HOOK_MEM_INVALID, fault)
    error = None  # 에뮬레이터 오류와 정상 ret 결과를 서로 구분한다.
    try:
        mu.emu_start(SET_FLAG, STOP, count=8)
    except UcError as caught:
        error = str(caught)
    return dict(pointer=hex(pointer), eip=hex(mu.reg_read(UC_X86_REG_EIP)),
                stored=struct.unpack('<I', mu.mem_read(MISSION + 0x64, 4))[0],
                error=error, invalid=invalid)


def inspect_map():
    """최신 TEST02의 원시 영역/객체와 영어 시나리오를 기존 독립 판독기 및 C++ 로더로 읽는다."""
    original = ROOT / 'originals'
    data = (original / 'd/TEST02.fort').read_bytes()
    catalog = cpp_fort_smoke.Catalog(original, 'd', 116)
    sections, end = fort.split_sections(data)
    conversion = catalog.conversion(sections[7])
    objects = []  # 영역별 저장 순서와 청크 안의 좌표/수량/소유자를 그대로 보존한다.
    # 현재 Fort의 Terr00~Terr19만 읽고 존재하지 않는 제단이나 운반 내용을 만들지 않는다.
    for territory in range(20):
        section = sections[15 + territory]
        if len(section) <= 1:
            continue
        version, chunks = fort.read_chunks(section, conversion, catalog)
        # 청크 번호는 영역 내부의 저장 순서이며 월드 청크 좌표라고 해석하지 않는다.
        for chunk, entries in enumerate(chunks):
            # 객체의 qa와 owner를 서로 다른 원시 값으로 보존한다.
            for entry in entries:
                objects.append(dict(territory=territory, chunkOrdinal=chunk, version=version, **asdict(entry)))
    command = [str(ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe'),
               '--inspect-mission', str(original), 'TEST02']
    result = subprocess.run(command, capture_output=True, check=True, timeout=60)
    text = result.stdout.decode('utf-8')
    assert 'mission type: Tutorial' in text and 'myStartMoney = 500000' in text
    assert end == len(data) and len(objects) == 14
    # 20개 영역 레코드는 해석을 덧붙이지 않고 저장된 6바이트씩 보존한다.
    return dict(summary=fort.parse_fort(data, catalog), objects=objects,
                territoryRecords=[sections[6][index:index + 6].hex() for index in range(0, 120, 6)],
                cppInspection=text)


def inspect_live_evidence(work):
    """게임 실행 없이 완료된 A/B 조작·프로세스 종료·Windows 오류 기록을 같은 PID로 대조한다."""
    start = json.loads((work / 'out/002-start.json').read_text(encoding='utf-8'))
    session = start['sessionId']
    session_root = ROOT / 'extracted/analyzeManager' / session
    positive = json.loads((work / 'out/012-go.json').read_text(encoding='utf-8'))
    negative = json.loads((work / 'out/018-missing.json').read_text(encoding='utf-8'))
    final = json.loads((work / 'out/019-end.json').read_text(encoding='utf-8'))
    exit_state = json.loads((work / 'exit-monitor.json').read_text(encoding='utf-8'))
    event = ET.parse(work / 'application-error.xml').getroot()
    namespace = {'e': 'http://schemas.microsoft.com/win/2004/08/events/event'}
    # 동일 이벤트의 이름 붙은 필드를 읽어 다른 프로세스의 종료로 오인하지 않는다.
    fields = {item.attrib['Name']: item.text for item in event.findall('e:EventData/e:Data', namespace)}
    assert positive['error'] is None and not positive['results'][0]['isError']
    assert negative['sessionId'] == session and not final['results'][0]['running']
    assert fields['ExceptionCode'] == 'c0000005' and fields['FaultingOffset'] == '000639da'
    assert int(fields['ProcessId'], 16) == exit_state['pid']
    assert Path(fields['AppPath']).resolve() == (session_root / 'game/Netstorm.exe').resolve()
    assert exit_state['wait'] == 0 and exit_state['exitCode'] == 0xc0000005
    assert not (session_root / 'game/d/TEST02.english').exists()
    assert digest(session_root / 'game/d/TEST02.english.review-backup') == digest(ROOT / 'originals/d/TEST02.english')
    return dict(session=session, pid=exit_state['pid'], exitCode=exit_state['exitHex'],
                eventId=1000, faultOffset=fields['FaultingOffset'], faultAddress='0x4639da',
                positiveBattleScreenshot=positive['results'][0]['copy'],
                missingEnglishScreenshot=negative['results'][0]['copy'],
                stopped=final['results'][1]['closed'],
                eventRecordId=event.find('e:System/e:EventRecordID', namespace).text)


def main():
    """보호 파일의 전후 해시를 확인하고 정적/실행 증거를 합친 보고서만 새로 쓴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, default=WORK)
    parser.add_argument('--report', type=Path, default=REPORT)
    args = parser.parse_args()
    # 명시 출력 위치도 두 보호 원본의 하위인지 각각 확인한다.
    if any(args.report.resolve().is_relative_to(ROOT / protected) for protected in ('originals', 'originalCD')):
        raise ValueError('보고서는 보호 원본 폴더 밖에 저장해야 한다.')
    # 확인할 원문 목록은 보호 PE 두 판본과 최신 사용자 맵/시나리오 및 정밀 소스다.
    watched = [ROOT / name for name in ('originals/Netstorm.exe', 'originalCD/NETSTORM.EXE',
               'originals/d/TEST02.fort', 'originals/d/TEST02.english', 'originals/netstorm.tarc',
               'extracted/refined/originals/Netstorm.c')]
    # 각 원문의 최초 해시는 마지막 대조에도 같은 키/바이트 단위로 사용한다.
    before = {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in watched}
    image = pefile.PE(str(watched[0])).get_memory_mapped_image()
    normal = native_set_flag(image, MISSION)
    missing = native_set_flag(image, 0)
    assert normal['error'] is None and normal['stored'] == 1 and normal['eip'] == hex(STOP)
    assert missing['invalid'] == [dict(eip='0x4639da', access=20, address='0x64', size=4, value=1)]
    # 실제 함수의 네 명령을 주소/바이트/피연산자 순서로 보존한다.
    report = dict(schema=1, host=socket.gethostname(), lastFullDecompileHost='VM-W11-Codex',
                  lastFullDecompileDate='2026-10-22', map=inspect_map(),
                  native=dict(normal=normal, missing=missing,
                              instructions=[dict(address=hex(ins.address), bytes=ins.bytes.hex(),
                                                 mnemonic=ins.mnemonic, operands=ins.op_str)
                                            for ins in Cs(CS_ARCH_X86, CS_MODE_32).disasm(image[0x639d0:0x639e0], SET_FLAG)]),
                  live=inspect_live_evidence(args.work))
    # 원문을 다시 읽어 이 검사 도중 내용이 바뀌지 않았음을 확인한다.
    assert before == {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in watched}
    evidence = [args.work / name for name in ('exit-monitor.json', 'application-error.xml', 'actions.jsonl',
                'out/002-start.json', 'out/011-battle.json', 'out/012-go.json', 'out/015-return.json',
                'out/018-missing.json', 'out/019-end.json', 'shots/008-editor.png',
                'shots/011-briefing.png', 'shots/012-battle.png', 'shots/018-missing.png')]
    # 보관 원문과 조작/화면의 해시를 함께 남기되 영상 자체를 Git에 넣지 않는다.
    for path in evidence + [Path(__file__).resolve(), ROOT / 'tools/ns_driver.py',
                            ROOT / 'tools/cpp_fort_smoke.py', ROOT / 'tools/fort.py']:
        before[str(path.relative_to(ROOT)).replace('\\', '/')] = digest(path)
    report['sha256'] = before
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(objects=len(report['map']['objects']), nativeCases=2,
                          exitCode=report['live']['exitCode'], fault=report['live']['faultAddress'])))


if __name__ == '__main__':
    main()
