# Damageable 삭제 준비의 효과·소리 접두와 실제 종속 순회

2026-10-08, **마지막 디컴파일 수행 PC: HJOW-Athlon**. AGENTS.md·두 LEFT_JOBS를 읽고 현재/마지막 호스트 일치를 확인했다. 새 `damageablepredestroy` 목록을 같은 PC에서 읽기 전용 Ghidra로 내보냈다(세 판본 각각 9개). 정밀 디컴파일·원본 게임/복사본·클론 창 실행, 보호 파일·AGENTS.md·dotnetpj 변경, 커밋/푸시 없이 콘솔/정적 검사를 수행했다.

## 주소와 구현 범위

| 몸체/전역 | 10.78 | CD 10.72·추가 10.37 |
|---|---|---|
| Damageable preDestroy | `0044b4b0` | `004615e0` |
| 붕괴 효과(외부) | `004605f0` | `00454800` |
| 폭발 효과(외부) | `004605d0` | `004547d0` |
| 위치 소리(외부) | `004a9d70` | `00438c00` |
| 전역 priestFree 소리(외부) | `004a9cb0` | `00438b50` |
| spot BYTE 배열 포인터 | `005c7c44` | `0052fe48` |
| spot 좌표 bias float | `00500edc` | `00502b70` |
| CRT 절삭 | `004e49c0` | `004f161c` |
| contained Begin/Next | `004b2270` / `004b1b20` | `004eb8d0` / `004eb900` |
| 기본 true 필터 | `0044daa0` | `0040f230` |
| 사제 kind 비교 DWORD | `005412d0` | `0051cbbc` |
| 권한 해방 구간(외부, 끝 제외) | `0044b626`~`0044b80c` | `0046174e`~`0046195a` |
| 공통 preDestroy(연결) | `004b0950` | `004add20` |

[`RawDamageablePreDestroy`](../../cpppj/src/o/RawDamageablePreDestroy.h)는 extra `&9`가 0인 **첫 진입 조건**에서 아래 순서를 실행한다. abstract/buried면 접두를 모두 생략하지만 같은 flags의 공통 preDestroy는 항상 호출한다. dead/void는 생략 조건이 아니다.

1. flags `0x200000`: 붕괴 효과(SID)→그 뒤 현재 raw 좌표의 `collapse.wav` 요청(flags 0).
2. flags `0x100000`: 폭발 효과(SID)→그 뒤 현재 raw 좌표의 `explosion.wav` 요청(flags 0). 두 비트가 함께 있으면 두 경로를 모두 순서대로 실행한다.
3. 현재 raw 좌표+`0.9999f`를 x87 정밀도로 더한 뒤 0 방향 절삭해 `spot[y*256+x]`를 읽는다. BYTE `&6`이 0이 아니면 실제 contained finder를 순회한다.
4. finder 후보의 `+0x12` WORD가 현재 사제 kind DWORD(기본 158)와 같을 때마다 `priestFree.wav`를 전역 요청한다. 원본 인자는 `(name,0,0,0,1,0)`다. 첫 일치에서 멈추지 않으며 65536/0xffffffff를 WORD로 줄이지 않는다.
5. 그 뒤 현재 boss가 참이고 flags `&0x800`이 0이면 종속 해방 구간을 요청한다.
6. 같은 flags의 공통 preDestroy를 호출한다. 깊이 감소는 연결한 공통 몸체가 수행한다.

붕괴/폭발 하위 함수와 소리의 **호출 조건·인자·순서**를 복원했다. 파편 생성·실제 소리 출력은 외부 훅이다. 권한 해방의 생성자/타입/좌표 보정·소유자 지정/배치 몸체도 외부 경계다. 이 구간의 실행 조건만 복원했다. 기존 일반 Pop/비표면 가상 지원 범위를 넓히지 않았다.

## 좌표·커서와 효과 중 변경

spot은 보호막 finder의 무 bias 절삭과 다르다. `0.9999f`의 실제 값은 `0.9998999834060669`다. native는 `fld/fadd` 뒤 바로 CRT 절삭하며 덧셈 결과를 float로 저장하지 않는다. C++도 두 float 입력을 double로 올려 더하고 int로 바꾼다. 예를 들어 raw `20.0001f`에 float 반올림을 먼저 넣으면 21이 되지만 원본은 20 셀을 고른다. 독립 fixture가 이 경계를 포함한다.

