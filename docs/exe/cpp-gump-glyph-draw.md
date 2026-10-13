# cpppj 실제 글리프 출력과 돌 버튼 Draw 연결

작업 PC: **VM-W11-CODEX**(`10.0.0.17`), 기록일: **2026-10-12**. AGENTS.md·두 LEFT_JOBS를 확인하고 마지막 디컴파일 PC와 현재 PC가 같은 것을 확인했다. [글자 측정](cpp-gump-text-measurement.md) 다음 단계인 실제 글리프 출력·돌 버튼 가상 Draw 분배를 완료했다.

`gumpglyph-functions.json`의 **3/3/3개** 함수를 읽기 전용으로 내보냈다. 기존 GUI 픽셀 회귀에 필요한 누락 산출물도 `gumpvisual`의 패치판 **2개**만 다시 내보냈다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-12.** 보호 원본과 Ghidra 프로젝트에는 변경을 저장하지 않았다.

## 원본 대응과 출력 규칙

| 처리 | 10.78 | CD 10.72·추가 10.37 |
| --- | --- | --- |
| 글리프 포인터 검사·전진 폭 반복 | `004a3430` | `00426250` |
| 글꼴 선택·길이·그림자/외곽선/본문 | `004a3f70` | `004260f0` |
| 기본 Gump 상대 위치·문맥 위임 | `00465050` | `00493ed0` |

[`GumpTextDraw`](../../cpppj/src/client/GumpTextDraw.h)는 실제 FontStore 캐시를 선택하고 공통 `DrawBitmapGlyphText`로 출력한다. 측정의 첫 폭 표와 ABC A를 출력 좌표에 쓰지 않는다. **글리프 포인터가 있을 때만 세 번째 폭 표(B+C)로 전진**한다. 특별 빈 공백 프레임은 포인터가 있으므로 전진하지만 픽셀을 그리지 않는다. 포인터가 없는 바이트는 폭도 전진하지 않는다. 바이트 0·128~255도 부호 없는 표 색인이다.

길이 -1은 NUL 앞까지, 명시 양수 길이는 중간 NUL도 출력한다. 다른 음수 길이는 원본의 `count != 0` 반복이 수십억 회 진행할 수 있어 호스트 계약에서 거부한다. NUL 누락/범위 초과도 거부하며 이 범위 밖 접근을 원본과 같은 동작이라고 판정하지 않는다. 저수준 반복은 입력을 확인한 뒤 출력하며, 상위 FontStore 조회·잘못된 스타일 로그나 이미 그린 버튼 선은 앞서 발생할 수 있다.

그림자 bit 2 → 외곽선 bit 4의 **위 `(0,-1)`·오른쪽 `(1,0)`·아래 `(0,1)`·왼쪽 `(-1,0)`** → 본문 순서다. 같은 패스마다 시작 x를 다시 잡는다. 원본의 고정 좌표 표와 PE 명령을 확인했으며 기존 Renderer의 네 방향 순서를 이 공통 반복으로 정정했다. 좌표/폭 덧셈은 signed overflow 없이 하위 DWORD 감김을 유지한다.

각 패스는 문자열이 비어 있거나 캐시 표가 비어 있어도 글꼴 색 표의 **100번 BYTE**를 바꾸고 색 표 설치 경계를 호출한다. `MakeGumpGlyphPixelHooks`는 256바이트 전체를 현재 VFX 색 상태로 복사하고 기존 VFX 해제·투명 합성기를 같은 `GumpPixelTarget`의 현재 inclusive 클립에 연결한다. GumpDraw의 클립 복구는 이 최종 본문 색을 복구하지 않는다. 콜백 실행 중 문자열/문맥/글꼴 입력을 변경하는 경우는 이 API의 계약 밖이다.

패치판 스타일 0~4, CD·10.37 스타일 0~2를 선택한다. **범위 밖 스타일은 원본 로그 경계 후 normal 슬롯 0**으로 대체한다. Ghidra의 패치판 디컴파일에는 fallback 대입과 숨겨진 EAX/EBX 인자가 생략되므로 실제 명령으로 보완했다. 슬롯 2/미생성 조합 및 CD 슬롯 6 bold는 빈 표다. 측정과 달리 출력의 잘못된 슬롯은 안전 예외로 거부한다.

## 같은 저장소의 돌 버튼 연결

`StoneButtonGump::Draw`는 기존 Draw 계획의 네 선을 출력하고 라벨을 실제 `GumpTextDraw::DrawGump`에 전달한다. wrapper는 현재 트리 Position과 상대 오프셋을 DWORD로 더한다. 기본 문맥은 Position/투영 뒤 해석하므로 투영 경계가 바꾼 현재 기본 문맥을 읽는다. 돌 버튼은 저장한 정상/눌림/비활성 **Draw 문맥**을 명시적으로 전달하며 기본 **측정 문맥**을 대신 쓰지 않는다.

`BindStoneButtonDraw`는 돌 버튼의 가상 Draw만 감싸고 다른 파생 타입의 기존 분배를 유지한다. 실제 같은 생성·트리·수집·정렬·kind 2·GumpDraw 클립 보호·FontStore·표면까지 두 판본의 C++ 연결을 확인했다. 정상/눌림 이동·비활성 그림자 생략·현재 클립·최종 색 유지, NUL 누락 예외 뒤 클립 복구와 앞서 그린 선 유지도 확인했다. 배경 자식은 별도 표시 항목이며 이번 검사에서는 기존 fallback으로 전달했다.

