# 실제 수집 유닛 상한·Outpost 삭제 반영 뒤 Node26 전체 회귀

WB04-E1441, 2026-11-27, HJOW-Athlon. Node26.13.0 전체 **6,919/6,919 통과**, 실패·취소·건너뜀·todo0·종료0, 855328.0132ms(855.328초)다. 기존 명령 `node --test --test-concurrency=2 tests/*.test.mjs`와 오라클/단언을 유지했다.

E1436의 현재 간헐천 집계·건설 중 합산 생산 상한과 새4개 검사, E1434의 Outpost 현재 두 목록/지역 통지/피해 부모 삭제 연결과 새3개 검사를 포함한다. 시작 소스/실행 파일 SHA는 고정 JSON을 따른다. 작성 소스·설정·대응표·공개 자산 등3,543개의 원시 SHA-256이 종료까지 전부 같고 줄바꿈만 다른 파일도0이다.

Node26이 종료하고 해시/개수를 확인한 뒤 같은 입력의 E1442 Node22를 순차 시작했다. 두 전체를 동시에 실행하지 않았다. 작성 입력 고정은 Node22 종료·검사까지 유지한다. Git 제외 GameAux 수정 후보, 실제 최종 소스 와이드 AI/장기 간헐천 관찰은 별도이며 이 전체 통과를 후보 반영/그 관찰의 완료로 세지 않는다.

전체 회귀 자체는 완료 단위 주기에 가산하지 않는다. 현재 v26=0/4·v22=8/8(Node22 실행 중)이다. Node22의6,919개 통과를 미리 주장하지 않는다.

[원문](webpj-e1441-evidence/node26.log) · [시작 입력](webpj-e1441-evidence/source-freeze.json) · [종료 해시/결과](webpj-e1441-evidence/source-freeze-check.json) · [순차 실행 상태](webpj-e1441-evidence/sequential-state-at-node26-end.json) · [출처/해시](webpj-e1441-evidence/provenance.json).
