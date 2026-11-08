# webpj AI 피해(RawAiDamage)·생성/삭제 통지(RawAiObjectNotice)·월드 등록(RawAiWorldRegistration) 이식 — WB04-E180

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여든 번째 부분이다. AI 피해 반응의 목록 추가와 로그 순서를 맡는 `o/RawAiDamage`(패치 `00415460`·`00414450`·`00411200`), 생성/삭제 통지를 같은 Player→AI 본체/목록에 반영하는 `o/RawAiObjectNotice`(패치 `004166e0` 바깥 몸체·`00414de0` 본문·`00410f80` 간헐천 제외·`00410620` 건설 통지), 기존 월드와 간헐천을 같은 본체 목록에 등록하는 `o/RawAiWorldRegistration`(패치 `00415720` 초기 전체 스캔·`004153c0` 간헐천 등록/제거·`004152b0` 동맹보다 가까운지·`00411000` 표면 분류)을 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 세 몸체와 세 시험·`ai-damage-x86.tsv` 의 변경 없음을 `git diff --numstat` 으로 확인). 피해만 독립 PE 관측 fixture(149행)가 있고 나머지 둘은 cpppj 계약 시험이다.

## 1. 옮긴 것

`www/core/raw-ai-damage.js`(DOM 없음)

- `new RawAiDamage(memory, pool, types, hooks)` — `types` 는 현재 188 TYPE 의 `{ flags2, group }`, `hooks = { home(attacker)→SID(0045c100), log(token, { more, attacker, type, x, y })(004105b0) }` 는 도달한 위치에서만 요구한다.
- `notice(token, victim, attacker)` — 공격자가 0 이면 끝내고, 피해 대상 genus(TYPE 164 는 0x400000 추가)와 네 현재 목록(+0x360 방어·+0x36c 미움·+0x378 매우 미움·+0x384 추적)으로 두 단계 반응을 수행한다. 추적 목록의 대상이거나 genus 0x80604200 이면 미움/매우 미움이 되고(0x80404200 이고 공격자가 매우 미움 목록에 없으면 +0x404 에 능력 비트 1 기록), 방어 genus 가 방어 목록에 있으면 미움, 추적 genus 가 추적 목록에 있거나 미움이면 공격자를 미움 목록에 등록하고 비행(0x100000) 공격자는 같은 홈(group 3, free 아님, dead 는 거부하지 않음)도 등록하며, 매우 미움이고 공격자가 매우 미움 목록에 없으면 추가(비행 홈 포함) 뒤 "I hate MORE" 로그를 요청한다. 미움 등록은 0/중복에서만 반환하고 목록이 가득 차도 로그는 호출한다. 목록 추가는 00414450 처럼 가득 차면 검색도 하지 않는다.

`www/core/raw-ai-object-notice.js`

- `new RawAiObjectNotice(memory, pool, types, players, now, outpostType, disabled, hooks)` — `players` 는 9×180 BYTE Player 기록(+0xb0 이 AI 본체 토큰), `now()`/`outpostType()`/`disabled()` 는 각각 0055b4d0·00541250·00594fc8 의 현재 값 getter, `hooks = { carrierTargets, distanceSquared, log }`. 로그 종류는 `AI_OBJECT_LOG { built, madeGeyserIneligible, cannotExcludeGeyser }` 이다.
- `notice(owner, sid, added)` — disabled 이거나 owner 가 signed 비양수이면 아무것도 읽지 않고 끝내고 owner 가 9 이상이면 거부하며, 현재 Player+0xb0 이 0 이 아닐 때만 `update` 를 호출한다.
- `update(token, sid, added)` — TYPE 별 개수(+0x6d4 + TYPE×4)를 갱신하고 genus 를 한 번 읽어, 삭제 시 수집/신전 포인터와 추적/미움 목록 정리·간헐천 제외·잃은 수집자 30초 지연을, 그리고 다이스(+0x2a4)·방어(+0x360, 근접 거리 180.5 미만 전체 방문 뒤 추가)·수집자(+0x2dc, 건설 통지)·워크샵(+0x2f4)·시설(+0x300/+0x30c/+0x318, 삭제 시 상태 초기화와 대기 목록 재예약)·공략 목록(+0x2e8, 가득 차면 첫 항목 제거 뒤 추가, +0x3c8 에 3 가산)·타이머(+0x5b8/+0x5c0)를 원본 순서대로 갱신한다. 각 하위 효과 뒤에는 현재 본체/타입/목록을 다시 읽고 타이머는 float 로 축소하지 않고 QWORD 로 쓴다.

