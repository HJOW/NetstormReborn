# cpppj 삭제 준비 분할과 일반 다리/섬 Unpop

2026-10-07. [영역/일반 Pop](cpp-regiongraph-reconstruction.md)에 이어 삭제 준비 Graph 분할과 일반 다리/섬의 비전투 공간 해제를 복원했다. [Graph](../../cpppj/src/o/Graph.cpp), [raw 연결](../../cpppj/src/o/RawGraph.cpp), [Unpop](../../cpppj/src/o/SquidUnpop.cpp), [콘솔 검사](../../cpppj/tests/RawGraphTests.cpp), [독립 x86 기록](../../cpppj/recovery-graphremove-evidence.json), [실행 도구](../../tools/decomp_graphremove_oracle.py).

이번 사용자 지시에 따라 원본/복사본 게임 프로세스와 클론 창을 실행하지 않았다. Ghidra 헤드리스·읽기 전용 분석, 허용한 함수의 격리 에뮬레이션, Release 빌드와 콘솔 CTest만 사용했다. 창 검사는 raw GameWorld 연결 후 다른 PC에 인계한다.

## 디컴파일과 판본

추가 `original1037/netstorm.exe`를 별도 프로젝트에서 다시 전체 디컴파일했다. **3,711개 함수·실패 0**, `extracted/original1037/decomp/netstorm.c`와 `functions.tsv`다. GIF 분석 경고는 있었지만 분석/내보내기/저장은 완료했다. 이번 전체 산출물 SHA는 새 독립 기록에 저장했고 과거 기록의 SHA는 변경하지 않았다.

CD와 새 10.37 실행 파일의 전체 차이는 파일 `0003314c`/VA `00433d4c`의 `75→eb` 한 바이트다. `InsertCD` 창 생성 분기를 건너뛰는 정적 변경이며 전체 CD 검사 우회를 증명하지 않는다. 두 `netstorm.ver`는 모두 10.37이고 기존 `Cd1072` 구조 식별자는 유지한다. 패치 10.78을 복원 기준으로 사용한다. [자료 비교](original1037-comparison.md).

현재 PC에 없던 과거 영역/그래프 내보내기는 기존 기록을 덮어쓰지 않고 새 `extracted/graphremove/<판본>/`에 통합했다. 읽기 전용 `ExportCreation.java`로 패치 **95+기하 2개**, CD/10.37 각각 **90+기하 2+위치 조회 1개**를 내보냈다. 완료 로그와 불연속 함수 몸체를 확인했고 프로젝트 변경은 버렸다. SID/기본 생성의 기존 추출물과 준비/관찰 도구를 공유한다. 기존 대응 2,496쌍·검토 앵커 24쌍은 유지한다.

| 역할 | 패치 10.78 | CD/추가 10.37 |
|---|---|---|
| 삭제 준비/분할 | `004637b0` | `0045bfd0` |
| 일반 위치/타입 finder 생성 | `004b24c0` | `004ebbe0` |
| 발자국 인접 교차 | `0041d9b0` | `0043fe10` |
| 사각형 교차 | `0041d8c0` | `0043fa70` |
| 일반 Unpop | `004afe50` | `004ad0b0` |
| 비전투 표면 알림 | `00481510` | `00442f30` |

## 실제 순서와 C++ 계약

