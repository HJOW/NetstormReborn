# webpj Player 내용물 통지(00490750→004d55e0) 이식 — WB04-E9

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아홉 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawPlayerContentNotice`(10.78 PATCH 전용)를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-player-content-notice.js` — `RawPlayerContentNotice(pool, state, hooks).notify(parent, form, reason)`.

1. **진단 접두**: 첫 비교(`cursorA != cursorB`)에서 `problemCode ← fallbackCode`. `moneyChecks`가 비0이면 PlayerKnown(`battleShown`)일 때 현재 SP(`money(localPlayer)`)를 조회하고 `fallbackCode`를 **다시 읽어** 7000 초과면 `problemCode`에 쓰며, `moneyChecks`를 하나 줄인다(32비트 감김). 첫 SP 분산배열 주소가 0이거나 첫 순열이 -1이면 `problemCode ← fallback`.
2. **종속 체인 순회**: 부모 SID(`+6` WORD)에서 시작한다. 각 Form의 `next`(`+4` WORD)는 등록 조회·가상 처리보다 먼저 읽어 보존하고, Form 타입(`+10`)이 현재 `gumpFormType`과 같으면 등록 ID(`+18` DWORD)로 Gump 토큰을 조회해 비0일 때 `handle(token, contentEvent, form, reason)`을 부른다. 순환이거나 풀 범위 밖인 SID는 거부한다. 부모/자식 상태를 임의로 걸러내지 않는다.

전역은 호출한 원본 명령 위치에서 한 번 현재 값을 읽는 getter로만 받고(초기값·프레임 사본을 재사용하지 않음) `problemCode`·`moneyChecks`만 쓴다. 풀 판본·전역 조회기·소비자 누락은 통지를 성공으로 대신하지 않고 생성 시 거부한다.

## 2. 검증 (`tests/player-content-notice.test.mjs`, 2건)

| 검사 | 근거 | cpppj 대응 |
| --- | --- | --- |
| 88행: 진단 접두 뒤 11개 전역의 최종 값, SP 조회·가상 handle 호출 순서와 인자, 부모 포함 네 슬롯 전체 50 BYTE(200 BYTE) — 변경 시나리오 3종(SP 조회 중 전역 변경, handle 중 next/등록/이벤트 변경, 타입 변경) 포함 | `player-content-notice-x86.tsv`(판본 열 없는 10.78 전용 표) | CampaignLifetime_PlayerNoticeIndependentPeFixture |
| 누락 거부·순환 체인 거부·현재 전역 재조회 | (독립 입력) | — (JS 보강) |

최초 실패와 교정: 구현 직후 88행과 계약 검사가 처음부터 통과했다. 기대값은 원문 열만 썼다.

## 3. 경계

- cpppj `RawCampaignLifetimeBindings`(Player 통지·Content 수명·삭제 장부가 한 `RawSurfaceWorld`에 연결되는 통합 검사)는 해당 World 조립 단위(WB05)에서 대조한다.
- Gump 등록표(`00463ff0`)와 가상 `+28` 처리는 호스트가 `hooks.gump`/`hooks.handle`로 공급한다. 웹 쪽 Gump 호스트와의 연결은 UI 조립 단위에서 한다.

## 4. 다음 시작점

`RawPlayerWorld`(공통 PostPop 부모가 선행)·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState`·`RawPlayerGeyserBounds`·`RawPlayerPlacementAnchor`·`RawPlayerColor.h`, 그리고 `SquidPostPop` 처리기 본체·종류별 PostPop·`SquidDisplay`·`SquidFrame`.
