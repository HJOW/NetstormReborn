# cpppj ButtonGump 원본 기계어 대조·연결 검증

> 같은 날 후속에서 [돌 버튼 생성·배경·부착·수명 및 기본 도움말의 정적 구현](cpp-stone-button-gump-reconstruction.md)을 추가했다. 아래 실행 검증 수치는 이 문서의 앞 단계 코드에 대한 결과이며 새 생성/도움말 코드의 검증 결과가 아니다.

작업 PC: **VM-W11-CODEX**(`10.0.0.17`), 기록일: **2026-10-12**. AGENTS.md·두 LEFT_JOBS를 읽고 현재 PC가 마지막 디컴파일 PC와 같은 것을 확인했다. 사용자가 이 PC의 창 실행·기존 게임 실행 테스트를 허용했다. [지난 정적 구현](cpp-button-gump-reconstruction.md)의 남은 동작 검증을 진행했다.

`buttongump`의 읽기 전용 내보내기를 **25/22/22개**로 늘렸다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-12.** 새 **독립 원본 742개**, cpppj 누적 **520,756개**, 두 x87 최상위 정상 반환 **1,640회**를 관찰했다. 새 C++ 검사 **8개**와 전체 **827개**가 통과했고 기존 클론 메뉴 GUI **54개 상태**도 통과했다.

## 독립 관찰 범위

`tools/decomp_buttongump_oracle.py`는 세 실제 PE를 각기 격리 Unicorn 메모리에 읽고 내보낸 기계어 몸체를 정상 반환까지 실행한다. C++ 구현이나 예상 상태/호출 순서를 계산하는 함수를 사용하지 않는다.

| 판본 | 독립 입력 | x87 최상위 반환 |
| --- | --- | --- |
| 10.78 패치판 | 252 | 556 |
| CD 10.72 | 245 | 542 |
| 추가 10.37 | 245 | 542 |
| 합계 | **742** | **1,640** |

전체 버튼 기본 생성, 기본/돌 버튼 값·누름 setter, 활성/점멸/pulse setter, 입력, 등록표 통지, idle, 화면→로컬 좌표를 실행했다. 패치판 포함 helper와 CD의 인라인 포함 분기도 실제 명령을 사용한다. CD 입력은 실제 진입 `00440ce0`, 패치 입력 위임 꼬리는 별도 범위 `00424c69`를 포함한다.

버튼이 호출하는 **Gump 기본 생성·첫 빈 등록 칸·serial 갱신, 첫 캡처, idle 표 등록/해제, 부모 전파, 최상위 팝업 순회·실제 Kill 및 dialog 해제, CRT 절삭, 예약 만료 helper**도 실제 몸체로 실행한다. 첫 전수 생성에서 허용 밖으로 검출한 패치 예약 만료 helper `004180a0`를 분석/내보내기 목록에 추가했다. 실패 기록은 보존했으며 원본 몸체를 응답으로 대체하여 우회하지 않았다.

명시 응답 경계는 다음과 같다.

- **벽 시계:** 완성 입력 double 비트를 반환한다. 원본 벽 시계 전체의 숫자 계산은 기존 별도 시계 근거이며 이번 관찰은 버튼의 호출 순서·예약 쓰기를 본다.
- **왼쪽 버튼 폴링:** 완성 입력의 사건/좌표 세 DWORD를 공급한다. 전달 좌표와 다른 폴링 좌표도 입력에 포함한다.
- **가상 redraw/부모 이벤트/Push:** 실제 인자와 호출 직전 전체 상태를 기록하고 명시한 raw 응답 쓰기/반환만 적용한다. 파생 Form 및 실제 dirty/Push 처리기의 원본화는 후속이다.
- **기본 Gump 도움말 처리기:** 인자와 반환 DWORD를 관찰한다. 도움말 창 생성 몸체는 별도 복원 대상이다.
- **소리:** 정확한 `button.wav`와 다섯 0 인자를 검사한다. 장치를 열지 않는다.
- **월드 투영:** 명시한 정수점을 반환한다. 이후 로컬 위치 CRT 절삭·DWORD 덧셈·float 뺄셈은 원본 명령으로 실행한다.

## 대조한 상태와 조건

각 행은 전체 여섯 **0x170바이트 입력/출력**, 등록표 1,000칸·판본별 idle 500/80칸·overlay 100칸·삭제 큐 1,000칸·dialog 16칸과 일곱 스칼라를 보존한다. 0x170은 기본/돌 버튼 입력과 미사용 꼬리까지 담는 관찰 버퍼 폭이며 모든 원본 Gump의 할당 크기라는 뜻이 아니다. 콜백 직전에도 같은 전체 상태를 기록한다.

주요 입력은 다음을 포함한다.

- 생성 전 미사용 바이트, 등록 칸/serial DWORD 감김, 색 BYTE의 부호 확장, 음수/큰 정수 크기.
- 기본/돌 버튼 두 상태 OR, 같은 값 쓰기의 redraw, 부모 redraw가 상태 또는 배경 포인터를 변경한 뒤의 배경 눌림 전환.
- 일반·토글·즉시·비활성·팝업 유지와 상위 플래그, 눌림/안쪽·바깥쪽 뗌, 값 명령의 serial 일치/불일치와 원래 사건 분기로 계속 진행하는 동작.
- 캡처를 유지한 밖/복귀 통지·후속 뗌, 폴링 release, 비활성 통지, 반복 시각 만료/미만료·pulse 해제, 시계/부모 콜백 뒤 현재 예약/플래그 재조회.
- 팝업의 visible/void/dead 차이, 실제 Kill의 큐/캡처 변경과 Push 콜백이 next를 바꾸어도 미리 저장한 next로 진행하는 동작.
- 루트 위치 생략, 각 단계 float 저장, 월드 부착 부모 기준점/자식 로컬 생략과 정수 덧셈 감김, 경계의 음의 0·NaN 제외.

