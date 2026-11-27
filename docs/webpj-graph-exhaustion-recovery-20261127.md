# 실제 월드의 그래프 풀 소진 복구 연결

WB04-E1416, 2026-11-27, HJOW-Athlon. 캠페인1-6의 실제 아군 AI 장기 실행에서 `그래프가 소진되어 전체 풀 재구성이 필요합니다` 오류를 재현했다. E1413 제단 피해 수정 후보를 적재한 영어 앱에서 첫 오류는 게임1882초·고정 프레임75395, 수집 시점은75,000관측프레임·1884.175초이며 오류2개로 종료1했다. 일본어 UI 변경 전에 적재한 앱이므로 일본어와 관계없다. 제단 예약 피해의 앞선 오류가 다시 발생한 것도 아니다.

실제 `RawSurfaceWorld`가 `RawGraph`에 `recovery: 'fullPool'`을 전달했으나 공용 열거값 `GRAPH_RECOVERY.fullPool`은 `'FullPool'`이다. 문자열 대소문자 차이로 Add/Detach/Inspect에서 전체 풀 복구를 선택하지 못했다. 연결부를 공용 상수로 교정했다. `RawGraph`의 기본 Reject 정책, 전체 재구성 몸체, 번호251개와 공유 표/풀은 바꾸지 않았다. 읽기 전용으로 확인한 현재 C++ 클론의 `RawSurfaceStorage::Graph()`도 `GraphRecovery::FullPool`을 사용한다.

## 회귀 검사

실제 월드를 조립하고 초기 표면을 적재한 뒤 기존 할당기로 남은 번호를 모두 예약한다. 떨어진 표면을 정상 Create/Frame/Pop 경로로 추가하면 수정 전에는 실게임과 같은 `executeAllocate → executeAdd → SquidPostPop` 오류가 발생한다. 수정 뒤에는 전체 풀을 재구성해 Pop이 성공한다. 같은 풀·레코드 배열의 정체성, 기존 다리 SID, 새 표면, 실제 연결 성분 수와 표면 합을 검사한다. 성공값이나 가짜 Graph를 주입하지 않는다.

최초 검사 작성에서 존재하지 않는 `world.setFrame`을 사용한 오류를 고쳐 공개된 `core.frame.set` 경로로 재현했다. 또 원문 Add가 재구성 직후 빈 번호도 예약한다는 점을 반영해 빈 예약의 정확한 개수 대신 실제 표면 성분·합·빈 번호 확보를 검사했다. 최초 도구 오류와 기대값 실패를 숨기지 않고 별도 원문으로 남긴다.

`raw-surface-world`, `raw-graph`, `graph`, `squid-postpop` 집중 **86/86**, 실패·취소·건너뜀0·종료0이다. 기존 원본10.78 graphrebuild96행과 다른 Graph/Pop 계약도 유지한다. 새 검사가 기존 `SURFACE_GRAPH` 상수를 사용하면서 no-unused-vars 예외 한 건이 불필요해져 webpack 선행 ESLint가 빌드를 실제 차단했다. `--prune-suppressions`로 해당 파일의 예외2→1을 줄였고 새 규칙 예외는 추가하지 않았다.

## 실제 AI 후속 범위

수정된 본소스의 ES 모듈을 그대로 적재해 같은1-6/ForceEnemy3 조건으로120,000프레임 상한의 실제 아군 AI 후속을 시작했다. 시작 SP4000·동맹·이름·기술은 보존하며 모듈 경로를 가로채지 않는다. 실제 Add가 번호 소진 상태로 호출되는 순간만 전후 레코드 수·게임 시각·오류를 읽는다. 게임 명령·목표·HP·SP·완료값을 합성하지 않는다. 최종 Canvas 제출만 미루며 내부 Renderer/Screen/Kernel 프레임은 모두 실행한다. 1-6 뒤의 Next Mission은 누르지 않는다.

**구현 검증 완료·실게임 후속 진행 중:** 선행 ESLint/운영 빌드 종료0, 재번들 동일성/webpack 관문3/3·영어 L2 1/1을 통과했다. 운영 번들은3,508,534바이트, SHA-256 `72ea8d094b55216452615ed982bceb46e82ab3166957eb47a5225108e2bd7b25`다. 기존 미사용 선언 예외는177→176건(no-unused-vars89·no-useless-assignment51·no-unused-private-class-members36)이다. 코드/설정 검사들은 각각 별개 실행이며 합산하지 않는다.

본소스의 실제 AI 후속은 구현 기록 시점에42,000관측프레임·1059.175초·게임 오류0이다. 기존75,000관측프레임을 아직 지나지 않았고 자연 승리/저장은 미완료다. E1419 후속 관찰로 계속하며, 구현 단위 완료를 승리 검증으로 세지 않는다. 시작부터 이 표본까지 ES모듈1070파일 SHA가 모두 같다.

[수정 전 회귀](webpj-e1416-evidence/before.log)·[수정 후86건](webpj-e1416-evidence/focused.log)·[빌드](webpj-e1416-evidence/build.log)·[번들/관문3건](webpj-e1416-evidence/bundle-gate.log)·[영어 L2](webpj-e1416-evidence/l2.log), [최초 실게임 오류/덤프](webpj-e1416-evidence/original-ally-force3-altarfix-dissolvedalliance-failure.json)·[그 화면](webpj-e1416-evidence/original-ally-force3-altarfix-dissolvedalliance-failure.png)·[고정된 후속 표본](webpj-e1416-evidence/fixed-through-current.log)과 [환경/해시](webpj-e1416-evidence/provenance.json)를 보존했다.

## E1419 실제 후속과 사용자 범위 변경

실제1882초에251→18·같은배열복구를확인하고116,000관측프레임/2909.175초까지오류0이었다. 사용자추가지시로1-6승리검사를생략하고해당실행을중단했다. 최종상태/승리/저장은추정하지않는다. [원문·중단·경계](webpj-graph-live-recovery-stop-20261127.md).
