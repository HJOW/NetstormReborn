# 내부 미션 전환 flush·종료 수신 재반영 (WB04-E620)

cpppj1320 `5756ffd5`의 기존 내부 미션 전환 변경을 3.6절②로 판정해
`CompositeMissionTransport`·`RawConnectionRegistry`·`LocalMissionTransport`에
재반영했다. 전체 Session/Client 조립은 아직 미완료이며 앱 연결은0이다.

## 원인과 구현

State7이 이전 미션의 client9를 닫아도 같은 등록표의 이전 server1은
종료 통지를 받을 때까지 살아 있다. 새 조정자가 자기 노드만 flush하고
다른 owner의 수신을 생략하면 이전 끝을 처리하지 못하고 연결 번호 재사용도
막힌다. 원문의 현재 전역 ID 순회처럼 살아 있는 노드의 실제 장치를 호출한다.

- Registry.track에 선택형 flush/receive 장치를 더하고 직접 호출 입구를
  만들었다. 현재 토큰의 장치가 없으면 토큰을 포함한 오류다. 선택한 함수의
  참조는 callback의 자기 반납/등록 교체에도 복귀까지 유지한다.
- 내부 전송은 같은 고정 노드/송신 누적에 flush를 연결하고 같은 끝의
  type1/type2 통지·집합/단일 입력을 수신한다. 직접 수신은 임시 Poll 대여만
  확보하며 정상/예외 모두 자신이 확보한 관문만 반납한다.
- 복합 전송의 Unknown flush/수신은 등록소의 같은 실제 장치로 전달한다.
  없는 토큰은 계속 명시 오류이며 조회/직접 송신·방 종료의 다른 owner 경계는
  그대로다. 종료 성공을 합성하거나 이전 전역 칸을 강제로 지우지 않는다.

판정한6개 단위의 base는 `5756ffd5`다. 위3개는②(싱글플레이 결과 영향)다.
같은 커밋의 EditorLocalTransport·LocalTcpTransport·LocalUdpTransport는
각각 Edit/LAN 전용이므로①(웹 제외)로 기록하고 구현하지 않았다.
원본/기존 fixture·cpppj·다른 프로젝트 인계는 변경하지 않았다.

## 검사와 최초 실패

cpppj `MissionConnectionFlushTests.cpp`의4건을 옮기고 직접 수신의
callback 재진입 거부/예외 뒤 대여 반납과 기존 끝의 buffered 프레임 실제 전달
2건을 더했다. 이전 client9 종료→
다음 Source 생성→이전 server1 flush→실제 `ReceiveConnectionEnded`의
type2 처리→이전 토큰 반납→같은1번 재배치를 실제 내부 전송으로 검사한다.
등록된 closure가 이전 전송을 보유하며 C++ weak_ptr 만료 시점 자체를
JS 강제 GC 검사로 가장하지 않는다.

기존 Unknown 검사도 원문 변경에 맞춰 flush/수신의 실제 빈 결과를
확인하도록 바꿨다. 첫 집중58건 중56건 통과였고 새 검사 정리에서 잘못 쓴
shutdownDevices/비공개 internal 접근을 실제 공개 API로 고쳤다.
다음57/58에서 독립 open의 자동 접속을 명시 분리(open(false)→connect)하여
58/58 통과했다. 이어 다른 Source의 buffered 누적을 새 조정자의 flush/poll로
실제 수신하는1건을 보강해 최종59/59 통과했다. 이 보강 전에 해당 검사 파일을
이미 읽은 Node v26의 진행 중 전체는 종료하고 최종 전체를 새로 실행했다.
Node v22는 보강된6건을 읽었음을 로그로 확인한다.
제품 기대값/기존 원본 fixture는 바꾸지 않았다.

기존 local-mission-transport-x86의66데이터행 대조(Zone·Login·Batch·Install)를
유지한다(SHA-256 `34288e9109dd6d15df5231191b61f1f193c97d4a01d72a8aa59d812e3852368c`,
머리글 포함67줄). 신규 검사에는 전용 원본 관측이 없으며 cpppj1320 소스 계약이다.
게임 창·LAN·실제 미션 플레이/승리 검사는 실행하지 않았다.
Node v22.23.3·v26.13.0 최종 전체는 각각5,933건 통과,
실패·취소·건너뜀0·종료0이다(`--test-concurrency=4`). v24는 기존 지시로
생략·호환 가정이다. 최종 양버전 로그에 buffered 보강 검사도 포함됐다.
대응표 오류0·UTF-8·구문·git diff --check 통과다. manifest806항목을 유지했고
기존 local-mission-transport의 범위/계산 칸을 갱신했다.
로그는 `extracted/webpj-e620-node22.log`·`webpj-e620-node26-final.log`,
중단한 v26 최초 진행 로그도 같은 접두로 보존한다.

다음은 E619 이후 생성자의 Template 설치·MakeRuntime·승패/AI와
외곽 Session 조립이다. RawCampaignSessionLogin의 기존③ 경계도 유지한다.
