# 758 — World 지식 카드와 Storm/World 선택 메뉴

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 변경하지 않았다. 단위758은 완료했고 전체 목표는 미완료다.

## 변경과 원문 근거

실제 이름표00542b90의 ShowTecSideMenu(005434a8→00542b84→02000132), ShowTecMenu2(005434b0→00542b88→02000133)를 기존 HTML 이름조회·문자열 고리·FIFO·일반 명령 허용표에 연결했다. UberGump→RawMenuControls→현재 RawCampaignSession의 기존 playerKnown/challenge 관문을 거친다. F6의 기본 Storm 메뉴는 유지한다.

493210의 World는 theme4..7, EARTH/FIRE/WATER./MOON, 아이콘12..15를 사용한다. 기존 Storm과 같은112BYTE 기록 수집·CRT 정렬·네 줄 누적 배치를 공유한다. 4938c0의 선택 메뉴는 Storm 다음 World의300폭 두 카드, ~[IMANA.16], 원문 텍스트와 명령을 같은 실제 Gump 작성기로 만든다. 마지막 위치 재조회·popup 표식 쓰기 순서를 보존했다.

기본 loose english/cfg에서 두 이름의 직접 활성 caller는 찾지 못했다. 원문 이름/숫자/일반 명령 경로를 복원했으며 새 기본 버튼을 임의로 만들지 않았다. CD10.72의 동일 함수 대응과 전체 메뉴 동일성은 이번 범위가 아니다.

## 검증과 보호

원본 PE의 World25/Side25 입력을 기본 x87 정밀도에서 정상 반환50회 관측했다. 이름표/토큰/실제 큐21입력은 별도다. root가 각각 다시 관측한 fixture 전체 바이트가 일치했다. 두 정밀도 검사라고 합산하지 않는다. 도구의 주석 보완 전 SHA는 최초 native 보고서에 그대로 보존하고 재관측 보고서와 커밋 sourceSha256을 구별했다.

Release 제품과 기존 netstorm_script_bindings_tests를 빌드했다. 경고/오류0이며 KnowledgeWorldMenu 새2개와 기존 KnowledgeMenu2개, 총4개/실패0이다. 실제 HtmlScriptCommandValue 이름조회→FIFO와 원문주소 허용표도 검증했다. 최초 좁은 이름조회 검사와 강화 후 로그는 각각 tests-knowledge-v1/v2.log로 보존했다. 실행대상은 새로 만들지 않았다.

보호 2667개 모두 불변, 새fixture2개를 더해 2669개/fixture849다. 다음 기준은 extracted/cpp-resume-758/protected-after.json이다. 실제 제품 SHA-256은 `812d4e2d5a7ebeecb62b84d0a91d651c315b5b29899a47171d432fa55692fb8a`다.

원시는 extracted/cpp-resume-758의 build-v1/v2.log, tests-knowledge-v1/v2.log, native.json/names-native.json 및 두 재관측 보고서에 있다. 커밋 요약은 [보고서](../../cpppj/knowledge-world-menu-20261103.json)를 따른다.

## 경계와 다음

원문 TYPE/소유/제외/번역/GUI 생성·부착·가상 호출은 명시 장치 경계다. 제품 연결은 컴파일/집중검사이며 이번 실제 GUI 관측은0이다. 물리 입력/PCM·포획/희생·특정적공격·승리·자연완주·MP·전체C++/34/ZIP 검사도0이다. 마지막 GUI746와 전체34/ZIP645의644제품은 별도 근거다.

다음은759 kind17 공통 생성자와760~765의 준비된 독립 작업이다. 사용자 확인/멀티플레이 검사 생략과 단계 순서를 유지한다.
