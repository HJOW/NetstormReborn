# 캠페인1-3의 실제 AI 패배 경로

WB04-E1408, 2026-11-27, HJOW-Athlon. 기본 AI의 장시간 미결정 관측(E1396)과 별개로, 메인 메뉴를 실제28프레임(0.7초) 진행한 뒤 같은 아군 AI를 켰다. 시작SP2000·이름·동맹·기술을 보존했고 체력·사제·승패·난수 상태를 직접 바꾸지 않았다. 이 입력 시각 변형은 C++과 맞춘 엄격 오라클이 아니다.

아군 사제는194.675초에 포획됐고356초에 실제 희생을 마쳤으며356.025초에 사망했다. 수집기는13,900프레임에서 원래 Failure/Continue 안내와 `Tell TryAgain` 명령을 읽고 종료했다. 화면에서도 Failure 본문과 Continue 단추를 확인했다. 게임 오류는0이며 실제 적 AI가 아군을 제압한 **패배**다.

종료 때 아군/적 AI 모두 initialized1·stop0이었다. 아군 지갑2600·적9350, 아군 사제SID0·적15001이다. 승리 단언은 실패했고 이후 완료값 저장·다음 미션 이동 검사는 진행하지 않았다. 이전 기본 조건의60,000/120,000프레임 미결정 결과를 이 결과로 덮어쓰지 않는다. **1-3 자연 승리는 여전히 미완료**다.

[관측 JSON](webpj-e1408-evidence/result.json), [원문 로그](webpj-e1408-evidence/flow.log), [종료 덤프](webpj-e1408-evidence/terminal.tsv), [실제 종료 화면](webpj-e1408-evidence/terminal.png), [조건을 포함한 수집기](webpj-e1408-evidence/collector.cjs.txt), [출처·해시·종료값 회수 한계](webpj-e1408-evidence/provenance.json)를 보존했다. 이 단위는 읽기 전용 실게임 관측이며 제품 소스 변경은 없다.
