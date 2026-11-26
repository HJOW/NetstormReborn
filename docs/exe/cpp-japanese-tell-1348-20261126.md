# 일본어 Tell 첫 구간 — 단위1348

2026-11-26, HJOW-Athlon. 마지막 디컴파일 PC도 HJOW-Athlon이며 디컴파일 변경은 없다.

Tell의 COMMENTS~WELCOME2 구간을 번역했다. 입력 제목·길드/관리 경로의 표시 라벨·프로필·해상도 경고·회수 제한·시작 안내·Warning 문장을 일본어로 옮겼다. 비활성 온라인/관리 명령은 번역만 하며 실행하지 않았다. 파일은 UTF-8 BOM/CRLF이다. 뒤의 About~END는 원본 영어로 유지한다.

`translation-progress.json`의 partialFiles에 tell을 기록했다. 감사 도구는 존재42파일·전체 번역41파일·Tell 부분 번역을 구별하고, `--require-all`은 Tell/Help2미완료를 종료1로 거부했다. 한국어와 원본 영어42파일의 섹션·키·명령·AI/SP·치환·링크·그림에 더해 모든 색상/스타일 토큰도 비교한다. 새 스타일 감사는 기존41파일에서도 차이0이다.

Warning 태그의 표시 문장과 Tell tip0~39 값은 번역 가능한 값으로 구분했다. Warning 명령 자체·치환·스타일 및 팁 키·내부 명령·링크는 계속 비교한다. 최초 도구가 팁 문장을 실행 설정으로 취급한 assignments 불일치1건은 교정했고, 실제 원시 분석과 설명을 `audit-1348-first-note.txt`에 보존했다.

일본어 집중9/9검사와 실제GUI3회가 통과했다. 직접 WELCOME 본문을 열고 실제 확인→Welcome2→메인 메뉴 버튼을 눌렀다. 최초 설치 자연 진입 검사를 반복한 것으로 세지 않는다. 회수 제한·프로필 완료·해상도 경고·구역 제한·옛 유효성 경고도 표시만 확인했고 설정 적용/온라인 버튼은 누르지 않았다.

사제/섬 경고는 캠페인에서 원문의 missionPost 억제 때문에0건이었다. 이를 변경하지 않고 메인 메뉴의 같은 장치에서 표시를 요청해0→1→2줄과 일본어 전체 문장을 확인했다. 실제 사제 무력화나 적 공격 재현은 아니다. [시작 안내](../screens/cpp-japanese-tell-1348-20261126/welcome.png) · [다음 안내](../screens/cpp-japanese-tell-1348-20261126/welcome2.png) · [회수 제한](../screens/cpp-japanese-tell-1348-20261126/denysalvage.png) · [경고2줄](../screens/cpp-japanese-tell-1348-20261126/main-island-warning.png).

제품 C++/재빌드는 없다. EXE SHA-256 `458f23e09021c5c8d7230459c1499034d8fddfb69a471da60af5c39e4741b918`, 보호 원본/CD·AGENTS/README와 설치 사본3,212파일 지문은 전후 동일하다. [검증 JSON](../../cpppj/japanese-tell-1348-20261126.json), 원시 자료는 `extracted/cpp-followups-1328/*1348*`에 있다.

공식 전체 번역은41/43을 유지한다. 다음1349는 Tell About~UCampaign 구간(다음 Credits 직전)이다. 이후 Credits 묶음, NewTech~TipList, AllianceRequested~END를 각각 별도 단위로 진행한 뒤 Tell 전체를 완료 처리한다.
