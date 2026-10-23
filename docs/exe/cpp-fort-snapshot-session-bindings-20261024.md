# 전체 Snapshot의 실제 Session 바인딩

2026-10-24 읽기 전용 설계다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 기존 제품·Client·Session·CMake·LEFT·공유 빌드·GUI·git은 변경하지 않았다. 아래 위치는 조사 시점이며 심볼명을 함께 기준으로 삼는다. 제안 API는 미구현이다.

## 같은 현재 저장소에 연결할 지점

| 경계 | 기존 정확한 위치 | 새 부모의 최소 연결 |
|---|---|---|
| 전체 부모 | [RawFortSnapshotParent.h](../../cpppj/src/o/RawFortSnapshotParent.h)의 네 진입 | Session에 Fort/Battle/현재칸/현재칸존재시의 명시 진입을 추가하고 같은 `FortSnapshotParentHooks` 바인더를 동기 호출한다. |
| 현재 선택/모드 | [Session.cpp](../../cpppj/src/client/RawCampaignSession.cpp):2929~2933의 `LocalOwner/TemplateOwner/CurrentTechnologyParent`, Impl의 `playerState/core.ownerMode` | globals.templateOwner는 `playerState.templateOwner`, fort/battle은 `core.ownerMode.fort/battle`의 현재 getter다. Player owner와 Template owner를 혼동하지 않는다. |
| 현재 manager | Session.cpp:3191~3194·3234~3236의 `headers` 읽기 | `Headers()[slot*20]`의 현재 DWORD를 **원문이 읽는 매번** 조회한다. 전체 호출 시작 때 저장하거나 `Template(slot)` 접근을 진단 앞으로 옮기지 않는다. |
| stream 참조 | [Storage.h](../../cpppj/src/client/CampaignTemplateStorage.h):20~24·43~50의 `TemplateWriteStream/CreateSection/WriteSection/AccessSection` | 한 동기 Snapshot 호출의 지역 `vector<TemplateWriteStream>`에 실제 manager/section/generation을 보관하고 부모 토큰을1기반 인덱스로 연결한다. 파일 section 번호나 현재 선택으로 바꾸지 않는다. |
| TypeNames/Deck | Session.cpp:3188~3208 `SnapshotFortBasicSection/TypeNames/Deck` | 같은 기존 명시 메서드를 child로 재사용한다. 기존 State/TYPE/Production 자료를 그대로 쓴다. |
| 기술 getter | Session.cpp:2933 `CurrentTechnologyParent()` | 새 부모의 `technologyParent`에 현재 getter만 연결한다. 새 부모가 이 값을 Create보다 먼저 캡처한다. |
| 기술 본문 | Session.cpp:1851~1867 `Impl::ContentWriter(ref)`,3210~3214 `SnapshotFortTechnology` | `technology(parent,token)`에서 ref를 꺼내 **`SaveFortContents(parent,1,s.ContentWriter(ref))`**만 호출한다. 새 private helper `WriteFortTechnology(parent,ref)`면 충분하다. |
| Objects/Territory | Session.cpp:3216~3251 `SnapshotFortTerritory/Objects` | 기존 실제 현재 지도·공유버전·Finder/Factory writer를 그대로 호출한다. 지역 ref 표와 부모 ref 표를 섞지 않는다. |
| Money 본문 | [RawFortMoneySave.h](../../cpppj/src/o/RawFortMoneySave.h), Session.cpp:310 `Impl::random` | 부모가 받은 token 그대로 **`SaveFortMoney(token,s.random,write)`**를 호출한다. write는 같은 ref의 `Storage.WriteSection`이다. `Sp.Get`, 별도 RNG가 아니다. |
| Mission 본문 | [RawFortMissionSave.h](../../cpppj/src/o/RawFortMissionSave.h)의 `SaveFortMission(stream,hooks)` | 부모 token 그대로 넘기고 서비스로 받은 실제 current/name/field/assertion에 **이 호출의 지역 write만** 결합한다. 새 Mission 섹션은 만들지 않는다. |
| Badges | [RawFortBadgesSave.h](../../cpppj/src/o/RawFortBadgesSave.h)의 `SaveFortBadges/ReadFortBadgeCount` | 같은 manager/create/access 어댑터와 Client 장부6160BYTE를 연결한다. 최초 count 할당은 함수가 끝날 때까지 유지한다. 장부 생성·초기화 모듈은 별도 작업자가 복원 중이다. |
| 마지막 Notify | Session.cpp:418·791~795의 `templateState`와 [RawPlayerTemplateState.cpp](../../cpppj/src/o/RawPlayerTemplateState.cpp):78~89 | parent.notify는 같은 `templateState->Changed(slot)`다. 현재 `BindFileSave` 미설치이며 이를 정상 성공으로 삼을 수 없다. |

**기술 stream의 내부 토큰은 항상1이다.** `ContentWriter(ref).access`가 token!=1을 거부한다. 부모 ref 표의 토큰을 그대로 `SaveFortContents`에 넘기면 두 번째 기술 섹션부터 실패할 수 있다. 실제 ref를 꺼낸 다음 위와 같이 내부1로 다시 감싼다. 반면 Money/Mission의 write는 부모 토큰을 부모 표에서 해석한다.

