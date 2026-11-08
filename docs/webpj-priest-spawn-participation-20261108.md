# webpj 사제 생성·참여 조회·포획 안내(RawPriestSpawn·RawPriestParticipation·RawPriestTakenNotice) 이식 — WB04-E68

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 예순여덟 번째 부분이다. 사제 수명 부품 중 생성·참여·포획 안내 세 파일을 cpppj 기준선 `1f00a25a` 의 `o/RawPriestSpawn`·`o/RawPriestParticipation`·`o/RawPriestTakenNotice` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(BYTE 폭, 다른 소유자 위치, CD 가상 호출)와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-priest-spawn.js` — `PriestSpawnState`, `RawPriestSpawn(pool, types, spots, state, hooks)`, `makePriestSpawnHooks`

- `spawn(x, y, type, word)`(0044b2e0): 좌표에 float32 `0.99999` 를 더해 절삭한 칸에서 시작하여 북→동→남→서 나선(한 변 길이 1,1,2,2,…,11,11, 총 132개 후보)으로 이동한다. 범위 밖(가장자리 0/255)도 한 칸 이동하며 예산을 소비한다. 후보 칸의 spot 이 0 이면 검사 없이, 아니면 배치 검사가 참이고 spot 에 0x10 이 없을 때 채택하여 생성(type, 0) → WORD 전체 복사 → 새 자산 타입이 다리 타입이면 표면 알림 → 소유자(하위 7비트) 지정 → Pop → Carrier 검사 래퍼(genus 0x30000 비트가 있을 때만 가상 +0xcc 요청, 없으면 원본 assert)이다. 132개를 소진하면 현재 debug 전역이 켜져 있을 때 원본 진단으로 거부한다. 타입이 첫 자산 타입(70) 미만이거나 표 밖, 비유한/정수 범위 밖 좌표는 효과 전에 거부한다.
- `makePriestSpawnHooks`: 종속 해방 훅의 `spawnPriest` 만 실제 몸체로 연결하고 다른 훅을 보존한다(다른 풀 거부).

`www/core/raw-priest-participation.js` — `RawPriestParticipation(pool, hash, types, priests, carrier, state, altar)`

- `find(소유자)`(00494f10): 살아 있는 사제 목록(E66 `RawPriestLookup`)의 첫 SID → 풀을 SID 2 부터 free 비트만 건너뛰며 순회하여 Carrier genus 의 포획 내용물(mask 0x20, 현재 사제 타입)의 원래 소유자 하위 7비트가 같으면 즉시 반환, 제단 타입의 다이스(`DaisAt`: 현재 다이스 타입과 flags2 에 따른 finder 단계)가 dead 가 아니고 제단의 원래 소유자 WORD 전체가 같으면 마지막 일치 제단을 기억하여 끝에 반환(없으면 0)한다. 실제 제단 몸체가 설치되면 그 `daisAt` 을 쓴다(다른 풀 거부).
- `refresh(소유자)`(00495060): 소유자가 유효하지 않아도 조회는 수행하고, 소유자 1..8 일 때만 현재 공유 소유자 상태(없으면 독립 배열)에 결과 없음 2 / 사제 타입 0 / 그 밖 1 을 쓴다.

`www/core/raw-priest-taken-notice.js` — `RawPriestTakenNotice(pool, localOwner, deadlines, hooks)`

- `notify(부모, 이전 소유자)`(004927f0): 참여 갱신 먼저 → 현재 소유자가 로컬이면 끝 → 이전 소유자 기한 칸 범위 확인 → 안내 키/음향(내 사제 `WarnYourPriestTaken`+ourPriestCaptured.wav, 중립 `WarnNeutralPriestTaken`, 적 `WarnPriestTaken`+enemyPriestCaptured.wav) → 벽시각이 기한 이하일 때만 이름 둘과 256바이트 서식(`키(이전,현재 이름,이전 이름,현재 색,이전 색)`) 전달 → 안내 뒤 다시 읽은 벽시각 + 20초를 기한으로 저장한다. 기한 억제 중에도 음향과 기한 재설정은 수행한다.

## 2. 검증

- `tests/priest-spawn.test.mjs`(2건): `priestspawn-x86.tsv` 10.78 408행(하위 호출 기록 Q/N/O/L/S/K, 생성 슬롯 raw, 다리 타입 전역, 지도의 Adler-32), 누락 경계·잘못된 지도/표 크기·비유한 좌표·타입 범위 거부와 132개 후보 소진 진단·비 Carrier 검사 assert·다른 풀 거부.
- `tests/priest-participation.test.mjs`(4건): `priest-participation-x86.tsv` 304행(Find 결과, 갱신 뒤 소유자 상태 9칸, 전체 24000슬롯 풀의 Adler-32, 목록 checksum, 조회가 풀·목록·4단계 버킷 머리를 바꾸지 않음; 실제 `RawPriestLookup`·`RawCarrierPreDestroy.findContained`·`RawSquidFinder` 사용), live 의 이른 성공 뒤 손상된 후속 목록/타입/다이스 전역 비조회와 DWORD 소유자 폭, 다른 풀 Carrier 거부와 종속 순환 거부, 공유 소유자 상태 표만 쓰고 목록/개수/fallback 을 보존.
- `tests/priest-taken-notice.test.mjs`(5건): cpppj 에 독립 fixture 가 없는 파일이라 소스 주석의 순서 계약을 하위 효과 호출 기록으로 확인한다(내 사제/중립/적 사제의 키·음향·색, 로컬 소유자 조기 종료와 색 대체, 기한 억제 중 음향과 기한 재설정, 범위/256바이트 초과/판본/누락 훅 거부).

최초 실패와 교정: 새 모듈은 위 fixture 712행 전부를 첫 실행에 통과했다. 참여 시험의 Carrier 조회 훅 이름(`damageablePre`)만 시험 쪽에서 교정했다.

## 3. 경계와 남은 것

- 종속 해방(`RawDamageableRelease`)과 실제 사제 소유자까지의 합성(cpppj `priest_spawn_release_uses_real_factory_and_priest_owner`)은 해방 파일 이식 때 대조한다. 제단 몸체(`RawAltarSacrifice`)가 이식되면 `altar` 인자로 실제 `daisAt` 를 연결한다.
- 포획 안내의 실제 호출(`RawCampaignSession`)은 세션 이식 때 연결한다.

## 4. 다음 시작점

`RawPriestRegen`(회복 분배기와 패치 추적), 사제 배치 계열(`RawPriestPlacement*`), `RawAssetPostPop`, 후보/지형 권한(`RawCanonPlacementPermission`) 이다.
