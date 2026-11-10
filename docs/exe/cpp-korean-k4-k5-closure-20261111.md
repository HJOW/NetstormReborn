# K4 잔여 3건 교정과 K5 마감 (제품 1281, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 재디컴파일 없음).

## 1. K4 잔여 교정

| 잔여 | 원인 | 교정 | 확인 |
| --- | --- | --- | --- |
| ① 옵션 메뉴 끝 `Pass Server Diagnostic` 영어 잔재 | 번역표에 키 없음(메뉴 코드는 후행 공백이 있는 `"Pass Server Diagnostic "`을, 호스트 옵션 목록은 공백 없는 키를 사용) | `assets/lang/korean/xlat.korean`에 두 키 → `서버 진단 통과`. 번역표 키의 후행 공백은 유지되므로 두 키 모두 필요 | `KoreanTranslation_RepositoryFileIsFoundParsedAndEncodable`에 두 키 단언 추가 |
| ② 한국어 크레딧 창 제목이 `NetStor`로 잘림 | 좁은 메뉴 창에서 `NetStorm 크레딧`처럼 영문+한글이 섞인 H2 제목이 줄바꿈되면서 첫 줄 끝 글자가 잘린다(영문 단독 `NetStorm Credits`와 넓은 창의 `NetStorm 10.72 패치 크레딧` 제목은 정상). 혼합 문자열의 H2 폭 계산/줄바꿈 원인은 이번에 확정하지 못했다. | 해당 창의 한국어 제목만 `크레딧`으로 줄여 한 줄에 맞췄다(`assets/lang/korean/d/tell.korean`의 `[Credits]`). 다른 제목은 바꾸지 않았다. | 실제 화면에서 `크레딧` 한 줄 표시(`extracted/cpp-resume-1278/cr2/credits-list.png`) |
| ③ 맵 목록 이동 행 문구 | `~o< Previous 50 (…of N)`/`~oNext 50 > (…)`가 코드에 영어로 고정 | `TellMapMenuHooks::translate` 추가, 고정 단어 `Previous`/`Next`/`of`만 번역(키 3개 추가: `이전`/`다음`/`/`). 번역 장치가 없으면 영어 문구가 바이트 단위로 그대로다 | 432맵 목록 첫 화면에서 `다음 50 > (51-100 / 432)` 표시. 새 단위 시험 `MapList_ScrollRowsTranslateFixedWords` |

## 2. 새로 발견한 번역 결함: 11.01 패치 크레딧

크레딧 목록의 세 번째 항목 `Netstorm 11.01 Patch Credits`와 창 본문이 한국어 모드에서도 영어로 나왔다(1276 화면에서 확인, 잔여 목록에는 없었음). 코드가 만드는 항목이라 `tell.korean`에 번역이 없었다.

- `ProductTellBody(target, original, korean)`: 한국어이면 목록 항목을 `NetStorm 11.01 패치 크레딧`으로 덧붙이고, `Patch1101.Credits` 본문(제목·버전·HJOW 개발·AI 사용·버튼 `뒤로`/`취소`)을 한국어 고정 본문으로 제공한다. 영어는 기존 표기·본문 그대로다.
- 호출자 `UberGump::TryTell`이 현재 언어가 한국어인지(`kKoreanLanguageNumber`) 전달한다.
- 새 시험 `KoreanIntegration_ProductCreditsEntryAndBodyFollowLanguage`(영어 표기 유지·한국어 표기·중복 방지·본문 제목/HJOW/AI 포함·다른 대상 불변). 실제 화면(`cr1/credits-list.png`, `credits-1101.png`)에서 한국어 항목과 본문을 확인했다.

## 3. 같은 날 발견해 고친 소스 결함(주석이 코드를 삼킴)

`RawMenuHtml.inc`에서 내가 줄 중간에 `//` 설명을 넣어 같은 줄 끝의 `};`를 주석으로 만든 실수를 컴파일 오류(Impl 클래스 선언 연쇄 오류)로 발견해 주석을 줄 끝으로 옮겼다. 같은 유형의 이전 사례(`hooks.setOwner`)는 1278에서 교정했다.

## 4. K5(마감) 판정

- 번역표·본문: `netstorm_korean_text_tests` 34건, `netstorm_user_data_root_tests` 12건, `netstorm_solo_map_list_tests` 6건 모두 통과.
- 배포 정합성: 1280의 ZIP 바이트 검증(1423 일치)과 푼 배포본의 한국어 실행(외부 D2Coding, 메인 8버튼, Edit 목록, 크레딧 3항목)을 마쳤다. 새 xlat/`tell.korean` 변경은 `tools/verify_cpp_package.py`가 저장소 원천과 같은 해시로 확인한다(최종 출시 직전 재실행).
- 제외: 자연 완주·공격·희생·승리, MP 실행은 하지 않았다.

## 5. 남은 것(사용자 확인 필요)

- K2: 물리 IME(조합 중 표시/후보창)·OS 클립보드 붙여넣기는 사람 입력이 필요하다.
- K3: `.korean` 본문 전체 문장 교정은 사람 검수다(표본 확인만 했다).
- 영문+한글 혼합 H2 제목의 줄바꿈/잘림 근본 원인은 남아 있어 다른 좁은 창에 새 혼합 제목을 만들면 같은 증상이 나올 수 있다.
