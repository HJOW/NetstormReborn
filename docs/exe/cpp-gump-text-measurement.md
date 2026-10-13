# cpppj 기본 Gump 글자 측정과 돌 버튼 글꼴 연결

작업 PC: **VM-W11-CODEX**(`10.0.0.17`), 기록일: **2026-10-12**. AGENTS.md·두 LEFT_JOBS를 확인하고 현재 PC와 마지막 디컴파일 PC가 같은 것을 확인했다. [돌 버튼 정적 복원](cpp-stone-button-gump-reconstruction.md)의 실제 폰트 측정 경계를 진행했다.

`gumptext-functions.json`으로 판본마다 두 함수를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-12.** 기존 전체 정밀 디컴파일 및 stonebuttongump 내보내기는 변경하지 않았다.

## 원본 대응과 측정 규칙

| 처리 | 10.78 | CD 10.72·추가 10.37 |
| --- | --- | --- |
| Screen 글자 측정 | `004a3df0` | `00425f90` |
| 기본 Gump 문맥 선택·위임 | `00465020` | `00493ea0` |
| 별도 슬롯 높이 지정의 정적 근거 | `004a39f0` | `00425d10` |

`GumpText`는 기존 `FontStore`가 읽은 `.chfnt`의 **첫 폭 표**를 사용한다. ABC A나 그리기 전진 폭을 측정 폭으로 사용하지 않는다. 높이는 캐시의 `tmHeight` 대신 원본이 별도 전역에 지정한 **14/13/0/20/48/14/12**를 사용한다. 이 차이를 확인하기 위해 단위 검사 캐시 높이는 31~37로 공급했다.

정상 스타일은 해당 슬롯의 표를 선택한다. 패치판은 normal/italic/bold/strikeout/underline, CD·10.37은 normal/italic/bold이다. 범위 밖 스타일의 **폭은 normal 슬롯 0**, **높이는 요청 슬롯**이다. 생성되지 않은 슬롯/스타일 표는 폭 0으로 처리한다. CD의 슬롯 6 bold도 비어 있다. 이 상태는 원본 초기 글꼴 구성이 유지되는 저장소를 대상으로 한다. 런타임에 raw 캐시 표를 임의로 수정하거나 초기화를 완료하지 않은 상태는 이번 FontStore 어댑터의 범위가 아니다.

길이 -1은 NUL 앞까지 측정한다. 명시한 양수 길이는 중간 NUL과 128~255 바이트도 폭 표에 포함한다. -1 이외의 음수 길이는 0글자다. 합산은 하위 DWORD를 보존하고 C++ signed overflow를 만들지 않는다. 그림자 bit 2가 있으면 signed X/Y 여백을 더하며 외곽선 bit 4는 크기를 바꾸지 않는다.

출력 폭/높이가 같은 변수일 때 판본 차이를 유지했다. 패치판은 폭→높이를 쓴 뒤 현재 출력값에 X/Y를 순서대로 더한다. CD·10.37은 그림자 덧셈에 저장해 둔 폭 합계/높이를 사용한다. 정상 별도 출력에서는 같은 결과지만 alias 출력에서는 차이가 난다.

문자열은 호스트가 소유한 바이트 범위로 받는다. 범위 안에 NUL이 없는 -1 입력, 부족한 명시 길이, 범위 밖 슬롯은 호스트 예외로 거부한다. 이 안전 계약을 원본의 범위 밖 읽기/임의 전역 접근과 같다고 판정하지 않는다. 입력 폭 표·문맥은 출력 두 참조와 겹치지 않아야 한다.

## 돌 버튼 연결

`BindStoneButtonText`는 기존 `StoneButtonGumpHooks::measure`를 실제 `GumpText`에 연결한다. `StoneButtonGump::FitToText`와 `SetLabel`이 현재 기본 32바이트 문맥, 같은 노드의 측정 필드와 라벨을 사용한다. 정상/눌림/비활성 Draw 문맥을 측정 문맥으로 대체하지 않는다.

