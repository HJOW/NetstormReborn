# 기존 Fort의 일반 Editor 설치 제안

2026-10-24 **제안·미적용·미빌드**다. AGENTS와 공통/닷넷 인계 상단을 재확인했고 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 기존 정밀 C/보호 PE를 읽고 Capstone으로 명령을 정적으로 확인했다. 제안 작성 단계에서는 게임·GUI·원본실행·공유빌드·CMake·LEFT·기존 소스·git 변경이 없었다. 산출물은 [저장소에 보존한 제안 patch](patches/cpp-generic-editor-install-20261024.patch)와 이 문서다. 최초 문맥의 `git apply --check --whitespace=error`만 통과했으며 실제 컴파일/설치 성공으로 판정하지 않는다. 단위382의 Session 변경 뒤에는 현재 문맥을 다시 검토해야 한다.

## 원문 위치와 제안 범위

| 원문 | 실제 순서/관문 | 제안 |
|---|---|---|
|004b71a5~004b71e1|공통정리→Fort1→편집분기 Editor1/missionPost1/fullDeck1.최초 객체보다 앞이다.|Session 생성 입력 `CampaignInstallMode::ExistingEditor`에서 먼저 동기화한다.캠페인은 기존 입력을 보존한다.|
|004b736a~004b7397|현재 battleOwner로 Player.Create→현재 templateOwner CoreData(0)→Territory회복→World(alternate)|새 `LoadExistingEditorWorld`의 명시 부분 단계다.Player는 같은 `RawPlayerWorld::Create`를 호출한다.CoreData는 Editor/missionPost가 모두0일 때만 본체를 실행하므로 실제 Editor에서는 자연 생략한다.회복은 필수 장치로 남긴다.|
|004b5f70/004b6024~004b6078|Builder/Territory120BYTE/PlaceAll/Generate→Objects→Money→Badges→Technology→Graph(false)|새 WorldRuntime `LoadExistingFort(beforeWorld)`는 같은 `RawFortWorldLoad::Load(0)`을 실행한다.기존 캠페인 `Load`를 호출하지 않는다.네 child 직전에 현재 templateOwner를 각각 재조회하는 원본 모듈을 재사용한다.|
|004b73ae~004b7436|Deck→사제→Camera→생산UI→CoreData쓰기→전체Snapshot→State bit0해제/Notify→Subscriber/lastEditFort→EditorPlayers|World단계 뒤의 별도 외곽이다.patch는 이 전체를 완료하거나 `loaded=true`로 가장하지 않는다.|

기존 `RawCampaignWorldRuntime`는 일반 Fort의 Money/Badges/Technology를 모두 명시 오류로 두고 캠페인 부모004b5300만 실행한다. 새 `RawExistingFortChildren`은 실제 `RawFortMoneyLoad`/`RawFortMetaLoad`,진입 전 현재상태 선택,wrapper 안에서 늦게 선택하는 stream을 받는다. `MakeFortWorldLoadBinding`/`MakeFortWorldChildrenHooks`가 실제 Generate/Object/Money/Meta/Graph를 합성한다. 빈 child나 alternate/populate 성공을 만들지 않는다. `stateOwner`를 모듈·selector보다 오래 유지해 factory 지역 상태의 참조가 소멸하지 않게 한다.

WorldRuntime에서 기존 `Load()`는 그대로 캠페인이다. 새 진입은 BeginLoad→필수 beforeWorld(Player/회복)→일반Load(0)→FinishLoadAfterGraph이며 Graph(false)를 두 번 호출하지 않는다. Session `Load()`가 ExistingEditor 입력을 받으면 명시 거부한다. `LoadExistingEditorWorld()`도 전체 `loaded`를 켜지 않으므로 기존 공개 SnapshotFort의 설치 완료 계약을 우회하지 않는다. 이 제안은 기존 Fort의 실제 World/장부 로드 합성안이며 사용자 Editor 전체 진입 patch는 아니다.

## 같은 실제 자료로 child 구성

