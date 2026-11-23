# webpj L3: Tutorial3 (Capture The Priest) 시작 구간 (WB04-E1309, 2026-11-24)

Tutorial1·2 와 같은 틀(`e2e/l3-common.js`·`tools/l3-collect.mjs`)로 **Tutorial3**(Capture The Priest)을 cpppj 와 대조하기 시작했다. 첫 구간(시작 안내 창 두 쪽과 닫은 뒤 시작 화면, 240프레임)은 화면 0픽셀·덤프 일치(환경 의존 행 `sp`·`player:*` 제외)이고 웹 코드는 바꾸지 않았다.

## 틀

- 폴더 `tests/fixtures/cpppj/candidate-225553df/tutorial3-l3/`(단계표 `steps.json`·출처 `oracle.json`·체크포인트), 웹 시험 `e2e/l3-tutorial3.spec.js`(Tutorial2 스펙을 미션 이름·폴더만 바꿔 복제).
- 오라클 수집: `node webpj/tools/l3-collect.mjs tutorial3-l3`.

## 미션 개요(영어 스크립트 `tutorial3.english`)

시작 SP 2000, 시작 기술 sunArcher·sunWalker, 신전·워크샾이 이미 지어져 있고 적 AI 는 끈다(`aiOff`)·적은 사제만 있는 섬이다. 단계 흐름: A 골렘 만들기(다리로 간헐천에 잇고 골렘이 첫 크리스털을 신전에 가져올 때까지) → B 적 섬에 다리로 잇기(남쪽) → C 발사기 생산 → D 제너레이터(Wind Generator 생산 투입) → … 적 사제 포획·희생. 단계는 cpp `report` 의 `mission_gate` 두 번째 값(`A`=65…)으로 본다.

## 입력(1024×768, Tutorial3)

| 프레임 | 동작 | 인자 | 결과 |
| --- | --- | --- | --- |
| 100 | snapshot·session-dump | `a100` | 'Capture The Priest!' 안내 창으로 정지(SP 2000, 내 섬·간헐천 셋 보임) |
| 120 | click-if | `MORE` | 'Build Golems'(BACK·OK) |
| 160 | snapshot | `a160` | 둘째 안내 창 |
| 180 | click-if | `OK` | 안내 창 닫기 |
| 240 | snapshot·session-dump | `a240` | 자유 진행(단계 A, 골렘 생산·간헐천 다리 연결이 목표) |

## 남은 일

단계 A(간헐천 다리·골렘 생산·첫 크리스털) → B 적 섬 잇기 → C 발사기 → D 제너레이터 → 적 사제 포획. 전투·적 이동이 길게 이어지므로 체크포인트를 짧은 간격으로 두고 어긋나는 첫 프레임을 `mission_gate`·프레임별 덤프(`runL3({ compare: false })`)로 좁힌다.
