# 지식(NSInfo) HUD 패널의 웹 범위 판정 (WB04-E1171)

Uber 루트의 `createKnowledge`가 만드는 NSInfo(원문 0047ca50 계열, `ui/hud-ns-info*.js`)를 단독 웹 미션에서 연결해야 하는지 소스와 원본 화면으로 판정했다. **연결하지 않는다(웹 범위 밖).**

## 근거

1. **정체:** NSInfo는 "준비 창"이다. 플레이어 8명 정보(PlayerInfo)·명단·제목·구분선·전투 정보(BattleInfo)를 가진 멀티플레이/메타맵 로비 패널이다(`ui/hud-ns-info-create.js` 머리말, cpppj `HudNsInfoCreate.cpp`).
2. **단독 미션 부착 경로:** cpppj `HudNsInfo::Attach`(`HudNsInfo.cpp:21~28`)는 기본 부착과 배경 크기 설정 뒤 `if (!metamap() || !playerKnown()) return;`으로 끝난다. 메타맵 배치(`metamapLayout`)는 메타맵 전용 분기에만 있고, 단독 미션(`metamap = 0`)에서는 화면 배치가 없다. 멀티플레이 UI·재접속/메타는 웹 범위에서 제외했다(인계 1절).
3. **원본 화면:** 원본 단독 미션 스크린샷(`screenShots/Bridge the Gap - Started.png`)의 왼쪽 HUD에는 SP 표시·생산 덱·미니맵뿐이고 준비 창 패널이 없다. 웹의 같은 화면도 좌측 하단 85×82 영역이 원본과 픽셀 단위로 일치한다(`e2e/uber-hud-layout.spec.js`).
4. **영향:** `createKnowledge`를 연결하지 않으면 Uber 루트의 `knowledge` 자식 토큰이 0일 뿐 화면·입력·배치에 차이가 없다(HUD 배치 e2e가 `knowledge: 0`으로 고정). 연결하면 보이지 않는 자식만 늘어난다.

## 결론

- 인계의 "지식(NSInfo) 패널 공장" 항목을 웹 범위 밖으로 닫는다. `HudNsInfoMeta`·`NativePlayerInfoInput`·`NativeNsInfoInput`·`HudPlayerButtonDraw`의 "단독 경로 확인"도 같은 이유로 대상이 아니다(메타 전용). 이미 이식된 `ui/hud-ns-info*.js`는 소스 계약 시험으로 남기고 앱에는 연결하지 않는다.
- 멀티플레이 UI를 구현하게 되면 그때 `createKnowledge`와 메타 Player 메뉴(`View Player Info`, [E1167](webpj-player-menu-reachability-20261122.md))를 함께 연결한다.
- 기준선 뒤 cpppj 변경 없음(`HudNsInfo.cpp`·`UberHud.inc`).

## 대응표 정리(E1176)

- 위 판정에 따라 `tools/port-map.mjs`에 `mp-nsinfo`(exclude-mp) 규칙을 재기준선 규칙 앞에 더해 `HudNsInfoMeta`·`HudPlayerButtonDraw`·`NativeNsInfoInput`·`NativePlayerInfoInput` 네 단위(407줄)를 이식 대상 todo에서 제외로 옮겼다. 재기준선 문서([E498~E505](webpj-rebaseline-meta-20261115.md))가 "5-B·6에서 단독 경로를 확인"으로 남겼던 항목이며, 공유 슬롯 계산 `RawBattleOptionSlots`는 이식 대상으로 유지한다. `tests/port-map.test.mjs`의 후보 신규 단위 분류 시험은 이 네 키의 예외를 명시한다. 남은 양은 137단위·31,863줄 → 133단위·31,456줄이다.
