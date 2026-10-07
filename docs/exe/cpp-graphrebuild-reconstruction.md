# cpppj 전역 Graph 재구성·소진 할당 복원

2026-10-07. **1차 기준은 10.78**이다. [Graph](../../cpppj/src/o/Graph.cpp)의 전체 표면 재구성과 [RawGraph](../../cpppj/src/o/RawGraph.cpp)의 전체 풀 어댑터를 추가했다. `DESKTOP-HJOW`에서는 게임/복사본·업데이터·설치 도구·클론 창을 실행하지 않았다. Ghidra 읽기 전용 헤드리스, 제한된 Unicorn 함수 실행, 콘솔 빌드/CTest만 사용했다.

## 원본에서 확인한 순서

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 전역 재구성 | `00463110` | `0045bd60` |
| 표 초기화 | `00462b20` | `0045b570` |
| 번호 확보 / 소진 시 재구성 | `00463330` | `0045c390` |
| 표면 flood | `00462f10` | `0045bb90` |
| SID 순회 | `0040eb40` | 재구성 몸체에 인라인 |

전체 초기화는 앞 **251개 레코드의 surfaces/inUse WORD만** 지운다. 0번을 사용 중으로 예약하고 그 reserved WORD의 **low byte만 1**로 바꾼다. 251..253과 다른 reserved WORD는 보존하며 254번 sentinel의 앞 두 WORD를 `0x7dfd`로 복원한다.

그다음 첫 빈 번호를 임시 그래프로 확보한다. SID 오름차순으로 free 슬롯을 건너뛰고, Surface 타입이면서 기준점 spot의 내부 비트 8이 꺼진 표면을 임시 번호에 모은다. `resetAll=false`에서는 기존 번호가 254인 표면만 수집한다. 이 순회는 dead/void/contained/buried를 제외 조건으로 쓰지 않는다. 기준점 내부 표면의 기존 번호도 별도로 지우지 않는다.

두 번째 SID 순회는 **현재** 번호가 임시 번호인 표면마다 새 번호를 확보하여 원본 LIFO flood를 실행한다. 이전 flood가 번호를 바꾼 이웃은 다음 순회에서 다시 시작하지 않는다. 수집에서 제외했던 내부 표면도 기존 번호가 임시 번호이면 이 단계에서 처리된다. 마지막으로 임시 레코드의 앞 두 WORD만 지운다. 전체 소진 할당은 이 재구성을 마친 뒤 첫 빈 번호를 다시 확보한다.

## C++ 계약과 남은 연결

- `Graph::Rebuild(bool)`은 **전체 표면 스냅샷**에서 원본의 두 순회를 수행한다. `AllocateWithRecovery()`는 251개가 사용 중일 때 전체 초기화 재구성 후 번호를 확보한다.
- 기존 `Graph::Allocate()`는 지역 스냅샷에서도 쓰므로 소진 시 변경 전에 예외를 유지한다. 지역 Add/Detach에 전체 월드라고 가정한 자동 재구성을 넣지 않았다.
- `RawGraph::Rebuild(bool)`과 `RawGraph::Allocate()`는 매 호출 전체 할당 풀에서 타입/프레임/좌표/상태를 다시 읽고 graph byte·표·스택만 반영한다. 여유 번호 확보는 원본처럼 풀/프레임/지도를 읽지 않는다.
- 정수 지도 좌표·SID 5..32767·정상 타입/프레임 범위다. void/contained 표면도 전체 순회에 포함할 수 있지만 0단계 해시가 그런 슬롯을 가리키면 손상 입력으로 거부한다. 공간/좌표/owner/state/next/payload와 비표면 graph byte는 보존한다.
- raw 어댑터는 현재 원본 자산 타입(74 이상) 범위다. 미복원 form/process 내부 타입이 할당돼 있으면 전체 재구성 전에 거부한다. 이 제한과 자동 호출 연결을 해결한 뒤 실제 raw GameWorld에 적용한다.
- 손상 프레임/해시/범위·누락된 membership·재구성 중 재소진은 복사본에서 실패하여 모든 번호/표/스택을 보존한다. `Rebuild(false)`는 처음부터 빈 번호가 있는 입력만 지원한다. 그 경로의 중첩 전체 재구성은 후속이다.

