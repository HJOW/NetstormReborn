# webpj AI 지갑(RawAiWallet)·AI 경제(RawAiEconomy)와 보상의 현재 고정 SP 모드 — WB04-E166

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백예순여섯 번째 부분이다. AI 본체의 지갑(+0x2d8) 가산/조회(`00413bb0`·`00413c20`)를 옮긴 `o/RawAiWallet`과, AI 전략 말미의 지원·포획 예산·데모 하한·분산 SP 쓰기(`004138c0`)를 옮긴 `o/RawAiEconomy`, 그리고 cpppj 가 기준선 뒤(`2b048419`)에 바꾼 `SquidReward` 의 SP 고정 두 모드 현재값 읽기를 JS 보상 모듈에 맞춘 것이다. AI 계열(33개 파일) 네 번째 단위이며 E164 의 `RawAiBrainMemory` 위에서 동작한다.

기준선 `1f00a25a` 이후 변경: `RawAiWallet`(.cpp/.h)·`AiWalletTests.cpp` 와 `SquidReward`(.cpp/.h)는 `2b048419`(20261109-06 CP)에서 바뀌었다(Native 가 재할당하는 Player base 와 선택 소유자 수명, 보상의 현재 고정 SP 모드). 이 단위는 그 변경을 포함한 현재 HEAD 를 옮겼으며 PORT_MAP 기준선은 이 두 행에 `2b048419` 로 기록했다. `RawAiEconomy` 는 기준선 뒤 변경이 없다.

## 1. 옮긴 것

`www/core/raw-ai-wallet.js`(DOM 없음)

- `new RawAiWallet(source, players, reads)` — `source` 는 `RawAiBrainMemory` 또는 `acquire()`(→ `{ memory, owner }`, 모드/owner/NULL brain 의 조기 반환을 모두 지난 뒤에만 선택하고 같은 호출 안의 데모 조회 뒤 재조회까지 유지), `players` 는 실제 9×180 BYTE 기록 또는 현재 base 를 사용 위치에서만 돌려주는 함수(오래된 표를 보관하지 않음), `reads = { disabled(), demo() }` 는 각 원본 위치에서 현재 값을 읽는다.
- `credit(owner, amount)` — 비활성이거나 signed owner < 1 이면 반환, owner ≥ 9 거부, 현재 +0xb0 이 0 이면 반환. 그 밖에는 +0x2d8 에 DWORD 감김 가산, 데모이면 같은 brain 을 다시 조회해 signed 1190 이하를 1190 으로. SP/난수/화면은 갱신하지 않는다. `balance(owner)` — 같은 관문 뒤 현재 brain 의 signed 지갑 읽기만 한다. 해제/가짜/크기가 다른 본체는 지갑 성공으로 대신하지 않고 거부한다.

`www/core/raw-ai-economy.js`

- `new RawAiEconomy(memory, sp, now, hooks)` — `sp` 는 같은 분산 SP 저장소(`ScrambledSpStore`), `now()` 는 현재 QWORD 시각, `hooks = { demo(), captured(owner) }`. `reconcile(token)` — +0x5d8/+0x5dc 의 double 기한이 현재 이하이면 다음 기한(현재 + 1.0)을 쓰고 지갑이 `+0x290 + 2200`(DWORD 감김 뒤 signed) 미만이면 `+0x24c` 를 더한 뒤 데모 하한. 기한과 무관하게 `+0x264`·`+0x260` 의 포획 판정(현재 비용/지갑을 다시 읽어 `비용 + 2200` 으로 보정 후 데모 하한)과 최종 SP 쓰기(지갑 int32 → float → `+0x260` owner)를 항상 수행한다. NaN 기한은 지원만 생략한다.

`www/core/squid-reward.js`(E-D5b 에서 만든 모듈의 보정)

- SP 가산의 고정 모드를 값형 필드 대신 `currentFixedSpA()`/`currentFixedSpB()` 로 읽는다. `state.share('fixedSpA', owner, 'fort')`·`share('fixedSpB', owner, 'editor')` 로 소유자 칸(bool 또는 editor DWORD, 0 이 아니면 켜짐)을 매 가산 위치에서 현재값으로 빌리며 빌림이 없으면 기존 독립 입력(`fixedSpA/B`)을 쓴다. 기존 reward/sp-add-bits 관측 결과는 변하지 않는다(전부 독립 입력).

## 2. 검증

`tests/ai-wallet.test.mjs`(10건)·`tests/ai-economy.test.mjs`(7건). 두 몸체 모두 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다). 같은 입력과 단언을 옮겼다.

- 지갑: 조기 반환 시 소유자 미선택과 선택한 소유자의 데모 재조회 유지, 보상 부모의 SP 고정 두 모드 현재값 읽기, 조회의 쓰기 없음, 모드 우선/비양수 owner/NULL brain/손상된 물리 포인터, 비활성·NULL brain 의 모드 읽기 횟수, 8행 감김/데모 하한/나머지 바이트 보존, 데모 조회가 Player/지갑을 바꿔도 저장한 brain 의 현재 지갑 재조회, 크기 없는/해제된 포인터와 누락 조회기 거부, 보상이 같은 brain 에 정수 절삭만 전달, 현재 Player base 재할당 추적.
- 경제: 기한 경계와 단일 지원(큰 시간 점프도 한 번), 지원의 DWORD 감김과 데모 하한, 포획/데모 하위가 바꾼 owner·비용·지갑 재사용, 비용 + 2200 감김과 현재 지갑 재조회, 최종 SP 쓰기와 float 반올림이 본체 정수 지갑을 바꾸지 않음, NaN 기한/선택된 누락 하위의 도달 위치 거부와 생성자 거부.
- cpppj 지갑 시험 중 C++ 약한 참조(`weak_ptr`)로 소유자 반납을 관찰하는 부분은 JS 에 대응이 없어 "호출 중 외부 보유자를 놓아도 선택한 소유자를 끝까지 쓴다" 는 동작으로 확인했다. CD(10.72) 보상 분기는 범위 밖이다.

최초 실패와 교정: 새 모듈과 시험은 모두 첫 실행에 통과했다(기존 `squid-reward` 시험도 그대로 통과).

## 3. 경계와 남은 것

- 실제 Player 표/본체/현재 모드 조회기·현재 데모(`00463b50`)/사제 포획(`00492340`)/시각(`0055b4d0`) 연결은 후속 AI 세션 연결 단위이다. 지갑을 보상 부모에 연결하는 `reward.bindPatchWallet((owner, amount) => wallet.credit(owner, amount))` 연결 방법은 시험에 있다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiBrainInitialUpdate`(`00414710`), `RawAiMovement`·`RawAiBuildChoice`·`RawAiDropSelection` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
