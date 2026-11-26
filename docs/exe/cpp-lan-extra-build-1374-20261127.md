# LAN 추가 건물·다리 실제 건설 — 단위1374

2026-11-27, HJOW-Athlon. 마지막 디컴파일 호스트도 HJOW-Athlon이다. 기존 복원 코드/자료를 읽었으며 새 디컴파일·원본 실행·원본 변경은 없다. 게임 소스/제품1372 EXE는 변경하지 않았다. SHA-256은 b400a38a93a54c89d6b90e13fefa1f31d49656c203dbeb785e8bfd788a0618a2다.

상대 봇의 --produce-golem 다음에 --extra-build를 선택하면 실제 사제→Construct→Workshop→Sun Workshop 항목과 허용 위치 클릭으로 썬 워크샾을 건설한다. 생산 목록의 실제 첫 다리 아이콘과 원본 내용물 SID를 대응시키고, 자연 공급된 pattern0/quality2를 확인한 뒤 다리를 놓는다. placement-reach는 현재 커서의 실제 배치 허용 위치만 탐색하며 생성은 정상 마우스 클릭과 기존 프로토콜이 수행한다. SP·HP·객체·ACK·패턴·품질을 주입하지 않는다.

## 건설 완료 구별

첫 실행 build-1은 썬 워크샾 객체가 존재한다는 이유만으로 완성 판정을 해 너무 일찍 비교했다. 당시 같은 SID15250/TYPE85에서 host HP0, bot HP2816→50이었지만 모두 extra131(0x83)의 abstract bit0가 켜져 있었다. 건설 중에도 Factory 가상표는0x509148로 같고 raw+26이 실제 진행 HP이므로, 객체 존재/TYPE/가상표는 완성 증거가 아니다. 최초 보고/화면/실패 결과를 보존했으며 원본 fixture/기대값은 수정하지 않았다.

RawConstructionProcess.cpp의 실제 Complete는 권한 측이 HP최대에 도달했을 때 unpop→abstract 해제→repop(9)을 실행한다. 이에 따라 도구는 양쪽에서 HP3000과 extra bit0 해제를 모두 기다린다. 두 번째 build-2의 워크샾 직후/다리 직후 양쪽4보고는 HP3000/extra130이고, 첫 실행의 건설 중2보고는 완료로 선택되지 않는 것도 같은 판별기로 확인했다. 정상 도착의 기본0.6초와 duration0 요청의10초를 임의 대기로 대신하지 않는다.

## 실제 TCP 결과

새 두 Fort/프로필·실시간127.0.0.1 TCP의 build-2를 통과했다. 1373 경로로 Wind 신전·골렘을 먼저 생산하고 양쪽 SP1100을 확인했다. 썬 워크샾의 명시 비용800 차감 후 양쪽 SP300이며, SID15205/sunFactory/owner1/좌표139,206/HP3000/extra130이 일치했다. 애니메이션 프레임은 별도 시각이므로 동일성 조건에서 제외했다.

첫 실제 다리 내용물 SID108/pattern0/quality2/flags64를 확인했다. 신규 다리는 양쪽 SID15207/bridge/owner1/territory-1/좌표143,198/frame35/HP0이며 전체 object 행이 같았다. bridgeConnector를 bridge에 포함시키지 않고 자산명 일치로 구별했다. pattern0은 원본1×1 유효 칸 하나이므로 신규 bridge는 정확히1개다. Bridge는 HP가 없는 타입이라 보고 HP0이 정상이며, raw+26을 직접 체력으로 해석하지 않는다. 다리 비용은0이고 양쪽 SP300이 유지됐다.

호스트 Saves/!state.dat는976BYTE, owner1/SP300이고 SHA-256은0e11d65fdb51f42e6b6a1dc726071e6184431f15af61d2a9cde4080c42f9e414다. 당시 파일을 extra-after-build-state.dat로 보존했다. 두 게임 모두 종료0이다. 현재 라이브러리에 netstorm_construction_current_tests.exe를 다시 링크해 빌드 종료0을 확인했고 전체19/19(현재 건설 상태·요청·예측 정리/확정 교체의 기존 독립 원본 회귀)를 통과했다. Python 구문 검사도 통과했다. 연속1373~1374 검증 전후 보호 원본/CD·fixture·AGENTS.md·README.md 2,810파일은 SHA-256이 같다.

[호스트 화면](../screens/cpp-lan-extra-build-1374-20261127/host.png)·[상대 화면](../screens/cpp-lan-extra-build-1374-20261127/bot.png)·[검증 JSON](../../cpppj/lan-extra-build-1374-20261127.json). 원시는 extracted/cpp-lan-1374/build-1·build-2와 extracted/build-lan-extra-tests-1374.log·tests-lan-construction-all-1374.log다. [봇 사용법](../testing/cpp-lan-opponent-guide.md).

다음1375는 참가자 이탈 뒤 새 프로세스의 실제 재접속이다. 공식 원본의 지원 범위/진입점을 먼저 확인하며 새 ID 참가와 기존 참가자 복구를 구별한다. 최신 ZIP 갱신도 남았다. 포획/희생·특정 적 공격/승리·실물 LAN·사용자 직접 확인 검사는 실행하지 않았다.
