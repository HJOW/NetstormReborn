# cpppj 다리 삭제 전후 훅과 낙하 이벤트 좌표

2026-10-07. **1차 기준은 10.78**이다. 다른 AI의 [붕괴 스캔/수명 복원](cpp-bridgedecay-reconstruction.md)을 이어서 다리 preDestroy/postDestroy의 판단·효과 순서와 지연 낙하 이벤트의 좌표 포장을 복원했다.

소스: [RawBridgeLifecycle.h](../../cpppj/src/o/RawBridgeLifecycle.h), [RawBridgeLifecycle.cpp](../../cpppj/src/o/RawBridgeLifecycle.cpp). 검사: [BridgeLifecycleTests.cpp](../../cpppj/tests/BridgeLifecycleTests.cpp). 독립 결과: [bridgeeffects-x86.tsv](../../cpppj/tests/fixtures/bridgeeffects-x86.tsv), [감사 기록](../../cpppj/recovery-bridgeeffects-evidence.json), [실행 도구](../../tools/decomp_bridgeeffects_oracle.py).

**실제 삭제/낙하의 완료를 뜻하지 않는다.** raw 풀에서 현재 필드를 읽고 원본 순서로 외부 효과를 호출하는 어댑터다. 일반 탐색·기본 Squid 삭제·walker 가상 낙하·소리·제거 통지는 호출자가 연결한다. 기존 GUI GameWorld는 아직 임시 객체 모델이며 이 어댑터를 사용하지 않는다.

**2026-10-07 후속:** [일반 공간 탐색](cpp-rawfinder-reconstruction.md)의 실제 4단계 해시/next·동적 필드를 복원하고 `MakeBridgeLifecycleHooks`로 연결했다. 새 독립 x86 1,776개는 Begin/Next를 대체하지 않는다. 아래 6,170개·탐색 대체 범위·157개·창 허용은 이 문서의 당시 검사 기록이며 기존 fixture를 변경하지 않았다. 기본 삭제/낙하/소리·raw GameWorld는 계속 후속이다.

## 원본 함수

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| preDestroy | `004221b0` | `004498e0` |
| 연결 객체 참조 검사 | `00422290` | preDestroy에 인라인 |
| postDestroy | `00422300` | `00449c60` |
| 낙하 이벤트 등록 래퍼 | `00421f90` | `004490b0` |
| 좌표 포장/등록 | `00421530` | 래퍼에 인라인 |
| 주변 좌표 보정 | `0041d7a0` | `00440a80` |
| 칸 위 좌표 절삭 | `0041d770` | `00440a50` |

읽기 전용 Ghidra로 `extracted/bridgeeffects/<판본>/creation.c`·`functions.tsv`를 새로 내보냈다. 패치 12개·CD/10.37 각 7개이며 타입 genus 조회와 CRT `_ftol`도 포함한다. 프로젝트에 변경을 저장하지 않았다. 세 완료 로그는 같은 폴더의 `<판본>-ghidra.log`다.

## preDestroy와 좌표 보정

abstract/buried(extra & 9)가 아니면 주변 ±1칸 탐색을 시작한다. 특정 타입(`005412c4` / CD `0051cbb0`)만 연결 검사에 넘기며, 마지막에는 extra와 무관하게 공통 preDestroy를 호출한다. 인자 flags는 공통 단계에 그대로 전달한다.

연결 객체의 **첫 참조는 +12 WORD, 두 번째는 +8 WORD**다. 둘 다 `1..capacity-1`에 있을 때 첫 참조의 extra & 9가 0이고, 두 참조 중 하나가 dead(state & 2)이면 그 연결 객체의 destroy(0)를 호출한다. 두 번째 참조의 extra, 참조의 free/void는 추가로 거르지 않는다. 링크 자체가 이미 dead여도 helper는 같은 가상 호출을 할 수 있으며 중복 삭제 방지는 기본 destroy의 책임이다.

탐색 중심은 단순 절삭이 아니다. 실제 명령은 `trunc(double(float좌표) + double(0.9999899864196777f))`다. 상수 비트는 **`3f7fff58`**, 패치 `00502310` / CD `005017a0`에 있다. 표면 조회의 `0.9999f`와도 다르다. C 디컴파일에는 `fadd`가 빠져 있었다. C++의 중간 덧셈을 float로 좁히면 경계에서 틀릴 수 있으므로 double로 계산한다.

첫 검사에서 무보정 절삭을 쓴 C++이 탐색 범위 504건에서 실패했다. 기계어를 다시 확인해 수정하고 `20.00002`/`20.000005` 등 두 상수를 구별하는 경계 입력을 추가했다. 독립 결과는 원본 명령으로 생성했다.

## postDestroy와 동적 효과 순서

일반 다리에서는 **제거 통지 → 원본 좌표의 `bridgeFall.wav` → 칸 위 탐색 → walker마다 vtable +200 → 공통 postDestroy** 순서다. 칸 위 좌표는 보정 없이 0 방향으로 절삭한다. walker는 genus의 `0x10000` 비트로 판단한다. abstract/buried이면 공통 postDestroy만 호출한다.

`BridgeLifecycleHooks.begin/next`가 탐색 순서와 0 종료 표식을 공급하며 `emit`이 외부 효과를 소비한다. 앞선 destroy/fall 콜백은 같은 풀을 바꿀 수 있다. 각 후속 판단은 그 시점의 타입/참조/state를 다시 읽는다. 이 순서를 고정 스냅샷으로 대체하지 않는다. 제거 통지/소리 이후 좌표도 다시 읽는다.

