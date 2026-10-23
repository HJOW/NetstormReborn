# Fort 선택의 pending 이름과 다음 State 예약

시작 HEAD는 `c4009b9b / 20261023-25 CP`다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**으로 일치한다. 기존 정밀 디컴파일과 보호 PE를 읽었으며 새 디컴파일이나 원본 게임 실행은 없다.

## 원본 계약과 구현

`extracted/refined/originals/Netstorm.c:168985`의 AskEditFort(020000c9)는 mode2 → PathSpec(lastEditFort,0,0,0) → pending00460bb0 → 목록 Refresh2 → Tell LoadEditable 순서다. 콜백이 바꾼 mode/next를 복구하지 않으며 이 본문은 다음 State를 예약하지 않는다.

ReadFort(020000ca)는 같은 원문168995부터 **사건 y**의 이름을 pending으로 전달하고, 복귀 뒤의 현재 mode를 읽는다. mode0이면004b3550이 next6을, 그 외에는004b3560이 next1을 쓴다. 기존 next가 있어도 덮어쓴다. `State::Post`의 선착순 UI 전환 관문으로 대체할 수 없다.

004b3560은 next1을 먼저 쓰며 inBattle&&currentState14일 때만 다음 보존을 수행한다.

1. 현재 Fort 경로를 조회한 다음 현재 Template 슬롯으로 실제 파일을 읽는다.
2. 현재 슬롯의 State를 조회하고 첫 BYTE의 0x02 비트를 지운다.
3. 현재 double 게임 시각을 CRT `_ftol`처럼 signed64로 절삭해 하위 DWORD를 얻는다. 현재 슬롯의 State를 다시 조회해 +c에 쓴다.
4. 현재 owner의 섬 수가1 미만이면 현재 슬롯의 State를 다시 조회해 첫 BYTE의 0x04 비트를 지우고 마지막 현재 슬롯을 Notify한다.

파일 읽기와 각 Core 조회가 현재 슬롯을 바꾸는 경우도 있으므로 하나의 슬롯/State 포인터로 고정하지 않는다. 다음 상태는 진입 때 한 번만 쓰며 이후 콜백이 바꾼 next를 덮어쓰지 않는다. 다른 State 바이트와 next의 부가35DWORD는 보존한다.

새 `client/FortSelection`이 이 본체를 구현한다. `SetPendingFortName`은 첫 NUL 앞에 점이 있으면 진단하며, 진단이 복귀하면 원본처럼 이름을 복사한다. current 파일명/제목/dirty는 변경하지 않는다. `FortSelectionScriptCode`와 `QueueFortSelectionAction`은 기존 명령 parser·80칸 문자열 고리·40칸 입력 큐를 사용한다. 기존 Tell 맵 행의 ReadFort 상수도 같은 헤더를 사용하도록 옮겼다.

## 독립 원본 관측

