# 건설 표시 프레임·입자 확장·Draw 복원

2026-10-11, **HJOW-Athlon**. AGENTS.md와 두 LEFT_JOBS 문서를 읽고 현재 PC와 마지막 디컴파일 PC가 같음을 확인했다. `constructionframe` 목록 **11/11/11개**를 세 Ghidra 프로젝트에서 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본 게임·analyzeManager·클론 창·화면 장치는 실행하지 않았다.

`RawConstructionFrame`은 앞 단계 `RawConstructionVisual`이 생성한 상태를 받아 HP별 활성화, 인접 입자 추가, 수명/팔레트 갱신, 만료 픽셀 복원, 이전/새 사각형 무효화와 Draw 요청을 복원한다. 최대 HP·dirty 큐·장치 blit·BaseProcess Kill은 호출자가 연결하는 경계다. **실제 RenderProcess 등록/프레임 호출·SHP 캡처·장치 출력·표시 삭제는 후속이며 게임 내 건설/미션 완주는 아직 불가능하다.**

## 원본 몸체

| 역할 | 10.78 패치 | 10.72 CD / 추가 판본 |
|---|---|---|
| 전체 RunFrame | `00422f00` | `004ea7e0` |
| 가상 인접 확장 | `00422ea0` | `004ea780` |
| 입자 한 개 추가 | `00422e30` | `004ea700` |
| 전체 Draw | `00423200` | `004eaa90` |
| 시각 초기화 / 갱신 | `00418400` / `00418430` | `0045e280` / `0045e2b0` |
| 팔레트 DWORD 조회 | `00487e50` | `0045d340` |
| 표시 사각형 무효화/변경 | `00487ba0` | `0045ce80` |
| 실제 정수 / 실수 난수 | `004558c0` / `00455930` | `0048cbd0` / `0048cc50` |
| 실제 CRT ftol | `004e49c0` | `004f161c` |

명시 경계는 최대 HP `004adcb0`/CD `004aed30`, dirty 큐 `00499310`/CD `00456d10`, 장치 blit `004a2020`/CD `00423920`, BaseProcess Kill `0041bf10`/CD `0048f7b0`이다. 실제 실행기의 호출 규약·this·인자·스택 복구를 검사한다. 내부 난수·시각·팔레트·사각형 계산은 응답 경계로 대체하지 않는다.

## HP 활성화와 확장

CD는 소스 포인터가 0이거나 부모가 0/풀 밖/free이면 아무 효과 없이 반환한다. 패치는 소스/부모 유효성을 assert하고 free 검사를 추가하지 않는다. C++는 손상 저장소와 패치의 잘못된 부모를 예외로 거부한다.

좌표 사각형을 먼저 계산하고 시각을 갱신한 뒤 최대 HP를 조회한다. 목표는 원본의 signed 변환과 CRT 절단에 따라 **HP/(최대 HP−1)×불투명 수**로 계산한다. 최대 HP−1의 DWORD 감김도 유지한다. 패치는 raw +26의 signed DWORD HP를 float로 저장한 **후** 최대 HP 경계를 호출한다. CD는 최대 HP 경계 **후** 현재 signed WORD HP를 읽는다. 경계 중 HP 변경으로 이 차이를 검증한다.

누적 selected가 목표보다 작고, inactive 입자가 남아 있고, total이 capacity보다 작을 때만 하나를 무작위로 고른다. 가상 확장을 먼저 실행한 뒤 고른 입자 active를 켜고 활성 구간 앞쪽으로 교환한다. 새 이웃 때문에 변한 selected/total을 다음 반복에서 다시 읽는다. **가득 찬 저장소는 남은 inactive 입자가 있어도 활성화를 시작하지 않는다.**

원본 가상 인자는 출력 픽셀 포인터다. C++는 같은 출력 버퍼의 픽셀 번호로 정규화한다. 확장 조건은 **width < pixel < lastRow**이고, 이웃은 **오른쪽·왼쪽·위** 순서다. 행의 좌우 경계는 별도로 검사하지 않으므로 선형 인덱스의 행 넘김도 그대로 유지한다. 0xff만 투명이며 이미 선택한 픽셀은 소스에서 제거된다. 아래 방향은 추가하지 않는다.

