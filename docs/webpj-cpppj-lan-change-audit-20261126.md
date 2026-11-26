# webpj C++ LAN 변경16행 판정과 공통 수신·읽기 진단 반영

- 단위: WB04-E1348, 2026-11-26, HJOW-Athlon.
- 검토한 C++ 변경:1364~1370 `f7ed546a3`·`0ff95f9da`·`205fa2c07`·`af2523282`·`a7509bbc7`·`a3bea51fa`·`ffd35d367`.
- 원문 디컴파일 PC는 현행 인계와 같은 HJOW-Athlon이다. 재디컴파일·제품 C++ 수정 없이 행의 기존 base부터 현재 커밋까지 `.cpp/.h/.inc`를 함께 읽었다.

LAN 보완이라는 커밋 제목만으로 웹 제외로 처리하지 않았다. `RawMissionClientPackets.player`의2bh 채널 해제는 `missionPost`로 막히지 않는다. 현재 내부 Session의 실제 수신기에 `missionPost=true`·로컬 Player 확정 전·비활성 Player 정보를 주어도 `removeChat(subscriber,0x20)`가 선택되는 것을 확인했다. 이에 선언·수신기·현재 앱 장부까지 연결했다. 기록100개를 모두 방문해 일치하는 ID의 플래그 BYTE에서 지정 채널만 지우며, 자기ID·0·반복ID도 제외하지 않고 이름·다른 채널·미사용 바이트를 보존한다. TCP 기능을 추가하지 않는다.

`RawFortObjectStream`의 물리 읽기 오류 문맥도 모든 미션의 손상된 Fort에 적용될 수 있어 반영했다. 오류에 `stream`·`pos`·`size`를 넣되, 이미 만든 객체와 실패 위치까지 진행한 커서를 유지한다.

| 대응표 행 | 판정·웹 도달 근거 | 새 base |
| --- | --- | --- |
| CampaignClientServices | ② `removeChat`은 현재 앱 채팅 장부로 연결. `creatingIslands`의 일반 전투 안내·State7/즉시8 일반 진행은①: 웹 내부 미션 Zone의0x100 비트 부재(E1328), 기존 미션 진행 서비스는 유지. AI 이름 변경은 E1347 완료. | `205fa2c07` |
| CampaignLoginServices.h | ② 선택 시 필요한 `removeChat` 함수 칸 추가. 주어지지 않은 칸을 빈 성공으로 바꾸지 않는다. | `205fa2c07` |
| EditorLocalTransport | ① exclude-edit: 소유 TCP 끝·listener 반납. 웹 내부 LocalMissionTransport는 별도 수명이다. | `205fa2c07` |
| GameWorld | ① `raw_network_role`은 LAN 서버 인수 진단 출력만 추가. 게임 계산·Session 덤프 형식 변경 없음. | `a3bea51fa` |
| RawCampaignArcherBindings | ① 외부 서버 시각 보정 뒤 음수 경과 허용. 웹은 내부 권한 서버·양의 고정 시각/단조 프레임을 쓰고 비서버 Commander 보정을 받지 않아 해당 입력 경로가 없다. | `a7509bbc7` |
| RawCampaignCursor | ① 비용 차감 뒤 복구 저장. 웹 미션/스커미시는 `blocked=missionPost`이며 실제 `raw-cursor-drop.js`의 `battle && !blocked`가 저장 호출을 생략한다. | `ffd35d367` |
| RawCursorDrop | ① `.h`의 역사적 `battleRefresh` 실체 설명 변경. 몸체 변경 없음. 위 미션 관문 때문에 앱 호출도 없다. | `ffd35d367` |
| RawCampaignSession | ① 일반 LAN 생성자의 Fort/special/chaff 공유 모드 해제와 복구 저장 closure. `missionPost`인 웹 설치는 일반 LAN 생성자 분기로 들어가지 않는다. | `ffd35d367` |
| RawCampaignSessionLogin.inc | ② 미션에도 선택되는2bh `removeChat` 연결. 나머지 peerAddress·버전 거부·일반 복구 저장·재접속 결과는①: 내부 양끝의 같은 버전, 서버 권한, 미션 저장 관문 및 외부 재접속 부재. | `a3bea51fa` |
| RawChatChannels | ② `removeFromChannel`을 실제100기록에 구현. 첫 일치 조기 반환·자기/0 제외·이름 초기화를 하지 않는다. | `205fa2c07` |
| RawMenuControls | ① Multiplayer Options의 Push/Attach 갱신 수명. 웹에 해당 창·호출자가 없다. 공통 도움말은 E1347 완료. | `f7ed546a3` |
| RawMenuMultiplayer.inc | ① exclude-mp: 색 제어 문자를 쓰는 LAN 창과 자식 Move/Attach 수명. | `f7ed546a3` |
| RawSurfaceWorld | ① 외부 시각 보정의 역행 허용. 웹 내부 서버가 쓰는 게임 시각은 위 Archer와 같은 단조 경로다. | `a7509bbc7` |
| UberLan.inc | ① exclude-mp: 이전 TCP Source의 소유 연결·listener 정리. | `205fa2c07` |
| RawFortObjectStream | ② 직접 BYTE 읽기 실패의 스트림·위치·물리 크기 문맥. 기존 부분 생성과 커서 보존 검사를 확장했다. | `af2523282` |
| RawLocalMissionState | ① 재접속 수락17·거부18과 결과 로그. 웹에 외부 서버 재접속·서버 인수 요청 경로가 없다. 기존 내부 미션 상태 진행은 유지한다. | `a3bea51fa` |

②5행·①11행,③ 남김 없음. 대응표16행의 base·판정 근거를 갱신한다. 대응표 밖 신규 파일7개는 기존 판정을 유지하며 기준선 전체 교체 때 등록한다.

집중 검사72/72 통과(4.5초), 대응표7/7 통과(172.3초), 대응표의 기존 행 변경 보고0. 새 파일7개는 위 설명대로 남는다. 운영 번들은3,446,525바이트다. Chromium4/4 통과(4.2분): 같은 입력 반복 결정론과4:3·16:9·16:10 시작 상태의 기존 C++ 화면·덤프가 일치한다. 정기 Node26.13.0 전체6,856/6,856 통과(820.2초, 실패·취소·미실행0). Node22.23.5 전체도6,856/6,856 통과(774.6초, 실패·취소·미실행0)했고 두 버전은 순차 실행했다. 원시 검토 diff·검사 로그는 Git 제외 `extracted/webpj-e1347-cpp-diff.txt`·`webpj/extracted/webpj-e1348-*`에 보존한다.
