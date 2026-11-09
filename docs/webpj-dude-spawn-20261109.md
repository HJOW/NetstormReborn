# webpj Dude 생성·최초 Pop 후처리(RawDudeSpawn) 이식 — WB04-E206

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "Dude 생성" 묶음(RawDudeSpawn → RawDudeNotice → RawDudeDrop)의 첫 부분이다. Dude 생성(00450a20)과 최초 Pop 후처리(00450cd0)를 옮겼다. 원본 144행은 실제 보호 Dude 자료 위에 Factory·Owner·Pop/Unpop·표시·Frame·공통 후처리를 모두 조립해 재생했다.

- cpppj 기준선 `1f00a25a`. 소스·시험·fixture 모두 기준선 뒤 변경 없음. 근거 커밋: 소스·시험 `654ee45a`, fixture `860030e7`.
- 원본 fixture: `dudespawn-x86.tsv` originals 144행(SHA-256 `c365fb07…`). originalCD 144행은 범위 밖이다.
- 공유 장치(모두 기존 이식분):
  - WB04-A `sid-pool.js`, WB04-B `squid-hash.js`
  - WB04-E2 `squid-postpop-state.js`, D 단계 `squid-postpop.js`(`setBody('dude', …)` 분배)
  - `squid-factory.js`·`squid-owner.js`·`squid-pop.js`·`squid-unpop.js`
  - E59 `squid-display.js`·`squid-frame.js`·`squid-frame-binding.js`·`render/dirty-regions.js`
  - 시험 장치 `tests/player-scene-support.mjs`(cpppj DudeSpawnSupport Assets 대응)·`tests/capture-assets-support.mjs`(정의의 maxHitPoints·SHP 헤더)

## 1. 옮긴 것

`www/core/raw-dude-spawn.js`(DOM 없음, 10.78 전용)

- `createDudeSpawnState(population, { blocked, dudeType })`: 생성 한도와 최초 Pop 등록이 공유하는 현재 상태다(cpppj `DudeSpawnState`). population 은 `SquidPostPopList`, 차단은 0055a4e8, dudeType 은 00541170(기본 70)이다.
- `RawDudeSpawn(pool, factory, owner, state, { pop }).spawn(x, y, owner)`(00450a20)
  1. 현재 개수 → 현재 용량 → 차단 전역 순서로 읽는다. signed 개수 ≥ 용량이거나 차단이면 0 이다. TYPE·좌표·배열은 미리 검사하지 않는다.
  2. 실제 `SquidFactory.create(dudeType, 2)` 로 만든다.
  3. 물리 슬롯의 프레임 DWORD(+36)를 0 으로 쓴다.
  4. `SquidOwner.set` 으로 소유자를 지정한다.
  5. `pop(sid, x, y, 0)`(실제 가상 +90)을 부르고 새 SID 를 돌려준다. Pop 미연결은 앞 효과 뒤 그 위치에서 거부한다.
- `RawDudePostPop(pool, frames, state, hooks, bridgeType)`(00450cd0)
  - `handles(sid)`: 실제 10.78 Dude 가상 표 `0x508cc8` 만 본다. 현재 TYPE·할당 상태는 관문이 아니다.
  - `postPop(sid, flags)`: 최초 flags&1 일 때 다음 순서로 실행한다.
    1. 물리 슬롯 +12 WORD 에 idle 작업 0x5101 을 쓴다.
    2. 현재 TYPE BYTE 가 현재 Bridge DWORD 전체(005411a0 getter, 기본 82)와 같으면 `notifySurface`(004214a0)를 부른다.
    3. signed 개수 < 용량이면 그 개수 위치 배열에 SID 를 저장하고, 그때의 현재 개수 + 1 을 쓴다. 음수 위치와 물리 배열 범위는 저장 위치에서 거부한다.
    4. 현재 TYPE 의 'I' 첫 프레임(없으면 −1)을 `setFrame(sid, first, 0)` 으로 쓴다.
    - flags 와 무관하게 마지막에 `base(sid, flags)`(004b0d30)를 부른다. 앞선 효과는 하위 실패에도 되돌리지 않는다.
  - `validate(sid, flags)`: 원문 호출에 끼우지 않는 명시 사전 검사다(할당·VT·TYPE·목록·I 프레임·Frame/Base 연결).
