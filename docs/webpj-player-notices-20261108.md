# webpj Player 정리·알림 묶음 이식 — WB04-E8

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여덟 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawPlayerLinksCleanup`·`o/RawPlayerNotification`·`o/RawPlayerDamageNotice`(모두 10.78 PATCH 전용)를 한 번에 옮겼다. 셋 모두 100줄 안팎이고 독립 원본 관측이 있으며 서로 의존하지 않는다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

### `www/core/raw-player-links-cleanup.js` (0048fcd0)

`cleanupPlayerLinks({ players(), release(token) })`: Player 0..8마다 `+0x9c`(기술 링크)와 `+0x08`(개수)을 0으로 쓰고, `+0x68`(동적 Deck)이 비0이면 반납 장치를 부른 뒤 **현재 base를 다시 읽어** `+0x68`을 0으로 쓴다(0이면 후속 0 쓰기도 생략). 배열·Player 자체는 반납하지 않는다. 각 원문 load/store 직전에 `players()`를 다시 조회하므로 free가 base를 바꾸어도 반납 인자와 뒤의 0 쓰기 대상을 구별한다. 짧거나 NULL인 물리는 그 위치에서만 `RangeError`이며 앞선 store는 되돌리지 않는다.

### `www/core/raw-player-notification.js` (00490da0, 0042b210)

- **`notifyPendingPlayerResult`**: known(595344)·battle(594fbc) 전역이 모두 참이고 현재 owner 기록의 대기 타입(`+0xa0`)과 기술 링크(`+0x9c`)가 있으면 기술 getter(0048fc60)를 부른 뒤 대기 타입으로 분기한다. 2/3은 제단 승급(`+0x71`을 최대화하고 증가했을 때만 `AltarUpgrade` 안내 후 종료), 93은 Core 합계가 43일 때 획득하고 `+0x71=0`, 154는 `+0x71=0`만, 그 밖은 `contained(technology,4,pending,0)==0`이면 획득한다. 획득은 호출 순간의 현재 대기 타입/owner를 전달한 뒤 갱신(`refresh`)한다. 마지막에 `+0xa0`만 0으로 쓴다(이웃 기록·Core BYTE 보존).
- **`coreTechnologyWeight`**: Core 기술 마스크(`+6`)의 32비트를 낮은 번호부터 보고 켜진 비트마다 **현재 TYPE 개수를 다시 조회**하며 비트가 같은 첫 TYPE의 `level+1`을 더한다(32비트 감김). 마지막에 `Core+5` BYTE를 더해 BYTE로 돌려준다. 마스크와 `Core+5`는 TYPE 조회 뒤에도 현재 물리에서 다시 읽는다.

### `www/core/raw-player-damage-notice.js` (00416760)

`RawPlayerDamageNotice`: challenge 모드이거나 `owner<1`이면 반환, `owner≥9`이면 `RangeError`, `Player+0xb0`의 AI 토큰(opaque32)이 0이면 반환, 소비자(00415460)가 없으면 거부, 아니면 `ai(token, victim, attacker)`. Player 기록은 쓰지 않으며 모드와 AI 필드는 호출마다 다시 읽는다.

## 2. 검증 (`tests/player-notices.test.mjs`, 8건)

| 검사 | 근거 | cpppj 대응 |
| --- | --- | --- |
| 링크 정리 162행(정상 138·중단 18·NULL 6): 상태, 현재 base/owner, 두 Player 1620 BYTE, 반납 인자, 현재 base 재조회와 free 순서(재진입·base 교체·포인터/다음 기록 변경 포함) | `player-links-cleanup-x86.tsv` | PlayerLinksCleanup_ReplaysIndependentPe162 |
| 소유한 할당 반납·0 토큰 건너뜀·장치 누락/짧은 물리가 앞선 효과를 남김 | (독립 입력) | PlayerLinksCleanup 나머지 2건 |
| 대기 결과 118행: 전체 9기록, owner, 부분 효과, 호출 순서 | `player-notices-x86.tsv` | PlayerNotification_Native118PendingResultsAndPartialEffects |
| Core 합계 92행: 결과 BYTE, 현재 Core 마스크/rank 재조회, TYPE 개수 변경, getter 순서 | `core-technology-weight-x86.tsv` | PlayerNotification_Native92CoreTechnologyWeight |
| 선택하지 않은 장치는 요구하지 않음, 마스크 0은 TYPE을 읽지 않음 | (독립 입력) | PlayerNotification_SelectedDevicesAndEdition(CD 거부 제외) |
| 피해 통지 144행: 호출 여부, 세 인자, 기록 불변 | `player-damage-notice-x86.tsv` | CampaignNotice_PlayerDamageNativeWrapper |
| 현재 모드/AI 필드 재조회, AI 누락 거부, 범위 거부 | (독립 입력) | CampaignNotice_PlayerDamageCurrentStateAndMissingAi |

최초 실패와 교정: 구현 직후 8건이 처음부터 통과했다. 기대값은 원문 열만 썼다.

## 3. 경계

- 이 묶음은 CD 10.72를 다루지 않는다(원본 모듈이 10.78 전용이며 판본 거부 검사는 웹에 판본 인자가 없어 옮기지 않았다).
- 실제 세션의 전역(`global`)·기술 getter·내용물 조회·안내·획득·갱신·AI 피해 몸체는 호스트가 hooks로 공급한다(World/Session 조립은 WB05).

## 4. 다음 시작점

`RawPlayerWorld`(공통 PostPop 부모 `SquidPostPop`의 `PostPopBase`/`ValidateBase`가 선행), `RawPlayerTechnology`, `RawPlayerContentNotice`, `RawPlayerRegular`, `RawPlayerTemplateState`, `RawPlayerGeyserBounds`, `RawPlayerPlacementAnchor`, `RawPlayerColor.h`, 그리고 `SquidPostPop` 처리기 본체·종류별 PostPop·`SquidDisplay`·`SquidFrame`.
