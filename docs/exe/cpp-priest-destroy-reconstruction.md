# 사제 삭제 준비 복원과 회복 form 정리

2026-10-08, 기준 판본 **10.78**. 마지막 디컴파일 수행 PC: **HJOW-Athlon**(IP 192.168.0.94). AGENTS.md와 두 인계 문서를 읽고 현재 호스트가 마지막 디컴파일 PC와 일치함을 확인했다. 새 `priestdestroy` 목록을 같은 PC에서 읽기 전용으로 내보냈다(세 판본 각각 **4개**). 정밀 디컴파일은 다시 하지 않았다. 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.

[`RawPriestPreDestroy`](../../cpppj/src/o/RawPriestPreDestroy.h)는 **사제 preDestroy 본문**의 목록 압축→보호막 조회→범위 안 SID의 가상 삭제 요청→Carrier 호출을 복원한다. **보호막 공간 조회·가상 삭제의 파생 몸체 및 Carrier→Damageable 효과는 외부 훅**이다. 이 단계를 사제의 전체 삭제/일반 Pop/GUI 완성으로 보지 않는다.

| 동작 | 10.78 | CD / 추가 10.37 |
| --- | --- | --- |
| 사제 preDestroy | `004919b0` | `0040c330` |
| 사제 목록 | `005954c4` | `00549188` |
| 목록 제거 | `0040ea00` | preDestroy 안의 inline 압축 |
| 보호막 SID 조회(외부) | `004918e0` | `0040bf00` |
| 보호막 삭제 helper | preDestroy 안의 범위 검사 | `0040c0d0` |
| 보호막 가상 삭제(외부) | vtable `+0x10`, flags **0** | 동일 |
| Carrier preDestroy(외부) | `00426890` | `004e43b0` |
| 실제 사제 가상 표 | `0050f210` | `005003e0` |

원본의 `extra & 9`가 0일 때 활성 목록의 모든 같은 SID를 제거하고 나머지 순서를 보존한다. 비활성 꼬리 DWORD는 지우지 않는다. 조회 훅에서 돌려준 SID가 **1 이상·풀 크기 미만**이면 flags **0**으로 가상 삭제를 요청한다. 원본은 이 범위에 예약 번호 1~4도 포함하며 여기서 타입/소유자/free/dead를 다시 검사하지 않는다. 정상 월드의 조회 결과는 같은 위치·소유자의 보호막을 가리켜야 한다. C++ 훅은 원본 SID 폭인 16비트 번호를 반환한다. 임의의 32비트 정수를 포인터 감김으로 유효 주소에 매핑하는 손상 입력은 지원하지 않는다.

조회/삭제 훅 뒤 extra가 바뀌어도 이미 선택한 분기의 삭제 요청을 계속하고, 마지막 Carrier에는 처음 받은 flags를 그대로 넘긴다. 추상/매장 상태면 목록/보호막 단계를 건너뛰고 Carrier만 호출한다. 공통 Destroy가 먼저 dead를 켜므로 **dead/void 자체는 preDestroy를 생략하는 조건이 아니다.** 깊이 감소는 연결된 Carrier의 공통 preDestroy 완료에 맡긴다.

C++의 SID/실제 사제 타입/가상 표/free·contained 상태/활성 목록 범위 보호와 순수 `validateCarrier` 검사는 첫 목록 변경 전에 실행한다. 이 검사는 원본의 추가 효과가 아니며 raw/목록을 변경하면 안 된다. `MakePriestPreDestroyHooks`는 실제 사제의 PreDestroy만 분배하고 선택·나머지 사건·form Unpop은 전달받은 훅을 유지한다. ProcessHost의 자산 훅으로 연결할 수 있다.

[`decomp_priestdestroy_oracle.py`](../../tools/decomp_priestdestroy_oracle.py)는 세 PE 각각 **2,184개**, 총 **6,552개**의 전체 preDestroy 정상 반환을 관찰했다. 목록 압축·CD 보호막 삭제 helper·SID 범위 검사는 실제 명령이며 조회/보호막 가상 삭제/Carrier 세 경계만 대체한다. 각 PE에서 실제 목록 제거/helper **879회**, 조회 **879회**, 보호막 삭제 요청 **547회**, Carrier **2,184회**를 기록한다. 반환 EIP/ESP·thiscall 인자·보존 레지스터와 허용 코드/쓰기 범위를 확인하며 assert/OS 호출은 0회다. 활성/비활성 중복·dead/void·추상/매장·flags 전체 비트·SID 0/1/4/5/self/풀 끝/65535·조회 중 extra 변경을 포함한다. C++는 사건 순서/인자·count·목록 저장소 전체·raw 슬롯 전체를 독립 fixture와 대조한다.

연결 검사는 두 판본의 server/client 실제 사제 생성자→공통 postPop 장부→사제 prefix→실제 Regular/ProcessForm·Kernel 등록→새 preDestroy→공통 Destroy 종속 순회→Kernel 제거와 form/자산 SID 반납을 실행한다. 보호막은 타입 167의 **합성 void 자산**이며 조회/파생 Carrier·Damageable pre/post 효과는 대체하고 기존 공통 장부 몸체를 직접 호출한다. **일반 사제/보호막의 공간 및 파생 삭제 효과를 복원한 검사가 아니다.** 보호막의 중첩 삭제 및 이미 dead인 부모 재삭제가 깊이를 깨뜨리지 않고, 목록/회복 Kernel/현재 타입 수/비용이 정리되며 누적 생산 수는 유지됨을 검사한다.

후속은 보호막의 실제 공간 조회 `004918e0`/CD `0040bf00`·생성/해제 `00493d30`/CD `0040bfb0`, Carrier/Damageable pre/post의 하위 효과, 사제 낙하 `004941f0`/CD `0040c880`, 전체 postPop와 Unpop/Repop이다. 이 경계가 갖춰진 뒤 일반 Pop/비표면 raw GUI를 연결한다. 장시간 원본/클론 창·픽셀·mutations 검사는 기존 인계를 유지한다.

최종 Release 빌드 **경고/오류 0**, CTest 내부 **299개·실패 0**(100.12초), 감사 **38종 모두 통과**. 새 독립 입력 6,552개를 포함한 누적은 **190,873개**다. 로그는 `extracted/cpp-priestdestroy-build.log`·`cpp-priestdestroy-ctest.log`·`cpp-priestdestroy-audits.log`에 있다.

```powershell
# 기존 로컬 구성의 Release 빌드/전체 회귀
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
# 저장된 독립 증거 감사(생성에는 같은 PC의 Ghidra 내보내기가 필요하다)
python -X utf8 tools/decomp_priestdestroy_oracle.py --verify
```
