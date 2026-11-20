# 스커미시 시작 섬 fort 자료 묶음 (WB04-E1103)

12차 스커미시는 참가자마다 원본 다인용 fort의 전투 영역(Terr) 하나를 시작 섬으로 쓴다. 후보 22개(영역 기준)는 fort 12개에 들어 있는데, 이 파일들은 `netstorm.tarc`의 32개 fort에 없고 원본 설치본의 `d/*.fort` 느슨한 파일이므로 웹 자료에 새로 묶었다.

## 구현

- [prepare-assets.js](../webpj/tools/prepare-assets.js)에 새 묶음 `skirmish`를 더했다. 공용 원본 폴더 `assets/game-data/d/`의 `FWOS9`·`Imp1`·`Imp3`·`LCA`·`Razorianwar6`·`Thundere`·`TpVpath`·`school5`·`sh3_n_2`·`sw_m2`·`tnron42`·`tnron51`의 `.fort` 12개(합계 약 61KB)를 변환 없이 `webpj/www/data/d/`로 복사하고 `manifest.json`에 경로·크기·SHA-256과 묶음 설명을 적는다. 파일 이름 목록은 후보 표(`SKIRMISH_START_ISLANDS`)와 같아야 하며 시험이 확인한다.
- 이 묶음은 메인 메뉴 필수 묶음(`menu`)이나 한국어 묶음에 섞이지 않는다. 앱은 스커미시를 시작할 때만 `AssetLoader.loadBundle('skirmish')`로 받는다(연결은 E1104 이후). 첫 화면 요청 목록은 그대로이며 `tests/external-access.test.mjs`의 영어·한국어 요청 목록 시험이 이를 확인한다.
- `originals/`의 파일은 읽지도 바꾸지도 않았다(공용 원본 폴더의 복사본을 쓴다).

## 검증

- 새 `tests/skirmish-fort-bundle.test.mjs` 3건: 매니페스트의 `skirmish` 묶음 항목이 후보 표의 fort 이름 12개와 정확히 같고 메뉴 묶음에 fort가 섞이지 않음; 복사본이 공용 원본과 바이트·크기·SHA-256이 같고 서명 `F`·섹션 35개가 길이 표대로 읽혀 `makeSkirmishEmptyWorldFort`가 비어 있지 않은(원본보다 작은) 결과를 냄; 후보 표의 영역 번호 22개가 각 fort 영역 표(20×6바이트)에서 비어 있지 않은 기록이며 패턴 번호가 영역 패턴 표 안임.
- `node webpj/tools/prepare-assets.js --check`가 복사본·매니페스트가 원본과 같다고 확인하고, 자산·적재기·fixture 목록 시험(12건)이 통과한다.

## 남은 일 (12차 나머지)

- E1104~: `RawIslandPartition`·`RawCentralIsland`·`RawBattleIsland` 이식과, 시작할 때 `skirmish` 묶음을 받아 빈 월드 fort로 미션을 시작·무작위 간헐천·AI 난이도/동맹 적용을 실제 Session에서 확인하고 Campaign 항목의 기본값을 켠다.
