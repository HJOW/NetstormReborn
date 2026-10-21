# 원본 월드 코어 W03 일곱째 조각 — 피해가능객체 삭제 후 처리(RawDamageablePostDestroy)

2026-10-21, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 일곱째 조각이다.
여섯째 조각까지는 피해가능객체의 **사전** 파괴(붕괴·폭발·해방)만 이식돼 있었다.
이번 조각은 그 **사후** 처리(드롭·섬 정리·주변 HP 예약·다리 약화·공통 post)와,
그가 필요로 하는 **HP 조회**·**다리 금간 프레임 검색** 몸체를 이었다.
근거는 커밋된 cpppj 소스(`RawDamageablePostDestroy.cpp`·`SquidReward.cpp`·`Bridge.cpp`)와 원본 fixture(`damageablepostdestroy-x86.tsv`)뿐이다.
새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawDamageablePostDestroy.cs` (신규) | `DamageablePostDestroyState`(전투·권한·생성 수·제외 타입 등), `DamageablePostDestroyHooks`(드롭·섬 정리·Regular·다리 붕괴 조회·표면 알림·표시 갱신·다리 삭제·공통 post), `RawDamageablePostDestroy`(`PostDestroy`·`Drops`·`GenusAt`) | `RawDamageablePostDestroy.cpp` 의 드롭 → 섬 정리 → HP 예약 → 다리 순회 → 공통 post 순서 |
| `RawHitPoints.cs` (신규) | `RawHitPointState`(제단·사제 타입·사제 1/4 HP·debug assert), `RawHitPoints`(`HasHitPoints`·`MaxHitPoints`·`TypeHitPoints`) | `SquidReward.cpp` 의 HP 조회 부분. SP·지갑·샘 풀 계산은 이 몸체에 포함하지 않는다 |
| `BridgeFrameSearch.cs` (신규) | `FrameNumber`·`FrameSide`·`CrackedFrame`·`NormalFrame`·`FindMasked`·`FindNumber` | `Bridge.cpp` 의 금간·단단한 프레임 검색(금간 번호 10 이상, 단단한 번호 20 이상, 금간 플래그 0x20) |
| `X86RawDamageablePostDestroyTests.cs` (신규) | 원본 10.78 몫 2,246행 재생 1건 | `cpppj/tests/DamageablePostDestroyTests.cpp` 의 `DamageablePostReplayScene`·`Hooks`·`Prepare` 열과 같다 |
| `RawDamageablePostDestroyTests.cs` (신규) | 합성 4건(미연결 효과·자료 크기·잘못된 번호·비유한 좌표·HP 조회 범위) | `DamageablePostDestroyTests.cpp` 의 `DamageablePostDestroy_PackedCountAndRequiredCreationBoundary` 와 같은 구성 |

기존 C# 모듈을 그대로 쓴다: `RawSquidNeighbors.Center`(발자국 중심)와 `RawSquidFinder`(일반 탐색).

### 순서와 규칙

`RawDamageablePostDestroy.PostDestroy(sid, flags)` 는 원본 순서대로 다음을 실행한다.

1. `flags & 0x300000` 이면 드롭을 만든다.
2. 부모가 abstract·매림이 아니고 타입 플래그1의 `0x400` 이 켜져 있으면 섬 정리를 확인한다.
   `0x1000000` genus 를 가진 일반 후보가 그 칸에 있고, 부모 genus 의 `0x10000000` 이 꺼져 있으며,
   `flags & 0x200000` 이거나 부모 타입이 무음 표면 타입(147)이 아니면 섬 정리 드롭을 부른다.
3. 전투 중이고 부모가 HP 를 갖고 abstract·매림이 아니면 주변 HP 예약을 한다.
   공격자 소유자는 `flags` 의 16~19비트다. 피해는 `최대 HP / 2 + 1` 이고 소용돌이 genus 는 400 으로 고정한다.
   `flags & 0x400000` 이 아니면 반경 `발자국 / 2 + 1` 안의 HP 후보마다 `Regular` 을 부르고,
   count 에 `공격자 소유자 × 65536 + 피해` 를 넣는다.
4. 부모 genus 의 `0x130000` 이 꺼져 있으면 반경 2 안의 다리 후보를 순회한다.
   금간 프레임(`0x20`)이 아니거나 예약된 다리 붕괴가 있으면 약화하고, 그중 단단한 프레임(`0x40`)은 건너뛴다.
   금간 프레임이고 예약된 붕괴가 없으면 권한이 있을 때 다리를 삭제한다.
5. 공통 postDestroy 를 부른다.

드롭 수량은 `1` 에서 시작해 genus `0x4200` 이면 `5`, genus `0x20000` 이면 `2` 이다.
제외 타입(133·156·144·138·187)에 걸리면 만들지 않고, 특별 타입(135)이면 `3` 이며, 현재 생성 수가 20 을 넘으면 절반이 된다.

약화는 life WORD 의 `0x78` 마스크를 `bridgeCrackLife - 1` 값으로 바꾸고 표면 알림을 낸 뒤,
금간 프레임을 찾아 쓰고 표시 갱신을 부른다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,262개 통과**(Assets 363 + Core 899; 이번 +5).

| 검사 | 내용 |
| --- | --- |
| `damageablepostdestroy` 10.78 몫 2,246행 재생 | "Post" 절 1,606행은 공통 post 를, "Drops" 절 640행은 드롭 수량 계산만 부른다. 매 행마다 외부 경계 순서와 관찰 대상 7개 슬롯의 전체 바이트가 일치한다 |
| 합성 4건 | 미연결 하위 효과·타입/프레임 표 크기·다른 풀의 HP 조회·할당되지 않은 번호·타입 표 밖 번호·비유한 좌표·HP 조회 범위와 debug assert 를 실행 전에 거부한다 |

`damageablepostdestroy-x86.tsv` 는 총 4,412행(2 판본 × Post/Drops 2절)이며 10.78 몫 2,246행만 재생했다.
`originalCD` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).

10.78 몫에서 실제로 잡히는 경계는 드롭(`D:`) 486행·섬 정리(`I:`) 60행·Regular HP 예약(`R:`) 72행·
다리 붕괴 조회(`F:`) 170행·표면 알림(`N:`) 150행·표시 갱신(`U:`) 142행·다리 삭제(`X:`) 130행·공통 post(`B:`) 1,606행이다.

**변이 확인:** 구현 12곳을 하나씩 망가뜨려 10곳이 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| 드롭 수량 규칙(genus `0x20000` → 2) 제거 | 원본 재생 |
| 현재 생성 수 20 초과 시 반으로 나누기 제거 | 원본 재생 |
| 드롭 대상 genus 마스크 `0x174200` → `0x174000` | 원본 재생 |
| 피해 계산의 `+1` 제거 | 원본 재생 |
| Regular count 의 `공격자 × 65536` 패킹 제거 | 원본 재생 |
| HP 예약 반경의 `+1` 제거 | 원본 재생 |
| 약화 life WORD 계산(`life - 1` → `life`) | 원본 재생 |
| 단단한 프레임(`0x40`) 검사 제거 | 원본 재생 |
| HP 조회의 제단(dais) 분기 제거 | 원본 재생 |
| 금간 프레임 강제 시 단단한 번호 대체값 `1` → `2` | 원본 재생 |
| 좌표 절삭의 유한성 검사 제거 | 합성 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

**잡히지 않은 2곳과 이유**

- `GenusAt` 의 skipSurface 플래그: 섬 정리에 쓰는 genus 가 `0x1000000` 이라 항상 `genus & 6 == 0` 이고,
  플래그를 0 으로 바꿔도 0단계 순회에서 같은 후보가 먼저 나온다(구현은 원본 규칙 그대로).
- 좌표 절삭의 **CRT 범위 경계**: 원본 `_ftol` 의 signed 64 절삭 경계는 이 장면의 좌표 범위에서
  일반 `(int)` 캐스트와 같다. 유한성 검사(NaN·무한대)는 합성 검사가 잡는다.

## 3. 이번에 하지 않은 것 (경계)

- **삭제 보상(SP·지갑·샘 풀) 계산은 `RawHitPoints` 에 넣지 않았다.** 원본 `SquidReward` 의 HP 조회 부분만 옮겼고,
  보상 계산은 별도 조각(삭제 보상 계산)에서 옮긴다.
- `RawDamageablePostDestroy` 의 섬 정리 드롭·일반 드롭은 **외부 효과로만** 연결했고, 실제 섬/유닛 생성 몸체는 없다.
- Damageable **피격**(hit), 사제 경계, 내용물 조회(`contained`)·내용물 수명, Form 수명, 신전 삭제는 아직 옮기지 않았다.
- CD 10.72 몫, 파생 분배 허브·실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

Damageable 피격(`damageablehit`) → 종속 조회·내용물(`contained`·`contentlifetime`) →
Form 수명(`formlifecycle`·`formtake`·`form-packet`) → 신전 삭제 → 삭제 보상(SP·지갑·샘 풀) 계산 →
파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
