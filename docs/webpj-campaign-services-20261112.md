# webpj 캠페인 로그인/전송 서비스 묶음 — WB04-E326 (2026-11-12, 4-C)

3.1절 4-C 의 client **CampaignLoginServices**(기준선 28줄, 기준선 뒤 +20줄)와 **CampaignTransportServices**(기준선 114줄 = `.cpp` 66 + `.h` 48, +21줄)를 `core/campaign-login-services.js`·`core/campaign-transport-services.js` 로 옮겼다. 두 묶음은 규칙 본체가 아니라 **로그인 부모가 같은 Client 의 실제 장치를 빌리는 좁은 계약**(C++ `std::function` 칸 모음)이다. 실제 호출 쪽(`RawCampaignSessionLogin`·`ClientCampaignTransport`·4-D Session 조립)이 아직 없으므로 이 단위는 칸의 이름·형식·바인더 연결과 "빈 칸은 선택 위치에서 명시 거부" 계약만 확정한다.

## 1. 규칙

- **로그인 서비스(`CampaignLoginServices`):** plain 객체의 칸은 함수 30개와 값 4개(`requestPhase` `{ value }` 상자·`transport` 전송 서비스 객체·`reconnectFile` 저장 파일 장치·`frameLoginTimer` boolean)로 모두 34칸이다(기준선 17 + 기준선 뒤 17). `createCampaignLoginServices(칸들)` 은 알 수 없는 이름·함수 칸의 비함수·boolean 아님·상자 아님·객체 아님을 거부하고, `undefined`/`null` 칸은 비어 있는 채로 둔다(`frameLoginTimer` 기본 false). `requireLoginService(services, 이름, 위치)` 는 비어 있거나 함수가 아닌 칸을 `로그인 서비스 <이름> 누락 (<위치>)` 로 거부해 C++ 의 "선택 위치에서 명시 거부(빈 성공 금지)"를 옮긴다.
- **전송 서비스(`CampaignTransportServices`):** 칸은 `lifetime`·`composite`·`detachSession`·`callbacksActive`·`registry`·`dispatch`·`serve`·`listen`·`connect`·`connectPeerText`·`assign`·`send`·`sendMutable`·`flush`·`poll`·`peer`·`connection`·`exists`·`isOpen`·`status`·`address`·`peerAddress`·`close` 이다. `bindInternalCampaignTransport(transport)` 는 실제 `LocalMissionTransport`(E324)에 연결한다: 공통 칸(`lifetime`/`registry`/`dispatch`/`send`/`sendMutable`/`poll`/`address`/`close`=등록소 `disconnect`/`peerAddress`)에 `serve`=`open(port, false)`(서버 끝만 열기), `connect`/`connectPeerText`=주소 해석 없이 내부 client9 접속(`connect` 는 20BYTE 가 아니면 접속 전에 `RangeError`), `assign`/`flush`/`peer`/`connection`/`exists`(=연결 칸 존재)/`isOpen`(=존재·닫힘 없음·상대 생존)/`status`(완료 1·아니면 0, buffered bit 2 는 안 읽음)를 더한다. 바인더를 만드는 것만으로 열기·접속·시계 호출이 없고, 열기 전 송신/flush 는 전송의 계약 오류가 그대로 나온다(빈 성공으로 바꾸지 않음). 필수 owner/등록소가 없으면 `TypeError`.
- **주소 표시 `campaignPeerAddressText(address)`:** 원문 004e0a30(1,0,0) — 20BYTE 주소의 +4..+7 이 모두 0 이면 `*`, 아니면 점 표기 IPv4, 뒤에 big-endian 포트(+2 상위/+3 하위) 를 `:` 로 붙인다. family WORD(+0 하위, +1 상위)가 정확히 6(SPX)이면 거부하고(0x0106 같은 값은 표시), 20BYTE `Uint8Array` 가 아니면 `TypeError`. DNS 조회 없음.

## 2. 기준선 뒤 변경 판정

