# 11차 난이도 계산과 AiCreate 연결 이식 (WB04-E554·E555)

`o/AiDifficulty`(11차 난이도 배율과 10차 NB10 강제 적 정리)를 `webpj/www/core/ai-difficulty.js`로 옮기고, 같은 기준선에서 이미 바뀌어 있던 `o/RawAiCreate`의 두 hook(강제 적 정리·난이도)을
웹의 `RawAiCreate`(`www/core/raw-ai-brain.js`, E164)에 연결했다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다. 이 모듈은 DOM·설정 파일·세션에 의존하지 않는다.

## 옮긴 범위

- `aiDifficultyFromValue`/`aiDifficultyToValue`: 저장값 1은 쉬움, 3은 어려움, 그 밖(0/누락/범위 밖/음수)은 보통(2)이며 왕복이 맞는다. 상수 `AiDifficulty`·`DIFFICULTY_CONFIG_KEY='difficulty'`·`EASY_CYCLE_FACTOR=4`·`HARD_CYCLE_FACTOR=0.5`.
- `applyAiDifficulty(settings, level)`: 600바이트 설정의 **사본**을 돌려준다(입력 불변). 보통은 같은 바이트이고, 쉬움은 +0x220/+0x224/+0x228 세 주기(단정도)에 4배, 어려움은 0.5배를 곱하며(0 이하/비유한 주기는 그대로),
  쉬움만 +0x234 채취 유닛 수가 1 이상일 때 절반(내림, 최소 1)으로 줄인다. 크기가 600바이트가 아니면 거부한다.
- `sanitizeAiForcedEnemy(settings, owner)`(10차 NB10): 강제 적(+0x240)이 소유자 범위(1..8) 밖(부호 없는 값)이거나 자기 자신이면 0(자동 선택)으로 바꾼 사본을, 바꿀 것이 없으면 `null`을 돌려준다.
- `RawAiCreate.create`: 색/신전 분기 뒤 생성 직전에 ① 강제 적 정리 ② (hook이 있으면) `difficulty(owner, 정리된 600바이트)`의 저장값으로 난이도 배율을 적용해, 생성자와 초기 갱신에 **같은 사본**을 넘긴다(입력 설정 불변, 보통이면 사본을 만들지 않음).

## 시험

`webpj/tests/ai-difficulty.test.mjs` 7건: cpppj `AiDifficultyTests.cpp`의 다섯 시험(값 해석/왕복·NB10 정리·보통 비트 보존·쉬움 주기 4배/채취 절반(내림·최소 1·0 유지)·어려움 주기 절반과 크기 거부)과
`RawAiCreate` 연결(난이도 hook의 인자·생성자/초기 갱신의 같은 사본·입력 불변·보통/0/범위 밖은 배율 없음), 변형 확인으로 보강한 경계(601바이트 거부·owner 0의 자동 선택·알 수 없는 단계의 어려움 경로·0 이하/비유한 주기·음수 채취).
원본에 난이도가 없어 원본 관측 fixture는 없고 manifest 항목도 추가하지 않았다. 기존 `ai-brain.test.mjs` 160행/AiCreate 시험은 그대로 통과한다.

**변형 확인:** 상수·오프셋·검사·해석·배율·절삭·정리 조건 34가지를 넣어 30가지가 시험에서 잡혔다(처음 8가지가 살아남았고 경계 시험으로 4가지를 잡았다). 남은 4개는 동등 변형이다:
저장값의 `>>>0`↔`|0`(1/3과의 비교라 같은 저하위 비트), 비유한 검사 제거(`Infinity`는 4배해도 `Infinity`), 0 이하 검사의 `>0`↔`>=0`(0×배율=0), 단정도 저장 전의 `Math.fround`(`setFloat32`가 같은 반올림을 한다).

## 남은 경계

- 옵션 메뉴·스커미시 화면의 난이도 선택 UI와 설정 파일(`difficulty` 키) 연결, 실제 Session의 `difficulty` hook 공급은 client 단계(5·6)의 일이다. 12차 스커미시는 AI별 난이도를 이 hook으로 넘긴다.
