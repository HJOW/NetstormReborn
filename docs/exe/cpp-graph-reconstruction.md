# cpppj 표면 그래프 연결·flood·병합 복원

2026-10-06. 원본 Graph.cpp의 기존 표/스택·소진 전 경로를 정수 표면 스냅샷 계층에 복원했다. `Graph`는 `SurfaceFinder`의 실제 이웃 순서를 사용하며 타입 프레임/발자국/내부/죽음 필터를 공유한다. 후속에서 [raw SID·공통 postPop/지원 Pop 연결](cpp-rawgraph-reconstruction.md)을 추가했으며 GameWorld는 아직 별도다.

## 원본 근거와 동작

| 경로 | 패치 10.78 | CD 10.72 |
|---|---|---|
| 표면 등록/연결 병합 | `004633c0` | `0045b7d0` |
| LIFO flood | `00462f10` | `0045bb90` |
| 첫 미사용 그래프 확보 | `00463330` | `0045c390` |
| 그래프 반납 | `00462e50` | `0045c460` |
| 표면 수 감소 | `00462a50` | `0045b530` |
| SID의 그래프 레코드 조회 | `00462c80` / `00462bc0` | `0045b7a0` / `0045b6f0` |
| raw 그래프 byte 읽기/쓰기 | `004ad390` / `004ad3e0` | `004adbc0` / `004adbd0` |

읽기 전용 헤드리스 Ghidra로 패치 13개/CD 16개 몸체를 내보냈다. 출력은 `extracted/graph/<판본>/creation.c`, `functions.tsv`, 완료 로그는 `extracted/graph-originals-ghidra.log`, `extracted/graph-cd-ghidra.log`다. 영역 통지/삭제 분할 입구도 분석 자료로 내보냈지만 이번 실행 fixture는 그 경로에 들어가지 않는다. 대응 2,496쌍·검토 앵커 24쌍은 유지한다. 원본 게임 프로세스·OS API를 실행하지 않았다.

그래프 레코드는 6바이트이며 첫 WORD는 signed 표면 수, 둘째는 사용 여부다. 셋째 WORD는 이번 경로에서 변경하지 않는다. 정상 번호는 0~250, 무효 번호는 254다. sentinel의 앞 두 WORD는 `0x7dfd`이며 사용 개수에 포함하지 않는다. 할당은 첫 미사용 레코드의 앞 두 WORD를 0/1로 설정하고, 반납은 0/0으로 만든다. 표면 수 감소는 사용 중이고 양수일 때만 수행하며 0이 되면 미사용으로 전환한다.

flood는 4,096 DWORD 스택을 사용한다. 현재 객체의 그래프가 목적 번호와 같으면 이웃을 확장하지 않는다. 다른 번호이면 기존/목적 그래프의 표면 수를 각각 감소/증가시키고 byte 번호를 바꾼 뒤, y/x 탐색 순서의 이웃을 push한다. 마지막 이웃부터 방문하고 중복 push도 보존한다. 목적 번호 254에서는 기존 수만 감소시킨다. 직접 flood는 `inUse`를 자동으로 켜거나 지우지 않는다. 카운터는 16비트로 감긴다.

**표면 수는 객체 방문마다 1 증가한다. 여러 칸 발자국도 한 객체다.** 앞선 [다리 붕괴 분석](bridge-pieces.md)의 모든 섬을 칸 수로 센다는 설명은 정정했다. 실제 섬 저장 객체와 월드 연결까지 복원한 결과로 확대하지 않는다.

등록은 유효 좌표·free/dead/void가 아닌 객체만 처리한다. 이웃 그래프 번호의 중복을 제거하고 가장 큰 signed 표면 수를 선택한다. 크기가 같으면 먼저 탐색한 번호가 남는다. 연결이 없으면 새 그래프를 확보해 flood한다. 연결이 있으면 현재 객체를 승자에 붙여 1 증가시키고, 다른 연결을 승자로 flood한 뒤 패배 레코드를 반납한다. 기존 번호를 가진 객체에 등록을 다시 호출할 때도 원본의 증가를 보존한다.

