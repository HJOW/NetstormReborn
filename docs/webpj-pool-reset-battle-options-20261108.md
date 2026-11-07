# webpj 월드 Pool 정리 부모·미션 전투 옵션 이식 — WB04-E37

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 서른일곱 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawSquidPoolReset`과 `o/RawMissionBattleOptions`(10.78)를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-squid-pool-reset.js` — 월드 Pool 정리 부모(004af170)

- `resetSquidWorldPool(pool, invalid, capacity = 0)`: 이미 옮긴 `SidPool.reset(capacity)`를 재사용하고, 하위가 정상 반환한 **뒤에만** 삭제 기록과 실패점을 정리한다. 하위가 실패하면 기록·실패점의 뒤 효과를 실행하지 않는다.
- `finishSquidPoolReset(pool, invalid)`: 삭제 기록 80DWORD(client/other 각 20쌍) 비우기 → 실패점 **y 다음 x**를 −10000.0f(`0xc61c4000`)로 쓴다. 실패점은 손실 위치·미사일 이전점 등 다른 −10000 필드와 별개이다.

`www/core/raw-mission-battle-options.js` — 미션 전투 옵션

- `resetMissionBattleOptions(options)` [0041cb90/0041cc10]: 87바이트의 BYTE 0..22와 63..86을 0으로 쓰고(23..62와 양쪽 이웃은 보존) BYTE 71..78에 색 0..7, BYTE 0..21에 22개 기본 선택을 설치. 크기가 87이 아니면 쓰기 전에 `TypeError`.
- `RawMissionBattleOptions`: 옵션 사본 없이 매 호출마다 현재 저장소를 읽는다. float는 NaN 페이로드를 보존하려고 **비트**로 보관한다(`rangeTable`은 `Uint32Array`, `range`/`rangeSquared`는 `{bits}`). `updateRange()` [004b4860]는 x87 FLD/FSTP32처럼 signaling NaN의 quiet 비트만 켜 저장하고 `min(표[2], 현재 범위)` 후보보다 표[0]이 크면 표[0], 아니면 표[2]가 현재 범위보다 작으면 표[2]로 바꾼 뒤 제곱(NaN은 피연산자 NaN 보존, `clamp`/`min`/`max`로 바꾸지 않음). `typeCost()`(1000/2000/3000/5000), `currentValue()`(0/25/50/75/100/150), `minimumDensity()`/`maximumDensity()`(10..25 / 30..45, 서로 다른 현재 선택), `initializeDensity()`(State7: 두 한계 쓰기 → 반값 범위 난수 두 번 → DWORD 합계), `setSelectedTypeCost()`, `applyDerived()`(범위 → 비용 → 현재 TYPE 쓰기 → 현재 값), `clearTypes()`(현재 256바이트만 0, 이웃 보존). 표 밖 인덱스는 원본의 스택/인접 전역 읽기라 `RangeError`로 구별.

## 2. 검증

`tests/squid-pool-reset.test.mjs`(3건)와 `tests/mission-battle-options.test.mjs`(4건). 원본 10.78 관측은 `cpppj/tests/fixtures`를 직접 읽는다.

- `squid-pool-reset-x86.tsv` 12행 중 10.78(`originals`) 6행(`originalCD` 6행은 범위 밖): 부모 호출 전 양쪽 삭제 기록 hex, 호출 뒤 free/cursor/목록 머리·꼬리/전체 raw Pool Adler-32/삭제 기록 Adler-32/삭제 기록 hex/실패점 Y·X 비트. 0과 음수 인자, 두 x87 정밀도 기대가 같음. 계약: 하위 reset은 기록을 보존하고 부모는 지운 뒤 재확보(`capacity 40000`/`freeCount 39995`/`cursor 1`), 하위 실패(용량 5)는 모든 상태와 실패점을 보존하고 기록 정리만 실행하면 다른 Pool 필드를 보존.
- `mission-option-reset-x86.tsv` **296행**(초기화 변형 F/P: 이웃/예약 보존·네 번째 float·실제 파생값), `mission-battle-options-x86.tsv` **3305행**(x87 특수 float와 0x67 부모의 288 조합: Range/Cost/Value/Clear/Apply의 범위·값·TYPE 비용 188개·사용표·옵션 바이트, 입력 중복 없음). 계약: 현재 옵션/선택 TYPE/범위표 변경을 사본이 가리지 않음, signaling NaN 두 종(`0x7f800001`/`0xff800001`)의 quiet 변환, 잘못된 크기/표 밖 인덱스/188 이상 TYPE 거부, 밀도 난수의 반값 범위와 합계.

최초 실패와 교정: 7건이 처음부터 통과했다.

## 3. 경계

- 실제 Client의 대여/외부 권한 거부(`ResetClientSquidPool`)와 Board/Dais 실패점 재조회(`SharedBoardAndDaisPointRequeryAfterAssertion`)는 해당 소비자 단위에서 대조한다. 이 단위의 `invalid`는 호출자가 소유한 `{x, y}` 객체이다.
- 옵션 수신/로그인/TYPE 소비자와 AI·지형의 밀도 연결, 미션 시작의 옵션 초기화(`TutorialInit`의 `resetOptions`)는 해당 단위에서 한다. CD 10.72는 범위 밖이다.

## 4. 다음 시작점

`TutorialInit`(`resetOptions` 훅으로 이제 `resetMissionBattleOptions` 사용 가능, 152행)·`TraceChat`·`HtmlInputDialog`·`DialogCommandLifetime`·`LanguageOptions`. 큰 묶음은 `RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawBridgeConnect`, Player 나머지이다.
