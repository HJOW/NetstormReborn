# 일반 자산 BaseHit·DamageableHit (2026-10-13)

현재 분석 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 보호 원본10.78/CD10.72를 읽기만 했고 게임/GUI/OS를 실행하지 않았다. 기존 `victimdamage`, 새 `damageablehit-helper`의 현호스트 정밀 readOnly 내보내기를 사용한다.

## 복원 범위

`RawDamageableHit`은 사제의 HP1·감시해제와 구별하는 일반 자산의 실제 피해 몸체다. 기존 `RawPriestDamage::Adjust`의 공통 레벨 보정, `RawDamageableBurning` 및 `SetRawSquidHitPoints`를 그대로 소비한다. 기존 완료 파일은 변경하지 않았다.

| 항목 | 10.78 | CD10.72 |
| --- | --- | --- |
| BaseHit | `004add10` | `004aeda0` |
| DamageableHit | `0044c8e0` | `00462300` |
| HP 보유 판정 | `004adc60` | `004aecf0` |
| 공통 비가상 HP 쓰기 | `004adca0` | `004aed20` |
| 레벨 보정 | `0044bfe0` | `00462280` |
| burning 판정 | `0044c060` | `00462480` |
| HP 저장 폭 | raw26 signedDWORD | raw26 signedWORD |

BaseHit는 live/free/dead·extra9·NoDamage를 판정하고 실제 virtual+0x9c 방어 검사를 수행한다. 레벨 보정 후 HP를 뺀다. 서버에서 결과가1 미만이면 현재 피해자/공격 owner 전역을 기록하고 실제 virtual+0x10 삭제를 요청해 false를 반환한다. client에서는 HP1로 제한하고 현재 owner에 피해를 통지한다. 10.78 dead/디버그 held는 원본 진단이며 CD는 해당 자연 반환을 유지한다.

삭제 flags는 공격 owner를16비트 왼쪽으로 민 값에 `0x100000`을 더한다. 공격자 genus에 `0x100`이 있으면 `0x4100000`으로 바뀐다. CD의 별도 death-context 두 helper도 실제 PE로 실행한다. 10.78 BaseHit의 HP쓰기는 몸체 내부 직접 저장이며 CD는 공통 setter를 호출한다. C++는 동일 raw 폭의 공유 setter를 사용한다.

DamageableHit는 먼저 HP 보유를 검사한다. genus `0x400000`은 **dais 부류**이며 현재 extra bit0을 사용한다. 일반 자산은 TYPE_HAS_HP `0x10`이다. 이 genus를 거주 구역으로 해석하지 않는다. 현재 local owner의 실제 주 섬 피격이면 wall clock으로 경고를 제한하고, extra bit4이면 virtual+0x88 재평가 후 BaseHit를 실행한다. 생존 반환일 때만 기존 실제 burning 판정을 호출한다.

## 섬 피격 경고의 판본 차이

현재 지면 자산이 일반 island genus2이고 island3x3 비트가 없으며 raw8의 섬 번호가 현재 Player의 주 섬과 같을 때만 경고를 처리한다. 주 섬은 패치 Player+0x78/CD+0x70이다. 경고 시계는 gameTime과 다른 실제 wall clock이며 일반적인 실제 게임 시각으로 바꾸지 않는다.

경고가 due이면 10.78은 event12→`islandUnderAttack.wav`→format `islandUnderAttack`→텍스트 통지를 요청한다. CD는 같은 sound만 요청한다. 두 판본 모두 due 여부와 무관하게 해당 주 섬 피격 뒤 마감을 다시 갱신한다. 간격은 원본 float30초다. 패치 timer의 실제 `00418084`는 **fadd qword[clock]**이며 두 판본 모두 `double(now)+double(float30)`을 저장한다. Ghidra의 패치 `float(now)` 표현을 따르지 않는다. 현재 getter/clock/UI 경계의 호출 순서를 보존한다.

## 독립 원본 관찰과 한계

`tools/decomp_damageablehit_oracle.py`는 양판본 **5,846개 고유 정상 반환**, 판본마다2,923개를 관찰했다. Base의 HP·피해·레벨·권한·free/held·가상 방어, Damageable의 HP/dais·unrealize·burning, 주 섬/경고 마감과 bomb genus를 교차한다. 초안5,848개에는 기본 derived와 alarm 격자의 동일 입력 재방문2개가 포함되어 제거했다. 이 재방문을 신규 고유 입력으로 세지 않는다. 원본 assert/OS 호출은0이며 dead 진단 격자는 아직 별도 관찰에 포함하지 않는다.

