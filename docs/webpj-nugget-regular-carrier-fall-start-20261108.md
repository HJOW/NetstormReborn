# webpj 너겟 Regular·Carrier task·Carrier 낙하 시작(RawNuggetRegular·RawCarrierTask·RawCarrierFallStart) 이식 — WB04-E72

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일흔두 번째 부분이다. 실제 Regular/Kernel 위에서 도는 두 몸체 — 너겟의 왕복 프레임·지면 검사·낙하(`o/RawNuggetRegular`), Carrier 의 task 조회/생성/낙하 유지(`o/RawCarrierTask`)와 낙하 시작(`o/RawCarrierFallStart`) — 를 cpppj 기준선 `1f00a25a` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기와 10.37 행, x87 확장 정밀도(64비트) 관측은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-nugget-regular.js` — `RawNuggetRegular(pool, aFrameCounts, spots, boss, hooks)`

- `handles(sid)`: 현재 가상 표 기록값이 0x50dc30(너겟)이면 이 분배 대상이다(현재 TYPE/할당 상태를 새 선행 조건으로 쓰지 않음).
- `handle(sid, 사건, 개수, payload)`: 개수 0 이면 payload 그대로. 사건 0/1 은 감소/증가 왕복 프레임이다(끝점이면 예약 확보 성공 때만 게임 난수 2..7 의 지연으로 후속 예약을 만들고 0, 아니면 프레임을 ±1 지정하고 증가일 때만 추가 갱신 후 0.1초). 사건 2 는 소멸(flags 0x100000) 후 -1. 사건 3 은 현재 좌표(0.9999 를 더한 `_ftol64` 하위 DWORD, 감긴 지도 색인)의 spot & 6 이 없을 때 낙하이다(죽은 객체면 0, 개수 100 초과면 권한자만 삭제 후 -1, 개수 10 에서 +35 BYTE 를 20 으로 쓰고, 가속 낙하 y 를 254 로 제한해 Unpop → Pop(flags 0x40) 후 0.05초). 그 밖은 마지막 Damageable 처리기에 넘긴다. `canFall(sid)` 는 같은 지면 검사이며 지도 밖 색인은 거부한다.

`www/core/raw-carrier-task.js` — `RawCarrierTask(pool, now, hooks)`, `CarrierTaskAllocationError`

- `set(부모, task, 지연)`(426db0): 부모의 현재 Regular 를 조회 → 없으면 new(0x28) 확보(실패는 원본의 NULL 쓰기 경로이므로 `CarrierTaskAllocationError`) → 확보한 메모리에 생성자(task 가 5105 가 아니면 공유 형)를 실행하고 다음 기한만 쓴다(생성/부착 효과가 바꾼 event/count 는 보존). 있으면 기존 event 가 5105(낙하)일 때 새 task 가 다르면 원문 진단(Carrier.cpp 407) 뒤에도 낙하를 유지하고, 모든 경우 event 를 정한 값으로, count 를 0 으로, 기한을 `현재 시각 + 지연` 으로 쓴다(payload 는 보존). 기한 계산은 x87 의 FLD(float)/FADD(double) 와 같은 NaN payload/부호 선택과 quiet 변환을 유지하며 현재 시각은 생성/진단 콜백 뒤에 읽는다.

`www/core/raw-carrier-fall-start.js` — `RawCarrierFallStart(pool, players, boss, priestType, hooks)`

- `begin(부모)`(4287d0): 이미 5105 낙하 Regular 가 있으면 즉시 참. 권한자는 현재 이동 프로세스 종류를 조회해 이동을 제거하고, extra 의 선택 비트(4)가 켜져 있으면 선택을 해제한다. 포획 사제 Form 이 있으면 부모 좌표를 거의 올림(0.99999, 41d7a0)해 그 Form 의 이전 소유자(원래 소유자 하위 7비트)의 Player 기록 +0xa8/+0xac 에 쓰고 권한자는 사제를 해방한다. 마지막으로 5105 낙하 task 를 지연 0 으로 예약하고 참을 반환한다. 이전 소유자의 Player 기록 범위가 물리 크기를 넘으면 쓰기 전에 거부한다.

## 2. 검증

- `tests/nugget-regular.test.mjs`(3건): `nugget-regular-x86.tsv` 9019행(반환 float 비트, 호출 순서 F/R/X/A/I/C/D/U/P/Y/B, 슬롯 50 raw, 게임 난수 상태; 기본 CRT 53비트 열), 초기 count 와 정상 분배·명시 늦은 효과 누락 구별, 정확한 가상 표만 분배와 현재 TYPE 의 A 구간·연결 오류 거부.
- `tests/carrier-fall-start.test.mjs`(2건): `carrier-fall-start-x86.tsv` 2850행(task 설정/낙하 시작의 하위 호출 순서 Q/A/C/!/H/M/D/S/K/R, 프로세스 event/count/payload/기한/현재 시각/권한/사제 종류/Player 기록 Adler-32, 부모/Form raw; 확보 실패 480행 포함; NaN 지연/시각 payload 선택 포함), 이미 낙하 중이면 다른 경계 없이 반환·미연결 task 하위 거부·물리 Player 범위를 쓰기 전에 거부.

최초 실패와 교정: 너겟은 첫 실행에 통과했다. 낙하 시작은 한 행(진단 변이 7)에서 원본이 "같은 주소에 프로세스를 다시 생성"하는 것을 시험이 새 객체로 대체해 어긋났다(호출자가 쥔 포인터가 최신 객체를 가리켜야 함). 시험의 프로세스 훅이 최신 객체에 위임하는 안정 뷰를 돌려주도록 고쳤고 제품 코드는 바꾸지 않았다.

## 3. 경계와 남은 것

- 웜 40 경로 생성자와 가상 표 시험(`CarrierFallStart_WormNativeConstructorAndVtables`)은 `RawPathRuntime` 이식 때 대조한다. 너겟의 Damageable 처리기, 공간(Unpop/Pop)·프레임 지정·삭제의 실제 월드 연결, 사제 해방·이동 제거·선택 해제 본체도 해당 파일 단위의 몫이다.

## 4. 다음 시작점

`RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, 그 밖의 작은 몸체(`RawAiCommandState`·`RawBurningVictim`·`RawGeyserLifetime` 등)이다.
