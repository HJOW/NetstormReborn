# 실제 기본 UI dirty 연결·합성 갱신 정리 — 단위625

현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이며 날짜는2026-10-23이다. AGENTS/양쪽 LEFT를 재독했다. 기준은624 커밋350ebc50이며 보호2622/fixture763을 유지했다.

## 변경

RawButtonControls의 tree/frame/button redraw가 버리던 token/erase를624의 GumpRedraw.Whole과 동일 Collection/현재 Node/현재 Renderer에 전달한다. 메뉴의 가상 Redraw는 기존 fancy/decoration 분배를 유지하고 기본 요청만 별도 baseRedraw로 연결해 재귀 호출을 만들지 않는다. 같은 RawMenuTree를 빌리는 모든 소유자가 기본 요청 장치를 공유한다. 독립 표시/CD의 미설정 장치는 기존 changed 통지를 유지한다.

영역 없는 기존 changed 소비자는 별도 invalidateAll 경계를 유지한다. RawProductionControls의 blink처럼 visible을 바꾼 뒤 changed만 호출하는 경로에 가시성 검사 후 dirty를 붙이면 이전 그림이 지워지지 않을 수 있다. 이 경로를 기본 사각형 요청으로 임의 치환하지 않았다. 각 소비자의 정확한 원문 호출/영역 복원은 후속이다.

Native UberGump.Compose는 빈 Overlay/Background를 매번 설치하지 않는다. 선택/배경은 단일 Native 목록/Frame이 소유하며, 빈 포인터를 설치할 때 생기던 전체 무효화를 제거했다. CD/독립 합성은 기존 계약을 유지한다. 원래 표면/입자의 현재 Renderer dirty 연결과 Windows10/11 DIB/전체화면 호환 경계도 유지한다.

관측 행 raw_ui_dirty는 기본 요청/실제 사각형/영역 없는 변경 누계이고 raw_renderer_dirty_area는 전체 화면 dirty 개수/누적 논리 픽셀 면적이다. 게임 판정·난수에는 쓰지 않는다.

## 검증

집중155개/실패0·실제 등록3535이며 최종 제품/검사 빌드는 경고·오류0이다. 새 실제 버튼 검사는 누름/이탈/복귀/해제의 원래 사각형과 페이지/팔레트 변경 경계를 확인한다.624의 원문1474행은 기존 fixture로 다시 재생했으며 새 원문/fixture 생성은 없다.

첫 빌드에서 inline RedrawBase 인수 token이 같은 구조체 멤버를 가리는 C4458을 확인했다. 실제625 CMake 부모9676/자식만 확인 후 중단하고 nodeToken으로 고쳤다. 실패/중단 로그와 완결 v2를 구별했다.

새 한글 배포본의 실제13회는 기존 Frame/구름/날씨·브리핑/ESC4회＋세 화면비 창/전체화면 pan6회＋QWASZX 다리1회＋첫 스톤 타워 건설1회＋UI 전체 새로고침 비교1회다. 모두 종료0이며 실제 창DC RGB가 논리 캡처와 같다.

UI 비교는 구름 없는 정적 메뉴에서 hover/Options/닫은Options/Help/닫은Help의5상태를 실제 입력으로 만든 뒤, 같은 상태의 다음 명시 refresh 결과와 전체RGB를 비교했다.5개 모두 차이0이고 실제 창RGB도 같다. 관측한20개 dirty 중4개가 부분 영역이다. 이 비교는 원본 게임 픽셀 일치나 모든 UI 소비자 완료를 의미하지 않는다.

정적 메뉴의100번째 보고에서 실제 Prepare100/dirty7, 구름 메뉴는 Prepare100/dirty100 중 전체 화면7·부분93이다.624의 동일 main-tint 관측은 Prepare100/dirty8이었다. 이를100→7 성능 향상이나 전체 미션 성능으로 확대하지 않는다.

보호2622/fixture763은 불변이고 ZIP1376항목/자산1358개가 실행 뒤/압축/보호 원천과 같다. 게임 SHA-256은 `dd9ffc162aa79f4b3ba830f31b0552c69ed436eaf0419be9b06f5847bb78b38e`, ZIP은 `ac4e7b5c7991b5e87eee60ce74328b487db88aedb73ceb5e17e8bb01727a5c02`, 최종 검사 실행 파일은 `862c39beac7daa765a5c1c1398db29c7ae7bd4b462b9db9bb7e10e62816f1f08`다. [완결 집계](../../cpppj/ui-dirty-runtime-20261031.json)·[UI 비교 도구](../../tools/cpp_ui_dirty_reference_smoke.py), 원시 `extracted/cpp-ui-dirty-625-20261031/`를 따른다.

## 다음

새625 제품의 전체34 진입을 다음626에서 확인한다. 최신 완결 전체34는622제품/623이다. 그 뒤 장식/메뉴 행의 이미 계산된 영역과 생산 blink/미니맵/구조 변경 등 changed 소비자를 원문 순서대로 연결한다. Renderer grid/debug·전체 UI/Edit/시나리오·수명/Native·남은 Win10/11 호환성/물리키/포커스·PCM·후속 MP무시험/4~7차와 자연 완주/전체 원본 동일성은 미완료다. 금지 재현/공중3종 전용/신규MP 검사는 없고 전체 목표는 active다.
