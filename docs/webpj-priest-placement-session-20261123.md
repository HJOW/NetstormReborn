# webpj 사제 배치 검사(MayPlace)를 Session 에 설치 (WB04-E1269, 2026-11-23)

[LEFT_JOBS 점검](webpj-left-jobs-audit-20261123.md)(E1263)이 3.8절 "입력·장치"에 남긴 `raw-priest-placement-*` 미연결의 도달성을 확인하고 연결했다.

## 문제

`RawPriestSpawn.spawn`(사제 생성)은 나선으로 후보 칸을 찾으며 **후보 칸의 spot 이 0 이 아니면** 그 칸에서 `mayPlace`(사제 배치 검사)를 부른다. 이 장치는 `RawPriestWorldBindings.bindPlacement()` 로 넣게 돼 있는데 Session 이 한 번도 부르지 않아, 그 지점에서 `사제 월드 배치 검사 몸체 미연결` 오류가 났다.

도달 경로: 건물이 파괴될 때 안에 갇힌(붙잡힌) 사제를 풀어 주는 `releaseContained`(`raw-campaign-session-damageable.js`)가 건물 자리에서 사제를 만든다. 이 해방은 건물을 지우기 전이라 건물 발자국의 spot 이 아직 0 이 아니므로 첫 후보 칸이 곧바로 이 판정을 거친다(`priestFree.wav` 도 이 경로의 소리다). 단독 미션·스커미시에서 적에게 붙잡힌 사제를 되찾는 상황에서 실제로 밟을 수 있고, 31개 미션·18분 스커미시 e2e 는 이 상황을 만들지 않아 지금까지 드러나지 않았다.

## 연결

- `www/core/raw-campaign-session-construction.js` `bindGameplayPlacement()` 끝에서 `#bindPriestPlacement()` 를 부른다. 사제 체인(`raw-priest-placement-*` 여섯 모듈: 모양 → 지형 → 기하 → 충돌 → 미리보기 → 최종 접두)을 만들어 `o.priests.bindPlacement()` 로 넣는다.
- 원문은 사제와 일반 배치가 12×12 차단 배열·관계·충돌·기하·모양 전역을 공유하므로 일반 배치 파이프라인과 같은 상태 묶음(`o.state.gameplayPlacement`)을 빌리고, 사제 전용 지형 지역 배열(`createPriestPlacementTerrainState`)만 따로 갖는다. 표면 지도는 일반 파이프라인과 같은 `hash.view(0)`, 프레임·기준점·SHP 는 같은 `PriestPlacementAssets` 자료다.
- 호출 때마다 현재 로컬 Player(`playerState.battleOwner`)·클라이언트 모드(`processState.server`)·무시 genus(`pushGenus`)를 같은 전역에 써 넣는다(원본은 전역을 그때그때 읽는다).

## 검증

- `tests/raw-campaign-session-gameplay-placement.test.mjs` 새 시험: 설치 전에는 점유된 첫 후보 칸(`snap(100.25)`=101 이라 (101,101) 점유)에서 `사제 월드 배치 검사 몸체 미연결` 오류이고 아무것도 만들지 않으며, `connectGameplayPlacement()` 뒤에는 같은 입력이 오류 없이 사제와 딸린 객체를 만들고 로컬 Player 가 일반 배치 전역에 반영된다. 변형 확인: 연결 호출을 빼면, 현재 Player 갱신을 빼면 각각 실패한다(원복 확인).
- 사제 배치 모듈 여섯 개의 원본 관측 시험(`priest-placement*.test.mjs`)은 기존대로이며 이번 변경은 Session 조립만 더했다.
- Session·사제 관련 Node 시험 묶음(`raw-campaign-session-*.test.mjs`·`priest-*.test.mjs`) 778건 통과(로그 `extracted/webpj-e1269-node26.log`).

## 남은 것

- 이 시험은 연결과 호출을 확인하며, 실제 미션에서 붙잡힌 사제를 풀어 주는 장면을 자연스럽게 만든 e2e 는 아니다(붙잡힘 상태를 직접 만들어야 해서 비용이 크다). 해방된 사제의 자리가 원본과 같은지는 원본 관측 시험이 모듈 단위로 확인한다.
