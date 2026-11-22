# 메타·일반 Sprite 표시 갱신의 웹 범위 판정 (WB04-E1203)

재기준선 문서([E505](webpj-rebaseline-meta-20261115.md))가 "메타 화면 외에 일반 Sprite `0x50cc08` 갱신도 포함한다"며 `port`로 남긴 `ClientMetaSpriteRefresh`·`NativeMetaSpriteRefresh`(Form dirty 때 가상 표 `0x50c9f0` 메타 Sprite·`0x50cc08` 일반 Sprite 객체의 화면 사각형을 무효화하고 Gump 를 다시 그리는 갱신)를 단독 웹에서 닿는지 판정했다. **닿지 않는다. 이식 대상에서 뺀다.**

## 근거

1. **생성 경로가 없다.** 이 두 가상 표를 객체에 써 넣는 코드는 cpppj 와 웹 어디에도 없다(표를 읽는 곳은 `RunSpriteRegular` 분배·`SquidDisplay` 갱신·`SquidPop` 목록뿐). 자산 TYPE 116개의 이름에도 Sprite 로 만들어지는 TYPE 이 없다(`mcloud`·`challengeIsland`는 이름만 적재 순서에 있고 코드 사용이 없다).
2. **실제 미션에 객체가 없다.** 실제 Chromium 에서 영어 캠페인 미션 20개(Tutorial1·2·5, The War Begins ~ Final Confrontation 중 19개, bc1menu76, bc8fury76)를 시작해 객체 풀 전체를 훑어 첫 DWORD(가상 표)가 `0x50cc08`·`0x50c9f0`인 객체를 세었더니 모두 0개였다(한 미션 이름 오타로 하나는 시간 초과).
3. **소비자도 메타 전용이다.** `NativeMetaSpriteRefreshGlobals`는 챌린지(CS) 전역·`challenge` 모드를 읽고, 웹 `core/squid-display.js`의 Sprite 갱신(`sink.metaRefresh`)은 장치가 없으면 "미연결"로 거부하지만 위 이유로 호출되지 않는다.

## 처리

`tools/port-map.mjs`에 `mp-meta-sprite`(`exclude-mp`) 규칙을 더하고 두 단위를 이식 대상 todo 에서 제외로 옮겼다(`tests/port-map.test.mjs`의 예외 목록과 고정 분류 기대값 수정). 남은 양은 104단위·28,712줄 → 102단위·28,591줄이다. 이미 이식된 `RawSpriteRegular`·`SquidDisplay` 의 Sprite 분기는 소스 계약 시험으로 남기고 앱에는 연결하지 않는다.
