# 캠페인 건설·간헐천 연결과 사제 배치 자산 이식 (WB04-E562)

4-D 구획 C의 `client/RawCampaignConstructionBindings`를
`webpj/www/core/raw-campaign-construction-bindings.js`로, 그 시험 장면과 Session 조립이 쓰는 자료 묶음
`client/PriestPlacementAssets`를 `webpj/www/core/priest-placement-assets.js`로 옮겼다. 두 단위 모두
기준선 `225553df`와 HEAD의 cpppj 소스가 같다(기준선 뒤 변경 없음).

## 옮긴 범위

### RawCampaignConstructionBindings

- `RawCampaignConstructionState`: 세션 동안 유지하는 건설 전역 묶음(request·command·confirm·clear·
  notice·place·packet·geyser).
- `bindConstructionCore(core)`: 독립 World가 소유한 실제 부품 13개(pool·hash·spots·types·frames·
  bookkeeping·ownerMode·factory·owner·building·virtualDestroy·surfaces·bridgeState)만 고른다.
  `bindConstructionRuntime(world)`: 같은 World의 생성/Pop/가상 삭제 부모와 저장소를 빌린다.
- `makePlayerGeyserBoundsState(ownerMode, source)`: PlayerBounds가 매번 읽는 상태 뷰. battle/metamap은
  같은 World의 소유자 모드를 읽는다. C++의 "같은 주소인가" 확인은 `bounds.ownerMode === core.ownerMode`로
  옮겼다.
- 생성자: 실행 부모/저장소 정체성과 네 가상 경계 → 같은 풀/장부·프레임 표·Player 9기록·전용 최근
  목록(5칸 이상, 전체 목록과 다른 객체)·소유자 모드·필수 서비스 다섯 → `validateState` → Place →
  Clear → Notice → Confirm → Request → Command → PlayerBounds → LocalGeyser(보상 상태의 간헐천 자원
  pool을 같은 소유자에서 읽고 쓰는 상자 연결) 순서로 조립한다.
- Confirm의 `transmitAndHandle`: 방송(`broadcast.sendMutable`) → 진단 사본 512BYTE 보관과 본문 범위
  (27 + SID 수 × 2) → 같은 wire를 통지 처리기에 직접 전달.
- `validateState`: 받침 TYPE(배치 상태·World·간헐천·확정 패턴 3번), 세 패턴 표, 다리 TYPE, 로컬
  Player, 권한 넷, Fort 모드, 받침 TYPE의 미복원 flags1(0x8010)·다이스 genus(0x400000)·다이스 TYPE을
  현재 값으로 대조한다. 공개 입구(confirm·request·receive·refund·reject·chargeShortfall·confirmSupport·
  generateLocalGeysers·populateGeysers)가 먼저 부른다. `receiveCommandPacket`·`receiveNotice`는 원문처럼
  선행하지 않는다.
- `confirmSupport`: LocalGeyser 전용 — noIsland TYPE이고 Player·인자·방향·flags·시각·품질이 모두 0인
  요청만 허용한다.
- `register(dispatch, prepare)`: 0x3e 수신(`registerConstructionCommand`)과 0x3f 통지(packet observer의
  길이 확정과 `sidCount` 진단, 버전 1·정책 0·가변 길이, 직접 수신기).
- `destroyHooks(fallback)`/`destroy(fallback)`: (TYPE, 가상 표)가 함께 맞는 받침 157(`0x506f08`)·섬
  받침 94(`0x506d98`)·종유석 95(`0x506e50`)·그루터기 96(`0x501dd0`)만 표면 수명의 Pre/Post와 같은
  World의 가상 삭제기로 보내고 나머지는 fallback에 둔다.
- 선택되지 않은 일반 건설 경계(전체 MayPlace·밀어내기·실패 섬 정리·기준점·기록기·운반/사제 경로·
  비용 안내·SP/취소 통지)는 `캠페인 일반 건설 미연결: 이름` 오류로 그 호출에서 거부한다.

### PriestPlacementAssets

- `frames[type]` = `{ frames, defaultFrame }`(자산 70 이상은 `.type` 정의의 프레임 순서와 특수 기본
  프레임, 내장 타입은 빈 표), `hotspots[type]` = `{ x, y }`, `shapes`(`squidRendererShapes`의 SHP 물리
  프레임 표), `geometry.patternTypes`(로딩 파일 이름으로 해석한 여덟 패턴 TYPE).

## 옮기지 않은 것

- cpppj의 Native 장기 경로: 저장소에서 직접 빌리는 `BindConstructionCore(storage)`/
  `BindConstructionRuntime(storage)`, 현재 Player base 조회기(`runtime.players`), Player span/방송 객체
  없는 세 번째 생성자, `PlayerBounds.BoundsRecord`. 웹 `RawSurfaceStorage`에 대응 개념이 없어 항상
  독립(typed) 경로이며 Client 장기 소유자가 생기는 4-D 구획 E에서 추가한다.
- 실제 `ClientConstructionGlobals`는 별도 todo 단위다.

## 시험

`webpj/tests/raw-campaign-construction-bindings.test.mjs` 10건. cpppj `CampaignConstructionTests.cpp`의
`ProtectedConstructionScene`(보호 TYPE 188·프레임·SHP, 실제 공간/삭제/간헐천 후처리, SP 저장소, 삭제
패킷 writer, 방송)을 그대로 조립했다.

- cpppj 시험 2건 이식(첫 실행 일치)
  - `CampaignConstruction_ProtectedNoIslandPacketAndSelfNotice`: 서버 SID 9개, 0x3f 통지 45BYTE, 각
    SID의 표면/spot 등록, 예측 조각 삭제, 깊이 0과 전체 다시 그리기, 일반 TYPE 받침 확정 거부.
  - `CampaignConstruction_ProtectedLocalGeysersUseCurrentWorld`: 비활성 Player 무생성, Player 1의
    간헐천 2개(최근 목록·전체 목록·비용 WORD 2000), 자원 pool 18000 → 14000, 공유 전역 불일치 거부.
- 웹 계약 8건: PriestPlacementAssets 전 항목 대조, 구성 거부, Core 선택과 범위 상태 뷰의 현재 값 조회,
  `validateState` 항목별 불일치(19가지 × 입구 9곳), 미연결 일반 건설 경계, 외부 효과·gameplay 연결
  (가상 owner·기록기·환불/취소 송신·로컬 예측 훅), 0x3e/0x3f 등록과 통지 직접 수신, 받침류 삭제 분배.

변형 확인: 건설 연결 154종은 시험 보강 뒤 모두 잡혔다(1차 생존 29종은 받침류 삭제 분배의 양성 경로와
구성 거부 조합이 비어 있던 것). PriestPlacementAssets 13종 가운데 1종(패턴 이름 없음 판정의 −1 → −2)은
고정된 여덟 이름이 모두 있어 닿지 않는 방어 분기라 동등 변형이다.

## 남은 경계

- 앱 수명에는 연결하지 않았다(PORT_MAP `port`). 실제 MayPlace 파이프라인·사제/운반 경로·기록기·비용
  안내/SP 통지 송신은 Session 조립(4-D 구획 D)에서 연결한다.
- 다음 시작점: 4-D C 구획의 나머지 `RawCampaign*Bindings` 가운데 Tutorial1 Load/Start가 요구하는 것과
  D 구획(Session 본체).
