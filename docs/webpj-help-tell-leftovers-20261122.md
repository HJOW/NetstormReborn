# 도움말·Tell 남은 두 행의 닫음 (WB04-E1197)

`QuickHelpWindow`·`TellFileMenus` 대응표 행이 partial 로 남긴 이유(언어 전환 때 창·목록 재적재, 긴 제목 80바이트 절단 NB24)가 웹에서 닿는지 확인했다.

## 언어 전환(두 행 공통)

웹은 옵션 메뉴의 Language 선택을 설정에 저장하고 주소의 `lang` 을 지운 뒤 **페이지를 다시 불러** 적용한다(`netstorm.js _chooseLanguage`, E1095). 열린 도움말 창·Tell 파일 목록·번역 캐시가 모두 새로 만들어지므로 원본 10.78 이후 변경(언어 변경 때 `InvalidateLanguageFiles`·열린 창 재사용)이 막으려던 오래된 목록·창 문제가 없다.

## 긴 제목 NB24(`TellFileMenus`)

`Title` 을 80바이트로 자르는 수정이다. 출하 대본 82개(`\D\*.english`·`*.korean`·`*.fort` 읽기, 영어·한국어 각각)의 `title="…"` 41개를 모두 세어 최대 길이가 20바이트이고 80바이트 이상은 0개임을 확인했다. 사용자 제작 캠페인(User Made Campaigns)과 편집기는 웹 범위 밖이라 긴 제목이 들어올 경로가 없다.

## 도움말 전 주제

영어 도움말 파일의 모든 주제(`<a name=…>` 128개)를 게임 자원과 같은 구성으로 `createHelpWindow` 에 열어, 지원하지 않는 기능 알림과 예외가 하나도 나오지 않음을 확인했다(앞서 Tell 섹션 487개도 같은 방식으로 점검해 미지원은 멀티플레이·관리자 전용 `$Input`/`$Zsend`/`*SOLOLIST*`/`*FORTLIST*`/`*TECHLIST*` 뿐이었다).

## 처리

두 행을 `port` 로 닫았다(남은 양 113단위·29,534줄 → 111단위·29,266줄). 점검 도구는 scratchpad 의 `helpscan.mjs`·`titles.mjs`·`tellunsupported.mjs` 이다.
