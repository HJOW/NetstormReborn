#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""dotnetpj 후속 단계/임시 타이머의 10.78 기계어·실제 ROM·C# 화면 연결과 빌드 해시를 기록한다.

사용법: dotnetstageframes 읽기 전용 Ghidra 내보내기와 Release 게임 빌드 후
python -X utf8 tools/analyze_dotnet_stage_frames.py
게임/GUI/에뮬레이터/테스트를 실행하지 않는다.
"""
import datetime
import json
import re
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT, digest, read_ranges

# 보호 PE는 읽기만 하고 근거/디스어셈블 산출물은 작업 공간에 기록한다.
PLAN = ROOT / 'tools/ghidra/dotnetstageframes-functions.json'
REPORT = ROOT / 'dotnetpj/stage-frames.json'
BINARY = ROOT / 'originals/Netstorm.exe'
FOLDER = ROOT / 'extracted/dotnetstageframes/originals'
BUILD = ROOT / 'extracted/dotnet-stage-frame-20261013-build.log'
# 불변 기준 PE 해시와 원본의 없음/신전 선택/회수/에너지 표시 타이머 ROM 주소다.
ORIGINAL_SHA256 = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
ROM_DOUBLES = {'none': 0x500460, 'not_vortex_and_salvage': 0x506590, 'energy_display': 0x506588}


def main():
    """원본 함수 순서/실제 명령·타이머 ROM/현재 코드·빌드 해시와 실행 검증이 남은 범위를 기록한다."""
    if digest(BINARY) != ORIGINAL_SHA256:
        raise RuntimeError('보호 PE가 기준 SHA-256과 다름')
    plan = json.loads(PLAN.read_text(encoding='utf-8'))
    exported, table = FOLDER / 'creation.c', FOLDER / 'functions.tsv'
    functions = read_ranges(table)
    if [entry for entry, _ in functions] != plan['editions']['originals']:
        raise RuntimeError('원본 함수 주소/순서가 계획과 다름')
    pe = pefile.PE(str(BINARY))
    data = pe.get_memory_mapped_image()
    base = pe.OPTIONAL_HEADER.ImageBase
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    assembly, timing = [], {}
    # Ghidra 범위마다 원본 PE의 실제 바이트를 읽는다. 원본 함수를 실행하지 않는다.
    for entry, ranges in functions:
        assembly.append(f'; 함수 {entry}')
        commands = []
        # 불연속 범위 사이의 다른 함수/데이터를 이어 붙이지 않는다.
        for low, high in ranges:
            assembly.append(f'; 범위 {low:08x}-{high - 1:08x}')
            # 실제 명령과 +78/+80/+84/+98/게임 시각/2·4초 ROM 참조를 별도로 기록한다.
            for instruction in decoder.disasm(data[low - base:high - base], low):
                line = f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}'
                assembly.append(line)
                if any(value in instruction.op_str for value in ['0x78', '0x80', '0x84', '0x98',
                    '0x55b4d0', '0x506588', '0x506590']):
                    commands.append(line)
        if commands:
            timing[entry] = commands
    assembly_path = FOLDER / 'assembly.txt'
    assembly_path.write_bytes(('\n'.join(assembly) + '\n').encode('utf-8'))
    rom = {name: {'address': f'{address:08x}', 'value': struct.unpack_from('<d', data, address - base)[0]}
        for name, address in ROM_DOUBLES.items()}
    build_text = BUILD.read_text(encoding='utf-8')
    warnings, errors = re.search(r'경고\s+(\d+)개', build_text), re.search(r'오류\s+(\d+)개', build_text)
    if not warnings or not errors:
        raise RuntimeError('Release 빌드 로그의 경고/오류 개수를 읽지 못함')
    files = [Path(__file__), PLAN, ROOT / 'tools/analyze_styletext.py', BINARY, exported, table, assembly_path, BUILD]
    # 실제 화면 경로와 연결 전 헤드리스 경로를 함께 기록한다. 기존 fixture를 새 결과로 덮지 않는다.
    for name in [
        'dotnetpj/src/Netstorm.Core/Simulation/MissionDialogFrameGate.cs',
        'dotnetpj/src/Netstorm.Core/Simulation/BattleSession.TutorialFrames.cs',
        'dotnetpj/src/Netstorm.Core/Simulation/TutorialStages.cs',
        'dotnetpj/src/Netstorm.Core/Simulation/BattleSession.cs',
        'dotnetpj/src/Netstorm.Core/Simulation/BattleSessionFrameClock.cs',
        'dotnetpj/src/Netstorm.Core/Display/OriginalGameClock.cs',
        'dotnetpj/src/Netstorm.Game/FortMapViewer.cs',
        'dotnetpj/src/Netstorm.Game/FortMapViewer.TutorialDialog.cs',
        'dotnetpj/src/Netstorm.Game/FortMapViewer.Session.cs',
        'dotnetpj/src/Netstorm.Game/FortMapViewer.Units.cs',
        'dotnetpj/src/Netstorm.Assets/TutorialDialogScript.cs',
    ]:
        files.append(ROOT / name)
    report = {
        'last_decompile_host': socket.gethostname(),
        'recorded_at': datetime.datetime.now().astimezone().isoformat(),
        'recorded_date': '2026-10-13', 'edition': '10.78',
        'method': '같은 PC Ghidra 읽기 전용 내보내기·실제 PE 필드/시간 명령/ROM 정적 판독·Release 게임/Assets/Core 빌드',
        'functions': [entry for entry, _ in functions],
        'timing_commands': timing, 'timer_rom': rom,
        'original_fields': {'deadline': '+78 double', 'delay': '+80 float 0.5', 'lock': '+84 int 10',
            'temporary_timer': '+98 double 0/현재 게임 시각+2 또는4', 'game_time': '0055b4d0 double'},
        'precision': '004c3423 fld DWORD [esi+80]→004c3429 fadd QWORD [0055b4d0]→fstp QWORD; 디컴파일 C의 현재 시각 float 변환은 적용하지 않음',
        'implementation': {
            'stage_gate': '현재 원본 게임 시각+0.5f 확장·정지/잠금에서 단계 예약 거부·시각 도달 또는정지 상태에서만 카운터 진행/다이얼로그10 재설정·초기 요청 구분·현재 단계 Tell',
            'screen_frame': 'Update finally에서 초기/후속 잠금→7프레임 재개→실제 정지 동기화→한 번 단계 판정·명령/따라잡기 틱 중복 판정 생략·0틱/정지 화면도 단계 함수 호출·정지 중 단계 증가 거부',
            'temporary_timer': '튜토리얼2 C/F/H의 +98·같은 원본 RawGameSeconds+2/4·포함 경계·단계 변경에서0·C의 선택 해제/타이머 유지·F의 선택 변경 후 예약 유지',
            'commands': '같은 화면의 F4/회수 성공 사건만 별도 보관·일반 Drain과 독립·차단/비실행 프레임 뒤 사건 제거·과거 틱 사건 재생 없음',
            'tell': 'NotVortex 실제 Tell 콜백 성공 직후 정지/팝업 정리·같은 단계 판정의 후속 진행 거부·단계 안내는 잠금 완료 뒤 실제 OpenStage/F8 기록',
            'headless': '화면 연결 전의 기존 ProcessCommands/RunTicks·TimerTick·즉시 안내 이벤트 유지·실제 화면 전체 시간 모델의 검증 결과가 아님',
            'state': '화면 연결 시에만 절대 타이머/잠금/시각/보관 사건 검사합 추가·기존 자동 진단에만 stage-at/tutorial-time/tutorial-timer 추가',
        },
        'build': {'warnings': int(warnings[1]), 'errors': int(errors[1]), 'configuration': 'Release --no-restore'},
        'validation': '새 테스트 추가/실행·독립 PE/C# 타이머/순서/프레임 대조·GUI/게임 실행 미수행',
        'new_original_observations': 0, 'new_comparisons': 0,
        'boundaries': [
            '0.5초/10/7프레임·포함 경계/정지/창 재등장/동시 보정 안내/단계 조건·영어/한국어 실제 실행 및 독립 PE/C# 대조',
            '004c3170 가상+20 재지연의 상위 호출 연결·원본 전체 미션 이벤트 분배·튜토리얼3~6·실제 명령 ID/시점',
            'Gump 등록/지연 삭제/모든 다이얼로그·팝업 대응·정지 중 단계 함수의 실제 실행 대조·경고 창 정지 분류',
            '24Hz/8틱 상한·이동/전투/건설/생산/수확/의식/AI/애니메이션 타이머·두 시각/새 경로 보간·FPS/Capture/OS 초기화',
            'GUI의 합성 RunTicks/미션 전환/감김 시각 회귀·실제 캠페인 완주/진행 저장·Failed/TryAgain 폭·Graph/표면 identity',
        ],
        'files': {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in files},
    }
    REPORT.write_bytes((json.dumps(report, ensure_ascii=False, indent=2) + '\n').encode('utf-8'))
    print('dotnet 단계 프레임 정적 기록 완료:', len(functions), '개, PC', report['last_decompile_host'])


if __name__ == '__main__':
    main()
