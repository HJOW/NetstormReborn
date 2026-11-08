# webpj 신전 PreDestroy(RawVortexPreDestroy) 이식 — WB04-E80

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여든 번째 부분이다. 신전(Vortex)의 실제 가상 PreDestroy(`o/RawVortexPreDestroy`, 004dca50)를 cpppj 기준선 `1f00a25a` 에서 옮겼다. E79 의 소유자 변경(`raw-vortex-owner.js`)과 짝이 되는 삭제 쪽이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-vortex-pre-destroy.js` — `RawVortexPreDestroy(pool, players, temples, residences, outposts, loading, hooks)`

- 진입 때 extra 비트 0·3(9)이 없으면: 신전 목록·거주 목록에서 이 SID 의 모든 중복을 제거(`SquidPostPopList.removeAll`; 앞방향 안정 압축, 비활성 물리 꼬리 보존) → 현재 좌표의 섬 번호 → 섬이 없으면 진단 0x2ab, 섬의 현재 신전 SID 가 이 SID 가 아니면 진단 0x2ac(둘 다 정상 복귀하면 계속) → 섬 신전 비움 → 섬 소유자 0 → 효과 뒤 **현재 좌표**로 지역 소유 요청 → 추가 목록(outposts)을 매 반복의 현재 개수·항목으로 순회하며 이 신전과 같은 소유자인 항목의 지역 소유 요청(항목이 풀 밖이면 거부).
- 공통 Damageable PreDestroy 를 같은 flags 로 부른 뒤, 로딩 깊이(getter)가 0 이고 **현재** extra&9 가 없으면 생산 갱신을 부른다. 그 뒤 **현재** 소유자(+34)가 0 이 아니면 그 Player 의 신전 DWORD(+0x7c)가 이 SID 일 때 0 으로 쓰고 읽은 값을 복사본(+0x80)에 쓰며, 둘째 신전 DWORD(+0x84, 복사본 +0x88)도 같게 처리한다. 진입 extra 검사와 Damageable 이후 extra 검사는 서로 다른 현재 읽기이고, 소유자는 Player 필드 접근마다 다시 읽는다(9 이상은 거부).
- 생성자는 PATCH 풀·Player 9기록(1620바이트)·로딩 getter 를 확인하고, 선택한 위치의 미연결 효과는 앞선 원문 쓰기를 보존한 채 거부한다.

## 2. 검증

- `tests/vortex-pre-destroy.test.mjs`(2건): `vortex-pre-x86.tsv` 102행 — 변이 조합(extra·소유자·신전 참조 일치·섬/신전 진단 분기·로딩 깊이)과 외부 효과 변경(Damageable 이 소유자/extra/로딩을 바꿈, 생산 갱신이 소유자를 바꿈)에서 아홉 Player 전체 raw, 슬롯 50/60/61 raw 150바이트, 세 목록의 count+물리 꼬리, 로딩 깊이, 외부 효과 호출 순서가 같다. 계약: 필수 Damageable 이 없으면 목록/지역의 앞선 실제 효과를 남기고 Player 정리를 실행하지 않으며, 잘못된 Player 저장소/판본/로딩 getter 와 풀 밖 outpost 를 거부한다.

최초 실패와 교정: 새 모듈은 위 fixture 102행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 섬 목록/지역 투표/생산 갱신(`0047d890`)/공통 Damageable PreDestroy(`0044b4b0`)의 실제 본체 연결과 신전 Post/Destroy 진입점의 실제 가상 표 분배는 해당 단위에서 한다.

## 4. 다음 시작점

`RawFenceRebuild`와 Fence 생성 부모, `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawShapeToBuffer`(+ 캡처 계열), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
