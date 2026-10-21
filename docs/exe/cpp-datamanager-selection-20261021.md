# DataManager 섹션 검색·선택과 실제 미션 State 조회 — 단위263

시작 커밋은 `e569dc85`다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-CODEX / 2026-10-21**로 같다. `AGENTS.md`와 양 인수인계의 최신 작업·현행 목표·실행 보류 지시를 확인했다. 새 디컴파일·원본/CD/AGENTS/dotnetpj 수정과 게임·별도 창 실행은 없다.

이번 단위는 DataManager의 검색 `0044ce60`과 선택 `0044ceb0`을 복원하고 실제 미션 Player의 State 조회에 연결했다. 기존 코드는 같은 State 바이트와 커서를 사용했지만 manager.current를 설정하는 섹션 검색을 생략했다. 이제 `RawCampaignSession → RawPlayerTemplateState.Get → CampaignTemplateStorage.SelectState → SelectDataManagerSection`이 같은 manager 목록과 State 커서를 사용한다.

## 근거와 관측 범위

| 식별자 | 근거 | 확인한 내용 | 한계 |
| --- | --- | --- | --- |
| DS263-S1 | 현재 PC `extracted/refined/originals/Netstorm.c`와 `functions.tsv`, 보호 PE의 두 함수 명령 | head부터 current를 옮기는 대소문자 무시 검색, 첫 일치/실패, 두 선택 진단과 현재 pos 초기화 | 디컴파일은 원래 소스가 아니다. |
| DS263-N1 | `cpppj/tests/fixtures/datamanager-selection-x86.tsv`, `cpppj/datamanager-selection-native.json` | 864고유 입력·864정상 반환·28,578명령. 검색 본문 진입864회·선택432회. 전체256BYTE와 각 비교/진단 직전 같은 메모리, 반환EAX·스택 정리·EBX/ESI/EDI/EBP·x87 제어어·쓰기 범위 | CRT stricmp와 진단만 명시 외부 경계다. 실제 CRT/OS/게임 실행이 아니다. |
| DS263-B1 | `extracted/cpp-datamanager-selection-20261021-build2.log`, `cpppj/datamanager-selection-session.json` | 최종 Release 종료0·경고0·오류0, 라이브러리·게임·검사 실행 파일 생성 | C++ 검사/CTest/실제 플레이를 실행한 근거가 아니다. |

REA `open_binary`는 보호 PE를 식별했으나 Ghidra 의사코드 요청은 H: 속성 조회의 Win32 87 오류로 실패했다. 같은 실패 호출을 반복하거나 REA 설정을 변경하지 않았다. 현재 PC의 기존 정밀 산출물과 독립 PE 격리 명령 실행을 사용한 뒤 REA 세션을 닫았다. 원본 EXE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다.

제품 수정 전에 최초 관측을 완료하고 다음 세 파일을 `extracted/datamanager-selection-20261021-first-capture/`에 동결했다. 보존한 최초 fixture SHA-256은 `6216c81a623bfb9c40feeab3916264d006f3c7661d85aa15de05beeae63755b9`다. 관측을 다시 생성하지 않았다.

- `tools/decomp_datamanager_selection.py`
- `cpppj/tests/fixtures/datamanager-selection-x86.tsv`
- `cpppj/datamanager-selection-native.json`

입력은 목록0/1/3개·정방향/역방향·State/Territory/누락 이름·중복 대소문자 이름·data0/end0/비어 있지 않은 버퍼·writable0/1을 교차한다. 비교 뒤 current 교체 및 두 진단 뒤 current 교체도 포함한다. 누락 current가 진단 뒤에도0으로 남는 잘못된 메모리 경로는 정상 반환864행에 포함하지 않는다. 제품은 이 경우를 명시 오류로 거부한다.

## 제품 변경

`RawDataManagerSelection`은 manager의12BYTE와 각 노드60BYTE를 빌린다. 비교가 실패했을 때도 비교 전에 캡처한 노드 대신 **비교 후 current**의 next를 읽는다. 일치 반환·빈 버퍼 진단·최종 pos0 쓰기도 해당 지점의 current를 다시 읽는다. 다른 노드와 manager의 vptr/head, 노드의 이름·data/end/alloc/next를 보존한다.

