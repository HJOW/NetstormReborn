# 공통 프레임 진행과 사제 낙하의 실제 프레임 연결

2026-10-10, **HJOW-Athlon**. 기준은 10.78이다. 마지막 디컴파일 수행 PC도 **HJOW-Athlon, 2026-10-10**이다. 새 `frameadvance` 목록을 같은 PC에서 Ghidra 읽기 전용으로 내보냈다(10.78 2개, CD/추가 10.37 각각 4개). 기존 `setframe`·`graphremove`·`bridgeevent` 내보내기를 재사용했다. 원본 게임·복사본·클론 창은 실행하지 않았다.

[`RawFrameAdvance`](../../cpppj/src/o/RawFrameAdvance.h)는 현재 글자 구간의 첫 번호/개수로 프레임을 진행하고 실제 [`SquidFrame::Set`](../../cpppj/src/o/SquidFrame.h)을 호출한다. `MakePriestFallFrameHooks`는 [`RawPriestFall`](../../cpppj/src/o/RawPriestFall.h)의 시작·착지 지정과 J 방향 진행을 같은 지정기로 연결한다. 일반 사제 Pop의 파생 분배는 다음 단계다.

## 함수와 raw 필드

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 공통 프레임 진행 | `004afc90` | `004acce0` |
| 현재 타입 조회 | `0049a840` | `004abf50` |
| 현재 방향 조회 | 진행 몸체에서 signed side 직접 읽음 | `004ae4b0` |
| 공통 프레임 지정 | `004acee0` | `004acbc0` |
| 글자별 첫 번호/개수 생성 | `0049b060` | `00444da0` |

프레임은 패치 +0x24 signed DWORD / CD +0x22 unsigned BYTE다. 코드 포인터는 타입 +0x124, 글자별 첫 번호는 +0x130, 개수는 +0x170이다. 패치는 signed side를 직접 인덱스로 사용한다(+0x2c/+0x6c에 side×4를 더함). CD는 방향 `signed side - 'A'`를 쓴다. 유효한 A~P 구간에서는 같은 표를 읽는다. 새 C++은 원본의 표 밖 메모리 읽기를 예외로 진단한다.

## 진행 계약

현재 프레임을 `current`, 같은 글자의 첫 번호를 `first`, 개수를 `count`, signed 증분을 `delta`라 하면 다음 순서다.

1. `relative = current + delta - first`를 **32비트 DWORD 연산**으로 계산한다.
2. signed `relative >= count`이면 `count`를 한 번 빼고, 그 외에 `relative < 0`이면 한 번 더한다.
3. `first + relative`의 signed DWORD를 원래 flags와 함께 실제 프레임 지정에 넘긴다.
4. 2번에서 보정했으면 참, 보정하지 않았으면 거짓을 반환한다.

길이 3의 첫 프레임에서 +9를 진행하면 보정 뒤 상대 번호가 6으로 남는다. 원본은 여러 바퀴를 반복해서 감싸지 않는다. 비연속 글자 배열도 첫 번호와 **전체 같은 글자 수**를 사용하므로 다음 번호의 글자가 다를 수 있다. `delta=0`도 실제 지정에 도달하며 기존 지정의 두 표시 갱신/단계 바이트 보정 규칙을 따른다. number/variant는 진행 조건에 쓰이지 않는다.

프레임 표는 참조하므로 현재 자산 표 교체가 다음 호출에 반영된다. 구성 때 같은 풀을 확인하고, 낙하 연결은 직접 지정과 진행이 **같은 SquidFrame 객체**를 쓰는지 확인한다. 풀·프레임 표·지정기는 연결보다 오래 살아야 한다.

## 표시·공간·낙하 연결

공통 지정은 [기존 계약](cpp-setframe-reconstruction.md)대로 현재 프레임의 크기와 저장된 해시 단계를 비교한다. 같은 단계에서는 old 표시→쓰기→new 표시다. 단계가 다르고 프레임도 바뀌면 실제 Unpop→쓰기→현재 위치 Pop(flags | 0x50)이다. 따라서 새 프레임 크기에 따른 해시 이동은 다음 변경에서 일어날 수 있다.

