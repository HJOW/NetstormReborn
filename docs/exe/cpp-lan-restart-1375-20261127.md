# LAN 기존 참가자 재시작·복구 — 단위1375

2026-11-27, HJOW-Athlon. 마지막 디컴파일 호스트도 HJOW-Athlon이다. 기존 10.78 디컴파일을 읽었으며 원본/디컴파일은 수정하지 않았다. 기존 참가자 복구의 세 결함을 교정해 실제 TCP 복구·집중130/130·튜토리얼 회귀를 통과했다. 새 ID 중도 참가 추가 여부는 이후1380 사용자 지시로 원본 범위 유지(추가하지 않음)로 확정됐다.

## 대상 구별

원본 일반 Login0x27과 Reconnect0x63은 기존 Player[1..8]의 subscriber와 일치하는 슬롯을 찾는다. 알 수 없는 새 ID는 owner0으로 거부하며, 진행 중 전투의 새 ID 참가 기능은 없다. 기존 참가자의 저장된 ID를 새 프로세스에서 복구하는 경로와1369의 호스트 장애 뒤 서버 인수를 구별한다. 새 ID 참가를 추가 요구에 포함할지 사용자에게 범위를 문의했다.

`tools/cpp_lan_opponent.py --restart-peer`는 기존 상대의 실제 프로세스를 종료하고 같은 격리 사용자 폴더의 cfg/Fort/복구 저장을 다시 읽는다. 새 로비/Fort를 생성하지 않으며 저장 바이트·참가자·ACK·월드·SP를 주입하지 않는다. 제품의 실제 시작 복구 질문과 Yes 버튼을 사용한다.

## 첫 실제 실행과 시계 결선

`extracted/cpp-lan-1375/restart-1`은1372 제품에서 신전·골렘·썬 워크샾·단일 다리와 양쪽 SP300까지 통과했다. 상대 정상 종료 뒤 호스트의 연결 비트 해제도 확인했다. 그러나 재실행은 복구 질문 없이 메인 메뉴에 머물러 제한 시간에 실패했다. 최초 실패 관측을 보존했으며 host/bot/restarted 세 프로세스는 모두 종료0이다.

이전 상대의 실제976BYTE 저장 파일 timestamp(+264)는61초였다. 새 Client의 WallSeconds는 시작 후 경과 초이므로61초보다 작아 StartupStateCheck의 양의 경과 조건에서 탈락한다. 원본00461090은 time32−0x32c85780의 기준 초(0055b4a4)를 저장하고,00460cd0/00460d70은 기준 초×1000과 경과/보정 밀리초를 DWORD로 감싼 뒤 초로 바꾼다. 현재 GameClock에 이 기준 초가 빠져 있었던 것이 원인이다.

GameClock의 기본 기준 초0은 기존 독립 Native 대조용으로 유지하고, 실제 Client 시작에는 원본 epoch를 공급하도록 수정한다. 초기 Capture도 원본 InitFullTime과 같이 실행해 첫 프레임 delta에 epoch 전체가 들어가지 않게 한다. 명시 검사 전용 고정 시각 모드는 기존 기준0과 최초 캡처/프레임 수를 유지한다. 실제 실행은 UTC 초가 바뀐 뒤 밀리초 원점/time32를 잡아 빠른 재시작의 분수 시각 역행도 막는다. 원본 바쁜 대기 대신 Sleep(1)로 OS에 양보하며 우선순위를 별도로 바꾸지 않는다.

## 월드 대조 기준

공유 객체의 SID·자산·소유자·위치·목표·수량·HP·내용물 수를 비교한다. 개별 클라이언트의 현재 그림 애니메이션 프레임은 비동기 관측이라 제외하며, 다리는 품질/손상이 frame에 있으므로 그 frame과 raw_bridge_fields도 비교한다. private player 객체는 LocalFort가 자기 소유자 번호로 생성하므로 제외한다. 로컬SID<15000의 island/islandStalag/bridgeConnector는 원본 flags2와 PostPop 이웃 생성 경로에 따라 SID만 제외하고 색/방향 frame을 포함한 나머지 전체 행의 다중집합을 대조한다. 같은 이름의 서버SID는 공유 객체로 남긴다. 근거는 RawCampaignSessionLogin.inc:102, RawPlayerWorld.cpp:58~64, RawIslandPostPop.cpp:113~120, RawBridgeConnect의 kConnectCreateFlags=2와 LinkImpl, SidPool의 flags2다.

일반 로그인14만으로 전체 월드 수신 완료를 판정하지 않는다. 실제 host와 재실행한 peer의 공유 객체·SP가 일치할 때까지 기다리며, 전투 중 건설/생산한 객체가 새 프로세스에 도착했는지도 포함한다.

## 두 번째 실행과 재접속 SP 장치

