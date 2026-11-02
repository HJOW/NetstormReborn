#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""5차의 읽기 전용 설치 요구를 위한 현재 파일 장치/소비자와 향후 검증 범위를 고정한다."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import socket

ROOT = Path(__file__).resolve().parents[1]  # 소스/보고서의 저장소 상대 경로 기준이다.
EXTENSIONS = {'.cpp', '.h', '.inc', '.c'}  # 제품 소스와 포함되는 C/C++ 파일만 정적 점검한다.
PATTERN = re.compile(r'\b(?:std::)?(?:fopen|_wfopen|freopen|_wfreopen|_open|_wopen|_creat|CreateFile(?:2|[A-Z])?|CreateDirectory[A-Z]?|RemoveDirectory[A-Z]?|DeleteFile[A-Z]?|CopyFile(?:Ex)?[A-Z]?|MoveFile(?:Ex)?[A-Z]?|ReplaceFile[A-Z]?|SetFileAttributes[A-Z]?|GetTempPath[A-Z]?|GetTempFileName[A-Z]?|WriteFile|_wremove|_wchmod|WriteFileBytes|WriteBitmap)\s*\(|std::(?:filesystem::(?:create_director(?:y|ies)|rename|remove|copy_file|temp_directory_path)\s*\(|(?:ofstream|wofstream|fstream)\b)')  # 동적 모드/주석도 남겨 수동 분류 누락을 줄인다.