[`FrameAdvanceTests`](../../cpppj/tests/FrameAdvanceTests.cpp)의 공통 공간 검사는 실제 Factory·SHP 메타 공급·Display·Unpop·Pop을 사용한다. 작은 프레임→큰 프레임→다음 진행의 단계 1→3 이동과, 구간 끝→첫 프레임→다음 진행의 3→1 이동을 확인한다. 표시 때 읽는 프레임과 픽셀 영역도 관찰한다.

사제 검사는 실제 vtable·HP/이동 불가 조회·SharedRegular 타입 61·form/Kernel·0x25b와 실제 지정/진행/Display를 합성한다. 시작 A→J, J의 3→4→5→3, J 상태의 재요청 생략, 원본 0.1f 반복, 지도 WORD 65535의 착지 허용, 진행 뒤 착지 프레임 지정, 예약 제거를 두 raw 배치에서 검사한다. SHP main/그림자 영역도 실제 계산한다. 이 검사의 프레임 크기는 같은 해시 단계이며, 낙하 시작의 권한은 거짓이다. 보호막·소리는 호출 관찰 경계이고 일반 사제 공간 Pop은 연결하지 않았다.

## 독립 원본 대조

[`decomp_frameadvance_oracle.py`](../../tools/decomp_frameadvance_oracle.py)가 세 PE 각각 7,680개, 총 **23,040개**를 두 x87 정밀도에서 실행한다. 코드 배열 2(연속/비연속) × 실제 종류 플래그 4 × 저장 단계 5 × 현재 프레임 8 × signed 증분 12 × flags 2다. 증분에는 INT_MIN/INT_MAX, ±257, 구간 길이보다 큰 값과 0이 포함된다.

진행·지정·현재 프레임 단계 계산·헤더 조회·구판 방향·+0x4c 넘김은 실제 원본 명령이다. 글자 표도 각 PE/코드 배열/정밀도별 실제 Table로 생성한 바이트를 재사용한다. 목표 프레임과 반환값을 Python에서 계산하지 않는다. 표시(+0x88/+0x8c)·Unpop·Pop은 호출 시점 프레임과 인자 기록으로만 대체한다. 타입·코드·SHP는 합성 입력이다.

각 PE에서 실제 진행/지정 각각 **15,360회**, 타입 helper **46,080회**, 글자 표 생성 **4회**다. CD/추가 10.37의 방향 helper는 각각 15,360회다. 진행 정상 반환 46,080회와 표 생성 반환 12회를 검사했다. 비교 대상은 정수 반환·효과 순서/인자·슬롯 전체이며 EIP/스택·비영 보존 레지스터·SEH·x87 제어/TOP·허용 실행/쓰기·assert/OS 0을 검사한다. `--verify`는 SHA와 정확한 입력 순서/누락/중복도 감사한다. 이전 fixture/원본 감사 도구는 수정하지 않았다.

## 검증 및 남은 단계

Release 경고/오류 **0**, CTest 내부 **457개·실패 0**(111.87초)이다. [LEFT_JOBS.md](../../LEFT_JOBS.md)에 최종 인계를 남겼다. 새 C++ 검사 6개는 세 원본 관찰 재생, 실제 표시/공간, 실제 낙하/예약, 잘못된 연결/자산 표 변경 검사다. 전체 원본 근거 감사는 **69종 모두 통과**했다.

**일반 사제 Pop/Activate의 파생 postPop 연결, 보호막 Pop/destroy의 공간·소리 수명**이 다음 단계다. 이 어댑터는 프레임 훅을 실제 지정기로 바꾸며, 일반 사제 Pop을 자동 허용하지 않는다. raw GUI 건설·경제·전투·승패는 이어서 복원한다. outpost/LAN은 3차, 한국어·화면비·60/120프레임·요구사항/MCP는 4차다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name frameadvance
python -X utf8 tools/decomp_frameadvance_oracle.py
python -X utf8 tools/decomp_frameadvance_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```
