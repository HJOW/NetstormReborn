# LAN 골렘 실제 생산·배치 검증 — 단위1373

2026-11-27, HJOW-Athlon. 마지막 디컴파일 호스트도 HJOW-Athlon이다. 기존 복원 본체/원본 자료를 읽었으며 새 디컴파일·원본 실행·원본 변경은 없다. 제품은1372의11.01 그대로이며 EXE SHA-256도 b400a38a93a54c89d6b90e13fefa1f31d49656c203dbeb785e8bfd788a0618a2로 같다.

tools/cpp_lan_opponent.py에 --produce-golem을 추가했다. 새 두 Fort/격리 사용자 폴더·127.0.0.1의 실제 TCP로 두 참가자가 로그인14/PlayerKnown/Ready까지 진입한다. 호스트는 실제 사제 우클릭→Construct→Temple→현재 보이는 Wind 항목을 클릭한다. 메뉴 항목의 실제 명령/인자는 클릭 위치 조회에만 쓰며 직접 실행하지 않는다. 원래 마우스 pointer/클릭과 실제 커서의 legal·배치·공간·source 허용을 확인해 신전을 건설한다. 완성 후 production:133의 실제 불투명 아이콘을 클릭하고 같은 방식으로 골렘을 배치한다. 패킷·ACK·객체·SP·저장 성공을 주입하지 않는다.

Wind 신전만으로 골렘 생산이 활성화되며 별도 워크샾은 필요하지 않다. RawCampaignProductionBindings.cpp의 신전 부류/허용표·RawProductionControls.cpp의 실제 생산 취득 경로를 따른다. sunwalker.type의 level1/운반 부류는 RiftTypeTable.cpp의 기본 단가400 처리로 비용400 SP, 최대HP50이다. 이전 metadata의 후처리 전 cost0을 기대값으로 쓰지 않는다. stockEnabled=0인 현재 표준 옵션에서는 재고 수량0 자체가 취득을 막지 않는다. 원문의 재고 사용/벽시계 보충 규칙은 변경하지 않았다.

## 결과

새 두 프로필·실시간 TCP의 실행1회에서 통과했다. 신전 배치 후보는600,420, 골렘 배치 후보는450,420이었다. 신전 완성 뒤 양쪽 SP1500, 골렘 배치 뒤 양쪽 SP1100이며 다른 소유자의 SP6500은 유지됐다. 양쪽 신규 객체는 SID15249/sunWalker/owner1/좌표116,206/HP50으로 일치했다. 별도 시각의 애니메이션 프레임은 동일성 조건으로 삼지 않았다. 호스트 Saves/!state.dat는976BYTE, owner1/SP1100이며 SHA-256은653cac6f03fb790ba8cdc2c563a0d0e61e2a41d5d514700da034b8c2b3cd7edd다. 당시 파일을 production-after-unit-state.dat로 보존했다. 두 게임 모두 종료0이다.

현재 netstorm 라이브러리에 관련 집중 실행 파일을 다시 링크했고 빌드 종료0이다. netstorm_cursor_drop_tests.exe 12/12, netstorm_selection_key_input_tests.exe --filter InventoryItemGump 2/2(독립 원본132입력·실제 자산/트리/픽셀/클릭)를 통과했다. 기존 fixture는 수정하지 않았다. 도구의 Python 구문 검사도 통과했다. 원본/CD·fixture·AGENTS.md·README.md 보호2,810파일의 전후 SHA-256은 같다.

[호스트 화면](../screens/cpp-lan-production-1373-20261127/host.png)·[상대 화면](../screens/cpp-lan-production-1373-20261127/bot.png)·[검증 JSON](../../cpppj/lan-production-1373-20261127.json). 원시는 extracted/cpp-lan-1373/production-1, 빌드/검사 로그는 extracted/build-lan-production-tests-1373.log·tests-lan-cursor-1373.log·tests-lan-inventory-1373.log에 있다. 도구 사용법은 [상대 봇 안내](../testing/cpp-lan-opponent-guide.md)를 따른다.

단위1373의 유닛 생산 확인만 완료했다. 다음1374는 추가 건물·다리 건설이며, 이탈 뒤 새 참가자 재접속과 최신 ZIP 갱신도 남았다. 포획/희생·특정 적 공격/승리·실물 LAN·사용자 직접 확인 검사는 실행하지 않았다. DN-MP-05의 실제 생산 검증 참고 근거만 공통 후속 문서에 남기며 DN 코드/진행률/검사 수를 올리지 않는다.