기존 `Renderer::Text`도 공통 글리프 반복을 사용하고 기존 해제 이미지 캐시는 유지한다. 기존 메뉴는 여전히 UberGump 어댑터를 사용한다. 이번 연결 검사가 실제 Client에서 새 raw 버튼을 자동 생성/구동한다는 뜻은 아니다.

## 독립 원본 관찰과 검증

`tools/decomp_gumpglyph_oracle.py`는 실제 세 PE의 세 몸체와 내부 strlen/표 선택/포인터 검사/전진 폭/패스 좌표를 격리 x86으로 실행한다. **VFX 색 설치·셰이프 출력, 이미 별도로 복원한 Position, 잘못된 스타일 로그만 명시 외부 경계**다. 실제 게임 프로세스·OS/GDI·픽셀 래스터는 실행하지 않는다.

18개 합성 캐시 입력의 첫 폭 99/ABC A -31과 서로 다른 signed 세 번째 폭·포인터 유무를 공급했다. 세 판본 **각 256개·새 독립 원본 768개**를 실행했다. C++는 같은 입력을 실제 `.chfnt`로 저장하고 FontStore가 읽어 선택한 글꼴/글리프 코드/좌표/색/로그/출력 순서를 **768개 모두** 대조한다. 독립 원본 누적은 **521,671 + 768 = 522,439개**다. 이번 몸체는 정수 코드이며 별도 x87 모드 관찰 수를 추가하지 않는다.

정상 반환·스택 purge·callee-save, 내보낸 불연속 실행 범위, 글꼴 색 BYTE/스택 이외 쓰기 거부, 전체 캐시·문맥·노드·문자열 불변과 색 표의 나머지 255바이트 보호를 확인했다. SHA/행 수/함수 진입/색·글리프·로그·Position 경계 수의 재감사도 통과했다. 이 대조는 외부 경계 콜백이 입력을 수정하지 않는 조건이다.

새 C++ 검사 **8개**가 통과했다. 원본 관찰 전수 외에 손계산 한 픽셀 캐시로 실제 전진/공백/없는 포인터·명시 NUL·그림자/외곽선/색 표·클립·현재 기본 문맥·Renderer 공통 경로와 raw 돌 버튼 연결을 검사했다. 최종 Release **경고/오류 0**, 전체 CTest 내부 **841개·실패 0**(150.84초, 전체 150.89초)다.

기존 메뉴 GUI **54개 상태**와 해상도 전환·전체화면 표식/설정 저장·재실행, 메뉴 배경 **774,432픽셀**, 두 판본 GIF **각 831,008픽셀** 대조를 통과했다. 원본 버튼 몸체와 독립 `.chfnt` 해독에 의한 추가 GUI **14개 상태·81개 버튼 계획·선/글자 32,220픽셀**도 통과했다. 이 GUI는 기존 메뉴 어댑터 회귀다. 보호 원본·설정/존재 상태 불변과 복구를 확인했으며 돌 버튼 GUI 검사에는 원본 **1,394개 파일**이 포함된다.

자료: `cpppj/gumpglyph.json`, `cpppj/tests/fixtures/gumpglyph-x86.tsv`, `GumpGlyphTests.cpp`, `GumpTextTests.cpp`, `extracted/gumpglyph/`, `extracted/gumpglyph-*.log`, `extracted/cpp-menu-smoke/`, `extracted/cpp-gumpvisual-smoke/`. SOURCE_MAP은 생성 도구로 확인했고 내용 차이는 없다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name gumpglyph
python -X utf8 tools/decomp_gumpglyph_oracle.py
python -X utf8 tools/decomp_gumpglyph_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 2
ctest --test-dir cpppj/build --build-config Release --output-on-failure
python -X utf8 tools/cpp_menu_smoke.py
# 기존 GUI 버튼 대조 산출물이 없는 PC에서는 패치판 두 몸체를 읽기 전용으로 내보낸다.
powershell -NoProfile -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name gumpvisual -Edition originals
python -X utf8 tools/cpp_gumpvisual_smoke.py
```

## 다음 작업

raw 배경의 실제 타입/SHP 저장소 공급·가상 Draw 분배는 [같은 날 후속 완료](cpp-gump-background-draw.md)했다. MenuGump/Form·OS 큐/폴링·overlay·Client 자동 UI 프레임이 남았다. 기존 돌 버튼 **전체 생성/부착/소멸자**·배경 생성·기본 도움말 전체 몸체의 독립 PE 대조도 미완료다. 글리프 VFX 래스터 전체 몸체의 임의 캐시 입력 독립 대조와 수정 가능한 콜백 입력 모델도 이번 완료 범위에 포함하지 않는다.

**새 raw 버튼의 Client 자동 GUI 연결과 게임 내 건설/미션 완주는 아직 불가능하다.** 원본/복사본 게임 프로세스와 소리 장치는 실행하지 않았다. 지정 예외 PC에서 클론 GUI만 검증했다. AGENTS.md·보호 원본·dotnetpj/인계는 수정하지 않았으며 커밋/푸시하지 않았다.
