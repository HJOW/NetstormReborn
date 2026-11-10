# helpWord 캐시 (M7.1) — 작업1268

2026-11-11 / HJOW-Athlon. **원문 0054db38 helpWord 전역 캐시의 갱신 규칙(4413c0/441300)을 Client 수명의 단일 소유자로 구현하고 소비자 전부를 같은 캐시에 연결했다.** 사전 감사는 [M7.1/2 준비](cpp-help-cursor-remaining-audit-20261108.md)를 따랐다. 마지막 Client 디컴파일 2026-11-03(HJOW-Athlon)과 현재 호스트가 같고 새 디컴파일/원문 변경은 없다. **M7.2~M7.5·M7·M8과 전체 목표는 미완료이며 MP 실행은 0이다.**

## 구현

- `client/ClientHelpWord.h/.cpp`: 최초 NULL, `Interpret(설정 표시 바이트)`가 원문 규칙을 적용한다. 설정이 없거나 빈 문자열이면 아무것도 바꾸지 않고(기존 캐시 유지), 비어 있지 않으면 기존 캐시와 ASCII 대소문자 무시로 같을 때 옛 표기를 유지하고 다를 때만 교체한다(`Generation()`으로 구별). 비영 빈 문자열은 NULL과 구별된다.
- `ClientLocalStateGlobals::HelpWord()`가 Session/메뉴 교체와 무관한 한 개의 캐시를 소유한다. `Client::InterpretOptions`가 원문 435220의 위치(음향 옵션 앞)에서 `configuration_.Configuration().Get("helpWord")`를 `EncodeDisplayText`로 한 번 부호화해 `Interpret`에 넘긴다. 최초 시작·State 재해석·등록 메시지 재해석(`hooks.interpret`)·언어 변경이 모두 같은 함수를 거친다.
- 소비자: 객체 메뉴 `ObjectMenuState.helpWordRead`(항목 작성 위치에서 현재 캐시 조회, 독립 fixture의 값형 `helpWord`는 fallback 유지), Factory/풀숲/Inventory About 메뉴 콜백, 메타 객체 입력(`NativeMetaObjectInput`), 게임 메뉴 `bar.about`(FontBytes/Translate로 재변환하지 않음), BaseHelp의 특수 토큰 `61000002`(`RawHelpHooks::helpWord`, 제목 callback 뒤 해석 시점에 읽음, NULL이면 명시 오류).
- 한국어 `helpWord="정보"`는 표시 바이트 그대로 보관되며 `EncodeOriginalText`/설정 저장 경계는 넓히지 않았다.

## 검증

Release/x64 게임 2개 + 집중 대상 빌드 **191.97초/종료0**(최초 356.7초 실패는 새 검사 파일의 내 주석 실수 1건이며 교정), 경고·오류 0. 새 `netstorm_help_word_tests` **7/7**(키 부재/빈 값 NULL 유지, 기존 비NULL 유지, 대소문자 동일 옛 표기 유지, 교체와 generation, 비영 빈 문자열 구별, 한글 표시 바이트 정확 비교, 상태 전역 단일 캐시)와 `ObjectMenu_HelpWordReadUsesCurrentCacheAtItemTime`이 통과한다. `netstorm_menu_session_tests`는 처음 25건이 실패했다: `Run` 없이 만든 테스트용 Client에는 InterpretOptions가 없어 캐시가 NULL이었기 때문이며 두 시험 장면(`CampaignObjectMenuTests.cpp`)에 같은 규칙의 캐시 설치를 추가해 **6건**으로 줄었다. 남은 6건의 메시지는 "포획 안내 실제 Player 이름 미연결"·"표면/Template의동일로딩깊이owner대여계약"(20261108 도입)·"Factory의 선택된 원문 진단 장치 미연결"로 helpWord와 무관하다. **이 6건을 변경 전 기준선에서 직접 다시 실행해 확인하지는 않았다.** `netstorm_main_popup_tests`는 3건(`CampaignSaveGoMain_…`, `CampaignNativeFortContinuation_…` 2건)이 실패하며 이전 기록과 같은 이름·개수다.

최종 EXE의 정상 Tutorial4 진입 → 공장 메뉴(helpWord 항목 포함) → 생산 선택 → 건설 요청(TYPE75/owner1) → 600프레임 대기(게임 시각 13.6→24.2초) → 종료 코드 0, commandErrors 없음. 포획/희생·특정 적 공격·승리·공중3종·MP/Join/Test Battle 실행은 0이다. 보호 원본/CD/fixture/AGENTS 2,808파일은 불변이다.

## 남은 M7

M7.2(메타 활성 커서 준비), M7.3(조기/원격 희생 안내 — 정적 대조만), M7.4(marker 표시/갱신), M7.5(공통 assertion 창/파일 중앙화)는 미완료다.