REA로 보호 PE SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`를 열었으나 Ghidra 분석은 기존과 같은 `Read Windows path attributes failed [H:\] (Win32 87)`/target snapshot admission 오류다. 설정 변경이나 같은 요청 반복 없이 저장된 정밀 원문과 실제 PE 격리 실행을 사용했다.

`tools/decomp_fort_selection.py`는 c9의004d7408/ca의004d745f에서004d7b1a 합류까지, 별도004b3550/004b3560 본체와 CRT004e49c0~004e4a34를 실제 실행한다. **1,836입력 × 두 x87 제어어(27f/37f) = 3,672관측**, 정밀도 차이0, 실제 PE 본체77,956명령, 정규화한 종료 스택 차이0이다.

PathSpec/Pending/Refresh/Tell/현재 Fort 경로/LoadFort/Core/섬 수/Notify는 명시 외부 경계다. 이 장치에 공급한 콜백 변경과 실제 부모의 후속 mode/slot/owner/next 재조회를 구별한다. Time trace는 대체 시간 계산이 아니라 실제 `_ftol` 진입 표식이다. 원문이 게임 시각 double을 읽고 시간 DWORD를 직접 쓴다. 두32BYTE Core의 전체 바이트와 모든 효과 순서를 보존한다.

새 fixture는 `cpppj/tests/fixtures/fort-selection-x86.tsv`의 앞13열 입력/뒤9열 관측이다. 보고서는 `extracted/cpp-fort-selection-20261023-native.json`이며 기존 fixture는 수정하지 않는다. 이는 전체 입력 부모·실제 Gump·하위 파일 장치·Editor 월드 실행의 관측이 아니다.

## 검증

독립3,672행의 모든 출력, 입력 상위 플래그/뗌 관문, 실제 큐의 마지막 선택 덮어쓰기와 y 토큰, 점 진단/부분 효과/누락 장치 실패를 검사한다. `EditorFiles_FortSelectionUsesCurrentStateAndWriter`는 보호 TEST01을 새 CurrentMap 작업본에 복사하고 실제 CampaignTemplateStorage/RawPlayerTemplateState로 현재 파일 재읽기 → BYTE/DWORD 변경 → Notify → 저장 → 새 저장소로 재읽기를 수행한다. 선택한 NextMap은 pending일 뿐이며 현재 작업본의 이름은 유지되고 원본 바이트는 같다.

최종 Release 빌드5는 종료0·경고/오류0, 관련5검사는 실패check0이다. 새 통합 검사의 span/vector 비교 컴파일 오류를 수정했고 최종 감사에서 저장의 hidden 관문에 Template 슬롯이 아닌 실제 owner를 연결했다. 둘이 다른 경우도 구별하도록 숨김 입력을 보강했으며 독립 자료는 정확히3,672행을 모두 읽어야 통과한다.

수정 전 CTest1은 최종 코드로 재검증하기 위해 중단했다. 그 검사 중 실행 파일 잠금으로 실패한 중간 빌드4는 최종 검증으로 세지 않는다. 최종 전체 CTest2는 **2,352검사·실패check0·종료0/310.21초**이며 LastTest.log 내부 집계도 확인했다. 최종 검증 뒤 제품/검사 C++는 변경하지 않았다.

로그는 `extracted/cpp-fort-selection-20261023-*`다. UTF-8/추가 한국어 주석/diff/보호 경로를 확인했고 인수인계를 마감해 **20261023-27 CP**로 로컬 커밋한다. 작업 중 별도 세션의 `954acb92 / 20261023-26 DN` 배포 커밋이 들어왔으며 해당 변경과 미커밋 dotnet 파일은 이 단위에 섞지 않는다.

## 남은 실제 연결

이 단위는 **선택/예약 본체와 실제 파일·State 저장 장치 통합**이다. UberGump의 Guide/Gump 전체 입력 소비 순서와 같은 사건 묶음 안의 즉시 예약, State1/6 소비 및 Editor 설치는 아직 연결하지 않았다. 해당 명령을 UI 큐의 다음 Tick으로 미루면 같은 입력 묶음의 마지막 pending/next 동작을 잃으므로 실제 Event 어댑터를 연결할 때 주의한다. 현재 메인 Edit/ReadFort 활성화 관문은 유지한다.

다음 Editor 필수 경계는 원본004bf9a0의 현재 월드 snapshot이다. TypeNames004bd6f0 → 현재 기술/Technology004bcea0 → 실제 객체004be430 → Territory120BYTE004bf6d0 → Deck004befd0 → Money004832c0 → Mission00463a80 → Badges004be7f0 → Notify004bd360 순서다. 기존 RawFortImage의 섹션 묶음은 현재 월드의 이 writer를 대체하지 않는다. State1/2의 공유 모드 설정/기존·새 Fort 준비, generic 설치, Editor Player2..8 직접 ContainedCreate도 남아 있다.

다음 단위 준비에서 확인한 Money writer004832c0은 현재 SP를 저장하지 않는다. 공유 난수004558c0(0x3f2)을 소비하고 PE0050d7c4의333.0f를 더한 float4BYTE를 쓴다. SP.Get로 바꾸면 파일 내용과 공유 난수열이 달라진다. 다음 첫 연결은 같은 manager의 일반 쓰기 섹션 생성 경계와 TypeNames/Deck writer이며, Deck은 기존 RawCampaignProductionBindings::Refresh(owner)/RawProductionDeck::Get·Bytes를 재사용할 수 있다. 전체 snapshot 완료로 계산하지 않는다.

Early Missions·전체 캠페인·데모3개 사용자 완주, 커스텀 맵/시나리오 Edit, 다른 메인 메뉴와 GUI·음향 동일성 및 최신 배포 검증은 계속 활성 목표다. 원본/CD·AGENTS·다른 프로젝트 변경과 원격 push는 없다.
