# webpj 생산 조각 문자 키(ProductionShortcuts) 이식 — WB04-E36

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 서른여섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 `client/ProductionShortcuts`(10.78)를 옮겼다. 원문 Userinput 일반 키 구간의 생산 조각 단축키를 실제 40칸 입력 큐에 예약하는 분배이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/production-shortcuts.js` — `queueProductionShortcut(event, paused, hooks)`

- Guide/Gump 뒤의 일반 키 구간. 키 뗌이거나 정지(깊이>0)면 예약하지 않고 `false`(modifier와 활성 커서는 추가로 거부하지 않음). 수정키를 지운 코드를 문자로 해석한다.
- `q/Q`→슬롯 0, `w/W`→1, `a/A`→2, `s/S`→3, `z/Z/y/Y`→4, `x/X`→5: **예약 직전에** 현재 명령 전역(`hooks.command()`, `0x020000f4` = 현재 기술의 slot BYTE 취득)을 읽어 `hooks.queue(명령, 슬롯, 0)`.
- `e/E`: 현재 슬롯 수와 무관하게 여섯 명령을 FIFO로 예약하며 **매번** 현재 명령을 다시 읽는다.
- `d/D`: 별도 최근 항목 명령(`hooks.repeatCommand()`, `0x02000107` = 현재 lastType과 일치하는 생산 항목 재취득)을 소비 직전에 읽어 `(명령, 0, 0)`으로 예약하고 다리 슬롯 명령은 읽지 않는다.
- 그 밖의 문자는 소비하지 않는다. 선택된 문자만 명령 조회/큐 예약 장치를 요구하고, 중간에 큐가 실패해도 앞선 예약은 되돌리지 않는다. 포화 정책은 `InputQueue`(39개까지, 가득 차면 방금 넣은 사건 폐기)가 수행.

## 2. 검증 (`tests/production-shortcuts.test.mjs`, 2건)

- `production-shortcuts-x86.tsv` **8040행**: 원본의 앞선 관문(Guide/Gump 소비·뗌·콘솔)에서 끝나는 3760행은 도달 여부(`pausedCalls == 사건 수`)만 확인하고, 일반 키 구간에 도달한 **4280행**을 실제 `InputQueue`(독립 입력의 읽기/활성 고리 위치와 기존 활성 슬롯 표식을 공개 push/pop으로 재현) 위에서 대조한다: 사건별 예약 횟수·`queue:<명령>:<슬롯>:<셋째>` 호출 순서·현재 명령 조회 수·남은 활성 큐 전체(`code:x:y` 열).
- 계약: 뗌/정지/비선택 문자(`?`)와 열 키 전부에서 장치 비요구, `E`의 세 번째 큐 경계 중단 뒤에도 앞선 두 예약(`0x020000f4`, `0x020000f5`)과 읽기 3회 보존, `D`가 최근 항목 명령만 읽음, 선택 문자의 명령 조회/큐 누락 거부.

최초 실패와 교정: 2건이 처음부터 통과했다(모듈에 잘못 들어간 빈 `switch` 블록은 첫 실행 전에 제거).

## 3. 경계

- `D/d` 최근 항목 경로는 원본 관측 행이 없고 cpppj도 계약 검사만 한다(관측 fixture의 도달 행은 `repeatCommand`를 설치하지 않음). 실제 최근 TYPE 소비자(`getlastitem`)는 생산 UI 단위에서 연결한다.
- 현재 명령 전역(`00542ac4`/`00542b10`)과 정지 판정(`00460de0`)은 훅/호출자 값이다. 커서 회전·Drop 키는 별도 원본 구간이다. CD 10.72는 범위 밖이다.

## 4. 다음 시작점

`HtmlInputDialog`(109행)·`DialogCommandLifetime`(258행)·`LanguageOptions`·`RawSquidPoolReset`·`TutorialInit`·`TraceChat`·`State.cpp`. 큰 묶음은 `RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawBridgeConnect`, Player 나머지이다.
