# cpppj 다리/섬 연결·연결 객체 생성·소유자 전파

2026-10-08, `VM-W11-CODEX`. 기준은 10.78이다. 원본 게임·복사본은 실행하지 않았고 실제 PE의 제한 x86 명령과 콘솔 검사로 진행했다. **이 단계의 디컴파일(Ghidra 읽기 전용 내보내기)을 수행한 PC는 `VM-W11-CODEX`다.**

다리 전용 postPop의 접두([앞 단계](cpp-neighbor-reconstruction.md))가 부르던 **연결 함수 `004213b0`/CD `00448b00`**과 그 안의 **연결 객체 생성 `004210f0`/CD `004487c0`**, **소유자 전파 `00421240`/CD `00448910`**을 복원했다. 기록만 하던 콜백 경계가 실제 생성·소유자 지정·Pop으로 이어진다. [RawBridgeConnect](../../cpppj/src/o/RawBridgeConnect.h)가 본체이고 [RawSquidNeighborWalk](../../cpppj/src/o/RawSquidNeighbors.h)가 탐색기 순회다. [BridgeConnectTests](../../cpppj/tests/BridgeConnectTests.cpp)가 독립 기대값과 실제 raw 모듈 연결을 검사한다.

게임에서 보이는 동작으로 말하면 **다리가 섬 표면에 닿으면 그 사이에 연결 조각(bridgeConnector)이 생기고, 주인 없는 섬은 다리 주인의 것이 된다**(섬 받침·종유석이 그 플레이어의 색 프레임으로 바뀌고 섬 위의 표면 칸에 소유자가 들어간다).

## 함수 대응

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 연결 순회 | `004213b0` | `00448b00` |
| 연결 객체 생성 | `004210f0` | `004487c0` |
| 소유자 전파 | `00421240` | `00448910` |
| 좌표 한 칸에서 타입 찾기 / genus 마스크로 찾기 | `004b1fa0` / `004b2070` | `004eb4b0` / `004eb5c0` |
| 소유자 색 프레임 | `004421a0` | `004d0180` |
| 섬 받침·종유석의 소유자 지정(vtable +0x74 재정의) | `00442310`(둘이 같은 함수) | `004d01a0` · `004d0350`(같은 코드 두 벌) |
| 섬 받침의 postPop(vtable +0x20 재정의) | `004421c0` | `004d01d0` |
| 탐색기 Next / 연결 필터 | `004b1e70` / `004b1e80` | `004ec030` / `004ec040` |
| 한 칸 이동 / 거의 올림 / 자름 | `0041d0b0` / `0041d7a0` / `0041d770` | `0043f8d0` / `00440a80` / `00440a50` |
| 프레임 지정(이번에는 호출 경계) | `004acee0` | `004acbc0` |

타입 번호 전역: 연결 객체 `005412c4`/CD `0051cbb0`(bridgeConnector, 실제 번호 155), 섬 받침 `005411d0`/CD `0051cabc`(island, 94), 종유석 `005411d4`/CD `0051cac0`(islandStalag, 95), 표면 칸 `005412cc`/CD `0051cbb8`(noIsland, 157). 이 번호는 생성자 주소 표에서 확인했다(생성자 `00422720`·`00443b70`·`00443b90`·`00443bb0`).

## 복원한 동작

### 연결 순회 (`Connect`)

1. 자기 타입의 genus(타입 플래그 2)를 읽는다. **다리(비트 4)이면 상대 마스크는 `0x1000002`(island·섬 받침 계열), 아니면 `4`(다리)**다.
2. `004b23e0(자기, 4)`로 탐색기를 만든다. flag 4는 flag 0의 연결 필터에 **abstract(extra 비트 1) 후보 제외**를 더한 것이다.
3. 현재 이웃이 0이 아닌 동안: 이웃의 genus가 마스크에 걸리면 **다리 쪽을 첫 인자로** 연결 객체 생성과 소유자 전파를 차례로 부르고, 걸리든 아니든 Next로 넘어간다. 연결 객체의 소유자는 그때 다리 쪽 슬롯의 소유자 바이트다.

호출자는 세 곳을 확인했다: 다리 postPop 접두(`00422192`), **섬 받침 postPop(`0044221f`)**, noIsland postPop(`00442624`). 넷째(`00445d87`)는 목록 인자를 넘기는 미리보기 경로다(호출자는 복원하지 않았다).

### 연결 객체 생성 (`Link(first, second, owner)`)

