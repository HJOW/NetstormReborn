# cpppj Squid 프레임 지정

2026-10-08, `VM-W11-CODEX`. 기준은 10.78이다. 원본 게임·복사본은 실행하지 않았고 실제 PE의 제한 x86 명령과 콘솔 검사로 진행했다. **이 단계의 디컴파일(Ghidra 읽기 전용 내보내기)을 수행한 PC는 `VM-W11-CODEX`다.**

객체의 프레임 번호를 바꾸는 공통 함수 **`004acee0`/CD `004acbc0`**을 복원했다. [다리/섬 연결](cpp-bridgeconnect-reconstruction.md)의 소유자 전파가 섬 받침·종유석의 색 프레임을 바꿀 때 부르는 함수이고, 그 단계에서는 호출 경계로 남겼던 것이다. [SquidFrame](../../cpppj/src/o/SquidFrame.h)이 본체, [SetFrameTests](../../cpppj/tests/SetFrameTests.cpp)가 검사다.

## 함수 대응

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 프레임 지정 | `004acee0` | `004acbc0` |
| 현재 프레임의 해시 단계 계산·저장 | `004ace40` | `004acb00` |
| 프레임 추가 헤더 조회(범위 검사 포함) | `00419850` | (단계 계산에 인라인, 검사 없음) |
| 가상 표 +0x4c: 현재 위치에 다시 Pop | `0041c0d0` | `00401c60` |
| 가상 표 +0x8c: +0x88로 넘김 | `004ad670` | `004ae3e0` |

필드: 프레임은 패치 +0x24 DWORD / CD +0x22 바이트, **해시 단계 바이트는 패치 +0x21 / CD +0x1f**(Pop이 쓰는 그 바이트다).

## 복원한 동작

`SetFrame(frame, flags)` — flags는 0 또는 0x2000이어야 한다(그 밖은 원본 assert, C++은 변경 전에 거부).

1. 단계 바이트를 부호 있는 바이트로 읽어 둔다(`stored`).
2. **현재 프레임**으로 해시 단계를 구해 단계 바이트에 쓴다(`level`). island·bridge genus는 0이고, 그 밖은 프레임 추가 헤더 +0·+4(칸 단위 크기)의 큰 쪽이 2 이하면 1, 4 이하면 2, 넘으면 3이다([기존 ObjectLevel](cpp-hash-reconstruction.md)).
3. `level == stored`이면: 가상 표시 갱신 → 프레임 쓰기 → 가상 표시 갱신. flags에 0x2000이 있으면 +0x8c, 없으면 +0x88을 쓴다. **새 프레임이 현재와 같아도 두 번 갱신한다.** 다시 등록하지 않는다.
4. 다르면: 새 프레임이 현재와 같으면 (단계 바이트만 고친 채) 끝난다. 다르면 가상 Unpop(flags) → 프레임 쓰기 → 가상 표 +0x4c(flags | 0x50), 곧 **현재 위치에 Pop(x, y, flags | 0x50)**.

즉 원본은 새 프레임이 아니라 **현재 프레임의 단계가 저장된 단계와 맞는지**만 본다. 크기 단계가 다른 프레임으로 바꾸면 그 호출에서는 제자리에서 바뀌고(옛 단계의 버킷에 남는다), **해시 단계는 한 번 늦게 — 그 다음 프레임 변경에서 — 옮겨진다.** Unpop은 저장된 단계를 믿지 않고 네 단계를 모두 찾으므로([Unpop](cpp-unpop-reconstruction.md)) 이 지연이 체인을 깨지 않는다. 통합 검사가 이 순서를 실제 Unpop/Pop으로 확인한다.

판본 차이: CD는 프레임이 바이트라 하위 바이트만 쓰고, "새 프레임이 현재와 같은가"는 인자 DWORD와 현재 바이트(0 확장)를 비교한다(0x101은 1과 같지 않다). 패치판은 단계 계산 때 `00419850`이 현재 프레임의 범위(타입 플래그 1의 `0x440000`이 있으면 프레임 수의 두 배까지)와 SHP 유무를 assert로 검사하고, CD는 검사하지 않는다. C++은 프레임 크기를 훅으로 받으므로 그 검사는 훅의 책임이다. 새 프레임의 범위는 두 판본 모두 검사하지 않는다.

