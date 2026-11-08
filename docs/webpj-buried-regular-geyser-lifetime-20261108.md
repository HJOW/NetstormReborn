# webpj 오벨리스크 Regular·간헐천 수명(RawBuriedRegular·RawGeyserLifetime) 이식 — WB04-E75

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일흔다섯 번째 부분이다. 오벨리스크(buried)의 실제 Regular Handle(`o/RawBuriedRegular`, 004622f0)과 간헐천의 고유 수명 전·후처리(`o/RawGeyserLifetime`, 485580/485540)를 cpppj 기준선 `1f00a25a` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기와 10.37 행, x87 확장 정밀도 관측은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-buried-regular.js` — `RawBuriedRegular(pool, frames, setter, random)`

- `handles(sid)`: 부모의 가상 표 기록값이 0x50a748(오벨리스크)일 때만 분배 대상이다(TYPE 번호가 같은 다른 부모를 분배하지 않음).
- `handle(sid, 사건, 개수, payload)`: 사건 0 만 애니메이션/지연을 바꾼다. 현재 TYPE 의 B 구간 수+1 로 signed 개수의 나머지를 구해(0 나눗셈·최솟값÷−1 거부), 0 이 아니면 B 구간 첫 프레임 + 나머지 − 1 을 실제 `SquidFrame.set` 으로 지정하고 0.1초를 반환하고, 0 이면 A 구간 첫 프레임을 지정하고 게임 난수(6)+2(2..7)초를 반환한다. 다른 사건은 payload 를 그대로 반환한다.
- `handler(fallback)`: 다른 부모의 사건은 기존 실제 처리기로 전달하고, 없으면 거부한다.

`www/core/raw-geyser-lifetime.js` — `RawGeyserLifetime(pool, geysers, editor, hooks)`

- `handles(sid)`: 부모의 가상 표 기록값이 0x50da88(간헐천)인 경우만 허용한다.
- `preDestroy(sid, flags)`(485580): 가상 +78 의 현재 기본 프레임을 프레임 DWORD(+36)에 직접 쓴 뒤(일반 SetFrame 의 추가 Unpop/두 표시 갱신 없음) 표시(+88)를 한 번 부르고, 효과 뒤 현재 extra 의 비트 9 가 꺼져 있으면 Player AI 제거(416720)를 요청하고 지면 목록에서 이 SID 의 모든 중복을 제거(`SquidPostPopList.removeAll`; 비활성 물리 꼬리 보존)한 뒤 Damageable 전처리(44b4b0)를 같은 flags 로 부른다.
- `postDestroy(sid, flags)`(485540): 현재 편집 모드가 켜져 있고 extra 비트 9 가 꺼져 있으면 현재 좌표/flags/소유자로 섬 정리(46e6c0)를 한 번 부른 뒤 Damageable 후처리(44b9e0)를 같은 flags 로 부른다.

## 2. 검증

- `tests/buried-regular.test.mjs`(3건): `buried-regular-x86.tsv` 384행(사건 0 애니메이션 288·기본 사건 96의 슬롯 50 raw 전체, 반환 float 비트, 게임 난수 상태, 실제 `SquidFrame` 이 부른 표시 갱신 순서; 효과 안의 난수/WORD 변경 포함), 실제 가상 표/같은 풀/판본 검증과 다른 부모의 누락 거부.
- `tests/geyser-lifetime.test.mjs`(2건): `geyser-lifetime-x86.tsv` 2016행(preDestroy/postDestroy 의 호출 순서 F/U/I/P/T/D, 효과 뒤 `목록 개수,편집 모드`, 부모 슬롯 raw, 지면 목록 물리 꼬리 8칸 전체; 목록 패턴·extra·편집·변이 조합), 현재 가상 표만 선택하고 도달한 효과 누락·CD 혼합·편집 getter 오류 거부.

최초 실패와 교정: 새 모듈은 위 fixture 2400행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 오벨리스크의 프레임 단계 이동(저장 단계 변경)·재Pop, 간헐천의 기본 프레임 getter·표시·AI 제거·Damageable·섬 정리 본체는 해당 파일 이식 때 연결한다.

## 4. 다음 시작점

`RawFenceRebuild`와 Fence 생성 부모, `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, 그 밖의 작은 몸체(`RawAiCommandState`·`RawBurningVictim`·`RawDudeTask` 등)이다.
