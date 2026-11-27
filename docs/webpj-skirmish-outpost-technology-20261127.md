# 스커미시 Outpost 시작 기술과 실제 메뉴 — WB04-E1447

2026-11-27 HJOW-Athlon. 스커미시에서 금지하지 않은 Outpost도 시작 기술에 없어 건설 메뉴에 나오지 않던 문제를 고쳤다. 실제 보호 자산 TYPE126은 group10·가격600이며, 원래 `all` 기술 해석은 group10을 제외한다. 스커미시 생성 대본의 로컬/모든 AI 기술에만 `all;outpost`를 명시했다. 일반 `all` 해석과 캠페인/데모 대본은 그대로다. C++ SkirmishSetup에도 all만 지정하는 같은 차이가 있지만 이번 WB 작업에서 CPP를 수정하지 않았다.

실제 자산/원래 기술 해석기를 사용하는 신규 검사는 수정 전 실패했고 후보에서1/1 통과했다. 일반 all 제외와 참가자별 Outpost 한 번 포함·40개 용량을 확인한다. 본 소스 관련20/20, ESLint 선행 운영 빌드 종료0, 현재 소스 재번들1/1, 기존 Chromium 시작 상태/덤프 L2 1/1을 통과했다. 운영 번들은3,515,282바이트다.

실제 Chromium·1024×768·기본 시작SP5000·seed4에서 아군AI를 켜고 사제의 원래 우클릭 메뉴를 생성했다. 수정 전 양쪽 기술false/메뉴없음, 후보와 적용 본 소스는 양쪽true였다. 허용 때 실제 ClientMenuGump writer가 `Build Outpost for 600` 항목을 enabled=true로 만들고, 금지 때는 만들지 않았다. 본 소스 재실행에는 모듈 대체가 없고 게임 오류0이다. 이 항목은 Construct 하위 메뉴이며, 저장한 루트 메뉴 그림만으로 하위 항목의 실제 펼침을 주장하지 않는다. writer 관찰은 원래 함수를 그대로 전달했다. 첫 상대경로 import 실패와 화면 밖 사제 탐색 실패는 원문으로 보존했다. F5는 원래 사제 보기 입력이다.

실제 건설은 별도 진행 중이다. 원래 메뉴로 조각을 들 수 있으나 초기 시점에 Energy not satisfied가 표시되어 전력 선행조건을 확인하고 있다. 이 단위를 건설 완료·Outpost 전체 기능 완료로 세지 않는다. 게임 상태/자금/HP/목표를 강제로 바꾸지 않았다. 완료 뒤 v26=4/4·v22=4/8이므로 다음은 Node26 전체 회귀다.

[수정 전](webpj-e1447-evidence/before.log) · [후보](webpj-e1447-evidence/candidate.log) · [집중20개](webpj-e1447-evidence/webpj-e1447-main-focus.log) · [빌드](webpj-e1447-evidence/webpj-e1447-build.log) · [재번들](webpj-e1447-evidence/webpj-e1447-bundle.log) · [L2](webpj-e1447-evidence/webpj-e1447-l2.log) · [실제 본 소스 메뉴](webpj-e1447-evidence/browser-main.json) · [출처/해시](webpj-e1447-evidence/provenance.json).
