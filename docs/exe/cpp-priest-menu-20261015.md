# 사제 건설 메뉴와 선택 관문 복원

2026-10-15, 현재 PC와 마지막 정밀 디컴파일 PC는 **VM-W11-Codex**다. AGENTS.md·두 인수인계 문서를 확인했다. 기존 정밀 프로젝트에서 PATCH10.78 함수 6개를 `-Refined`·읽기 전용으로 내보냈다. REA 스킬의 근거 구분 원칙을 적용했으며, 루트에서 REA Ghidra 시작이 Windows 경로 속성 오류(Win32 87)로 실패한 뒤 기존 Ghidra 선택 내보내기와 PE 기계어의 격리 실행을 사용했다. 보호 원본 실행·수정·GUI 실행은 이 단위에서 하지 않았다.

## 구현 범위

새 `client/RawPriestMenu`는 현재 raw Pool·TYPE·FrameCode·9×180바이트 Player·건설 제외 타입 목록·ContainedFinderState를 직접 참조한다. 메뉴 계산이나 기술 내용을 다른 게임 상태로 복제하지 않는다.

| PATCH 주소 | 복원 내용 | C++ API | CD10.72 참고 대응 |
| --- | --- | --- | --- |
| `00494870` | 사제의 신전/워크샾/Construct/Prayer 하위 메뉴와 기본 객체 메뉴 위임 | `Open` | `0040d2f0` |
| `00461cf0` | 한 건설 행의 표시·허용·제외 타입 조건 | `AddBuild` | `00416350` |
| `00461bd0` | 워크샾 테마·금액 확인 후 미리보기 Form 생성/취득 | `SelectBuild` | `00416270` |
| `00493ac0` | 현재 Technology 내용물의 원소 테마 조회 | `HasTheme` | `0040ec90` |
| `004ad710` | 현재 물리 프레임의 첫 signed 문자에서 `'A'`를 뺀 상태 | `FrameState` | `004ae4b0` |
| `004c2400` | 타입 허용 getter의 원본 호출 지점 | 외부 `allowed`에 같은 `RawTotalMade::Allowed`를 연결 | 이번 단위 비교 미수행 |

CD 주소는 기존 전체 정밀 출력의 후보 대응이며 이번 신규 구현·실행 대조는 PATCH 전용이다. 생성자는 CD 판본을 거부한다. `004c2400`의 실제 상태·집계 구현은 공용 `RawTotalMade` 단위이며 이 모듈에서 중복 구현하지 않았다.

`BindPriestMenuControls`는 기존 `MenuGumpWidgets::BeginFancy/BeginVertical`, `MenuGumpControls::AddTitle/AddText/AddSubmenu/AddSeparator`, `MenuGump::TakeCurrent`를 사용한다. 새 메뉴 노드·출력·수명 구현을 만들지 않는다. `BindPriestMenuWorld`는 같은 Pool의 실제 `RawPriestState`, `SquidProcessHost`, `RawPlayerWorld`, `RawCarrierPreDestroy`, `SquidReward`, `RawContainedCreate`를 검사해 연결한다. 실제 커서 취득 `00447000`, 원본 기본 객체 메뉴 `004288f0`, 현재 SP 조회, 번역/서식기와 허용 표는 상위의 명시 경계다. 도달한 경계가 없으면 예외로 거부한다.

## 원본에서 확인한 조건

- 사제 owner BYTE는 로컬 owner DWORD 전체와 비교한다. 내 사제이고 같은 Host에 사건 `0x493`이 없을 때만 사제 전용 하위 메뉴를 구성한다. 모든 정상 분기의 마지막은 `004288f0` 기본 객체 메뉴 호출이다.
- 신전은 현재 Player `+0x7c/+0x84` 중 하나라도 존재하면 행을 비활성화하고 신전 하위 메뉴도 `~xTemple`·비활성으로 만든다. 워크샾은 허용된 원소 타입이 하나 이상이고 세 원본 모드 중 하나가 켜진 경우에만 나타난다. 네 워크샾의 기술 테마 검사 순서는 **3, 0, 1, 2**다.
- 원본 `00475c00`은 인자를 받지 않지만 호출자가 미리 올린 enabled/checked는 뒤의 `00478160`이 소비한다. 디컴파일 C의 두 인자 호출만 보고 조건을 버리면 안 된다. C++는 각 조건을 `AddSubmenu`에 명시 전달한다.
- Construct는 사제 이동 불가·사건 `0x25d`·현재 FrameCode K~O(상태 10~14)에서 비활성이다. 현재 물리 프레임 번호 자체나 raw WORD `+12`로 이 상태를 대신하지 않는다. 이동 불가인 경우에만 Construct 문구에 `~r`이 붙는다.
- 비용 부족은 건설 행의 enabled를 끄지 않는다. 부족금액의 `~r`과 원소 아이콘 번호를 바꾸고, 행의 enabled는 명시 비활성 및 실제 제외 타입 목록만으로 결정한다. 기술 허용이 꺼진 타입은 행 자체를 생략한다.
- 건설 선택은 워크샾 테마 확인 뒤 SP를 두 번 읽는다. 첫 비교는 비용을 **float로 저장한 값**, 둘째 비교는 **정수 FILD 값**과 SP float의 비교다. 2²⁴ 경계에서 둘은 다를 수 있다. 둘째 잔액 부족은 안내 없이 종료하며 첫 부족은 `BuildNoBucks`, 워크샾 테마 부족은 `NoTechTheme`를 요청한다. NaN은 두 비교에서 지불 불가다. editor는 해당 제한을 우회하되 원본의 금액 조회 순서는 유지한다.
- 성공하면 `Contained(parent,type,0,0x20,0x10,0)`을 생성하고 `00447000(form,0x160)`을 요청한다. 이 단계는 건설된 자산 생성이나 HP 진행이 아니다.