- **로그인 서비스(+20줄, 13커밋, 마지막 `3abed63a5`):** 새 칸 17개는 모두 로그인 부모가 부를 **장치 이름**일 뿐 규칙이 아니므로 ② 로 칸으로만 둔다. 일반 71h/67h 전투 설명·옵션 표시(`metaBattleDescription`/`metaBattleOptions`)·시간 동기화 게임 시각(`gameSeconds`)·프레임 타이머 관문(`frameLoginTimer`)·안내(`creatingIslands`/`progress`/`showTimerProgress`/`closeTimerProgress`)·재접속(`selectReconnectFort`/`reconnectFile`/`reconnectUi`)·동맹 예약(`queueAlliance`)·상태 표시(`status`/`clearStatus`)·Abort(`abortBattle`)·번역(`translate`)·프레임 벽시각(`frameWallSeconds`). 각 칸의 소비는 해당 로그인 단위(RawCampaignSessionLogin 등)에서 옮긴다. `reconnectFile` 이 담는 새 파일 RawSavedStateWrite(대응표 밖)의 웹 저장소 적응은 재접속을 옮길 때 정한다. `playerUiBusy` 는 이름만 역사적이고 실제로는 원문 00449200 의 전투 종료 관문을 조회한다(소비 단위에서 확인).
- **전송 서비스(+21줄, `.cpp` 5커밋·`.h` 4커밋, 마지막 `a76499e35`):** ② `connectPeerText`(`f72102823`, 원문 State8 의 저장 문자열 영역/포트로 접속, 내부 바인더는 문자열을 해석하지 않고 내부 client9 만 연다)와 공용 주소 표시의 외부 공개 `CampaignPeerAddressText`(`269de8951`) 반영. ③ **`composite` 칸과 `BindCompositeCampaignTransport` 전체**(Session 대여 `detachSession`/`callbacksActive`, 종료 패킷 2·기술 0x69 의 canonical 등록에 lease 표식을 붙이는 `RegisterCanonical`/`RegisterDirect`, `a76499e35`·`3f766a97f`·`18e3aadd0`)는 복합 전송 `CompositeMissionTransport` 를 옮긴 뒤 같은 모듈에 추가한다(PORT_MAP 상태 partial). ① TCP 바인더(`BindTcpCampaignTransport`, 실제 LAN 소켓)와 Editor 바인더(`BindEditorCampaignTransport`)는 LAN/Edit 제외라 만들지 않는다.

## 3. 검사 (`tests/campaign-services.test.mjs` 10건)

cpppj 에는 이 두 묶음을 직접 겨냥한 시험이 없다(`CampaignSessionTests`·`MissionStartingTailTests` 등 Session 시험이 간접 사용, 4-D 에서 옮김). 기준선 소스의 계약을 단위로 옮겨 확인한다.

- 주소 표시 1건(IPv4·wildcard 네 BYTE 각각·포트 폭/상하 BYTE·family 두 BYTE·SPX/길이/형 거부), 내부 바인더 5건(owner/등록소 누락 거부, 칸 구성과 생성 시 무효과(열기·접속·시계 호출 없음), serve/connect 가 실제 두 끝을 엶, poll 수락·상태 조회(닫힘 표시 상태 포함)·재배치·닫기, 열기 전 송신/flush 오류·존재하지 않는 연결의 false·peer 위임·주소 표시), 로그인 서비스 4건(34칸 이름 전체·만들기·형식 거부·선택 위치 거부).
- 첫 실행의 실패 1건은 시험 쪽 가정이었다: 열기 전 `send` 는 `false` 가 아니라 전송의 계약 오류("로컬 전송 연결을 열기 전 송신")이다. 바인더가 빈 성공으로 바꾸지 않는 것이 맞아 시험을 고쳤다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 35개(전송 22·로그인 13)를 돌려 첫 실행에서 4개가 남았다: family 두 BYTE 비교(하위 BYTE 만 읽는 변형)·`flush` 위임 제거·`isOpen` 을 존재로 대체·`peer` 위임 제거. 시험을 보강(family 0x0106, 닫힘 표시가 있는 존재하는 끝, 열기 전 flush 오류, `send` 가로채기로 `peer` 위임 확인)한 뒤 **0개**가 남았다.
- **앱 수명:** 아직 Session 이 이 서비스를 만들지 않는다(PORT_MAP session 0). 다음 항목인 `CompositeMissionTransport` 뒤에 `BindCompositeCampaignTransport` 를 더하고, `RawCampaignSessionLogin`·4-D 조립이 로그인 서비스를 채운다.
- 3.1절 4-C 는 23/30 이다. 다음: client `CompositeMissionTransport`. (후속: E327 에서 `bindCompositeCampaignTransport` 를 더해 PORT_MAP 이 partial→port 가 됐다, [문서](webpj-composite-mission-transport-20261112.md).)
