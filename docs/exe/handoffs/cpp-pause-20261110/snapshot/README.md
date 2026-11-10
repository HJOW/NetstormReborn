# NoWorld Fort/Battle Snapshot 구현 후보

2026-11-10 / HJOW-Athlon. 제품 단위1266 이후 접합용 후보다. 폴더 이름1265는 부모의 후속 debugBurst 단위와 별개다. **제품 미적용·미링크·게임/MP/단위 실행0**이며, 최종 후보 소스 두 cpp를 MSVC `/Zs` 단일 명령으로 구문 검사해 종료0을 확인했다. 제품 CMake·cpppj/src·LEFT_JOBS·docs·AGENTS·원본/CD/fixture는 이 후보 작업에서 수정하지 않았다.

## 파일과 API

- `cpppj/src/client/NativeFortSnapshot.h/.cpp`: 전체 Fort/Battle 저장과 조건부/무조건 현재칸 wrapper, 순수 Notify다.
- `cpppj/src/client/NativeFortPreparation.h/.cpp`: 기존 m2a 준비-prefix를 현행 API로 별도 구문 확인한 후보다. 004b3620의 실제 장기 Template/섬/지도 준비를 하고 snapshot callback을 마지막 원문 위치에서 호출한다.
- `candidate-syntax-final.log`·`candidate-syntax-final-exit.txt`: 최종 네 소스의 바이트를 고정한 뒤 두 cpp만 `/std:c++20 /utf-8 /EHsc /Zs` 단일 명령으로 검사한 결과다(종료0). `candidate-source-final-before-syntax.json`과 최종 해시가 같다. obj/lib/EXE 생성과 제품 빌드는 하지 않았다. 앞선 개별 로그와 로그경로 충돌 관측은 별도로 보존한다.
- `original-fragments.c`, `candidate-manifest.json`: 같은 호스트의 기존 정밀 생성 C에서 읽은 원문과 고정 해시/정적 범위다. 새 디컴파일은 하지 않았다.

`SnapshotNativeFort(client, templateStorage_, *missionOptions_, *missionRestrictions_, FortMissionSaveEffects(), editorCatalog_->Files(), slot)`는004bf9a0 전체 부모다. `SnapshotNativeBattle`은 같은 인자로004bf880을 실행한다. `SnapshotNativeCurrent(..., true)`는004bfb50의 최초 manager0 자연 반환을 먼저 보존한다. false는004bfac0의 독립 Battle/Fort 두 if이며 선행 manager 관문이 없다. `NotifyNativeFortTemplate(..., slot)`는004bd360만 실행한다.

## 실제 접합 순서

1. 제품 동결/빌드가 끝난 뒤 네 후보 소스를 현재 코드에 **작은 hunk로** 넣고 두 cpp를 제품 CMake에 등록한다. 현행 NativeFortSaveVirtual을 계속 사용하며 옛 m2c1 파일로 덮어쓰지 않는다. 필요한 새 제품 파일은 이 네 개뿐이다.
2. UberGump 안의 같은 `templateStorage_`, `missionOptions_`, `missionRestrictions_`, `FortMissionSaveEffects()`와 `editorCatalog_->Files()`를 넘긴다. 다른 Template/Player/Pool/허용표 사본을 만들지 않는다. 실제 Native만 이 경로로 보내고 독립 typed/CD 기존 계약은 별도로 유지한다.
3. 일반 SaveGoMain(현행 Uber1470)과 EditorTestBattle snapshot(1422)의 `world_` 선행 거부를 없애고 `SnapshotNativeCurrent(...,true)`에 연결한다. 기존 입력 뗌/Yes·No/next3·Guide 순서와 이후 current name/State 동작은 바꾸지 않는다.
4. JoinTestBattle(1520)의 준비는 `PrepareNativeFortDescend(client_, *templateStorage_, *rawButtons_, event.y, [this](auto slot) { SnapshotNativeFort(...,slot); })`로 연결한다. 원문 signed event.y·준비 저장이 이름검사보다 먼저인 순서·기존1bh 직접 본문을 유지한다. 이 준비-prefix는 next19를 쓰지 않는다. Descend next19는 기존 caller 꼬리에 그대로 둔다.
5. `SelectCurrentFortName`의 snapshot/Notify와 편집 복귀/SetFortType Notify도 Native에서는 같은 API에 연결한다. hooks.state는 새 `GetNativeFortTemplateState(client_, templateStorage_, slot)`에 연결할 수 있다. 반환 바이트는 같은 TemplateStorage의 실제 State이고, caller가 해당 owner를 계속 보유해야 한다. 후보의 순수 Notify는 상태 생성/Get을 앞당기지 않는다.
6. StateWorldCleanup의 saveFort/saveBattle 등 남은 실제 Native caller도 같은 API로 연결할 수 있다. 이때 snapshot 도중 전체 Player/Pool 정리나 Template 반납을 실행하지 않도록 현재 caller 수명 관문을 유지한다. 후보 자체는 새 Session/Runtime pin/World를 생성하지 않는다.

