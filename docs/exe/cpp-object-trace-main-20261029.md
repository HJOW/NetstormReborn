# 자산 Main·Task 진단과 원본 CRT 서식 복원 — 단위556

2026-10-29 / HJOW-Athlon. 작업 전 AGENTS.md와 인수인계를 다시 확인했다. 마지막 완료된 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 현재 PC와 같다. 디컴파일 산출물·보호 원본을 변경하지 않았다. 이전 회차554/555의 두 로컬 커밋은 진척으로 확인했다.

## 복원 범위

`RawObjectTraceMain`은 원본10.78의 Main `004b0fb0`, Dude Task `00450d60`과 실제 가상+0x70 분배다. 생성자 표의83개 자산 VT를 실제 PE에서 읽어81개 파생+base의82개가 Main, Dude `00508cc8` 하나가 Task임을 확인했다. 네 내장Form을 더한87개 표의 모든 분배를 검사했다. 자산의 부모/이전 가상함수는 모두 `004ad000/004ad010`이다. 두 짧은 몸체는 Ghidra 함수 목록에 별도 분리되지 않아 원본9/5BYTE를 그대로 관측기에 허용했다.

Main은555의 Base 로그가 복귀한 뒤 현재 contained/부모·이전, HP/다이스 관문, graph BYTE, 덤프 모드의 frame 생략, 섬/다리 sentinel127, owner/region/quantity, signed highlight/z, +0x28의8개 상태 비트를 읽는다. 부모 표현은 이전 가상+4→부모 가상+0 호출 순서다. TYPE와 공통 free/dead BYTE를 Main의 별도 상태 BYTE와 혼동하지 않는다. Task는 Main 로그 복귀 뒤 현재+0xc WORD를5101..5107로 제한하고 실제 문자열 표를 읽는다. 마지막 포인터 `0051a688`의 문자열은 정상 영문 이름이 아닌 `87 59 4e` 세BYTE이며 임의의 Unknown 문자열로 바꾸지 않았다.

실제 가상 분배는 생성자 VT 지원 표를 재사용하며 미지 VT를 Base나 임의 TYPE로 대신 처리하지 않는다. 554 재귀→같은 실제 풀→현재 가상 분배→Base/파생 로그의 합성 연결 검사도 추가했다. **현재 Client/Session의 선택 진단·로그 파일·Ctrl Shift F7은 아직 연결하지 않았다.** 게임 실행 파일은552 그대로다.

## 원본 CRT에서 찾은 실제 차이와 수정

초기 관측기는 sprintf를 Python 서식으로 대체했고 제품은 최신 CRT `%f`를 사용했다. 이 범위로는 원본의 소수점 반올림까지 증명할 수 없어 별도 원문 CRT 실행기를 만들었다. 첫 직접 실행은 CRT 부동소수 dispatch 미초기화로 실패했다. 원본 시작 `__fpmath`가 먼저 부르는 `004e3de3`의 실제 설치 몸체만 실행하고, OS 조회를 하는 후속 초기화는 호출하지 않았다. 각 호출의 x87 환경은 기존 관측기가 명시 설치한다.

`decomp_trace_crt.py`는 원본 `sprintf 004e58e9`, `__cfltcvt`, `__fltout2`, `$I10_OUTPUT`, `__fptostr`, `__cftof2`와 하위 CRT를 실제 기계어로 실행한다. 시작 설치의 `00543b2c..00543b40` 여섯DWORD만 추가 쓰기로 허용하며 CRT/게임 코드는 수정하지 않는다. 외부 OS/API·파일·게임을 실행하지 않는다. 로그의 cdecl 인자를 별도 원문 CRT 엔진으로 옮겨 Main 함수의 현재 스택/메모리를 건드리지 않는다.

실제 원문은 `0.015625 → 0.01563`, `-0.015625 → -0.01563`으로5이상 올림을 하고, 음의0은 `0.00000`으로 출력한다. 음수 미소값은 `-0.00000`을 유지한다. 큰 값은 원래17자리 가수 뒤0을 채우며 FLT_MAX도 최신 CRT의 전체 정확 정수 출력과 다르다. NaN/INF는 원본의 `1.#INF0`, `1.#QNAN`, `-1.#IND0` 등의 표기를 보존했다. `FormatOriginalTraceCoordinate`는17자리 가수→원래5이상 올림→소수점 배치로 복원했고 최신 CRT의 `%f`를 원본 출력으로 주장하지 않는다.

최종 원문 CRT fixture를 적용하자 기존 최신 CRT 제품은2단언 실패했다(`test-main-original-crt-before.log`). 제품 서식을 위 방식으로 고친 뒤 모두 통과했다. 기존 원본 fixture/기대값을 제품에 맞춰 약화하지 않았다. 이 단위의 초기 Python 대체 관측은 최종 수락에서 제외한다.

## 증거와 한계

- Main/Task **373고유 입력×두 정밀도=746관측**, C++의 전체 로그 바이트와 별도로 기록한 float32→double 비트/signed 정수 인자가 모두 일치했다. Main 상태256조합, TYPE/다이스/HP/graph/island 관문, 덤프 모드0/1/2, 작업 상하한, 로그 콜백 중 현재 필드/모드/작업 변경을 포함한다.
- 좌표 서식은 **887고유 float32×두 정밀도=1774관측**이다. ±0, 정상/비정규 최소값과 큰 값, 반올림 동률, NaN/INF, 여러 지수/가수 경계와 고정seed768개를 포함하며 원본 CRT 출력 바이트가 모두 일치했다. 두 종류 합계1260입력/2520관측이다.87개 VT는 이 수에 기계어 실행 사례로 합산하지 않는다.
- 최종 Release 검사target 경고/오류0, ObjectTrace8+ProcessSummary2+Kernel5+Contained15의 **고유집중30개/실패0**, 등록3408이다. 실제 게임/원본/네트워크 실행은0이다. 전체 C++/CTest와 자연완주·Win11 실기를 확인한 것으로 세지 않는다.
- 최초 합성 재귀 검사 실패2단언은 입력 슬롯의 초기free 상태/링크를 남긴 탓이었다. 세 슬롯을 명시 live raw 입력으로 초기화해 수정했다. v2 빌드의 `std::fill(int0→BYTE)` 경고2건은 BYTE0으로 고쳤다. 제품 논리 실패와 검사 입력/컴파일 경고를 구별한다.
- 보호1820파일과 기존fixture715개(합계2535)는 전후 바이트 동일하며 게임552도 동일하다. 보고서는 `cpppj/object-trace-main-20261029.json`, 원시는 `extracted/cpp-object-trace-main-556-20261029/`에 있다.

원시 `before.json`, `native.json`, `build-v1..v4.log`, `test-main-v1.log`, `test-main-v3.log`, `test-main-original-crt-before.log`, `test-main-v4.log`, 네 `test-*-final.log`와 임시 원문 CRT 관측기를 보존했다. 원본 CRT 초기화/숫자 출력 증거를 UI 표시/전체게임 동일성으로 확대하지 않는다.

다음은 현재 전역을 소유하는 실제 로그/들여쓰기 장치, 객체 덤프 `004d6020`의 경로/열기/닫기/후처리와 실패 순서, 실제 Kernel/ThunderProcess Form을 연결하고 Ctrl Shift F7을 구현하는 것이다. 전체 목표는 진행 중이다. 기존 보류·공중3종 정적 제한·MP 신규 검사 금지와 원격push 금지를 유지한다.
