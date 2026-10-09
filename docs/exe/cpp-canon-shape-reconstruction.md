# 일반 자산과 특수 패턴의 픽셀 범위 복원

2026-10-09, 마지막 디컴파일 PC **HJOW-Athlon**. AGENTS.md와 두 인수인계 문서를 확인했으며 기존 호스트와 일치한다. `canonshape` 함수 목록을 같은 PC에서 읽기 전용으로 내보냈다. 원본 게임·복사본·클론 창을 실행하지 않았다.

## 복원한 동작

원본 `0043cbd0` / CD·추가 10.37 `004ed7c0`은 사제뿐 아니라 모든 일반 타입과 특수 패턴을 처리한다. `CanonDecoder`를 로컬 좌표 `(0,0)`에서 생성하고 모든 유효 칸의 SHP 메타 헤더를 조회한다. 각 칸 좌표에 현재 픽셀 배율을 곱하고 타입 기준점을 보정한 뒤 최소/최대 범위를 합친다. 최초 엄격한 최솟값 갱신 때의 기준점 보정값도 반환한다.

`RawCanonPixelShape`는 이 전체 경로를 담당한다. `RawPriestPlacementShape`는 기존 사제/비패턴 입력 계약을 검사한 뒤 같은 계산기로 위임한다. 현재 타입 프레임 표/defaultFrame·SHP 물리 헤더·기준점·배율과 패턴 타입 전역을 매 호출 읽는다. 실제 자산은 기존 `PriestPlacementAssets`가 공급한 동일 자료를 사용한다.

- 영역 68개·다리 26개·섬 2개·받침 1개와 공유 전투 패턴을 지원한다. 빈 칸 건너뛰기와 회전/별칭 우선순위는 공통 decoder가 담당한다.
- 패치의 논리 프레임/이중 프레임·해제 표식 검사와 CD의 직접 물리 헤더 참조를 구별한다.
- signed WORD 헤더와 타입 DWORD 기준점 차는 원본의 low DWORD를 보존한다. 판본별 float32 중간 저장 위치·동률·부호 있는 0을 유지한다.
- 빈 decoder는 `(1000,1000,0,0)`과 초기 배율 두 값을 반환한다. 표 밖/원본 assert 입력은 기존 C++ 진단 계약을 따른다. 누락 패턴 프레임은 공통 decoder의 빈 프레임 계약을 유지하며 원본 assert 정상 경로로 검증했다고 주장하지 않는다.

## 함수 대응

| 용도 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 전체 픽셀 getter | `0043cbd0` | `004ed7c0` |
| decoder 생성 / 진행 | `00425c20` / `00425860` | `0041fcf0` / `0041feb0` |
| 공유 전투 패턴 판별 | `00425830` | `0041fcc0` |
| 패턴 셀 / 프레임 검색 | `00425700` / `0049a940` | `0041fbb0` / `004442f0` |
| 타입 / SHP 헤더 조회 | `0049a840` / `00419850` | getter/decoder 내부의 직접 조회 |
| 현재 픽셀 배율 | `0059a92c` / `0059a930` | `00565c44` / `00565c48` |
| 타입 기준점 | `+1dc` / `+1e0` | `+1bc` / `+1c0` |

## 독립 검증

`decomp_canonshape_oracle.py`는 새 내보내기 범위 안의 실제 PE 명령만 실행한다. getter·전체 ctor/Advance·셀/FindFrame·패치 SHP helper를 대체하지 않는다. 모든 assert/범위 밖 실행을 거부하며 쓰기 범위를 출력과 스택으로 제한한다. 정상 cdecl 반환·보존 레지스터·스택·x87 제어/상태를 검사한다. 원본 PE의 패턴 표를 직접 읽고 프레임 코드/물리 헤더·기준점·배율만 분석 입력으로 공급한다.

10.78 **3,150개**, CD **1,400개**, 별도 10.37 **1,400개**, 합계 **5,950개** 독립 입력에서 두 x87 정밀도의 여섯 출력 float 비트가 일치했다. 유효 칸은 각각 **12,510 / 5,560 / 5,560개**다. 모든 정상 패턴/방향·정방향/역순/중복 코드·별칭·빈 비패턴/명시 프레임을 포함한다. 음수/소수/큰/0 배율, signed WORD 경계값, DWORD 기준점 넘침과 동률도 대조한다. 기대 범위는 Python/C++ 계산으로 만들지 않고 실제 명령 관찰에서만 저장한다.

`cpp_canonshape_smoke.py`는 두 판본의 TYPE/SHP를 독립 파싱하고 원본 숫자 변환으로 기준점을 구한다. C++의 `--inspect-canon-shapes` 콘솔 출력과 원본 전체 getter의 여섯 비트를 비교한다. 특수 타입의 모든 패턴/네 짝수 방향과 나머지 자산의 기본 프레임을 확인한다. SHP 주소 표만 분석용으로 재배치하고 실제 코드/물리 헤더/순서를 보존한다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name canonshape
python -X utf8 tools/decomp_canonshape_oracle.py
python -X utf8 tools/decomp_canonshape_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_canonshape_smoke.py
```

최종 빌드/전체 테스트·실제 자산 대조 수치와 로그는 [LEFT_JOBS.md](../../LEFT_JOBS.md) 최신 완료 절에 기록한다. 근거 SHA는 `cpppj/recovery-canonshape-evidence.json`, 원본 관찰은 `cpppj/tests/fixtures/canonshape-x86.tsv`, C++ 대조는 `CanonPixelShapeTests.cpp`다.

## 후속 범위

이 단계는 픽셀 getter와 실제 자산 자료 연결을 완료한다. 일반 타입의 전체 MayPlace/지형·관계 연결, 일반 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사, 비표면 raw GUI·건설·경제·전투·승패는 후속이다. 자산 로더 전체 기계어/게임 플레이/GUI/OS 호출 검증으로 확대해서 해석하지 않는다. 장시간 mutations 전수·최대 지도/공간·창/픽셀 회귀는 기존 인계를 유지한다.
