# cpppj flag 0 첫 연결 이웃과 다리 끝 칸 변환 연결

2026-10-08, `HJOW-Athlon`. 기준은 10.78이다. 원본 게임·복사본·클론 창을 실행하지 않고 실제 PE의 제한 x86 명령과 콘솔 검사로 진행했다.

[RawSquidNeighbors](../../cpppj/src/o/RawSquidNeighbors.h)는 현재 raw 풀·일반 해시·spot에서 첫 연결 이웃을 구한다. [MakeBridgeNeighborHooks](../../cpppj/src/o/RawBridgeEvents.cpp)는 이 조회와 동일한 프레임 표를 끝 칸 변환에 연결한다. [NeighborTests](../../cpppj/tests/NeighborTests.cpp)와 [BridgeEventTests](../../cpppj/tests/BridgeEventTests.cpp)가 독립 기대값 및 실제 raw 모듈 연결을 검사한다.

## 인수인계의 flag 8 설명 정정

`004215d0`의 호출은 **`004b23e0(현재 객체 번호, 0)`**이다. CD `00449a20`의 인라인 끝 칸 변환도 마지막 인자로 **0**을 넘긴다. 앞선 문서와 주석의 “flag 8 표면 이웃” 설명은 잘못됐다. 기존 이벤트 fixture는 호출 인자 0을 이미 기록했지만 이웃 조회를 대체했으므로 두 탐색 경로의 차이를 검증하지 못했다.

생성자는 `004b1d70`/CD `004ebf30`에서 source 발자국 내부 비트를 확인하고 첫 조회를 수행한다. 결과가 **생성 직후 +0x34**에 저장되므로 후속 Next가 필요 없다. flag 0은 일반 Begin/Next에 연결 필터를 적용한다. flag 8의 0단계 지도 순서·중복 제거로 대신하면 첫 번호가 달라지고 다른 단계에 등록된 큰 발자국 후보를 놓칠 수 있다.

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| source 기반 생성자 | `004b23e0` | `004ebad0` |
| 첫 조회와 내부 발자국 검사 | `004b1d70` | `004ebf30` |
| 일반 Begin / Next | `004b16d0` / `004b1810` | `004eae20` / `004eafe0` |
| 연결 필터 | `004b1e80` | `004ec040` |
| 가로·세로 교차 XOR | `0041d9b0` | `0043fe10` |
| 중심 방향 / 프레임 접합 | `0041ce90` / `00441e40` | `0043f620` / `004d2e00` |

## 복원한 조회

source의 float 발자국·타입·현재 프레임·중심 좌표를 호출 때 읽는다. 발자국의 왼쪽/위 모서리는 원본처럼 1..255로 보정하고 전체 spot을 AND한다. 내부 비트 8이 남으면 바로 0을 반환한다. 그 밖에는 발자국을 한 칸 넓힌 정수 사각형으로 [RawSquidFinder](cpp-rawfinder-reconstruction.md)를 시작한다.

일반 탐색의 **단계 0→3·y→x·raw next 체인** 순서를 유지하며 buried 후보를 제외한다. 연결 필터는 표면 타입에 대해 후보 사각형을 가로/세로로 각각 한 칸 넓혀 source와 교차하는지 검사하고, **정확히 한 축만 교차**할 때 진행한다. 두 저장 중심 float의 차이로 방향을 정하며 동률은 세로다. 후보 프레임→source 프레임의 접합을 검사하고 마지막으로 dead와 후보 기준점의 내부 비트를 제외한다. free/void/abstract를 추가로 제외하지 않는다.

첫 성공에서 끝나므로 그 뒤의 후보·다른 버킷·손상된 next를 미리 읽지 않는다. 조회는 풀·해시·spot을 변경하지 않으며 별도 일반 finder를 만들어 삭제 훅의 탐색 커서를 덮지 않는다. 끝 칸 변환에서 잠시 쓴 L/M/N/O 프레임을 바로 읽고, 다음 호출에서는 변경된 프레임/체인을 다시 읽는다.

C++ 보호 계약은 지도 안 **1 이상·256 미만의 유한 좌표**, 양수 정상 발자국·자산 타입·유효 프레임이다. 원본 assert·비정상 주소·손상된 순환은 예외로 거부한다. flag 8 반복자나 flag 1/2/4 조합의 파생 필터 전체를 구현한 것은 아니다.

## 독립 x86 대조

[decomp_neighbor_oracle.py](../../tools/decomp_neighbor_oracle.py)는 기존 `bridgeevent`와 `graphremove`/`geometry`의 읽기 전용 Ghidra 내보내기를 재사용한다. 탐색기 대체를 제거하고 실제 생성자·일반 Begin/Next·연결 필터·발자국·기하·접합을 실행한다. 세 PE × 288개 입력 × 첫 이웃/이벤트 두 경로 = **1,728개 관찰**이다. 모든 입력을 x87 53/64비트에서 실행하고 두 관찰이 같을 때 한 행만 저장한다.

| 관찰 | 개수 | 비교 대상 |
|---|---:|---|
| 생성 직후 첫 이웃 | 864 | +0x34의 실제 번호 |
| 실제 탐색을 넣은 다리 이벤트 | 864 | 반환 float·효과 순서/인자·옛 슬롯/새 슬롯의 모든 바이트 |

