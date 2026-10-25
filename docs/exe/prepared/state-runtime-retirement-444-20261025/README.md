# 444 실제 AI·Player 링크 정상 복귀의 반납 증빙 준비본

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. AGENTS.md·공통/닷넷 인계를 다시 읽었다. 이번에는 기존 정밀C/보호PE를 정적으로 읽고 ignored 사본만 작성했다. **main 미적용, CPP 빌드·CPP/CLI 검사·게임·GUI·Native 실행 없음**이다. 아래 검사는 작성한 초안이며 통과 결과가 아니다. 사용자의 현재 작업 완료 뒤 일시정지 요청에 따라 이 범위만 동결하며 새 작업은 시작하지 않는다.

## 실제 자식과 준비한 범위

| 보호 PE 호출 위치 | 실제 자식 | 실제 효과와 이번 증빙 |
|---|---|---|
|004b2fd3|00413ae0 resetAi(owner)|cleanup은 owner1..8을 순서대로 방문한다. 현재 Player+b0가0이면 자연 반환한다. 비0이면00410100 Destruct→CRT free→현재 Player+b0 DWORD0이다. flags20은 지우지 않는다. 실제 정상 복귀 뒤에만 BrainResetPass를 발급한다.|
|004b30ff|0048fcd0 ClearPlayerLinks|Player0..8 각 +9c DWORD0→+8 DWORD0→+68 비0일 때 free→현재 base+68 DWORD0이다. +68이0이면 free와 뒤0쓰기 모두 생략한다. 실제 전체 정상 복귀 뒤에만 PlayerLinksPass를 발급한다.|
|004b312d|0048f3b0 Core(0,1)|old current의4·4·2 캐시 보존→현재NULL/active0 정상 복귀다. 개별 AI/Links 증빙으로 대체하지 않는다.|
|004b313f|004af170 PoolReset(0)|권고하는 old 고정뷰 반납 지점은 앞선 Core0 정상 뒤, 이 진입 전이다.|
|004b31ad|004910a0 InitializePlayers|이 진입 전 모든 old Player 고정 span 소비자와 Use를 반납해야 하는 하드 한계다.|

전체 기계어·정밀C 본문과 SHA는 `native-runtime-evidence.json`에 있다. 보호 PE SHA는 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 정밀C 원시 SHA는 `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`이다. CD10.72 대응 주소는 기존 정밀C의 참고 metadata이며 이번 코드는 PATCH10.78을 대상으로 한다.

## 실제 부모의 발급·소비 API

- `RawPlayerParticipation::RemoveBrainForState(owner)`가 기존 `RemoveBrain`을 직접 실행한다. 원문 cleanup의1..8만 선택할 수 있다. 빈 brain의 자연 정상 반환도 실제 부모 복귀이므로 증빙을 발급한다. 실패한 Destruct/free의 선행 효과는 복구하지 않으며 정상 증빙도 발급하지 않는다.
- `ReturnBrainForState(pass)`가 같은 모듈의 weak lifetime/포인터/owner, 현재 brain0, 진행 중 실제 효과0을 확인하고 해당 owner의 `optional<BrainResetPass>`에 noexcept 이동만 한다. 임의 성공 setter나 cleanupDone bool은 없다. 약한 lifetime은 같은 주소에 새 모듈이 만들어진 경우도 거부한다.
- 실제 증빙을 소비한 AI owner는 `AttachBrain`, `RemoveBrain`과 실제 생성 부모 `RawAiCreate::Create`의 재사용을 거부한다. Create의 관문은 색/신전/본체 할당보다 앞이다. 따라서 새 brain을 설치한 뒤 이전 증빙만 보고 소멸 정리를 건너뛸 수 없다. raw Player 배열을 우회 변경하면 `BrainReturnedForState`의 현재 정체성 재조회가 명시 수명 오류를 낸다. 이를 허용된 새 생성이나 정상 전체 반납으로 취급하지 않는다.
- `RawProductionDeck::CleanupPlayerLinksForState(currentPlayers)`가 기존 `CleanupPlayerLinks`에 매 원문 접근의 현재 Player getter와 **자기 실제 Release**를 공급한다. parent body를 복제하거나 free를 관찰 callback으로 바꾸지 않는다.
- `ReturnPlayerLinksForState(pass)`는 같은 실제 모듈/weak lifetime/호출 정상 복귀, 실제 소유 블록 전부 반납, 현재9 Player의 세 DWORD0을 확인한다. `optional<PlayerLinksPass>`에 실제 증빙을 소비한 경우에만 `~RawProductionDeck`의 old Player 재조회를 생략한다. 새 Get/Release/Bytes 등 실제 Deck 소비는 거부한다. 현재 필드나 블록이 남으면 부분 효과를 유지한 채 소비를 거부한다.
- AI 파괴/free와 Links 부모 전체의 동기 호출 대여를 RAII로 유지한다. 효과 callback이 이전 pass를 들고 재진입해도 실제 호출이 끝나기 전에는 소비할 수 없다. 실제 자식 복귀 뒤 증빙 발급/optional 보관에는 후행 allocation이 없다. 새 C++ 수명 identity의 할당은 모듈 구성 시 수행한다.
- `RawCampaignSession::{ResetAiForState,ReturnAiForState,ClearPlayerLinksForState,ReturnPlayerLinksForState}`는 위 실제 모듈의 발급/소비만 위임한다. `~Impl`은 해당 **같은 모듈이 실제 증빙을 소비한** owner/Deck만 반복 RemoveBrain/ReleasePlayerLinks에서 제외한다. 일반 경로와 기존 plain RemoveAi/ReleasePlayerLinks는 기존 효과를 유지한다. 전체 Session 정리를 완료했다는 공개 setter는 없다.

