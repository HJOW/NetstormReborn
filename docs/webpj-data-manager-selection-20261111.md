# webpj DataManager 섹션 검색·선택(RawDataManagerSelection) — WB04-E282 (2026-11-11, 4-B)

3.1절 4-B 의 **RawDataManagerSelection**(97줄)을 `core/raw-data-manager-selection.js` 로 옮겼다. 원본 `0044ce60`(검색)·`0044ceb0`(선택)이며 기준선(`1f00a25a`) 뒤 `RawDataManagerSelection.cpp/.h` 에는 변경이 없다(3.6절 ①). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

**항목 순서 조정:** 3.1절 목록은 `RawDataManagerImage` → `Resize` → `Selection` 순이지만 `Image` 는 `Resize` 의 훅 묶음(`DataManagerResizeHooks`: selection·list·memory)을 입력으로 쓰고 `Resize` 는 `Selection` 의 검색/비교를 쓴다. 선행 관계를 지켜 **Selection → Resize → Image** 순으로 옮긴다(0.1절 다음 작업에 기록).

## 1. 규칙

- **`findDataManagerSection(manager, name, hooks)`:** manager 12BYTE 의 head(`+4`)를 current(`+8`)에 쓰고 노드를 따라간다(next = 노드 `+0x38`). 이름은 노드 `+16` 부터 40BYTE 안의 NUL 종료 문자열이며 NUL 이 없으면 거부한다. 비교(CRT stricmp 경계 `compare` 훅, 없으면 공식 ASCII 비교) 뒤 일치하면 현재 current 를 돌려주고, 아니면 **캡처한 노드가 아니라 current 를 다시 읽어** 그 노드의 next 로 진행한다. 끝까지 없으면 current 는 0 이다.
- **`selectDataManagerSection(manager, name, writable, hooks)`:** 검색 → current 가 0 이면 진단(`MissingCurrent`, 뒤에 current 를 다시 읽음) → `writable == 0` 이면 현재 노드의 버퍼(`+0`) 또는 end(`+8`)가 0 일 때 진단(`EmptyRead`, 뒤에 다시 읽음) → 현재 노드의 pos(`+4`)만 0 으로 쓰고 current 를 돌려준다. 다른 노드와 필드는 그대로다.
- **`compareDataManagerSectionNames`:** ASCII 대소문자 무시, NUL 이전만 비교, 접두가 같으면 길이 비교. 128 이상의 글자는 CRT 로캘 의존이라 복원하지 않고 `RangeError` 로 거부한다.
- 노드는 호스트 주소로 자르지 않고 `stream(핸들)` 훅이 돌려주는 60BYTE 이상 `Uint8Array` 로 빌린다. 진단 장치가 필요한 지점에서 없으면 빈 성공 대신 거부한다.

## 2. 검사 (`tests/data-manager-selection.test.mjs` 3건)

- `datamanager-selection-x86.tsv` **864입력 전부**(머리글 행 제외): 전체 256BYTE(manager 12 + 노드 3개, 76BYTE 간격) 위에서 검색(F)/선택(S)을 이름 3종(`sTaTe`·`TERRITORY`·`missing`)·쓰기 허용·4개 변이(비교 중 current 변경 · 비교 결과 강제 · 진단 뒤 수리)로 실행해 실행 뒤 전체 메모리·반환 핸들·비교/진단 호출의 순서와 그때의 전체 메모리를 대조했다. **첫 실행에 모두 통과했다.**
- 손 시험(cpppj `DataManagerSelection_PhysicalContractsAndAsciiNames`)과 계약: 이름 비교 규칙·12BYTE 미만 manager 거부(불변)·빈 목록·진단 장치 누락 거부·비ASCII 거부, 노드 조회 장치/60BYTE/이름 NUL 누락 거부, 이름 영역 39글자 허용과 40글자 거부, 읽기 선택의 진단 장치 누락 거부(pos 미변경), 소문자 변환 범위(`@`·`[` 경계).
- 변형 확인 26개 중 처음에 5개가 살아남았고 4개를 위 경계 시험으로 잡았다(이름 크기 39, 소문자 변환 범위 양 끝, 진단 장치 누락). 남은 1개(`hooks.stream` 없는 호출을 `TypeError` 가 아니라 명시 오류로 거부하는 문구 차이)는 어느 쪽이든 거부되는 **동치 변형**이다. 변형 시험 중 무한 루프가 되는 변형은 시험 시간 제한(15초)으로 죽은 것으로 센다.

## 3. 남은 경계

- cpppj `CampaignTemplates_*` 시험(실제 Player Get·Money/Badges/Technology 선택 등)은 `CampaignTemplateStorage`(4-D)가 이 모듈을 쓸 때 같은 단언으로 닫는다.
- 3.1절 4-B 는 12/30 이다(Selection 은 순서 조정으로 먼저 옮겨 목록의 `RawDataManagerSelection` 항목에 체크한다). 다음은 **RawDataManagerResize**(100줄)이다.
