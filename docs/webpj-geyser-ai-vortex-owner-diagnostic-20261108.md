# webpj 간헐천 AI 등록·신전 소유자 진단(RawGeyserAiRegistration·RawVortexOwnerDiagnostic) 이식 — WB04-E78

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일흔여덟 번째 부분이다. 간헐천 등록/제거가 Player 1..8 의 AI 본체에 알리는 순회(`o/RawGeyserAiRegistration`, 00416720)와 신전 소유자 변경의 섬 누락 진단(`o/RawVortexOwnerDiagnostic`, 004dc58f..004dc5f8)을 cpppj 기준선 `1f00a25a` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-geyser-ai-registration.js` — `RawGeyserAiRegistration(playerRecords)`·`GeyserAiRegistrationBoundary`

- `register(sid, state)`: Player 0 은 순회 대상이 아니다. 1..8 의 현재 AI 포인터 DWORD(Player 한 칸 0xb4, +0xb0, 정렬되지 않아도 리틀엔디언)를 번호순으로 **매 호출·매 하위 뒤 다시 읽고**, 비영이면 연결된 실제 하위(004153c0)를 `(ai, sid, state)` 로 부른다. 하위가 연결되지 않았으면 빈 성공 대신 첫 요청의 `owner/aiPointer/sid/state` 를 담은 `GeyserAiRegistrationBoundary` 로 거부한다(AI 가 모두 없는 최초 로드는 자연 반환).
- `bindPatch(update)`: 실제 하위를 연결한다(비함수 거부). 공유 Player 9기록 저장소(복사 없음)가 부족하면 생성자가 거부한다.

`www/core/raw-vortex-owner-diagnostic.js` — `reportVortexOwnerMissingIsland(pool, sid, island, owner, count, hooks)`

- 원문 256바이트 버퍼에 `islandList.totalIslandIds=%d\nislandId=%d\npos=(%d,%d)\noldPlayerId=%d\nnewPlayerId=%d\n` 을 채워 실제 상세 저장 장치(`hooks.detail`)에 한 번 쓴 뒤 진단(`hooks.assertion('0', '\Ns\O\Vortex.cpp', 0x235)`)을 올린다. 진단이 정상 복귀하면 복귀하고, 상세를 쓴 뒤 진단 장치가 없으면 첫 기록을 되돌리지 않고 거부한다.
- 좌표는 슬롯 +14/+18 단정도 float 을 CRT ftol(signed 64비트 절삭) 하위 DWORD 로 바꾸며 비유한/범위 밖은 0, 옛 소유자는 슬롯 +34 BYTE, 새 소유자·섬 목록 개수는 요청 DWORD 의 signed 해석이다. TYPE/free 상태로 앞선 진단 접두를 취소하지 않는다.

## 2. 검증

- `tests/geyser-ai-registration.test.mjs`(3건): `geyser-ai-registration-x86.tsv` 10.78 302행(정상 84·하위 거부 218)의 분류·첫 요청·전체 Player 저장소(Adler-32 포함)가 같고, 같은 객체의 현재 Player 변경 재조회·Player 0 무시·부족한 저장소 거부, 연결한 하위의 번호순 호출·호출 뒤 다음 Player 재조회를 확인한다.
- `tests/vortex-owner-diagnostic.test.mjs`(2건): `vortex-owner-diagnostic-local-x86.tsv` 10.78 48행의 상세 문자열(ftol 좌표·signed 정수)과 진단 식·파일·줄이 같고, 정상 복귀/누락 진단·상세/판본 거부와 비유한·음수 좌표를 확인한다.

최초 실패와 교정: 새 모듈은 위 fixture 350행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 간헐천 AI 갱신(004153c0)의 실제 본체와 Player 저장소의 실제 연결, `RawVortexOwner`(신전 소유자 변경 부모: 슬롯 필드 제거·Player 18필드·섬 조회)는 해당 파일 이식 때 연결한다. 진단 helper 만 먼저 옮겼다.

## 4. 다음 시작점

`RawVortexOwner`, `RawFenceRebuild`와 Fence 생성 부모, `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawShapeToBuffer`(+ 캡처 계열: 선행 `DrawShape` 대응·SHP 자산 시험 장치), 그 밖의 작은 몸체이다.
