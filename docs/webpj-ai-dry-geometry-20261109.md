# webpj AI 가짜 부족 좌표 탐색(RawAiDryGeometry) 이식 — WB04-E163

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백예순세 번째 부분이다. AI 가 물(dry) 부족 지점의 대체 좌표를 고르는 원본 선분/수직 양방향 탐색(`00415bb0`)과 디버그 표식 수명(`00414d30` 정리·`00410540` 생성)·점 거리(`0041cfc0`)를 옮긴 `o/RawAiDryGeometry`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. AI 계열(33개 파일)의 첫 단위이며, 이후 `RawAiDrySpots`·`RawAiFakeDrySpots` 등이 이 탐색기를 쓴다.

## 1. 옮긴 것

`www/core/raw-ai-dry-geometry.js`

- `aiPointDistance(first, second)` — 두 float 좌표 차이의 제곱합/제곱근을 중간 float 저장 없이(배정도) 계산한다. 무한/NaN 은 그대로 전파된다.
- `new RawAiDryGeometry(pool, globals, hooks)` — `globals = { bounds{ left, top, right, bottom }(00544d14..20, 양끝 포함), debug(0052f3dc, 29 일 때만 표식), markerType(005411f8, 초기 104) }` 는 매 위치의 현재 값을 읽도록 같은 객체를 빌린다. `hooks = { surface(x, y)→WORD, create(type, flags)→SID, word(sid, word), pop(sid, point, flags), destroy(sid, flags) }` 는 도달한 것만 필요하며 없으면 앞서 수행한 효과를 유지한 채 그 위치에서 거부한다. PATCH(10.78) 풀로 용량 65536 이하여야 한다.
- `candidate(source, target)` — (디버그면 표식 정리 후 두 끝점 표시) 선분을 float 누적으로 방문(디버그면 각 점 표시)하고, 중점에서 수직 양방향(왼쪽 먼저)으로 이동하며 각 방향의 연속 빈칸이 4 가 될 때까지 반복해 왼쪽이 4 이면 왼쪽(동률도), 오른쪽만 4 이면 오른쪽 좌표를 돌려준다. 현재 경계(양끝 포함, NaN 제외) 안에서만 이동하며 각 축의 `0.9999f` 편향 합을 `ftol`(CRT signed64 하위 DWORD)한 칸이 지도 256×256 밖이면 지도 효과를 요청하지 않고 빈칸으로 본다. 원본의 반복 예산 부재도 유지한다.
- `clearMarkers()` — SID 1 을 건너뛰어 free 비트만 건너뛰는(dead 는 방문) iterator 로, 다음을 먼저 고른 뒤 객체 TYPE BYTE 가 현재 `markerType` 전체 DWORD 와 같고 kind WORD 가 93 인 것만 가상 삭제한다. `mark(point)` — 현재 TYPE/flags 2 생성 → kind 93 쓰기 → Pop(flags 0)이며 중간 실패도 앞선 효과를 취소하지 않는다.

이 모듈은 DOM 을 쓰지 않는다.

## 2. 검증

`tests/ai-dry-geometry.test.mjs`(12건). 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다). 같은 입력과 단언을 그대로 옮겼다.

1. 점 거리의 중간 float 저장 없음(3-4-5, float 최댓값 양끝의 유한성, NaN 전파).
2. 동률은 왼쪽 우선이며 두 방향을 매 반복 모두 방문(방문 칸 8개와 순서), 수직 대칭.
3. `0.9999f` 편향 합 뒤 ftol(30.00005→칸 32/41, 30.0002→칸 33/42).
4. 점유 칸이 각 방향 연속 횟수를 0 으로 만들고 오른쪽만 먼저 4 에 도달할 수 있음(방문 8개·10개).
5. 현재 점이 경계 끝값에 있으면 이동하고 이동 뒤 경계 밖 한 칸도 조회, 물리 256 밖은 빈칸이며 지도 효과 없음.
6. 디버그 표식의 순서(정리 → 두 끝점 → 선분 점 → 수직 양쪽, 14개)와 표식 도중 바뀐 현재 디버그/TYPE 전역 재조회.
7. 누락 효과가 부분 쓰기를 보존(생성만/생성+kind 쓰기), 표식 정리의 TYPE BYTE·WORD 93 일치, dead 방문/free 건너뜀, 삭제 전 다음 SID 저장, 삭제 뒤 현재 전체 DWORD TYPE(상위 비트가 있으면 불일치), 미연결 지도 경계 거부·PATCH 풀 계약.

최초 실패와 교정: 새 모듈은 12건 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 이 탐색기를 부르는 `RawAiDrySpots`·`RawAiFakeDrySpots`(가짜 부족 스캔/표식 수명)와 AI 두뇌(`RawAiBrain`) 연결은 AI 계열의 후속 단위이다. 실제 지도 WORD(`00542514`)·가상 생성/Pop/삭제는 호출자가 공급한다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

AI 계열(`RawAiDrySpots`·`RawAiFakeDrySpots`·`RawAiBuildSearch`·`RawAiWallet`·`RawAiEconomy` 등), `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
