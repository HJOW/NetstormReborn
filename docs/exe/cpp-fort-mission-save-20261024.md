# Guide의 현재 Mission 저장 본체 복원

2026-10-23 시작 후2026-10-24 완료한 **신규 독립 준비·미등록** 자료다. AGENTS.md와 공통 최신 상단/닷넷 인수인계를 다시 읽었고 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. 기존 정밀 C와 보호10.78 PE만 읽었으며 새 디컴파일·게임/GUI·공유빌드·기존 제품/CMake/LEFT/helper 변경·git은 하지 않았다. 신규 격리 경로의 컴파일·창 없는 검사는 실행했다. 실제 Client/Session 저장 바인딩은 후속이며 이 본체 완료를 Edit 사용자 저장/시나리오 왕복 완료로 계산하지 않는다.

## 원본 순서와 공개 API

보호 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`다. 정밀 원문은 `extracted/refined/originals/Netstorm.c:80901`,150360,105427을 읽고 각 실제 PE 명령을 다시 확인했다.

| 원본 본체 | 정확한 효과 | 신규 API |
|---|---|---|
| **00463a80**,43BYTE | Guide+800 현재 Mission 비0일 때만 아래 이름 저장 → **그 뒤 현재 Guide+800 재조회** →필드 저장. 현재가0이면 이름/stream/필드를 건드리지 않는다. | `SaveFortMission(stream,hooks)` |
| **004bcbf0**,92BYTE | null 문자열이면 Template.cpp155 진단 후 실제 빈 문자열 선택 →첫 NUL 길이 계산 →길이의 **하위WORD2BYTE** 일반쓰기 →같은 원문 주소의 **최초 전체길이** 일반쓰기. NUL은 쓰지 않는다. 길이0도 두 번 쓴다. | `SaveFortMissionName(stream,optional<NUL포함span>,hooks)` |
| **00482c40**,57BYTE | 호출 때 선택한 Mission+58 sequence DWORD를 임시에 캡처/쓰기 →그 뒤 **같은 Mission+54의 현재 flags**를 다시 읽어 캡처/쓰기. | `SaveFortMissionState(stream,selectedMission,hooks)` |
| **0044cce0**,96BYTE | signed32 용량 진단 →현재 data/pos 재조회→DWORD/나머지BYTE 전방복사→현재pos/end 갱신. | 기존 `WriteMemStream`을 write hook에 연결 |

`current()`는 Guide 현재 토큰, `name()`은 같은 Guide 원문의 optional span, `field(selected,offset)`는 **선택한 Mission**의 실제+58/+54 순수조회다. `write(stream,source)`는 실제 일반 MemStream writer, `assertion(NullName)`은 도달한 null 진단이다. [모듈 헤더](../../cpppj/src/o/RawFortMissionSave.h)와 [본체](../../cpppj/src/o/RawFortMissionSave.cpp)는 이 세 함수만 제공한다.

일반 Battle/Tutorial 구분이나 guided-only 조건은 없다. **현재 Mission이 있으면 같은 본체**다. 문자열 쓰기 중 Mission1→2가 되면 새2의 필드를 쓰지만, sequence쓰기 중 Guide현재가2가 되어도 이미 선택한1의 flags를 쓴다. 첫 길이쓰기 중 원문을 `Z NUL Y`로 바꾸어도 두 번째 쓰기는 새 NUL을 검색하지 않고 처음 캡처한 길이만큼 같은 주소를 읽는다. 이름 span/선택 Mission은 동기 write 중 수명이 보존되어야 하며 필드 hook을 매번 현재 Mission으로 바꿔 해석하면 안 된다.

물리적으로 NUL 없는 span,32비트 주소 길이 밖,선택된 미연결 hook은 명시 예외다. 원문의 비정상 호스트 메모리 접근을 C++에서 무제한 스캔하지 않는다. null 이름은 독립004bcbf0 진입에서 가능하며 정상 Guide는 자기 주소를 넘기므로 Guide 경로에서 null 이름을 관측했다고 주장하지 않는다.

## Template의 Mission 섹션 부모와 소유 분담

Fort004bf9a0의 Mission 구간은 **004bfa71~004bfaa9**, Battle004bf880은 **004bf94a~004bf982**다. 두 구간 모두 Money 호출 후 스택정리 일부를 포함한다. 실제 순서는 현재 `this->manager` 읽기/비0 검사 →없으면 Template.cpp**0x1e8(488)** 진단 → **현재 manager 재조회** →Create(manager,"Mission")0044d1b0 →반환 stream 캡처 →Guide00562ca0의00463a80 호출이다.

이번 보고서에 두 구간48관측을 별도 보관했다. Template 진단/Create와 전체 Fort/Battle 순서는 병렬 `RawFortSnapshotParent`가 소유하므로 제품 모듈에 중복 부모를 추가하지 않았다. [snapshot 설계](cpp-editor-snapshot-next-20261023.md)의 `mission(stream)` hook이 이 모듈의 `SaveFortMission`으로 이어지면 된다. 실제 Mission이0이어도 **부모가 만든 새 빈 Mission 섹션은 남는다**. 기존 섹션을 그대로 보존하거나 부모 Create를 생략하지 않는다.

10.72 정밀 대응은 Guide00427830/문자열00426920이며 Guide의 ret N과 구조가10.78과 다르다. sequence/flags00482c40의 CD 대응은 현재 정밀 표에서 확정되지 않았다. 이번 API/fixture를 CD 동작으로 확대하지 않는다.

## 제품보다 먼저 확정한 독립 관측

[생성기](../../tools/decomp_fort_mission_save.py)는 보호 PE를 격리 Unicorn에 복사한 뒤 위288BYTE 본체와 두56BYTE 부모 구간만 실행한다. 파일/OS/게임 진입은 없다. 원본 밖 Create/진단은 명시 외부 경계다. **0044cce0은 경계에서 인자를 기록한 뒤 실제 기계어를 계속 실행**하므로 용량 진단·현재stream 재조회·복사·pos/end를 Python으로 대체하지 않았다.

- 제품대조 fixture: **115고유입력×x87제어어27f/37f=230관측**. Guide 현재0/1/2, 독립 필드/이름 진입, 빈/NUL뒤 바이트/31BYTE·65535/65536/65537 길이, pos/end/alloc·signed end/capacity, 이름/포인터/sequence/flags callback 변경과 선택하지 않는 외부효과를 포함한다.
- 별도 Template 구간: **48관측**. Fort/Battle, manager0/11, 현재Mission0/1, 진단중manager22 및 Create중Mission2 변경을 포함한다. 제품대조230에 합산하지 않는다. DataManager 생성 몸체는 실행하지 않았다.
- 두 묶음 합계278관측 중 **정상266/명시중단12**, 실제 **3,384,982명령**, 두 정밀도 차이0이다. 중단은 Mission0/buffer0/capacity/write2/write3/null 각2다.
- Mission0은 원본+58 읽기 직전, buffer0은 실제 writer가 data/pos를 읽은 뒤 복사 직전에 **명시 물리 중단**한다. OS fault/정상 반환으로 가장하지 않는다. capacity/null/write 중단은 지정 외부 callback이 멈추며 앞선 효과를 보존한다.

65535 경계는 독립 문자열 함수 입력이다. 실제 Guide는+800에 현재 포인터가 있으므로65536BYTE 이름을 정상 Guide 입력으로 가장하지 않는다. 길이 WORD는 wrap해도 두 번째 쓰기의 전체 길이는 줄지 않음을 실제 PE가 결정했다.

최초 [fixture](../../cpppj/tests/fixtures/fort-mission-save-x86.tsv) SHA256은 **`ead46e5bd2408705cce7f79de814b767e03af872e0d0682e85409eb4803c9966`**다. 생성기·fixture·보고서를 **제품 작성 전** 확정했다. [보고서](../../cpppj/fort-mission-save-native.json)는 보호입력/정밀원문/helper/생성기/fixture SHA와48개 부모 관측을 보관한다. 생성기는 기존 fixture/보고서 덮어쓰기를 거부하고 `--verify`로 SHA만 확인한다. 제품을 맞추려고 원문 기대값을 수정하지 않았다.

출력은 실제16BYTE stream 전체, 두0x11000BYTE 물리버퍼 각각의 전체FNV-1a/앞64·뒤16BYTE,각 write의 길이/입력FNV,진단·호출순서,현재Guide/두Mission필드를 비교한다. FNV는 관측 형식이며 보호 근거 무결성은 별도 SHA256이다.

## 검사와 실제 연결 경계

[FortMissionSaveTests.cpp](../../cpppj/tests/FortMissionSaveTests.cpp)의 `FortMissionSave_IndependentNative230`은 입력열만으로 제품을 실행하고 이후230개 관측/명시중단12를 비교한다. writer는 기존 `WriteMemStream` 본체를 사용한다. `CurrentAndPhysicalBoundaries`는 현재0의 선택되지 않은 hook, Guide두 번/이름한 번 조회, 고정선택Mission flags, NUL누락·진단미연결,빈 이름의2/0BYTE 두 쓰기를 추가 확인한다. CMake용 fixture macro는 **`NETSTORM_FORT_MISSION_SAVE_FIXTURE`**다.

`extracted/fort-mission-save-20261024-isolated`의 독립 runner에 신규모듈/검사와 기존 RawMemStream/RawDataManagerLifetime만 연결했다. MSVC14.51 `/std:c++20 /EHsc /utf-8 /W4` 격리 컴파일은 경고0/종료0, **2검사/실패check0/종료0**이다. 공유 CMake·제품·netstorm_tests·빌드디렉터리는 변경/실행하지 않았다.

최초 링크는 기존 `DestroyMemStream` 정의 파일 누락으로 실패했으며 기존 RawDataManagerLifetime.cpp를 링크해 해결했다. 최초 실행의 buffer0는 기존 MemStream이 hook 호출 전에 내는 null 물리 거부를 검사 runner가 예상된 native중단으로 분류하지 못했다. **해당 null/비영길이 예외만** 명시 buffer0중단으로 대응하도록 검사 장치를 수정한 뒤 재컴파일/검사했다. fixture/생성기/제품 본체는 바꾸지 않았다. 알 수 없는 제품 예외를 정상관측으로 삼키지 않는다.

후속 Client hook은 guided 여부와 무관한 실제 MissionRuntime/Guide 이름과 selected token 수명을 연결해야 한다. Session은 같은 Storage stream에 `WriteMemStream`을 연결하고 Template 부모가 Mission 섹션을 생성한다. 실제 시나리오 저장/재파싱·재시작의 sequence/flags 보존, 전체snapshot/Notify·Money·Badges·005c89c0 공유와 Edit 진입/작업본/Save as/Test Battle/Return은 아직 별도다. [Client 후속 설계](cpp-editor-client-next-20261023.md)를 따른다.