입력은 J/K·hard 플래그 변형·정수/소수 좌표·지도 가장자리·네 해시 단계·큰 발자국·dead/buried/비표면/내부·등록 순서·권한 없음이다. 원본 assert 도달과 두 x87 정밀도의 관찰 차이는 0이다. 입력 상태는 합성이고 일반 Pop의 실제 등록을 이 x86 묶음에서 실행하지 않는다. 이벤트의 생성·삭제·소유자·Pop·표면 알림도 기존 외부 효과 대체를 유지한다.

[neighbor-x86.tsv](../../cpppj/tests/fixtures/neighbor-x86.tsv), [근거 JSON](../../cpppj/recovery-neighbor-evidence.json)에 원본 PE·도구·내보내기·결과 SHA와 대체 호출 수를 보존했다. `--verify`는 저장된 SHA/행 수/탐색 대체 없음의 감사이며 기계어 재실행은 아니다.

## 다리 전용 postPop의 접두

실제 다리 vtable `005034c8`/CD `00501ab0`의 +0x20은 공통 postPop이 아니라 **`00422150`/CD `00449890`**이다. 기존 SquidPop이 이 표를 거부하던 이유가 맞았으므로 공통 가상 표 목록을 바꾸지 않았다. [SquidPostPop::BridgePrefix](../../cpppj/src/o/SquidPostPop.cpp)는 flags 비트 1이면 extra 비트 2를 켠 뒤 연결 함수를 호출하고, 비트 1이 없으면 비트 4이면서 abstract(extra 비트 1)가 아닐 때만 호출한다. 그 뒤 공통 postPop이 실행된다. 공통 통계가 억제돼도 이 접두는 실행한다.

연결 함수 **`004213b0`/CD `00448b00`의 flag 4 이웃 순회·연결 객체 생성(`004210f0`)·소유자 전파(`00421240`)는 아직 콜백 경계**다. 명시적인 연결 콜백과 정확한 다리 가상 표가 있는 인스턴스만 Pop을 허용한다. 콜백이 없거나 다른 파생 표이면 좌표/해시/spot 쓰기 전에 거부한다. 이 변경으로 다른 미복원 파생 후처리를 허용하지 않는다.

[decomp_bridgepostpop_oracle.py](../../tools/decomp_bridgepostpop_oracle.py)는 세 실제 PE × flags 10종 × extra 8종 = **240개** 접두 관찰을 x87 53/64비트에서 얻었다. 접두 몸체는 실제 명령이며 연결 함수와 공통 후처리는 호출 순서/인자를 기록하는 대체다. assert 0이며 접두의 extra 한 바이트 밖 쓰기는 거부한다. [fixture](../../cpppj/tests/fixtures/bridgepostpop-x86.tsv)와 [근거 JSON](../../cpppj/recovery-bridgepostpop-evidence.json)에 이 경계를 기록했다. 공통 postPop 자체의 독립 근거는 [기존 복원](cpp-postpop-reconstruction.md)을 따른다. 새 C++ 검사는 연결 호출 때 extra가 이미 갱신됐는지와 전체 슬롯 보존, 콜백 누락/다른 표 거부 및 공통 통계 억제 순서를 확인한다.

```powershell
# 입력 내보내기가 없는 PC에서 먼저 실행한다.
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name bridgeevent,graphremove,bridgepostpop
python -X utf8 tools/decomp_neighbor_oracle.py
python -X utf8 tools/decomp_neighbor_oracle.py --verify
python -X utf8 tools/decomp_bridgepostpop_oracle.py
python -X utf8 tools/decomp_bridgepostpop_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```

## 실제 raw 모듈 통합 범위

별도 통합 검사는 실제 `bridge.type`의 60프레임과 다리 생성자 주소를 사용한다. 두 판본에서 소수 위치의 연결된 보통 칸·단단한 칸·고립된 칸을 구성하고 **지연 낙하 예약→Kernel→가상 표 분배→실제 첫 이웃→SquidFactory 생성→SquidOwner→다리 삭제 훅/공통 삭제 장부→Unpop/SID 반납/종속 ProcessForm 제거→SquidPop/postPop**을 이어 검사한다.

보통 칸에서는 새 L 프레임의 소유자·수명·부모 단어·플래그·좌표 비트, 해시 머리·spot·타입 수·비용 집계를 확인한다. 단단한 칸은 수명만 바뀌며 객체를 교체하지 않고, 고립된 칸은 삭제한 뒤 해시/spot이 비고 예약도 사라진다. 자기 이벤트 처리 중 옛 부모가 지워져 실행 중인 Regular까지 제거되는 경로를 포함한다.

이 통합 검사는 **비전투 Pop·Graph 비활성** 계약이며 다리 연결 객체/소유자 전파·파편·소리·보상·UI·전파는 외부 사건으로 남긴다. 실제 GUI GameWorld의 생성/배치/삭제가 이 raw 모듈을 호출하도록 연결하는 일, 다리 연결 함수 몸체, Kernel과 게임 루프 연결, Graph 활성 변환의 통합 x86 대조, walker 낙하·파편 생성/소리 출력·건설/경제/전투/승패는 남았다. 이 검사를 미션 완주나 실제 화면 동작의 증거로 해석하지 않는다.

## 최종 콘솔 검증

x64 Release 경고/오류 0·CTest 내부 **218개·실패 0**(이전 211 + 새 7, CTest 전체 86.80초)이다. 제한 x86 새 **1,968개**, 누적 **111,754개**이며 지원하는 감사 **27개 모두 통과**했다. 최종 로그는 `extracted/neighbor-ctest.log`다. AGENTS.md·원본·dotnetpj는 변경하지 않았으며 커밋/푸시하지 않았다.
