#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""dotnetpj 초기 브리핑/재개 프레임 이행의 원본 정적 근거와 현재 구현 해시를 기록한다.

사용법: dotnetdialogframes Ghidra 읽기 전용 내보내기와 Release 게임 빌드 후
python -X utf8 tools/analyze_dotnet_dialog_frames.py
게임/GUI/에뮬레이터/테스트를 실행하지 않는다.
"""
import datetime
import json
import socket
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT, digest, read_ranges

# 보호 PE는 읽기만 하고 계획·기록·기계어 자료는 작업 공간에 저장한다.
PLAN = ROOT / 'tools/ghidra/dotnetdialogframes-functions.json'
REPORT = ROOT / 'dotnetpj/recovery-dialog-frames-evidence.json'
BINARY = ROOT / 'originals/Netstorm.exe'
FOLDER = ROOT / 'extracted/dotnetdialogframes/originals'


def main():
    """원본 함수 순서/범위와 10/7 명령을 기록하고 C# 연결·빌드의 현재 해시 및 미검증 범위를 남긴다."""
    plan = json.loads(PLAN.read_text(encoding='utf-8'))
    exported, table = FOLDER / 'creation.c', FOLDER / 'functions.tsv'
    functions = read_ranges(table)
    if [entry for entry, _ in functions] != plan['editions']['originals']:
        raise RuntimeError('원본 함수 주소/순서가 계획과 다름')
    pe = pefile.PE(str(BINARY))
    data = pe.get_memory_mapped_image()
    base = pe.OPTIONAL_HEADER.ImageBase
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    assembly, gates = [], {}
    # Ghidra가 기록한 실제 함수 범위를 원본 PE에서 실행 없이 읽는다.
    for entry, ranges in functions:
        assembly.append(f'; 함수 {entry}')
        commands = []
        # 불연속 범위 사이의 다른 함수/데이터는 이어 붙이지 않는다.
        for low, high in ranges:
            assembly.append(f'; 범위 {low:08x}-{high - 1:08x}')
            # 실제 명령/피연산자와 카운터 10/7 관련 명령만 별도로 보관한다.
            for instruction in decoder.disasm(data[low - base:high - base], low):
                line = f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}'
                assembly.append(line)
                if '0x84' in instruction.op_str or '0xc8' in instruction.op_str or instruction.op_str.endswith(', 7'):
                    commands.append(line)
        if commands:
            gates[entry] = commands
    assembly_path = FOLDER / 'assembly.txt'
    assembly_path.write_bytes(('\n'.join(assembly) + '\n').encode('utf-8'))
    files = [Path(__file__), PLAN, BINARY, exported, table, assembly_path]
    # 같은 앱 시계/세션/프레임 입력과 새 실제 잠금 구현을 함께 기록한다.
    for name in [
        'dotnetpj/src/Netstorm.Core/Simulation/MissionDialogFrameGate.cs',
        'dotnetpj/src/Netstorm.Core/Simulation/BattleSession.cs',
        'dotnetpj/src/Netstorm.Core/Simulation/BattleSessionFrameClock.cs',
        'dotnetpj/src/Netstorm.Core/Display/OriginalGameClock.cs',
        'dotnetpj/src/Netstorm.Game/FortMapViewer.cs',
        'dotnetpj/src/Netstorm.Game/FortMapViewer.Session.cs',
        'dotnetpj/src/Netstorm.Game/FortMapViewer.TutorialDialog.cs',
        'dotnetpj/src/Netstorm.Game/FortMapViewer.Units.cs',
        'dotnetpj/src/Netstorm.Game/NetstormGame.cs',
        'dotnetpj/src/Netstorm.Assets/TutorialDialogScript.cs',
        'extracted/dotnet-dialog-frame-20261013-build.log',
    ]:
        files.append(ROOT / name)
    report = {
        'last_decompile_host': socket.gethostname(),
        'recorded_at': datetime.datetime.now().astimezone().isoformat(),
        'edition': '10.78',
        'method': '같은 PC Ghidra 읽기 전용 내보내기·PE 명령/전역 정적 판독·Release 게임/Assets/Core 빌드',
        'functions': [entry for entry, _ in functions],
        'counter_commands': gates,
        'original_fields': {'stage_lock': '+84=10', 'pause_owned': '+b8', 'quiet_resume': '+c8, 7'},
        'original_predicates': {'guide_count': '005cab2c > 0', 'dialog_pointer': '00565dd4 != 0',
            'root_popup': '004641d0: 루트 직계 자식 flags 4000'},
        'implementation': {
            'briefing': 'UI 즉시 A. 열기/생성자의 초기 A. 이벤트를 10개 연속 실제 화면 프레임으로 대체·차단 다이얼로그에서 10 재설정·본문 없는 섹션도 한 번만 요청',
            'resume': '열린 스크립트 안내의 소유권 유지·안내/외부 다이얼로그/팝업에서 연속 수 0·7번째 빈 화면 프레임에 자기 ScriptDialog만 해제',
            'integration': 'Update finally에서 한 번·현재 외부 창 상태와 inputBlocked 클릭 차단 이력 구분·기존 실제 중첩 시계 공유·초기 Tell 성공 뒤 메뉴/지식 팝업 닫기',
            'tutorial': '기존 RunsTutorial과 별개 TutorialFrameLocked로 초기 잠금/실제 정지 동안 명령·규칙 틱의 단계 변경 보류·검사합 포함',
            'diagnostics': '사용자 화면에 구현 값을 표시하지 않고 기존 자동 진단의 briefing-frames/resume-frames에만 추가',
        },
        'validation': 'Release 빌드 경고 0·오류 0; 새 테스트 추가/실행·독립 PE/C# 실행 대조·GUI/게임 실행 미수행',
        'new_original_observations': 0, 'new_comparisons': 0,
        'boundaries': [
            '실제 GUI 초기 10/재개 7·중첩/반복/미션 전환·영어/한국어 회귀와 독립 원본/C# 실행 대조',
            '원본 전체 Gump 등록/삭제 수명·전체 창 flags와 클론 외부 도움말/옵션/Leave 대응의 독립 대조',
            '후속 튜토리얼 단계 +78/+80(0.5f) 및 임시 +98 타이머·기존 단계/이동/전투/건설 등 실제 시각 이행',
            '24Hz/8틱 상한·두 시각/새 경로 보간·FPS/Capture/OS 초기화·경고 창 정지 분류·실제 캠페인 완주',
        ],
        'files': {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in files},
    }
    REPORT.write_bytes((json.dumps(report, ensure_ascii=False, indent=2) + '\n').encode('utf-8'))
    print('dotnet 안내 프레임 정적 기록 완료:', len(functions), '개, PC', report['last_decompile_host'])


if __name__ == '__main__':
    main()
