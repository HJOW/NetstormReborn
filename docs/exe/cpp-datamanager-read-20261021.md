# DataManager 파일 이미지 읽기와 실제 초기/재로딩 연결 — 단위269

시작 커밋은 `c2a3fa5e`다. 직전 턴은 섹션 생성/복제 부모와 State 생성 연결의 구현·빌드·문서·로컬 커밋을 마친 진행 턴이었다. `AGENTS.md`와 양 인수인계를 다시 읽었다. 현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-CODEX / 2026-10-21**로 같다. 새 디컴파일·원본/CD/AGENTS/dotnetpj 변경·게임/별도 창 실행은 없다. 기존 사용자 지시대로 C++ 검사/CTest/실제 플레이 검증은 통합 단계로 이월한다.

## 변경과 실제 소비자

이전 실제 저장소는 `FortTemplate::Parse`의 섹션 벡터로 노드를 구성했다. 파일에서 각 노드를 생성하는 원본 부모의 mark 진행·빈 섹션 병합·tail 압축은 실행하지 않았다. 이번 `RawDataManagerImage`는 `0044d380`의 전체 이름 순회와 인라인 읽기/병합 본문을 같은 실제 노드/버퍼 처리에 연결한다.

| 원본 | 구현 | 복원한 처리 |
| --- | --- | --- |
| `0044d380` | `ReadDataManagerImage` | 가상 이름 표 조회→입력pos2 진단→mark 시작→각 이름의 signed 짧은 길이→new60/생성→현재 원문 복사→검색/병합→mark 진행 |
| `0044cdb0` 및 부모의 인라인 | `AdvanceDataManagerMark` | 이전 signed 길이와pos-mark 확인→현재pos를mark에 기록→signed pos/end 진단→pos+2 |
| `004a8800`/`004a8850` | 같은 SNART 이름 표 처리 | 처음 구분자/NUL을 같은 배열에서NUL/FF로 변환하고 번호 순서의 같은 이름을 조회 |

새 섹션이 비어 있을 때 **같은 이름의 기존 섹션이 있으면 새 노드/버퍼를 반납하고 기존 것을 유지**한다. 새 내용이 있으면 기존 섹션을 제거하고 새 것을 끝에 붙인다. 기존 이름이 없으면 빈 새 섹션도 붙인다. 추가 과정의 기존 tail 압축과 반납은 원본 순서다. empty 생성자는 최소1BYTE를 할당하고 이후 tail 압축 때 반납할 수 있으므로 빈 파일 섹션을 처음부터NULL로만 구성하지 않는다.

섹션 길이에서2를 뺄 때 원본16비트 wrap/부호 확장을 보존한다. source data/pos는 생성이 끝난 뒤 다시 읽고, source pos/end를 먼저 진행한 뒤 복사한다. 목적지pos는 복사 뒤0으로 돌린다. 마지막 빈 이름을 읽기 전에 mark가 한 번 더 진행하므로 입력pos는 마지막payload 끝보다2 클 수 있다. 입력end 전체를 줄이거나 파일 뒤 남은 바이트를 추가 섹션으로 읽지 않는다.

실제 `CampaignTemplateStorage.InstallFile`은 보조 해석 자료와 같은 원문을 받고 빈 새 manager에서 읽기 부모가 실제35개 노드/할당/목록을 만들게 한다. 파일 이름 순서로 가짜 목록을 먼저 설치하지 않는다. 완료한 실제 목록을 보관하고 State/Territory typed cursor도 같은 버퍼에 연결한다.

초기 `UberGump.LoadMissionWorld`는 같은 선택 파일 원문을 `missionFortImage`로 Session에 전달한다. Session은 `InstallFile`을 호출한 뒤 그 입력의 공유 참조를 반납한다. 로그인 `Reload`도 같은 VFS 원문을 다시 읽어 `InstallFile`에 전달한다. 해석에 필요한 기존 `FortTemplate::Parse`와 명시 합성 Fort를 받는 독립 Session의 `Install` 어댑터는 유지한다. 별도 합성 입력을 몰래 파일 내용으로 덮지 않는다.

이번 범위는 **파일 이미지의 섹션 읽기 부모와 초기/재로딩 소비자**다. 실제 파일 열기/파일명 비교/서명 가상 부모(`0044d980`/`004bce10`/`004bf550`), 전체 DataManager 생성/저장/오류, 기본 섹션 전체 생성(`0044d870`), 복제 전송 소비자는 완료했다고 보고하지 않는다. 원본 힙 주소/미정 padding/외부 raw 포인터 정체성 전체도 아니다.

## 제품 수정 전 원본 관측

`tools/decomp_datamanager_read.py`로 **144고유 입력·144정상 반환·176,232명령**을 관측했다. 큰 부모의 인라인 복사/검색/압축과 mark/생성/제거/SNART 하위를 합친 **아홉 실제 본문을 직접 실행**했다. CRT ASCII 비교/new/free/가상 이름 표 조회만 명시 경계다. 실제 CRT/힙/OS/게임/Windows 예외 실행은 아니다.