## 독립 x86 대조

[decomp_setframe_oracle.py](../../tools/decomp_setframe_oracle.py)는 [새 내보내기](ghidra-exports.md) `setframe`와 기존 `graphremove`·`bridgeevent`의 몸체를 쓴다. 세 PE × 1,924 = **5,772개 관찰**이다(프레임 지정 1,920 + 넘김 함수 4). 입력은 타입 종류 4(bridge·isle·island·priest의 실제 플래그) × 저장된 단계 5(0~3과 0xff) × 현재 프레임 8 × 새 프레임 6(현재와 같은 값·표 밖·바이트를 넘는 0x101 포함) × flags 2다. x87 53/64비트에서 같은 관찰만 저장한다(약 10초).

비교하는 것은 **사건 순서·인자와 슬롯의 모든 바이트**다. 사건마다 **호출 시점의 프레임 필드**를 함께 적어, 프레임을 쓰는 시점이 표시 갱신/Unpop/Pop의 앞인지 뒤인지도 대조한다.

**대체(호출 사실과 인자만 기록):** 가상 표시 갱신(+0x88·+0x8c), 가상 Unpop(+0x48), 가상 Pop(+0x90). 프레임 지정 몸체·단계 계산·프레임 헤더 조회·+0x4c 넘김 함수는 실제 명령이다. base의 +0x8c가 +0x88로 넘어가는 것은 넘김 함수 행으로 확인했다(C++의 표시 갱신 훅이 두 경로를 하나로 받는 근거). 원본 assert 도달 0, 허용 범위 밖 실행/쓰기 0이다. 타입 구조체와 SHP 프레임 헤더는 합성이다.

C++은 수정 없이 첫 실행에서 5,772개와 일치했다.

### 변이 확인

[cpp_mutation_check.py](../../tools/cpp_mutation_check.py)에 변이 7개를 더했다(새 프레임의 표시 갱신 생략, 첫 표시 갱신 전에 프레임 쓰기, 같은 프레임이어도 다시 등록, 0x50 누락, 다리의 단계를 크기로 계산, 단계 바이트 쓰기 생략, Unpop 전에 프레임 쓰기). **실행한 것은 `frame-single-update` 하나이고 검출됐다. 나머지 6개는 실행하지 않았다**(사용자 지시로 15분을 넘는 작업을 다음 작업으로 넘겼다 — [LEFT_JOBS.md](../../LEFT_JOBS.md)).

## 실제 raw 모듈 통합

- `set_frame_real_unpop_and_pop_move_hash_level_on_next_change`: 두 판본에서 실제 생성자·Pop·Unpop으로 "제자리 변경 → 단계 바이트만 고침 → 다음 변경에서 Unpop/Pop으로 버킷 이동"을 확인한다.
- [다리/섬 연결 통합 검사](cpp-bridgeconnect-reconstruction.md#실제-raw-모듈-통합)의 프레임 지정 경계를 이 함수로 바꿨다. 섬 받침과 종유석은 색 프레임의 단계가 같아 제자리 경로다.

**범위와 남은 것:** 표시 갱신은 훅이다. 기존 [공통 표시 갱신](cpp-display-reconstruction.md)(`SquidDisplay::Update`)에 잇는 일과 GUI raw GameWorld 연결은 남았다. 프레임 크기를 실제 SHP 추가 헤더에서 읽는 공급자도 호출자 몫이다. 이 함수를 쓰는 애니메이션 프로세스들은 복원하지 않았다.

## 재현

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name setframe,bridgeevent,graphremove
python -X utf8 tools/decomp_setframe_oracle.py
python -X utf8 tools/decomp_setframe_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```

## 2026-10-08 프레임·표시 후속 통합

[SquidFrame 표시 갱신·실제 SHP 크기 공급](cpp-frame-display-integration.md)을 `MakeSquidFrameHooks`로 실제 Update/Unpop/Pop·Renderer에 연결했다. 전체 콘솔 검사는 251개·실패 0, 실제 두 판본 SHP 물리 6,950개 메타데이터·범위 안 크기 공급 6,946개 확인이 통과했다. Graph 활성·GUI raw 스프라이트 동기화·애니메이션 프로세스는 후속이며 위 독립 기계어 검증 범위는 그대로다.
