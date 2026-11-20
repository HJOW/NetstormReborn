# 한국어 여섯 번째 튜토리얼 (WB04-E1111)

여섯 번째이자 마지막 튜토리얼 `Tutorial6`(순수한 힘!)의 모든 단계 안내·버튼을 한국어로 옮기고 웹 한국어 묶음에 포함했다. 이로써 튜토리얼 여섯 개가 모두 한국어로 제공된다. 영어 원문과 실행 Header(Fort `RawPower`, 허용 기술, 시작 돈·기술), 섹션 순서(`A.`~`F.`와 `A1.`~`A3.`), 버튼·설정 명령, 원본 그림 태그(`windBalloon.*`·`mana.*`·`windArcher.*`·`sunFence.*`)와 치환 자리표시자 `{global.crossbowCost}`를 비교해 같음을 확인했다.

## 구현

- [Tutorial6.korean](../webpj/translations/Tutorial6.korean): 제목 `순수한 힘!`, 단계 제목·안내·인용구·버튼을 번역했다. 건물·유닛 이름은 사용자 지정 용어표와 같다(에어 쉽·윈드 워크샾·윈드 제네레이터·크로스보우·썬 바리케이드·썬더 캐논·훨리베이스). 마지막 단계(`F.`)에는 다음 튜토리얼 단추가 없고 `튜토리얼 나가기`만 있다. 용어표에 없어 영어로 나오는 `About` 메뉴 항목은 영어 그대로 두었다.
- [prepare-assets.js](../webpj/tools/prepare-assets.js)의 한국어 언어 파일 목록과 [korean-assets.js](../webpj/www/assets/korean-assets.js)의 `KOREAN_LANGUAGE_PATHS`에 `Tutorial6.korean`을 더했고 `node webpj/tools/prepare-assets.js`로 `www/data/d/`에 복사했다. 한국어 언어일 때만 받는다.

## 검증

- `tests/korean-mission.test.mjs`에 Tutorial6 시험을 더했다(복사본·Header·섹션 순서·명령·그림 태그·미션 설정과 `A.`·`F.` 섹션). 이제 번역하지 않은 미션의 영어 대체 경로는 캠페인 첫 미션 `TheWarBegins`로 확인한다. 한국어 묶음 목록과 외부 접속 감사도 통과한다.
- 실제 Chromium에서 이 미션을 끝까지 진행하는 화면 확인은 하지 않았다(미션 자연 진행은 사용자 확인 범위).