**Create 중복을 피한다.** `SnapshotFortTechnology`는 현재 기술 getter와 Create까지 수행하므로 새 부모의 technology child로 재사용하면 안 된다. `RawFortMoneySave::SnapshotFortMoney`도 별도 Create 부모이므로 전체 부모에서는 `SaveFortMoney`만 쓴다. Mission은 새 부모가 만든 같은 stream에 `SaveFortMission` 본문만 연결한다. TypeNames/Deck/Objects/Territory/Badges는 각 child가 자기 섹션 생성을 소유한다.

## 실제 Client에서 주입할 최소 서비스

`RawCampaignSessionServices`([Session.h](../../cpppj/src/client/RawCampaignSession.h):120~140)에 Mission의 **읽기/진단 hooks**, 장기 Badges span 소유권, 현재 파일 저장 서비스를 추가한다. 지역 token 표를 캡처한 write는 이 호출의 Mission hooks 복사본에만 붙이고 services에 저장하지 않는다. Snapshot 종료 뒤 dangling callback을 남기지 않는다.

Mission current는 [UberGump.cpp](../../cpppj/src/client/UberGump.cpp):153~165 `MissionRuntime`의 존재를 **guided 조건 없이** 조회한다. 기존258행의 Guide getter는 Tutorial 전용이므로 저장 판단에 재사용하지 않는다. 필드는 같은 `Runtime::state.sequence/flags`의+58/+54다. name은 `Runtime::init.name`의 NUL 포함 원문 span이며 Config.fileName/FortName으로 대체하지 않는다. name getter에서 만든 임시 문자열의 span을 반환하지 않는다.

현재 Mission의 불투명 비영 토큰을 Client가 소유하고 field(token,offset)에서 같은 수명을 검증한다. 호스트 포인터를 DWORD로 자르지 않는다. `SaveFortMission`은 이름 쓰기 뒤 current를 다시 읽으므로 초기 토큰을 고정한 adapter를 만들지 않는다. 같은 Mission의 sequence 쓰기 뒤 flags를 다시 읽는 것도 본문에 맡긴다.

Badges는 Client가 소유한 같은6160BYTE를 기존 `RawFortMetaLoad`와 `ReadFortBadgeCount`가 함께 빌린다. 원시 `Storage.Badges`는 파일 cursor이며 live 장부가 아니다. 실제 장부 초기화/갱신이 설치되기 전에는 Badges hook을 임의0 반환으로 채워 전체 저장 완료라고 하지 않는다.

## Notify·파일 writer와 수명

현재 파일 경로는 `templateStorage->Name(slot)`이며 캠페인 설치1240~1241행은 아직 mission 이름을 그대로 전달한다. 실제 사용자 작업 namespace/경로 준비 후 같은 manager의 이름을 바인딩해야 한다. 작업 파일을 준비하지 않은 상태에서 `EditorFiles.Writer()`만 추가하면 사용자 저장 경로가 완성되지 않는다.

`templateState->BindFileSave`는 같은 `Storage.SaveFile(slot,{readOnlyForts,playerState.battleOwner,hiddenPlayers},imageHooks,writer)`를 호출한다. hiddenPlayers는 Session.cpp:321의 실제 `battleOptions+0x3e`9BYTE다. **readOnlyForts0054db30의 Client 소유자는 현재 미구현**이므로 장기 DWORD 참조를 새로 주입해야 한다. 임시 getter 반환을 `DataManagerFileSaveState`의 const참조에 저장하지 않는다. imageHooks는 현재 TYPE 수/실제 진단을 연결하고 writer는 [EditorFiles](../../cpppj/src/client/EditorFiles.h)가 실제 권한/쓰기를 수행한다.

완료된 저수준 구성 예는 [EditorFilesTests.cpp](../../cpppj/tests/EditorFilesTests.cpp):24~38의 `Changed→BindFileSave→Storage.SaveFile→새 파일 재읽기`다. 저장 억제/hidden 검사는 이미지 생성 뒤이므로 이 callback을 앞에서 생략하지 않는다. `Changed`의 빈 이름 선행 throw와 inMission의 실제 전역 공유는 별도 교정 대상이다. 현재는 objectState.mission을 빌리며 generic Editor 모드 설치가 끝났다는 근거가 아니다.

`UberGump::ClearWorld`의287~313행은 **MissionRuntime을 먼저 해제하고 이후 Kernel에서 Session을 제거**한다. Mission 서비스는 Runtime 포인터 고정 캡처 대신 UberGump의 현재 getter를 캡처하고, 소멸 중 Snapshot/Notify 자동 호출을 추가하지 않는다. Client 장부/TemplateStorage/EditorFiles/readOnly DWORD는 이를 쓰는 Session보다 오래 살아야 한다. 지역 stream 표와 ContentWriter는 동기 호출 안에서만 유지한다.

Battle의 `coreData`는 현재 Session에 대응 writer가 없다. Fort에서 미선택이면 요구하지 않되 Battle이 도달하면 명시 실패해야 한다. 부모/Money/Mission/Badges 각각의 도달한 누락 장치를 일괄 사전 검사·noop으로 바꾸지 않는다. **부분 저장을 끝내고 마지막 Notify가 실패할 수 있다는 효과도 보존**한다. 실제 결합 검사는 Fort 순서/현재 슬롯 교체/빈 Mission/같은 live Badges/공유 난수 전진/작업 파일 재로드와 미연결 Battle CoreData의 실패를 별도로 확인한다.
