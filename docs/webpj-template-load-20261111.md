# webpj Template manager 생성자·Load와 Client 이름표 소유자(RawTemplateLoad · ClientTemplateSectionNames) — WB04-E281 (2026-11-11, 4-B)

3.1절 4-B 의 **RawTemplateLoad**(81줄)와 client **ClientTemplateSectionNames**(E272 이월)를 `core/raw-template-load.js` · `core/client-template-section-names.js` 로 옮겼다. 두 단위 모두 기준선(`1f00a25a`) 뒤 변경이 없다(3.6절 ①). 게임·브라우저 실행과 디컴파일은 하지 않았다.

## 1. RawTemplateLoad (`raw-template-load.js`)

- **`constructTemplateDataManager(manager, fileName, global)`(004bcc60):** manager276 의 지정 DWORD 만 쓴다: `+0` 가상표 `0x514690`, `+4`·`+8`·`+0x10c` = 0, `+0x110` = `0xffffffff`, 005c89c0 의 같은 4BYTE 전역 = 0, `+12` 부터 첫 NUL 까지의 이름과 종료 NUL. 이름 뒤 공간과 manager 밖 BYTE 는 건드리지 않는다. 256BYTE 이상(NUL 자리 없음)이거나 manager 가 276 미만, 전역이 정확히 4BYTE 가 아니면 쓰기 전에 거부한다. 파일명은 문자열(글자마다 한 BYTE, 0~255)이거나 `Uint8Array` 다.
- **`templateDataManagerFileName(manager)`(가상 +0, 004bcca0):** 같은 manager `+12` 의 NUL 종료 이름을 **살아 있는 뷰**(`Uint8Array` 부분 배열)로 빌린다(이름이 바뀌면 뷰도 같은 BYTE 를 본다). NUL 이 없으면 거부한다.
- **`loadTemplateFile(header, fileName, hooks)`(004bf550):** 20BYTE 헤더만 받는다. 헤더 `+0` 에 기존 manager 가 있으면 같은 이름이든 다른 이름이든 할당/읽기/해제 없이 이름만 비교(다르면 진단)하고 돌아간다. 빈 헤더면 `+4` 를 0 → 할당(276) → 비영이면 생성자 → 반환값(EAX)을 헤더 `+0` 에 설치 → 읽기 전용 사전 설정(`+8`)이 0 일 때만 판정(0041af80)해 `+8` 에 1/0 정규화 → `+16` 을 0 → 파일 읽기(0044d980)를 실행한다. 읽기 성공이면 헤더 `+0` 을 **다시 읽어** 비영이고 현재 슬롯일 때만 창 제목을 갱신한다. 실패면 읽기 뒤 헤더 `+0` 을 다시 읽어 비영이면 base 소멸 → 저장 포인터 free 를 부르고 `+0` 만 0 으로 쓴다. 헤더의 다른 필드와 파일명 뒤 바이트는 초기화하지 않는다. 장치(hooks)는 도달한 분기에서만 요구한다.

## 2. ClientTemplateSectionNames (`client-template-section-names.js`)

Client 수명의 005c8aa4 이름표 512BYTE 한 개를 파일 reader 와 신규 manager 초기화가 함께 빌린다. 생성자는 할당도 쓰기도 하지 않고, 처음 `names()` 에서만 E19 의 `getTemplateSectionNames` 를 실행해 512BYTE 한 개를 할당하고 277BYTE 를 쓴다(이후는 같은 배열 참조). 표의 끝 235BYTE 는 원문이 쓰지 않는 미정 영역이라 건드리지 않는다(기본 할당은 0 채움, 검사에서 알려진 채움 값으로 불변 확인). 토큰은 최초 0 · 성공 1 이고 할당이 null 이면 토큰 0 을 보존하며 다시 부르면 다시 할당을 시도한다. 배열이 정확히 512BYTE 가 아니면 거부한다. 소비자 `CampaignTemplateStorage`·`SoloMapCatalog` 는 4-D 몫이다.

## 3. 검사 (`tests/template-load.test.mjs` 10건)

- cpppj `TemplateLoadTests` 5건을 호출 순서 기록(trace)까지 그대로 옮겼다. 원 파일 머리 주석은 "사용자 중단에 따라 실행하지 않는다" 이지만 기존 빌드로 `netstorm_tests.exe --filter TemplateLoad_` 를 돌려 **5건 모두 통과**함을 확인했다(게임 실행 아님).
- cpppj `DataManagerInitializeTests` 의 `ClientTemplateSectionNames_LazyMutableAndUnknownTail`(최초 512 할당·같은 배열·SNART 제자리 변환·미정 끝 불변)·`AllocationZeroIsFailure` 를 옮기고 소유자 계약(기본 할당·두 소유자 독립·잘못된 할당/장치 거부)을 더했다. `TemplateSectionNames_Native12`(12행)는 E19 가 이미 대조했다.
- 계약 검사: 생성자 입력 모양 거부(쓰기 전), BYTE 배열·Latin-1 이름, 분기별 요구 장치(기존 파일은 fileName/sameName/wrongName 만, 새 파일은 allocate, 제목 장치는 현재 슬롯일 때만, 읽기 실패 소멸/반납), 헤더 크기 거부(장치 호출 전), NULL 할당이 생성자를 부르지 않고 0 포인터로 이어짐.
- 변형 확인 `raw-template-load.js` 29개·`client-template-section-names.js` 9개. 처음에 템플릿 변형 4개·이름표 변형 3개가 살아남았고, 헤더 크기 검사와 NULL 할당 분기는 시험을 더해 잡았다. 남은 5개는 **동치 변형**이다: 이름 끝 NUL 쓰기(`Uint8Array` 가 `undefined` 를 0 으로 저장), 읽기 전용 정규화(false 일 때 이미 0), 이름표 할당 크기 검사·서식 범위 검사(`getTemplateSectionNames` 가 항상 512 · 0..19 만 호출해 닿지 않는 방어), buffer 토큰 검사(없어도 상위가 같은 RangeError 를 던짐).

## 4. 남은 경계

- 실제 manager 읽기(`readFile` = 0044d980)와 base 소멸(`lifetime`)은 훅이며 각각 RawDataManagerImage·`raw-data-manager-lifetime.js` 가 맡는다. 헤더 9칸 정리는 E123 `raw-template-lifetime.js`.
- 3.1절 4-B 는 11/30 이다. 다음은 **RawDataManagerImage**(142줄)와 client `DataManagerFileRead` 이다.
