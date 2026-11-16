# 실제 ProcessForm 패킷 writer 부모 (WB04-E591)

`225553df`의 `o/SquidProcess.cpp`231..251 WriteFormPacket과
95..100 Transmission/BindDefaultFormTransmission, RuntimeCall의 pin,
Regular의 실제 shared writer 선택을 옮겼다. cpp/.h는 기존 행 base
417f22d2 및 현재 기준선 뒤 변경0이다. 직접 cpppj WriteFormPacket 시험과
전용 원본 fixture는 검색에서 찾지 못했다. 이번 검증은 실제 Pool/Kernel/
Host와 독립 패킷 기대값을 쓰는 소스 계약 대조다.

## LAN 전용이라는 옛 판정 정정

기존 SquidProcess 대응표의 ① 판정은 WriteFormPacket도 LAN 전용으로
제외했다. 현재 RawSurfaceWorld.cpp176..179는 단독 World의 PopRuntime에
Host.WriteFormPacket을 연결하고 RawSurfaceStorage.cpp318의 일반 Squid
writer가 이 부모를 소비한다. 따라서 **WriteFormPacket의 LAN 전용 판정은
철회**한다. ReceiveSharedRegular의 단독 도달은 실제 수신기/동기화 결선에서
별도로 재확인해야 하며 이번에 수신 완료로 세지 않는다.

다음 Session1611의 Runtime이 쓰는 일반 Squid/ProcessForm 송신 경로를
완성하는 데 필요한 부모라4-D C 선행 항목으로 추가했다. 상위 전송을 임시
Form 패킷이나 성공 함수로 바꾸지 않는다.

## 보존한 현재 상태·폭·순서

- 현재 Runtime을 pin하고 activeCalls를 올린 뒤 원문 부모 SID 진단을 한다.
  진단이 PID를 바꾸면 복귀 뒤 같은 실제 Kernel의 현재 객체를 선택한다.
  정상/예외에서 자기 호출 수만 반납하고 pin 실패는 증가 전 중단한다.
- 일반 Regular와 허용되지 않은 공유 Regular는0이다. 실제 shared 플래그를
  부착 전에 저장하고 Form TYPE를 나중에 바꿔도 이 플래그로 writer를 고른다.
  예약 재생성은 새 일반 생성자의 false로 되돌린다.
- 공유28BYTE는 현재 헤더/객체 부모/double 시각/사건/float payload/count를
  원래 위치에 쓴다. +4/+5와 뒤쪽 바이트는 보존한다. 고정 FormPacketHeader
  길이는 WORD라16비트로 좁힌 뒤 최소4를 적용한다(압축 DWORD 헤더와 구분).
- 다른 프로세스는 현재 전체 파생 writer와 패킷 버퍼를 사용한다. 양수 크기만
  범위를 확인해 복사하며 같은 시작 주소는 건너뛴다. 함수 표는 C++ 값 인수처럼
  설치/호출 때 복사하고 함수가 읽는 실제 메모리는 복사하지 않는다.
- 현재 전송 표에 네 함수 중 하나라도 있으면 그 표를 선택한다. 현재 World가
  없거나 표가 비었으면 같은 장기 기본 표를 쓴다. 기본 설치는 borrowed Host의
  최초 한 번이며 실제 함수4개와 진단을 요구한다. World/Kernel을 만들지 않는다.

## 검증·최초 실패·집계

첫 집중12건 중9건이 통과했다. 실패3건은 검사에서 현재 World의 표시
진단보다 독립 state 진단이 우선한다고 잘못 가정한2건과 실제 core.destroy를
baseDestroy라는 없는 이름으로 지정한1건이었다. 실제 World 진단 장치와
명시 NoWorld Host/올바른 삭제기를 써 입력을 고쳤다. 제품 단언 완화는 없다.
quiet NaN/무한대/음수0·double NaN 비트와 장기 기본 장치2건을 보강해
신규15건이며 기존 Process/장기 Host와 묶은 집중45건도 모두 통과했다.

정적 검토에서는 공유 헤더를 DWORD로 생각했던 초안을 WORD로 바로잡았고
함수 표의 입력 객체가 설치 뒤 바뀌어도 영향을 주지 않는 값 복사를 확인했다.
새 fixture/원본 실행은 없으며 manifest806항목을 유지한다. 기존 관련6개
fixture의 구현 추적만 갱신했다. Node v22.23.3·v26.13.0 최종 전체는
각각 **5,640건 통과, 실패·취소·건너뜀0**이다. 대응표1160행/오류0이며
구현 JS130170행·테스트761개·manifest806개다. port845/partial36/todo126,
잔여162모듈/34373 C++행은 같고, 4-D는 선행 항목을 포함해33/48이다.

로그는 `extracted/wb-e591-focused.log`, `wb-e591-node22.log`,
`wb-e591-node26.log`, `wb-e591-port-map.log`에 남겼다.

## 남은 경계·다음 시작점

Session 생성자는 여전히1610까지다. 다음은 RawSurfaceStorage311..325/
RawSurfaceWorld174..179·302..304의 실제 Squid 송신/삭제 부모를 연결한 뒤
Session1611 RawWorldRuntimeHooks로 이어간다. 장기 Factory/PopRuntime의
현재 수명과 동일 GameAux2048BYTE/진단/Host를 써야 하며 별도 송신 상태를
만들지 않는다. DefaultLifecycle/FormFactory·Native Kill·전체 수신/동기화는
기존4-D 잔여다. 실제 Runtime 공개·Load·프레임·L2/L3도 남았다.

cpppj/보호 파일 수정·디컴파일·게임/브라우저·자연 완주·실제 포획/희생·
특정 적 공격/승리·공중3종 전용 관측은 하지 않았다. Node24는 사용자 지시에
따라 생략하고22/26 통과 시 호환을 가정한다.
