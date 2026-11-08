# webpj 객체 전환·타입 변경·비로컬 의식 보상(RawObjectConversion·RawForeignRitualReward) 이식 — WB04-E106

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여섯 번째 부분이다. 비석(Buried)·주문(Spell) 객체의 Command 0x030000ce 전환(41e400/462290)과 그 아래의 타입 변경(`SquidFactory.retype`, 004b1620) 대조, 그리고 비로컬 의식 보상의 SP 덧셈(004482b0)·별도 Core 장부 기술 비트 쓰기(0048f380→0042b280)를 cpppj 기준선 `1f00a25a` 의 `o/RawObjectConversion`, `o/RawForeignRitualReward` 에서 옮겼다. 선행은 이미 이식된 `SquidFactory`(WB04-D4)와 `RawPlayerCoreRecords`(WB04-E3)이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-object-conversion.js` — `objectConversionBody`, `makeObjectConversionHooks`, `RawObjectConversion`

- `objectConversionBody(vtable)`: 실제 가상 6c 가 41e400(주문 16종 vtable 표 0x502370…0x503188)이면 0x41e400, 462290(비석 vtable 0x50a748)이면 0x462290 을 돌려주고 다른 클래스는 0 이다.
- `RawObjectConversion.command(sid, code, body)`: 명령 코드의 상위 입력 비트(0xfc000000)를 제거해 0x030000ce 가 아니면 미처리 명령으로 보고 부모(`base`)의 반환을 그대로 돌려준다. 맞으면 Unpop(flags 0) 뒤 본문별로 처리하고 Repop(flags 0) 뒤 1 을 반환한다. 비석 본문(462290): Unpop 뒤 **현재 QA WORD**(+12)로 타입 변경 → 현재 TYPE 의 기본 프레임을 프레임 DWORD(+36)에 쓴다. 주문 본문(41e400): Unpop 뒤 현재 TYPE 을 보관해 두고 비석 타입(현재 값)으로 타입 변경 → 프레임 0 → 보관한 이전 TYPE 을 QA WORD 로 쓴다. 선택하지 않은 경계는 요구하지 않고 도달한 누락은 앞선 쓰기를 보존한 채 거부한다.
- `makeObjectConversionHooks(pool, factory, hooks, assertion)`: 같은 풀의 실제 `SquidFactory.retype`·프레임 DWORD(+36)·QA WORD(+12, 하위 WORD 만) 쓰기를 hooks 에 연결한다. 타입 변경 진단은 0x22d(`isVoid()`)·0x22e(footX 불일치)·0x22f(footY 불일치)만 허용하며 `assertion(식, '\Ns\O\Squid.cpp', 행)` 이 복귀하면 전환을 계속한다. 다른 풀의 Factory·CD 판본은 거부한다.

`www/core/raw-foreign-ritual-reward.js` — `grantForeignRitualSp`, `grantForeignRitualType`

- `grantForeignRitualSp(owner, balance, set)`(004482b0): 현재 SP 조회(float)에 5000 을 더하고 기본 x87 53비트 정밀도로 절삭한 signed64 하위 DWORD(비유한/범위 밖은 0)를 SP 정책 setter 에 `set(owner 의 signed32 비트, 값)` 으로 넘긴다. 가산용 AddSp 나 AI 지갑을 호출하지 않는다.
- `grantForeignRitualType(records, owner, type, technologyBit, assertion)`(0048f380→0042b280): TYPE 의 기술 비트가 −1 이면 아무 것도 쓰지 않는다. 범위(0..31) 밖이면 진단(0x15a)을 부른 뒤 **정상 복귀하면** 계속하고, 별도 Core 장부의 owner 기록(+10 위치 비정렬 DWORD; owner 는 32비트로 감김)을 진단 뒤 현재 값으로 읽어 `1 << (bit & 31)` 만 켜서 쓴다(이웃 ID/랭크/padding 은 읽지도 쓰지도 않음). 물리 장부 끝을 넘으면 부분 BYTE 쓰기 없이 거부한다.

## 2. 검증

- `tests/object-conversion.test.mjs`(4건):
  1. `squid-retype-x86.tsv` 128행(10.78 전용). 이전/새 TYPE·상태·발자국 불일치 비트 → 타입 변경 뒤 50바이트 전체 raw·진단 행 순서(`A,557…`)·풀 free 개수 불변·다른 슬롯 보존.
  2. `object-conversion-x86.tsv` 36행. 비석/주문 Command 의 상위 비트·미처리 명령 부모 위임·Unpop 뒤 TYPE/QA 재조회 → 반환·50바이트 전체 raw·`U/P/B/A` 순서. 직접 hooks 와 실제 Session 바인더로 두 번 재생한다.
  3. 실제 Session 바인더로 타입 변경 진단 128행(void/발자국 진단 조합·생성자 없는 fallback)을 다시 재생한다.
  4. 계약: 선택하지 않은 경계 미요구·도달한 누락은 앞선 쓰기를 보존하며 거부·진단 행 범위 밖과 다른 풀·CD 판본 거부·프레임 DWORD/QA WORD 쓰기 폭.
- `tests/foreign-ritual-reward.test.mjs`(2건): `foreign-ritual-reward-x86.tsv` 의 기술 모드(mode 0) 144행(256바이트 Core 창 전체와 `T/A` 효과 순서)과 계약(SP 덧셈의 ftol 절삭·NaN/범위 밖 0·owner signed32 비트, 비트 없는 TYPE 무쓰기, 진단 뒤 현재 DWORD 읽기, 물리 끝 초과 거부, 미연결 거부).

최초 실패와 교정: 새 모듈은 위 fixture(타입 변경 128행[바인더 재생 포함 2회]·전환 36행[직접/바인더 2회]·기술 Core 144행)와 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- `RawForeignRitualReward` 의 SP 모드(mode 1) 실제 분산 SP 저장소/정책 대조 180행(`ReadPlayerSpAmount`·`SetPlayerSpAmount`·`ScrambledSpStore` 연결)은 해당 정책 이식 전이라 이번에 쓰지 않았다(PORT_MAP 은 partial). 공간 해제(unpop)/재등록(repop)/기본 프레임 조회/부모 명령은 훅 경계이다.

## 4. 다음 시작점

`RawPathProcess`(RunFrame·도착·Halt; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, `RawWorldGeyser`·`RawWorldGeyserBounds`, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 그 밖의 작은 몸체이다.
