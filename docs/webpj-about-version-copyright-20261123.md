# webpj 정보 ▸ 버전 창의 11.01 패치·Copyright (WB04-E1289, 2026-11-23)

사용자 요구(2026-11-23, [LEFT_JOBS.md 공통](../LEFT_JOBS.md#user-requests-20261123))를 cpppj 1330(`5a6c2d4d2`)에 맞춰 웹에 반영했다. 게임 안 ESC 메뉴 About ▸ Version과 메인 메뉴 Help ▸ Version은 같은 Tell 섹션 `About`을 연다.

## 원문 규칙 (cpppj `ProductPresentation.cpp` `ProductTellBody`)

- 원작 Tell `[About]`(영어 `tell.english`, 한국어 `tell.korean`)은 `(c) 1997 Titanic Entertainment, Inc. and Activision Inc.`와 `{gamemaster}.{gameminor} Patch by Ticonderoga Entertainment.`(한국어 `… 패치: …`)뿐이다. `gamemaster`·`gameminor`는 원본 설정 값 10·78이라 줄은 `10.78 Patch by Ticonderoga Entertainment.`로 읽힌다. 문서에 있던 "11.01이 Ticonderoga 패치로 치환될 수 있다"는 추측은 cpppj 실제 화면에서도 재현되지 않았고 웹에서도 같다(제품 `version` 값만 `v11.01`).
- cpppj는 `About` 본문에 `Copyright 2026 HJOW`가 없을 때 첫 `$Button=` 앞에 `NetStorm 11.01 Patch by HJOW.<br>`(한국어 `NetStorm 11.01 패치: HJOW.<br>`)와 `Copyright 2026 HJOW<br>` 두 줄을 넣는다. 단추가 없으면 본문 끝에 붙인다. 원작 저작권·10.78 패치 줄과 언어별 확인 단추는 그대로 둔다.

## 웹 변경

- `www/assets/product.js`:
  - `ABOUT_SECTION`·`PRODUCT_COPYRIGHT_LINE`(export)·`ABOUT_PATCH_LINES`(영어·한국어)를 더했다.
  - `productTellBody`에 `about`(대소문자 무시) 분기를 더했다. 같은 줄이 이미 있으면 그대로 돌려 두 번 가공해도 한 번만 들어가고, 없는 언어는 영어이다. `game-data.js`의 `presentTellBody`가 이미 `product` 판본에서만 이 함수를 부르므로 원본 대조용(`product: false`)은 원문 그대로이다.
- 일본어 줄(`NetStorm 11.01 パッチ: HJOW。`)은 일본어 지원(13차)을 웹에 넣을 때 더한다(cpppj는 이미 세 언어).
- Credits 본문(`Patch1101.Credits`)의 `Copyright 2026 HJOW` 줄은 그대로이며 두 곳이 같은 문구이다.

## 검증

- `tests/product.test.mjs`에 '버전 창' 시험을 더했다(영어·한국어 삽입 위치, 대소문자 무시, 중복 삽입 방지, 원작·10.78 줄·확인 단추 보존, 단추 없는 본문, 다른 섹션·없는 언어). 기존 Credits 시험의 `About` 불변 단언은 새 동작에 맞춰 이 시험으로 옮겼다. 7건 통과.
- 새 e2e `e2e/about-version.spec.js` 2건 통과(Chromium 4:3): 메인 메뉴에서 실제 `_openTell('About')` 창의 본문에 영어는 원작 줄·`10.78 Patch by Ticonderoga Entertainment.`·`NetStorm 11.01 Patch by HJOW.`·`Copyright 2026 HJOW`가 이 순서로 한 번씩, 한국어는 `10.78 패치: Ticonderoga Entertainment.`·`NetStorm 11.01 패치: HJOW.`·`Copyright 2026 HJOW`와 `확인` 단추가 나온다. 화면 캡처로 창 안에 네 줄이 모두 들어감을 확인했다.
- 변형 확인: About 분기를 끄면 Node 시험 1건과 e2e 2건이 모두 실패하고, 원복하면 통과한다.
- 관련 Node 시험 17개 파일 113건(product·tell-section·korean-tell·korean-help·raw-game-menu-bar·inventory-about 계열 등)과 e2e `game-menu-bar`·`tell-escape`·`language-choice` 포함 11건이 통과했다.

## 대응표·판정

- 대응표 `ProductPresentation.cpp` 행의 base를 `5a6c2d4d`로 올리고 메모에 E1289를 적었다(cpppj 1330 판정 ②, 웹 재반영 완료). `node webpj/tools/port-map.mjs`의 변경 보고에서 이 파일이 빠졌다. 남은 보고는 `KoreanResources.cpp`·`StyleTextBuffer.cpp`(일본어, 13차), `RawGameMenus.cpp`(ESC 한국어 점검), `UberGump.cpp`·`UserInput.cpp`(방향키 카메라)이며 각 항목에서 처리한다.

## 남은 확인

- 원본 게임 화면과의 직접 대조는 해당 없음(원본에 11.01 줄이 없는 추가 요구). 사람 검수 대상은 한국어 문구 `NetStorm 11.01 패치: HJOW.`뿐이며 cpppj와 같다.