`www/core/raw-ai-world-registration.js`

- `new RawAiWorldRegistration(memory, pool, types, players, allies, surface, objects, hooks)` — `allies` 는 방향별 81 DWORD, `surface` 는 같은 hash0 의 65536 WORD(`Uint16Array`), `objects` 는 같은 풀의 `RawAiObjectNotice`, `hooks = { center(sid)→[x, y], fartherGeyser(token, sid), missingSurface(sid) }`.
- `fullScan(token)` — SID 2 부터(SID 1 은 방문하지 않음) free 가 아닌 항목(dead 는 제외하지 않음)을 방문해 TYPE 70 이상·genus 2 가 아닌 것 중 간헐천은 `geyser(…, 1)` 로 등록하고, 본체 owner 와 같은 owner 의 비방어 자산은 즉시, 방어 자산은 임시 300개 목록(1200바이트 할당, 실패해도 반납)에 지연했다가 SID 순서로 통지한 뒤 +0x3c8 을 3 으로 쓴다.
- `geyser(token, sid, state)` — state 0 은 좌표/TYPE 를 읽지 않고 다섯 목록(+0x324/+0x330/+0x33c/+0x348/+0x354)의 모든 중복을 제거하고, 그 밖은 동맹이 더 가까우면 로그만, 아니면 +0x324 에 추가한 뒤 현재 표면(y → x 절삭, 0..255 밖은 표면 0)으로 분류한다(자기 소유 또는 owner 0 의 지면이면 +0x330, 섬/기타는 +0x348). `closerToAlly(token, sid)` 는 Player 0 을 방문하지 않고 현재 owner 행의 방향 동맹과 현재 Player+0x7c 신전 거리가 자신의 거리(float 로 저장, 동률이면 자신의 후보 유지)보다 엄격히 작으면 true 이다.

## 2. 검증

`tests/ai-damage.test.mjs`(3건)·`tests/ai-object-notice.test.mjs`(5건)·`tests/ai-world-registration.test.mjs`(5건).

- `ai-damage-x86.tsv` 149행(원본 `00415460`): 각 행은 독립 입력 17개로 실제 memory/Pool 에 입력 바이트만 설치하고 `notice` 를 한 번 호출해 호출 뒤 전체 본체 0xb08 바이트, 네 목록 backing 블록 전체(미사용 꼬리 포함), 홈/로그 요청 순서, 입력 슬롯 전체 폭의 보존, 할당 5개/해제 없음을 모두 대조한다. 미연결 효과의 요청 시점 거부와 앞선 목록 효과 보존, 가짜 토큰 거부, 공격자 0 의 무효과, 생성자 거부는 cpppj 계약이다.
- 통지(cpppj 계약, 독립 fixture 없음): 비활성/signed 비양수 owner/NULL 이 본체를 선택하지 않고 다음 호출이 현재 Player 포인터를 읽음, 복합 genus 생성의 중복 정책/거리 전체 방문/타이머 정밀도, 삭제의 중복 압축·가득 찬 제외 목록·시설 재예약과 inactive 꼬리 보존, 사제와 현재 outpost 제외 번호 분기.
- 월드 등록(cpppj 계약, 독립 fixture 없음): 전체 스캔이 기존 시설/수집자를 먼저·방어 자산을 나중에 등록하며 SID 1 을 건너뛰고 free 와 dead 를 구별, 임시 목록 300개 용량과 예외 시 임시 1200바이트 반납, 방향 동맹·현재 신전·동일 거리 유지와 자신의 거리만 float 로 저장하는 경계, 표면 소유·섬 마스크·범위 밖 진단과 제거의 좌표 비조회/모든 중복 압축. cpppj 의 네 번째 시험(`RawGeyserAiRegistration` 의 다음 Player 재조회)은 `raw-geyser-ai-registration.js`(WB04-E78)의 자체 시험이 이미 검사하므로 옮기지 않았다.

최초 실패와 교정: 새 모듈과 시험 13건은 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 홈 조회(`0045c100`)·로그(`004105b0`)·이동 프로세스 대상(`00426150`)·거리(`00411120`)·신전 중심(`004adea0`)·표면 진단(`00411000`) 하위 연결과 `RawGeyserAiRegistration` 에 `geyser` 를 `bindPatch` 로 묶는 세션 조립은 후속 AI 세션 연결 단위이다. 세 몸체는 모두 hook 경계로만 연결되어 있다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiCarrierActions`·`RawAiPreparation`·`RawAiStrategySetup` 등 AI 계열 나머지(`client/RawAiDebugMenu` 포함), `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
