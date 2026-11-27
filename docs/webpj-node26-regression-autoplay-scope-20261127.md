# 사용자 승리 검사 범위 반영 뒤 Node26 전체 회귀

WB04-E1429, 2026-11-27, HJOW-Athlon. Node26.13.0 전체 **6,910/6,910 통과**, 실패·취소·건너뜀·todo0, 종료0이다. 명령은 `node --test --test-concurrency=2 tests/*.test.mjs`, 시간은891468.9047ms(891.469초)다.

E1428 Node22 종료와 입력 해시 불변을 확인한 뒤 시작했으며 두 전체 검사는 동시에 실행하지 않았다. 시작 소스는 `f6a55f3a7`, 입력3539개는 양쪽에서 같았다. Node26 종료에도 모든 원시 SHA-256이 같고 줄바꿈만 달라진 파일도0이다. 현재 일본어 UI·ESLint/webpack·실제 FullPool 복구·스커미시 난이도 보존과 사용자1-3/1-4/1-6 승리 수집 거부를 포함한다. 기존 오라클과 단언을 완화하지 않았다.

두 전체 검사가 끝나 작성 입력 고정을 해제한다. 실행 중 마친 문서 단위 E1428은 입력 변경이 없고 전체 검사 완료 자체는 구현 단위 주기에 더하지 않는다. v26=0/4·v22=0/8로 재시작한다. Git 제외 스커미시·채취량 후보·실제 rAF 메모리·한국어 첫 안내 관찰은 별도이며 이 전체 통과를 해당 결과로 세지 않는다.

[전체 원문](webpj-e1429-evidence/node26.log)·[시작 고정](webpj-e1429-evidence/source-freeze.json)·[종료 대조](webpj-e1429-evidence/source-freeze-check.json)·[순차 두 결과](webpj-e1429-evidence/sequential-run-state.json)·[출처/해시](webpj-e1429-evidence/provenance.json). Node22의 처음 실행 파일 경로 오류는 [E1428](webpj-node22-regression-autoplay-scope-20261127.md)에 보존했다.
