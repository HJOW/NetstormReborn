# HTML `$Input` 입력 창·편집기 이름 입력의 웹 범위 판정 (WB04-E1192)

`SaveFortNameDialog`(원문 004d3860 계열)와 `RawMenuSaveFort.inc`(조립)의 남은 가지 — Save as 이름, 설명(`ShowDescription`), HTML `$Input=` 입력(`ShowInput`), 시나리오 입력(`ShowScenarioInput`), `ConfirmSaveFortName`·`CancelSaveFortName` — 가 단독 웹 미션에서 닿는지 대본으로 판정했다. **닿지 않는다. 이식 대상에서 뺀다.**

## 근거

- **`$Input=` 사용처:** 영어 대본 82개(`\D\*.english`·`*.html`)와 전역 `tell.english` 의 모든 Tell 섹션 487개를 `createTellWindow`로 열어 지원하지 않는 지시문을 모았다. `$Input=`은 아홉 곳이며 모두 전역 `tell.english`의 길드·관리자 섹션(`adminzone`·`admingettag`·`regguild`·`regguild1`·`guildlogin`·`guildlogin2` 등)이다. 단독 미션·캠페인 대본에는 한 곳도 없다. 같은 점검에서 `$Zsend=`는 두 곳(서버 전송, `zsend`·`sssguildsenddata`)이다.
- **나머지 가지:** Save as·설명·시나리오 입력은 편집기(Edit)에서 맵을 저장할 때 쓰는 입력이고(AGENTS 의 webpj 범위에서 Edit 제외), `ConfirmSaveFortName`·`CancelSaveFortName`은 그 확정·취소다. 단독 미션에서 닿는 유일한 가지는 송금 Send X 숫자 창(`ShowMoney`)이고 E1168 에서 이식·연결했다.
- 채팅 서식 입력 창(Edit Back/Front Format, E1179)은 같은 기반을 쓰는 형제 창으로 이미 연결돼 있다.

## 처리

`RawMenuSaveFort.inc`·`SaveFortNameDialog.cpp` 대응표 행의 상태를 partial → port 로 닫았다(남은 가지가 모두 편집기·멀티플레이 전용이므로 이식 완료로 센다). 인계 3.8절 "점 명령/HTML" 항목도 닫았다. 남은 양은 120단위·30,154줄 → 118단위·29,882줄이다.