시계 교정 제품의 `restart-2`는 실제 시작 복구 질문과 Yes를 통과하고 State8~13까지 진입했다. 그 뒤 `실제재접속SP지정미연결`로 재실행 참가자가 종료1이었다(host/이전 bot 종료0). 기존 LocalMissionState::Connected의 current+0x84 관문에서004477d0을 호출하지만 Login::StateHooks가 이 선택 장치를 제공하지 않은 결선 누락이다.

원본004477d0처럼 현재 Editor이면194162.0F를 선택하고, playerKnown일 때 같은 로컬 owner의 ScrambledSpStore에 쓰며 HUD 갱신은 항상 호출하도록 연결했다. 복구 파일의 SP를 Python에서 주입하거나 원본 reader가 소비하지 않는 지역 필드를 새 정책으로 읽지 않는다. 최종 SP는 실제 서버 로그인/기존 패킷 수신 결과와 대조한다. 수정 전 집중 검사는25/25(기존 시계/Archer/월드21·epoch/실제 복구 시간 관문2·원본 시작 관문2/390관측) 통과했으며, SP 결선 뒤 재빌드·검증 결과는 아래 최종 검증에 있다.

## 세 번째 실행과 거주 구역 복구 전역

`restart-3`은 SP setter와 State13→14를 통과한 뒤 거주 구역 SID15001/TYPE89의 Regular event3에서 frameCheck 진단과 SHP 물리 범위 오류로 재실행 종료1이었다(host/이전 bot 종료0). 호스트의 정상 frame13이 복구 후 테마 값12를 중복 가산해25로 바뀐 것이 직접 원인이다.

원본 Netstorm.c:123365/FUN_0049a2b0은 Residence 최초 PostPop에서005c85d4(savedGame)이 비영이면 테마 구간을 빼고 raw+42 기본 프레임을 기록한다. 기존 AssetPostPopState.residenceAnimation은 잘못 명명된 독립 bool이고 실제 복구 전역에 미결선이었다. 같은 ClientLocalStateGlobals::SavedGame 주소를 빌리는 sharedSavedGame과 현재 getter를 추가하고 실제 Session에서 연결했다. 독립 원문 fixture 입력은 기존 bool fallback으로 유지한다. Notify 효과 뒤 true/false 변경을 각각 읽는 두 회귀를 추가했으며 제품·시계·상태·거주 구역 후처리 집중 대상을 재빌드해 아래 최종 검증을 완료했다.

## 최종 검증

제품1375 EXE SHA-256은 `08a24d6532616b5c9c1511ae012b05b937ad19c95b60c1d93c16dd5044702dec`다. Release 제품과 세 집중 대상 빌드가 종료0이다. 시계25/25·상태14/14·자산 후처리91/91, 총130/130을 통과했다. 기존 Native fixture를 바꾸거나 오류/프레임 범위를 완화하지 않았다.

실제 `restart-4`(로비17140/게임17139)는 새 두 Fort로 신전·골렘·썬 워크샾·단일 다리를 건설한 뒤 상대를 종료하고 같은 저장소의 새 프로세스에서 복구 질문/Yes를 선택했다. 이전 owner2·subscriber6356994·OpponentBot과 두 Ready67이 복원됐다. 호스트 server/boss1/1, 복구 참가자0/0과 실제 로그인14/PlayerKnown을 확인했다.

공유 SID 객체6,525개와 로컬 생성 표면45개가 위 비교 기준으로 일치했다. 전투 중 만든 Wind 신전15229/HP5000, 골렘15231/HP50, 썬 워크샾15232/HP3000, 다리15234도 같은 SID/좌표/소유자/HP로 새 프로세스에 도착했다. 양쪽 SP는 owner1=300, owner2~8=6500이며 호스트·이전 상대·복구 참가자 모두 종료0이다. 복구 파일은976BYTE/SHA-256 `2f7acb0b786457a86e12aeeee443c593c9e0e9e39cc8263817b17e7049249415`다. [복구 질문](../screens/cpp-lan-restart-1375-20261127/prompt.png)·[호스트](../screens/cpp-lan-restart-1375-20261127/host.png)·[복구 참가자](../screens/cpp-lan-restart-1375-20261127/restarted.png). 참가자 캡처의 HUD5300은 RawProductionControls.cpp:235~237의 단계별 숫자 증가 중이며, 같은 시점의 실제 분산 SP 보고는6500이다.

한국어1280×720 실제 GUI에서 튜토리얼1의 F4 뒤 A→B 문구 갱신, F8 안내 재열기, 재시작, 튜토리얼2 전환, 이탈을 다시 통과했고 종료0이다. [튜토리얼 회귀](../screens/cpp-lan-restart-1375-20261127/tutorial.png). 원본/CD·fixture·AGENTS.md·README.md 보호2,810파일은1373~1375 전후 동일하다. [검증 JSON](../../cpppj/lan-restart-1375-20261127.json), 원시자료는 `extracted/cpp-lan-1375`에 있다. 최신 전수 관문1327을 새로 실행한 것으로 세지 않는다. 다음 독립 작업은1376 최종 ZIP 갱신이다.
