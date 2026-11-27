# 정상 rAF의30분 미션 반복과 heap 회수

WB04-E1435, 2026-11-27, HJOW-Athlon. E1431 수정 뒤 같은 앱에서 실제 아군AI1-1의 진입·약1분 진행·원래 Game→Leave Mission→ABORT로 나가기를 **27회, 총1818.297초(30분18초)** 반복했다. 고정 프레임·가짜 시계·최종 Canvas 출력 생략을 쓰지 않았다. 매번 모든 이전 World/Session이 GC로 회수됐고 게임/브라우저 오류0이다. 완료 조건은 최소30분과20회이며 둘 다 충족했다.

실제 미션 대기 합계는1622.004초, 관찰된 게임 시계 합계는1625.104초다. 시각은 서로 다른 CDP/호스트 경계에서 읽으므로 이를 정밀 시계 드리프트나 물리 FPS 측정으로 해석하지 않는다. 아군AI 생성 전후 이름·동맹·기술·그 시점의 실제SP가 같고 목표/HP/승패/완료값을 쓰지 않았다. 최초 브리핑과 정상 프레임 중 원래 지급이 일어나 활성화 당시SP는3000이었으며 강제 자금 입력은 없다.

최종 원시 heap에는 RawSurfaceWorld·RawCampaignSession·RawCampaignSessionOwners가 각각0개이고, 앱이 장기 소유하는 Storage/Pool만 각각1개다. 후반10회 회수 직후 JS heap은33,843,692~34,459,928바이트, 최종34,130,528바이트였다. 첫 미션 뒤25,923,444바이트보다 늘었으므로 앱 전체 메모리가 전혀 증가하지 않았다고 주장하지 않는다. 최종 code self-size는22,008,812바이트이며, 이 결과는 종료 세션의 회수와 이 반복 구간의 후반 범위다. GPU/RSS·모든 미션 장기전까지의 누수 부재를 입증한 것은 아니다.

실행은 debug.html의 실제 ES 모듈이며 내용물 수정 소스 SHA는 E1431 커밋296ce81ac와 같다. 수집 도중 저장소에 E1433 SID 정책을 반영했지만 브라우저는 처음 적재한 모듈을 계속 사용한다. 최종 SidPool의 필드에 새 `#allocation`이 없는 점도 확인했다. 따라서 이30분 결과는 E1431 런타임의 검증이며 E1433 회복 정책을 실행한 결과로 세지 않는다. 원시 JSON의 이전 제품 라벨(E1416/E1427)은 준비 당시 남은 문자열이며 위 출처/해시가 실제 기준이다.

같은 브라우저 수집 알고리즘을 `tools/mission-memory-long-profile.cjs`로 유지한다. 출력명/제품 라벨을 고치고 실행 당시 소스 SHA를 읽는 메타데이터만 더했다. 최소시간·횟수·실제UI/AI·회수 단언은 같다. 새 도구의 evaluate 콜백 파일을 기존 ESLint 브라우저 전역의 명시 목록에 추가했다. 첫 lint의 window 미등록24건도 보존했고 최종 전역 lint는0, Node 구문 검사와19/101 상한 거부2건도 통과했다.

M5의 장시간 미션 반복/메모리 자동 확인분을 완료한다. 물리120Hz·와이드 전투 종합 표시·소리 청취와 사람 검수는 별도다. v26=4/4·v22=4/8로 Node26 전체를 바로 실행한다.

[27회 전체 원문](webpj-e1435-evidence/long-memory.json)·[진행 로그](webpj-e1435-evidence/long-memory.log)·[최종 heap 집계](webpj-e1435-evidence/heap-summary.json)·[실행 수집기](webpj-e1435-evidence/executed-collector.cjs)·[유지 도구 차이](webpj-e1435-evidence/maintained-tool-adaptation.json)·[lint](webpj-e1435-evidence/lint.log)·[출처/해시/한계](webpj-e1435-evidence/provenance.json). 약74.8MB heap 원문은 Git 제외 조사 경로에 보존했다.