공식 섹션 이름에는 ASCII 비교를 사용한다. 비ASCII 이름의 CRT 로캘 전체는 복원하지 않았다. 비교와 진단의 외부 효과는 독립 원문과 대조할 수 있는 명시 소비자이며 누락된 소비자를 성공으로 처리하지 않는다.

`CampaignTemplateStorage`의 실제 소유 노드/섹션 벡터를 재사용한다. `SelectState`가 같은 State 커서를 노드에 반영하고 검색/쓰기허용 선택을 실행한 뒤 pos를 같은 커서에 반영한다. 빌린 헤더도 같은 manager.current와 State 바이트를 공유한다. 일반 `State` 조회는 관측용으로 커서를 유지한다. 새 `CurrentSection` 조회로 같은 manager의 선택을 확인할 수 있다. 이 노드/벡터 연결은 호스트 어댑터이며 전체 파일/MemStream 생성 구현이라는 뜻은 아니다.

## 검증과 보류

새 C++ 검사3개를 작성하고 컴파일했다. 사용자 실행 이월 지시에 따라 실행하지 않았다.

- 원본864행의 전체256BYTE·반환·비교/진단 직전 상태와 경계 뒤 current 재조회 대조.
- 물리적 범위·NUL/ASCII 이름·빈 목록·누락 진단 연결 거부.
- 실제 Player Get에서 같은 manager의 State 선택·빌림 공유·짧은 State28BYTE 확장·재조회 커서 초기화·반납.

정적 검토에서 공용 fixture 로더가 CSV 머리글도 행으로 읽는 점을 확인해 새 검사만 머리글을 건너뛰도록 했다. 최초 fixture는 변경하지 않았다. 최종 빌드로 해당 검사 소스가 컴파일된 상태를 확인했다.

`python tools/decomp_datamanager_selection.py --verify`, `python tools/cpp_datamanager_selection_audit.py --verify`, 기존 DataManager 소멸/Template 수명v2 원문 보존 검사가 종료0이다. `git diff --check`도 종료0이다. 새 정적 감사는864행의 고유성·전체 메모리 폭·수정 필드 범위와 실제 Session 소스 연결을 확인한다. 이를 C++ 검사 통과로 합산하지 않는다.

첫 감사 기록 시 PowerShell 빌드 로그의 UTF-16 BOM 때문에 읽기가 실패했다. 원래 바이트는 `build2.raw-utf16.log`에 보존하고 동일 내용을 UTF-8 `build2.log`로 변환한 뒤 감사를 완료했다. 제품/원문 입력/기대값을 변경하지 않았다. 현재 소스와 세 바이너리 해시는 새 session 보고서에 기록했다. 기존 보고서의 과거 소스·빌드·검사 해시는 갱신하지 않았으므로 과거 감사 결과를 이번 소스의 검증으로 사용하지 않는다.

최초 커밋 직전 추가한 스테이징 바이트 검사가 native JSON의 CRLF 정규화 차이를 검출했으나 셸이 다음 명령을 이어 실행해 첫 커밋이 만들어졌다. 이를 즉시 보정했다. `.gitattributes`에서 최초 native JSON만 `-text`로 원래 바이트를 보존하고 새 소스/검사/감사 도구/문서는 LF로 고정했다. 최초 자료나 그 해시는 변경하지 않았다. session JSON만 LF로 정리한 뒤 근거에 포함한 모든 소스/자료의 스테이징 SHA를 재확인하고 같은 단위 커밋을 수정한다.

## 남은 작업

DataManager 전체 생성·파일 읽기/저장·섹션 생성/추가·MemStream 할당/쓰기를 원본 소비자에 연결해야 한다. 빈 State의 원본 초기화에는 미정 스택 바이트가 포함되므로 임의로0을 채워 완료로 처리하지 않는다. 현재는 기존 비어 있지 않은 State 조회와 기존 짧은 State 확장 연결이다. Territory 등 다른 선택 소비자, 전체 비동기 State7/전투 정리 `004b2df0`, Fort 선택/Guide 공장·가상 소멸도 남는다.

목표11미션의 자연 완주·전략AI·지형/포획/희생/대피·외관/음향/동작 동일성과 통합 실행 검증은 미완료다. 목표를 계속 active로 유지하며 이번 단위 완료와 전체 목표 달성을 구분한다. 인수인계 후 **20261021-27 CP**로 로컬 커밋하고 push하지 않는다. 마지막 정밀 디컴파일 PC는 계속 **VM-W11-CODEX / 2026-10-21**이다.
