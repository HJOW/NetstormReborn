# webpj 표시 목록 수명(RawConstructionLifetime) 이식 — WB04-E129

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백스물아홉 번째 부분이다. 원본 `Particle.cpp` 의 표시 목록(RenderProcess 시스템 배열) 등록/제거와 `Build.cpp` 의 건설 표시 버퍼 소멸 순서를 옮긴 `o/RawConstructionLifetime`(cpppj 기준선 `1f00a25a`)이다. 선행은 [건설 표시 이식(E128)](webpj-construction-visual-20261108.md)의 `ConstructionVisualFields` 와 `SidPool` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 의 깊이 BYTE 위치(33)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-construction-lifetime.js`

- `createRenderProcessFields()` — RenderProcess 공통 필드(`token`·`form`·`parent`·사각형 4개·`slot`·`zOrder`). `token` 은 원본 포인터 대신 쓰는 0 이 아닌 호스트 식별자다.
- `RawRenderRegistry(pool, hooks, initialTokens)` — 원본 32비트 포인터 배열을 토큰 배열로 표현한 표시 목록. 경계는 `reserve(바이트)`·`releaseArray()`·`attach(fields, type, parent, flags)` 세 개이며 하나라도 없으면 거부한다.
  - `construct(fields, token, parent, type)`: 필드 초기화 → 첫 빈 칸 검색, 없으면 40칸 확장(새 배열 확보 → 기존 내용 복사 → 기존 배열이 비어 있지 않으면 반납) → `slot` 지정과 슬롯 0 의 깊이를 `zOrder` 로 기록 → `parent` 가 0 이 아니면 flags `0x10` 으로 부착 요청 후 진입 부모의 현재 깊이를 다시 읽는다. 부착 경계가 부모의 깊이 BYTE 를 바꿀 수 있고(관측 Mutate 1), 부착 경계가 정한 부모 SID(51)가 필드에 남는다(관측 Mutate 2). 토큰 0·중복, 살아 있는 등록 필드 재사용, 풀 밖 부모, 확장 범위 초과는 변경 전에 거부한다.
  - `destroy(fields)`: 번호·토큰이 일치한 등록만 지우고 base 부모/form 을 `-12345` 로 바꾼다. slot·zOrder·사각형은 그대로 남는다. 번호 범위 밖·토큰 0·불일치·중복 삭제는 변경 없이 예외로 거부한다.
  - `slots`(독립 사본)·`pool`.
- `destroyConstructionVisual(visual, registration, registry, flags, release)` — 원본 free 순서: 캡처 픽셀 → 출력 버퍼 → 입자 버퍼를 비어 있지 않을 때만 차례로 반납(`ConstructionRelease.SOURCE/OUTPUT/PARTICLES`) → 표시 등록 제거(`destroy`) → `flags` bit 0 이면 객체 반납(`OBJECT`). 반납 경계는 등록이 아직 살아 있을 때(버퍼) / 이미 제거된 뒤(객체) 호출된다. 공유 팔레트는 반납하지 않는다.
- `makeConstructionRegistryHooks(registry, registration, token, hooks)` — 건설 표시 생성(`RawConstructionVisual`)의 `attach` 경계를 `registry.construct` 로 바꾸고, 상위 부착이 정한 부모 SID(0~65535)·초기 사각형·`zOrder` 를 표시 객체로 되돌려준다. 토큰 0 은 거부한다.

## 2. 검증

`tests/construction-lifetime.test.mjs`(4건):

1. `constructionlifetime-x86.tsv` 의 `originals` 459행(Construct 138·Sequence 1·Render 64·Build 64·DeleteBase 64·DeleteInstant 64·DeleteNormal 64; `originalCD`·`original1037` 은 다른 판본이라 제외) — 목록 용량·빈 칸 위치(없음/처음/중간/끝)·부모·깊이 입력에서 외부 요청 순서(`A:` 확보 바이트·`F:T` 배열 반납·`B:` 부착 인자·`F:S/O/P/B` 버퍼·객체 반납), 슬롯 배열 전체, 필드 8개의 DWORD 비트, 삭제→재등록 연속 기록, 반환값이 같다.
2. 계약: 토큰 0·중복 토큰·살아 있는 필드 재사용·잘못된 번호/토큰 삭제·중복 삭제·경계 누락·토큰 0 어댑터·반납 경계 누락을 거부하고 정상 목록은 변하지 않는다.
3. 계약: 확보가 실패하면 기존 배열·토큰을 보존하고 부착을 요청하지 않는다.
4. 통합: 건설 표시 생성에 목록 부착을 연결하면 슬롯 0 에 토큰이 등록되고 부모/깊이가 표시 객체에 반영되며, 해제하면 `F:S;F:O;F:P` 순서로 반납되고 등록은 지워져(`-12345`) 목록 40칸이 남는다. 같은 토큰으로 즉시 건설을 다시 만들 수 있다.

최초 실패와 교정: 459행 대조에서 사건이 없는 행의 외부 요청 표기를 실행기의 `-`(빈 값) 대신 빈 문자열로 비교한 검사 쪽 실수를 고쳤다. 몸체 수정은 없었다.

## 3. 경계와 남은 것

- C++ 의 `ConstructionLifetime_CreateRunKillAndReuse` 중 실제 `RawConstructionFrame` 의 Run/Draw/Kill 연결(프레임 시험)은 `RawConstructionFrame` 이식 뒤에 확인한다. 위 4번이 같은 소유권 흐름 중 생성→부착→반납→재사용을 프레임 없이 확인한다.
- `RawRenderCollection` 이 쓰는 `RawRenderRegistry`·`RenderProcessFields` 가 이제 있으므로 수집기 이식(4305행)의 선행이 풀렸다.

## 4. 다음 시작점

`RawConstructionFrame`(Run/Draw/Kill; fixture `constructionframe-x86.tsv`) → `RawRenderCollection`(4305행), `RawConstructionProcess` 계열, Missile Host 팩토리(`RawGunProcess` 이후), `RawGunImpact`(SquidFrame/Host 장면 필요), `RawWindAviaryProcess`·`RawWindFlyer`, 비행체, 번개 수명/프레임/화면, `RawPathProcess` 계열(커널/Form 장면 선행), `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
