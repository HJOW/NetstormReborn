# 건설 재생의 실행·라벨과 현재 context (WB04-E601)

Construction 조립2731..2774가 참조하는 현재 재생 라벨 부모를 먼저
옮겼다. 근거는 `225553df`의 RawBuildReplay.cpp/.h 및
ClientBuildReplay.cpp/.h이며 기준선 뒤 변경은0이다. 이 단위는
**Raw의 Run/IsLabel/Notify와 Client의 두 현재 라벨 부모**까지만이다.
FILE/부분 scanf 판독·최신 rec 선택·실제 ProcessForm/Host 설치·키와
선택 UI는6단계에 남기고 두 대응 행을 todo→partial로 바꿨다.

## 실제 물리·효과 순서

- C++ 호스트의 같은200행×40BYTE, 이전 double 시각과 두 DWORD를
 8016BYTE의 DataView로 보관한다. 행 뷰/바이트 주소를 새 배열로
  바꾸지 않고 미판독 tail 입력을 보존한다. 기본0은 C++ 명시 초기값이며
  원본 미정 heap 기대값이라고 주장하지 않는다.
- Run은 signed current/count 종료 비교 → playing0 → Kill 순서다.
  소비 경로만 물리200행 범위를 확인한다. 현재 시각−이전 시각이 저장된
  단정도 지연보다 strict greater일 때만 두 번째 현재 시각을 저장하고
  인덱스를 진행한다. equality/NaN은 소비하지 않는다.
- 같은 행 span은 하위 효과 사이에도 유지한다. 좌표/이름/인자 필드는
  원문 위치에서 현재 값을 읽는다. 보드 밖 라벨도 시각/인덱스를 먼저
  소비하지만 패킷/이름 조회는 하지 않는다. 한 프레임 한 이벤트만 쓴다.
- 유효 좌표는 caller의25BYTE 이상 패킷에 실제 constructor를 실행한 뒤
  TYPE 이름을 조회한다. Y비트→type lowBYTE→X비트→canon lowWORD→
  회전 lowBYTE 순서를 지키며 품질BYTE16·시각과 패킷 꼬리를 덮지 않는다.
  좌표 bit_cast는 uint32 복사여서 음수0/신호 NaN 비트도 보존한다.
  목적지9에는 같은 원본25BYTE 구간을 보내며 예측 배치를 추가하지 않는다.
- 라벨은 이름 접두/count로 추정하지 않고 현재 보드 판정만 쓴다.
  Notify의 NULL callback은 current 조회보다 먼저 생략한다. 콜백이
  있으면 좌표 판정 뒤 같은 행의 현재 이름을2BYTE 뒤부터 읽고 현재
  callback으로 보낸다. 이전 NUL을 뒤 구간의 종료로 대신하지 않는다.
- Client 전역은 최초 playing0/current 빈 weak 참조다. 각 호출은 현재
  context를 강하게 잠가 상태/훅을 사용한다. NULL/만료는 명시 오류다.
  웹의 명시 dispose는 GC 시점과 무관하게 호스트 소유 종료를 구별하며,
  호출 중 stack pin은 끝까지 유지한다. dispose가 playing을 자동으로
  지우거나 빈 이벤트를 만들지 않는다. 실제 publish/playing1/Host는 잔여다.

## 근거·검증·범위

새 전용 원본 관측은 없다. 제공된 C++ 본문의 명시 입력·소비자 계약을
대조했다. 신규18건은 첫 실행부터 모두 통과했다. 물리 꼬리/폭·효과 뒤
재조회·패킷 품질·signed 종료·지연/NaN·앞선 실패 효과·현재 weak 교체와
호스트 수명 종료를 확인했다. 새 Source 계약을 원본 관측으로 등재하지
않았고 직접 연결된 manifest 항목은0개라806항목을 유지했다.

중단 후 재개 점검에서도 기준선 네 소스의 변경은0이고, 실행/라벨
몸체와 Client 현재 객체 조회가 같은 순서임을 확인했다. 이 두 모듈의
전용 cpppj 검사는 없으며 `ConstructionOwnerStateTests`의 전역 소유
검사는 이번 실행/라벨 몸체의 직접 검사로 세지 않았다. 기존 전체 검사
로그는 최종 요약 없이 중단되어 완료 증거로 사용하지 않고 Node22/26
전체 검사를 다시 실행했다. 이번 범위의 제품 코드 교정은 없었다.

Node v22.23.3·v26.13.0 최종 전체는 각각 **5781건 통과,
실패·취소·건너뜀0**이며 두 실행 모두 종료0이다. 대응표1160행/오류0,
JS131174행·검사771개·manifest806항목과 변경6파일의 UTF-8 판독도
확인했다. Node24는 사용자 지시로 생략하며22/26 통과 시 호환을 가정한다.
대응표는 port845/partial39/todo123, 잔여162/34373 C++행·4-D36/51·
6단계1/8·session0은 같다. FILE 판독을 이식 완료로 세지 않았다.

로그: `extracted/wb-e601-initial.log`, `wb-e601-port-map.log`,
`wb-e601-inventory.log`, `wb-e601-node22.log`, `wb-e601-node26.log`.
재개 검증 로그: `extracted/wb-e601-resume-node22.log`,
`wb-e601-resume-node26.log`, `wb-e601-resume-port-map.log`,
`wb-e601-resume-inventory.log`. 기존 중단 로그는 보존했다.

## 다음 시작점

생성자1652 ConnectConstruction2731..2774로 복귀한다. 같은 Storage의
최근 간헐천5칸/방향9DWORD와 현재 내부 mutable Chat/RemoteTell,
후설치 MayPlace/건설·Builder/사제/Graph 소비자를 연결한다. 위 라벨
부모는 같은 services.buildReplay를 호출 때 선택하며 FILE/Host 설치와
키/선택 나머지는6단계에서 이어간다. Construction 이후 Damageable/
IceTower/CarrierSteal/BombPostPop/AcidZap/AcidFence·전체 모듈이 남았다.

cpppj/보호 파일 수정·디컴파일·게임/브라우저 실행·자연 완주·실제 포획/
희생·특정 적 공격/승리·공중3종 전용 관측은 하지 않았다.
World/Session 공개·Load/Login·프레임·L2/L3는 남고 실제 미션은 아직
플레이할 수 없다.