1. `004af530(연결 객체 타입, 2)`로 새 객체를 만든다.
2. first의 위치(raw float)를 읽고, second의 중심(위치 − 발자국/2)을 **0.99999를 더해 자른 정수**로 만든다.
3. first에서 그 중심을 보는 **네 방향**(세로 차이가 가로보다 작지 않으면 세로)을 구해 first의 위치를 그 방향으로 한 칸 옮긴다.
4. 새 객체의 프레임 = 방향 / 2(북 0·동 1·남 2·서 3). 가상 소유자 지정(owner).
5. 목록 인자가 없으면: 가상 Pop(옮긴 위치, flags 0) → **+0xc 단어 = first 번호**(연결 객체 타입이 다리 타입 전역과 같을 때만 표면 알림) → **+8 단어 = second 번호**. 다리 preDestroy가 이 두 단어를 읽어 죽은 쪽의 연결 객체를 지운다([삭제 훅](cpp-bridgeeffects-reconstruction.md)).
6. 목록 인자가 있으면(미리보기): 등록하지 않고 위치·화면 좌표(x×16+0.5, y×11+0.5)만 쓴 뒤 번호를 목록에 넣는다. 그 뒤 원본은 `004b2070(중심, 0x10000000)`으로 그 칸을 조회하지만 **결과를 쓰지 않는다** — 상태를 바꾸지 않으므로 C++에는 옮기지 않았다.

원본은 두 인자의 타입을 검사하지 않는다(대조 입력에 뒤집은 순서도 넣었다).

### 소유자 전파 (`PropagateOwner(first, second)`)

1. second 위치의 칸에서 **섬 받침 타입**의 객체를 찾는다(`004b1fa0`). 찾는 타입이 island·bridge genus가 아니면 해시 0단계를 건너뛴다 — 섬 받침(genus `0x1000000`)은 프레임 크기로 정한 1~3단계에 있다.
2. first가 abstract/buried이거나, 받침이 없거나, **받침에 이미 주인이 있거나**, 받침이 abstract/buried이면 끝이다.
3. first의 소유자가 0이면 원본 assert(`newPlayerId != INVALID_PLAYER_ID`, Bridge.cpp 187행)다. C++은 훅을 부르기 전에 예외로 거부한다.
4. 받침에 가상 소유자 지정 → 받침에 프레임 지정(소유자 색 프레임, flags 0).
5. 받침 위치에서 **종유석 타입**을 찾아, 있으면 받침의 (방금 바뀐) 프레임을 그대로 프레임 지정한다.
6. 받침의 발자국(왼쪽/위는 자르고 오른쪽/아래는 0.99999를 더해 자른 정수 사각형)으로 일반 탐색을 해서 **noIsland 타입에만** 가상 소유자 지정을 한다.

미리보기 경로에서도 소유자 전파는 그대로 실행된다(원본 그대로다).

### 소유자 색 프레임과 섬의 소유자 지정

- `004421a0`: **전투 모드(`00594fbc`)이고 번호가 양수이면 색 표(`00531b08`)의 값 − 1, 아니면 8(중립)**이다. 색 표의 초기값은 0~8이 자기 번호다.
- 섬 받침·종유석의 소유자 지정 재정의: base 소유자 지정([앞 단계](cpp-owner-reconstruction.md))을 부른 뒤 프레임 필드를 그 색 프레임으로 **직접 쓴다**(표시 갱신 없음). 단 **미션 중(`00594fa0`, `global.inMission`)이고 현재 프레임이 8이면 그대로 둔다.**
- 섬 받침의 postPop: flags 비트 1(최초 등록)이면 종유석을 만들어 받침의 소유자를 주고 같은 위치에 Pop한 뒤 연결 순회를 하고, 그 뒤 항상 공통 postPop을 부른다.

## 탐색기 flag와 기존 코드 정정

연결 필터(`004b1e80`/CD `004ec040`)는 탐색기 flags의 네 비트를 본다. [RawSquidNeighborWalk](../../cpppj/src/o/RawSquidNeighbors.h)는 0~7을 지원한다.

| 비트 | 뜻 |
|---|---|
| 1 | 프레임 접합 검사를 하지 않는다 |
| 2 | surface 타입이 아닌 후보도 받는다 |
| 4 | abstract 후보를 제외한다 |
| 8 | 표면 지도 순회(`004b1c20`) — 이 클래스는 지원하지 않는다([flag 8은 SurfaceFinder](cpp-surface-reconstruction.md)) |