- `makeDudePostPopHooks(pool, frame, base)`: setFrame/validateBase/base 를 같은 풀의 `SquidFrame.set`·`SquidPostPop.validateBase/postPopBase` 에 잇는다.
- 웹 관례에 따라 cpppj 의 살아 있는 `const uint32_t&` Bridge 번호는 getter 로 받고, 판본 인자와 CD 10.72 분기(BYTE 프레임·유한 좌표 사전 검사·구성 때 공통 몸체 요구)는 옮기지 않았다.

## 2. 검증

`tests/dude-spawn.test.mjs`(20건 = cpppj 10.78 재생 1 + DudeParentCurrent 계약 18 + 웹 계약 1)

- 원본 144행: cpppj SpawnScene 과 같은 장면을 행마다 조립한다.
  - 장면 부품: SidPool(32768, server)·SquidHash·spot 65536·SquidPostPopState·DirtyRegions(1024×768) sink 의 SquidDisplay·SquidUnpop·SquidPostPop·SquidPop·SquidFactory·SquidOwner·SquidFrame
  - 이 단위의 RawDudePostPop 을 `setBody('dude')` 로 등록한다.
  - 준비 입력: base 74 를 SID 5 로 만들고, population stale 꼬리와 Totalmade 세 표 seed 를 채우고, 로컬 소유자를 1 로 둔다.
  - 행 입력: 용량·개수·차단·좌표 float 비트·소유자·최초 후처리(0x50) 여부.
  - 출력 17칸을 대조한다: 새 SID·population 전체·슬롯 50바이트·해시 네 단계 Adler·spot Adler·비용·생산 dirty·깊이·Totalmade 세 표·소유자별 공장 수·표시 억제·변경 사각형.
- 자료: TYPE 레코드는 `realPlayerAssets` 에 정의의 maxHitPoints(Dude 20)를 합친다. base 74 는 cpppj 처럼 빈 합성 레코드로 둔다. 시험 머리에서 Dude 의 발자국 0·HP 20·I 첫 프레임 64·65클러스터를 확인한다.
- DudeParentCurrent 18건은 cpppj 손검사를 1:1 로 옮겼다. 다만 NaN 좌표의 페이로드 비트(0x7fc12345)는 JS 수로 보존되지 않으므로 Pop 소비자에서 NaN 여부만 확인한다.

최초 실패와 교정: 없다. 모듈과 시험 모두 첫 실행에 통과했다. 비교가 실제로 작동하는지 보려고 다음 변형을 각각 돌렸다.

- idle 작업 번호(0x5101→0x5102)·첫 프레임 글자(I→A): 144행 대조가 실패했다.
- 프레임 DWORD 0 쓰기를 하위 2바이트로 줄이기: 통과했다. Factory 생성 직후 그 바이트가 이미 0 이라 관측할 수 없는 차이다.
- 확인 뒤 모두 되돌렸다.

## 3. 경계와 남은 것

- 실제 World 연결은 순서4~5 몫이다(PORT_MAP session 0): Player 의 장기 population 배열 binding·차단 전역·현재 Bridge DWORD·`notifySurface`(004214a0) 실제 표면 통지·Dude 생성 호출자(Dude 생성 사건/AI).
- 실제 가상 +90 Pop 은 호출자가 hooks.pop 으로 공급한다. 시험은 cpppj 처럼 `SquidDisplay.frameSize` + `SquidPop.pop` 으로 잇는다.

## 4. 다음 시작점

WB04-E207 은 같은 묶음의 RawDudeNotice(`DudeNoticeTests`, `dudenotice-x86.tsv`) → 그다음 RawDudeDrop(`DudeDropTests`, `dudedrop-x86.tsv`)이다.
