# webpj Player 전체 초기화(004910a0) 이식 — WB04-E7

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일곱 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawPlayerInitialization`(10.78 PATCH 전용)을 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-player-initialization.js` — `initializePlayerRecords(globals, hooks)`가 원문 순서로 다음을 실행한다.

1. **현재 Player base를 먼저 캡처**한 뒤 두 카운터(`005951f8` 로그인 Fort 수, `005951fc` 마지막 로그인 Player)를 0으로 쓴다.
2. 이전 base가 비어 있지 않으면 Player 0..8의 +0 공장 토큰을 **행마다 현재 base를 다시 읽어** 아홉 번 해제한다(NULL 토큰도 생략하지 않음). 이어 현재 base로 Player 영역을 반납하고, base 슬롯에 NULL을 다시 쓴다(해제 callback이 base를 바꿔도 이 위치에서 0이 된다).
3. 1620 BYTE를 할당하고(반환 주소를 슬롯과 REP 대상에 각각 사용) 405 DWORD를 0으로 쓴다.
4. owner 0..8마다: `+0x64=0`, `+0x58=0`, 120 BYTE 공장 할당(내용은 초기화하지 않으며 NULL 반환도 그대로 기록), `+0`=공장 토큰·`+4=30`·`+8=0`·`+0x2c=0`·`+0x78=127`, 자기 동맹 mask(`1<<owner`)를 `+0x54`에 쓰고 **현재 base에서 다시 읽어** 방향별 동맹 몸체(00490ea0)에 전달한다. 행 주소는 할당 전에 캡처하지만 동맹 인자는 현재 base 기준이다.

Core 선택/known0/PlayerLinks/Kernel 정리는 호출자의 앞선 원문 단계이며 여기서 실행하지 않는다. 앞선 부분 효과는 되돌리지 않는다. 메모리는 `hooks.memory(주소, 폭)`가 주는 같은 실제 물리 뷰만 쓰고, NULL·짧은 물리·누락 장치는 도달한 위치에서만 거부한다.

## 2. 검증 (`tests/player-initialization.test.mjs`, 3건)

| 검사 | 근거 | cpppj 대응 |
| --- | --- | --- |
| 202행(독립 입력 101개 × x87 제어값 027f/037f): 정상 174·NULL 물리 실패 8·외부 중단 20의 반환, 전역 8개, 세 Player 물리 전체, 방향표 81 DWORD, 채팅 4800 BYTE, 채팅 이벤트, 호출·전역 재조회 순서 | `player-initialization-x86.tsv`(10.78 전용 표) | PlayerInitialization_Replays202IndependentNativeObservations |
| 선택하지 않은 free는 요구하지 않음·누락/짧은 DWORD는 앞선 효과를 남김 | (독립 입력) | PlayerInitialization_RequiredDevicesKeepEarlierPartialEffects |
| 해제 callback이 받는 이전 base·아홉 번 해제 뒤 반납·행 필드 값 | (독립 입력) | — (JS 보강) |

동맹·Chat 하위 몸체는 이미 옮긴 `RawPlayerParticipation.setAlliance`(E5)와 `RawChatChannels`(E6)가 실제로 수행한다. 따라서 이 대조는 두 모듈을 외부 효과(known1 변경·재진입·base 변경 중단)와 함께 실제 부모 안에서 다시 검증한다.

최초 실패와 교정: 구현 직후 202행과 계약 검사가 처음부터 통과했다. 기대값은 원문 열만 썼다.

## 3. 경계

- cpppj `PlayerInitialization_CoreDetachCacheBeforeReleaseAndNewAliasAfterAllocation`(실제 `ClientCoreDataGlobals`의 4·4·2 cache/NULL 정리 시점)은 client Core 전역 단위를 옮길 때 대조한다. 이 단위는 해제 callback이 이전 base를 받는 시점까지만 검사한다.
- 실제 세션의 `hooks.memory`/할당 장치/base 슬롯은 World 조립(WB05)이 공급한다. 그 전까지는 값형 장면으로 독립 대조한다.
- CD 10.72 배치(Player 172 BYTE)는 범위 밖이다.

## 4. 다음 시작점

`RawPlayerWorld`(PlayerWorldState 소비자)·`RawPlayerTechnology`·`RawPlayerColor.h`(RawBridgeConnect 선행)·알림 계열, 그리고 `SquidPostPop` 처리기 본체·종류별 PostPop·`SquidDisplay`·`SquidFrame`.
