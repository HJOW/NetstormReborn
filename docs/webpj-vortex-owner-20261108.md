# webpj 신전 소유자(RawVortexOwner) 이식 — WB04-E79

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일흔아홉 번째 부분이다. 신전(Vortex)의 실제 가상 소유자 변경(`o/RawVortexOwner`, 004dc4e0, 가상 +0x74)을 cpppj 기준선 `1f00a25a` 에서 옮겼다. 앞 단위(E78)에서 옮긴 섬 누락 진단(`raw-vortex-owner-diagnostic.js`)의 부모이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(Player 0xac, 추가 목록·revision 생략)와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-vortex-owner.js` — `RawVortexOwner(pool, base, players, outposts, revision, hooks)`

- `handles(sid)`: 현재 슬롯의 가상 표가 신전 네 개(0x5196f0·0x519860·0x5197a8·0x519918)일 때만 분배한다(타입 이름으로 다른 몸체를 대체하지 않음).
- `set(sid, owner)`: ① 외부 효과 전에 읽은 이전 소유자(+34)·extra(+40)로, extra 비트 3(8)이 없고 이전 소유자가 있으며 그 Player 의 초기 신전 DWORD(Player 0xb4 의 +0x7c)가 이 SID 이면 두 DWORD(+0x7c, 복사본 +0x80)를 0 으로 지운다. ② 슬롯 상태 비트 2(4)가 없고 extra&9 가 없으면 섬 번호 조회 → 섬이 없으면 진단(`missingIsland`, 정상 복귀 시 계속) → 섬 소유자 변경 → 같은 좌표 지역 소유 요청 → 추가 목록(outposts)을 **매 반복의 현재 개수(signed)·항목**으로 순회하며 이 신전과 같은 소유자인 항목의 지역 소유 요청(free/dead 필터 없음; 항목이 목록 범위나 풀 밖이면 거부). ③ 공통 Owner(`SquidOwner.set`) 실행 뒤 소유자(+34)가 있고 extra&9 가 없고 그 Player 의 신전 DWORD 가 0 이면 두 DWORD 에 이 SID 를 쓴다. ④ 생산 갱신 뒤 revision 을 1 증가한다(DWORD 감김).
- 생성자는 같은 풀(`base.pool`)·Player 9기록(9×0xb4=1620바이트)·필수 훅 여섯·revision 보유 객체를 최초 쓰기 전에 확인한다. 섬/지역/생산 효과는 명시 외부 경계(훅)이며, 효과 뒤 현재 raw 소유자·좌표·목록을 다시 읽는다.

## 2. 검증

- `tests/vortex-owner.test.mjs`(2건): `vortex-owner-x86.tsv` 10.78 864행 — 옛 소유자·새 소유자·슬롯 상태·extra·초기 신전 DWORD·외부 효과 변이(섬 소유자 효과가 소유자/좌표를 바꿈, 첫 지역 효과가 추가 목록 개수/extra/소유자를 바꿈, 생산 갱신이 revision 을 바꿈) 조합에서 외부 효과 호출 순서(I/P/R; 내부 공통 Owner 표식 B 제외), 슬롯 50 raw, 아홉 Player 의 신전 네 DWORD, 공통 Owner 아홉 소유자 목록(활성 밖 stale 포함), revision 이 같다. 계약: 미복원 가상 표/잘못된 9기록/다른 풀/훅 누락은 최초 raw·Player 변경 전에 거부하고, 추가 목록 항목이 풀 밖이면 거부한다.

최초 실패와 교정: 새 모듈은 위 fixture 864행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 섬 번호 조회/섬 목록 존재 확인/섬 소유자 전파(`RawIslandRuntime`)·지역 투표와 지형 도장(`RawRegionOwnership` 등)·생산 갱신(`0047d890`)·missingIsland 의 실제 연결(E78 진단 helper 와 `RawCampaignOwnership` 조립)은 해당 단위에서 한다. cpppj 의 캠페인 조립 시험(`VortexOwner_CampaignSameCoreAndFirstRegion`)은 실제 보호 자산과 `RawIslandRuntime`·`RawCampaignOwnership` 이 필요해 이번 범위 밖이다.

## 4. 다음 시작점

`RawFenceRebuild`와 Fence 생성 부모, `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawShapeToBuffer`(+ 캡처 계열), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