원본 진입 횟수는 할당504·쓰기72·mark144·소멸96·생성자432·제거96·읽기 부모144·SNART 변환72·이름 조회576회다. 입력 수와 중첩 하위 진입 수를 합산하지 않는다. 전체1400BYTE 전후·각 외부 경계 직전 상태·스택 정리·보존 레지스터·x87 제어어·정상 FS0 복구·허용 쓰기 범위를 확인했다. void 부모의 미정EAX는 반환 계약에 넣지 않았다.

전체 스냅샷은 manager와MarkStream20(양쪽 보호 포함), 노드6개, 작은 버퍼7개, 원문/대체 입력2개, 이름 표64BYTE다. 기존 이름·입력 원문·보호 바이트를 보존한다. 반복 표기도 전체 바이트로 복원하며 체크섬이나 일부 샘플로 비교하지 않는다.

입력은 빈/기존 목록, 세 섹션의0/1/3/7/16BYTE 조합, 이미 변환된/변환 전 이름 표, 할당 중 입력data 교체, free 뒤current 변경, 짧은 논리end의 현재pos/end 갱신을 교차한다. 기존 State/Territory에 빈 새 내용을 읽는 유지 분기도 포함한다. OOM/실제 예외/잘못된 포인터/음수 길이의 실패 경로 전체는 정상 관측 범위 밖이다.

제품 수정 전에 최초 도구/fixture/JSON을 `extracted/datamanager-read-20261021-first-capture/`에 동결했다. 최초 기대값을 제품에서 만들거나 성공 관측을 다시 생성하지 않았다. 같은 PC 정밀 산출물과 보호 PE의 격리 실행을 사용하며 기존 REA H: 속성 실패 호출/설정 변경은 반복하지 않았다.

## 목표 파일과 검사 상태

새 C++ 검사4개를 작성·컴파일했다. 실행하지 않았으므로 통과 수로 보고하지 않는다.

- 최초144행의 전체1400BYTE와 모든 비교/할당/free/가상 이름 경계 상태 대조.
- mark 길이 진단이pos/end를 바꾼 뒤 현재 필드를 읽는 순서.
- 가상 이름 표 조회→시작pos 진단→mark 진행과 필수 이름 장치 거부.
- 목표11파일의 실제 읽기 부모 설치와35개 원문 바이트/목록 순서/빌린 헤더/같은 State·Territory cursor.

목표의 실제 loadFort는 Early6 `BridgeTheGap`, `SecretWorkshop`, `CaptureThePriest`, `TacticalCombat`, `SubtleDefense`, `RawPower`, 캠페인 `TheWarBegins`, `MasterOfWhirligigs`, 데모 `DemoStormRages`, `GuardMyBack`, `DissolvedAlliance`다. 보호 아카이브를 Python으로 읽어 모두35개 섹션임을 확인했다. 일부 파일은35번째 섹션 뒤6/56/550/143BYTE가 남으며 이를 임의로 없애거나 추가 섹션으로 쓰지 않는다. 이 파일 구조 확인은 C++ 실행이나11미션 플레이 검증이 아니다.

최종 Release는 **종료0·경고0·오류0**이다. 로그는 `extracted/cpp-datamanager-read-20261021-build.log`·원시 stdout `.raw.log`·실제 종료 코드 `-result.json`에 보존했다. 새 session 감사는 현재 소스/속성/빌드/세 바이너리와4검사 등록 문자열을 고정했다. **C++ 검사/CTest/게임은 실행하지 않았다.**

`python tools/decomp_datamanager_read.py --verify`, `python tools/cpp_datamanager_read_audit.py`는 최초144행·기존 생성220/확장286행 보존과 현재 호출 연결에서 종료0이다. `git diff --check`도 오류가 없다. 과거 session의 소스/빌드 핀은 덮지 않으며 최초 원문 보존만 확인했다. 원본 관측/정적 감사/컴파일을 현재 C++ 실행 검증과 구분한다.

## 남은 작업

DataManager 파일 열기/서명/현재 파일명/저장·오류·전체 생성, 기본 섹션 전체 이름 표 순회0044d870·복제의 실제 전송 소비자·외부 raw 노드 포인터 수명/정체성, 전체 State7/전투 정리004b2df0·Fort 선택/Guide 공장/가상 소멸은 남는다. 이번 파일 이미지의 전체 섹션 읽기 부모와 같은 초기/로그인 재로딩 연결은 완료 항목이다.

전체11미션 자연 완주·전략AI·지형/포획/희생/대피·외관/음향/동작 동일성·통합 실행 검증은 미완료다. 목표는 active다. 최종 검증/인수인계 뒤 **20261021-33 CP**로 로컬 커밋하며 push하지 않는다. 마지막 전체 정밀 디컴파일 PC는 계속 **VM-W11-CODEX / 2026-10-21**이다.
