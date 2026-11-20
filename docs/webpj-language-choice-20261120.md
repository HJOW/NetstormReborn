# 옵션 메뉴 Language(언어 선택) (WB04-E1095)

메인 메뉴 Options와 ESC 게임 메뉴 Options의 맨 앞에 `Language` 하위 메뉴(English·한국어)를 더했다. 이전에는 언어를 `?lang=ko` 주소 매개변수나 브라우저 언어로만 정할 수 있었다. cpppj 6차 한국어의 `Language` 하위 메뉴(원문 SetLanguage 명령)에 대응하는 웹 판본이다.

## 구현

- **선택과 적용.** 선택은 Tell 대상 `Patch1101.SetLanguage(n)`(1 English, 2 한국어)로 가고 `_openTell`이 창을 열지 않고 설정 `language`(0 자동·1·2)에 저장한 뒤 주소에서 `lang` 매개변수를 같은 출처 기록 교체(`history.replaceState`)로 지우고 페이지를 다시 불러온다(주소 이동은 외부 접속 감사가 금지한다). 웹은 한국어 글꼴·번역 묶음을 시작할 때 적재하므로 다시 불러와 새 언어로 시작하는 쪽이 가장 단순하고 안전하다. 현재 언어를 다시 고르면 아무 일도 없다. 이동 방식은 `options.navigate(주소)`로 바꿀 수 있다(검사용).
- **시작 언어.** `options.language ?? ?lang ?? 저장된 선택 ?? 브라우저 언어` 순서다. 저장된 선택은 `netstorm.settings`에서 직접 읽는다([language-choice.js](../webpj/www/platform/language-choice.js)의 `savedLanguage`; 설정 객체가 만들어지기 전의 시작 단계라 JSON을 직접 해석하고 오류는 무시한다).
- **항목 이름.** 항목은 번역하지 않는다. 영어 화면에는 한글 글꼴(한국어 묶음)이 없어 한국어 항목을 `Korean`으로, 한국어 화면에서는 `한국어`로 표기한다(영어 항목은 항상 `English`). 제목 `Language`는 한국어로 `언어`다. 현재 언어에 체크가 붙는다.
- **ESC 메뉴.** `RawGameMenus.options()` 맨 앞에 `languages` 장치로 같은 하위 메뉴를 만든다.
- **설정.** `language`를 설정 키(정수, 기본 0)로 올려 저장한다. 설정 키가 14개가 됐고 Reset All은 같은 저장소 키를 지우므로 선택도 초기화된다.

## 검증

- 새 `tests/language-choice.test.mjs` 4건(선택지·Tell 대상·저장된 선택 읽기와 오류 무시·메뉴 항목). 설정 키·메뉴 구조 변경으로 `popup`·`korean`·`frame-rate-option` 시험의 기대를 갱신했다.
- 새 `e2e/language-choice.spec.js`(Chromium): 현재 언어(English) 선택은 이동하지 않고, Korean 선택은 설정 `language=2`를 저장하고 `lang`을 뺀 주소(다른 매개변수는 유지)로 이동한다; 저장된 한국어 선택은 주소 매개변수 없이 한국어로 시작해 `옵션 ▸ 언어`에 `한국어`가 체크된다. `frame-rate.spec.js`는 ESC 메뉴 Options의 `Language`·`Frame Rate` 순서를 확인한다.

## 남은 경계

일본어 등 추가 언어(13차)는 선택지 표와 번역·글꼴 묶음을 더해 같은 경로로 연결할 수 있다. 영어 화면의 `Korean` 표기는 한글 글꼴이 항상 적재되는 구조로 바뀌면 `한국어`로 바꿀 수 있다. Difficulty(11차) 하위 메뉴는 남아 있다.