**정정:** 필터의 마지막 조건인 "후보 기준점의 spot 내부 비트"는 `spot[ftol(y + 0.9999) × 256 + ftol(x + 0.9999)]`를 읽는다(`00500edc`/CD `00507484`). 앞 단계의 C++은 좌표를 그대로 잘랐다. 정수 좌표에서는 같지만 **소수 좌표의 후보는 다음 칸을 본다.** 앞 단계의 대조 입력에는 이 차이가 드러나는 경우가 없었다. 이번에 전용 입력을 넣어 세 PE의 관찰로 확인했고 변이 확인으로 검출됨도 확인했다(아래). 또 방향 판정(`0041ce90`)에서 가로 차이는 x87 스택에 남은 값을, 세로 차이는 단정도로 저장했다 다시 읽은 값을 쓰도록 맞췄다(지도 좌표 범위에서는 결과가 같다).

순회 도중 호출자가 객체를 등록하면(연결 객체의 Pop) 원본처럼 그 뒤의 Next가 바뀐 체인을 읽는다. 연결 객체는 surface 타입이 아니라 이웃으로 돌아오지 않는다.

## 독립 x86 대조

[decomp_bridgeconnect_oracle.py](../../tools/decomp_bridgeconnect_oracle.py)는 [새 내보내기](ghidra-exports.md) `bridgeconnect`와 기존 `bridgeevent`·`graphremove`의 몸체를 쓴다. 세 PE × 3,600 = **10,800개 관찰**이며 장면 입력 1,056개는 세 판본이 공유한다. 모든 입력을 x87 53/64비트에서 실행해 두 관찰이 같을 때 한 행만 저장한다(판본마다 프로세스 하나, 약 10분).

| 관찰 | 판본당 | 실제로 실행한 것 / 비교 대상 |
|---|---:|---|
| 이웃 순회(Walk) | 1,856 | 탐색기 생성 + Next를 0까지, 장면 232개 × flags 0~7 / 이웃 번호 목록과 순서 |
| 연결 순회(Connect) | 464 | 등록 경로와 미리보기 경로 / 사건 순서·인자, 새 슬롯의 모든 바이트, 풀 Adler-32, 목록 |
| 연결 객체 생성(Link) | 392 | 소수 위치·여덟 방위·동률·뒤집은 인자 / 같음 |
| 소유자 전파(Owner) | 330 | 받침 유무·주인·abstract/buried·종유석·발자국 안팎 / 사건, 풀 Adler-32 |
| 한 칸 조회(FindAt) | 270 | 타입별 단계 건너뛰기 / 번호 |
| 색 프레임(Color) | 42 | 전투 여부·색 표·번호 / 반환값 |
| 섬 받침 / 종유석 소유자 지정 | 96 + 96 | 프레임·미션·전투·색 표 / 사건, 프레임 |
| 섬 받침 postPop(IslandPost) | 54 | 실제 연결 순회까지 / 사건, 새 슬롯, 풀 Adler-32 |

**대체(호출 사실과 인자만 기록):** 새 객체 생성(헤더만 만든다), 가상 소유자 지정(소유자 바이트만 쓴다), 가상 Pop, 프레임 지정(프레임 필드만 쓴다), 표면 알림, base 소유자 지정, 공통 postPop. 탐색·필터·기하·타입 조회는 대체하지 않는다. 원본 assert 도달 0, 허용 범위 밖 실행/쓰기 0이다. 타입 번호는 합성이지만 **플래그와 발자국은 실제 10.78 타입 표의 값**(bridge·isle·isleBig·priest·bridgeConnector·island·islandStalag·noIsland)이다.

장면 232개 가운데 마지막 32개는 **spot 칸 보정 전용 입력**이다: 소수 좌표의 후보 하나에 대해 "좌표를 자른 칸"과 "0.9999를 더해 자른 칸" 가운데 (앞쪽만 / 뒤쪽만 / 둘 다 / 없음)에 내부 비트를 넣는다. 세 PE 모두 **앞쪽에만 있으면 이웃으로 받고 뒤쪽에만 있으면 거부**했다. 처음 만든 입력(장면 200개)에는 이 차이가 드러나는 경우가 없어 변이 확인에서 미검출로 나왔고, 그래서 이 입력을 더했다.

C++은 첫 실행에서 색 표 밖 번호를 다루는 검사 코드의 조건 하나만 고쳤고, 복원 본체는 수정 없이 모든 행과 일치했다.

[bridgeconnect-x86.tsv](../../cpppj/tests/fixtures/bridgeconnect-x86.tsv), [근거 JSON](../../cpppj/recovery-bridgeconnect-evidence.json)에 원본 PE·도구·내보내기·결과 SHA와 대체 호출 수를 보존했다. `--verify`는 저장된 SHA/행 수/assert 0/실제 몸체 실행의 감사이며 기계어 재실행은 아니다.

