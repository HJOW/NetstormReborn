# webpj AI 초기 갱신(RawAiBrainInitialUpdate) 이식 — WB04-E168

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백예순여덟 번째 부분이다. AI 한 명이 막 만들어진 직후(또는 설정이 바뀔 때) 실행하는 패치 `00414710` 초기 갱신을 옮긴 `o/RawAiBrainInitialUpdate`(cpppj 기준선 `1f00a25a`, 기준선 뒤 `.cpp/.h` 변경 없음을 `git diff --numstat` 으로 확인)이다. 설정 600바이트 재복사, 선택된 이름, 실제 기술 부모 보장/목록 추가, 기술 내용물을 순회한 건설 분류와 최대 비용을 같은 AI 소유 메모리(E164)에 적용한다. AI 계열(33개 파일) 여섯 번째 단위이다.

## 1. 옮긴 것

`www/core/raw-ai-initial-update.js`(DOM 없음)

- `new RawAiBrainInitialUpdate(memory, player, technology, finder, types, globals, hooks)` — `player`/`technology` 는 같은 PATCH 풀의 `RawPlayerWorld`/`RawPlayerTechnology`, `finder` 는 현재 타입 6/권한/죽음 진단 전역(`ContainedFinderState`), `types` 는 현재 TYPE 표의 같은 배열(`{ flags2, cost, theme }`, 복사하지 않는다), `globals = { missionMode(), excludedType(), workshops[4] }` 는 각 원본 위치에서 현재 값을 읽고, `hooks = { defaultName(), requiredEnergy(type) }` 는 도달한 위치에서만 요구한다.
- `update(token, settings)` — ① 본체/설정 크기와 이름 owner(< 9)를 확인하고 owner 를 복사 전에 캡처한 뒤 600바이트를 본체 처음에 다시 복사한다. ② 현재 mission 표식이 켜져 있으면 설정 이름(없으면 기본 언어 이름)을 Player 9기록의 +0xc 에 쓴다. ③ 현재 owner 를 매 위치에서 다시 읽어 기술 부모가 없으면 Player 를 만들고(그래도 없으면 끝), 설정의 기술 문자열(+0x1e)이 있으면 목록에 추가한다. ④ 다섯 분류 플래그(+0x3b4..)와 최대 비용(+0x290)만 0 으로 하고 빌더 개수/원소 목록 개수/+0x394/+0x390/+0xad8 은 유지한다. ⑤ 기술 부모의 내용물을 기존 커서(다음 SID 를 먼저 저장, 방문 시 현재 flags 4/TYPE)로 순회하며 이동 계열(flags2 0x30000)은 빌더 목록에 쌓고, 요구 에너지 문자열 길이로 +0xad8 을 올리고, 최대 비용을 ftol 로 올린다(NaN 비용은 0). ⑥ 분류 제외 TYPE 이 아니면 theme(0..5)별 개수를 센다. theme 5 는 실제 stack 칸이 sun battery 계수와 같은 칸을 공유하며 0x800 battery 는 짝 계수와 첫 원소 3 → theme 치환을 한다. theme 별 첫 개수(5 로 나눈 나머지 1)마다 원소 목록에 추가하고 후보 워크샾의 ftol 비용이 더 크면 후보를 교체한다. ⑦ 마지막에 단독 battery 원소를 중립 3 으로 치환하거나 한 번 제거하며 목록 개수를 압축한다. 지갑(+0x2d8)/SP 는 바꾸지 않는다.

## 2. 검증

`tests/ai-initial-update.test.mjs`(4건).

1. 실제 PE 관측 `ai-initial-update-x86.tsv` 133행 전부(고유 입력 133개): 행마다 원본 실행기와 같은 입력(패턴별 기술 내용물 free/dead/비 contained·flags 혼합, 명시 TYPE 이름/비용/원소/flags/요구 에너지, 독립 시드의 미초기화 본체/600바이트 설정, 부모·이름·기술·빌더 입력)을 준비하고 실제 `RawPlayerWorld`·`RawPlayerTechnology`·`RawContainedCreate`·`RawCarrierPreDestroy` 를 같은 풀로 연결해 `update` 후 본체 0xb08 전체, 빌더 40바이트, Player 9기록, 새 Form 전체 raw 50바이트(할당 순서), 이름/타입/부착 요청 순서를 비교하고 지갑 999 불변을 확인한다.
2. 별도 관측 `ai-initial-update-theme5-x86.tsv` 16행: 첫 TYPE 의 theme 입력(5 포함)에서 실제 stack 칸 공유·후속 battery 중립화·원소 목록 압축·전체 본체를 대조한다.
3. cpppj 계약: 선택되지 않은 경계는 요구하지 않고(빈 hooks 로 끝남) 요구 에너지 누락은 일부 효과(빌더 1개 추가) 뒤에도 오류로 남으며 지갑/새 StartMoney 를 바꾸지 않는다.
4. 추가 계약(같은 원본 분기): 기본 언어 이름 조회 누락의 도달 위치 거부(복사된 설정 보존), 설정 크기/이름 owner/가짜 본체 거부, 생성자의 같은 풀/워크샾 4/전역 조회기/TYPE 표 거부, NaN 비용의 ftol 0 규칙.

최초 실패와 교정: 새 모듈과 시험은 4건 모두 첫 실행에 통과했다(149행 포함).

## 3. 경계와 남은 것

- 실제 Player 생성/기술/내용물 하위는 기존 몸체를 같은 풀로 연결했다. 실제 TYPE 표의 `theme`(원소 분류) 필드는 현재 웹 자산 TYPE 레코드에 아직 없어 시험이 명시 입력으로 채운다. 실제 세션 연결(M2 월드 조립) 때 TYPE 메타데이터에서 공급해야 한다. 설정 이름 언어 조회(`004410f0`)와 TYPE 요구 에너지(+0xa0) 문자열도 실제 자산 연결이 필요하다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

AI 계열 나머지(`RawAiDrySpots`·`RawAiFakeDrySpots`·`RawAiBuildSearch`·`RawAiDropSelection`·`RawAiDeployment`·`RawAiResourceChoice` 등), `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
