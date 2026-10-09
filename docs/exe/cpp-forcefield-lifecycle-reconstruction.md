# 보호막 일반 Pop·Regular 애니메이션·공간 삭제 수명

2026-10-10. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** AGENTS.md·두 인계·cpppj 계획/관련 문서를 확인했다. 같은 PC에서 새 `forcefieldpostpop` 목록 **7/6/6개**를 읽기 전용으로 내보냈다. 10.78 싱글플레이 우선, Windows 안정 구동 2차, outpost/LAN 3차, 한국어/요구사항/MCP 4차를 유지했다. 게임/창은 실행하지 않았다.

[`RawForcefieldPostPop`](../../cpppj/src/o/RawForcefieldLifecycle.h)을 `SquidPostPop::SetForcefieldPostPop`에 명시 등록하면 실제 보호막 타입 167/가상 표의 일반 Pop이 전용 예약 접두→공통 후처리로 이어진다. [`RawForcefieldRegular`](../../cpppj/src/o/RawForcefieldLifecycle.cpp)은 애니메이션 진행과 같은 위치의 사제 존재 검사를 처리한다. 보호막 생성 wrapper의 Pop과 삭제 요청을 실제 공간/form/Kernel 모듈에 연결해 검사했다. 전체 월드/GUI·오디오의 완료는 아니다.

## 원본 대응과 계약

| 동작 | 10.78 | CD·추가 10.37 |
|---|---|---|
| 타입 167 생성자 / 실제 가상 표 | `0044ab00` / `00507770` | `00483da0` / `005042b8` |
| 전용 postPop (+0x20) | `00448d20` | `004847d0` |
| Regular 처리기 (+0x5c) | `00448e00` | `004848c0` |
| 좌표의 첫 타입 일치 finder | `004b1fa0` | `004eb4b0` |
| 공통 Pop / firstPop | `004b02d0` / `004ad470` | `004ad490` / `004ae0f0` |
| 공통 postPop | `004b0d30` | `004ae180` |
| 공통 destroy (+0x10) | `004af780` | `004ab7e0` |
| 공통 pre/postDestroy (+0x14/+0x18) | `004b0950` / `004b0840` | `004add20` / `004adbe0` |
| 공통 Unpop (+0x48) | `004afe50` | `004ad0b0` |
| 프레임 진행 | `004afc90` | `004acce0` |
| 현재 사제 타입 DWORD | `005412d0` (초기 158) | `0051cbbc` (초기 158) |

실제 PE의 보호막 가상 표는 수정하지 않았다. 보호막의 Pop·표시·Unpop·소유자·삭제는 기존 공통 구현과 같지만 postPop/Regular 처리기는 전용 함수다. `SquidPop::Supports`의 전역 허용 표를 확장하지 않고 전체 예약 접두를 공급한 해당 인스턴스만 지원한다. 다른 풀의 등록과 미연결 일반 Pop은 공간 쓰기 전에 거부한다. 직접 접두/Regular 호출도 SID/free/contained/타입/가상 표를 검증한다.

postPop은 `flags & 1`일 때 `new(0x28)`→Regular 사건 **1·payload 0.08f**, 다시 독립 `new(0x28)`→Regular 사건 **2·payload 0.01f**를 요청한다. 첫 확보 실패여도 두 번째 확보/공통 base는 계속한다. 기존 예약을 검색하지 않으며 접두 자체는 extra 1/8/9로 차단하지 않는다. 실제 Pop이 정규화한 flags를 그대로 공급한다. 접두 뒤 공통 후처리가 장부/깊이를 한 번 처리한다.

`MakeForcefieldPostPopProcessHooks`는 확보 여부 입력을 보존하면서 실제 `SquidProcessHost::AddRegular`(타입 46)·부모 종속 form·Kernel에 연결한다. 모든 풀/상태/해시/타입/효과 대상과 등록한 접두는 연결보다 오래 살아야 한다. 예약 두 개와 공간/외부 효과를 포함한 전체 호출의 실패 되돌리기는 제공하지 않는다.

## Regular 사건과 현재 공간 조회

사건 **1**은 실제 프레임 진행 `(delta=1, flags=0)`을 호출하고 `0.08f`를 반환한다. 진행의 감싸기 반환은 읽지 않는다. `MakeForcefieldFrameHooks`로 [실제 프레임 진행/지정/표시/공간](cpp-frame-advance-reconstruction.md)을 연결한다. 주기 비트는 `0x3da3d70a`다.

사건 **2**는 현재 x/y와 현재 사제 타입 DWORD로 일반 finder를 만든다. 좌표는 기존 helper의 float→signed int→float→signed int, 즉 정상 범위에서 **0방향 절삭**이다. 0.9999f 덧셈을 사용하지 않는다. 정수 점 사각형/flags 0/기본 true 필터→현재 단계/버킷/next 순서에서 **타입만 일치하는 첫 SID**를 찾는다. 소유자·HP·free/dead/void 상태를 별도 필터로 추가하지 않는다. 매장 등 기본 finder의 조건은 그대로 적용한다. 반환 후보와 사제 타입 DWORD를 BYTE로 줄이지 않으므로 256은 타입 0과 같지 않다.