### 변이 확인

[cpp_mutation_check.py](../../tools/cpp_mutation_check.py)에 이 단계의 변이 19개를 더했다(이웃 필터 flag 3종·spot 칸 보정·방향 동률, 연결 마스크·소유자 출처, 연결 객체의 프레임·중심 올림·참조·미리보기 화면 좌표, 소유자 전파의 조건 네 가지·발자국 올림·타입 한정, 한 칸 조회의 단계 건너뛰기, 색 프레임, 섬 소유자의 미션 조건, 섬 받침 postPop의 최초 등록 비트).

- 첫 실행(장면 200개 기대값): **18개 검출, `neighbor-candidate-spot-no-bias` 1개 미검출.** 위 전용 입력을 더한 원인이다.
- 전용 입력을 더한 기대값으로 그 변이를 다시 실행: **검출.** 따라서 19개 모두 검출됐다.

변이 하나에 빌드와 전체 검사로 3~4분이 걸린다(19개 약 50분).

## 실제 raw 모듈 통합

`bridge_connect_real_raw_modules_capture_neutral_island`가 두 판본에서 실제 타입 번호·생성자 주소·다리 프레임 표로 다음을 잇는다.

1. 중립 섬 받침을 Pop → **섬 받침 postPop 재정의**([SquidPostPop::SetIslandPrefix](../../cpppj/src/o/SquidPostPop.h))가 종유석을 만들고(SquidFactory) 소유자를 주고(가상 표 분배 `MakeOwnerDispatch` → SquidOwner) 같은 자리에 Pop한다.
2. 소유자 3의 다리를 받침 동쪽에 Pop → 다리 postPop 접두 → **실제 연결 순회** → 연결 객체가 실제 풀에서 번호를 받아 다리 서쪽 칸에 등록된다(프레임 3, 소유자 3, 두 참조 단어, 해시 1단계).
3. 소유자 전파로 섬 받침의 소유자와 프레임이 3·2가 되고 종유석 프레임이 2, 발자국 안 표면 칸 둘의 소유자가 3이 된다.
4. 다른 소유자의 다리가 닿아도 주인이 있는 받침은 바뀌지 않고 연결 객체만 생긴다.

**범위와 남은 것:** 비전투 Pop·Graph 비활성 계약이다. 프레임 지정(`004acee0`)은 같은 날 [별도 단계](cpp-setframe-reconstruction.md)에서 복원해 이 통합 검사에 연결했다(표시 갱신만 훅으로 남는다). 섬 받침 preDestroy 재정의(`00442240`)도 [별도 단계](cpp-islandlifecycle-reconstruction.md)에서 복원했다. noIsland의 파생 postPop(`004423b0`, 프레임 자동 선택·받침 생성)은 미복원이라 그 객체는 검사에서 직접 배치했다. 미리보기 경로의 호출자, GUI raw GameWorld 연결, Graph 활성 통합은 남았다. 이 검사를 미션 완주나 실제 화면 동작의 증거로 해석하지 않는다.

## 재현

```powershell
# 입력 내보내기가 없는 PC에서 먼저 실행한다(읽기 전용).
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name bridgeconnect,bridgeevent,graphremove
python -m pip install --target extracted/oracle-python -r tools/requirements-decomp-oracle.txt   # 한 번만. 한글 주석 때문에 PYTHONUTF8=1 필요
python -X utf8 tools/decomp_bridgeconnect_oracle.py
python -X utf8 tools/decomp_bridgeconnect_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```

## 최종 검증

같은 날의 [프레임 지정](cpp-setframe-reconstruction.md)·[섬 삭제 훅](cpp-islandlifecycle-reconstruction.md)까지 넣은 상태에서 x64 Release 경고/오류 0·CTest 내부 **234개·실패 0**이다(앞 단계 218 + 이 단계 6 + 프레임 지정 5 + 섬 삭제 훅 5). 제한 x86 새 **18,132개**(이 단계 10,800 + 프레임 지정 5,772 + 섬 삭제 훅 1,560), 누적 **129,886개**다. 감사는 30개 가운데 29개가 이 PC에서 통과한다(`regiongraph`는 [내보내기 문서](ghidra-exports.md)의 남은 문제). 이 PC에서는 창 검사가 허용돼 클론 창 회귀 4종(`cpp_window_smoke`·`cpp_renderer_smoke`·`cpp_menu_smoke`·`cpp_world_smoke`)도 실행했고 모두 통과했다. 원본 게임·복사본은 실행하지 않았다. AGENTS.md·원본 파일은 변경하지 않았으며 커밋/푸시하지 않았다.