## 독립 기계어 대조

`tools/decomp_priestmenu.py`는 보호 PE 바이트를 Unicorn 가상 메모리에 읽고 Ghidra가 확인한 함수 범위·기존 CRT `__ftol` 몸체만 실행한다. 등록되지 않은 실행 주소는 즉시 거부한다. 모든 호출에서 정상 반환·스택·보존 레지스터·x87 제어어를 확인한다. 원본 게임 진입점/OS 호출은 실행하지 않는다.

최종 **고유 입력 905개**는 다음과 같다.

| 범위 | 고유 입력 | 직접 관찰 | 명시 경계 |
| --- | ---: | --- | --- |
| 전체 사제 메뉴 | 329 | 원본 `00494870`·`00461cf0`·상태 wrapper·Frame getter의 호출/분기와 UI 전체 효과 인자·순서 | 현재 허용/제외·SP·번역/설명·HP/사건/내용물/테마 query·GUI 효과 |
| 건설 선택 | 64 | 원본 `00461bd0`의 float/정수 금액 비교·모드/테마 분기·두 SP 조회 횟수·생성/커서 요청 | 비용/SP·테마 query·ContainedCreate·커서 취득/안내 |
| 기술 테마 getter | 256 | 원본 `00493ac0`의 flags4/테마/flags2 `0x800` 조건·정상 반환 | 현재 Technology·내용물 cursor의 명시 후보 순서·TYPE 주소 |
| 프레임 상태 getter | 256 | 원본 `004ad710`의 모든 signed-char 바이트 반환값 | 현재 TYPE/FrameCode 주소 |

초기 메뉴 입력 334개 중 같은 baseline 5개를 제거해 329개로 확정했다. 초기 398개/중간 910개 관찰은 같은 계열의 중간 결과이며 최종 905개에 더하지 않는다. 기존 helper·내용물 finder·메뉴 저장소의 이전 검증 수도 새 원본 관찰에 합산하지 않는다.

Fixture는 `cpppj/tests/fixtures/priestmenu-x86.tsv`, 근거 SHA/경계/개수는 `cpppj/priestmenu.json`이다. `python tools/decomp_priestmenu.py --verify`는 보호 PE·전체/선택 내보내기·현재 소스/검사·도구·fixture의 SHA와 각 종류의 행 수·입력 중복을 감사한다. 새 C++ 검사는 전체 메뉴·선택·getter512개 전수 대조 및 같은 실제 기술 체인 변경/부적격 속성/현재 signed 프레임 변경을 포함한다.

## 검증 및 남은 연결

최종 Release `netstorm_tests` 빌드는 경고/오류 **0**, 최종 집중 **4개·실패 체크0**이며 `--verify`의 모든 SHA/개수/중복 감사와 `git diff --check`가 통과했다. 전체 CTest는 getter512개 전수 검사를 추가하기 전의 검증이다. 상위에서 **1/1·내부1,174개·실패0, 121.34초/전체121.37초**를 확인했고, 이후 최종 네 번째 검사와 메뉴 baseline 중복 정리는 위 최종 집중 4개에서 따로 확인했다. 최종 네 번째 검사를 포함한 전체 CTest를 다시 실행했다고 주장하지 않는다. 초기 집중3통과 뒤 이 단위의 production 동작 변경은 없었다.

실제 `UberGump`의 사제 우클릭 경로 및 `RawCampaignSession`에 이 모듈은 아직 연결하지 않았다. 현재 Construct 비활성 경로를 이 단위가 활성화했다고 주장하지 않는다. 실제 cursor 선택/회전/취소·MayPlace→Local→Request/Confirm/Notice→운반 도착→건설 Process/Visual·생산/다리/수확·튜토리얼·승패/저장·Early Missions와 캠페인 1-1~1-2 완주 및 원본 픽셀/음향 동일성은 후속이다. 원본 DirectDraw 전체화면 오류 수정은 범위에서 제외한다.
