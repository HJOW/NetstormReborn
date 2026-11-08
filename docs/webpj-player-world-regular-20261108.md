# webpj Player 월드 연결·Regular 처리기·Template State(RawPlayerWorld·RawPlayerRegular·RawPlayerTemplateState) 이식 — WB04-E55

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 쉰다섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawPlayerWorld`(원본 00490250 Player 생성, 004905d0 파생 postPop, 0048fc60/0048fc90 Technology getter), `o/RawPlayerRegular`(00490810 Player 가상 +5c 소어 루저 Regular 처리기), `o/RawPlayerTemplateState`(004bd550 State getter, 004bd540/004bd360 Notify)를 옮겼다. 세 파일은 Regular 처리기가 앞의 두 몸체를 그대로 쓰는 한 묶음이라 함께 옮겼다. 웹은 10.78 전용이라 CD 10.72 분기(BYTE frame +34·0xac 기록·사전 Validate·즉시 로딩 깊이 증감)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-player-world.js` — `RawPlayerWorld(pool, base, records, state, hooks)`, `PlayerWorldState`

- `create(owner)`: PlayerKnown 0 이면 진단 → 진단 뒤 현재 `special` 로 y(10/1), signed32 감김 `owner+10` 의 float 로 x → 같은 칸의 현재 `playerType` 검색(있으면 진단) → 진단 뒤 다시 읽은 `playerType` 으로 Factory Create(flags 2) → 가상 Owner → 물리 슬롯 +36 DWORD 0 → 가상 Pop(flags 0). 하위 장치는 도달한 위치에서만 요구한다.
- `postPop(sid, flags)`: 사전 검사 없이 물리 슬롯 owner BYTE 의 9기록 Technology DWORD(+0x9c)에 SID 를 먼저 쓴다. 최초 비트·owner BYTE == 현재 `battleOwner` DWORD(257 ≠ BYTE 1)·fort 아님이면 예약 성공 시 event 1/payload 5.0 Regular. 예약 효과 뒤 다시 읽은 `special` 이 0 이 아니면 물리 +36 DWORD 1. 마지막에 `SquidPostPop.postPopBase`. 하위 실패 전의 쓰기는 되돌리지 않는다.
- `handles`(물리 슬롯 VT 0x50ee08)·`validate`(독립 명시 사전 검사: 할당 슬롯·owner 범위·공통 `validateBase`)·`currentTechnology`(PlayerKnown 0 이면 기록/owner 를 읽지 않고 0)·`technology(owner)`(진단 뒤 현재 기록). 9기록 밖 owner 는 진단으로 꾸미지 않고 물리 저장 전에 `RangeError`.
- `SquidPostPop.setBody('player', body)` 로 공통 후처리 분배에 연결한다(분배 순서는 E41 의 기존 표).

`www/core/raw-player-regular.js` — `RawPlayerRegular(player, mode, hooks)`

- `handle(sid, event, count, payload)`: owner BYTE ≠ 현재 `battleOwner` 또는 count 0 이면 원래 payload. event 1: 로그 29 `SORE LOSER TEST: ` → 로그 효과 뒤 현재 battle 이면 `STARTING.\n`·예약 성공 시 event 2/payload 60.0 Regular·0, 아니면 `NOT STARTING YET.\n`·payload. event 2: battle 이면 현재 `templateOwner` State 첫 BYTE |= 4 → getter 뒤 다시 읽은 `templateOwner` 로 Notify·0. 그 밖: 진단(0x41d) 뒤 payload. 여섯 하위 효과는 생성 때 모두 요구한다.

`www/core/raw-player-template-state.js` — `RawPlayerTemplateState(headers, states, names, mission, assertion, {allocate, allocationComplete})`, `initializePlayerTemplateStateBytes`

- `get(slot)`: manager 0 진단 뒤에도 0 이면 물리 경계 오류 → 현재 스트림 position 0 → 빈 State 는 미정 스택 28 BYTE 입력·원본이 쓰는 필드만 초기화(`(b0 & 0xf9) | 1`, 4~9·12~17·22~25 = 0)·`replace` 가 돌려준 새 스트림 → signed end < 28 이면 `bindMinimumSize` 부모의 새 스트림, 없으면 분리 할당(원문 복사·0 채움·position/end 28·이전 버퍼 반납) → 현재 bytes.
- `changed(slot)`: manager 0 은 파일명 첫 BYTE 물리 경계 → `.` 이름 진단 → isMine 이면 transient 진단 뒤 isMine 재조회 자연 반환 → 현재 inMission 진단 → manager 0 진단 → preventSave 자연 반환 → `bindFileSave` 의 실제 저장 부모(0044d8d0).
- 스트림/이름은 9칸 배열 또는 `(slot) => 값` getter(빌린 manager 의 현재 조회), inMission 은 `{ value }` 소유자다. 미연결 경계 문구는 `PLAYER_TEMPLATE_STATE_BOUNDARY` 로 내보낸다.