방어·삭제·unrealize·소유자 통지, wall clock·경고 UI, burning find/create/destroy는 명시 관찰 경계다. 실제 원본 주몸체·레벨/rank·HAS_HP·섬 genus/id·예약 마감·CD death context·현재 raw HP 저장을 실행한다. slot/type 플래그는 독립 합성 입력이며 실제 미션 조립과 모든 파생 타입의 virtual dispatch 완료를 증명하지 않는다.

저장물은 `damageablehit-x86.tsv`와 `damageablehit.json`이다. 현재4개 C++ 검사는 단위09의 최종 공유 Release에서 **실패0**으로 통과했다. `MakeDamageableGunHitHooks`는 **현재 raw vtable와 실제 virtual+0x98 진입주소의 대응표**를 받아 Base/Damageable만 분배한다. 현재 vtable를 타입 번호로 추측하지 않는다. 다른 파생 피해는 기존 callback으로 전달하며 누락은 예외다. 사제 피해는 기존 별도 몸체를 유지한다. 별도 C++ 검사는 current-vtable 분배 및 다른 파생 callback 보존을 검사하며 신규 PE 입력으로 더하지 않는다.

2026-10-14 첫 공유 빌드91450은 검사 코드의 존재하지 않는 `Random` 형식명으로 실패했다. 실제 `GameRandom`으로 한 줄만 교정했다. 교정 전 생산 source/h/test/oracle/fixture/전체보고서 및 실패 로그를 `hist/damageablehi-1d1d7`에 실제 바이트로 보존했다. `analyze_damageablehit_correction.py`는 한 줄 역변환과 생산/원본 입력 불변을 검사한다. 현재 native 보고서는 동일5,846관찰과 이전 전체보고서 SHA를 유지하며 현재 test SHA만 연결했다. 신규 native/고유 입력은 각각0이다. 교정 후 공유 C++ 대조는 대기 중이다.

컴파일 교정 후 첫 C++ 전수 대조는 두 판본 native 검사에서8checks 차이가 있었고 strict 경계/current-vtable 검사는 통과했다. 당시 source/h/test/oracle/fixture/전체보고서/교정도구·보고서와 로그를 `hist/damageablehi-8cde8`에 실제 바이트로 보존했다. 패치의 double 경고 마감과 C++ 검사 입력의 CD 지면 provider를 교정했다. CD native는 셀(20,21)에만 SID70을 공급했지만 실제 inline 조회는 raw좌표(20.75,21.9)에0.9999f를 더해 셀(21,22)을 읽으므로 지면0이다. C++ 검사는 같은 hash0 입력을 만들고 기존 `RawPathSurfaceAt`의 실제 보정 조회를 소비한다. 원본 입력/fixture/oracle는 바꾸지 않았다. 현재 schema2 lineage는 두 전체 checkpoint와 각 명시 변경만 허용하며 신규 native/고유 입력은0이다. **이5,846행은 CD 경고 활성/body의 실제 sound·마감 변경 관찰을 포함하지 않는다.** 해당 branch는 원본 명령을 읽어 복원했으며 활성 CD 지면 입력은 별도 후속 관찰로 추가해야 한다. 최종 공유 Release72046 종료0 뒤 DamageableHit4검사/실패0으로5,846전수·엄격한 경계·현재vtable 소비자가 모두 통과했다. 최종 로그는 run9-DamageableHit_final.log다. 단위09 전체 검사는 별도 관리한다.

## 실제 월드 후속

서버 사망의 실제 virtual destroy→DamageablePreDestroy→corpse/폭발·시각/음향→같은 RawSquidDestroy/Kernel 체인을 연결해야 한다. burning nonzero 생성자와 입자/애니메이션도 별도 미완료다. 독립 관찰의 삭제 callback 성공을 실제 피해자 삭제나 corpse 생성 완료로 세지 않는다.

CMake 등록은 `RawDamageableHit.cpp`, `DamageableHitTests.cpp`, `NETSTORM_DAMAGEABLEHIT_FIXTURE`가 필요하다. 공유 `RawSquidHitPoints`의 기존 본문/출처 lineage는 동료의 `squidhitpoints-lineage.json`에서 관리한다. 이 모듈은 현재 shared setter/기존 Adjust·Burning·SquidReward·사제3,424행 보고서의 SHA를 연결한다. 감사 명령은 `python -X utf8 tools/decomp_damageablehit_oracle.py --verify`다.
