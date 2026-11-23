# 대응표 `port` 행을 `session` 으로 올리는 판정 (WB04-E1283, 2026-11-23)

4-D 의 완료 기준(L2)을 닫으면서([난수·시각 맞춤](webpj-state-install-rng-20261123.md)) 4D-10b 가 요구한 "실제 결선 단위를 대응표에서 `session` 으로 올린다"를 도구로 했다. `PORT_MAP.tsv` 의 상태는 `todo`→`partial`→`port`(이식·검사 통과)→`session`(앱 수명 연결)→`oracle`(cpppj 오라클 비교 통과)이다.

## 규칙

- 수집: `e2e/session-coverage.spec.js`(옵트인, `NETSTORM_COVERAGE_OUT=경로`)가 고정 시각 Tutorial1(4:3)을 130프레임 도는 동안의 V8 함수 커버리지를 모듈별 `{ executed, total }` JSON 으로 쓴다(모듈 본문 전체를 덮는 최상위 함수는 세지 않음).
- 승격: `tools/port-map-session.mjs --coverage <json> --write` 가 `port` 행 가운데 **행의 웹 모듈이 모두 `executed > 0`** 인 행을 `session` 으로 바꾸고 메모 끝에 `[session WB04-E1282 Tutorial1 130프레임 실행]` 표식을 붙인다. `partial`·제외(`-`)·이미 `session`/`oracle` 인 행과 머리 행·줄바꿈은 그대로다. 같은 입력으로 다시 돌려도 결과가 같다(멱등, `tests/port-map-session.test.mjs`).
- 보수적 판정이다: 자료만 들고 함수가 없는 모듈이나 이 흐름에 쓰이지 않는 모듈이 하나라도 있는 행은 `port` 로 남는다.

## 결과 (E1283)

`port` 995행 중 **732행이 `session`, 263행이 `port`** 로 남았다(커버리지에 모듈이 없는 행 62·함수가 실행되지 않은 행 201 — 전투·AI·의식·특수 UI 처럼 Tutorial1 시작 130프레임에 쓰이지 않는 단위). 대응표 검사는 오류 0이고 `python -X utf8 tools/port_inventory.py` 의 "wb port" 열은 `session` 을 port 에 포함해 995 그대로다. 이 승격은 L2 근거(Tutorial1 4:3·16:9 화면 0픽셀)와 함께 읽는다.

## 앞으로

다른 시나리오(다른 미션·전투·AI)의 커버리지 JSON 을 같은 도구로 이어 돌리면 남은 `port` 행이 더 올라간다. `oracle` 은 cpppj 오라클과 단위별로 대조한 행에만 붙이며 이 도구가 만들지 않는다.
