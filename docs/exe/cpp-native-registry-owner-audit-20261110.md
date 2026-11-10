# Native 전체 등록표·연결 owner 정적 감사 — 작업1262

**제품1261을 유지하며 M6-l의 기본 지연큐/Native 등록표·연결 owner 구현과 정적 감사를 마감했다. 원문92행 중 활성 수신91행, 관찰/버전102행과 전체 길이138WORD를 대조했다. MP 실행 검증과 전체 NoWorld·모든 process/caller·M6/전체 목표의 완료를 뜻하지 않는다.**

## 집계 기준과 전수 대조

| 기준 | 원문 | 현행 범위 |
| --- | --- | --- |
|40b4b0의 수신 메시지 행|92|폐기7f를 뺀91행 일치|
|같은 수신 함수 주소 중복 제거|86|85주소 일치|
|observer/version 등록 행|104|폐기7f/80을 뺀102행 일치, 모두 version1/policy0|
|C3 004437f0 공유 행|7|0c/42/4d/55/56/57/5a 유지; 보호 PE의 실제 C3 확인|
|receiver 없는 observer 전용|12|80을 뺀11행 유지|
|NULL observer|9|37/38/39/3a/3b/3c/6a/73/74 그대로|
|0052ec40 길이 표|138WORD|실제 생성자가 사용하는 현행138WORD와 전부 일치|

원문 밖1(004e2f00)/2(004b6660)/5(004c1f60)를 위92행에 합산하지 않는다. 세 개를 모두 포함한 **메시지 키94 / 중복 제거 코드 주소88**은 집계 모델이며, 기본 설치 직후의 무조건 활성표 수가 아니다. 1은 실제 backend bootstrap,2는 Client 기본 설치,5는 원문의 InitializeServer/Attach 때만 등록한다. 5를 앞당겨 무조건 설치하거나 반환 때 지우지 않는다. 종전 '태그25/활성88/무태그63F'는 이 코드 주소 모델의 과거 조사값으로, 최신 메시지 행 수나 host 함수 객체 수가 아니다.

네 helper의 실제 spec63행(Login67 중복 제외), 기존 장기21행, 같은 C3 7행과 observer 전용11행을 각각 실제 설치 본문에서 대조했다. 16진/10진 표기를 정규화했고 receiver/observer/version/policy/길이와 실제 기본 부모 연결을 확인했다. 상세102행·소스/줄은 [보고서](../../cpppj/native-registry-owner-audit-20261110.json)의 `rows`다. `.cpp/.h/.inc` 전체의 등록/관찰/버전/길이/반납 후보는55파일336줄이며, 후보 수를 실제 패킷 writer 수로 주장하지 않는다. Config/AI/DotCommand의 같은 이름 메서드와 독립 typed/CD·R·별도89h 대기실을 구분했다.

## 실제 호출·소유 경계