## C++ 범위

[Graph.h](../../cpppj/src/o/Graph.h), [Graph.cpp](../../cpppj/src/o/Graph.cpp)는 표면 스냅샷 번호·raw graph/state 입력을 받으며, 레코드/스택/번호를 소유한다. 변경 결과는 읽기 전용으로 제공한다. `SurfaceFinder` 수명은 Graph보다 길어야 한다. 비표면 객체의 위치 조회는 지원하지 않는다.

손상된 번호/누락 membership·잘못된 표 크기·미복원 그래프 소진·과도한 연결/스택은 예외로 보고한다. flood/등록은 임시 복사본에서 처리하므로 보호 경로의 부분 변경을 호출자에게 전파하지 않는다. 이는 원본 assert UI/오류 복구의 재현이 아니다. 정상 입력의 방문/표 갱신 순서는 원본대로 보존한다.

## 검증과 재현

[독립 실행 기록](../../cpppj/recovery-graph-evidence.json), [기대값](../../cpppj/tests/fixtures/graph-x86.tsv), [실행 도구](../../tools/decomp_graph_oracle.py), [C++ 검사](../../cpppj/tests/GraphTests.cpp).

기존 [표면 탐색의 내보내기](cpp-surface-reconstruction.md)와 위 Graph 함수의 새 `ExportCreation.java` 내보내기를 준비한 뒤 실행한다.

```powershell
python -X utf8 tools/decomp_graph_oracle.py
python -X utf8 tools/decomp_graph_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

두 판본×x87 53/64비트의 4시퀀스에서 **2,560회**(등록/flood/감소/반납/할당 각 512회)를 대조했다. 체인·고리·떨어진 무리·다리/섬 혼합 프레임·여러 칸 발자국·내부/죽음/void/무효 좌표·동률/서로 다른 그래프·무효 번호 flood/반납·signed WORD 경계를 포함한다. 합성 타입/프레임·기존 표/스택·소진 전 입력이며 게임 초기화/생성자를 실행하지 않는다.

표 255레코드와 스택 4,096 DWORD의 전체 Adler-32, 모든 membership의 graph/state를 대조한다. 표/스택의 체크섬은 모든 바이트 직접 비교와 구별한다. 원본 탐색/기하/가상 필터/그래프 함수는 실제 명령을 실행하며 대체 함수/assert 도달은 0이다. 로그 수준을 낮춰 실제 로그 함수의 자연 조기 반환을 사용한다. malloc과 소진 복구는 실행하지 않는다. `--verify`로 원본/도구/부모 도구/fixture SHA-256과 호출 수를 확인한다.

x64 Release 경고/오류 0, CTest 내부 **128개·실패 0**. 동률의 첫 이웃 선택/패배 무리 병합·여러 칸 객체의 수 1·16비트 감김·미사용 WORD 보존·소진/누락/번호 손상의 변경 전 거부도 검사했다. 누적 **56,028개**는 제한 x86 입력 수이며 게임 전체 완성도가 아니다. 기존 공간 878개는 의존성 계약 대체, 수명 480개는 접두 구간, 해시 초기화 4개는 할당 없는 경로이고 생성자 359행/vtable 151행은 별도다.

이번 단계는 클론 창을 실행하지 않았다. Graph가 아직 기존 GUI 월드에 연결되지 않았으므로 단위/기계어 검사로 새 계산을 확인했다. 기존 창 허용은 유지하며, 연결 후 실제 클론 조작을 검사한다.

## 다음 연결

raw SidPool의 graph/state/실제 프레임과 0단계 해시·spot에서 스냅샷 구성·graph byte 반영·공통 postPop surface 분기는 [후속](cpp-rawgraph-reconstruction.md)에서 완료했다. 주변 영역 통지·정상 다리/섬 Pop·삭제 시 그래프 분할·소진 복구·섬/다리/건물 부착·raw GameWorld는 남았다. 생산 계산/건설/경제/전투/AI/승패와 미션 완주는 남았다.