`0x027f`와 `0x037f` 두 x87 정밀도로 같은 입력을 각각 실행했으며 **이번 입력의 관찰 차이는 0개**였다. C++ 재생은 기본 53비트 결과를 사용하고, 대안 결과 차이가 생기면 별도 JSON에 전체 관찰을 보존하도록 도구를 만들었다. 모든 가능한 double/NaN·손상 포인터·실제 SEH 예외·임의 정지 시계의 동일성을 주장하지 않는다.

스택 purge·보존 레지스터·SEH·x87 스택/제어어, 실제 실행/쓰기 범위, 노드 전후 보호 바이트·읽기 전용 가상 표, 완성 입력/행 순서·SHA·진입/반환·경계 집계를 감사했다. 게임 프로세스/OS/assert 호출은 0이다.

## C++ 연결과 실행 결과

새 `ButtonGumpTests.cpp`의 세 판본 재생은 저장 관찰과 **전체 상태 바이트·표·반환·콜백 시점/인자/순서**가 일치했다. 별도 연결 검사는 실제 `BindButtonGump`를 사용한다.

1. 같은 저장소의 트리 이벤트 전달→실제 캡처→실제 프레임 통지의 밖/복귀→현재 출력 입력→후속 뗌/캡처 해제→다음 idle 제외.
2. 현재 pulse 해제가 idle 표를 지워도 이미 진행 중인 idle 후반에서 반복 통지가 실행되는 순서.
3. 실제 팝업 Kill→같은 지연 삭제 큐·캡처 변경.
4. 루트/월드 부모의 로컬 좌표 분기와 잘못된 토큰/순환·콜백 예외 후 쓰기 보존.

**Release 빌드 경고/오류 0**, 새 집중 검사 **8개·실패 0**, 전체 CTest 내부 **827개·실패 0**(실행 90.41초, 전체 90.47초)이다. 이번 대조에서 지난 C++ 구현과 원본의 불일치는 발견되지 않아 게임 모듈의 처리 몸체는 수정하지 않았다.

`tools/cpp_menu_smoke.py`로 실제 클론 창을 실행해 **메뉴/브리핑/미션 진입·복귀 54개 상태**, 800×600/640×480/1024×768 전환·전체화면 표식 처리·옵션 저장 후 재실행을 확인했다. 메뉴 배경 **774,432화소**와 두 판본 GIF 색 번호 **각 831,008화소**를 대조했다. 메뉴 단계의 기존 `GumpControlInput` 어댑터 회귀이며 **새 raw ButtonGump가 Client 자동 UI에 연결된 검사는 아니다.**

기존 스모크가 원본/설정 존재 상태를 전수 기록하고 설정을 복구했으며 패치/CD 보호 파일 불변을 확인했다. AGENTS.md·두 원본 바이너리 SHA도 이전 값과 같고 보호 경로의 Git diff는 없다. dotnetpj와 그 인계는 변경하지 않았다. 원본 게임 전체 프로세스는 실행하지 않았으며 커밋/푸시하지 않았다.

## 자료와 재현

- `cpppj/tests/fixtures/buttongump-x86.tsv`: 기본 53비트 전수 관찰, LF/UTF-8.
- `cpppj/tests/fixtures/buttongump-x87-64.json`: 대안 모드 차이 자료(이번 결과는 빈 배열).
- `cpppj/recovery-buttongump-evidence.json`: 원본/내보내기/도구/fixture SHA와 호스트·집계.
- `extracted/buttongump/{originals,originalCD,original1037}/`: 현재 25/22/22개 내보내기.
- `extracted/buttongump-{oracle,oracle-final,validation-build,focused,ctest,menu-smoke}.log`, `extracted/cpp-menu-smoke/report.json`·BMP: 실제 실행 결과. 로그는 UTF-8로 정리했다.

함수 목록 JSON의 LF를 정리한 뒤 최종 관찰을 다시 생성했다. 입력 수를 누적에 중복 합산하지 않았으며 fixture SHA가 첫 생성과 같은 것을 확인했다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name buttongump
python -X utf8 tools/decomp_buttongump_oracle.py
python -X utf8 tools/decomp_buttongump_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 2
cpppj/build/bin/Release/netstorm_tests.exe --filter ButtonGump
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_menu_smoke.py
```

## 다음 작업

돌 버튼 전체 생성/텍스트 크기/배경 할당·부착/파생 소멸자, 기본 Gump 도움말 처리기를 이어 복원한다. 그 뒤 실제 Form/메뉴·OS 큐/폴링·overlay·Client 자동 UI 프레임을 연결한다. 삭제 Cleanup/메모리 해제·overlay/dialog의 모든 비영 조합은 기존 별도 Gump 수명 근거와 이번 버튼 관찰을 구분해 본다. 게임 건설/경제/전투/AI/승패가 남아 **미션 완주는 아직 불가능하다.**
