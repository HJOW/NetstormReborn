# webpj Fort Objects stream 적재(RawFortObjectStream) — WB04-E276 (2026-11-10, 4-B)

3.1절 4-B 의 **RawFortObjectStream**(121줄)을 `core/raw-fort-object-stream.js` 로 옮겼다. cpppj 에서는 `RawFortObjectLoad` 의 멤버 `ObjectStream/ChunkStream/SectionStream/LoadStream` 이며, 웹에서는 값형 경로를 이미 가진 `FortObjectLoad`(`pool` · `types` · `state` · `hooks`)를 그대로 쓰는 함수 `objectStream/chunkStream/sectionStream/loadStream(loader, …)` 로 옮겼다. 기준선(`1f00a25a`) 뒤 `RawFortObjectStream.cpp` 에는 변경이 없다(3.6절 ①; `RawFortObjectLoad.h` 의 공유 상태 대여 생성자 변경은 **RawFortObjectLoad** 항목에서 판정한다). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 하는 일

완성 레코드 목록을 만들지 않고 stream 을 읽으면서 같은 객체/내용물/월드에 즉시 적용한다(필드를 읽기 전에 객체를 만들고, 각 실제 효과 뒤에 현재 TYPE/전역을 다시 읽는다).

- `objectStream`(004be160): 위치 BYTE · 저장 종류 BYTE(0xf6 이상은 다리와 owner=255−저장값, 아니면 현재 변환표/checking 으로 치환) → 종류 0 이면 추가 BYTE 가 있을 때 진단 0x2e8 · island 는 추가 BYTE 를 읽지 않고 pos 만 증가 → create → 저장 프레임/QA/QB/다리 프레임/legacy·factory state → owner 결정(저장 마스크 · special/mission · 지움 마스크 · island 류 0 · 큰 섬 진단 0x334) → HP → 지형·난수 프레임 → pop(9) → container 면 `contents` 훅(E275 `raw-fort-contents-load.js`) → Fort 후처리 삭제.
- `chunkStream`(004be2c0): 표지 BYTE(99 아니면 진단 0x37a) · WORD 개수(unsigned) 뒤 객체를 읽은 즉시 적용한다. 뒤 레코드 실패 때 앞선 객체/난수를 유지한다.
- `sectionStream`(004be3c0): 원문 y/x 순서로 현재 지도 크기(`chunksX/Y`)와 Terr 허용 정책을 읽은 뒤에만 stream 을 소비한다. scope 는 Terr 1 · Chaff 2.
- `loadStream`(004be610): 깊이 증가 → 변환표 준비 → Terr[start,end) → (Fort/Chaff 모드면) Chaff → 정상 꼬리의 변환표 반납 → 소유자 마감. 예외는 깊이만 복구하고 변환표와 이미 만든 월드는 보존한다.

## 2. 검사 (`tests/fort-object-stream.test.mjs` 5건)

- 값형 경로와 **같은 고정 관측** `fortobjectload-x86.tsv` 의 원본 609행(객체 573 · 전체 순회 36)을 stream 경로로 재생했다: 객체 행은 저장 BYTE 를 stream 에 두고 변환표(identity + 저장 종류→입력 종류)와 함께 `objectStream` 으로, 순회 행은 20 Terr 와 Chaff 의 저장 입력을 `loadStream` 으로 실행해 효과 순서 · 전체 슬롯 · 난수 · 전역 상태를 같은 기대값과 대조했다. 내용물 17행은 `loadFortContents`(E275)로 읽는다. 첫 실행에 609행 모두 통과했다.
- 손 시험(cpppj `FortObjectStream_*`): 잘린 둘째 객체는 앞선 객체와 증가한 cursor 를 남기고 부분 생성 효과를 유지하며, 부모 순회 실패는 깊이만 복구하고 변환표를 반납하지 않으며 소유자 마감도 하지 않는다.
- 변형 확인 34개 중 10개가 처음에 살아남았다. 고정 관측이 건드리지 않는 경계라서 손 시험을 더해 잡았다: 저장 종류 246 은 다리와 owner 9, 변환 결과 0 뒤 추가 BYTE 진단 0x2e8, 큰 섬 진단 0x334, 청크 개수는 unsigned WORD(0x8000 개), 소유자는 슬롯 1..8 만, 정상 꼬리의 변환표 반납, Terr 이름 두 자리와 Chaff 조건, 음수 end(0x80000000)의 signed 남은 자료·WORD 비교. 한 변형(QB 의 부호 확장 DWORD)은 `loader.word` 가 하위 두 바이트만 쓰므로 관측 불가능한 **동치 변형**이다.
- 시험 도우미 `tests/fort-load-scene.mjs`: 값형 경로 시험의 장면 구성(입력 열 이름 · 슬롯 쓰기 · 프레임/내용물 프로필 · 효과 훅 · 이벤트 기록)을 별도 모듈로 분리해 두 시험이 같은 장치를 쓰게 했다(`fort-object-load.test.mjs` 6건은 그대로 통과).

## 3. 연결·남은 경계

- 실제 월드/Session 연결(변환표 준비 `PrepareFortTypeNames`, 템플릿 stream `select/access`, 실제 소유자 마감)은 **RawFortObjectLoad**(partial)·**Template**·**RawFortWorldLoad**·4-D 에서 이 훅에 연결한다. 이 단위의 훅은 cpppj `FortObjectStreamHooks` 와 같은 계약이다.
- 3.1절 4-B 는 5/30 이다. 다음은 **RawFortImage**(87줄)이다.
