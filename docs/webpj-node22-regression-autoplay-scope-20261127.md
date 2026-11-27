# 사용자 승리 검사 범위 반영 뒤 Node22 전체 회귀

WB04-E1428, 2026-11-27, HJOW-Athlon. Node22.23.5 전체 검사 **6,910/6,910 통과**, 실패·취소·건너뜀·todo0, 종료0이다. 명령은 `node --test --test-concurrency=2 tests/*.test.mjs`, 시간은846744.9484ms(846.745초)다.

시작 커밋은 `f6a55f3a7`다. 일본어 UI/자원·실제 그래프 FullPool 복구·ESLint/webpack 관문·스커미시 난이도 보존과 사용자 지정1-3/1-4/1-6 승리 수집 거부를 포함한 현재 입력3539개를 고정했다. 종료 때 모든 원시 SHA-256이 같고 줄바꿈만 달라진 파일도0이다. 기존 단언·오라클·검사 수를 유지했다.

Node22 전체 종료와 해시 대조를 마친 뒤 같은 입력의 Node26 E1429를 시작했다. 두 버전 전체를 동시에 실행하지 않았다. 실행 중에는 Git 제외 실제 AI/메모리 수집과 격리 C++ 지도 씨앗 입력 빌드만 진행했으며 웹 작성 소스·설정·공개 자산은 바꾸지 않았다. Node26 종료까지 이 고정을 유지한다.

첫 실행 준비는 Node22 경로를 `webpj/extracted` 아래로 잘못 지정해 WinError2로 중단됐다. 테스트 프로세스가 만들어지기 전 실패이며, E1411 근거에서 저장소 루트의 `extracted/webpj-node22` 실행 파일을 확인해 바로잡았다. 경로 오류도 별도 원문으로 보존했다.

v22 주기는0/8로 재시작하며 v26은4/4·E1429 실행 중이다. 이번 전체 검사는 미션 자연 승리나 물리120Hz·청취 확인을 대신하지 않는다.

[전체 원문](webpj-e1428-evidence/node22.log)·[시작 고정](webpj-e1428-evidence/source-freeze.json)·[종료 대조](webpj-e1428-evidence/source-freeze-check.json)·[처음 경로 오류](webpj-e1428-evidence/first-launch-path-error.json)·[출처/해시](webpj-e1428-evidence/provenance.json).