## 2. 검증

- `tests/player-world.test.mjs`(15건): `player-world-x86.tsv` 10.78 840행(Create 192·PostPop 576·getter 72). 실제 자료(`www/data` 의 netstorm.tarc TYPE 정의 → 188칸 레코드·zOrder·생성자 주소, `_shapes.shp` 셀 크기)로 실제 `SquidFactory`(Player152 생성자)·`SquidPop`·`SquidOwner`·`RawBridgeConnect.findTypeAt`·`SquidProcessHost`/`Kernel` 을 연결하고, 실행 뒤 전체 9기록·슬롯·네 단계 해시·spot·비용/알림/깊이·Totalmade 세 표·owner 공장 수·Regular 필드·Form 슬롯·PID·타입 순환·진단 수 20열을 비교한다. 계약 14건: cpppj `PlayerParentCurrent_*` 13건(물리 VT·free 슬롯 기록, 현재 getter, 예약 누락/실패/생성 누락, special 재조회, DWORD owner 비교, fort/비최초, 늦은 공통 실패, owner 범위, 생성 순서, Pop 누락, 진단 뒤 명시 기록)과 9기록 크기/같은 풀(웹 전용 보강).
- `tests/player-regular.test.mjs`(5건): `player-regular-x86.tsv` 10.78 329행(반환 317·경계 12: 빈 State 생성 0044d1b0 1·짧은 State 할당 004e4391 2·파일 저장 0044d8d0 9), `player-state-expand-x86.tsv` 99행(분리 할당 확장·다음 get 의 같은 버퍼), `player-state-initialization-x86.tsv` 2560행(생성 SCE 1536·자연 반환 SE 1024), 미연결 생성 거부 계약, 초기화 필드·최소 확장 부모·저장 관문(웹 전용 보강).
- 공용 장치 `tests/player-scene-support.mjs`: cpppj `DudeSpawnSupport::Assets(Patch1078)` 에 대응하는 실제 TYPE/SHP 자료와 WordAdler·double 비트 변환.

최초 실패와 교정: 세 모듈 모두 원본 3828행 전부를 첫 실행에 통과했다. player-world 재생이 8.2초 걸려 해시 체크섬을 배열 펼침 없이 계산하도록 검사 보조만 고쳤다(2.7초).

## 3. 기준선 뒤 변경(3.6절)

- `o/RawPlayerTemplateState` `1f00a25a` → `f261f530`(20261107-91 비Session Fort 등록 ID): Notify 본문을 `NotifyPlayerTemplateChanged(slot, hooks)` 자유 함수로 분리해 World 없는 Fort 구독자(`FortSubscriberClient`)와 공유했다. 관문 순서·진단·저장 선택이 같고 미션 조회를 함수로 받는 것뿐이라 단독 결과는 같다 → **①**. 대응 행 base 를 `f261f530` 으로 기록했다.
- `o/RawPlayerWorld`·`o/RawPlayerRegular` 는 기준선 뒤 변경이 없다.

## 4. 경계

- cpppj `PlayerWorld_SameSurfaceCoreRecordsAndInitialRegular`·`PlayerRegular_SameWorldInitialFiveAndSixtySecondChain`(실제 `RawSurfaceWorld` 의 첫 예약→5초→60초 사슬)과 `CampaignTemplates_*`(`CampaignTemplateStorage` 의 빈 State 재생성·Territory 압축)는 WB05 월드 조립·캠페인 Template 저장소 이식 때 옮긴다.
- 실제 로그(그룹 29)·Template 파일 저장 부모(0044d8d0)·State 섹션 재생성/최소 확장 부모(44d290)·Regular 처리기 등록(`setRegularHandler`)은 월드 조립에서 공급한다. 웹은 미연결이면 해당 위치에서 명시 오류다.
- PATCH 의 CD 전용 `loadingDepth`/`depthChanged` 인자는 두지 않았다(10.72 즉시 증감 전용).

## 5. 다음 시작점

Player 나머지 `RawPlayerTechnology`(00490330/004903b0/00490420/004914e0/004915c0, fixture `player-technology`)·`RawPlayerGeyserBounds`(0048f5c0, `player-geyser-bounds`)·`RawPlayerPlacementAnchor`(0048fdb0, `playeranchor`)와 종류별 PostPop 본체(Asset·Fence·Nugget·Buried·Carrier·Factory·Priest·Rune)·`SquidDisplay`/`SquidFrame` 이다.