Session factory의 `CampaignExistingFortContext`는 같은 world/player/object loader/reward/SP/깊이/Storage와 **Client 단일 `RawFortBadgesGlobals::Records()`**를 빌린다. `services.fortBadges`는 병렬 [전체Snapshot 제안](../../extracted/cpp-fort-snapshot-session-20261024-proposed.patch)의 필드를 재사용한다. 이 patch에는 해당 선언을 중복 추가하지 않았다. 해당 후속 patch의 필드/포함 헤더를 먼저 병합해야 하며 파일명 변경 시 링크를 갱신한다.

factory의 구체적 연결은 다음과 같다. 반환 모듈/값형 wrapper 입력 상태/변환표는 Client 또는 `stateOwner`가 장기 소유해야 한다. factory 인자의 임시 context 구조체 자체를 callback에 참조 캡처하지 않고 각 필드가 가리키는 실제 장기 객체를 캡처한다.

- Money: `RawFortMoneyLoad(Patch1078,context.reward,context.world.Core().bookkeeping,context.player.Records(),context.loadingDepth,currentMoneyState,&context.sp,hooks)`다. `selectMoney`는 **wrapper 전** 현재 헤더/정책 전역을 선택하고 `moneyStream`은 `context.storage.SelectMoney(slot)`을 **LoadSelected의 깊이 증가/진단 뒤** 반환한다. 저장 float를 시작SP로 대입하지 않는다. 원본 Fort/Editor의 고정SP 정책과 같은 게임난수/SP저장소를 사용한다.
- Badges: `RawFortMetaLoad(core.pool,context.badges,context.loadingDepth,currentMetaState,hooks)`다. 별도6160BYTE 배열/Session별 CRT 재초기화는 금지한다. `selectMeta`는 고정 this 슬롯/현재 manager 존재/현재 templateOwner를 설정하고 실제 `SelectBadges(slot)`을 wrapper의 원문 위치에서 호출한다.
- Technology: `technologyParent`는 **`context.player.CurrentTechnology()`**의 현재 DWORD를 읽고 SID/Pool 물리범위를 확인한 뒤 변환한다. 기술 getter 뒤 `SelectTechnology(slot)`을 부른다. TypeNames 준비/현재 변환표 해제/기술UI는 실제 필수 hooks다. `contents`는 그 시점 같은 고정 슬롯의 `storage.Template(slot)`과 `MakeFortMetaContentsHooks(core.pool,context.objects,fort,hooks)`를 사용해 기존 실제 Contents/ContainedCreate로 연결한다. 최초 Fort 참조를 모든 child에 고정하거나 TypeNames/free/UI를 빈 성공으로 채우지 않는다.
- 진단 callback이 현재 owner/manager/정책/변환표를 바꿀 수 있으면 `FortMoneyState`/`FortMetaLoadState`의 현재 입력도 원본 재조회 위치에 맞춰 갱신해야 한다. 기존 값형 입력을 실제 전역 alias라고 주장하지 않는다. stream 선택을 callback 앞 C++ 인수 평가로 당기지 않는다.

## 첫 생성 전 모드와 EditorPlayers

Session hunk는 Editor/missionPost/fullDeck뿐 아니라 같은 `core.ownerMode`,Player.fortMode,Asset/Regular.fortMode,Object의 special/mission/fort/chaffMode,FinishOwner,reward의 fixedSpA/fixedSpB/halfHealthMode/aiMirrorDisabled,사제 regen의 missionPost 소비자를 첫 생성 전에 맞춘다. **Object.mission=00594fa0은false이고 Object.chaffMode/missionPost=00594fa4는true**다. 둘을 같은 bool로 합치지 않는다. Editor의 special 덕분에 저장된 실제 owner를 보존하며 모든 섬을owner1로 바꾸지 않는다.

추가 RawSurfaceWorld 두 hunk는 private `BridgeDecayMode.editor`와 실제 `bridgeState.battle`을 함께 설정한다. `globals.editor`만 켜면 기존 다리 붕괴 스캔이 계속되는 누락을 피한다. 새 `ConfigureInstallModes`는 Runtime/객체가 생기기 전만 허용한다. 장기 모드 변경/새 Session 없이 Editor↔Battle 전환까지 해결한 API는 아니다.

