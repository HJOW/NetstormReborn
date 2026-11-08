# webpj AI 직접 명령·연소 피해 대상 앞단·일반 Dude task(RawAiCommandState·RawBurningVictim·RawDudeTask) 이식 — WB04-E76

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일흔여섯 번째 부분이다. AI 본체를 직접 쓰는 두 명령(`o/RawAiCommandState`, 00413c60/004143b0), 연소 세 부모의 공통 앞단(`o/RawBurningVictim`, 423d50·424110·424530 입구)과 일반 Dude task 설정(`o/RawDudeTask`, 00450670)을 cpppj 기준선 `1f00a25a` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기와 10.37 행, x87 확장 정밀도 관측은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-ai-command-state.js` — `RawAiCommandState(hooks = { players(), body(token) })`

- `crankAiMoney()`(00413c60): 중립 0 을 방문하지 않고 owner 1..8 의 Player9 표(180바이트×9) +0xb0 AI 본체 포인터를 **방문마다 새로** 읽어 비영이면 실제 0xb08 본체의 +0x2d8 에 30000 을 쓴다(앞선 본체 해석이 후속 Player 를 바꾸면 다음 방문이 현재 값을 읽고, 실패하면 거기서 멈춘다). 지갑 Credit/데모 하한/분산 SP 는 읽지 않는다.
- `setAiParameter(packed)`(004143b0): owner 는 일반 1..8 이 아니라 packed 하위 3비트(0 이면 즉시 반환), 선택자는 비트 4..11 의 8비트(4 초과는 반환), 값은 signed 상위 16비트이다. 선택자 0 은 float 값을 +0x220 과 +0x600 에 쓰며 **두 쓰기마다 Player+0xb0 을 다시 읽는다**(첫 brain 하나로 합치지 않음). 선택자 1 은 float 을 +0x228, 2·3·4 는 정수 DWORD 를 +0x230/+0x234/+0x24c 에 쓴다(첫 brain 사용).
- 선택하지 않은 하위 경계(owner 0·본체 없음·미지원 선택자)는 요구하지 않고, 도달한 Player9 표 길이(1620)와 본체 길이(0xb08)가 어긋나거나 재조회한 본체가 NULL 이면 성공으로 바꾸지 않고 거부한다. 한 DWORD 외의 본체 바이트는 보존한다.

`www/core/raw-burning-victim.js` — `runBurningVictimParent(current, entry, checking, hooks)`·`makeBurningVictimHooks(pool, types, state)`·`BurningVictimEntry`

- `current`/`checking` 은 호출마다 현재 값을 읽는 getter 이다(진단 callback 이 값을 바꾸면 다음 읽기가 본다). checking 이 비영이면 첫 robust 실패 때 `isSidRobust(burningSid)` 진단(.\Burning.cpp, 캡처 0x6f/연소점 0x91/프레임 0x11f)을 올리고 정상 복귀하면 계속한다. **두 번째 robust 는 생략하지 않으며**, 실패하면 `null`(조기 반환), 통과하면 그 시점의 현재 번호를 고정한다.
- 고정한 슬롯으로 캡처만 가상 표 비영(0x77), 세 부모 모두 HP 보유(캡처 `s.hasHitPoints()` 0x7f / 연소점 `burningVictim.hasHitPoints()` 0x98 / 프레임 `s.hasHitPoints()` 0x126)를 확인하고 실패하면 진단 후 정상 복귀 시 고정 번호를 반환한다. 이후 캡처/HP 계산/입자 이동 몸체는 호출자가 이어서 실행한다.
- `makeBurningVictimHooks`: 같은 풀의 슬롯에서 가상 표 DWORD 비영 여부, HP 보유(Dais 형 TYPE 은 extra(+40) bit0, 일반은 현재 flags1 bit4)를 읽고, 독립 robust 는 번호 판정(`isSidRobust`)만 쓰며 checking 이 비영이면 미연결 상세 장치를 선택 지점에서 거부한다. 독립 진단은 빈 성공 없이 오류이다.

`www/core/raw-dude-task.js` — `RawDudeTask(pool, { clock, bridgeType = 82 }, hooks)`·`makeDudeTaskHostHooks(pool, host, hooks)`·`DudeTaskFallAssertError`

- `set(sid, event, delay)`(00450670): 슬롯과 예약 머리/free SID 쓰기 계약을 확인하고 float 지연·현재 시각의 유한성을 확인한 뒤, 이전 task WORD(+12)가 낙하(0x5105)이고 새 event 가 다르면 원본 진단(`DudeTaskFallAssertError`)을 올린다. 그 뒤 현재 Regular(마스크 fff0/값 5100)를 조회하고 없으면 실제 생성(payload 5·flags 0x50)을 요구하며, **같은 Regular** 를 event/count 0/`현재 시각 + float 지연(double 합)` 으로 갱신하고 task WORD 에 event 하위 16비트를 쓴 뒤 현재 TYPE 이 다리(82)이면 실제 표면 통지를 요구한다.
- `makeDudeTaskHostHooks`: 같은 Host 의 `findEventMasked`/`addRegular` 에 연결하며 풀이 다르면 거부한다.

## 2. 검증

- `tests/ai-command-state.test.mjs`(3건): `ai-command-state-x86.tsv` 1940행(Money 256·Param 1684) 전부 — AI 부착 마스크와 재조회 변경 조합에서 아홉 본체의 7DWORD 와 나머지 모든 0xb08 바이트·Player 포인터가 같고, 본체에 도달한 DWORD 쓰기 순서(`소유자:위치:폭:값`)도 같다. 물리 폭 어긋남·미연결·후속 Player 변경·실패 중단 계약.
- `tests/burning-victim.test.mjs`(3건): `burning-victim-parent-x86.tsv` 504행(세 부모, robust 결과 마스크·가상 표·HP·진단 복귀·변이 조합)의 결과·checking·현재 SID·선택 SID·호출 순서가 같고, 미선택 장치 없이 조기 반환·checking 상위 비트 보존, `makeBurningVictimHooks` 의 풀 연결을 새로 확인한다.
- `tests/dude-task.test.mjs`(3건): `dudetask-x86.tsv` 10.78 384행(실제 `SquidProcessHost`/`Kernel` 의 기존 count7·time27 Regular 와 새 생성 포함)의 반환/원본 assert 분류·호출 순서·슬롯 50바이트·최종 Regular(시각 비트·event·payload 비트·count)가 같고, 미연결 생성/표면 통지 거부와 판본·풀·비유한 입력 거부를 확인한다.

최초 실패와 교정: 새 모듈은 위 fixture 2828행과 계약 검사를 모두 첫 실행에 통과했다. 새로 쓴 `makeBurningVictimHooks` 시험에서 `isSidRobust` 가 live 판별 앞에서 번호 범위(1..65534)를 먼저 검사하는 순서를 잘못 기대한 것은 시험 기대를 교정했다(제품 변경 없음).

## 3. 경계와 남은 것

- AI 본체의 실제 생성/해제(`RawAiBrain`)와 Player9 표의 실제 연결은 해당 파일 이식 때 연결한다. 연소 세 부모의 이후 캡처/HP 계산/입자 이동 몸체(`RawBurningFrame`·`RawBurningSources` 등)도 아직 옮기지 않았다.
- `RawDudeDrop`(실제 드롭 두 갈래)은 `RawDudeSpawn`·`RawDudeNotice` 가 선행이라 다음 이후로 남긴다.

## 4. 다음 시작점

`RawFenceRebuild`와 Fence 생성 부모, `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, 그 밖의 작은 몸체(`RawMetaZoneCleanup`·`RawColorSortStartup`·`RawAmbientSound` 등)이다.
