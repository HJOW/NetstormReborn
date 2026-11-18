# dotnetpj 0.607 — 요새 일반 Pop 결선 검증 완료(R4-2)

2026-11-18, HJOW-Athlon. 2026-11-17 사용자 중단 지시로 미완이던 요새 일반 Pop 결선(인수인계 0.4의 0.607 항목 1~4)을 이어서 검증 완료했다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 한 줄 요약

`HostSession`의 요새 Pop 훅이 지원 타입(표면 157·섬 94·종유 95)을 지형 분배 Pop으로 등록하고, 비용이 있는 요새 타입(71·75·78·89·122·123·124·158)은 전용 파생(R6)이라 기록만 남긴다. 실패하던 신규 검사 1건을 가설 (a)(Pop된 타입 비용 0)로 확정해 TotalMade 누적 단언으로 바꿔 통과시켰다.

## 한 일

- `dotnetpj/tests/Netstorm.Client.Tests/HostSessionFortPopTests.cs`의 `FortGeneralPopRegistersSupportedTypes` 단언을 수정했다. `DumpTotals().TotalCost` 비교 대신 `DumpTotalMade()` 세 표 합계 비교로 바꿨다. 지원 타입의 비용이 모두 0이라 총계가 그대로인 것이 원인이다(아래 진단). `SumTotalMade` 도우미를 추가했다.
- 제품 코드는 2026-11-17 중단 커밋(`f9ee2ffc3`) 그대로다. `HostSession.TryPopFortTerrain`·`RawPostPop.RequiresDedicatedPostPop`·`TerrainPopObject` 결선은 손대지 않았다.

## 진단 (임시 검사로 실측, 검사 삭제 후 파일 제거)

- `thewarbegins` 적재: 지형 생성 뒤 `TerrainPopCalls` 1673건, 요새 적재 뒤 1834건(+161). `FortPopSkips` 40건 전부 `dedicated-r6`이다.
- 새로 등록된 161건은 표면 157·섬 94·종유 95(비용 0, `HandlesSurface`·`HandlesIsland` 분기)이며, 비용이 있는 40건(71·75·78·89·122·123·124·158)은 `RequiresDedicatedPostPop`이 참이라 기록만 남는다.
- 장부: `TotalCost` 0→0(그대로), `TotalMade` 지역 합계 1673→1813·전역 합계 1673→1834. 비용 0 타입도 개수 장부는 누적되므로 단언을 여기로 옮겼다. 가설 (b)(관문 차단)·(c)(장부 불일치)는 아니다. `TerrainPopCalls`의 type 목록과 `RiftTypeTable` 비용 대조로 판정했다.

## L2 재측정 (같은 시드 1760000000, HJOW-Athlon 실측)

| 경로 | cpp 슬롯 | dn 슬롯 | 공통 | cpp전용 | 타입일치 | 소유자일치 | 그래프일치 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| thewarbegins | 1941 | 1885 | 1885 | 56 | 1641 | 497 | 240/255 |
| tutorial1 | 843 | 834 | 834 | 9 | 763 | 140 | 254/255 |
| bc1menu76 | 952 | 848 | 846 | 106 | 735 | 213 | 254/255 |

- 0.606(VM-W11-Codex) 대비 thewarbegins: dn 1856→1885(+29), cpp전용 85→56(-29), 소유자 464→497(+33), 타입 1633→1641(+8). 요새 지원 타입 등록 효과다.
- tutorial1은 동일, bc1menu76은 cpp전용 111→106(-5)·소유자 204→213(+9). dn전용 2건(저번위 지형 슬롯 10·20)은 지형 생성 차이이며 요새 결선과 무관하다.
- 덤프는 `extracted/dotnet-l2-0607/` 아래 경로별로 보관했다(`summary.json`은 마지막 경로만 남기는 도구 동작이라 3경로 수치는 위 표가 기준이다).

## 검사

- 신규 포함 Client 전체 1265/1265 통과(Release). 수정 전 1264/1265(신규 단언 1건 실패)에서 해소됐다.
- Assets 선별 70건 통과. Core 전체 회귀는 이번 단위에서 돌리지 않았다(마지막 전체 측정 0.589).

## 남은 경계 (R4-2 잔여)

- 비용이 있는 요새 타입 8종(71·75·78·89·122·123·124·158)의 실제 Pop·후처리는 R6 파생 본체 대기다. `FortPopSkips`의 `dedicated-r6` 기록이 그 목록이다.
- 농지(121 대 97)·그래프 per-Pop 이후·첫 지역(+0x78)·AI·정기 처리는 0.606 분석 그대로 R4-2~R4-5 잔여다. 31경로 전체 재수집은 다음 조각이 끝날 때 같은 도구·같은 시드로 반복한다.
- `extracted/`의 0.607 일회성 스크립트(`swap_fort_hook.py` 등)는 이 PC에 없어 삭제할 것이 없었다. `mesa/`·`dn-gl-run/`도 이 PC에 없다(HJOW-Athlon은 실장치 실행).
