# webpj 일반 자산 Draw 분배 조건 RawPlainAssetDraw·RawPlainUnitCapture·RawPlainWorkshopCapture (WB04-E372, 2026-11-13, 5-A 착수의 첫 단위)

이월 부품(6-P·7-P1~P3)이 모두 끝나고 4-D 는 선행 대기라(0.1절), 3.1절 규칙대로 허브가 요구하는 client 소형 단위를 5-A 쪽에서 앞당겨 연다. 5-A 의 `o/` 4단위 중 세 개 — **RawPlainUnitCapture**(31줄)·**RawPlainWorkshopCapture**(30줄)·**RawPlainAssetDraw**(35줄; 기준선 집계는 각각 29·28·33) — 를 `core/raw-plain-unit-capture.js`·`core/raw-plain-workshop-capture.js`·`core/raw-plain-asset-draw.js` 로 옮겼다. 세 파일 모두 기준선 뒤 cpppj 변경이 없다(`git diff --numstat 1f00a25a HEAD` 0 — PORT_MAP base `1f00a25a`). 소비자(`RawCampaignFactoryDisplay`·`RawCampaignSession` 의 Draw 분배)는 아직 웹에 없어 앱 수명에는 연결하지 않았다.

## 1. 규칙

- **공통:** 슬롯 길이가 50 미만이면 거짓. 그 밖에는 TYPE 바이트(+10)와 비정렬 little endian 가상표 DWORD(+0, 낮은 네 바이트만, 부호 없음)가 표의 한 쌍과 **둘 다** 일치해야 참이다. 타입 번호만 맞거나 가상표 주소만 맞는 슬롯은 일반 Draw 가 아니다(같은 주소를 공유하는 다른 TYPE·타입만 같은 미복원 가상 표 거부).
- **유닛(`isPlainUnitCapture`):** 열세 쌍 — 70/508cc8·73/50a1e8·111/5040c0·133/50e568·135/503e50·137/503d80·138/50a050·141/5040c0·144/50a388·145/50e738(애시드)·146/504198·156/503f20·158/50f210. 보호 PE 에서 `+78=4acfe0`·`+a0=41c0f0`·`+a4=41c100` 을 각각 확인한 표이며 111 과 141 이 가상표를 공유한다.
- **워크샾(`isPlainWorkshopCapture`):** 다섯 쌍 — 129(아이스 캐논)/50b9b8·136(썬 바리케이드)/509470·140(크로스보우)/50b6e8·143(맨오워 풀)/5099e8·148(반더 타워)/50b7c0. 같은 가상표를 쓰는 128 은 별도 처리기가 있어 이름이나 주소 하나만으로 포함하지 않는다.
- **전체(`isPlainAssetDraw`):** 유닛 ∪ 워크샾 ∪ 건물·제단·사제 보호막·포스 필드 열 쌍 — 78/50e940·79/50ea08·139/509aa0·142/50ead0·147/50e940·149/50e668·150/50e888·164(제단)/5076b8·167(사제 보호막)/507770·170(포스 필드)/509388. `+78=4acfe0`, `+a4=41c100` 공통 Draw 를 쓴다. 제단은 현재 BYTE 대신 원래 소유자 WORD, 보호막은 TYPE 발 기준점을 같은 공통 Draw 에서 쓴다는 cpppj 이력(2026-10-27 포스 필드 추가, 2026-10-28 제단·보호막 실제 생성자 VT 연결)을 따른다.

## 2. 웹 범위

- 10.72(CD) 분기는 없다(표가 10.78 전용). 이 모듈은 판별만 하며 Draw 자체(`Renderer`·`SpriteDraw`)와 소비자 연결은 5-A 의 후속 항목이다. DOM 을 쓰지 않는다. 가상표를 읽는 helper `vtableOf` 는 유닛 모듈이 내보내 워크샾·전체 모듈이 재사용한다(웹 적응, C++ 은 각 파일에 같은 루프를 둠).

## 3. 검사 (`tests/plain-asset-draw.test.mjs` 6건)

- 전용 원본 관측 fixture 가 없는 보호 PE 표 대조이므로 **기대 표를 시험 안에 원본 수치로 따로 적은** 계약 시험이다(모듈 상수 재사용 없음): 유닛 13·워크샾 5·건물 10 쌍이 각각 [유닛, 워크샾, 전체] 판별에서 기대 조합(참/거짓)이다. 모든 쌍에서 TYPE 을 ±1·0·255 로 바꾸거나 가상표의 네 바이트를 각각 +1 하면(캐리 없이) 표에 있는 다른 쌍과 우연히 맞는 경우만 참이다. 111/141, 78/147 처럼 가상표를 공유하는 쌍의 교차(79/50e940, 128/50b9b8)는 거짓, 길이 49 는 모든 판별기가 거짓이고 50 은 판정한다, 이웃 바이트(0xa5)와 가상표 이후 바이트를 읽지 않고 최상위 비트가 있는 값을 부호 없이 비교한다.

## 4. 변형 확인과 남은 경계

- 세 모듈의 연산자·상수 자동 변형 39+16+27 = 82개를 적용했고 **전부 잡혔다**(표가 작고 모든 쌍·모든 바이트를 단언하기 때문).
- 남은 경계: 소비자(`RawCampaignFactoryDisplay` 등 Draw 분배·`RawCampaignSession` 의 `IsPlainBuildingCapture` — 75~77 등 별도 조건 포함)는 5-A 후속과 4-D 에서 연결한다.
- 다음 5-A 항목은 `o/RawColorMapCache`(adapt)·`ClientColorMapFile`(adapt) — 색표 캐시(웹은 계산 생성), 그다음 색표 소단위 `SpriteColorMaps`·`PreviewColorMaps`·`ProductionColorMaps` 이다(3.1절 5-A 목록).
