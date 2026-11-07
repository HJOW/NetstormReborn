# webpj Chat 채널 기록(RawChatChannels) 이식 — WB04-E6

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여섯 번째 부분이다. Player 전체 초기화(`RawPlayerInitialization`)가 실제 동맹·Chat 몸체를 거쳐 4800 BYTE 채팅 저장소 전체를 대조하므로 그 선행 단위로 먼저 옮겼다. cpppj 기준선 `1f00a25a`의 `client/RawChatChannels`(원문 00444ee0·00445080·00445140, 10.78 PATCH 전용)이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-chat-channels.js` — 채팅 기록 100개(한 기록 48 BYTE: 이름 40·플래그 +0x28·미사용 3·subscriber +0x2c)를 **복사하지 않고 빌려** 원문 효과를 그대로 쓴다.

- **`add`(00444ee0)**: 현재 자기 subscriber와 같으면 이름·효과를 읽기 전에 -1. subscriber -1은 기존 -1을 갱신하지 않고 첫 빈 기록(0)만 예약하며 플래그 0x10을 항상 지운다(모두 차 있으면 실패). 그 밖은 첫 기존 기록, 없으면 첫 빈 기록(가득 차면 -1). 새 기록은 subscriber DWORD를 쓰고 이름(NULL 이름이면 `Unknown%08x`)을 40 BYTE로 채우며 주변 stale 값은 초기화하지 않는다. 기존 기록은 이름 첫 BYTE를 0으로 쓴 뒤 이름이 있으면 덮고 플래그를 OR한다. 마지막은 `+0x27=0`→플래그 쓰기→이름 정제(0042b0e0)→빈 갱신(004437f0) 두 번이다.
- **이름 정제**: 금지 문자(0xa0 `% . ? = ( ) [ ] ~ " ; :` `` ` `` 공백)를 지우고 같은 위치를 다시 검사한다. NUL 뒤 stale BYTE는 보존하고, 빈 이름이면 `Cheat<subscriber 하위BYTE 10진>`을 쓴다.
- **`addToChannel`**: AI의 8인자 호출에서 실제로 읽히는 인자 1/2/6만 같은 `add`에 전달한다(Player 동맹 몸체가 쓰는 입구).
- **`clear`**: subscriber가 비제로인 기록에만 플래그에서 하위 BYTE 마스크를 지운다(빈 기록의 stale 플래그 유지).
- **`resetOccupied`(00445140)**: subscriber가 비제로인 기록의 48 BYTE 전체를 0으로 쓴다(State 정리용, 빈 기록과 두 subscriber 전역은 그대로).
- 이름은 원본 바이트의 이진 문자열이다. 관찰 콜백(`CHAT_CHANNEL_CALL.nameChanged`/`refresh`)은 하위 몸체 진입 위치만 알리며 계산을 대체하지 않는다.

## 2. 검증 (`tests/chat-channels.test.mjs`, 4건)

| 검사 | 근거 | cpppj 대응 |
| --- | --- | --- |
| Add/Clear 2448행: 전체 4800 BYTE 변경과 하위 진입 순서(`N,<subscriber>;R;R`) | `chat-channels-x86.tsv`(판본 열 없는 10.78 전용 표) | ChatChannels_Native2448 |
| 현재 자기 subscriber 변경·이름 정제 진입 시점의 플래그/이름 상태·정제 후 갱신 상태 | (독립 입력) | ChatChannels_CurrentSubscriberAndEffectOrder |
| 저장소 크기·조회기 누락 거부 | (독립 입력) | ChatChannels_StorageAndEditionContracts |
| `resetOccupied` 점유/빈 기록, `Cheat255`/`Unknown00000abc` 이름 | (독립 입력, 위 2448행에도 일부 포함) | — (JS 보강) |

최초 실패와 교정: 구현 직후 2448행이 처음부터 통과했다. 기대값은 원문 변경 목록에서만 만들었고 모듈 출력을 기대값으로 삼지 않았다.

## 3. 경계

- cpppj `ChatChannels_ClientLifetimeAndSharedAiBinding`·`ClientDefaultAiNameUsesCurrentPathSpec`(실제 `Client`의 `ChatRecords()`·`CurrentSubscriber()` 수명, AI 서비스 두 벌이 같은 저장소를 공유, 언어 Config의 기본 AI 이름)는 해당 client/세션 단위를 옮길 때 대조한다.
- CD 10.72 배치는 범위 밖이다(원본 모듈도 10.78만 허용).

## 4. 다음 시작점

`RawPlayerInitialization`(Player 9기록 전체 초기화; 이 모듈과 `RawPlayerParticipation.setAlliance`를 실제 자식으로 사용), 이어서 `RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerColor.h`·알림 계열, `SquidPostPop` 처리기 본체 등.
