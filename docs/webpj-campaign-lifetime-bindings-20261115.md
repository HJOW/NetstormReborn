# 캠페인 내용물 수명 연결 이식 (WB04-E521)

4-D 구획 C의 `client/RawCampaignLifetimeBindings`를 `webpj/www/core/raw-campaign-lifetime-bindings.js`로
옮겼다. 기준선 `225553df`와의 차이(+9줄)는 Native 장기 Storage 경로의 소유자 확인이며 웹 결과에는
영향이 없다(3.6절 ①).

## 옮긴 범위

- `RawCampaignLifetimeBindings(world, forms, state, effects, reward, player, fallbackParent)`: 같은
  PATCH 풀이 아니면(다른 풀의 Player 통지 포함) 어떤 등록도 하기 전에 거부하고, 같은 풀이면
  `ContentFormLifetime`(TYPE 6의 실제 pre/Unpop/부모 알림/post/SID 반납)을 월드의 삭제기와 Form 분배
  훅으로 한 번 만들어 Form 분배기에 등록한다(두 번째 등록은 중복으로 거부).
- `destroyHooks(fallback)`: 입증된 (TYPE, 가상 표) 쌍 — 84(빈 간헐천)+`0x501dd0`, 103+`0x50c3b0`,
  104+`0x501dd0`, 105+`0x509810`, 106+`0x5096a0`, 134+`0x5070c0` — 의 Pre/Post 만 월드의 공통 삭제 몸체
  (`core.surfaces.hooks().emit`)로 보내고 그 밖의 효과·타입은 기존 분배(fallback)에 남긴다.
- `destroy(fallback)`: 같은 쌍은 공통 삭제기(`core.virtualDestroy.destroy`), 다른 파생은 fallback이며
  fallback이 없으면 명시 오류.
- `parentNotice`: Player 가상 표(`0x50ee08`)만 Player 통지로 보내고 다른 부모는 주입된 경계 또는
  명시 오류.
- 모듈 함수 `bindPlayerContentNoticeGumps`(같은 Gump 등록 배열 + 현재 handle)와
  `makeCampaignDeletionEffects`(선택 해제·유닛 손실 소리 변환, Refund는 실제 reward 몫이라 거부).

## 옮기지 않은 것

- cpppj의 Native 장기 Storage 경로(`UsesNativeTransmissionDevices`: Client 수명이 이미 같은 Content
  본문을 등록한 경우). 웹 `RawSurfaceStorage`에는 대응 개념이 없어 웹은 항상 독립(typed) 경로이며,
  Client 장기 소유자(`ClientMain`)가 생기는 4-D 구획 E에서 같은 자리에 추가한다.

## 시험

`webpj/tests/raw-campaign-lifetime-bindings.test.mjs` 6건(실제 TYPE 188칸 월드·`ContentFormHost`·
`ContentLifetimeState`·`SquidReward`): 풀 거부와 단일 등록, parentNotice 분배(Player/주입 경계/없음),
Pre/Post 쌍 라우팅(여섯 쌍·다른 가상 표·비Pre/Post 효과·fallback 없음), 가상 삭제 라우팅, Gump 연결,
삭제 효과 변환. 본체(Player 통지 88입력 원본 관측, Content 수명, 삭제기)는 각 단위 시험이 소유한다.
공통 삭제 몸체와 삭제기는 호출 경로만 관찰한다(실제 표면 수명은 E248 시험).

## 남은 경계

- 앱 수명에는 연결하지 않았다(PORT_MAP `port`). 실제 `World.ClearProcesses` 선행 해제는 Session 소유.
- 다음 시작점: 4-D C 구획의 `RawCampaignAssetBindings`·`RawCampaignProductionBindings`·
  `RawCampaignConstructionBindings`(실제 `ClientConstructionGlobals` 선행 판단 필요).