사제가 있으면 `0.01f`(`0x3c23d70a`), 없으면 자기 가상 destroy에 flags 0을 요청하고 **-1.0f**를 반환한다. 삭제 뒤 슬롯/프로세스를 다시 쓰지 않는다. 실행 중인 Regular를 포함한 두 종속 form은 부모의 공통 destroy→실제 `SquidProcessHost::Hooks`에서 제거된다. 기존 `RunRegular`의 Kernel 번호 확인이 처리기 안에서 지워진 객체를 재예약하지 않게 한다.

다른 사건은 0.0f다. 패치에서 원본 debug 전역이 켜져 있으면 assert를 보고하므로 C++은 예외로 진단한다. CD에는 그 debug assert가 없다. count/payload는 읽지 않는다. 일반 Regular의 시각/예약·종료 규칙은 기존 구현을 재사용한다.

## 독립 원본 관찰과 C++ 합성 검사

[`decomp_forcefieldpostpop_oracle.py`](../../tools/decomp_forcefieldpostpop_oracle.py)는 실제 postPop/Regular 처리기·좌표 타입 helper·일반 finder/CRT를 정상 반환까지 실행한다. 각 PE **1,208개**, 세 판본 **3,624개**, 두 x87 정밀도에서 같은 관찰을 저장했다. 판본별 접두 입력 128개·Regular 입력 1,080개이며 실제 정상 반환은 접두 **256회**, 처리기 **2,160회**, 총 **7,248회**다. [fixture](../../cpppj/tests/fixtures/forcefieldpostpop-x86.tsv), [근거 JSON](../../cpppj/recovery-forcefieldpostpop-evidence.json).

접두는 flags 8종·두 확보 성공/실패·extra 4종을 교차한다. Regular는 사건 1/2/99·소수/보드 가장자리 좌표·현재 사제 타입 158/159/256·빈/다른 타입/매장/dead/void/다른 소유자/발자국 교차/다른 단계 후보·count/payload(NaN 비트 포함)를 교차한다. Python/C++은 존재/반환/예약의 기대 분기를 계산하지 않는다. 효과 순서/인자·raw 전체·float 반환 비트·ABI/비영 보존 레지스터/SEH/x87 제어·TOP/허용 실행·쓰기/assert·OS 0/SHA/정확한 입력·실제 진입을 감사한다.

각 PE 실제 좌표 타입 helper/finder 시작 **720회**, 다음 순회 **1,080회**다. 명시 대체는 common postPop **256회**, new **256회**, Regular 생성 **128회**, 프레임 진행 **720회**, 가상 자기 삭제 **528회**다. 후처리의 공통 장부·Regular 생성/form/Kernel·진행/삭제는 앞 단계 독립 근거와 이번 C++ 합성에서 실제 모듈로 실행한다. 원본 실행은 타입/해시/raw 합성 입력이며 전체 일반 Pop·오디오·GUI를 원본 프로세스에서 실행한 것은 아니다.

[`ForcefieldLifecycleTests.cpp`](../../cpppj/tests/ForcefieldLifecycleTests.cpp)의 새 검사 5개는 세 PE 관찰 재생, 두 raw 배치의 실제 factory/owner·보호막 생성 wrapper→Pop/예약·중복 생성 방지→프레임 진행/Display→사제 실제 Unpop→존재 검사에서 자기 삭제→실제 공통 pre/postDestroy/장부·Unpop/SID 반납/종속 form·Kernel 제거→재생성/외부 삭제를 검사한다. 사제는 합성 등록 입력과 실제 vtable이며 보호막은 실제 생성자를 사용한다. 보호막의 두 프레임/SHP·타입/비용은 합성 입력이다. 소리 확보 실패 경로를 사용하며 오디오 출력/프로세스는 미복원이다.

최종 검증: Release 경고/오류 **0**, CTest 내부 **467개·실패 0**(121.04초), 원본 근거 감사 **71종 모두 통과**. 누적 인계 **384,382 + 3,624 = 388,006개**. 로그 `extracted/forcefieldpostpop-{export,oracle,build-final,ctest-final}.log`·`forcefieldpostpop-audits.json`.

## 재현과 후속

```powershell
tools/ghidra/export_functions.ps1 -Name forcefieldpostpop
python -X utf8 tools/decomp_forcefieldpostpop_oracle.py
python -X utf8 tools/decomp_forcefieldpostpop_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

보호막 일반 Pop/애니메이션·존재 검사/공통 삭제 연결은 이번에 완료했다. 보호막 SfxProcess/파일 조회·실제 소리 재생과 정지 수명, 사제/보호막이 실제 월드/GUI에서 이 raw 모듈을 사용하는 수명 연결, 저장 맵 비표면 객체·미션 목록 수명, GUI 건설·경제·전투·승패는 후속이다. 현재 미션 완주는 불가능하다. 실제 자산 전수·장시간 변이·최대 지도/SID 소진·창 픽셀·Windows 반복 실행은 별도 검증이다. 영어=원본 글꼴 캐시·한국어=D2Coding 정책과 Unicode 한국어 출력의 4차 인계를 유지한다.
