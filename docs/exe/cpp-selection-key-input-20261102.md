# 선택 키 입력 부모·View 큐·직접 미션 진입 — 단위704

2026-11-02, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. H/F4/F5의 직접 Home 처리를 원문 일반 View 명령으로 바꾸고 N/P/R의 입력 부모 정지 관문을 연결했다. 실제 짧은 검사에서 발견한 `--mission`의 메인 부모 미정리 회귀도 수정했다.

## 입력과 Guide

`SelectionKeyInput`은 원문004d96b6의 정지 관문 뒤 실제 jump table/수정키 분기와 같은 다음 동작을 사용한다.

- H/h는 ViewHome(005429ac/020000ae), 두 인자0이다. F4도 같은 큐지만 Shift 또는Ctrl이면 거부한다. Alt를 별도로 거부하지 않는다.
- 일반/Shift F5는 ViewYourPriest(005429d4/020000b8)와 현재 localOwner 두 인자다. Ctrl+F5는 기존 다음 유닛 선택에 원문 Shift 비트 전체를 넘긴다.
- N/n과 P/p/R/r는 기존702/703 몸체다. 일반 키 부모의 정지 관문을 먼저 지난다. 702의 직접 몸체가 정지 중 첫 선택/카메라를 시도한다는 사실은 원시 N 입력이 정지 중 이 관문을 건너뛴다는 뜻이 아니다.
- 문자 하위16비트만 비교하지 않고 실제03ffffff 마스크 전체를 비교한다. 뗌과 다른 명령/마우스 사건을 키로 오인하지 않는다.

이제 H/F4는 기존 `GameMenuActions::ViewHome`의 playerKnown/메타맵/Player+7c/+84 관문을 거친다. F5도 기존 메뉴의 명시 owner 경로다. 큐 포화 때 직접 카메라로 우회하지 않는다.

Guide 앞에서 원시 F4를020000ae로 합성하던 변환을 제거했다. 원시 키는 원래대로 Guide에 전달하고, 실제 큐의 Home 사건이 다음 입력에서 한 번 도착한다. 단위 검사에서 Early1 단계A의 제작수0으로 원시 F4가 단계를 올리지 않고 큐 Home만 올리는 경계와 기존 ViewHome 차단/실행을 연결했다. 실제 Early1 자연 진행/완주의 검사가 아니다.

## 원문과 C++

[원문 도구](../../tools/decomp_selection_key_input.py)는004d96b6..004d9bba의 공통 정지/키 분배를 **480입력**으로 실행하고004d9bbb에서 스택 증가0을 확인했다. paused00460de0·queue00452fe0·next004da880·priest 입구004d9884는 명시 경계다. key-up 관문은 이 범위 밖이며 전체 UserInput/OS 실행으로 확대하지 않는다. 보호 PE의 두 명령 초기값도 확인했다. [fixture](../../cpppj/tests/fixtures/selection-key-input-x86.tsv)·[원문 보고서](../../cpppj/selection-key-input-native-20261102.json)를 따른다.

새4개는480행 전체 대조, 정확한 키/정지·수정키 관문, 실제 InputQueue→Early1 Guide→기존 View 메뉴 접합, 큐 포화/장치 누락이다. 기존 N3·P/R3·GameMenuActions2·TutorialOne4와 함께 **16개 통과·실패0**이다. 기존410/220/610/104/266행 등의 fixture 재생은 새 원문 실행 수에 합산하지 않는다. 첫 build-v1의 새 optional 비교 signed/unsigned 경고2개는 기대0을0U로 바꿔 해결했다. 최종 build-v4 경고·오류0, focused-v2 통과다.

## 실제 실행과 회귀 수정

[실제 도구](../../tools/cpp_selection_key_smoke.py)의 최초 live-v1은 로딩 중 고정 Play Mission 클릭 때문에 exit1이었다. 현재 표시된 안내 버튼만 조건부로 누르도록 고친 live-v2는 exit0이었지만, 모든 관측이 Loading이었다. 원시 보고에서 살아 있는 메인 부모201/현재 모달201, 미션 Init/Start0을 확인했다.

700의 실제 메인 부모가 `--mission` 직접 진입에서 반납되지 않아 Guide의 dialogsClear를 막았다. 실제 메인 버튼 클릭은 이미 같은 부모를 Kill하지만 CLI는 그 클릭을 생략한다. `StartMission` 예약은 첫 Native3보다 앞일 수 있으므로, **실제 BeginMission 시점**에 살아 있는 메인 부모가 있으면 같은 공통 Kill만 예약한다. 이 CLI는 원본에 없는 진입 경로임을 한국어 코드 주석에 명시했다. 메인 serial을0으로 합성하거나 Guide의 모달 관문을 제거하지 않았다. 중간 build-v3는 예약 위치에 정리를 두었다가 실행 전에 실제 Begin 위치로 수정한 빌드이며 게임 실행으로 세지 않는다.

최종 live-v3는 같은 앱 내 키 계획으로 캠페인1-1의 정상 Mission/Session·종료0을 확인했다. P 선택은SID15000, 반복 P/N/CtrlF5는 같은 SID여도 실제 카메라 revision이3→4→5→6으로 늘었다. 정지 동안 선택/카메라/게임 시각과 revision6이 유지됐다. H와F4는(784,804), F5는(976,793)으로 실제 왕복했다. Ctrl/Shift F4와 key-up은 revision을 늘리지 않았다. 캡처 `home.bmp`의 실제 화면도 확인했다. 원본 전체 픽셀 일치/물리 입력 증거는 아니다.

**실제 게임3회**를 구별한다: 최초 도구 클릭 실패 exit1, 제품 직접 진입 미정리의 exit0/Loading 실패, 수정 뒤 최종 exit0/기능 확인 통과다. 실패 원시를 덮거나 같은 기대 fixture를 고치지 않았다. 최종 GUI 제품 SHA는 아래와 같다.

## 제품·보호·다음

실행 제품704/표시11.01 SHA-256은 `e3a7a5e6735c581a3f8e987937e71750be35025e79bb2be6c348f58451d7d9f3`다. [제품 보고서](../../cpppj/selection-key-input-20261102.json)와 원시 `extracted/cpp-selection-key-input-704/`를 따른다. 세 실행의 전후 AGENTS·원본1394·CD425·기존fixture802의2622개 변경0과 새fixture1 포함 현재2623개를 확인했다. 허용 설정은 기존 보존 도구로 복원했다.

전체 C++·전체34·새 ZIP·실제 PCM·자연 완주·포획/희생·특정 적 공격·승리·공중3종 전용 생산/생성/행동·MP/통신 검사는 없다. 전체34/ZIP의 최신 근거는 여전히645의644제품이다. 메뉴/키의 같은 제품 통과를 다른 미검증 조합에 확대하지 않는다.

다음은 남은 ESC/View·도킹·DD 조정/확인 UI와 Edit 지형/객체·시나리오 writer/파일 수명이다. 전체 목표는 미완료이며 MP 무시험 사전 구현과4~7차 순서를 유지한다.
