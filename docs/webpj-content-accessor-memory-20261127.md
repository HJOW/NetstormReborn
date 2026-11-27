# 내용물 통지 getter의 이전 세션 참조 해제

WB04-E1431, 2026-11-27, HJOW-Athlon. 실제rAF·아군AI1-1의 진입/1분 진행/원래 메뉴 나가기를 반복하던 중 두 번 모두11회차에서 마지막 World/Session이 회수되지 않았다. 기존 E1404의 짧은6회 검증으로는 드러나지 않은 수명 문제다. 게임 오류는0이었지만 회수 단언은 실패했으며 두 실패를 보존했다.

두 번째 실패에서 heap snapshot을 단언 전에 수집하고1초 추가 대기/GC를 해도 같은 객체가 남았다. 약한 간선과 WeakMap 조건부 간선을 제외해 본 경로는 `FeedbackVector → AllocationSite → 객체 형상/AccessorPair → get problemCode → o → RawCampaignSessionOwners → World/Session`이었다. V8가 보관한 getter의 문맥이 이전 세션을 강하게 참조하는 관찰이다. 첫 단순 BFS의 WeakMap 경로를 확정 원인으로 사용하지 않았으며, 무한 누적 누수를 입증했다고도 세지 않는다.

`raw-campaign-session-contents.js`에서 Player 내용물 통지의 상태 객체를 별도 팩터리로 옮겨 약한 주소만 캡처한다. 원래 SP 저장소·사제 재생 검사·진단 전역 주소는 각각 WeakRef로 보존하고 Player/TYPE/사건은 현재 owner에서 읽는다. 현재 세션이 살아 있는 동안 같은 읽기/쓰기 순서를 유지하며 회수 뒤 호출은 명시 오류다. 원본 자원 검사·SP·체력·목표·승패 코드는 바꾸지 않았다.

집중 Node14/14, ESLint 선행 운영 빌드 종료0, 재번들 동일성/production·development 실패 관문3/3, 기존 C++ 오라클의 화면·시계·난수·덤프를 그대로 대조하는 Chromium L2 1/1을 통과했다. 번들은 3,508,979바이트·SHA-256 `11a920aa8c8ebb4d914fb2421541eb74335e2cf7f17cd646edec46b1a9f5f12d`다. 기존 오라클/단언을 완화하지 않았다.

수정 뒤 실제rAF 반복은 현재 15회 연속 모든 World/Session 회수·게임 오류0이다. 원래 실패했던11회차를 지났음을 확인했다. 최소30분·20회 관찰은 계속 실행 중이며 E1435에서 별도로 완료 판정한다. 이번 단위를 장시간 M5 전체 완료나 물리120Hz 확인으로 세지 않는다. v26=2/4·v22=2/8이다.

[첫 실패](webpj-e1431-evidence/first-failure.log)·[두번째 실패/추가대기](webpj-e1431-evidence/second-failure.log)·[강한 경로](webpj-e1431-evidence/heap-strong-path.txt)·[수정 뒤 현재 관찰](webpj-e1431-evidence/fixed-progress.log)·[집중](webpj-e1431-evidence/content-focus.log)·[빌드](webpj-e1431-evidence/build.log)·[번들/관문](webpj-e1431-evidence/bundle-lint-focus.log)·[Chromium L2](webpj-e1431-evidence/chromium-l2.log)·[출처/해시](webpj-e1431-evidence/provenance.json). 약85MB heap 원문은 Git 제외 조사 경로에만 두고 경로/해시를 근거에 기록했다.