원본 삭제 준비는 8 DWORD 위치/타입 정보에서 x/y, 다섯째 type, 여덟째 **프레임 번호**를 읽는다. 프레임 번호는 FrameCode 자체와 구별한다. `GetGridSid`는 해시 객체 `+12`의 **0단계 기준점 머리**를 읽는다. 별도 발자국 지도나 네 단계 전체 조회가 아니다. 0단계 머리가 없으면 그래프 분할은 자연 반환한다. `RawGraph::Detach`는 SID에서 같은 위치/타입 정보를 구성하고 같은 머리 확인을 한다. 당시 미지원이던 동일 기준점의 정상 다른 SID 조회는 [추가 판본 비교 후속](cpp-reference-versions.md#cpppj-후속-같은-위치의-다른-sid-조회)에서 별도 x86 384회로 복원했다. 아래 137개/1,158회는 최초 단계 기록이다.

삭제할 원천은 호출자가 이미 **dead bit 2**로 표시해야 한다. 일반 finder는 네 해시 단계→y/x 버킷→next 순서로 살아 있는 표면 후보를 검사한다. 원천/후보 발자국의 가로 확장 교차와 세로 확장 교차 중 하나만 참이어야 한다. 매몰/죽은 후보와 내부 spot을 제외하고 실제 타입/프레임의 연결을 확인한다. 이후 flood는 기존 flag 8 이웃 탐색을 사용한다. 삭제의 초기 이웃 목록을 flood 이웃 목록으로 대체하지 않는다.

패치 디컴파일의 `(작음 != 같음)`은 정상 정수 좌표에서 `<=`다. 세 판본 모두 경계 접촉을 교차로 인정한다. 실제 함수 진입 시 ECX 사각형/스택 인자를 관찰하여 확인했다. 처음에는 이를 `<`로 읽었고, 기계어 대조로 수정했다. 위치 조회 역시 별도 grid라고 해석했던 부분을 실제 전역 주소와 조회 명령으로 정정했다.

발자국 helper의 두 번째 점은 1..255로 보정하며 초기 점이 무효이면 양 끝을 그 점으로 재설정한다. 가장자리 대조에 이 보정을 포함했다. CD/10.37 일반 finder는 매몰 제외 뒤 후보의 좌표가 0이면 dead/표면 필터 전에 무조건 `pos.isValid()` assert를 호출한다. 탐색 중 이 경로에 도달하면 oracle은 즉시 중단하며 UI/OS를 실행하지 않는다. C++는 상태 변경 전 예외로 거부한다. 최종 기대값은 패치의 0 후보와 CD/10.37의 최소 1 후보를 각각 사용한 정상 반환 입력이다.

기존 그래프 `254` 또는 미사용 레코드는 분할하지 않는다. 탐색 순서대로 현재 이웃 번호를 다시 읽고, 같은 기존 그래프에 남은 무리를 새 번호로 flood한다. 일반 모드는 마지막 연결 하나를 기존 그래프에 남길 수 있고 전역 rebuild 정책은 각 연결을 분할한 뒤 기존 레코드를 반납한다. 다른 그래프 번호의 이웃도 남은 연결 수에 포함한다. 연결 목록이 비면 일반 모드는 개수를 감소하지 않는다. 감소는 보통 1, 특수 타입은 9이며 signed WORD 감김·reserved WORD·미사용 스택 흔적을 보존한다. 원천의 graph byte를 별도로 지우지 않는다.

`ValidateDetach`/`Detach`는 복사본에서 계산하고 성공 시 번호/표/스택만 반영한다. 소진·잘못된 SID/프레임·체인 순환·다른 위치 SID·과도한 연결은 공간 해제 전에 거부한다. rebuild **분할 정책**을 복원한 범위이며 전역 그래프 소진 복구는 아직 구현하지 않았다.

일반 다리/섬 `Unpop`은 비전투 알림 큐 null의 실제 자연 반환 범위에서 허용한다. 기존 void 표시→genus spot AND→해시 머리/이전 next 제거→선택한 공통 표시 갱신 순서를 유지한다. 두 번째 Unpop은 void 조기 반환한다. 건물 부착은 계속 거부한다. 원본 Unpop 자체는 Graph 분할을 호출하지 않으므로 `Detach`와 별도 호출이며, 상위 파생 삭제/참조 수명을 복원한 것이 아니다.

## 독립 기대값과 검증

세 실제 PE×x87 53/64비트, **6시퀀스·1,158회**다. CD/10.37은 같은 코드 배치지만 각각의 실제 PE를 별도로 매핑한다. 함수 몸체에 유일한 분기 변경 주소가 포함되면 거부한다. 함수/OS 대체 없이 실제 명령을 실행하며 동결 시계와 null 비전투 알림 큐의 자연 반환을 사용한다. assert 도달은 0이다.

| 공개 호출 | 횟수 |
|---|---:|
| 삭제 준비 Detach | 384 |
| 일반 Unpop | 768 |
| void SID 반납 | 6 |

각 PE의 실제 내부 도달은 Detach 128, Unpop 256, 비전투 알림 86, 반납 2회다. Allocate/Flood는 패치 각각 38, CD/10.37 각각 52회이며 가장자리 입력의 차이를 포함한 도달 수다. 같은 입력에 대한 알고리즘 차이로 해석하지 않는다. 준비 Reset 2/Create 14회와 내부 도달은 공개 호출 수에 더하지 않는다. 0단계 원천 부재·분리된 무리·별도 번호·무효 번호·미사용 음수 레코드·dead/매몰/내부 후보·지도 가장자리·1/9 감소·일반/rebuild 정책·표시 플래그·반복 해제·반납을 포함한다.

7개 raw 슬롯의 모든 바이트와 dirty 항목은 직접 비교한다. 전체 풀/4단계 해시/spot/255개 레코드/4096 DWORD 스택/통계는 Adler-32다. 기존 raw/영역 fixture는 수정하지 않았다. 합성 타입/FrameCode/SHP, client SID 32768, 정수 좌표, 기존 확보 표/스택, 소진 전·비전투·AI/배치 선택 없음 범위다.

최종 x64 Release 경고/오류 0, CTest 내부 **137개·실패 0**, 새 기록의 원본/도구/부모 도구/내보내기/전체 10.37 산출물/fixture SHA·호출 수 확인을 완료했다. 제한 x86 입력 누적은 **60,258개**이며 게임 완성도가 아니다.

```powershell
& tools/ghidra/run_decomp.ps1 -Edition original1037
# 필요한 SID/기본 생성 추출물을 앞선 복원 문서대로 준비한 뒤 새 함수 묶음을 내보낸다.
$plan = Get-Content -Encoding UTF8 tools/ghidra/graphremove-functions.json | ConvertFrom-Json
foreach ($edition in 'originals', 'originalCD', 'original1037') {
    $groups = $plan.editions.$edition
    foreach ($group in $groups.PSObject.Properties) {
        $suffix = if ($group.Name -eq 'base') { '' } else { '/' + $group.Name }
        $output = "extracted/graphremove/$edition$suffix"
        & tools/ghidra/run_script.ps1 -Edition $edition -Script ExportCreation.java -ScriptArgs (@($output) + @($group.Value))
    }
}
python -X utf8 tools/decomp_graphremove_oracle.py
python -X utf8 tools/decomp_graphremove_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

Ghidra 스크립트는 프로세스 종료 코드 외에 각 `생성/Take 디컴파일 완료` 로그도 확인한다. 빌드가 종료한 뒤 CTest를 실행한다. 전체 Ghidra 분석을 다른 환경에서 재생성하면 함수 몸체/출력 SHA가 달라질 수 있으므로 기존 기록의 검증과 새 기록의 생성은 구별한다.

## 후속 인수인계

전역 그래프/SID 소진 복구, 동일 위치의 다른 SID 조회, 건물 부착·dirty/grid 갱신·참조 수명·파생 destructor·form/process·상위 수신 목록과 raw GameWorld 연결이 남았다. 이어 생산 덱/자원/SP 차감·Construction·다리 배치·경제/전투/AI/승패를 진행한다. raw 월드 연결 후 TEST01/1-1의 생성·표시·선택·이동·해제·재진입 창 검사는 **다른 PC에서** 수행하고 현재 PC에서는 실행하지 않는다. [최신 인수인계](../../LEFT_JOBS.md).