예약 0번과 임시 번호를 유지하므로 한 번의 전체 재구성은 독립 무리 **249개까지** 만들 수 있다. C++ 검사는 249개 성공과 250개에서 변경 전 거부를 확인한다. 원본의 재귀 소진/assert 경로는 실행하지 않았다. 이 경계 검사는 아래 7슬롯 x86 행렬과 별도의 호스트 보호 검사다.

**SID 소진 복구는 별도 후속**이다. 10.82 DevLog의 10.73/74 기록(줄 1122)과 10.78 `004af1d0`의 다리 최대 50개 삭제 루프를 대조했다. Graph 번호 재구성에 이 파생 destructor/서버·클라이언트 통지를 섞지 않는다. Add/Detach/Pop의 자동 전역 재구성 연결, 건물 부착/dirty/grid, 특수 타입 위치 조회, 파생 삭제/참조 수명, raw GameWorld도 남았다. 미션 완주는 아직 불가능하다.

## 독립 검증과 재현

[새 도구](../../tools/decomp_graphrebuild_oracle.py), [고정 fixture](../../cpppj/tests/fixtures/graphrebuild-x86.tsv), [SHA/호출 기록](../../cpppj/recovery-graphrebuild-evidence.json). 기존 도구/fixture/기록은 변경하지 않았다.

세 실제 PE × x87 53/64비트 × 16입력 × 세 경로 = **288회**(직접 Rebuild 192·소진 Allocate 96). 각 실제 **32768슬롯 전체 순회**를 실행하며 iterator를 건너뛰거나 호스트 함수로 바꾸지 않는다. 명령마다 Ghidra 불연속 몸체 범위를 확인하고, assert/OS/미검토 코드/허용 범위 밖 쓰기는 즉시 실패한다. 이미 확보한 스택과 로그 0을 사용하여 할당/로그 I/O를 자연스럽게 피한다. 판본별 내부 Rebuild 96·Allocate 406·Flood 278 도달은 공개 호출 수에 더하지 않는다. 실제 Reset 2/Create 14회 준비도 별도다.

7개 raw 슬롯과 255개 Graph 레코드는 전체 바이트를 직접 비교한다. 전체 풀/네 단계 해시/spot/4096 DWORD 스택/통계는 Adler-32, dirty 항목은 직접 비교다. 합성 타입/FrameCode/SHP·정수·비전투 null 큐 범위이며 실제 플레이 증명이 아니다. 최종 Release/CTest 결과는 [LEFT_JOBS](../../LEFT_JOBS.md)의 최신 절에 기록한다.

새 디컴파일은 다음 명령으로 읽기 전용 프로젝트에서 재현한다. 세 로그의 완료 마커는 각각 5/7/7개다.

```powershell
& tools/ghidra/run_script.ps1 -Edition originals -Script ExportCreation.java -ScriptArgs @('extracted/graphrebuild/originals','00463110','00462b20','00462ac0','0040eb40','00463330')
& tools/ghidra/run_script.ps1 -Edition originalCD -Script ExportCreation.java -ScriptArgs @('extracted/graphrebuild/originalCD','0045bd60','0045b570','0045b560','0045b4d0','0045b4e0','0045b520','0045c390')
& tools/ghidra/run_script.ps1 -Edition original1037 -Script ExportCreation.java -ScriptArgs @('extracted/graphrebuild/original1037','0045bd60','0045b570','0045b560','0045b4d0','0045b4e0','0045b520','0045c390')
python -X utf8 tools/decomp_graphrebuild_oracle.py
python -X utf8 tools/decomp_graphrebuild_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```

Ghidra/Unicorn 재생성은 기존 SID/생성 및 `extracted/graphremove/` 내보내기도 필요하다. 일반 콘솔 CTest는 고정 fixture를 사용하므로 원본 실행 파일/Python/Ghidra가 필요 없다. **GUI 회귀는 `DESKTOP-HJOW` 이외의 PC에서** raw 월드 연결 후 진행하며, 원본 비교 실행은 해당 AGENTS.md/사용자 지시를 확인한다.