`InstallExistingEditorPlayers(ui)`는 world 단계 후,정확한 외곽 순서에서만 호출해야 한다. 실제 players/handle/typeCount/typeAt을 같은 Session으로 연결하고 기술 부모는 **`player.Create(owner)`**,내용물은 **`contained.Create(parent,type,0,4,0x10,0)`**를 재사용한다. Find/Ensure/CurrentTechnology로 대체하지 않는다. 번역/30BYTE format/마지막0046e980은 ui에 남는 필수 실제 경계다.0046e980은 noIsland(157)/island(94) 겹침·좌표 검사와 표면 기록·삭제·Graph를 포함하므로 빈 finish는 허용하지 않는다. 최초 정적 설명의 사제/신전 추정을 실제 PE 타입 상수 확인으로 교정했다. 이 메서드도 전체 loaded를 켜지 않는다.

## 아직 필요한 의존성과 첫 검증

| 단계 | 남은 실제 필수 경계 |
|---|---|
|현재 이름/선택/파일 설치|CurrentName/Client next/EditorFiles 작업 namespace/Subscriber는 상위가 먼저 연결한다.월드 후에 이름을 끼워 넣지 않는다.|
|Territory회복004b5ec0|실제120BYTE의 각flags&~4→같은manager Territory쓰기→State첫BYTE&~4와원본진단/로그다.patch의 `recoverEditorTerritory`는 필수 미연결이다.|
|TypeNames004bd800|255BYTE 실제할당/현재typeCount와hash 검색/기존 pointer/pos/end/깊이의전체몸체가 아직 없다.미리Parse했다는 이유로 빈 prepare/free를 넣지 않는다.별도 독립 복원이 다음 작업이다.|
|Deck004bf190|fullDeck1도 Deck.Get/ClearEntries/Set모드 후 전체 타입 순회/직접Append가 있어 호출 전체를 생략할 수 없다.기존 RawProductionDeck primitive를 사용한 부모가 필요하다.|
|CoreData/사제|CoreData읽기·쓰기는 Editor/missionPost관문으로 실제 생략한다.004923e0도 missionPost1에서 자연 반환한다.일반Fort의0분기를 성공으로 가장하지 않는다.|
|전체Snapshot/State/후처리|병렬 SnapshotFort의 공개 loaded 계약을 유지한다.설치 중에는 같은 private snapshot 본체 분리가 필요하다.그 뒤 State bit0해제/Notify/Subscriber/lastEditFort/EditorPlayers0046e980과 최종 연결·loaded 확정을 구현한다.|
|NewFort|alternate1/populateIsland,새manager/Unnamed 초기화는 이번 기존Fort 경로에 포함하지 않는다.도달하면 기존 명시 오류를 유지한다.|

첫 통합 검사는 실제 작업본 한개의 Editor 생성 입력으로 **Player 첫Pop 시점의 모드**,Territory회복/원본120BYTE,Objects→Money→Badges→Technology→Graph0 순서,실제 SP/동일 Badges 배열/실제 기술 내용물을 확인한다. TypeNames/진단/Notify callback의 owner/manager 변경과 실패 후 부분효과도 비교한다. 그 뒤 외곽을 닫아 동일 작업본 변경→전체저장→재로드·Save as·Test Battle/Return을 검증한다. 이번 제안에는 GUI/실제 저장 성공 관측이 없다.

정밀 C 근거는 `Netstorm.c:144619~144655`(회복),`144659~144710`(일반World),`145710~145801`(외곽),`152075/152135`(CoreData관문),`152382~152404`(Deck),`117507`부근(사제)다.10.72 일반World 대응주소는004330c0이며 이번 제안의 명령 대조는10.78 기준이다. 최초 PowerShell 멀티라인Python 인수 전달은SyntaxError로실패해 파일을 만들지 않았고,ASCII Base64 전달로 교정해 신규patch만 생성했다. 기존 소스를 덮어쓰지 않았다.
