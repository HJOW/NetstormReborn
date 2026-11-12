# webpj Client 수명 전역 — Fort 이름·CoreData (WB04-E331, 2026-11-12, 4-D A 구획)

3.1절 4-D A 구획의 client **ClientFortGlobals**(기준선 51줄 = `.cpp` 27 + `.h` 24, 기준선 뒤 변경 없음)와 **ClientCoreDataGlobals**(87줄, 변경 없음)를 `core/client-fort-globals.js`·`core/client-core-data-globals.js` 로 옮겼다. 둘 다 규칙 본체가 아니라 같은 Client 가 Session·메뉴보다 오래 쥐는 원문 전역의 **소유자**이며(앞 단위 E325 와 같은 방식), 소비자는 값을 복사하지 않고 이 객체의 배열/상자를 **빌린다**.

## 1. 규칙

- **ClientFortGlobals:** 현재 Fort 이름(원문 0055b490 이 가리키는 256BYTE)과 예약 Fort 이름(0055b494 의 별개 256BYTE)을 서로 복사하지 않는 두 `Uint8Array`(`currentBytes()`/`pendingBytes()`)로, `current()`/`pending()` 은 **첫 NUL 앞까지** 한 글자 = 한 바이트(latin1)로 읽는다(NUL 뒤 미작성 바이트는 읽지 않고, NUL 이 없으면 빈 문자열로 가장하지 않고 `RangeError`). 파일 키는 `SoloMapFileSpec`(005327ac, 9BYTE, 기본 `fortSpec`+NUL, `selectSolo()` 로 `SOLOSpec`, `selectFort()` 로 복구)의 `bytes` 가 키가 바뀌어도 같은 배열이며, `selectFileSpec()`(005327e0 포인터 지정)은 내용을 바꾸지 않고 `selectedFileSpec()` 이 경로 작성 위치에서 현재 키를 다시 읽는다. 목록/설정 dirty(00542678, 보호 PE 최초 1)와 선택 모드(005cae84, 최초 0)는 `catalogDirty()`/`selectionMode()` DWORD 상자다. 접근자는 호출마다 같은 객체를 돌려준다.
- **ClientCoreDataGlobals:** 현재 포인터 슬롯 `current`(00595350, 최초 `null`, 읽기/쓰기 속성)·활성 `active`(00595348, 최초 0, DWORD 폭)·lastKnown 10BYTE `cached()`(005953f0)·9기록 장부 `records()`(144BYTE, 00595360)·현재 Player 물리 뷰 슬롯 `players`(0059534c, 소유하지 않음, 최초 빈 뷰)를 보관하고, 새 Client 에서만 장부 9기록과 lastKnown 을 CRT 순서대로 한 번 구성한다(Session 생성/파괴가 다시 초기화하지 않음). `storeMetadata(owner, subscriber, core)` 는 0048f350 의 값 인자를 같은 장부에 쓴다. `borrow(player, metamap)` 는 `player`(`PlayerWorldState`)의 현재 `templateOwner`/`battleOwner`/`playerKnown` 속성과 이 소유자의 `active`/`current`/`players` 슬롯, 고정 `cached`, 읽는 시점의 `metamap`(상자/함수/수)을 사본 없이 잇는 살아 있는 묶음이고, `read`/`select`/`restore` 는 기존 `raw-player-core-data.js`(E3)의 `readPlayerCoreData`/`selectPlayerCoreData`/`restorePlayerCoreData` 를 이 묶음과 같은 9기록 장부(`{ bytes, origin: 0 }`)로 부른다. 숨은 reset 이나 사본 동기화는 없다.

## 2. 웹 범위와 기준선 뒤 변경

