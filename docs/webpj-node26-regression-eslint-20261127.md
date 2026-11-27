# ESLint 적용 뒤 Node26 전체 회귀

WB04-E1405, 2026-11-27, HJOW-Athlon. E1400 ESLint/webpack 선행 검사와 E1406 동일 번들의 갱신 시각 보완 뒤 Node26.13.0 전체 검사를 실행했다. `node --test --test-concurrency=2 tests/*.test.mjs`가 **6,897/6,897 통과**, 실패·취소·건너뜀·todo0, 종료0이었다. 실행 시간은1174.102초다.

전체 검사 동안 작성 JS·설정·패키지·운영 번들2222파일을 고정했다. 시작과 종료의 원시 SHA-256이 모두 같았고 LF 정규화로만 같아지는 변경도 없었다. 검사에 포함된 E1404 미커밋 메모리 계측 도구와 ESLint 등록도 고정 목록에 포함한다. 별도 메모리 후보와 AI 조건 변형은 Git 제외 `extracted`에서만 실행했으며 본 소스를 바꾸지 않았다. 다른 세션의 dotnetpj 작업은 웹 고정 대상 밖이다.

이전 E1397의6,895개에서 E1400 webpack 선행 관문 검사가 두 개 늘었다. 이번 결과는 선행 관문의 실제 임시 webpack 생성 검사도 포함한다. 게임 화면·음향·허용 미션 자연 승리 검증을 Node 통과로 대체하지 않는다.

[전체 원문](webpj-e1405-evidence/node26.log), [시작 해시2222개](webpj-e1405-evidence/source-freeze.json), [종료 대조](webpj-e1405-evidence/source-freeze-check.json), [환경·종료값·증빙 해시](webpj-e1405-evidence/provenance.json)를 보존했다. v26 주기를0/4로 다시 시작하고, E1397 뒤 누적한 v22 주기는5/8을 유지한다. 다음은 E1404 진단 콜백 참조 수정과 실제 미션 반납 재검증이다.