Append는 용량 안에서 total을 먼저 늘리고 실제 공유 실수 난수로 수명을 정한 뒤 색/목적지를 보존하고 source를 0xff로 바꾸며 selected를 늘린다. CD의 잘못된 픽셀 입력은 **total만 증가한 채 0을 반환**한다. 패치의 범위 밖 읽기는 실행하지 않고 호스트에서 거부한다. 용량이 가득 차면 두 판본 모두 아무 변경 없이 반환한다.

## 시각·만료·팔레트

첫 프레임이면 +0x30의 lastTime을 현재 시각으로, +0x38의 elapsed를 double 0으로 초기화하고 +0x7c를 켠다. 이후 **(현재−이전)×10**을 float delta(+0x40)에 저장한다. scaledDelta(+0x44)는 **float로 저장하기 전** 시간 차이에 전역 rate를 곱한다. elapsed는 **저장한 float delta를 기존 double에 더한 double**이다. 두 판본이 같다. Ghidra가 패치 elapsed를 float 합산으로 표시하지만 실제 `fadd qword`/`fstp qword` 명령과 큰 누적 시각 관찰로 이를 바로잡았다. 이 값의 게임 시간 단위는 아직 장치 프레임 주기와 연결하지 않았다.

활성 입자는 delta만큼 수명을 줄인다. 양수면 팔레트 DWORD의 하위 BYTE를 출력한다. 만료되면 원래 색을 복원하고 active/total을 각각 줄인 뒤 **현재 칸←마지막 활성 입자, 마지막 활성 칸←마지막 전체 입자** 순서로 두 번 복사한다. 현재 칸을 다시 처리하므로 만료 때 반복 인덱스를 증가시키지 않는다. 사용을 마친 칸의 잔여 값도 원본과 비교한다.

inactive 입자도 수명이 줄지만 원래 색을 확정하지 않는다. 만료하면 실제 공유 난수로 0~maximum 수명을 다시 정하고 남은 수명과 delta를 더한다. 패치는 난수+delta+저장한 잔여 수명, CD는 난수+저장한 잔여 수명+delta 순서다. 정수/실수 난수는 앞 생성 단계와 같은 `GameRandom` 상태를 소비한다.

색 조회는 life를 maximum까지만 제한한다. 패치는 count와 1/maximum을 각각 float로 저장한 뒤 비율을 곱하고, CD는 signed count를 곱한 뒤 maximum으로 나눈다. 음수 인덱스는 0이다. 반환값은 DWORD이며 프레임만 하위 BYTE를 사용한다. **life≥maximum이면 count와 같은 인덱스를 읽을 수 있다.** 원본의 count는 실제 읽기 배열 길이와 다르므로 이번 입력은 count 경계 뒤 **알려진 한 DWORD**도 명시한다. C++는 제공한 span을 넘어서는 읽기를 예외로 거부하고 임의의 인접 메모리를 읽지 않는다. CD의 Pentium FDIV 우회 전역은 0인 경로만 관찰했다.

## 사각형·Draw·소유권

RunFrame의 왼쪽/위는 부모 x/y×현재 배율−hotspot이다. 패치 right는 이미 float로 저장한 left+width이고 CD right는 저장 전 left+width다. 두 판본 모두 bottom은 저장 전 top+height다. 이 사각형은 최대 HP/무효화 경계가 부모를 바꾸어도 프레임 시작 때의 값을 사용한다.

이전 사각형을 현재 카메라에 맞춰 왼쪽/위 −1, 오른쪽/아래 +1로 무효화하고, 새 사각형도 같은 순서로 요청한다. 두 요청 사이에 카메라가 바뀌면 두 번째 요청은 바뀐 카메라를 사용한다. 사각형 float 필드는 두 요청 뒤에 갱신한다. 마지막에 현재 부모 abstract 비트를 다시 읽어 꺼졌으면 flags 0의 Kill을 요청한다. 이 요청 자체는 상태 소유권을 해제하지 않는다.

Draw는 **capacity가 0일 때만** 생략한다. total이 0이어도 capacity가 있으면 복구한 출력 버퍼를 제출한다. 부모 좌표와 카메라는 Draw 때 다시 읽는다. x는 float로 저장한 뒤 hotspot을 빼고 y는 저장 전 값에서 hotspot을 빼며 CRT로 signed 정수화한다. 장치 요청은 x/y, 현재 output span, pitch=width, clip=(0,0,width,height)이다. 원본 Draw의 두 스택 인자는 사용하지 않는다. 콜백은 span을 보관하지 말고 호출 중에 읽어야 한다.

