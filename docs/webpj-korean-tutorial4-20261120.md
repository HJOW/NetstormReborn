# 한국어 네 번째 튜토리얼 (WB04-E1109)

네 번째 튜토리얼 `Tutorial4`(전술 전투)의 모든 단계 안내·버튼을 한국어로 옮기고 웹 한국어 묶음에 포함했다. 영어 원문과 실행 Header(Fort `TacticalCombat`, 허용 기술, 시작 돈·기술), 섹션 순서(`A.`~`I.`와 `A1.`·`B1.`·`C1.`·`E1.`·`I1.`), 버튼·설정 명령, 원본 그림 태그(`sunCannon.*`)를 비교해 같음을 확인했다.

## 구현

- [Tutorial4.korean](../webpj/translations/Tutorial4.korean): 제목 `전술 전투`, 단계 제목·안내·인용구·버튼(다음·뒤로·확인·튜토리얼 나가기·다음 튜토리얼)을 번역했다. 건물·유닛 이름은 사용자 지정 용어표와 같고(썬 캐논·썬 디스크 발사기·윈드 제네레이터·골렘·제단·워크샾·신전) 인물 이름은 음역했다. 건설 메뉴는 번역되는 형식(`제단 건설`)으로 썼고, 용어표에 없어 화면에 영어로 나오는 메뉴 항목(`Upgrade`·`Drop`·`Salvage`·`Options / Cursor Snap To Grid`)은 영어 그대로 두었다.
- [prepare-assets.js](../webpj/tools/prepare-assets.js)의 한국어 언어 파일 목록과 [korean-assets.js](../webpj/www/assets/korean-assets.js)의 `KOREAN_LANGUAGE_PATHS`에 `Tutorial4.korean`을 더했고 `node webpj/tools/prepare-assets.js`로 `www/data/d/`에 복사했다. 한국어 언어일 때만 받는다.

## 검증

- `tests/korean-mission.test.mjs`에 Tutorial4 시험을 더했다: 복사본이 번역 원본과 같고, 영어 원문과 Header(제목 제외, 첫 안내 섹션 앞까지 — 이 튜토리얼은 `[Options]`가 없다)·섹션 이름 순서·버튼/설정 명령·그림 태그가 같으며, 한국어 미션 설정에서 `loadFort`·`tutorialNumber`·제목·`A.`·`I1.` 섹션을 읽는다. 아직 번역하지 않은 다음 튜토리얼(Tutorial5)은 영어 대체 경로를 유지함을 새 기준으로 확인한다. 한국어 묶음 목록과 외부 접속 감사도 통과한다.
- 실제 Chromium에서 이 미션을 끝까지 진행하는 화면 확인은 하지 않았다(미션 자연 진행은 사용자 확인 범위).
