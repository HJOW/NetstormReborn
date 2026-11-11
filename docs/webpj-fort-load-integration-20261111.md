# webpj 4-B 적재 통합 시험 — WB04-E304 (2026-11-11, 4-B 마감)

3.1절 4-B 의 마지막 항목 **적재 통합 시험**이다. 코드 모듈은 더하지 않고, 4-B 에서 옮긴 부품(실제 풀·해시·Graph 를 가진 `RawSurfaceWorld`, `RawIslandBuilder`/`RawIslandRuntime`, 정상 지형 `RawNormalTerrainWorldBinding`, Fort 적재 부모 `RawFortWorldLoad`와 연결 훅)을 **실제 Tutorial 1(`\d\thewarbegins.fort`)** 로 함께 돌려 결과를 독립 기대값과 대조한다. 게임·브라우저·cpppj 실행은 하지 않았다.

## 1. 검사 (`tests/fort-load-integration.test.mjs` 1건)

- **입력**: `netstorm.tarc` 의 실제 타입 표(188칸 TYPE 레코드·프레임 코드·셰이프 셀 크기)와 실제 `thewarbegins.fort` 의 Territory 120BYTE. 월드·섬 목록·청크 지도·정상 지형·Carrier·배치 파이프라인은 모두 실제 모듈이다.
- **지형**: 적재가 만든 지면(97)/edgeFarm(121) 객체의 칸과 지역 번호로 65,536바이트 마스크(y × 256 + x, 빈 칸 255)를 만들면 독립 Python 재현(`terrain-expected.tsv`)·문서(`docs/exe/terrain-and-bridges.md`)의 값과 같다: **마스크 SHA-256 `b24af563…36d1`, 지면 칸 1505, 영역 0 = 954 · 영역 1 = 551**. 타입 모델(WB03-W)이 같은 값을 내므로 raw 경로(E290~E296 성장·도장 + E302 소비자)가 타입 모델과 칸 단위로 일치한다.
- **섬/청크**: 실제 `RawIslandBuilder` 가 놓은 16×16 청크 256칸(섬 번호·방향 글자·record 6BYTE·순번)이 타입 모델 `IslandBuilder`(`layoutFort`)의 결과와 같고 섬 수는 2 이다. (이 비교가 청크 항목 `side` 표현 불일치(Builder 는 BYTE 번호, 타입 모델은 글자)를 드러내 E303 에서 `RawTerrainGrowth` 를 고쳤다.)
- **후처리/수명**: 후처리 순서가 generate → 저장 후처리(objects → money) → graph0 이고, Kernel 은 비어 있으며 후처리 깊이 0 이다.

## 2. 범위와 4-D 로 넘기는 것

3.2절 4-B 의 완료 기준은 "Tutorial1 Fort 적재 결과(객체·지형·섬)가 cpppj 와 같다"였다. 이 시험은 그중 **지형·섬**을 독립 기대값으로 닫는다. 다음은 **4-D 로 넘긴다**(사유 포함):
- **저장 객체(Terr/Chaff 의 객체 레코드)의 실제 적재**: `FortObjectLoad`/Money/Meta child 를 실제 월드 몸체로 돌리려면 사제·신전·간헐천·다이스 등 파생 후처리 몸체와 Session 소유 장치(Player 기록·AI·UI·전송)가 모두 등록돼야 한다. 이는 cpppj 도 `RawCampaignSession` 이 소유하는 연결이므로 4-D 의 Session 조립에서 하나로 한다. 이 시험의 child 는 호출 순서만 기록하는 어댑터이다.
- **cpppj `session-dump` 와의 숫자 대조**: 기준선 오라클 실행 파일(`1f00a25a`)에는 session-dump 가 없고(3.5절 1번), 기대값 표는 cpppj 가 아니라 독립 Python 재현이다. cpppj 현재 빌드의 `--ui-script` session-dump 를 쓰려면 재기준선·시나리오 형식(4-D)이 필요하다.

## 3. 4-B 마감

4-B 는 35/35 로 마감한다(E274~E304). 체크 목록은 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261111-4b-through-wb04-e304)으로 옮겼다. 다음 단계는 4-C(패킷·로그인·내부 미션 전송)이다.
