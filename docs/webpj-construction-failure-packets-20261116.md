# 건설 환불·배치 취소 공용 패킷 (WB04-E602)

생성자1652 ConnectConstruction이 요구하는 `RawConstructionFailurePackets`
전체 공개 API를 먼저 옮겼다. 근거는 `225553df`의 `.cpp/.h` 33줄이며
기준선 뒤 변경은0이다. 전용 cpppj 직접 시험/원본 관측 fixture는 없다.
기존 Canon/Finder의 원본 대조를 이 모듈의 직접 관측으로 세지 않았다.

## 실제 wire와 삭제 순서

- 환불6ch는11BYTE이며 Player/source/kind 하위BYTE와 signed 금액의
  비정렬 DWORD를 그대로 쓴다. 취소2fh는15BYTE이며 좌표float 비트와
  type/argument/direction 하위BYTE를 보존한다.
- 수신은15BYTE/PATCH·현재 TYPE 범위를 확인한 뒤 같은 현재 Canon을
  만들고, 유효 칸마다 현재 TYPE 발자국으로 일반flags0 Finder를 쓴다.
  matching abstract는 모두flags8로 삭제하며 첫 후보·로컬SID·소유자
  제한을 더하지 않는다. 앞선 삭제 뒤에도 다음 후보/현재 noIslandType을
  다시 읽는다. 실제 삭제 실패는 그 위치에서 전파한다.
- 원문에 없는 헤더/선언 길이 검사를 추가하지 않았다. JS의 물리 span
  범위 검사는 명시 오류로 남겼고 선택하지 않은 삭제 장치는 요구하지 않는다.

## 검증과 남은 경계

신규7건은 첫 실행부터 통과했다. 바이트 전체/signed 양끝·단정도 비트/
음수0/NaN·비정렬 span·실제 Finder의 여러 후보와 삭제 후 현재 값·선택된
실패를 대조했다. 대응표 메모의 규칙 접두 누락은 검토 중 복구했다. 최초 전체 검사에서는
웹 경로의 중복 webpj 접두 때문에 대응표2건이 실패했다(각5786통과/2실패).
경로를 기존 규약의 www/core 기준으로 고치고 최종 전체를 재실행했다.
manifest 직접 연결은0개라806항목을 유지한다. Source 계약을 원본 관측으로
등록하지 않았다. Node v22.23.3·v26.13.0 최종 전체는 각각5788건 통과, 실패/취소/
건너뜀0이며 종료0이다. Node24는 사용자 지시로 생략하고 호환을 가정한다.
대응표1160행/오류0, port846/partial39/todo122·session0,
잔여161단위/34340줄과 JS131215줄/검사772파일을 확인했다.

Session의 기본 sendMoney/sendReject/RemoteTell·ConnectConstruction과
Native 고정 수신기는 다음 결선에 남았다. World 게시·Load/Login·L2/L3·
실제 미션 플레이는 아직 미완료다. 보호 파일/cpppj 수정·디컴파일·게임/
브라우저 실행·자연 완주·실제 포획/희생·공중3종 전용 관측은 하지 않았다.

신규7건을 포함한 관련27건 통과. 최초/최종 전체와 집중 로그는
`extracted/wb-e602-node22.log`, `extracted/wb-e602-initial-node26.log`,
`extracted/wb-e602-final-node22.log`, `extracted/wb-e602-final-node26.log`,
`extracted/wb-e602-focused.log`에 구별해 둔다.
