# webpj 내장 설명 HTML 부모 HtmlBodyGump와 EmbeddedTell 원본 관측 (WB04-E466, 2026-11-15)

기준선 `1f00a25a`의 `HtmlBodyGump.cpp/.h`를 `www/ui/html-body-gump.js`로 옮겼다. 일반 Tell 팝업 부모(`HtmlWindowGump`)와 구별되는 **내장 설명용 HTML 부모**로, 두 8192BYTE 영역(본문/링크)과 실제 다중 줄 자식의 생성/갱신/부착을 맡는다. 이 단위에서 E437이 "cpppj에도 직접 시험이 없다"고 적었던 `EmbeddedTell`의 원본 관측 `embedded-tell-x86.tsv` 82행이 처음으로 연결됐다(E465 감사의 두 번째 발견).

## 이식한 것

- `HtmlBodyGumpNode`(공통 기본 필드 + `text`(+a0 현재 다중 줄 자식) + `body`/`links` 8192BYTE), `HtmlBodyGumpState`(화면 크기·종류 BYTE(부호 확장)·다중 줄 기본 색), `HtmlBodyGump`:
  - `construct`(46c9c0): 기본 생성 `(token,0,0,0,1)` 뒤 가상 표 `0x50bd38`·종류·자식 0·두 영역의 **첫 바이트만** 0·화면 절반 크기(정수 나눗셈, 0 쪽 절삭)를 기본/배율 크기에 각각 쓴다. 나머지 영역 바이트는 보존한다.
  - `clear`(46ca50), `parse`(46ca70: 두 첫 바이트 0 → 파서 출력이 없을 때만 두 배열/8192 한도 설치 → 순회), `setText`(46cac0: 이전 자식을 Kill하되 새 생성 전까지 토큰 보존 → 새 다중 줄(0,0,5,색) 생성·부착·StyleText·scroll 0·Slider 1·scrollable 1 → 부모에 Push 표시(8)가 없을 때만 Push 뒤 같은 현재 x/y로 재부착), `setHtml`(46cc60: 두 첫 NUL → 본문/링크에 직접 쓰는 기본 파서 → 본문 첫 NUL 앞으로 `setText`, 본문에 NUL이 없으면 `RangeError`), `fit`(46cbc0: 자식 Push → 부모 기본 폭에서 가로 여백을 뺀 값과 같은 높이를 기본/배율 크기에 저장 → `(inset,0)` 부착), `attach`(46cc30: 기본 부착 뒤 여백 0으로 다시 맞춤).
  - 각 효과 뒤에 노드/기본 필드를 다시 읽고, 선택된 실제 처리기가 없으면 `Error`(미연결)다.
- 기존 앱의 `tell-window.js`(partial)는 건드리지 않았고, 앱 Tell 창 결선은 RawMenuControls 허브 몫이다.

## 기준선 뒤 판정

기준선 뒤 cpppj 변경 없음(base `1f00a25a`).

## 검증

- **원본 관측 82행**(`embedded-tell-x86.tsv`, 두 x87 정밀도, 입력 mask로 현재 미션/fallback/missing 조회·`SetHtml` 직접 호출·이전 자식·Push 표시·serial 변이): 반환 serial·부모 flags/가상 표/serial·자식 유무·**본문/링크 8192BYTE 전체 지문**(NUL 뒤까지)·호출 순서(`mission/fallback/missing/new/parse/allocateText/constructText/attach/styled/scroll/slider/scrollable/push/kill`)가 첫 실행에서 모두 일치했다. E437 `showEmbeddedTell`의 조회 순서와 serial 반환이 원본과 같음을 이 관측이 확인한다. manifest `embedded-tell`로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.
- 독립 시험 4건: 생성(가상 표·종류 부호 확장·영역 보존·정수 절반 크기), 자식 수명(Kill 순서·새 생성 전 토큰 보존·Push 표시가 있으면 재부착 안 함·`clear` 두 번), 맞춤/부착/파싱/본문 한도, 기본 상태/생성 인자/NUL 앞 본문/빈 본문/여백 0 맞춤.
- 변형 확인: 자동 변형 56개 중 첫 라운드 생존 16개(상태 기본값, 기본 생성 인자, 부모 재부착 인자, 맨 앞 NUL, 영역 첫 바이트 초기화, 빈 본문, 부착 뒤 여백)를 시험 보강 뒤 모두 잡았고 남은 1개는 동등 변형(`index <= end`가 읽는 NUL은 `setText`가 다시 잘라냄)이다. 첫 실행에서 어긋난 곳은 시험 기대값 두 곳뿐이었다(Push 표시 8이 기본 생성에서 이미 켜져 있음, 맞춤 호출의 부착 모드 값). 모듈은 고치지 않았다.

## 같은 단위의 부수 산출물

`docs/webpj-fixture-audit-20261115.md`: E465에서 발견한 "직접 시험 없음" 오기를 전수 감사한 결과로, 아직 연결하지 않은 cpppj fixture/시험의 이월 목록이다(LEFT_JOBS 3.8절).

## 남은 것

앱 Tell 창의 `GameData.findTellSection`/기존 `tell-window.js` 교체와 `EmbeddedTell`의 실제 소비자(RawMenuControls `ShowEmbeddedTell`)는 허브 단계다.
