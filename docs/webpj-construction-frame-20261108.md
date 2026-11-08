# webpj 건설 표시 프레임(RawConstructionFrame) 이식 — WB04-E130

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백서른 번째 부분이다. 건설 중인 건물 위 입자 효과의 프레임 갱신(RunFrame)·인접 입자 추가·팔레트 조회·시각 누적·Draw 제출을 옮긴 `o/RawConstructionFrame`(cpppj 기준선 `1f00a25a`)이다. 선행은 [건설 표시(E128)](webpj-construction-visual-20261108.md)의 필드와 공유 게임 난수 `GameRandom`, `SidPool` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(HP 16비트 폭·나눗셈 순서·잘못된 픽셀의 `total` 선증가·비활성 조기 반환)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-construction-frame.js`

- `createConstructionFrameFields()` — 표시 상태(`visual`)와 시각 helper 가 쓰는 float 둘(`delta`, `scaledDelta`).
- `createConstructionFrameState()` — `now`(게임 시각 double)·`rate`(scaledDelta 에만 곱하는 float)·`cameraX/Y`(dirty/Draw 에서 빼는 화면 원점).
- `RawConstructionFrame(pool, visualState, state, palette, random, hooks)` — 경계는 `maxHitPoints(sid)`·`invalidate(rect)`·`draw(request)`·`kill(flags)` 네 개(하나라도 없으면 생성 때 거부). `palette = { token, count, entries }` 는 색수 DWORD 와 읽을 수 있는 DWORD 배열을 구분하며 live 로 읽는다.
  - `run(fields)` (00422f00): 부모 좌표×배율−hotspot 으로 표시 사각형 계산(왼쪽 위는 float 저장, 오른쪽은 저장된 왼쪽 + 폭, 아래는 double 높이 가산) → `clock` → 부모 HP(경계 호출 **전**에 읽음)와 최대 HP 경계(호출 중 부모 raw·시각이 바뀔 수 있음) → 목표 선택 수 = ftol(HP/(최대 HP−1)×불투명 수) → 선택 수가 목표 미만이고 활성<총수<용량인 동안 활성 앞 구간에서 난수로 입자 하나를 골라 이웃 확장(`expand`) 후 활성 위치로 교환 → 활성 입자의 수명 감소(저장은 float, 만료 판정은 double 값)·만료 시 원래 색 복원 후 마지막 활성/마지막 전체 입자로 **두 번 덮고 같은 위치 재처리**·미만료 시 색 표 갱신 → 미활성 입자의 수명 감소·만료 시 난수로 수명 재설정(`life + delta + 저장된 life` 순서로 float 저장)·색 표 순환 → 이전/새 사각형 dirty 큐 → 부모 +40 바이트의 abstract 비트가 0 이면 `kill(0)`. dirty 경계가 카메라나 abstract 비트를 바꿀 수 있고 이후 읽기는 변경된 값을 쓴다.
  - `draw(fields)` (00423200): 용량 0 이면 부모/장치에 접근하지 않고 생략, 총수>용량·부모 0 은 거부, 아니면 부모 좌표 float 저장→카메라 감산→hotspot 감산→ftol 로 위치를 정해 출력 버퍼(pitch=폭, clip=0,0,폭,높이)를 제출.
  - `expand(fields, pixel)` (00422ea0): 저장소 검증 후 첫 행/마지막 행은 건너뛰고 오른쪽·왼쪽·위 이웃을 불투명하면 입자로 추가한다. 행 좌우 경계는 검사하지 않는다(원본 동일). 앞 선택이 바꾼 버퍼/용량을 다시 읽는다.
  - `append(fields, pixel)` (00422e30): 용량이 차면 거짓, 범위 밖 픽셀은 거부(원본의 범위 밖 접근을 옮기지 않음), 아니면 입자를 만들고 원본 색을 투명으로 바꾼다.
  - `color(life, maximum)` (00487e50): 패치는 역수를 float 로 먼저 저장하고 `색수(float)×min(life,최대)×역수` 를 double 로 곱한 뒤 ftol, 음수는 0, 배열 밖은 거부. 비유한·최대≤0·색수<0 은 거부.
  - `clock(fields)` (00418400/30): 첫 프레임에 시각 기준을 초기화하고 `(now−lastTime)×10` 을 float `delta`, `×rate` 를 `scaledDelta`(둘 다 float 저장)로 두며 누적 시각에는 저장된 float `delta` 를 더한다.
- CRT `ftol` 은 64비트 절단의 하위 DWORD 이다(비유한/범위 밖은 0).

## 2. 검증

`tests/construction-frame.test.mjs`(4건):

1. `constructionframe-x86.tsv` 의 `originals` 435행(Run 202·Expand 64·Append 8·Color 90·Clock 49·Draw 18·Sequence 4; `originalCD`·`original1037` 은 다른 판본이라 제외) — 부모 raw(좌표/HP/상태)·표시 전역·64비트 double 시각 패턴·입자 3개·난수 상태·색 표 입력에서 외부 요청 순서(`H` 최대 HP·`I` dirty 사각형·`D` 장치 제출·`K` 종료), 필드 14개(사각형·delta·scaledDelta·선택·불투명·firstDraw·활성·용량·총수·시각·누적)의 비트, 원본 픽셀·출력 버퍼·입자 12칸 전체, 난수 상태, 반환값, 5단계 연속 Run/Draw 기록이 모두 같다. 경계 호출 중 변경(HP 1·100 변경, 좌표+시각 변경, 카메라 변경, 부모 abstract 비트 해제)도 포함한다.
2. 계약: 마지막 행 불일치·출력 크기 불일치·입자 목적지 범위 밖·범위 밖 Append·부모 0·색 배열 부족·최대 수명 0 을 쓰기 전에(난수·픽셀 변화 없이) 거부.
3. 계약: 종료 경계 누락 거부, 용량 0 Draw 생략, 총수>용량 Draw 거부.
4. 통합: 실제 `RawConstructionVisual.create`(일반/즉시)로 만든 상태를 연속 Run/Draw 에 넘겨 첫 프레임 `delta=0`·모든 입자가 활성화·만료되어 출력 버퍼가 원래 픽셀로 복원되고 선택 수가 불투명 수와 같아지며, 부모 abstract 비트를 지우면 마지막 요청이 `K:0` 이다.

최초 실패와 교정: 새 모듈은 435행 fixture 를 첫 실행에 통과했다. 검사 쪽 입력 열 수 확인식만 한 번 고쳤다.

## 3. 경계와 남은 것

- C++ 의 `ConstructionLifetime_CreateRunKillAndReuse`(Run 의 마지막 `kill` 을 표시 해제에 연결하는 통합 시험)는 이제 `RawConstructionLifetime`(E129)과 이 프레임이 모두 있으므로 이식할 수 있으나 Kill 의 실제 소유자(`ConstructionProcess` 계열)와 함께 확인하는 것이 자연스러워 후속으로 둔다. 위 4번이 프레임 쪽 종료 요청까지는 확인한다.
- 월드/클라이언트가 소유하는 실제 최대 HP 경계·dirty 큐·장치 blit 는 후속 연결이다.

## 4. 다음 시작점

`RawRenderCollection`(4305행; 선행 `RawRenderRegistry`·`RenderProcessFields` 는 E129 에서 완료), `RawConstructionProcess` 계열, Missile Host 팩토리(`RawGunProcess` 이후), `RawGunImpact`(SquidFrame/Host 장면 필요), `RawWindAviaryProcess`·`RawWindFlyer`, 비행체, 번개 수명/프레임/화면, `RawPathProcess` 계열(커널/Form 장면 선행), `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
