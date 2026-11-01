# 영문 HTML 명령 목록과 DumpSquids — 단위715

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 정밀 자료 기준일은 **2026-10-23**이다. 714 이후의 비멀티 Edit/시나리오 연결을 넓혀 확인했다. **715 / 표시11.01**, EXE SHA-256은 `217668552e8d2c4fe42b6182654cfe8399a8a52cedd97d347fcab6a62273e74c`다. 새 게임/GUI/외부 통신은 실행하지 않았다.

## 실제 데이터와 누락

[리터럴 조사 도구](../../tools/cpp_html_command_inventory.py)는 보호 PE의 이름표288개와 loose499/TARC50의 **영문 문서549개**를 읽었다. `$Menu` 둘째 인자·다른 메타의 명시 명령·1차 `<$태그>`에서94종을 모았고 그중57개가 스캔한 PE 이름표에 있었다. 출처를 합쳐 실제 파일 가림을 계산하거나 동적 Config 치환을 실행한 결과는 아니다. CPP 따옴표 문자열 히트는 주석/무관 분기를 포함할 수 있어 구현 완료로 판정하지 않았다. 이 최초 소스 조사는 `6d6b77bb` 기준이며 후속 제품에 맞춰 보고서를 덮어쓰지 않았다.

CPP 문자열이 없던 등록 이름13개 중 `DumpSquids`는 loose 시나리오의 명시 태그60곳에 쓰였다. 원본 이름표 **00543418→00542b3c / 0200011b**이며 실제 일반 입력은 `CreateRandomGuns` 등과 함께 명시 제외한다. 개발자 Ctrl+Shift+F7의004d6020 객체 덤프/프로세스 요약과 다른 경로다. 따라서 별도 덤프를 호출하는 새 기능을 만들지 않고 `ScenarioEffectScriptCode`/현재 HTML 조회·큐와 같은 명시 무동작 소비에 연결했다. 메뉴 허용과 일반 입력의 분류도 같은 네 이름을 사용한다.

나머지 문자열 부재12개는 TARC Tell의 Setup/Challenge 참가·항해/동맹/랭크/서버 시작·guild/zone 갱신 문맥에서 확인했다. 실제 문맥은 `extracted/cpp-html-command-inventory-715/tell-unmapped-contexts.json`에 있다. 후순위 MP 사전 구현 후보이며 새 검사/실행을 하지 않았다. 공유 경계인 LeaveBattle/RequestStormpower도 새 이름 대조에서 보수적으로 제외했다. 비등록37종에는 메타/동적 인자/공백·쉼표가 들어간 원문/오자 후보가 섞여 있어 임의 별칭·trim을 추가하지 않았다.

## 실제 조회기와 Menu 인자

`HtmlScriptCommandValue`는 실제 Client HTML이 쓰는 같은 `DialogScriptCode`를 읽기 제공한다. 별도 표를 만들어 통과시키지 않으며 GameWorld/문자열 고리/입력 큐를 실행하지 않는다. [PE 이름 고정 도구](../../tools/decomp_html_solo_command_names.py)는 위57종에서 MP 관련/공유14개를 뺀 **43개**의 포인터/원문 이름/현재 코드 DWORD를 보호 PE에서 다시 읽었다. 이 단계는 정적 PE 읽기이며 기계어 실행이 아니다.

CPP 검사는 같은 실제 조회기에 **43개×원래/소문자/대문자/교대=172개 표기**를 전달해 모두 원문 코드를 확인했다. 별도 명시 메뉴 검사는 실제 `HtmlWindowMetadata.Process`에 `$Menu=Trace,DumpSquids,6008`을 넣고 숫자 인자6008/같은 명령·변하지 않은 문자열 고리/단일 메뉴 열을 확인한 뒤 실제40칸 큐→동일 입력 소비자까지 연결했다. Gump/메뉴 발행 장치는 명시 경계이며 전체 HTML GUI나 모든 명령 몸체를 실행한 검사가 아니다.

## 독립 원문과 검증

[원문 도구](../../tools/decomp_dump_squids_input.py)는 제품 변경 전에 실제 일반 입력의 **48입력×x87 정밀도/반복3=144관측**을 동결했다. 누름/뗌·상위 플래그·인자0/6008/ffffffff·독립 Edit/미션 입력을 대조했다. 기존 실제 큐/원문 일반 입력 문맥을 쓰고 큰 Guide/Gump/UI 장치만 기존 명시 입력으로 제공한다. 모든 관측이 실제 몸체 trace 없음·정상 완료·스택 차이0이며 정밀도/반복 차이0이다. 게임·OS·덤프 파일·통신은 실행하지 않았다.

- 새 무동작 fixture48행 SHA: `c92a0531a8d1dd6ef4ab8540aa7b11543bfc9d21b08849e19506043d38758f7a`.
- 새 이름표43행 SHA: `9e63cd5933ec3f5770bda260076794eff2c4315e04bf977b18cc50e6dd21fc5a`.
- **새3/기존3 집중6개 실패0**: 실제 이름표43/172표기, Menu 둘째 인자/큐, DumpSquids 원문48행, 확장한 기존 이름/큐, 기존 표시576행 replay, 기존 뗌/선택 장치 부재 계약이다.
- build-v1/v2/v3 모두 경고/오류0이다. **제품은v2**, 검사 실행기는v3이다. 첫 검사에는 진행 중 컴파일보다 늦게 수정된 schema 행 제외 코드가 들어가지 않아5단언이 실패했다. 원문 TSV를 고치지 않고 해당 검사 파일의 재컴파일을 명시해 최종6개가 통과했다. Gump 입력은 명시0 초기 상태로 제공한다.
- 기존 보호 **2631개 전체 변경0**, 새 fixture2개를 더한 다음 보호 기준은 **2633개**다. AGENTS는5867f35e 그대로다. 최초 원문/PE 이름표의 읽기 검증도 통과했다.

## 한계와 다음

이번 연결은 데이터에서 발견한 실제 등록 이름과 원문 명시 제외를 복원한다. 549문서의 명시 목록/43개 이름 일치만으로 모든 TARC/Config 동적 확장·명령 몸체·모든 Menu/파일 실패 수명·전체 게임 동일성을 완료로 판정하지 않는다. 사용자 데이터의 잘못된 쉼표/철자와 등록 이름을 구별한다. CD 10.72의 새 몸체 대조도 하지 않았다.

남은 Edit 지형/객체·소유자 도구, 동적 시나리오 writer/명령 부모·파일 수명·연속 Retry/Next/Abort와 State3 특이 분기를 잇는다. 마지막 실제 GUI는714의 Early1이고 전체34/ZIP은645의644제품이다. 새 게임/자연 완주/물리 키·픽셀·PCM/포획·희생·특정 적 공격·승리/공중3종 전용/새 MP·통신/전체34·전체C++·CTest·ZIP은0이다. 전체 목표는 미완료다.

[완료 보고서](../../cpppj/dump-squids-20261102.json), [리터럴 목록](../../cpppj/html-command-inventory-20261102.json), [무동작 원문](../../cpppj/dump-squids-input-native-20261102.json), [PE 이름표](../../cpppj/html-solo-command-names-native-20261102.json)를 따른다. 원시 자료는 `extracted/cpp-dump-squids-715/`와 `extracted/cpp-html-command-inventory-715/`다.
