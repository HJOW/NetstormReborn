# 같은 Storage·Host·예약의 실제 Squid 송신 (WB04-E594)

`225553df`의 `client/RawSurfaceStorage.cpp`169·311..325,
`RawSurfaceWorld.cpp`174..179·302..304와 실제 ProcessForm writer 호출을
연결했다. `RawSquidTransmission` 자체는 E313의 원문 본문 그대로다.
행 base는 Storage/World `14fd017c`, wire `be567d46`, Host `417f22d2`이며
이번 해당 cpp/h의 기준선 뒤 변경은0이다. cpppj 송신/World 부모 검사와
기존 원본 fixture 검사를 함께 읽었다. 전용 현재 대여의 원본 관측이나
게임/브라우저 실행은 추가하지 않았다.

## 실제 결선

- Storage가 한 RawSquidTransmission을 지연 보유한다. 같은 Pool/현재
  TYPE·다리 기본 TYPE·같은 Factory의 현재 HP/사제1/4·진단·로그와
  GameAux의 실제2048BYTE/헤더/압축 상태를 사용한다. Acquire/World 생성은
  하지 않으며 일반/준비 World 교체에도 버퍼와 송신기 주소를 유지한다.
- BindPopRuntime은 실제 transmit만 요구하고 동일 현재 대여를 약하게
  보관한다. 등록은 생성/송신/ACK를 하지 않는다. 각 방송/연결/대상/
  명시 ProcessForm 호출은 선택된 대여를 pin하며 중간 반납/교체가 기존
  호출의 함수를 바꾸지 않는다. 새 장치는 다음 선택 위치에서 읽는다.
- Pop은 현재 대여의 실제 transmit을, 대여가 없으면 같은 장기 본문의
  Pop을 선택한다. writer가 실제 패킷을 먼저 쓰고 양수 반환 뒤 방송 또는
  현재 연결/대상 송신을 선택한다. 장치 누락/예외여도 앞선 쓰기는 남긴다.
- World는 임시 `(sid, flags)` 성공 함수 대체를 제거하고 실제 장기 본문을
  연결한다. Runtime이 아직 없으면 **writer 실행 뒤** 원문 방송 조기
  반환을 한다. 대상 연결/송신은 실제 Runtime을 요구한다. Runtime 최상위 함수
  표는 C++ 값 인수처럼 보관한다. 실패/종료는 자기 짧은 대여만 반납한다.
- ProcessForm은 현재 World의 실제 Host를 사용한다. 외부 Kernel을 쓰는
  독립 World의 프로세스를 장기 Kernel에서 다시 찾지 않는다. Pop 대여의
  processPacket이 없으면 같은 Storage Host/PID의 WriteFormPacket으로
  이어진다. 전체 DWORD 인자·같은 예약·실제0 writer를 보존한다.
- 삭제는 같은004ac260의 `(-1,-2,0,flags)`를 거친다. 공통8BYTE/현재 선언
  WORD·최근 SID/token 소비와 두 client SID 진단 순서도 원문 부모를 쓴다.

## 검사·첫 실패

초기 기존55건 중47건 통과/8건 실패였다. 옛 임시 송신 훅을 직접 쓰던
3건과 Runtime을 연결하고도 자산 방송 장치를 제공하지 않던5건이다.
제품의 누락 오류를 완화하지 않았다. 전자는 같은 실제 송신기의 Pop을
관찰하고 원문 호출을 계속하는 검사 전용 spy로, 후자는 실제 writer의
패킷을 별도 배열에 기록하는 관측 장치로 갱신했다. 자산/피격/커서
공유 검사 장면도 같은 관측을 추가했다. 기존 Form/효과 로그와 섞지 않는다.

신규16건의 첫 실행은15건 통과/1건 실패였다. 외부 Kernel의 장기 Host
fromForm이 null을 반환하는데 예외를 기대한 검사 입력을 바로잡았다.
보강 Regular 검사에서는 API에 실제 RegularProcess를 주지 않은 오류와
client Form의 앞선 진단 장치를 누락한 오류를 교정했고0x2d8 진단도
기대값에 포함했다. 신규19건과 기존 wire/Host·World/지형·Session 기반·
자산/피격/커서/게임플레이를 묶은 **관련174건은 모두 통과**했다.

독립 기대값으로35BYTE/선언27·8BYTE/다른 선언 WORD/표식·같은 예약
alias·현재 HP 압축 마스크·전체 DWORD 인자·예외의 부분 쓰기·같은 Host
선택을 대조했다. 이전 송신 본문의 원본 fixture/출력 자체는 변경하지
않았다. manifest806항목 중 bridgedecay1개 구현 추적만 갱신했다.

Node v22.23.3·v26.13.0 최종 전체는 각각 **5,687건 통과, 실패·취소·건너뜀0**이다. 대응표1160행/오류0도 확인했다. JS130468행·
검사764개이며 port845/partial36/todo126·잔여162모듈/34373 C++행은
같다. 4-D 선행 완료를 포함해36/51이다. Node24는 사용자 지시로 생략하고
22/26 통과 시 호환을 가정한다.

로그는 `extracted/wb-e594-initial.log`, `wb-e594-new.log`,
`wb-e594-focused.log`, `wb-e594-node22.log`, `wb-e594-node26.log`,
`wb-e594-port-map.log`다.

## 남은 경계·다음

이번 결선은 현재 Source의 typed World/장기 송신 부모다. Native의
BindTransmissionDevices/mutable Player observer와 Processes의 기본
Lifecycle/FormFactory·SerializeProcessForm·기본 Form 방송 설치는 실제
Client 결선에서 남는다. World/Session 전체 Runtime pin 공개·수신기/
Peer 동기화도 아직이며 이것들을 typed 관측으로 대체하지 않았다.

다음은 Session 생성자1611 RawWorldRuntimeHooks(현재 Asset 삭제가
World.TransmitDeletion을 요구하던 부모가 이제 준비됨) → 이동/건물 →
전체 모듈 생성이다. 생성자는 아직1610까지이며 실제 Runtime 공개/
Load/프레임·L2/L3는 없다. 미션은 아직 플레이할 수 없다. 다른 지역
Graph/Pop/PostPop/Frame/Owner 등의 Storage 소유권 전환도 잔여다.

cpppj/보호 파일 수정·디컴파일·자연 완주·실제 포획/희생·특정 적 공격/
승리·공중3종 전용 관측은 하지 않았다.
