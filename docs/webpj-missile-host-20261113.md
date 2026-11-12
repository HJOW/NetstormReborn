# webpj 투사체 Host 공장 RawMissile 마감 (WB04-E361, 2026-11-13, 7-P2 셋째 항목)

3.1절 7-P2 셋째 항목 "RawMissile (partial 394줄, +24)"의 남은 부분을 `core/raw-missile.js` 에 옮겨 `RawMissile` 을 마감했다(PORT_MAP partial → port). 생성자·프레임·진단·`MissileProcess` 는 E125·E126 에서 끝났고, 이번에 **Host 공장 세 함수**(`makeMissileFactoryHooks`·`createMissile`·`makeGunMissileHooks`)와 Kernel 소멸 시 자기 삭제 관찰(`MissileProcess.destroy`, C++ `~Missile`), 이것들이 있어야 할 수 있는 **실제 Host 자기 삭제 시험**을 더했다. 앞서 이 일은 `SquidProcessHost`·`RawGunProcess` 이식 뒤로 미뤄져 있었다.

## 1. 규칙

- **`makeMissileFactoryHooks(pool, factory, hooks)`:** 같은 풀의 실제 `SquidFactory` 의 `create(type, flags)` 만 Missile 훅의 `create` 에 연결한 **새 묶음**을 돌려준다. 나머지 월드 효과 훅(Pop·owner·비행 효과 등)은 호출자의 것을 그대로 두고 원래 묶음은 바꾸지 않으며, 풀이 다르면 `TypeError`('풀 불일치')로 거부한다.
- **`createMissile(host, logic, request)`:** 풀이 다르면 생성 전에 거부한다. 그렇지 않으면 실제 생성자(`construct`: 표시 자산 생성·소유자·공간 Pop·비행 효과)를 새 `MissileFields` 에 실행하고, 그 표시 자산을 부모로 `logic.processType`(원본 missileProcessType 28)으로 원본 flags **0x10** 부착(`host.attach`)한 `MissileProcess` 를 돌려준다. 부착이 안 되었거나 `MissileProcess` 가 아니면 `null`.
- **`makeGunMissileHooks(host, logic, hooks)`:** GunAnimation 의 엄격한 `createProjectile` 경계만 실제 `createMissile` 로 바꾼 새 묶음이다(나머지 훅 보존, 풀이 다르면 거부). 부착에 실패하면(생성 자산 삭제 중) 오류로 거부한다. 요청 객체는 가공하지 않고 같은 객체로 생성자에 전달한다.
- **`MissileProcess.destroy()`:** Kernel 이 이 프로세스를 지울 때(`remove` → `destroy`) 불린다. C++ `~Missile` 처럼 진행 중인 프레임의 자기 삭제 관찰(`alive`)을 끈다. 진단 장치가 `host.kill` 로 자기를 지워도 `frame` 은 삭제된 필드를 다시 읽지 않고 이전 좌표/종료를 반복하지 않는다. 이전에는 `dispose()` 가 있었지만 Kernel 이 부르는 `destroy()` 와 이어져 있지 않아 합성 `alive` 로만 시험했다.

## 2. 웹 범위와 기준선 뒤 변경

- 10.72(CD)는 범위 밖이다(PATCH 만). 기준선 뒤 변경 `20261109-60 CP`(발사자 역할 DWORD 를 비교 위치마다 읽는 shared getter)는 3-E 판정 E263 ③ 그대로이며 웹은 `MissileState` 필드를 소비 위치에서 읽는다(PORT_MAP base `cc562ccf` 유지).
- 앱 수명 연결(월드의 Pop·owner·비행 효과·사정선·포구 훅 구현과 `RawCampaignArcherBindings` 의 조립)은 후속 단계 몫이다. 이 모듈은 DOM 을 쓰지 않는다.

## 3. 검사

- **`tests/raw-missile-host.test.mjs`(5건):** ① cpppj `Missile_ActualFactorySpaceKernelAndDelete`(PATCH) 이식 — 실제 `SquidFactory`·`SquidOwner`·`SquidPop`/`SquidUnpop`·`SquidPostPop`·`SquidDestroyLifecycle`·`SquidProcessHost`·`Kernel`·`RawGunHit` 를 한 풀에 묶어 `createMissile` 로 부착한 투사체가 표시 자산(발사자 소유자 1, free/dead/void 아님, 종속 head 에 Form)과 Pop 1회로 시작하고, 시각 20 의 한 프레임에 사거리 종료로 Kernel 크기 0·표시 자산과 Form 삭제·총 사거리 갱신 1회·Pop 2회·삭제기 깊이 0 이 되는지 **첫 실행에서** 확인(처음 실패는 시험의 요청 기본값 누락이었다: 속도 20·표시 80·포구 무효 좌표 (0,0)); ② `makeMissileFactoryHooks` 의 create 교체·나머지 훅과 원래 묶음 보존·풀 불일치; ③ `createMissile` 의 부착 인자(TYPE 28/지정값·표시 자산·flags 0x10)·요청 동일성·부착 실패 null·풀 불일치 시 생성 전 거부; ④ `makeGunMissileHooks` 의 createProjectile 교체·실패 거부·풀 불일치; ⑤ `MissileProcess.destroy`/`dispose` 와 실제 `Kernel.remove` 의 연결.
- **`tests/missile-checking.test.mjs`(2건 추가, 총 8건):** cpppj `MissileChecking_ActualHostDeletionInFirstDiagnosticStopsFields`·`…InLateDiagnosticStopsFields` 이식 — 진단 장치가 실제 `host.kill(process, 0x10)` 으로 자기 프로세스를 지우면 첫 진단은 `['A105']`, 후반 진단은 `['U12','P1097859072','B','C','A105']` 에서 멈추고 Kernel 크기 0·Form 삭제·표시 자산 살아 있음·이전 좌표 −10000 이 유지된다.

## 4. 변형 확인과 남은 경계

- 새 코드에 의미 변형 25회(소멸 연결 제거·풀 검사 제거/반전·hooks 펼치기 제거·create 인자 순서/생략/고정·부착 인자 TYPE/flags/부모/필드 객체·결과 형 검사·createProjectile 실패 처리와 요청 가공 등)를 적용했다. 1차에서 1개(요청 가공 `{ ...request, ricochet: 1 }`)가 남았고 **시험이 요청을 같은 객체로 단정하지 않은 공백**이라 보강해 25개 모두 잡혔다. 동등 변형은 없다.
- 변형하지 않은 방어선: 오류 메시지 문구(핵심 낱말만 확인).
- 다음 7-P2 항목은 **RawProjectileGeometry**(partial: muzzle 71 외·hotspot·directionFrame·아이스/썬더(129/131) 사정선의 소비자 fixture 대조와 실제 SHP 헤더 연결)이다.