## 원문과 대조한 핵심

- 기존 `o/RawFortSnapshotParent`·Basics·ContentSave·ObjectSave·ObjectsSave·RegionSave·TerritorySave·MoneySave·MissionSave·BadgesSave·CoreWrite·SpWrite·PlayerCoreRefresh를 재사용한다. Fort와 Battle의 자식 순서가 다르다.
- 같은 Template의 SerializationGlobalsOwner에서 version/scope/ownerSavedMask/battleSavedMask를 직접 빌린다. Objects가 version2를 쓰고 Terr/Chaff scope를 남기는 효과를 복구하지 않는다.
- 기술 부모는 원문0048fc60/현 RawPlayerWorld처럼 현재 Player+9c DWORD를 읽는다(옛 WIP의 +6c 오류를 교정했다). 실제 Content TYPE는 매 후보 읽고 next는 반환 전에 저장하는 독립 커서를 사용한다. 같은 Pool/Hash/TYPE/Player+68 DeckStorage·9Player·CoreData·Badges·공유 CRT/GameRandom을 사용한다.
- Deck mode0의00475530 flags1 관문→004752b0 가상+b4/실제 내용물·Allowed 조회→같은 Deck 활성화를 수행한다. fullDeck/mission·getTeleportTechFree·현재 Factory count/modulo/포인터·signed power·부분쓰기·압축/가중합 순서를 유지한다. 임시 RawProductionDeck을 생성하지 않는다.
- RegionAllowed는0046f200의 **현재 editor/missionPost**를 읽고 signed originalOwnerBYTE 또는 currentOwnerDWORD를 선택한다. 옛 Session finishState 캐시를 사용하지 않는다.
- 00432430은 높이0에서도 최초(0,0) selector/진단을 먼저 수행한다. HasIsland는0046f6e0/46f680의 ftol·Hash WORD·현재 terrain TYPE를 읽는다.
- CoreData의 missionPost/editor 자연 반환은 기존 부모가 판단한다. 선택되면 같은 실제 Core·기술·다이스 count→현재 seed·같은 CRT/GameRandom 재시드→원문 SP writer까지 연결한다. 새 난수·SP/Player 초기화는 없다.
- Notify는 기존 RawPlayerTemplateState.Changed의 isMine/preventSave/미션 진단과 실제 CampaignTemplateStorage.SaveFile·EditorFiles.Writer를 사용한다. 이미지 생성 뒤 현재 readOnlyForts/숨김9BYTE를 읽고 사용자 작업 파일에 쓴다. 파일 진단 식/행도 원문에서 연결했다.

## 마감되지 않은 범위

후보 구현/구문 검사가 제품 접합·링크·허용 실행 검사 통과를 뜻하지 않는다. 제품 CMake/입력/이름·State caller 연결, 사용자 홈/읽기 전용 설치의 실제 저장, 단순 Edit 왕복·미션 진입/대기 검증은 부모 작업이다. MP/Join/사용자 직접 플레이 제외를 유지한다. 일반 저장 진단 정상 복귀 뒤의 임의 외부 메모리 변조나 모든 원문32비트 heap/SEH 일치까지 검증한 것은 아니다.

처음 후보 구문 검사에서 옛 WIP의 `o/RawCarrierDropPoint.h` 경로가 현행에 없어 C1083을 확인했고 client 경로/namespace로 교정했다. 최초 tool 관측은 `snapshot-syntax-first-observation.txt`에, 최종 원시 로그는 위 파일에 보존했다. 게임 실행이나 통신으로 이 실패를 확인하지 않았다.

## 사용자 중단에 따른 최종 동결

사용자가 현재 단위 마감 뒤 목표 일시중지를 요청해 후보를 여기서 동결했다. 새 구현 확장/제품 접합은 하지 않는다. 최종 재개 인계는 `HANDOFF.md`, 함수별 C/PE inclusive 범위와 대응은 `native-function-ledger.json`(51함수), 진단 식/파일/행은 `diagnostic-map.json`(50행)을 따른다. 이 인계는 제품1265 완료나 목표 일시중지 상태를 직접 변경하지 않으며 부모가 처리한다.
