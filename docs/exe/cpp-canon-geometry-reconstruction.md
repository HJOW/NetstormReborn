# 일반 자산과 패턴 배치의 모양 순회·finder 사각형 복원

2026-10-09, **마지막 디컴파일 수행 PC: HJOW-Athlon**. AGENTS.md·두 LEFT_JOBS와 관련 문서를 확인했고 기존 디컴파일 호스트와 일치한다. 새 `canongeometry` 목록 **12/11/11개**를 같은 PC에서 읽기 전용으로 내보냈다. 원본 게임·복사본·클론 창 실행, 보호 파일/AGENTS.md/dotnetpj 변경, 커밋/푸시는 없다.

## 구현한 범위

`RawCanonPlacementGeometry`는 `CanonTypeQuery`의 **타입 번호와 패턴 번호/프레임 인자를 구별**하여 decoder를 구성한다. 일반 타입의 기본/명시 프레임, 영역 68개·다리 26개·섬 2개·받침 1개·공유 전투 패턴의 전체 유효 칸을 지원한다. 최초 미리보기 정수 범위와 각 칸의 finder 정수 사각형을 계산한다.

`RawPriestPlacementGeometry`는 기존 사제/비패턴 입력 계약을 유지하며 범위 계산과 순회를 공통 모듈에 위임한다. 사제 호출의 첫 인자는 이전처럼 타입 번호이고 명시 프레임을 켜지 않는다. 기존 미리보기·후보 충돌·지형 배열·나선 생성 통합도 같은 순회를 사용한다.

공통 순회는 **decoder 생성/첫 프레임 선택 → 지역 초기화 → 칸 준비 → finder/후보 검사 → 칸 지형 종료 → Advance → 최종 지역 판정**이다. finder/후보 또는 칸 종료가 거부하면 후속 칸과 최종 지역 판정을 생략한다. 빈 decoder에는 지역 초기화와 최종 판정만 호출한다. 하위 지역/후보 정책은 명시된 필수 경계이며 임의로 허용하지 않는다.

모든 칸은 원래 소수 좌표와 절삭한 float 지형 좌표를 함께 제공한다. 타입 발자국은 칸마다 현재 값을 다시 읽는다. 시작 후 defaultFrame 변경은 이미 고른 첫 프레임을 바꾸지 않는다. 요청 인자는 값으로 보존한다. 순수 모양 계산은 사제 이외 타입의 genus를 제한하지 않는다.

## 원본 산술과 계약

| 동작 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| decoder 생성 / 진행 | `00425c20` / `00425860` | `0041fcf0` / `0041feb0` |
| 최초 정수 범위 | `00425b90` | `004202e0` |
| 모양 좌표 snap | `0041d770` | `00440a50` |
| finder 인자 계산 구간 | `0049b8c0`→`0049b95a` | `004455ab`→`00445641` |
| 실제 finder 진입 경계 | `004b16d0` | `004eae20` |
| CRT 절삭 | `004e49c0` | `004f161c` |

미리보기 범위는 첫 현재 좌표와 decoder의 전체 순회 개수/발자국으로 계산한다. finder 사각형은 이 범위와 다르다.

```text
snappedX = float(trunc(x)), snappedY = float(trunc(y))
left = trunc(wide(snappedX) - (footX - 1))
top = trunc(wide(snappedY) - (footY - 1))
right = trunc(wide(x) + float(0.99999))
bottom = trunc(wide(y) + float(0.99999))
```

원본은 절삭 좌표를 별도 스택 복사본에 저장하고 decoder의 원래 좌표를 유지한다. 오른쪽/아래 계산에서 덧셈 결과를 먼저 float로 반올림하지 않는다. 지역/후보 경계가 발자국을 바꾸면 다음 칸 계산에 반영한다.

실제 자산 **dude는 발자국 0×0**이다. 일반 모양 계산은 0 발자국도 위 산술로 처리하며 별도 원본 관찰로 검증했다. 사제 어댑터는 기존 양수 발자국 계약을 유지한다. 음수 발자국/정수 변환 범위 밖/비유한 좌표는 C++ 진단 계약으로 구별한다. 전체 배치의 강제 허용/유형별 정책과 이 순수 사각형 산술을 혼동하지 않는다.

