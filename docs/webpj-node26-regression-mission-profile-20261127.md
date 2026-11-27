# 관찰 네 단위 뒤 순차 Node26 전체 회귀

WB04-E1452, 2026-11-27 HJOW-Athlon. Node26.13.0 전체 **6,923/6,923 통과**, 실패·취소·건너뜀·todo0·종료0,772038.3005ms(772.038초)다. 기존 `node --test --test-concurrency=2 tests/*.test.mjs`와 오라클/단언을 유지했다. E1450과 같은 E1440/E1447 제품 입력이며, 관찰·문서 네 단위(E1445/E1444/E1448/E1449) 뒤 정기 주기로 실행했다. 시작 HEADfe55fd808이다.

작성 소스·설정·대응표·공개 자산 등3,545개 원시/LF SHA-256이 종료까지 모두 같다. Node26 종료/해시/개수를 확인한 뒤 E1453 Node22를 순차 시작했다. 두 전체는 동시에 실행하지 않는다. Node22 종료/검사까지 작성 입력 고정을 유지한다. 현재 v26=0/4·v22=8/8(Node22 실행 중)이며 회귀 자체는 주기에 더하지 않는다.

Git 제외 E1454 성능 후보의 부분 RGBA 갱신·프레임 전송 합산·독립 버퍼 행 복사는 이 회귀에 포함되지 않았다. 후보 Node/브라우저/CPU 관찰과 구별하며 Node22의 통과를 미리 주장하지 않는다.

[원문](webpj-e1452-evidence/node26.log) · [시작 고정](webpj-e1452-evidence/source-freeze.json) · [종료 검증](webpj-e1452-evidence/source-freeze-check.json) · [순차 상태](webpj-e1452-evidence/sequential-state-at-node26-end.json).