`ConstructionFrameFields`가 생성 단계의 `ConstructionVisualFields`를 소유한다. 원본의 136바이트 포인터 객체를 실제 호스트 메모리 주소로 재현하지 않는다. C++ 연결 검사는 두 판본·두 생성 방식에서 실제 Create→첫 Run→Draw→HP 완료 갱신→픽셀 복원→입자 0 상태 Draw→abstract 종료 요청을 순서대로 호출한다. 클라이언트/Kernel의 자동 표시 실행에 등록했다는 의미는 아니다.

## 독립 근거와 검증

`tools/decomp_constructionframe_oracle.py`는 패치 **435개**(Run 202·Expand 64·Append 8·Color 90·Clock 49·Draw 18·Sequence 4), CD/추가 각 **441개**(CD 조기 반환 4·잘못된 Append 2 추가), 합계 **1,317개**를 두 x87 정밀도로 실행했다. 직접 호출한 루트 정상 반환은 **2,994회**다. 앞 누적 495,966개에 더해 **497,283개**가 됐다. 실제 허용 몸체/쓰기·ABI/보존 레지스터/x87/SEH·SHA/입력 순서/필드 폭/전체 용량 입자/두 버퍼/루트 진입·반환/외부 사건 집계를 감사했다. 원본 assert/게임/OS는 0이다.

새 C++ 검사는 **6개**다. Release 경고/오류 **0**, CTest 내부 **674개·실패 0**(145.58초, 전체 145.61초), 저장 근거 감사 **104종 모두 통과**. 별도 사본의 새 변이 **8개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과): HP 폭·누적 시각 float 변환·시간 배율의 반올림·이웃 방향·가득 찬 용량 활성화·만료 입자 복사·색 역수 반올림·total 0 Draw 생략. 사본 소스는 원래 바이트로 복구했다. 로그 `extracted/constructionframe-*.log`. AGENTS.md·보호 원본·dotnetpj 변경과 커밋/푸시는 없다.

입력은 HP 0/양수/음수/WORD 초과, 첫/이전 시각, 순방향/역방향 시간, 큰 double 누적, 활성/미활성/빈/가득 찬 저장소, 투명 후보, 선형 행 경계, 용량 1~3칸 추가, 끝 픽셀, 버전별 조기 반환, HP/좌표/시각/카메라/abstract의 경계 중 변경을 포함한다. 각 판본의 네 연속 시나리오는 다섯 Run/Draw의 **모든 중간 필드·두 버퍼·전체 용량 입자·난수**를 관찰한다. 직접 Color의 DWORD 반환과 Append의 슬롯 반환도 비교한다. 기대 입자/출력은 같은 알고리즘으로 재계산하지 않는다.

정상 CTest는 저장 fixture만 읽으며 PE/Python/게임/OS를 실행하지 않는다. 원본 생성/삭제/등록과 실제 SHP 팔레트/장치의 내용, 임의 손상/NaN/무한값, 모든 x87 오차 입력의 동등성을 증명한 것은 아니다. 후속은 **표시 소멸자와 RenderProcess 부착/목록·클라이언트 자동 Run/Draw·실제 SHP 캡처/팔레트 생성/출력 연결**이다. 타입 준비 소리 자료, 부모 공간·소유 사제 contained/경로, 집합/지연 패킷 분배와 건설 GUI도 남아 있다.

관련 파일: `cpppj/src/o/RawConstructionFrame.{h,cpp}`, `cpppj/tests/ConstructionFrameTests.cpp`, `cpppj/tests/fixtures/constructionframe-x86.tsv`, `cpppj/recovery-constructionframe-evidence.json`, `tools/decomp_constructionframe_oracle.py`, `tools/ghidra/constructionframe-functions.json`.

재검사: `python -X utf8 tools/decomp_constructionframe_oracle.py --verify`, `cpppj/build/bin/Release/netstorm_tests.exe --filter ConstructionFrame_`, `ctest --test-dir cpppj/build -C Release --output-on-failure`. 내보내기: `powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructionframe`.