실제 저장소 연결 검사에서는 기본 생성·등록, 부모/배경 생성, FontStore 측정, 가상 Attach의 배경 크기 변경, 기본 트리의 자식 깊이 재배치, Kill/지연 삭제·자식 우선 scalar 소멸자를 연결했다. 측정만 할 때 배경 크기를 자동 변경하지 않는 동작도 확인했다. null 라벨의 첫 바이트만 쓰기, 짧은 strncpy의 0 채우기, 마지막 라벨 바이트 보존, 부분 문맥/측정값 보존과 할당 실패 후 부모 등록 유지도 확인했다.

기본 도움말은 메뉴 콜백이 바꾼 현재 제목·본문·이벤트·주제·루트를 다음 단계에서 읽는 순서와 판본별 마지막 인자를 C++에서 확인했다. **이 연결 검사는 기존 돌 버튼/배경/도움말 전체 몸체의 새 독립 PE 대조를 대신하지 않는다.**

## 독립 원본 관찰과 검증

`tools/decomp_gumptext_oracle.py`는 세 실제 PE의 측정 및 기본 Gump 위임을 격리 x86으로 실행한다. 실제 함수와 내부 strlen 루프를 대체하지 않는다. 폭 표·슬롯 높이를 입력으로 공급하며, 직접/기본 문맥/명시 문맥 위임 경로와 출력 alias를 포함한다. 정상 반환·스택 purge·callee-save, 내보낸 실행 범위, 출력/스택 쓰기 범위, 전체 캐시/높이·문맥·노드·문자열 불변과 출력 보호 바이트를 감사했다.

세 판본 각 **305개**, 새 독립 원본 **915개**다. C++는 원본 파일/Python 없이 저장한 18개 입력 폭 표로 임시 `.chfnt` 캐시를 만들고, 실제 FontStore가 조회한 결과를 **915개 모두 대조**한다. 이 관찰은 정수 함수이므로 별도 x87 모드 관찰 수를 추가하지 않는다. cpppj 누적 독립 원본 입력은 앞 단계 **520,756 + 915 = 521,671개**다.

새 C++ 검사는 **6개**다. 측정 전수 대조·바이트 범위 계약·돌 버튼 측정/트리/자식 우선 삭제·라벨 변경·할당 실패·도움말 콜백 순서를 포함한다. 최종 Release **경고/오류 0**, 전체 CTest 내부 **833개·실패 0**(내부 86.89초, CTest 전체 86.92초), 근거 재감사 통과다. 기본 Gump 측정 및 도움말 검사는 두 판본을 포함하며 돌 버튼 트리/수명 연결 검사는 10.78 상태다.

자료는 `cpppj/gumptext.json`, `cpppj/tests/fixtures/gumptext-x86.tsv`, `extracted/gumptext/`, `extracted/gumptext-build-final.log`, `extracted/gumptext-ctest.log`에 있다. SOURCE_MAP은 생성 도구로 확인했으며 내용 차이는 없다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name gumptext
python -X utf8 tools/decomp_gumptext_oracle.py
python -X utf8 tools/decomp_gumptext_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 2
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

## 남은 범위

실제 글리프 출력과 GumpDraw의 돌 버튼 텍스트 분배는 [같은 날 후속 완료](cpp-gump-glyph-draw.md)했다. 타입/SHP 배경 공급, MenuGump/Form·OS 입력 큐/폴링·overlay·Client 자동 UI 프레임이 남았다. 최신 돌 버튼/배경/도움말 전체 몸체의 독립 PE 실행 대조도 계속 미완료다. Client/UberGump는 기존 메뉴 어댑터를 사용하며 **새 raw 버튼의 자동 GUI 연결과 게임 내 건설/미션 완주는 아직 불가능하다**.

이번 단계에서 원본/복사본 게임 프로세스·클론 GUI·소리 장치를 실행하지 않았다. AGENTS.md·보호 원본 파일·dotnetpj/인계는 수정하지 않았으며 커밋/푸시하지 않았다.
