# 조각 회전 방향(C 키·옵션 "Counterclockwise Piece Rotation") 연결 (WB04-E1151)

앱에 연결되지 않은 모듈을 찾는 점검(E1150, 71개)에서 남아 있던 `rotation-key-input` 과 `rotmode` 설정이 실제 플레이에 연결돼 있지 않음을 확인하고 연결했다.

## 증상

- 옵션 메뉴의 `Counterclockwise Piece Rotation` 을 켜도, 미션에서 `C` 키를 눌러도 건설 조각(다리·건물)의 우클릭 회전 방향이 바뀌지 않았다. 언제나 시계 방향(0→2→4→6→0)으로만 돌았다.
- `rotmode` 는 웹 설정(저장되는 키) 목록에 없어 게임 데이터에만 있었고, 브라우저를 다시 열면 사라졌다(원본은 `options.cfg` 에 저장).

## 원인

- 원본(cpppj `Client::InterpretCursorOptions`)은 설정을 해석할 때마다 회전 방향 전역(`cursorGlobals().rotationMode()`)을 0 으로 쓰고 `rotmode` 를 읽는다. 웹에는 이 동기화가 없어 회전 방향 전역은 계속 0 이었다.
- `C`/`c` 키 입력 분배기(`core/rotation-key-input.js`, 원문 004d96b6)는 모듈과 시험만 있고 입력 루프에 연결돼 있지 않았으며, 키 허용 목록에도 `c` 가 없었다.
- `rotmode` 가 설정 키로 정의돼 있지 않았다.

## 수정

| 위치 | 내용 |
| --- | --- |
| `www/netstorm.js` `_syncClientCursorOptions()` | 게임 데이터의 `rotmode`(없으면 0, 부호 있는 32비트 그대로)를 회전 방향 전역에 쓴다. 옵션·설정 쓰기 통지(`_onSettingChange`·`_onConfigWrite`)·설정 동기(`_syncGameData`)·초기화 때마다 부른다. |
| `www/netstorm.js` 입력 루프 | `c`/`C` 키를 지원 목록과 문자 키 허용 목록에 더하고 `dispatchRotationKeyInput` 으로 전달한다(정지 중이면 소비만, 대화창이 열려 있으면 처리하지 않음). 쓰기는 `_applyCampaignConfig('rotmode', …)` 를 거쳐 설정 저장소까지 간다. |
| `www/platform/settings.js` | `rotmode`(toggle, 기본 0)를 설정 키로 올려 로컬 저장소에 저장한다. 옵션 메뉴 토글과 C 키 모두 재시작 뒤에도 남는다. |

## 시험

- `e2e/mission-rotation-key.spec.js`(4:3·16:9 통과): The War Begins! 에서 `Q` 로 다리 조각을 쥐고 우클릭 회전을 잰다.
  - 기본: +2(시계). `C` 한 번 → 회전 방향 전역·게임 데이터·설정·로컬 저장소의 `rotmode` 가 모두 1, 조각은 유지, 우클릭은 −2(6). `C` 한 번 더 → 모두 0, +2.
  - 옵션 토글이 쓰는 `_applyCampaignConfig('rotmode', 1)` → 전역 1, 저장, 우클릭 −2.
  - 변형 확인: 동기화 본문을 지우면 옵션 토글 단계에서, C 키 분배를 지우면 `C` 한 번 단계에서 실패한다.
- Node: `tests/progress.test.mjs` 에 `rotmode` 설정 키 시험을 더했고 `popup.test.mjs`·`e2e/first-run.spec.js` 의 기본 설정 스냅샷에 `rotmode: 0` 을 더했다(설정 키 31 → 32개). 관련 Node 52건, 인접 Chromium 묶음 11건(카메라 Alt·객체 명령·게임 메뉴 막대·건설 위치) 통과.

## 남은 것

- 원본 `D`/`U`/Shift+숫자 키는 웹·cpppj 모두 구현이 확인되지 않았다(원본 분석이 먼저 필요).
- 앱에 연결되지 않은 나머지 모듈(`move-dialog`·`hud-ns-info`·`raw-theme-technology`·`text-clipboard` 등)은 같은 방식으로 하나씩 확인한다.
