# cpppj raw SID 그래프와 공통 postPop 연결

2026-10-06. [독립 Graph 계산](cpp-graph-reconstruction.md)을 실제 `SidPool`의 graph/state/프레임, `SquidHash`의 0단계 머리, 공유 spot에 연결했다. 공통 postPop의 표면 생성 분기와 지원하는 Pop에서 선택적으로 사용한다. 기존 GUI의 GameWorld는 아직 별도다.

## 입력과 반영

[RawGraph.h](../../cpppj/src/o/RawGraph.h), [구현](../../cpppj/src/o/RawGraph.cpp)은 타입과 실제 `FrameCode` 배열을 복사하고 풀·해시·spot을 참조한다. 프레임 코드는 SHP 표시 경계 헤더와 구별한다. 패치 frame은 +36 DWORD, graph는 +30 byte이며 CD는 +34/+28 byte다. 각 계산에서 현재 슬롯을 다시 읽어 프레임 변경과 noGraph 리셋을 반영한다.

0단계 머리를 그대로 SurfaceFinder 지도에 전달한다. 다중 칸 발자국 전체를 임의의 SID로 채우지 않는다. 살아 있는 표면과 지도에 등장하는 비표면 후보를 읽고 기존 표면/죽음/내부/프레임 연결 필터를 적용한다. 표면 SID는 5~32767, 좌표와 발자국은 지도 안 정수 입력으로 제한한다. 해당 스냅샷의 표면 프레임도 유효해야 한다. 일반 소수 좌표 이동 경로를 근사하여 지원하지 않는다.

등록/flood는 복사본에서 완료한 뒤 변경한 슬롯의 graph byte만 쓴다. 다른 raw payload와 state는 보존한다. 그래프 표의 reserved WORD와 4096 DWORD 스택의 오래된 값도 다음 스냅샷으로 이어 받는다. 소진·손상 지도/프레임·누락 표면·과도한 스택은 쓰기 전에 거부하며 원본 assert UI/오류 복구를 재현한 것은 아니다.

## Pop과 후처리

`SquidPostPop(pool, types, state, &graph)`로 연결하며 기본 null 인자는 기존 호출을 유지한다. 풀·표면 타입·해시·spot이 다른 연결은 거부한다. graphsEnabled 상태에서 Surface이고 flags에 `0x203`이 있으며 noGraph(`0x2000000`)가 없으면 원본 Add를 수행한다. noGraph의 최초 등록 리셋은 이전대로 graph byte만 바꾸며 표면 수를 임의로 보정하지 않는다.

Pop의 사전 검사는 최종 좌표, void 해제, 0단계 머리 등록, 발자국 genus OR 이후 spot을 예측한다. 옛 void 상태로 검사하여 실제 Add의 소진을 놓치는 경우를 막는다. 성공한 실제 Pop은 기존 순서대로 공간·표시·firstPop·void 해제를 마친 뒤 Activate/postPop에서 그래프를 갱신한다. 겹침의 원본 조기 반환 동작은 기존 Pop의 책임이다.

이번에 연결한 Pop은 기존 지원 범위의 일반 표면과 매몰 다리다. **정상 다리/섬 Pop의 영역·부착 효과와 postPop 주변 영역 통지는 아직 지원하지 않는다.** AI 부착과 배치 선택 해제도 기존 사전 거부를 유지한다. `0x800` destroyGraph assert 분기를 정상 경로로 바꾸지 않았다.

## 실제 x86 대조

[근거](../../cpppj/recovery-rawgraph-evidence.json), [기대값](../../cpppj/tests/fixtures/rawgraph-x86.tsv), [도구](../../tools/decomp_rawgraph_oracle.py), [C++ 검사](../../cpppj/tests/RawGraphTests.cpp).

기존 읽기 전용 Ghidra의 SID/생성/Pop/표시/postPop/Graph/표면 내보내기를 재사용한다. 새 함수 대응을 추가하지 않았으며 기존 2,496쌍/검토 앵커 24쌍을 유지한다. 도구는 검토한 함수 몸체·쓰기 범위만 허용하고 실제 Reset/Create로 7개 SID를 확보한다. 게임 진입점·OS·원본/복사본 게임 프로세스를 실행하지 않는다.

```powershell
python -X utf8 tools/decomp_rawgraph_oracle.py
python -X utf8 tools/decomp_rawgraph_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

두 판본×x87 53/64비트의 4시퀀스에서 **1,536회**를 대조했다.

| 공개 호출 | 횟수 |
|---|---:|
| 직접 공통 postPop | 640 |
| Pop | 128 |
| 직접 Add | 512 |
| 직접 Flood | 256 |

합성 타입/FrameCode/SHP, 기존 확보 풀·표·스택, 비전투·AI 없음·배치 선택 없음·소진 전 범위다. 떨어진 무리·동률 병합·다중 칸·죽은/비표면/내부 후보·현재 프레임 번호·noGraph 리셋 후 재등록을 포함한다. 대체 함수와 assert 도달은 0이다. 각 판본 준비 Reset 2/Create 14회와 패치 비용 인코딩 896회 접두 구간은 공개 호출 수에 포함하지 않는다. 내부 그래프/표시/postPop 도달도 상위 호출에 이미 포함된다.

7개 raw 슬롯은 모든 바이트를, dirty 표는 모든 좌표/플래그/순서를 직접 비교한다. 전체 풀/4단계 해시/spot/그래프 표/스택/세 통계 표는 Adler-32로 비교한다. 체크섬 대조를 전체 바이트 직접 비교로 표시하지 않는다. `--verify`는 PE·도구·부모 도구·fixture SHA-256과 호출 수를 확인한다.

x64 Release 경고/오류 0, CTest 내부 **132개·실패 0**. 그래프 소진/소수 좌표의 Pop 쓰기 전 거부, 프레임·지도·raw membership 재판독, 손상 후보·다른 풀/지도/타입 연결·미복원 영역 통지도 검사했다. 최초 검사를 빌드 완료 전 시작해 실행 파일 잠금으로 링크가 실패했으며, 검사 종료 후 빌드→전체 검사 순서로 다시 완료했다. 누적 제한 x86 입력은 **57,564개**이고 게임 전체 완성도가 아니다. 앞선 공간 878개는 의존성 계약 대체, 수명 480개는 접두 구간, 해시 초기화 4개는 할당 없는 경로다.

이번 단계는 새 GUI 검사를 실행하지 않았다. raw Graph가 기존 GUI GameWorld에는 아직 미연결이므로 새 동작은 raw 기계어/단위 검사로 확인했다. 이전 PC의 클론 창 허용은 유지한다. AGENTS.md·기존 원본 자료·C#은 수정하지 않았다.

## 다음 연결

먼저 주변 영역 통지 `00462d40` ↔ CD `0045c210`과 정상 다리/섬 Pop의 영역·부착 효과를 복원한다. 이어 삭제 시 그래프 분할·소진 복구·dirty/grid·참조 수명과 raw GameWorld를 연결해야 한다. raw 월드 연결 뒤 TEST01/1-1의 생성·표시·선택·이동·해제·재진입을 클론 창에서 검사한다. 다리 배치/Construction·생산 덱/자원/SP 차감·건설/경제/전투/AI/승패·미션 완주는 남았다.