[`RawContainedFinder`](../../cpppj/src/o/RawContainedFinder.h)를 공용으로 분리하고 기존 Carrier의 `FindContained`도 연결했다. 다음 SID는 반환 전에 저장한다. 외부 소리에서 현재 후보 next/부모 head를 바꿔도 이미 저장한 다음 후보로 진행하며 아직 읽지 않은 후보의 타입/next 및 현재 타입 DWORD는 그 시점에 읽는다. 기본 true 필터를 사용하고 free/dead/void/contained 후보를 추가로 제외하지 않는다. Damageable의 Begin은 원본 `yesIKnow=1`처럼 패치판 권한 없는 dead 부모 assert를 허용한다. Carrier 조회는 기본 false를 유지한다.

효과 중 extra가 abstract/buried로 바뀌어도 첫 ordinary 진입 조건을 다시 평가하지 않는다. 좌표는 각 효과와 위치 소리 뒤 새로 읽고, 사제 kind·finder 타입·boss도 각 사용 시점의 값을 읽는다. 범위/순환·누락 하위 훅과 ordinary의 비유한/보드 밖 좌표를 C++에서 거부한다. abstract/buried는 좌표를 읽지 않으므로 NaN도 공통 몸체까지 전달한다. 후속 훅이 유효 범위 밖 좌표나 손상 체인을 만드는 입력은 정상 원본 대조에 포함하지 않는다.

## 실제 연결과 독립 대조

`MakeDamageablePreDestroyHooks`는 같은 풀의 Carrier validateDamageable/damageablePre만 새 몸체로 연결한다. 사제 보호막 조회→새 Damageable 접두→Carrier 조회→실제 공통 장부/깊이→Regular/ProcessForm/Kernel 정리·SID 반납을 기존 두 판본 × server/client 합성 검사에서 실행했다. 붕괴/위치 소리/해방 요청은 각각 1회이며 회복 form 타입 46은 기본 contained 타입 6에 해당하지 않는다. 파편/소리 출력·종속 해방·보호막 파생 수명/일반 공간 해제는 기록 경계다.

[`decomp_damageablepredestroy_oracle.py`](../../tools/decomp_damageablepredestroy_oracle.py)는 세 실제 PE 각각 **1,632개**(Damageable/Carrier 각 816개), 총 **4,896개**를 두 x87 정밀도로 실행한다. 두 정밀도의 관찰은 같고 정상 EIP/ESP·thiscall/cdecl 인자·보존 레지스터·x87 상태·허용 코드/스택 쓰기를 검사한다. assert/OS 호출은 0회다. C++는 사건·현재 권한/타입/사제 kind 전역·입력 5개 raw 슬롯 전체를 비교한다.

**대체 범위는 함수 호출과 코드 구간을 구별한다.** 붕괴/폭발·위치/전역 소리·공통 pre·Carrier 전역 후처리의 함수 진입은 기록 대체한다. 권한 해방은 위 표의 **중간 코드 구간 첫 명령에서 끝으로 건너뛰어** `releaseContained` 요청만 기록한다. 이 구간의 내부는 실제 실행하지 않았다. 바깥 boss/flags 조건, spot 덧셈/CRT, contained Begin/Next/기본 true 필터, Carrier 조건 조회/정상 반환은 원본 명령이다. 파일/범위 SHA와 대체 구간의 양끝을 감사한다.

각 PE·두 정밀도 합계: Damageable 3,264회·Carrier 1,632회·CRT 1,920회·Begin 1,408회·Next 2,448회·true 필터 1,656회·Carrier 조회 832회. 기록 대체는 붕괴/폭발 및 각 위치 소리 640회씩, priestFree 736회, 권한 해방 구간 320회, 공통 pre 3,264회, Carrier 전역 후처리 616회다. 누적 독립 x86 입력은 **207,577개**다. 기존 Carrier의 독립 2,880행도 공용 커서로 다시 대조한다.

최종 Release 경고/오류 0·CTest 내부 **311개·실패 0**(91.03초), 감사 **41종 모두 통과**다. 검증 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 이 단계 기록에 남긴다. 로그는 `extracted/cpp-damageablepredestroy-build.log`·`cpp-damageablepredestroy-ctest.log`·`cpp-damageablepredestroy-audits.log`다.

## 다음 작업

권한 해방 구간은 원본 타입 footX가 1이면 현재 좌표, 아니면 중심 좌표를 선택하고 좌표 보정→다시 contained finder→후보 flags `0x20`·전투/허용 genus→걷는 타입과 일반 타입의 별도 생성/좌표/소유자/배치를 수행한다. 다음은 이 조건/좌표 접두와 외부 생성/배치 요청을 복원하는 것이다. 파편/소리 하위 몸체, Carrier 전역 후처리 `00485e40`/CD `004151d0`, 보호막 생성/해제·사제 낙하·전체 postPop/공간 효과도 남아 있다.
