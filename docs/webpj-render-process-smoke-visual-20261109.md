# webpj 공통 표시 저장소(RenderProcessSystem)·연기 표시 시스템(SmokeVisualSystem) 이식 — WB04-E192

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아흔두 번째 부분이다. E183~E191 에서 옮긴 입자 효과 몸체들을 실제 Host/Kernel 과 하나의 원본 표시 목록에 붙이는 첫 단위로, 모든 `*VisualSystem` 이 공유하는 `o/RenderProcessSystem`(토큰·등록/부착 분배·출력 분배·수집, 패치 `00487960` Form 삭제 영역)과 첫 소비자인 `o/SmokeVisualSystem`(E191 냉각 연기 process 21 의 Host 부착·프레임/출력·자기 삭제·소멸 수명)을 옮겼다.

- cpppj 기준선 `1f00a25a`. 두 소스·시험·fixture 모두 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋: `RenderProcessSystem` `c258c7f3`, `SmokeVisualSystem`·시험 `fbdcb4df`, 삭제 영역 fixture `3fda610d`.
- 공유 장치: D5c `SquidProcess`/`SquidProcessHost`·WB04-C1 `Kernel`·D2 `SquidUnpop`/`RawSquidDestroy`, E129 `RawRenderRegistry`, E131 `RawRenderCollection`, E128 `createConstructionVisualState`, E191 `raw-smoke-effect.js`.

## 1. 옮긴 것

`www/core/render-process-system.js`(DOM 없음)

- `renderProcessRemovalBounds(fields, cameraX, cameraY)` — 00487960: 네 변을 먼저 정수화한 뒤 left 는 `float(left − 카메라)`, right/bottom 은 `float(float(정수) − 카메라)` 로 저장하고, top 만 저장 없이 넓은 `(top − 카메라) − 1` 을 정수화한다. 정상 signed32 밖은 거부한다.
- `RenderProcessSystem(pool, host, state, hooks)` — Host 와 풀이 같아야 한다. 원본 registry 의 attach 를 토큰별 실제 Host 부착 callback 으로 바꿔 넣고(필드 객체가 다르면 거부), `track`(비영 토큰 발급, 이미 등록된 필드/경계 누락 거부, 32비트 감김 거부)·`ready`(원본 슬롯 등록 뒤에만)·`forget`·`draw`(원본 슬롯 순서, 매 차례 현재 슬롯 재조회)·`drawToken`(출력 callable 을 먼저 꺼내 자기 삭제에도 이번 호출 유지)·`collect`(기존 수집기에 준비 완료 항목만 공급)·`lookup`/`size`/`slots`/`registry` 를 제공한다.

`www/core/smoke-visual-system.js`

- `SmokeVisualProcess`(SquidProcess 파생) — `runFrame`/`onFormDestroy`/`draw`/`destroy` 를 시스템 내부 연결로 넘긴다. C++ 소멸자는 웹에서 `destroy()` 이며 Kernel 제거·부착 실패·생성 실패(부착 전)·부모 0 정리 때 불린다.
- `SmokeVisualSystem(rendering, random, lifetime, frame, palette, hooks)` — 출력/반납 경계 누락과 표시 배율 불일치(프레임 16/11 ≠ 수집 상태)를 거부한다. `addCooling(parent, [x, y])`(부모 0 거부, 10회/간격 3)·`add(request)`(8회/간격 5)는 같은 생성 경로를 쓴다: 대기 객체 → `track` → 수명 생성(등록 안에서 실제 Host 부착, 부착 실패면 즉시 소멸 뒤 오류) → 준비 완료 → 부모 0 이면 시스템 보관. 생성 중 중첩 생성은 거부하고, 실패하면 부착된 객체는 Host 삭제(flags 0x10), 부착 전 객체는 직접 소멸한다. 프레임/출력은 준비 완료 객체만 매번 배율을 확인하고 `RawSmokeFrame` 을 실행하며 kill 은 그 프로세스의 Host 삭제로 연결한다. Form 삭제 직전에는 `renderProcessRemovalBounds` 로 dirty 를 낸다. `dispose()` 는 C++ 소멸자(부모 0 객체 직접 소멸, 나머지 Host 삭제)다.

## 2. 검증

`tests/smoke-visual-system.test.mjs`(6건).

- cpppj `SmokeVisualSystemTests.cpp` 4건을 1:1 로 옮겼다(실제 SidPool·SquidHash·SquidUnpop·RawSquidDestroy·Kernel·SquidProcessHost 장면). 냉각 연기의 TYPE 21 Form·200입자·깊이 6·10프레임 방출·출력·수명 끝 다음 프레임 자기 삭제와 입자 → 객체 반납·삭제 깊이 0, 생성 중 프레임/출력/수집 무동작과 중첩 거부·확보 실패의 객체만 반납·공유 색 1회 생성, 일반 연기 기본값, 부모 0 독립 소유와 정리 때 반납.
- `render-removal-x86.tsv`(10.78, 기본 027f 48행 + 비교 037f 48행, SHA-256 `70592ed5…`): 00487960 의 영역 float 넷·카메라 둘 입력에 대한 signed DWORD 넷과 flags 0 을 DWORD 단위로 대조한다(cpppj `FieryTailVisualSystemTests.cpp` 의 `RenderProcessSystem_RemovalBoundsNative027f` 와 같은 재생).
- 웹 추가 계약: 토큰 발급/ready 거부/forget 중복 무시/부착 필드 불일치/Host 풀 불일치/경계 누락 거부와 손 계산 삭제 영역 2건.

최초 실패와 교정: 모듈과 cpppj 이식 시험 4건·fixture 48입력은 첫 실행에 통과했다. 내가 손으로 계산한 삭제 영역 기대값 두 개(top 의 −1 여유, 음수 left 의 0 방향 절삭)가 틀려 1건 실패했고 시험 쪽 기대값을 고쳤다(모듈 변경 없음).

## 3. 경계와 남은 것

- 실제 월드(Session/Client)에 공통 표시 저장소와 연기 시스템을 만들어 붙이는 일, 냉각 wrapper 호출자(현재 SHP 포구 조회)·공유 색 owner·dirty/선 래스터 연결은 순서4~5 몫이다(PORT_MAP session 0 유지).
- 나머지 `*VisualSystem`(Burning·Construction·Dissolve·EvilRay·FieryTail·Lightning·Shrapnel)은 이 저장소 위에 같은 방식으로 얹는다. cpppj 의 Dissolve/FieryTail/Lightning 시스템 시험도 `RenderProcessSystem` 을 함께 쓴다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "입자 표시 마무리": `DissolveVisualSystem`·`FieryTailVisualSystem`·`LightningVisualSystem`(각각 cpppj 시험 있음)·`ShrapnelVisualSystem`·`EvilRayVisualSystem`·`ConstructionVisualSystem`, 그리고 선행 부품이 필요한 `RawBurningFrame`→`BurningVisualSystem`.
