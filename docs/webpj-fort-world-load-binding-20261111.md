# webpj Fort 적재 부모 월드 연결(RawFortWorldLoadBinding) — WB04-E303 (2026-11-11, 4-B)

3.1절 4-B 의 client **RawFortWorldLoadBinding**(기준선 40줄)을 `core/raw-fort-world-load-binding.js` 로 옮겼다. E286 의 Fort 적재 부모(`RawFortWorldLoad`)에 정상 지형(E302)·저장 객체·SP·Meta·Graph 를 한 실제 월드의 필수 경계로 연결하는 훅 묶음이다. 선행: E286·E302·E299. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

`makeFortWorldLoadBinding(world, normal, objects, money, meta, moneyStream, selectMeta, metaStream, baseHooks)`:
- 다른 풀의 정상 지형 소비자나 `moneyStream` 누락은 부모 실행 전에 `TypeError` 로 거부한다. child 어댑터(`makeFortWorldChildrenHooks`)가 풀 불일치·선택 함수 누락을 같은 위치에서 거부한다.
- 훅: `generate(builder, region)` = `normal.generateOriginal(builder.owner, builder.bounds, region)`, `money(fort, owner)` = `money.loadSelected(() => moneyStream(fort, owner))` — Money 선택은 wrapper 의 깊이 증가/진단 뒤에 **지연**해서 호출한다(JS 인수 평가로 먼저 선택하지 않음), `rebuildGraph(resetAll)` = 월드 Graph 재구성(인자 그대로), objects/badges/technology 는 child 어댑터. 나머지(템플릿·territory 조회·진단·Populate)는 호출자의 기존 필수 훅으로 남는다(군도/Island0 Populate/TypeNames/UI 는 미복원 범위).
- 기준선 뒤 cpppj 변경은 없다(3.6절 ①). CD(10.72)는 웹 목표 밖.

## 2. 검사 (`tests/fort-world-load-binding.test.mjs` 3건)

- **장면(cpppj `FortWorldLoadBindingTests.cpp` 이식)**: 실제 월드·정상 지형·섬 목록·Builder 부모로 적재해, 기존 PlaceAll 의 원본 골든(`fortworldload-builder-consumer-x86.tsv` originals 행)과 **생성 전** Territory 120BYTE·물리 퍼즐 코드 4BYTE 열·초기 청크 3072BYTE·초기 섬 3204BYTE 가 같고, **Generate 직전** 청크/섬 3204·호출 횟수(Place 1·Add 1·Bounds 1·진단 0)가 같으며 적재 뒤에도 같다. 후처리 순서(generate → objects → money → badges → technology(부모 경계·선택·UI 경계) → graph0)·깊이 1 복귀·게임 난수와 CRT 99 캐시 소비(SP 저장소는 초기화 전)·섬 1·청크 (2,2) 지역 0·Territory 위치 120·모든 생성 SID 의 지역 WORD 0·121 의 HP 73(1 ≤ 개수 ≤ 60)·Kernel 비어 있음을 확인한다.
- **환원**: cpppj 시험은 빈 저장 목록에서 실제 객체/Money/Meta child 모듈을 돌리지만, 웹은 child 를 호출 순서·인자·깊이 계약을 기록하는 관찰 어댑터로 바꿨다(각 child 본문은 E276~E286 단위가 소유, Tutorial1 Fort 의 실제 child 통합은 4-B 마지막 적재 통합 시험).
- 훅 배선(generate 인자 전달·Money 선택 지연·Graph 인자 그대로)과 연결 거부(다른 월드·누락·풀 불일치).
- **변형 확인** 10개 중 1개만 남았고 동등하다: `generate` 의 owner(Fort 부모의 builder 상태 owner 가 항상 1 이라 상수로 바꿔도 같음).
- 원본 골든을 이식 추적 목록에 새 항목(`fort-world-load-builder-consumer`)으로 등록했다(소비 검사·웹 모듈·근거 소스 포함).

## 3. 남은 경계

- 3.1절 4-B 는 34/35 이다. 다음: 4-B 적재 통합 시험(Tutorial1 Fort 객체·지형·섬 vs cpppj `--inspect-fort`/session-dump) — 실제 child 모듈(객체·Money·Meta)과 Tutorial1 Fort 파일로 이 연결을 다시 돌린다.
