# Carrier 삭제 준비와 contained 조회 복원

2026-10-08, **마지막 디컴파일 수행 PC: HJOW-Athlon**. AGENTS.md·두 LEFT_JOBS 문서를 읽고 현재/마지막 호스트 일치를 확인했다. 새 `carrierpredestroy` 목록을 같은 PC에서 읽기 전용 Ghidra로 내보냈다(세 판본 각각 7개). 원본 게임/복사본·클론 창 실행, 보호 파일·AGENTS.md·dotnetpj 변경, 커밋/푸시 없이 콘솔/정적 검사를 수행했다.

## 실제 주소와 순서

| 몸체/전역 | 10.78 | CD 10.72·추가 10.37 |
|---|---|---|
| Carrier preDestroy | `00426890` | `004e43b0` |
| contained 조건 조회 | `004ac9f0` | `004ac560` |
| contained finder Begin | `004b2270` | `004eb8d0` |
| contained finder Next | `004b1b20` | `004eb900` |
| 기본 true 가상 필터 | `0044daa0` | `0040f230` |
| Damageable preDestroy(외부) | `0044b4b0` | `004615e0` |
| 인자 없는 전역 후처리(외부) | `00485e40` | `004151d0` |
| 현재 권한 DWORD | `00540bc4` | `00540a2c` |
| finder 타입 DWORD(기본 6) | `00541088` | `0051c978` |

[`RawCarrierPreDestroy`](../../cpppj/src/o/RawCarrierPreDestroy.h)는 같은 flags의 Damageable을 먼저 부른 뒤 **그 시점의 현재 권한**을 읽는다. 권한이 있으면 현재 부모의 contained 조회 `(mask=1, kind=0, index=0)`를 실행한다. 반환은 포인터가 아닌 SID이며 0이 아니면 인자 없는 전역 후처리를 호출한다. 전역 후처리에 this/flags/SID를 새 인자로 붙이지 않는다. 미복원 Damageable와 전역 후처리는 각각 명시적인 훅이다.

조회는 부모 WORD `+6`을 머리로 삼고 종속 WORD `+4`를 따라간다. 다음 링크를 먼저 읽은 뒤 후보 타입 BYTE를 전역 DWORD와 비교한다. 기본값은 6이며 256/0xffffffff를 BYTE로 줄이지 않는다. 기본 true 필터를 통과한 후보에서 `+0x12` WORD가 kind와 같은지, `+0x14` WORD와 mask의 AND가 0이 아닌지 검사한다. kind/mask가 0이면 그 조건은 생략하고 index개의 일치를 건너뛴 뒤 첫 SID 또는 0을 반환한다. WORD 조건에 0x10000 이상의 DWORD 입력을 넣어도 원본 비교를 보존한다. free/dead/void/contained·extra 후보 필터나 Regular/Kernel 조회를 추가하지 않는다.

패치판 Begin의 `005e4794 != 0 && 부모 dead && !boss` assert 경로는 C++ 직접 조회에서 거부한다. CD에는 이 검사가 없다. 일반 직접 자산 preDestroy는 free/contained·form 타입·풀 범위 오류와 누락된 외부 훅을 효과 전에 거부한다. 잘못된 next/순환은 조회 중 거부한다. 첫 일치를 반환한 뒤 아직 방문하지 않은 손상 체인은 읽지 않는다. 전체 파생 가상 지원/일반 Pop의 지원 범위는 확장하지 않았다.

## 실제 사제 삭제 연결

`MakeCarrierPreDestroyHooks`는 같은 풀의 사제 validateCarrier/carrierPre 경계만 실제 Carrier 몸체로 바꾼다. 기존 실제 보호막 조회·삭제 요청을 보존한다. 사제 목록 변경 전 순수 검사를 호출하고, Carrier 직접 호출 때 다시 검사한다. 깊이 감소는 연결한 Damageable 경계의 책임이다.

기존 두 판본 × server/client의 사제 생성자→보호막 조회→회복 Regular/ProcessForm/Kernel→삭제→장부/깊이→SID 반납 검사에 실제 Carrier를 연결했다. Damageable은 실제 공통 lifecycle만 부르는 합성 경계다. 회복 form은 타입 46이므로 기본 contained 타입 6 조회에서는 제외되지만, 이후 실제 공통 종속 정리가 Kernel/form을 제거한다. 종속을 가진 사실만으로 전역 후처리를 호출하지 않는 것을 확인한다. 자산은 합성 void/해시 입력이며 일반 공간 해제와 보호막 전체 파생 수명의 완료를 뜻하지 않는다.

## 독립 기계어 대조

[`decomp_carrierpredestroy_oracle.py`](../../tools/decomp_carrierpredestroy_oracle.py)는 실제 세 PE 각각 **960개**(직접 조회 480개·Carrier 480개), 총 **2,880개**를 정상 반환까지 실행한다. 직접 조회에는 대체가 없다. Carrier에서 Damageable/전역 후처리 두 몸체만 기록 대체한다. 그 밖의 Carrier·조건 조회·Begin·Next·기본 true 가상 필터는 실제 명령이다.

각 PE의 실제 조회/Begin은 720회, Next 1,497회, true 필터 988회, Carrier 480회다. 외부 Damageable 480회·전역 후처리 146회다. Damageable 중 권한·머리·마지막 종속 WORD 변경, 빈 체인·다른 타입·후보 상태·조건 생략·WORD 상한·순번 소진·전체 flags를 포함한다. 정상 EIP/ESP·thiscall 인자·보존 레지스터·허용 코드/스택 쓰기를 검사하고 assert/OS 호출은 0회다. C++는 반환 SID·사건·현재 권한·입력 5개 raw 슬롯 전체를 비교한다. 잘못된 체인/debug assert는 별도 C++ 보호 검사이며 정상 x86 관찰 수에 넣지 않는다.

최종 Release 경고/오류 0·CTest 내부 **307개·실패 0**(101.70초), 감사 **40종 모두 통과**다. 검증 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 이 단계 기록에 남긴다. 로그는 `extracted/cpp-carrierpredestroy-build.log`·`cpp-carrierpredestroy-ctest.log`·`cpp-carrierpredestroy-audits.log`다. 누적 독립 x86 입력은 **202,681개**다.

2026-10-08 후속: [Damageable 효과·소리 접두와 실제 종속 순회](cpp-damageable-predestroy-reconstruction.md)를 완료해 Carrier/사제 연결 검사에 적용했다. 공용 `RawContainedFinder`를 사용하며 기존 Carrier 독립 2,880행도 통과했다. 파편/소리 출력과 권한 해방 생성/배치 내부는 계속 외부 경계다. 다음 작업은 최신 LEFT_JOBS를 따른다. 아래는 앞 단계 인계다.

## 다음 작업

Damageable pre `0044b4b0`/CD `004615e0`는 flags `0x200000`/`0x100000`의 파편·collapse/explosion 소리, 지면과 contained 조회에 따른 priestFree 소리, 권한/flags에 따른 추가 공간 효과 등을 갖는다. 이를 하위 효과별로 나눠 복원하거나 전역 후처리 `00485e40`/CD `004151d0`를 진행한다. 보호막 생성/해제 `00493d30`/CD `0040bfb0`·낙하 `004941f0`/CD `0040c880`·전체 postPop/Unpop/Repop도 남았다. 일반 사제 Pop/비표면 raw GUI와 실제 포획·미션 완주는 계속 미완성이다.
