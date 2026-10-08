# cpppj 섬 받침 preDestroy·noIsland postDestroy

2026-10-08, `VM-W11-CODEX`. 기준은 10.78이다. 원본 게임·복사본은 실행하지 않았고 실제 PE의 제한 x86 명령과 콘솔 검사로 진행했다. **이 단계의 디컴파일(Ghidra 읽기 전용 내보내기)을 수행한 PC는 `VM-W11-CODEX`다.**

[다리/섬 연결](cpp-bridgeconnect-reconstruction.md)에서 가상 표를 확인한 섬 계열의 삭제 쪽 재정의 둘을 복원했다. [RawIslandLifecycle](../../cpppj/src/o/RawIslandLifecycle.h)이 본체, [IslandLifecycleTests](../../cpppj/tests/IslandLifecycleTests.cpp)가 검사다.

게임에서 보이는 동작으로 말하면 **섬이 사라질 때 그 섬에 닿아 있던 다리들이 (조금 뒤) 끝 칸 모양으로 바뀌고, 섬 표면 칸이 사라지면 그 위에 서 있던 보행 유닛이 떨어진다.**

## 함수 대응

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 섬 받침 preDestroy (island vtable +0x14) | `00442240` | `004d0250` |
| noIsland postDestroy (noIsland vtable +0x18) | `00442640` | `004d2a40` |
| 지연 낙하 예약의 넘김 함수 | `00421f90` → `00421530` | (직접 `004490b0`) |
| 한 칸 탐색기 생성자 | `004202f0` | (호출자에 인라인) |
| 공통 preDestroy / postDestroy | `004b0950` / `004b0840` | `004add20` / `004adbe0` |

## 복원한 동작

### 섬 받침 preDestroy (`IslandPreDestroy(island, flags)`)

1. 받침이 abstract/buried(extra `& 9`)가 아니면 받침의 중심(위치 − 발자국/2)을 구하고 `004b23e0(받침, 0)`으로 flag 0 연결 이웃 탐색기를 만든다.
2. 이웃마다: **flags에 `0x1000`이 없고**, 이웃의 genus가 다리(비트 4)이며 dead가 아니면 **그 다리에 받침의 중심 좌표로 지연 낙하(`0x2692`)를 예약**한다. 예약된 이벤트는 다음 프레임에 [다리 이벤트 처리기](cpp-bridgeevent-reconstruction.md)가 받아 그 다리 칸을 끝 칸(L·M·N·O)으로 바꾼다.
3. 그 뒤 항상 공통 preDestroy(flags)를 부른다.

탐색기는 `0x1000`이 있어도 만들고 끝까지 순회한다(상태를 바꾸지 않는다). 패치판 넘김 함수 `00421f90`은 디버그 검사 전역(`005e4794`)이 켜져 있고 대상이 다리가 아니면 assert하지만, 호출 조건이 이미 다리라 도달하지 않는다(대조 입력의 절반은 그 전역을 켜고 실행했다).

### noIsland postDestroy (`SurfacePostDestroy(surface, flags)`)

1. 표면 칸이 abstract/buried가 아니면, 자기 위치를 자른 한 칸에서 `004202f0(x, y, 1)` — **해시 0단계를 건너뛰는** 일반 탐색 — 을 한다.
2. 걸린 객체마다 genus에 walker(`0x10000`)가 있으면 그 객체의 가상 표 +0xc8(낙하)을 부른다.
3. 그 뒤 항상 공통 postDestroy(flags)를 부른다.

[다리 postDestroy](cpp-bridgeeffects-reconstruction.md)의 "칸 위 walker 낙하"와 같은 구조이고, 다른 점은 0단계를 건너뛰는 것(flag 1)과 파편/소리 통지가 없는 것이다.

## 독립 x86 대조

[decomp_islandlifecycle_oracle.py](../../tools/decomp_islandlifecycle_oracle.py)는 [새 내보내기](ghidra-exports.md) `islandlifecycle`과 기존 `bridgeconnect`·`bridgeevent`·`graphremove`의 몸체를 쓴다. 세 PE × 520 = **1,560개 관찰**(섬 받침 preDestroy 260 + noIsland postDestroy 260, 장면 520개는 세 판본 공유)이며 x87 53/64비트에서 같은 관찰만 저장한다(약 3분). 비교하는 것은 사건의 순서와 인자다. 10.78의 관찰에서 섬 받침 260개 가운데 106개가 지연 낙하를 하나 이상 예약하고, 표면 칸 260개 가운데 69개가 walker 낙하를 부른다. 입력은 받침/표면의 extra(abstract·buried 포함), flags(`0x1000` 포함), 둘레의 다리(dead·abstract·한 축만 이어지는 프레임)·다른 표면·비표면, 내부 spot, 같은 칸/옆 칸·여러 해시 단계의 walker와 매몰 객체다. C++은 수정 없이 첫 실행에서 모두 일치했다.

**대체(호출 사실과 인자만 기록):** 지연 낙하 예약(`00421530` ↔ CD `004490b0`), 가상 walker 낙하(+0xc8), 공통 preDestroy/postDestroy. 탐색기 생성·Next·연결 필터·일반 탐색·중심 계산·넘김 함수는 실제 명령이다. 두 재정의의 몸체는 풀에 쓰지 않으며(쓰기 허용 범위를 비워 두고 실행했다) C++ 검사도 풀 전체가 그대로임을 확인한다.

### 변이 확인

[cpp_mutation_check.py](../../tools/cpp_mutation_check.py)에 변이 5개를 더했다(`0x1000` 무시, 죽은 다리에도 예약, 다리가 아닌 표면에도 예약, 0단계 포함, abstract/buried 표면 칸도 처리). **이번에는 하나도 실행하지 않았다**(사용자 지시로 15분을 넘는 작업을 다음 작업으로 넘겼다 — [LEFT_JOBS.md](../../LEFT_JOBS.md)). `island-pre-schedules-dead-bridge`는 연결 필터가 dead 후보를 먼저 거르므로 검출되지 않을 가능성이 있다. 그 경우 그 조건은 원본에서도 도달하지 않는 중복 검사라는 뜻이니 결과를 이 문서에 적는다.

## 실제 raw 모듈 통합

`island_lifecycle_real_process_schedules_bridge_falls`가 실제 프로세스 계층([ScheduleBridgeFall](cpp-process-reconstruction.md)·Kernel)에 잇는다: 받침 둘레에 살아 있는 다리·dead 다리·다리가 아닌 표면을 두고, `0x1000`이 있으면 예약이 없고, 없으면 **살아 있는 다리에만** 예약되며, 다음 Kernel 프레임에 그 다리의 이벤트 처리기가 받침 중심 칸을 포장한 payload로 불리는 것을 확인한다.

**범위와 남은 것:** 다리 이벤트 처리기 몸체와 공통 삭제까지 한 흐름으로 잇는 일(섬 삭제 → 다리 끝 칸 변환 → 연결 객체 정리), walker의 가상 낙하 몸체, noIsland의 postPop 재정의(`004423b0` — 프레임 자동 선택·받침 생성), GUI raw GameWorld 연결은 남았다.

## 재현

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name islandlifecycle,bridgeconnect,bridgeevent,graphremove
python -X utf8 tools/decomp_islandlifecycle_oracle.py
python -X utf8 tools/decomp_islandlifecycle_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```