비패턴 기본 프레임은 원본처럼 프레임 개수로 검사하지 않는다. 실제 MayPlace는 방향 코드 조회를 요구하지 않으므로 선택된 메타 코드가 없을 때 정보용 `side`는 0을 제공한다. SHP/픽셀 getter의 물리 헤더 및 패치 frameCheck 진단과는 별개다. 정상 패턴 자료의 프레임/방향은 실제 코드와 일치한다.

## 독립 원본 대조

`decomp_canongeometry_oracle.py`는 실제 decoder 생성/전체 진행/Bounds·패턴 셀/검색·방향 조회를 정상 반환까지 실행한다. 각 유효 칸의 좌표/현재 타입 발자국을 **MayPlace 중간 구간의 지역 입력으로 공급**하고 snap/CRT/finder 인자 계산을 실행한다. finder 몸체 진입 전에 인자를 관찰해 종료한다. 전체 MayPlace·원본 finder 몸체·후보별 지형/관계/OS/게임을 실행했다고 해석하지 않는다.

10.78 **3,159개**, CD **1,404개**, 별도 10.37 **1,404개**, 합계 **5,967개** 독립 입력의 두 x87 정밀도 관찰이 일치했다. 모든 정상 패턴/방향·정방향/역순/중복 코드·별칭·빈 비패턴/명시 모드·소수/음수/지도 끝 좌표와 칸 사이 발자국 변경을 포함한다. 0×0 및 축 하나만 0인 발자국도 별도로 교차했다. 모든 기대 범위/좌표/칸 순서는 실제 명령 관찰에서만 저장한다.

ABI/정상 반환/스택/보존 레지스터/x87·허용 코드/쓰기·assert/OS 0회/SHA를 검사한다. 중간 구간은 전체 함수 반환/보존 레지스터 검사가 아니라 finder 경계와 x87/스택 검사임을 구별한다. C++의 필수 경계 누락·준비/후처리/거부 순서와 실제 `RawSquidFinder`/해시/슬롯을 이용한 다중 칸 거부도 확인한다.

`cpp_canongeometry_smoke.py`는 모든 실제 TYPE 자료를 독립 파싱하여 코드/defaultFrame/발자국을 원본 계산에 공급한다. `--inspect-canon-geometry` 콘솔 출력의 최초 범위와 모든 칸의 원래/절삭 좌표·프레임/라벨/방향·finder 인자를 비교한다. 10.78 **116개 자산·512개 조합·2,132개 유효 칸**, CD **101개 자산·497개 조합·2,117개 유효 칸**이 일치한다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name canongeometry
python -X utf8 tools/decomp_canongeometry_oracle.py
python -X utf8 tools/decomp_canongeometry_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_canongeometry_smoke.py
```

최종 Release 경고/오류 **0**, CTest 내부 **372개·실패 0**(114.35초), 감사 **53종 모두 통과**했다. 누적 독립 fixture는 **269,113개**다. 기존 사제 배치/지형/생성 통합과 두 판본 실제 자산 대조도 통과했다.

근거 SHA는 `cpppj/recovery-canongeometry-evidence.json`, 관찰은 `cpppj/tests/fixtures/canongeometry-x86.tsv`, C++ 재생/순서/실제 finder 검사는 `CanonPlacementGeometryTests.cpp`다. 최종 검사 수치/로그는 [LEFT_JOBS.md](../../LEFT_JOBS.md) 최신 완료 절에 기록한다.

## 후속

이 단계는 일반/패턴의 모양 순회와 finder 인자 계산 및 기존 사제 경로의 공통 연결을 완료한다. 일반 타입의 배치 접두·미리보기/충돌 정책·지형/지역·관계 판정, 일반 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사와 비표면 raw GUI·건설·경제·전투·승패는 후속이다. 실제 미션 완주나 전체 MayPlace의 기계어 대조 완료로 확대해 해석하지 않는다. 장시간 mutations 전수·최대 지도/공간·창/픽셀 회귀 인계는 유지한다.