## 전체 반납과의 경계

이번은 실제 AI/Links 자식의 정상 복귀 증빙과 그 소비자 중복 효과 방지만 준비한 것이다. **CleanupStateWorld의 제품 coordinator에 연결하지 않았다.** 실제 resetAi hook은 각 owner 위치에서 발급/소비하고, ClearPlayerLinks effect는 자기 위치에서 발급/소비해야 한다. 발급 함수 호출만 건너뛴 채 소비 증빙을 만들 수 없다.

`~ProductionDeck`은 실제 링크 증빙을 소비한 경우에만 old span을 다시 읽지 않는다. 다른 참여/Player/Factory/내용물/생산/미션/로그인 소비자와 서비스 callback의 고정뷰는 여전히 Session에 있다. 개별 AI/Links 증빙과 Core0 정상 복귀만으로 `ClientPlayerRecords::Use`를 먼저 반납하면 안 된다. 모든 고정뷰 소비자 소멸 후 마지막 Use를 반납하고 PoolReset/InitializePlayers를 이어야 한다. 새 집중 검사도 Core0 뒤 살아 있는 Session에서는 실제 Player 재할당이 거부되며, 전체 Session 소멸 뒤에만 계속할 수 있음을 관측한다.

442의 `ClearProcessesForState`/`ReturnProcessBindingsForState`와 현재 binding만 정리하는 일반 종료는 그대로 보존했다. 이번 AI/Links 증빙을 Kernel 단일 Clear나 다른 process logic/System의 반납 증빙으로 대체하지 않는다. Regular/SharedRegular 밖의 잔류 process/effectSystem, AI/Player 뷰를 포함하는 전체 모듈 반납, Gump Kill/Cleanup/Replace와 새 preSession HUD, 입력/커서 해제의 정확한 위치는 여전히 후속이다.

기존 `ClientGameplayGlobals::Use::~`는 일반 종료에서 known0·CursorDrop 기본 대입·editor0을 쓴다. 따라서 미래 Native 중간 반납에서 현재 일반 Session 소멸을 그대로 호출하면 후반 Native store를 앞당길 수 있다. 게임 필드 쓰기 없이 소비할 실제 Gameplay lease와 전체 old View 소멸 coordinator가 추가로 필요하다. 이번은 이를 가짜 완료로 우회하지 않는다. Main Edit State1/전체 사용자 편집·저장/전체 Early·캠페인·데모 완주도 계속 미완료다.

## 사본과 병합 기준

제품7파일과 기존 검사3파일, 총10파일의 사본이다. **Session.h/.cpp의 입력 base는 동결된442 준비본**이고 나머지는 당시 main815fbd67의 파일이다. `manifest-original.json`에 각 원래 source 경로/SHA/바이트를 기록하고 `main-before/`에 정확한 원시 base를 보존했다. 439440441442와 공통 Session 파일은 hunk 병합하며 준비 사본 통째로 덮지 않는다. 현재439~442 묶음에444를 넣지 않는다.

새 검사12개는 기존 등록된 `PlayerParticipationTests.cpp`의5개, `ProductionDeckTests.cpp`의4개, `CampaignSessionTests.cpp`의3개다. 따라서 CMake 신규 등록은 없다. 기존 TEST_CASE 본문/Native fixture/기대 숫자는 바꾸지 않고 필요한 include와 새 사례만 추가했다. manifest에 기존 사례 이름 및 본문 보존 SHA와 새 이름을 구분한다.

- 참여5개: owner1..8 실제 Destruct/free/flags 보존, 실패 prefix/증빙 부재, foreign·이동·소멸 뒤 동일 주소 거부, 실제 파괴효과 재진입 거부, retired Create의 선행 거부와 raw 정체성 변경 진단.
- Deck4개: 실제36회 current getter/정확 세 DWORD·다른 BYTE 보존, 선택 free 실패와 소비 시점 현재 필드 재확인, foreign·이동·동일 주소 재사용 거부, 실제 전체부모 호출 대여와 빈 Deck27회 getter.
- Session3개: 실제 AI18할당/해제와9Deck/Kernel 소멸 순서, 다른 Session 증빙 거부·실제 링크 부분실패, Core0 뒤에도 필요한 전체 고정뷰 대여와 최종 Player 재할당 경계.

## 동결·검증 상태와 재개

사용자 일시정지 요청으로 독립 추가 리뷰는 착수하지 않았다. 444 작성자는 제품 API/생성·파괴 순서와 새 검사 입력을 정적으로 읽었다. **빌드/검사/게임 실행 통과를 주장하지 않는다.** root가 재개 뒤 병합·빌드하고 위12개와 기존 Participation/AI Brain·Create/PlayerLinks/ProductionDeck/Session/Kernel/StateCleanup 회귀를 검증해야 한다.

`prepared.patch`는 저장한 base10파일에 대한 UTF-8 unified patch, `manifest.json`은 준비본/증빙/patch SHA와 새 사례 목록이다. `archive-files.json`은 장기 Git 보관에 필요한 patch·metadata·base의 정확한 경로/바이트를 나열한다. 생성 결과 cpppj10사본은 검토 편의를 위한 출력이며 base+patch에서 재현할 수 있다. main 문서/CPP/원본/AGENTS를 바꾸거나 원격 push하지 않았다.