`NotifyRemoval`의 내부 목록 효과(`00460600` → `004604a0`)와 공통 pre/postDestroy 전체는 이 단계에서 실행하지 않는다. 원본 32비트 vtable 주소는 호스트에서 호출하지 않는다. 잘못된 자기 SID·누락 콜백·범위 밖 좌표/타입은 안전하게 거부하며, 콜백 이후 오류까지 전체 롤백한다고 보장하지 않는다.

## 지연 낙하 이벤트

이벤트 번호는 **`0x2692`**다. 좌표마다 원본 `_ftol`의 하위 DWORD를 얻고 `(x & 255) | (y << 8)`로 포장한다. 이 DWORD를 **signed int의 수치로 float 변환**해 payload로 저장한다. 정수 비트를 float 비트로 재해석하는 방식이 아니다. 정상 월드 0..255에서는 `x + 256*y`가 정확히 보존된다.

`DelayedFallPayload`는 이 계산만 복원한다. 이벤트 프로세스 생성·현재 시각 저장·실행/취소·끝 칸 변환 `004215d0`는 후속이다. 패치 래퍼와 직접 포장 함수를 모두 실행해 결과가 같은지도 확인했다. CD/10.37은 래퍼 내부의 실제 포장 명령을 실행했다.

## 독립 대조 범위와 재현

| 입력 종류 | 새 x86 입력 |
|---|---:|
| 연결 helper | 882 |
| preDestroy | 2,520 |
| postDestroy | 1,800 |
| 낙하 이벤트 래퍼 | 726 |
| 직접 포장 함수 | 242 |
| 합계 | **6,170** |

세 실제 PE × x87 53/64비트다(별도 연결 helper/직접 포장은 패치판만). 무효 참조·첫/둘째 extra 비대칭·free/void/dead 조합·빈/역순/중복 탐색·앞선 삭제로 다음 링크 상태가 바뀌는 사례·낙하 콜백의 타입 변경·음수/소수/바이트 경계/비유한 payload를 포함한다. 모든 호출의 정상 반환·스택·x87 TOP/제어 워드·FS 예외 목록 복구를 확인한다. 원본 assert 도달/OS 호출은 0이다.

**대체 범위:** 일반 탐색 begin/next는 준비한 순서의 번호를 공급한다. destroy/fall은 호출 사실을 기록하고 dead/type만 바꾼다. 공통 pre/postDestroy·소리·제거 통지·할당·이벤트 등록도 진입점 대체다. 탐색 범위의 좌표 보정/절삭, 참조 검사, genus 조회, 순서와 좌표 포장은 원본 명령이다. 전체 raw 풀에서의 일반 탐색 결과나 실제 삭제·낙하·소리 성공을 증명하지 않는다. 제한 실행기는 새 내보내기의 불연속 몸체 밖 명령과 허용하지 않은 메모리 쓰기를 거부한다. 앞선 독립 도구/기대값은 보존했다.

```powershell
python -X utf8 tools/decomp_bridgeeffects_oracle.py
python -X utf8 tools/decomp_bridgeeffects_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```

C++ 검사는 저장된 fixture만 읽으므로 원본 PE·Python·Ghidra가 필요 없다. 일반 탐색기와 raw GameWorld를 연결한 뒤 붕괴 → 삭제 준비 분할/Unpop/반납 → walker 낙하·소리·표시를 실제 미션에서 확인해야 한다. 이 단계의 클론 창 검사는 기존 임시 월드의 회귀이며 새 raw 삭제/붕괴의 플레이 검증으로 세지 않는다.

## 이번 검증 결과와 창 실행 범위

최종 x64 Release 경고/오류 0·CTest 내부 **157개·실패 0**(51.89초). 이전 152개에 새 5개를 더했고 누적 제한 x86 입력은 **81,816개**(75,646 + 6,170)다. 기존 붕괴 도구/fixture 감사도 통과했다. 검사 로그는 `extracted/bridgeeffects/build.log`·`ctest.log`에 있다.

사용자가 **이번 작업에 한해** 호스트 `DESKTOP-HJOW`의 창 실행을 허용했다. `tools/cpp_world_smoke.py`로 새 클론 창의 캠페인 1-1 진입·선택·1.8칸/초 이동·일시정지·우클릭 메뉴·허공 이동 거부·카메라/Home·복귀/재진입과 TEST01 시작 SP 50,000·사제 도착을 확인했다. **20개 조작 상태**, 초기 자료 **6개·393,216 마스크 바이트·2,593개 객체**를 대조했고 선택 화면도 확인했다. 새 raw 다리 훅을 GUI에 연결한 검사는 아니다. 원본 게임/복사본·업데이터/설치 도구는 실행하지 않았다.

이번 보고서/이미지는 `extracted/bridgeeffects/world-smoke-report.json`·`selected.bmp`·`TEST01.bmp`에 보존했다. 원본 여섯 디렉터리의 **2,782개 파일 SHA/목록이 작업 전과 같고**, 허용된 설정도 원래 바이트/존재 상태로 복구했다. 검사 클론 창은 종료됐다. AGENTS.md·C#·LEFT_JOBS.dotnetpj.md는 변경하지 않았다. **이번 창 허용은 다음 작업까지 연장된 것으로 해석하지 않는다.**
