# cpppj 주변 영역 그래프 무효화와 일반 다리/섬 Pop

2026-10-07. [기존 raw Graph 연결](cpp-rawgraph-reconstruction.md)에 원본 영역 helper `00462d40` ↔ CD/10.37 `0045c210`을 연결했다. [구현](../../cpppj/src/o/RawGraph.cpp), [검사](../../cpppj/tests/RawGraphTests.cpp), [x86 도구](../../tools/decomp_regiongraph_oracle.py), [근거](../../cpppj/recovery-regiongraph-evidence.json).

## 원본 동작과 C++ 연결

원천 객체의 발자국 경계로 일반 SquidFinder를 만들고 0~3단계 해시의 y/x 버킷·next 체인을 순회한다. 끝 버킷은 기준점보다 한 칸 넓어지며 각 단계 경계로 제한한다. 후보의 발자국이 원천 경계와 교차하고 기준점 spot에 내부 비트 `8`이 있으면 Surface(`flags1 & 0x800`)의 기존 그래프를 Remove한 뒤 번호를 `254`로 바꾼다.

일반 finder는 매몰 extra `8`을 건너뛰고 **dead 상태의 후보를 제외하지 않는다**. 이는 기존 표면 이웃 탐색과 다른 경로다. 비표면/교차하지 않는 발자국/내부 비트가 없는 후보는 무효화하지 않는다. 이미 `254`인 번호는 감소하지 않는다. Remove는 활성 레코드의 양수 개수만 줄이고 0이 되면 inUse를 지우며 reserved WORD와 flood 스택은 보존한다.

공통 postPop에서 graphsEnabled, `type.flags2 & 0x50444208`, `flags & 3`, noGraph 비트 없음인 경우 영역 helper를 호출한 뒤 기존 Surface Add 분기를 실행한다. 같은 계산 복사본의 번호/표를 Add가 읽고 전체 성공 후 반영한다. noGraph 리셋과 기존 비용·목록·통계·깊이 처리는 유지한다.

Pop 사전 검사도 최종 좌표·void 해제·새 해시 머리/next·genus OR 이후 spot을 반영한다. 옛 void 좌표로 영역을 조회하거나 Add 전에 그래프 표를 부분 변경하지 않는다. 해시 체인 순환, 미할당 후보, 잘못된 번호/발자국, 소수 좌표, 소진은 실제 공간 쓰기 전에 거부한다. 영역 처리 후 Add가 실패하는 경우에도 graph byte·표·스택·슬롯·해시·spot·비용/깊이를 보존하는 검사를 추가했다.

일반 다리/섬 Pop의 비전투 알림 helper는 원본에서 dirty 큐가 null이면 자연 반환한다. 이 범위에서 기존 등록·표시·firstPop·postPop을 허용했다. 전투 dirty 큐를 구현한 것은 아니며 건물 옆면 부착은 계속 거부한다. **일반 다리/섬 Unpop과 삭제 시 분할, 전역 그래프 소진 복구, 건물 부착, raw GameWorld는 남았다.**

## 대조 범위와 재현

세 실제 PE × x87 53/64비트, 6시퀀스의 공개 호출 **1,536회**를 사용한다. CD와 새 10.37의 그래프 코드 배치는 같으므로 서로 다른 게임 알고리즘은 두 종류다. [구버전 비교](original1037-comparison.md).

| 공개 호출 | 횟수 |
|---|---:|
| 영역 helper | 480 |
| 직접 공통 postPop | 480 |
| Pop | 288 |
| 직접 Add | 288 |

새 Ghidra 내보내기는 `extracted/regiongraph/<판본>/creation.c`, `functions.tsv`, `bounds/`다. 패치 13+3개, CD/10.37 각 14+1개 함수이며 CD/10.37의 InsertCD 비교 함수는 실행 허용 목록에서 제외한다. 같은 버킷의 next 체인, 네 단계, 죽은/매몰/비표면/이미 무효인 후보, 원천 경계 밖 기준점과 교차하는 큰 발자국, 기존 비활성/0/음수 WORD 표, Pop의 옛 좌표 90→새 좌표 20을 포함한다.

7개 슬롯의 모든 바이트와 모든 dirty 항목을 직접 비교한다. 전체 풀·4단계 머리·spot·표·4096 DWORD 스택·세 통계 표는 Adler-32다. 대체 함수/assert 없이 실제 Reset/Create/일반 탐색/후처리/Graph를 실행하고 EIP/ESP/x87/쓰기 범위를 확인한다. 준비 Reset/Create, 비용 인코딩 접두 구간과 내부 호출은 상위 공개 호출 수에 더하지 않는다.

패치 Pop 96회 중 32회는 기존 내부 spot과 충돌하여 void를 유지하고 앞선 좌표/spot 쓰기만 남긴다. CD/10.37은 같은 입력에서 등록을 계속한다. 실제 raw 출력의 void 상태로 Pop 결과를 구분한다. 이 차이 때문에 내부 영역/Add 도달 수가 판본별로 다르며 공개 호출 수에는 충돌 호출도 포함한다. 최종 x64 Release 경고/오류 0, CTest 내부 **134개·실패 0**을 확인했다. 제한 x86 입력 누적은 **59,100개**이며 게임 완성도를 뜻하지 않는다.

합성 타입/FrameCode/SHP, client SID 32768, 정수 좌표, 기존 확보 표/스택, 비전투 dirty 큐 null, AI 미부착/배치 선택 없음이라는 한계가 있다. 일반 finder 전체의 부동소수점/roof/특수 flag 경로를 복원한 것은 아니다. 두 기존 원본, 추가 원본, 도구/부모 도구/내보내기/fixture SHA와 호출 수를 `--verify`로 확인한다.

```powershell
python -X utf8 tools/decomp_regiongraph_oracle.py
python -X utf8 tools/decomp_regiongraph_oracle.py --verify
python -X utf8 tools/compare_original1037.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

빌드 종료 후 검사를 실행한다. 이번 단계는 GUI GameWorld에 아직 연결되지 않은 raw 계층의 기계어/단위 검사다. 기존 클론 창 실행 허용은 유지하며 raw 월드 연결 후 TEST01/1-1 생성·표시·선택·이동·해제·재진입을 검사한다.
