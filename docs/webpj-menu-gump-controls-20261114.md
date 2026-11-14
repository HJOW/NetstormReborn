# webpj controlled 메뉴 항목 작성 래퍼 MenuGumpControls (WB04-E429, 2026-11-14)

RawMenuControls 선행 **MenuGumpControls 242줄(.cpp/.h, partial→port)**을 새 모듈 [`www/ui/menu-gump-controls.js`](../webpj/www/ui/menu-gump-controls.js)로 이식했다. 기준선 `1f00a25a` 전체 소스를 읽었다([정적 복원 문서](exe/cpp-menu-gump-controls.md)). **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손으로 계산한 값, 실제 `MenuGump`/`MenuGumpLayout`/기본 생성 저장소 연결이다. 원본 직접 관측 수치로 세지 않는다. CD 10.72의 서식 없는 전달·명령 0 처리와 CD 전용 두 helper(`CopyChildTextContextCd`, `ClearSelectionUntilVisibleChildCd`)는 옮기지 않았다.

## 기준선 뒤 변경 (3.6절)

기준선 뒤 변경은 없다.

## 구현

- **공통 규약**: 모든 래퍼는 먼저 현재 controlled 메뉴를 읽고(`menu.current()` → 공유 노드 확인; 현재 칸이 비어 있으면 `include`가 false여도 거부) 설정 조회 뒤에 **다시 현재 칸을 읽어** 실제 `layout.append`에 보낸다. 설정 조회가 현재 controlled 칸을 바꿔도 마지막에 읽은 칸이 쓰인다.
- **색 접두**: 설정의 전경 → 배경 순서로 `readInt(키, {value})`를 두 번 부르고(누락 키는 0), 활성 항목(또는 강제 색)은 `~[C.<전경 signed 십진수>]~[B.<배경>]~E<원문>`을 만든다. 색 조회는 활성/비활성과 무관하게 항상 일어나지만 비활성은 접두 없이 **`%%`만 한 `%`로 해석**하고 다른 `%`(끝의 `%` 포함)는 원본이 인자를 넘기지 않은 무효 스택 입력이라 `TypeError`로 거부한다. 입력은 첫 NUL까지만 쓰며 결과가 256바이트 이상이면 `RangeError`(원본 지역 버퍼)다. 비활성 항목은 이어서 레이아웃이 `~*~x` 접두(4바이트)를 붙여 StyleText 버퍼 한도(256)를 따로 검사한다.
- **`addText`/`addChecked`**: 일반 항목은 문자열·스타일·명시 인자(`0x10480`) 비트에 명령이 0이 아닐 때만 명령 비트(1)와 비활성(`0x200`)을 더한다. 체크 항목은 명령이 0이어도 명령 비트를 켜고 체크(4) 비트를 더하며 색 키는 `MenuCheckedColor/BGColor`다. `include`가 false면 설정을 읽지 않고 생략한다.
- **`addTitle`/`addHeader`**: 제목은 `~3~E`, 머리는 `~E` 뒤에 `~[C.]~[B.]` 제목/머리 색, 본문, `~.`를 붙인다. 장식(`0x800000`) 메뉴는 현재 `title` 토큰에 다중 줄 파서 `parse`만 호출하고(제목 토큰 0이면 거부, 항목 추가/Reflow 없음), 일반 메뉴는 메뉴 색으로 한 번 더 감싸 비활성 항목(`0x10480|0x200`, 명령/인자/자식 0)으로 추가한다.
- **`addSubmenu`/`addSeparator`**: 하위 메뉴는 `MenuSubColor/BGColor` 색으로 `0x10400` 항목(비활성/체크 비트 선택)을 만들고 자식 연결과 `0x20` 비트는 실제 `append`가 한다. 구분선은 `0x10010`에 빈 문자열로 `append`하며 선두/연속/40개 한도는 `append`가 처리한다(명령/인자는 쓰지 않는다).
- **`noMenus`/`bindMenuGumpControlConfig`**: 메뉴 생성/소멸 개수 DWORD가 0인지만 본다. 설정 연결은 `GameData.readInt(키, 현재)`의 값만 홀더에 쓰고(하위 DWORD signed) 변경 여부는 버린다.

## 검사와 발견

새 `tests/menu-gump-controls.test.mjs` **14건 통과**(공용 장면 `tests/menu-gump-support.mjs`, 설정은 값 표 대역, 제목 파서는 호출 기록 대역).

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 2종 4건):** 비활성/하위 메뉴 항목도 색을 읽는다는 점(원본 `Colors` 호출이 활성 분기보다 앞이다), 비활성 한도가 포맷 버퍼(256)가 아니라 `~*~x` 접두를 붙인 레이아웃 버퍼에서 먼저 닿는다는 점을 바로잡았다.

**변형 확인:** 연산자/정수 상수 변형 58개 중 49개를 잡았다(처음 생존 13개 → 선두 NUL·NUL 뒤 긴 입력·기본 인자·제목 항목의 인자 0·짝수 설정 값 보강). 남은 9개는 동등 변형이다 — 끝 `%`는 `undefined !== '%'`로 어느 경우든 거부되는 경계, 명령 비트(1)가 이미 켜진 체크 항목의 `(checked ? 4 : 0)`·`(enabled ? 0 : 0x200)`, 명령/인자 비트가 없는 항목·구분선·하위 메뉴가 전달하는 쓰이지 않는 명령/인자 값이다.

## 남은 경계

- `MenuGumpDraw`·`MenuGumpInput`·`MenuGumpWidgets`와 `RawMenuControls` 연결. 앱 메뉴(`www/ui/menu-list.js`·`popup-menu.js`)는 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
