# webpj 일반 입력 분배(Pause 키·회전 방향 키·URL 명령) 이식 — WB04-E20

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 스무 번째 부분이다. cpppj 기준선 `1f00a25a`의 `client/PauseKeyInput`·`client/RotationKeyInput`·`client/UrlInput`(10.78)을 옮겼다. 모두 Guide/Gump/Console을 지난 일반 입력 하나를 받아 소비 여부를 돌려주는 작은 분배기이며 각각 독립 원본 관측이 있다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/pause-key-input.js` | 004d8dd3..004d8df1 | `dispatchPauseKeyInput(event, {command, queue})`: 뗌 비트가 없고 수정키를 제거한 코드가 가상 키 `0x130000`이면 현재 Pause 명령 DWORD를 예약 직전에 조회해 일반 FIFO에 `(명령, 0, 0)`으로 예약하고 true. 그 밖은 장치를 요구하지 않고 false. Shift/Control/Alt/outside/pressed는 거부하지 않고 현재 정지는 읽지 않는다. |
| `www/core/rotation-key-input.js` | 004d96b6, 004d97a0..004d97ce | `dispatchRotationKeyInput(event, rotation, {paused, configure})`: 수정키 제거 키가 `C`/`c`이고 뗌이 아니면 소비. 정지 중이면 효과 없이 소비, 아니면 회전 방향 DWORD를 0↔1로 반전해 쓴 뒤 같은 반전값으로 `rotmode=<값>`을 Config에 기록한다. Config 실패 때도 반전 쓰기는 보존. |
| `www/core/url-input.js` | 4d7b09..4d7b1e | `dispatchUrlInput(event, text, activate)`: 수정키 제거 코드가 URL 명령(`0x0200005b`)이 아니면 false, 뗌이면 효과 없이 true, 아니면 두 번째 DWORD(`event.y`의 32비트 전체)를 문자열 토큰으로 `text(token)`을 얻어 `activate`로 링크를 활성화한다. 모드는 조회하지 않는다. |

## 2. 검증 (`tests/key-inputs.test.mjs`, 7건)

| 검사 | 근거 |
| --- | --- |
| Pause 키 246행(상위 비트·뗌·선행 소비·현재 명령·실제 40칸 FIFO 포화와 예약 순서) + 정확한 코드/장치 누락 | `pause-key-input-x86.tsv` |
| 회전 방향 키 264행(C/c·수정키·뗌·정지·Config 재진입/중단의 방향·할당문·호출 시점) + 정확한 코드/부분 효과 | `rotation-key-input-x86.tsv` |
| URL 입력 96행(두 번째 DWORD 전체 비트·뗌·수정키·모드 독립) + 소비 범위/장치 누락 | `url-input-x86.tsv` |

최초 실패와 교정: 7건이 처음부터 통과했다. 기대값은 원문 열만 썼다.

## 3. 경계

- cpppj `PauseKeyInput_QueuedCommandUsesCurrentPauseAndExistingActions`(실제 `GameMenuActions`가 예약된 Pause를 소비 시점의 정지로 처리)·`RotationKeyInput_ActualClientConfigAndWorldResetLifetime`(실제 Client Config/커서 전역)·`UrlInput_MetadataNameQueueAndCurrentStringLifetime`(메타 resolver·스크립트 문자열 고리 수명)은 해당 Client/UI 단위를 옮길 때 대조한다.
- 입력 큐는 이미 옮긴 `InputQueue`(`input-queue.js`)와 `InputCode`를 그대로 쓴다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
