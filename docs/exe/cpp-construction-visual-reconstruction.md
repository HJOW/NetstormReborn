# 건설 표시 생성과 초기 입자 복원

> 후속 [건설 표시 프레임·입자 확장·Draw 복원](cpp-construction-frame-reconstruction.md)에서 RunFrame/입자/시각/팔레트/사각형/Draw 호출을 완료했다. 이 문서는 앞 생성 단계의 검증 이력이다. 실제 표시 삭제/등록·SHP 캡처/장치 연결은 계속 후속이다.

2026-10-11, **HJOW-Athlon**. AGENTS.md와 두 LEFT_JOBS 문서를 읽고 현재 PC가 마지막 디컴파일 PC와 같음을 확인했다. `constructionvisual` 목록 **13/13/13개**를 세 판본의 Ghidra 프로젝트에서 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본 게임·analyzeManager·클론 창은 실행하지 않았다.

`RawConstructionVisual`은 즉시/일반 건설 표시의 생성, 공유 팔레트 요청, 캡처 버퍼 준비, 좌표 동기화와 초기 입자 선택을 복원한다. `MakeConstructionVisualHooks`는 실제 `RawConstructionProcess::Start`의 표시 경계를 연결하고 생성한 초기 상태의 소유권을 호출자의 저장소로 넘긴다. **프레임마다 입자를 활성화/이동하는 몸체와 실제 화면 출력은 아직 복원하지 않았다. 게임 내 건설/미션 완주도 아직 불가능하다.**

## 원본 몸체

| 역할 | 10.78 패치 | 10.72 CD / 추가 판본 |
|---|---|---|
| 즉시 건설 표시 생성 | `00423840` | `004eab60` |
| 일반 건설 표시 생성 | `00423900` | `004eac30` |
| 공통 생성자 | `004232c0` | `004ea140` |
| 캡처/버퍼 준비 | `00422d90` | `004ea410` (`004ea6e0` 래퍼) |
| 좌표 동기화/초기 입자 호출 | `00423770` | `004ea630` |
| 초기 입자 선택/섞기 | `00423560` | `004ea4a0` / 추가 helper `004ea700` |
| 부모 유효성 검사 | `004afd00` / `004ac040` / `004ad730` | `004ace80` (항상 참) |
| 실제 실수/정수 난수 | `00455930` / `004558c0` | `0048cc50` / `0048cbd0` |
| 실제 CRT ftol / stack probe | `004e49c0` / `004e4a40` | `004f161c` / `004f1bd0` |

패치의 생성 래퍼는 부모의 owner, vtable, 타입, free/dead/void와 종속의 타입/free/void를 검사한다. CD의 해당 helper는 항상 참이다. C++는 원본 assert/null 접근·풀 밖 접근·순환 종속을 예외로 거부한다. 정상 입력에서 판본의 검사 차이를 유지한다.

## 생성 순서와 공유 팔레트

객체 0x88바이트 확보 → RenderProcess 부착 → 공통 생성자 필드 초기화 → 현재 공유 팔레트 확인 → 파생 수명 설정 → 진입 인자의 부모 이미지 캡처 → 출력 버퍼 확보/투명 채우기 → 현재 부모 좌표 동기화 → 입자 선택 순서다. +0x30/+0x38의 프레임 시각/누적 시각은 0으로 초기화하며, 프레임 간격 float(+0x40/+0x44)은 후속 RunFrame의 시각 helper가 설정한다. 부착 경계가 캐시를 채우면 팔레트 생성은 건너뛴다. 좌표/배율은 캡처 경계 이후의 현재 자료를 다시 읽는다.

공유 팔레트가 없으면 8바이트 색 표 생성, 두 색 띠 생성, 두 띠 합치기를 요청한다. 첫 띠는 **RGB (0,0,1) → (1,1,1), 8색**, 둘째는 **(1,1,1) → (128/255,128/255,128/255), 3색**이며 공통 인자는 double **2.4**다. 합치기의 세 번째 띠는 null이다. 실제 호출 스택의 여섯 double·색수 DWORD·마지막 double을 세 PE에서 확인했다. RGB를 단일 회색 값으로 축약하지 않는다. 팔레트 생성 내부와 최종 색 표 내용은 이번 몸체의 응답 경계다.

즉시 표시는 수명 **2~9**, 일반 표시는 **float32 `0x3fa66666`~2**다. 부모 zOrder BYTE는 signed char로 확장한다. 부모 x/y float에 현재 배율을 곱한 결과를 float로 저장하고 표시 사각형의 left/right와 top/bottom을 각각 같은 값으로 맞춘다. 후속 RunFrame은 이 사각형을 갱신할 때 이전/새 영역의 재표시를 요청한다. 화면 hotspot은 캡처 경계가 공급한다. 동기화가 CRT로 변환한 두 좌표를 초기 입자 함수에 전달하지만 그 인자는 실제 함수가 사용하지 않는다.

## 초기 입자

캡처의 `0xff`만 투명이다. 색 0도 불투명으로 센다. 불투명 수를 세고 **max(500, 불투명 수/6)**칸을 확보한다. 기존 입자 저장소가 있으면 먼저 반납한다. 원본 입자는 12바이트이며 active BYTE·원래 색 BYTE·0 패딩 WORD·수명 float·출력 픽셀 포인터로 구성된다. C++는 포인터를 출력 버퍼 내 픽셀 번호로 표현하고 미사용 칸을 모두 0으로 둔다.

