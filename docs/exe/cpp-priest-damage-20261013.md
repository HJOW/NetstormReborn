# 사제 직접 피격과 burning 판정 복원 (2026-10-13)

분석 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 보호 원본 10.78/CD 10.72 파일은 읽기만 했다. `victimdamage` 6개 및 `victimdamage-helper` 9개 함수씩을 현재 정밀 프로젝트에서 읽기 전용으로 내보냈다. 게임/OS 실행은 없다.

## 실제 피격 사슬

`RawPriestDamage`는 사제 가상 `+0x98`의 실제 피격 몸체다. NoDamage로 피해를0으로 바꾸고 현재 회복 Regular `0x25d` 또는 held 방향10..14를 확인한다. 해당하면 `0x25c/0x25d`를 모두 취소하고 현재 타입의 defaultFrame으로 바꾼다. 플레이어 레벨 보정 뒤 HP를1 이상으로 제한해 기존 **RawPriestState.SetHitPoints**로 전달한다. 이후 현재 소유자에게 피격 사제/공격자를 통지하고 burning 판정을 수행한다. 권한이 있고 현재 HP가1이면 현재 타입1 DependForm들을 원본의 선저장 next 순서로 통지/제거한 뒤 false를 반환한다.

`MakePriestGunHitHooks`는 GunHit의 피해 경계를 실제 사제 몸체로 분배한다. `MakePriestDamageWorldHooks`는 같은 **SquidProcessHost.FindEvent/Kill**, 기존 HP setter, **RawDamageableBurning**을 연결한다. 회복 취소는 현재 첫 일치 Regular를 반복 조회해 실제 Form/Kernel을 제거한다. HP setter의 절반 경계 권한과 좌표 보정은 기존 구현을 재사용한다. nonempty 감시 사슬은 같은 **DependFormHost**의 통지/양방향 쌍 삭제를 사용한다.

| 몸체 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 사제 Hit | `00494380` | `0040cb10` |
| 사제 HP setter | `004942b0` | `0040ca40` |
| 플레이어 피해 보정 | `0044bfe0` | `00462280` |
| held 방향 검사 | `004ad710`에서10..14 | `0040c800`→`004ae4b0`,10..14 |
| burning 검사 | `0044c060` | `00462480` |
| HP1 감시 iterator | `004b22d0/004b1b80/004b2390` | `004eb960/004eb990/004eba30` |

## CD 10.72 차이

패치는 Challenge `0052f7e0+0x4f`의 플레이어1..8 rank BYTE를 `0041c5b0`의24칸 표로 변환한다. rank11/23은 실제 레벨15/100이다. 다른 플레이어 번호에는 레벨0을 돌려준다. 보정은 높은 공격자 레벨이면 `damage×(차이+20)/20`, 낮으면 `damage×20/(차이+20)`이다.

CD는 Player `+0x68` BYTE를 그대로 레벨로 사용한다. 보정 기준은20 대신 **8**이다. 따라서 동일 rank 입력의 결과가 패치와 다를 수 있다. `PriestDamageState.levels`에는 패치의 원본 rank BYTE/CD의 원본 level BYTE를 소유자 번호로 공급한다. 두 판본 모두 중립 소유자0 또는 보정 차단 전역이 있으면 그대로 반환하며, DWORD 곱셈 감김과 signed 정수 나눗셈 절삭을 보존한다.

패치 사제 HP는 signed DWORD, CD는 signed WORD다. CD 저장 후의 절반 경계와 HP1 검사에서도 다시 부호 확장한 실제 WORD를 읽는다. 죽은 사제 입력은 패치가 원본 assert로 거부하고 CD는 true로 자연 반환한다. 이번 native 격자는 살아 있는 사제만 포함한다.

## burning의 실제 범위

`RawDamageableBurning.Refresh`는 free 또는 현재 타입의 `damageEffect==0`일 때 조회/생성 효과 없이0을 반환한다. nonzero면 현재 HP와 실제 TypeHitPoints의 절반을 비교하고 process14의 첫 현재 Form을 찾는다. 패치 checking 모드에서는 두번째가 없어야 한다. 저HP이며 기존 process가 없으면 bulf135만 `bulfHurt.wav`를 요청한 뒤 실제 burning 생성자를 요청한다. 절반 이상으로 회복했고 기존 process가 있으면 실제 Form 삭제를 요청해0을 반환한다.

조회/생성/삭제/소리 경계는 명시 callback이다. nonzero 생성자 `00424090/CD004a6720`와 파생 burning 애니메이션 자체는 후속이다. callback이 빠지면 예외이며 빈 성공으로 처리하지 않는다. 사제의 원본 타입은 damageEffect0이므로 이번 사제 경로의 자연zero 반환은 실제 원본 분기다.

## 독립 원본 근거와 검증

제한 x86 실행기 `tools/decomp_priestdamage_oracle.py`는 **3424개 고유 정상 반환**을 기록한다. 판본별1712개는 rank336, 전체 사제Hit+기존HPsetter1152, 전체 burning판정224다. 원본 assert/OS 호출은0이다. 패치는 실제 rank getter2472회, 두 판본 모두 실제 사제HPsetter1152회, 실제 burning 몸체1376회를 실행했다. HP1의 빈 감시 사슬도 실제 원본 FormFinder를 실행했다.

회복 query/취소·frame setter·소유자 통지·가상 공간 Unpop/Repop·burning find/create/destroy/sound는 명시 관찰 경계다. native 격자는 각 몸체와 경계 인자/순서·전체 raw 쓰기를 증명하며, 이 효과들의 실제 게임 구현 완료를 증명하지 않는다. C++ 추가 연결 검사는 실제 Regular 취소/Kernel 제거와 HP1 사제의 두 nonempty 의존 쌍 통지/삭제를 같은 풀에서 수행한다. 공간/frame/GUI는 그 검사에서도 관찰 경계다.

저장물은 `cpppj/tests/fixtures/priestdamage-x86.tsv`, `cpppj/priestdamage.json`, `PriestDamageTests.cpp`다. manifest에는 새 C++/테스트 소스, 두 PE, 실행기, 정밀 export와 fixture SHA를 포함한다. 감사 명령은 `python -X utf8 tools/decomp_priestdamage_oracle.py --verify`이며3424개로 통과했다. CMake 등록 후 **집중4개 검사·전체974개 검사/실패0, 최종 Release 두 타깃 종료0·경고/오류0**으로 대조했다. 실제 소비자 검사 입력에는 원본 타입158의 constructorAddress를 공급했다. 통합 근거는 `run6.json`, 최종/초기/교정 로그는 `extracted/run6-*`다.

## 남은 실제 월드 연결

- 피격 사제의 defaultFrame/실제 SquidFrame.Set, 소유자 피해 통지를 같은 월드에 연결한다.
- nonzero burning의 파생 생성/실제 애니메이션/삭제와 다른 피해자의 기본 Squid/Damageable 가상 피해를 복원한다. 일반 HP 감소로 사제·신전·건설 예외를 대체하지 않는다.
- 캠페인 초기 적 SunArcher71의 실제 회전/발사 및 표시 자산 sunDisc83을 복원한다. 사제 피격 단위만으로 캠페인 전투 완성을 주장하지 않는다.
