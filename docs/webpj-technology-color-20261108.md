# webpj 기술 획득·테마 기술·소유자 색 변경 이식 — WB04-E13

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 열세 번째 부분이다. cpppj 기준선 `1f00a25a`의 작은 `o/` 단위 셋(모두 10.78, 각각 독립 원본 관측 보유)을 한 번에 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/raw-new-technology.js` | 00492a30 | `acquireNewTechnology(type, hooks)`: 기술 부모 getter 한 번 버린 호출 → 다시 읽은 부모로 조회(`004acac0(parent,4,type,0)`; 반환은 SID가 아닌 종류 WORD) → 없을 때만 부모를 다시 읽어 contained 생성(`004afa70(parent,type,0,4,0x10,0)`) → 기술이 이미 있어도 생략하지 않는 저장 snapshot → 현재 TYPE 이름(첫 NUL까지)을 `global.newTechName`에, 그림 서식(`<!`"%d.*`">`, signed type)을 `global.newTechPic`에 쓰고 → 마지막에 안내 `NewTech`. 선택된 장치 누락은 앞선 부분 효과를 남긴 채 그 위치에서 거부한다. |
| `www/core/raw-theme-technology.js` | 00492ae0 | `RawThemeTechnology(hooks).apply(theme)`: 타입 0부터 현재 개수(반복마다 다시 읽음) 미만까지 group 10·theme 5·선택 테마와 다른 기록을 건너뛰고, 현재 기술 부모로 contained 조회 후 없을 때만 부모를 다시 읽어 생성한다. 다섯 경계가 모두 있어야 생성된다(누락은 빈 성공으로 숨기지 않음). |
| `www/core/raw-player-color.js` | 0043ca70 (+ `RawBridgeConnect`의 OwnerColorFrame) | `changePlayerColor(pool, state, owner, color, setFrame)`: 색 표를 먼저 쓰고(되돌리지 않음) SID 2부터(0/1 번 제외) free만 건너뛰며 섬 받침/종유석 슬롯의 소유자 프레임(`ownerColorFrame`: 전투가 아니거나 소유자 0 이하면 중립 8, 아니면 표 값−1)을 소비자에 보낸다. 프레임 효과 뒤 현재 슬롯·풀 길이를 다시 읽고 dead/void/abstract는 제외하지 않는다. 소비자 누락은 해당 슬롯에서 거부하되 색 표 쓰기는 유지한다. |

## 2. 검증 (`tests/technology-color.test.mjs`, 6건)

| 검사 | 근거 |
| --- | --- |
| 기술 획득 45행(현재 부모 재조회·중복 기술도 저장/안내·각 중단 뒤 두 Config 부분 효과) + snapshot 누락이 앞선 효과 보존 | `new-technology-x86.tsv` |
| 테마 기술 1200행(필터·개수/부모 재조회·생성 기록) + 다섯 경계 필수 | `theme-technology-x86.tsv` |
| 소유자 색 변경 216행(색 표 36 BYTE·풀 800 BYTE 창·프레임 요청 순서) + 소비자 누락/범위 밖 소유자 | `player-color-x86.tsv` |

최초 실패와 교정: 구현 직후 6건이 처음부터 통과했다. 기대값은 원문 열만 썼다.

## 3. 경계

- `OwnerColorFrame`은 원래 `o/RawBridgeConnect`에 있다. 이 단위에서 `raw-player-color.js`에 함께 두었고, `RawBridgeConnect`를 옮길 때 그쪽 모듈이 같은 함수를 가져다 쓰도록 정리한다(PORT_MAP의 `RawBridgeConnect.cpp`는 `todo`로 남음).
- `RawArcherTypeData`(TYPE 정의 파서 의존)는 이 묶음에서 제외했다.
- CD 10.72 경로는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
