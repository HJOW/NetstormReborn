# 실제 Client GameAux·Outpost 기술 반영 뒤 Node26 전체 회귀

WB04-E1450, 2026-11-27 HJOW-Athlon. Node26.13.0 전체 **6,923/6,923 통과**, 실패·취소·건너뜀·todo0·종료0이다.847930.4057ms(847.930초), 기존 `node --test --test-concurrency=2 tests/*.test.mjs`와 오라클/단언을 유지했다.

E1440 실제 Client의 건설 차단·충돌 마스크·지역 요청량·공급자 순환 공유3검사와 E1447 스커미시 Outpost 시작 기술1검사를 포함한다. 시작 HEAD8c59c80d7의 작성 입력3,545개 원시/LF SHA-256이 끝까지 전부 같으며 줄바꿈만 다른 파일도0이다. Node22 전체는 현재 주기가 아직 차지 않아 동시에 실행하지 않았다.

고정 중 실제 관찰과 문서만 마감한 E1445·E1444·E1448 세 단위를 이월한다. 전체 회귀 자체는 주기에 가산하지 않는다. 현재 v26=3/4·v22=7/8이며 다음 완료 단위 뒤 두 버전 전체를 순차 실행한다. 소스 고정은 해제했다. 이 전체 통과가 허용 밖 캠페인 승리·사람 언어 검수·물리120FPS를 의미하지 않는다.

[원문](webpj-e1450-evidence/node26.log) · [고정 입력](webpj-e1450-evidence/source-freeze.json) · [결과/해시](webpj-e1450-evidence/source-freeze-check.json) · [실행 상태](webpj-e1450-evidence/run-state.json).
