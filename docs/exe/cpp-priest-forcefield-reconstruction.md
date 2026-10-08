# 사제 보호막 조회와 삭제 준비 연결

2026-10-08, 기준 판본 **10.78**. 마지막 디컴파일 수행 PC: **HJOW-Athlon**(IP 192.168.0.94). AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 현재 PC와 마지막 디컴파일 PC가 같음을 확인했다. 새 `priestforcefield` 목록을 읽기 전용으로 내보냈다(10.78 **12개**, CD/추가 10.37 각각 **10개**). 정밀 디컴파일은 다시 하지 않았다. 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.

[`RawPriestForcefield`](../../cpppj/src/o/RawPriestForcefield.h)는 사제 보호막 조회의 **좌표 절삭→한 점의 일반 finder→타입/소유자 필터→첫 일치 SID**를 복원한다. 기존 [사제 preDestroy](cpp-priest-destroy-reconstruction.md)의 조회 훅을 `MakePriestForcefieldHooks`로 연결한다. **직접 조회에는 외부 대체가 없다.** 보호막 생성/해제와 가상 삭제·Carrier/Damageable의 파생 효과·일반 사제 Pop/공간 해제·GUI는 후속이다.

| 동작 | 10.78 | CD / 추가 10.37 |
| --- | --- | --- |
| 사제 보호막 조회 | `004918e0` | `0040bf00` |
| 좌표 절삭 helper | `0041d770` | `00440a50` |
| CRT 정수 절삭 | `004e49c0` | `004f161c` |
| 일반 Begin / Next | `004b16d0` / `004b1810` | `004eae20` / `004eafe0` |
| 기본 true 필터 | `0044daa0` | `0040f180` |
| 기본 finder 가상 표 | `00502318` | `005003d0` |
| 보호막 타입 전역(DWORD) | `005412f4` | `0051cbe0` |
| owner BYTE | raw `+34` | raw `+32` |
| 연결한 사제 preDestroy | `004919b0` | `0040c330` |

원본은 raw x/y float를 복사하고 각각 `float→정수(0방향 절삭)→float`로 저장한 뒤 정수로 다시 읽는다. 이 조회는 **좌표 편향을 더하지 않는다.** [HP/지면 조회](cpp-priest-state-reconstruction.md)의 0.9999f/0.99999f와 구별한다. C++는 유한한 signed 32비트 정상 범위에서 같은 정수 좌표를 사용하며, NaN/무한대/범위 밖 입력은 예외로 거부한다.

정수 `(x,y,x,y)`와 flags **0**으로 실제 [RawSquidFinder](cpp-rawfinder-reconstruction.md)를 시작한다. 탐색 순서는 **level→y→x→버킷 next**이며, 기본 true 필터와 실제 타입 발자국의 교차 조건을 사용한다. 넓은 발자국은 사제와 float 좌표가 달라도 해당 점을 덮으면 후보다. `extra & 8`의 매장 후보는 finder가 제외한다. **free/dead/void/contained 상태는 추가로 제외하지 않는다.** 사제 자신의 추상/매장·dead/void 역시 직접 조회를 생략하는 조건이 아니다. C++에서는 호출 대상 사제의 실제 타입/가상 표·SID·free/contained를 보호한다.

후보의 type BYTE를 **DWORD 보호막 타입 전역**과 비교하고, 같으면 현재 사제와 후보의 owner BYTE를 비교한다. 첫 일치 SID를 반환하고 전체 소진이면 0이다. 타입 전역 256/0xffffffff를 BYTE로 줄이면 잘못된 객체를 고르게 되므로 원본 폭을 유지한다. 소유자 비교에는 0~255의 BYTE가 그대로 사용된다. 매 조회에 독립 finder 커서를 만들며 실제 풀/해시·타입 전역을 사용한다. 타입 발자국은 생성 시 복사한 타입 표 입력이다.

일반 Begin의 경계 규칙도 유지한다. 특히 좌표 -1.x의 절삭값 **-1**은 오른쪽/아래 인자의 보드 끝 sentinel로 해석될 수 있어 탐색 범위가 한 줄/열 전체로 넓어진다. 보드 밖 사제의 입력과 후보의 잘못된 좌표는 별개다. 이번 후보 좌표는 두 판본에서 유효한 범위이며, C++의 후보 좌표/발자국/체인 보호는 기존 finder 계약을 따른다.

[`decomp_priestforcefield_oracle.py`](../../tools/decomp_priestforcefield_oracle.py)는 **1,488개 장면**에서 조회/삭제 준비를 세 PE와 두 x87 정밀도로 실행한다. 각 PE **2,976개**, 총 **8,928개**의 독립 관찰이다. 좌표 helper/CRT·Begin/Next·기본 true 필터·타입/소유자 순회·pre 목록 압축/범위 검사는 실제 명령이다. **조회에는 대체가 없고 Pre에서는 보호막 가상 삭제와 Carrier 두 경계만 기록 대체한다.** 코드/쓰기 범위·EIP/ESP·thiscall 인자·보존 레지스터·x87 제어/TOP을 검사하며 assert/OS 호출은 0회다.

각 PE의 실제 조회/좌표/Begin은 두 정밀도를 합쳐 **3,970회**, Next **19,050회**, 기본 true 필터 **17,008회**다. 사제 preDestroy **2,976회** 중 목록 제거/helper **994회**를 실제 실행했고 외부 보호막 삭제 **482회**, Carrier **2,976회**를 기록한다. 같은 자리의 여러 타입/owner·등록 순서·네 해시 단계·발자국·매장/나머지 상태·소수/경계/음수 좌표·DWORD 타입 폭을 포함한다. C++는 반환 SID·목록 저장소 전체·사건 순서/인자·사제와 후보 raw 슬롯 전체를 비교한다.

이전 삭제 연결 검사도 수동 보호막 SID 반환에서 실제 조회로 바꿨다. 두 판본 server/client의 실제 사제 생성자→회복 prefix→ProcessForm/Kernel→사제 preDestroy→실제 finder 조회→보호막 삭제 요청→공통 장부·회복 form/Kernel 제거·SID 반납을 합성한다. 공간 등록은 **합성 void 자산 입력**이며 실제 일반 Pop/Unpop·해시 해제와 보호막 파생 삭제의 완성 검사가 아니다. 보호막 가상 삭제/Carrier·Damageable pre/post 효과는 명시적으로 대체하고 공통 삭제 장부/깊이/반납 몸체를 실행한다.

후속은 Carrier pre `00426890`/CD `004e43b0`→Damageable pre `0044b4b0`/CD `004615e0`의 하위 효과, 보호막 생성/해제 `00493d30`/CD `0040bfb0`, 사제 낙하 `004941f0`/CD `0040c880` 및 전체 postPop·Unpop/Repop다. 이 경계를 갖춘 뒤 일반 Pop/비표면 raw GUI와 보행 진행/종료를 연결한다.

최종 Release **경고/오류 0**, CTest 내부 **303개·실패 0**(93.06초), 감사 **39종 모두 통과**, 누적 독립 x86 **199,801개**다. 로그는 `extracted/cpp-priestforcefield-build.log`·`cpp-priestforcefield-ctest.log`·`cpp-priestforcefield-audits.log`다.

```powershell
# 기존 로컬 구성의 Release/전체 회귀
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
# 새 독립 증거 감사
python -X utf8 tools/decomp_priestforcefield_oracle.py --verify
```