이미지의 마지막 픽셀을 출력 버퍼에 먼저 복사하고 원본 버퍼에서는 투명으로 바꾼다. 불투명 수는 이 제거 **이전**에 센다. 이후 `(width-1)/2` 열에서 시작해 중앙, 오른쪽, 왼쪽을 교차하며 각 열을 방문한다. 열마다 아래에서 위로 탐색하되 `row > height/3`만 검사하고 첫 불투명 픽셀 하나를 선택한다. 선택 픽셀은 원본 버퍼에서 제거하고 누적 selected를 늘린다. selected·active·첫 표시 표식은 Seed 진입 때 지우지 않는다.

각 입자의 수명은 게임의 공유 난수 상태에서 상위 16비트를 소비해 정한다. 끝으로 **width회**, 매번 현재 입자 수를 상한으로 두 난수를 소비하고 입자 전체를 교환한다. 입자가 0개면 교환 난수는 소비하지 않는다. 별도 난수 생성기나 호스트 난수로 대체하지 않는다.

## 건설 시작 연결과 남은 경계

어댑터는 같은 SidPool과 필수 저장 콜백을 요구한다. Start가 즉시/일반 표시를 선택해 Create를 호출하고, 저장 콜백이 초기 상태를 소유한 뒤 건설 프로세스를 부착한다. 캡처 중 현재 시각이 바뀌면 실제 Start의 시작/끝 시각은 바뀐 시각으로 계산된다. 이 호출 순서와 두 번째 생성의 팔레트 재사용을 통합 검사한다.

이번 명시 응답 경계는 객체/버퍼 확보·입자 반납, RenderProcess 등록/부모 부착, 실제 SHP 캡처, 색 띠/공유 표 생성이다. C++ 이미지/입자 저장소는 초기 상태를 보존하지만 클라이언트의 표시 목록에 등록되어 프레임마다 실행되는 기능은 후속이다. RunFrame은 `00422f00` / CD `004ea7e0`, Draw는 `00423200` / CD `004eaa90`, 입자 확장 가상 함수는 `00422ea0` / CD `004ea780`이다. 전체 RunFrame, 입자 활성화/수명/색/이동, 파생 Draw/삭제, 실제 SHP 캡처와 렌더러 연결을 다음 단계에서 복원해야 한다. 타입 준비 소리(+0x90) 자료, 가상 표시/부모 Unpop/재등록, 하위 경로/공간/사제 contained와 건설 GUI도 남아 있다.

## 독립 근거와 검증

`tools/decomp_constructionvisual_oracle.py`는 각 PE **592개**(Create 520·Seed 60·Sync 12), 합계 **1,776개**를 두 x87 정밀도로 실행했다. 주 루트 정상 반환은 **3,552회**다. 앞 누적 494,190개에 더해 **495,966개**가 됐다. 실제 함수 범위/허용 쓰기·ABI/보존 레지스터/x87/SEH·SHA/입력 순서/필드 폭/버퍼 길이/입자 목적지·난수 진입/정상 반환/외부 사건 집계·미사용 입자 0을 감사한다. 원본 assert/게임/OS는 0이다.

새 C++ 검사는 **6개**다. Release 경고/오류 **0**, CTest 내부 **668개·실패 0**(143.97초, 전체 144.01초), 저장 근거 감사 **103종 모두 통과**. 별도 사본의 새 변이 **8개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과): 시작 RGB·selected 초기화·최소 용량·마지막 픽셀 제거·짝수 폭 중앙 열·행 경계·섞기 난수 수·zOrder 부호. 사본 소스는 원래 바이트로 복구했다. 로그 `extracted/constructionvisual-*.log`. AGENTS.md·보호 원본·dotnetpj 변경과 커밋/푸시는 없다.

입력은 즉시/일반·차가운/재사용 캐시·1×1~61×60 및 용량이 가득 차는 1001×3 크기·일곱 투명/색 패턴·세 난수 시드·기존 입자 반납·selected 감김·signed zOrder·0/음수 배율·부착/캡처 중 전역 변경을 포함한다. 입자 선택/좌표/난수는 실제 PE 몸체가 실행한다. C++ 검사는 저장된 원본 관찰을 읽으며 기대 선택을 같은 알고리즘으로 다시 계산하지 않는다. 정상 CTest는 원본 PE·Python·게임을 실행하지 않는다.

관련 파일: `cpppj/src/o/RawConstructionVisual.{h,cpp}`, `cpppj/tests/ConstructionVisualTests.cpp`, `cpppj/tests/fixtures/constructionvisual-x86.tsv`, `cpppj/recovery-constructionvisual-evidence.json`, `tools/decomp_constructionvisual_oracle.py`, `tools/ghidra/constructionvisual-functions.json`.

재검사: `python -X utf8 tools/decomp_constructionvisual_oracle.py --verify`, `cpppj/build/bin/Release/netstorm_tests.exe --filter ConstructionVisual_`, `ctest --test-dir cpppj/build -C Release --output-on-failure`. 내보내기: `powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructionvisual`.