- 두 파일 모두 기준선 뒤 cpppj 변경이 없다(`git log 1f00a25a..HEAD -- <파일>` 의 추가 줄 0, 3.6절 ①).
- **ClientFortGlobals 의 `Catalog()`** 는 50행 목록 캐시 `SoloMapListState`(단독/Edit 맵 목록, `exclude-edit` 인 `SoloMapCatalog`/`RawSoloMapList` 소유)와 dirty 를 함께 돌려준다. 웹은 목록을 옮기지 않으므로 이름·설정과 같은 dirty DWORD 한 칸만 `catalogDirty()` 상자로 둔다. 파일 이름 장치(`SetPendingFortName` 등 Edit 의 이름 변경)도 Edit 제외라 만들지 않는다.
- ClientCoreDataGlobals 의 `Restore(edition, …)` 의 `edition` 인자는 10.78 전용이라 받지 않는다.

## 3. 검사 (`tests/client-fort-core-globals.test.mjs` 11건)

- **cpppj 시험 이식:** `FortNameDevices_ClientBuffersAndPendingPhysicalCopy` 의 Client 버퍼/키 부분(초기 빈 이름·dirty 1·서로 다른 버퍼·현재/예약 독립·`SelectSolo` 뒤 키 주소 고정)과 `ClientCoreRecords_OneOwnerHasSeparateRecordsAndLastKnown`·`ClientCoreRecords_CoreSelectionDoesNotResetSavedRecords`(별개 장부/lastKnown, padding 보존, 선택 0 이 장부를 초기화하지 않음, 새 Client 는 새 장부).
- **웹 계약:** 이름의 NUL 경계(미종료 거부, ANSI 상위 바이트)·255글자 이름·키 복구와 선택 포인터·dirty/모드 DWORD 폭, 비영 선택의 Player 안 10BYTE 별칭과 살아 있는 borrow(템플릿/전투 owner 구분·active/current 쓰기 전파·metamap 상자/함수/수), metamap 시 lastKnown 캐시, 범위 밖 Player 물리 거부, `read` 의 진단 복귀 뒤 현재 재조회, `restore` 의 missionPost 관문·원격/로컬 owner·저장 게임의 저장 BYTE 되돌림과 Chat 재초기화.
- 첫 실행의 실패 1건은 시험 쪽 기대값 오류였다: 저장 게임 경로의 `restore` 는 Core 의 여섯 번째 BYTE(Player+0x71 저장 BYTE)를 복사 전 Player 값으로 되돌리는 것이 원문 동작인데 복사 결과를 그대로 기대했다. 모듈은 고치지 않았다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 45개(Fort 17·CoreData 28)를 돌려 첫 실행에서 CoreData 5개가 남았다(초기 null/빈 Player 뷰, 두 초기화 호출, 템플릿 owner 접근자). 시험을 보강(새 소유자의 초기 현재/활성/Player 뷰, borrow 의 템플릿 owner)한 뒤 **4개가 남았고 모두 동등**하다: ① `ClientCoreDataGlobals` 생성자의 `initializePlayerCoreRecords`/`initializePlayerCore` 호출 제거 2개 — JS 배열이 이미 0 이고 두 초기화는 0 을 쓰는 것뿐이다(C++ 는 PE 가상 zero 뒤 CRT 가 0 을 쓰는 순서를 보존) ② `SoloMapFileSpec.#write` 의 NUL 쓰기 제거 — 두 키가 모두 8글자이고 아홉째 바이트는 이미 0 ③ `selectFileSpec` 이 다른 키를 가리키는 변형(`selectedFileSpec` 이 항상 같은 키를 읽음) — 웹에는 키 객체가 하나뿐이라 선택 포인터가 항상 같은 객체다.
- **앱 수명:** 아직 Client 가 이 객체들을 만들지 않는다(PORT_MAP session 0). 4-D 의 `Client` 조립이 한 번씩 만들고 `ClientPlayerRecords.bindCoreData` 가 `players` 슬롯을 같은 소유자에 잇는다.
- 3.1절 4-D 는 0/24 이다(이 단위는 A 구획의 두 항목 일부; `ClientCoreDataGlobals` 는 A 구획 다섯째 줄에 `ClientSpGlobals`·`ClientNsInfoGlobals` 와 묶여 있어 그 줄은 아직 체크하지 않음). 다음: A 구획 `ClientPlayerRecords`·`ClientPlayerRuntime`.