| 실제 소스 / 경계 | 대조한 연결 |
| --- | --- |
|client/ClientCampaignTransport.cpp / MakeClientCampaignTransport|공유 Registry/표·현재 Client 전역·실제 장기 기록 owner를 사용하고 Publish 뒤 BindSession 전에 기본 F를 설치한다.|
|client/MetaConnectionRuntime.cpp / 생성자|실제 TCP/UDP가 같은 등록표/Registry를 공유한다. 기본 설치 뒤16/1c/1e/1f의 기존 metadata만 다시 쓴다.|
|client/UberEditor.inc / Editor 생성|실제 Editor 생성 뒤 Open 전에 같은 기본 설치 부모를 호출한다.|
|client/RawCampaignSession.cpp / Native gameplay/Editor 등록과 반납|알려진50행은 같은 F를 빌린다. Editor64/50행/41/52를 실제 저장 baseline으로 복구한다. typed의 short 등록은 별도 자연 분기다.|
|client/RawCampaignSessionLogin.inc / Login 등록|31/27/63/Client8/72/3d/78의 Native 등록은 같은 고정 F를 대여하며32/71은 기존 long F를 빌린다.|
|client/CampaignTransportServices.cpp / BindCompositeCampaignTransport|실제 전체 owner lease를 만들고2/69는 같은 F를 먼저 복사해 재등록한다. 독립 typed fallback을 Native F로 포장하지 않는다.|
|o/PacketRegistrationTable.cpp / BindSession/Detach/PersistCanonicalBaseline|현재 전체 owner를 실행 중만 pin하고 touched 슬롯만 복구한다. clock5의 실제 전역 등록만 반환 baseline으로 승격한다.|
|o/RawClockSynchronization.cpp / InitializeServer/Attach|원문 밖5는 실제 초기화 때만 설치한다. F는 LocalState·실제 Client 시계/Registry만 보유하며 과거 Login this를 저장하지 않는다.|
|o/RawPacketDispatch.cpp / SnapshotReceiver/DeliverReceiver|queue에는 immutable F만 보관하며 short closure를 거부한다. 직접 배송의 실제 lease pin과 enqueue된 F를 구별한다.|
|o/RawPacketDelay.cpp / Enqueue/Pump|같은 Client 분포/CRT/wall과 실제 node/address를 사용한다. F를 observer1 전에 고르고 현재 선언 길이를 복사한다. FIFO/strict deadline/주소·깊이 복구를 유지한다.|
|client/ClientPacketSources.cpp / Resolve/Shutdown|weak 실제 Source 경로를 저장하고 선택 배송/최종 정리 때만 실제 device controlblock을 보유한다. 종료는 node 뒤 listener/weak 장부 순서다.|
|client/ClientConnectionGlobals.cpp / BindDevicePin/ShutdownDevices|같은 node의 실제 Source를 보유하며 queue에 World/Session pin을 붙이지 않는다. App 최종 경계에서 순환을 끊는다.|
|client/CompositeMissionTransport.cpp / Poll/Send|coordinator 전체를 Poll 중 보유하고 모든 backend의 Begin/End를 정상·예외 모두 처리한다. pending은 각1회/현재 정상22칸은 sentinel까지 읽는다.|
|client/LocalTcpTransport.cpp / 소켓/node/지연큐|같은 node에서 queue를 pump한다. activeCalls/depth와 retired 상태를 구별하고 queue를 실제 release 때 비운 뒤 최외곽 Poll에서 node를 수거한다.|
|client/LocalUdpTransport.cpp / 소켓/node/지연큐|실제12번 노드의 저장 주소로 배송 뒤 원래 recvfrom을 진행한다. queue/reset·소켓/노드 반납은 같은 실제 owner가 담당한다.|
|client/ClientPacketRecord.cpp / Open/Write/Close|장기 FILE와 실제 FrameTime을 공유한다. 방향/현재 wall/frame/address/sender의21BYTE 접두 뒤 현재 선언 길이 본문을 기록하고 홈 경로에 쓴다.|
|client/ClientMain.cpp / 현재 표 공개/최종 소멸|현재 표는 Client의 실제 shared_ptr다. Source/queue/Session 정리 뒤 nativePacketTable을 반납하며 짧은 Source 수명과 혼동하지 않는다.|
|client/LanBattleLobby.cpp / 독립89h|별도 CLI 대기실 계약/queue OFF의 short receiver이며 원문92행에 합산하지 않는다.|
|root/PacketRegistration.cpp / R 전용|R의 RegisterDirect/ResetCallbacks를 Client 활성표 writer로 세지 않는다.|

기본 부모는 Meta 생성자, 일반 Composite 생성/재진입, Editor 생성의 세 실제 위치다. 해당 Source 안의 TCP/UDP는 하나의 실제 등록표를 공유한다. Client가 공개하는 현재 전역 표와 과거 Source의 표를 혼동하지 않으며, 저장 F가 배송 때 현재 packet 종류/현재 표의 G로 다시 분배하지 않는다. 31h의 Login 별도 선행 등록도1260에서 같은 F로 교정됐고 이번에 `.inc`까지 포함해 확인했다. Native 재등록과 typed 대체 경로를 구별했다.

## 검증·계획

이번 단위는 읽기 전용 정적 감사다. **제품 소스2,283개·보호 원본/CD/fixture/현행 AGENTS22a0877c3 2,808개·Netstorm/R SHA-256이 모두 불변**이다. 새 빌드/단위 검사/GUI/MP/기록 실행/디컴파일/ZIP은0이다. 최근 제품1261의353.615초 Release·비MP300/300·Tutorial4 생산/건설/600프레임·일반Edit2회는 이전 단위 근거로 유지하며 이번 실행으로 합산하지 않는다.

현재 호스트와 마지막 Client(2026-11-03)/R(2026-11-07) 디컴파일 호스트는 HJOW-Athlon으로 같다. AGENTS는 수정하지 않았다. 오류/예외 공통 처리·상세 홈 로그 E1~E3는 사용자8661e1060 결정의 **7차**로 반영돼 있고 한국어 K1~K5 뒤에 진행한다. 기타 일본어 등은11차다. 사용자22a0877c3의 WB8/9 제외·7차 console 정책은 별도이며 CPP 순서를 바꾸지 않는다.

다음은 **M6-m JoinTestBattle**의 실제 개발자4이름/현재주소/server0/current29와 선행4b3620→기존1bh4b8b20 직접 호출이다. Editor TestBattle(c2)와 구별한다. 큰 NoWorld/자산·AI/배치/Fort 상위 Finish, 모든 process VT와 Snapshot/debugBurst 등 M6-n 전수 감사는 남는다. 새 수신 본문·가짜 ACK·성공상수를 만들지 않는다. 원시 근거는 `extracted/cpp-resume-1262/`다.