def sha(path):
    """근거 소스의 현재 전체 바이트 SHA-256을 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def reference(path, text):
    """보고서에 적은 소비자가 현재 소스에 실제 존재하는지 확인하고 줄을 고정한다."""
    source = ROOT / path
    matches = [number for number, line in enumerate(source.read_text('utf-8').splitlines(), 1) if text in line]
    assert matches, (path, text)
    return dict(path=path, lines=matches, text=text)


def family(key, name, status, old, target, refs, note):
    """생성·읽기·삭제를 같은 이관 단위로 기록하며 현재 미연결 writer도 구별한다."""
    return dict(id=key, name=name, status=status, currentLocation=old, phase5Target=target,
                references=[reference(path, text) for path, text in refs], note=note)


def inventory():
    """사용자 aaafbbc4 요구와 현재 소스를 대조한 파일군/정적 장치 목록을 만든다."""
    files = []
    for directory in ('cpppj/src', 'cpppj/third_party'):  # 분석기·테스트의 산출물은 제품 소스와 별개로 둔다.
        files.extend(path for path in (ROOT / directory).rglob('*') if path.is_file() and path.suffix in EXTENSIONS)
    files.sort()
    sites = []
    for path in files:  # 실제 코드와 주석/동적 모드를 구분해 읽기 전용 열기도 근거로 남긴다.
        for number, line in enumerate(path.read_text('utf-8', errors='replace').splitlines(), 1):
            if PATTERN.search(line):
                sites.append(dict(path=path.relative_to(ROOT).as_posix(), line=number,
                                  commentOnly=line.lstrip().startswith(('//', '/*', '*')), text=line.strip()))
    families = [
        family('options', '설정·캠페인 진행', '현재 생성/읽기', '<game>/d/options.cfg', 'UserRoot/Saves/options.cfg', [
            ('cpppj/src/client/ClientOptionsDisk.cpp', 'SaveClientOptionsInGameDirectory'), ('cpppj/src/client/ClientMain.cpp', 'configuration_([this]'), ('cpppj/src/o/ConfigInterface.cpp', 'ConfigInterface::Startup')], '기존 설정 읽기/초기 저장/변경 저장/다음 실행을 함께 전환하며 설치 폴더는 읽기 참조만 허용한다.'),
        family('options_replacement', '설정 교체 임시·복구 파일', '현재 조건부 생성/정리', 'options.cfg.save-PID-N/{next.cfg,previous.cfg}', 'UserRoot/Saves 내부의 같은 볼륨 전용 폴더', [
            ('cpppj/src/client/ClientOptionsDisk.cpp', 'MakeOptionsSaveDirectory'), ('cpppj/src/client/ClientOptionsDisk.cpp', 'RemoveOptionsSaveFiles')], '1224 교체/실패 복구/지연 정리 폴더까지 포함한다. 최종 options만 옮겨서는 충족하지 않는다.'),
        family('saved_state', '상태 체크포인트', '현재 읽기/삭제·전체 writer 잔여', '<game>/d/!state.dat', 'UserRoot/Saves/!state.dat', [
            ('cpppj/src/client/SavedStateFiles.cpp', 'SavedStateFilePath'), ('cpppj/src/client/SavedStateFiles.cpp', 'DeleteSavedStateFile'), ('cpppj/src/client/UberEditor.inc', 'reader.path=')], '선택/재시작/종료의 reader/delete를 같은 물리 경로로 바꾼다. 전체 writer 연결 완료로 세지 않는다.'),
        family('fullscreen_marker', '전체화면 표식', '현재 읽기/삭제·생성 검색 결과 없음', '<game>/fullscreenStateFile.dat', 'UserRoot 아래의 같은 표식 정책', [
            ('cpppj/src/client/ClientMain.cpp', 'void Client::ClearFullScreenState'), ('cpppj/src/client/UberGump.cpp', 'fullscreenMarker')], '현재 호환판이 생성하지 않는 표식을 새로 만들지 않는다. 존재 확인/삭제도 설치 경로를 수정하지 않도록 전환한다.'),
        family('editor_maps', '사용자 맵·편집 작업본', '현재 생성/복사/읽기/수정/삭제', '<game>-editor 아래의 fort/시나리오/작업 디렉터리', '신규 맵 UserRoot/CustomMaps, 그 밖 작업 디렉터리도 UserRoot 내부', [
            ('cpppj/src/client/EditorFiles.cpp', 'EditorWorkingRoot'), ('cpppj/src/client/EditorFortFileDevices.cpp', 'PrepareCurrent'), ('cpppj/src/client/EditorFiles.cpp', 'EditorFiles::CopyMissing')], '원래 설치 맵과 사용자 맵을 함께 읽고 신규 저장은 CustomMaps다. 재적재/삭제 tombstone과 8차 english 생성도 같은 정책을 사용한다.'),
        family('editor_metadata', '편집 장부·잠금·임시 파일', '현재 자동 생성/읽기/교체', '<game>-editor-state/catalog-v1.dat·catalog.lock·*.pending', 'UserRoot 내부 metadata namespace', [
            ('cpppj/src/client/EditorMapCatalogFiles.cpp', 'kLedger='), ('cpppj/src/client/EditorMapCatalogFiles.cpp', 'catalog.lock'), ('cpppj/src/client/EditorFiles.cpp', 'EditorFiles::WriteAtomic')], '장부 identity/sourceRoot 정책, 이전/새 설치 간 사용자 맵 보존과 충돌을 함께 설계한다. 설치 폴더 형제 위치도 쓰기 불가일 수 있다.'),
        family('gif', '게임 GIF 스크린샷', '현재 입력 시 번호 생성/존재 확인/저장', 'GifOutSpec1을 기본 설치 경로에서 해석', 'UserRoot 내부 캡처 namespace', [
            ('cpppj/src/client/GifScreenshot.cpp', 'SaveNumberedScreenshot'), ('cpppj/src/client/ClientMain.cpp', 'hooks.exists=[this](auto path)'), ('cpppj/src/client/ClientMain.cpp', 'ResolveScreenshotDiskPath(options_.gameDirectory,path)')], 'GifOutName/GifOutSpec1과 충돌 검사 reader를 저장 장치와 함께 바꾼다.'),
        family('bmp_sequence', '연속 BMP 캡처', '현재 입력 시 이름 생성/저장', 'GifSequenceName을 GameDirectory에 결합', 'UserRoot 내부 캡처 namespace', [
            ('cpppj/src/client/ClientCaptureSequence.cpp', 'hooks.prefix='), ('cpppj/src/client/ClientCaptureSequence.cpp', 'WriteIndexedBitmapFile(')], '출력 부모 디렉터리·완료 안내·번호/이름·재시작/반납까지 같은 대상이다.'),
        family('diagnostics', '객체·프로세스 진단 덤프', '현재 입력 시 생성/재열기/재쓰기', '<game>/squids.txt 및 CWD/procSumm.txt', 'UserRoot 내부 진단 namespace', [
            ('cpppj/src/client/ObjectDumpDisk.cpp', 'directory/"squids.txt"'), ('cpppj/src/client/ObjectDumpDisk.cpp', 'hooks.read='), ('cpppj/src/client/ClientDiagnostics.cpp', 'summary.open=')], 'procSumm의 CWD 의존을 제거하고 squids의 텍스트→읽기→바이너리 재쓰기 모두 같은 UserRoot 경로로 바꾼다. stdout/stderr는 현재 앱이 자동 생성하는 이름 있는 로그 파일과 구별한다.'),
        family('explicit_outputs', '명시 CLI/검사 출력', '명시 경로 출력·정책 검토 대상', '--screenshot/--audio-report/--ui-report 및 .failed.bmp/변환/설정 export', '자동 기본 출력과 구별해 5차에 전체 경로 정책 점검', [
            ('cpppj/src/app/main.cpp', 'const auto saveUiReport='), ('cpppj/src/client/ClientMain.cpp', 'std::ofstream report(options_.audioReport'), ('cpppj/src/platform/FileOutput.cpp', 'WriteFileBytes'), ('cpppj/src/platform/Bitmap.cpp', 'WriteBitmap')], '이 목록은 blanket 예외 허용이 아니다. 기본 자동 산출물/명시 export/분석 도구 파일을 구별하고 읽기 전용 설치 검증에서 누락하지 않는다.'),
        family('read_only_inputs', '현재 읽기 전용 캐시/구 서비스 입력', '현재 입력 또는 비활성', 'rootservers.dat/!rootservers.dat·기존 자산', '이관 대상 cache를 분류하고 자산은 설치 경로 읽기 전용', [
            ('cpppj/src/client/ClientMain.cpp', '!rootservers.dat')], '현재 읽기만 하는 파일을 생성 완료로 세지 않는다. 비활성 온라인 검증/security.dat writer를 되살리지 않는다.'),
        family('later_outputs', '추후 추가할 생성물', '4~8차 구현 때 갱신', '아직 전체 구현 전', 'MCP 기본 캡처/소리 포함 녹화·새 cache/log/temp도 UserRoot 내부', [
            ('AGENTS.md', '게임 구동 시 자동 생성되어야 하는 모든 파일')], '미래 writer 추가 시 이 목록과 reader를 함께 갱신한다. 원래 맵/자산은 수정하지 않는다.')]
    return dict(unit=733, date='2026-11-02', host=socket.gethostname(), userRequirementCommit='aaafbbc4',
                platformRequirement=dict(commit='61067075', target='Windows10/11 x86-64/AMD64',
                                         consideration='Windows11 ARM 지원 고려·빌드/실기 미검증',
                                         reference=reference('AGENTS.md','ARM 플랫폼용 Windows 11 지원도 고려한다')),
                productUnit=732, phase5Implemented=False, phaseOrderChanged=False,
                userRoot=r'%USERPROFILE%\Saved Games\Netstorm',
                scan=dict(files=len(files), matches=len(sites), sites=sites,
                          limit='정적 후보 목록이다. 주석/읽기 전용/동적 모드가 섞여 있으므로 모든 writer가 검증됐다는 증거가 아니다.'),
                families=families,
                acceptance=['일반 권한·실제 쓰기 거부 설치 사본에서 실행/옵션저장/재실행이 성공한다.',
                            '자동 생성/교체/백업/잠금/임시/삭제 대상과 대응 reader가 UserRoot 정책을 따른다.',
                            '다른 CWD/Unicode 사용자 홈·설치 경로에서도 설치/그 형제/CWD에 자동 파일을 남기지 않는다.',
                            '기존 설치 파일은 읽기 참조로 보존하고 사용자 맵의 신규 저장/재적재·기존 맵 병행·목록 스크롤/무제한을 확인한다.',
                            '이전 옵션 이관/두 위치 우선순위/두 설치의 사용자 데이터 충돌과 캐시 장부 identity를 결정하고 회귀 검사한다.',
                            '예외/중단·1224 매핑/교체·잠금/권한·쓰기 실패와 전체화면 종료 후 재실행도 확인한다.',
                            '자연 승리·포획/희생·특정 적 공격 재현과 공중3종 전용·새 MP 검사는 사용자 제한을 유지한다.'],
                verification=dict(builds=0, cppTests=0, gameExecutions=0, multiplayerTests=0),
                sourceSha256={path.relative_to(ROOT).as_posix():sha(path) for path in files} | {'AGENTS.md':sha(ROOT/'AGENTS.md'), 'tools/cpp_phase5_file_inventory.py':sha(Path(__file__))})


def main():
    """최초 보고서는 새 파일만 만들고 verify는 원문/목록의 완전 일치만 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--verify',type=Path)
    args=parser.parse_args()
    assert not args.output.exists()
    report=inventory()
    if args.verify:
        assert json.loads(args.verify.read_text('utf-8'))==report
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8')
    print(json.dumps(dict(families=len(report['families']), files=report['scan']['files'], matches=report['scan']['matches'], verified=bool(args.verify))))


if __name__=='__main__':
    main()
