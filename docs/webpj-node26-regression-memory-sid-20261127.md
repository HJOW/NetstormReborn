# 메모리 수명·SID 회복 반영 뒤 Node26 전체 회귀

WB04-E1438, 2026-11-27, HJOW-Athlon. Node26.13.0 전체 **6,912/6,912 통과**, 실패·취소·건너뜀·todo 0, 종료 0이다. 원래 명령 `node --test --test-concurrency=2 tests/*.test.mjs`와 단언을 유지했고 1126952.8814ms(1126.953초)가 걸렸다.

입력 기준은 `bf50da040005de761e4a0e24594a2aff2389d265`다. E1431 내용물 접근자 수명 수정, E1433 실제 SID 회복과 Native 47행 추적 등록, E1435 장기 메모리 도구를 포함한다. 작성 소스·설정·대응표·공개 자산 등 3,541개 파일의 원시 SHA-256이 시작·종료 때 모두 같고 줄바꿈만 달라진 파일도 0이다. 실행 중 별도 AI/네이티브 입력 오라클은 Git 제외 후보만 사용했다. 이 전체 결과를 후보의 실제 플레이 검증으로 세지 않는다.

첫 실행은 새 Native fixture 등록 누락 1건을 확인한 뒤 자체 검사 트리를 중단한 부분 실행이다. [그 실패와 등록 보완](webpj-node26-sid-fixture-registration-20261127.md)을 유지하며 이번 통과와 구별한다. 이번 재실행 종료까지 작성 입력을 고정했고 다른 Node 전체와 동시에 실행하지 않았다.

전체 통과 자체는 구현 단위 주기에 더하지 않는다. 입력 고정을 해제하고 v26=0/4, v22=4/8로 이어간다. Node22의 최신 전체 결과는 E1428의 6,910/6,910이며 이번 6,912개를 Node22에서도 실행했다고 해석하지 않는다.

[원문](webpj-e1438-evidence/node26.log) · [시작 고정](webpj-e1438-evidence/source-freeze.json) · [종료 해시/결과](webpj-e1438-evidence/source-freeze-check.json) · [실행 상태](webpj-e1438-evidence/run-state.json) · [수집기](webpj-e1438-evidence/full.py) · [출처/해시](webpj-e1438-evidence/provenance.json).
