# 스커미시 선택 아이콘 통일 — 제품1387 (2026-11-29)

마지막 디컴파일 PC 호스트명과 이번 작업 PC는 **HJOW-Athlon**이다. 재디컴파일과 디컴파일 원문 변경은 없다. 1381 뒤 중지 상태는 유지하고, 이번 사용자 요청의 메뉴 표시만 수정했다.

## 동작

`SkirmishSetup.cpp`의 인공지능 수·개별 난이도·적/동맹·금지 유닛 행을 `$Checked=라벨,Tell,대상,선택값,1`로 생성한다. 이 행은 기존 `HtmlWindowMetadata` → `MenuGumpControls::AddChecked` → `MenuGumpDraw`의 선택 비트 `4`와 원본 선택 SHP 출력 경로를 사용한다. `*`와 `[X]/[ ]` 문자 표시는 없애고 금지 안내도 영어·한국어·일본어에서 아이콘 설명으로 고쳤다.

`$Checked`의 대상 열은 쉼표를 허용하지 않으므로 새 동작 대상은 `Patch1101.SkirmishSet(diff;2;3)`처럼 세미콜론을 쓴다. `ParseSkirmishTarget`에서 이를 쉼표로 바꿔 기존 인자 처리에 넘긴다. 기존 `SkirmishSet(diff,2,3)` 대상과 설정 저장 형식은 계속 지원한다. 공통 HTML 메타 처리기와 디컴파일 원문의 문법은 수정하지 않았다.

메타 처리기는 `$Menu`를 `$Checked`보다 먼저 모으므로, 금지 유닛 목록의 다음/이전 쪽·모두 해제도 선택값 `0`인 `$Checked`로 생성한다. 유닛 후보 뒤 탐색·해제 행의 순서를 보존하고 해당 탐색 행에는 선택 아이콘을 켜지 않는다. cpppj 캠페인의 구분선/사용자 제작 캠페인 배치는 기존대로다.

## 검증

- `cmake --build cpppj/build --config Release --target NetstormCpp netstorm_skirmish_setup_tests --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true` 종료0.
- `netstorm_skirmish_setup_tests.exe` **9/9 통과**. 기존 대상/설정/대본/맵/메뉴 검사와 실제 HTML 메타 처리기 통합 검사(영어·한국어·일본어 × 수/AI/금지 첫쪽/금지 끝쪽의12화면)를 포함한다. 선택 비트·동작 인자 전체·설정 적용·탐색 행 순서를 확인했다.
- `netstorm_product_ui_tests.exe` **25/25 통과**. 원본 HTML 메타128입력 회귀를 포함한다. 이 메타 처리기 본체는 이번 작업에서 바꾸지 않았다.
- 실제 GUI **1회 종료0**, 미션 진입0. 메인 Options와 같은 선택 비트4가 AI 수2·Hard/Ally에 켜지는 것을 보고서와 화면에서 확인했다. 수2→3, 난이도Hard→Easy, Ally→Enemy, 금지 첫쪽의 Wind Generator, 끝쪽의 Outpost 선택, 다음/이전·모두 해제, 취소→Campaign까지 실제 입력 경로로 확인했다. 금지 후보 뒤 탐색 행 순서도 확인했다.
- 보호 대상 **2,809파일**(originals·originalCD·기존 C++ fixture·AGENTS.md)의 이름/내용 해시가 전후 같다. `NETSTORM_USER_DATA`로 이번 실행의 임시 저장 루트를 사용해 기존 사용자 설정을 보존했고, 원본의 허용 설정 두 파일도 전후 바이트/존재 상태를 복원했다.
- 제품 EXE SHA-256: `a6ec6cb839b0f6dbbd14b9932639b6550dcdb42fbcf3d9e5c78b66cdda734b65`.

로컬 실행 근거는 `extracted/cpp-skirmish-marker-1387/`의 `steps.tsv`, `states.tsv`, `summary.json`, `process.json`, `protection-before.json`, `protection-after.json`, `options.bmp`·`count.bmp`·`ai.bmp`·`bans.bmp`·`outpost.bmp`다(Git 제외). 화면은1024×768 영어 창 모드이며, 한국어·일본어는 위 메타 통합 검사 범위다. 전체 관문·배포 ZIP 재생성과 자연 전투 플레이는 이번 변경 범위가 아니다. 최신 ZIP1378에는 이번1387이 포함되지 않는다.
