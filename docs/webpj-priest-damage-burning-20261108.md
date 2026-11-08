# webpj 사제 피격과 Damageable burning 판정(RawPriestDamage·RawDamageableBurning) 이식 — WB04-E67

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 예순일곱 번째 부분이다. 사제 피격(원본 00494380)과 그 안에서 호출되는 Damageable burning 판정(0044c060)을 cpppj 기준선 `1f00a25a` 의 `o/RawPriestDamage`·`o/RawDamageableBurning` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(BYTE 레벨 `+8` 기준, signed WORD HP, 진단 없는 burning)와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-damageable-burning.js` — `DamageableBurningState`, `RawDamageableBurning(pool, types, hp, state, hooks)`

- `refresh(victim)`(0044c060): free 이거나 현재 TYPE 의 damageEffect 가 0 이면 어떤 조회도 하지 않고 0. 아니면 burning 프로세스를 조회하고, checking 전역(전체 DWORD 의 비영, 실제 Client 의 같은 값을 빌리는 `sharedChecking` getter)이 켜져 있고 두 번째 프로세스가 있으면 원문 진단('findProcess (mySid, burningSystemProcessType, 1 ) == 0', `\Ns\O\Damageable.cpp` 0x210)을 요구한다. 현재 HP(signed DWORD)가 TYPE 최대 HP 의 절반(`최대*2/4`, 곱셈 감김)보다 작으면 프로세스가 없을 때만 bulf 타입은 위치 소리 후 생성, 아니면 기존 프로세스가 있으면 삭제하고 0 을 반환한다. 진단이 현재 HP/TYPE/전역을 바꿔도 앞서 저장한 현재 HP·절반·찾은 프로세스는 다시 읽지 않는다. 선택한 하위 경계의 누락(소리·생성·삭제·진단)은 묵시 성공으로 대체하지 않고 이미 실행한 효과도 되돌리지 않는다.

`www/core/raw-priest-damage.js` — `PriestDamageState`, `RawPriestDamage(pool, types, frames, state, hooks, depend)`, `makePriestGunHitHooks`, `makePriestDamageWorldHooks`

- `adjust(피해, 공격자 소유자, 피해자 소유자, 상태)`(0044bfe0, 정적): 보정 차단이거나 소유자 0 이면 피해를 그대로 두고, 아니면 Challenge rank BYTE 를 24칸 표로 레벨로 바꿔(소유자 1..8 밖은 0, 표 밖 rank 는 거부) 피해자 레벨이 낮으면 `(공격자-피해자+20)*피해/20`, 높으면 `피해*20/(피해자-공격자+20)` 를 DWORD 곱셈 감김과 0 방향 나눗셈으로 계산한다(0 나눗셈·최솟값÷-1 거부).
- `hit(피해자, 피해, 공격자 소유자, 공격자)`(00494380): dead 는 원본 assert 로 거부, 무적 전역이 비영이면 피해 0 → 현재 회복 사건(0x25d)이 있거나 프레임 방향이 10..14(붙들림)이면 0x25c/0x25d 를 모두 취소하고 기본 프레임으로 → 보정 피해를 현재 HP 에서 빼서 최소 1 로 HP 지정(공간 전환은 E62 `RawPriestState.setHitPoints`) → 현재 소유자에게 통지 → burning 갱신 → 현재 권한자이고 HP 가 1 이면 의존 감시 체인을 저장된 다음 순서로 순회하며 Depend Form 을 통지하고(`DependFormHost.notify`) 살아 있으면 감시 쌍을 제거(`remove`)한 뒤 거짓을 반환한다. 효과 뒤 소유자/HP/권한/무적을 다시 읽는다.
- `makePriestGunHitHooks`: GunHit 콜백의 `damage` 를 실제 사제 표적에만 연결하고 다른 표적은 명시 fallback(없으면 거부)으로 넘기며 나머지 필드를 보존한다. `makePriestDamageWorldHooks`: 회복 사건 검색/반복 삭제를 같은 `SquidProcessHost` 에, HP 설정을 `RawPriestState.setHitPoints` 에, 표시 갱신을 `RawDamageableBurning.refresh` 에 연결한다.

## 2. 검증

- `tests/priest-damage.test.mjs`(5건): `priestdamage-x86.tsv` 10.78 1712행(본문 0 레벨 보정·1 사제 Hit·2 burning 갱신의 반환, 경계 호출 순서 B/A/X/S·C/F/N/U/R, 슬롯 50 raw, 보정 결과 4바이트; 실제 `RawPriestState` HP setter·`SquidReward` 최대 HP 사용), 자연 zero 는 성공하고 누락한 nonzero 생성 몸체는 거부, 실제 `SquidProcessHost`/`Kernel` 의 Regular 취소와 HP1 사제의 `DependFormHost` 감시 쌍 통지/삭제(여섯 Form 수명 종료, 삭제 깊이 0), 표시 갱신 뒤 권한 재조회, 연결 오류·손상 입력의 무변경 거부.
- `tests/damageable-burning.test.mjs`(2건): `damageable-burning-checking-hardware-x86.tsv` 10.78 550행(현재 DWORD 와 중복 진단, 저장값/현재값, 최대 HP 오버플로 절반 계산, 동작 비트 1/2/4/8 로 하위 효과 안에서 전역·슬롯을 바꾸는 입력), 누락한 선택 장치와 진단 중단이 이미 실행한 검색/위치음을 되돌리지 않음.

최초 실패와 교정: 새 모듈은 위 fixture 2262행 전부를 첫 실행에 통과했다. 검사 쪽에서 구성자가 다른 풀 자체를 거부한다고 잘못 가정한 한 건(원본은 감시 호스트의 풀만 비교)을 교정했다.

## 3. 경계와 남은 것

- `RawGunHit` 이식 전이라 `makePriestGunHitHooks` 는 GunHit 훅 객체의 `damage` 필드만 대체하는 형태로 옮겼다. 실제 GunHit 호출 합성은 `RawGunHit` 단위에서 대조한다.
- burning 프로세스의 생성/삭제/소리 본체(`BurningProcess` 계열)와 캠페인/세션 쪽 호출(`CampaignDamage`·`CampaignSession`·`DamageableHit` 시험)은 해당 파일 이식 때 연결한다.

## 4. 다음 시작점

`RawPriestRegen`(회복 분배기와 패치 추적), `RawPriestSpawn`, `RawPriestParticipation`/`RawPriestTakenNotice`, 사제 배치 계열(`RawPriestPlacement*`), `RawAssetPostPop` 이다.
