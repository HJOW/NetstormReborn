# Pause 키 연결과 앱에 연결되지 않은 모듈 분류 (WB04-E1163)

## Pause 키

- 원본 도움말(`interfaceHelp`): "Shift-F9 or the Pause button Pause the game." 웹은 Shift-F9 와 게임 메뉴 Pause 만 동작했고 **Pause 키는 아무 일도 하지 않았다**. `core/pause-key-input.js`(가상 키 0x13 → 일반 Pause 명령 예약)는 이식돼 있었지만 앱에 연결돼 있지 않았다.
- 수정: `platform/browser-key-events.js` 의 가상 키 표에 `Pause`(0x13)를 더하고, `netstorm.js` 의 키 입력에서 `Pause` 를 큐에 넣고 입력 루프가 `dispatchPauseKeyInput` 으로 일반 Pause 명령(`0x020000cf`, 게임 메뉴 Pause 와 같은 경로)을 예약한다. 대화창이 열려 있으면 처리하지 않는다.
- 시험: `e2e/pause-menu.spec.js` 의 새 시험이 Pause 키로 정지(`Game Paused!` 안내·시계 멈춤)와 해제를 확인한다. 지원 목록에서 빼면 실패한다.

## 앱에 연결되지 않은 모듈(68개) 분류 — 확인한 것

| 모듈 | 판정 | 근거 |
| --- | --- | --- |
| `core/pause-key-input.js` | **연결함(E1163)** | 위 |
| `core/client-battle-completion.js` | 대체 경로 있음 | 앱은 `session.battleFinished()`(E1152 로 승패 흐름 확인) |
| `core/move-dialog.js`·`core/raw-destroy-at.js`·`core/raw-create-at.js` | 웹 범위 밖 | 쓰이는 대본은 User Made Campaigns 의 `sw_m*`·`Imp*`(DestroyAt 4개·MoveDialog 여러 개)이며 LEFT_JOBS 1절이 제외한다 |
| `core/object-owner-commands.js` | 웹 범위 밖 | 일반 입력 SetOwner 는 Edit 전용 |
| `core/raw-ai-debug-menu.js`·`core/raw-theme-technology.js` | 개발자 도구 | AI 디버그 메뉴·CheatAllTechnology |
| `core/thunder-process.js`(+`palette-flash.js`) | 원본 창 모드와 같은 무동작 | 번개 팔레트 효과는 **전체화면에서만** 구름 색(228~245번)을 바꾸고 창 모드는 지속 1/60초로 표시하지 않는다(`palette-flash.js` 머리). 웹은 전체화면 옵션이 없고 창 모드에 해당한다. 장면 음악 구동기의 `thunderFlash` 가 비어 있는 것과 맞는다 |
| `core/movie-timer.js`·`core/native-intro-state.js` | 웹 범위 밖/후순위 | 인트로 영상(Smacker)은 LEFT_JOBS 의 동영상 항목 |
| `ui/text-clipboard.js`·`ui/client-text-entry-*.js`·`ui/hud-chat-geometry.js`·`ui/raw-chat-*.js`·`core/client-chat-runtime.js` | 채팅 창 공장 | [F3 채팅](webpj-chat-toggle-f3-20261121.md) 의 조립 설계 메모 |
| `ui/hud-ns-info*.js`·`ui/meta-image-*.js`·`ui/raw-background-gif-controls.js`·`core/raw-fort-*.js`·`core/raw-outpost-lifecycle.js` | 멀티플레이/메타 | NSInfo 준비 창·메타 이미지·Fort 메타(Badges)·Outpost 는 멀티플레이 전용 |

나머지(예: `raw-priest-placement-*`·`client-production-sync`·`production-color-maps`·`raw-battle-resource-frame`·`storm-power-refresh` 등)는 대체 경로가 있는지 모듈별로 확인하지 못했다. 단독 미션이 31개 미션·장시간 진행·우클릭 전수 점검으로 오류 없이 도는 것은 확인했으므로 실제 미연결 공백이 있다면 눈에 띄지 않는 부수 기능일 가능성이 크다.
