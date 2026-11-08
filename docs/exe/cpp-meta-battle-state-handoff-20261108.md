# 1153 — M2.2b 메타 State7과 실제 전송/콜백 인계

제품1153/표시·파일·제품버전11.01이다. 직전은1152(`efc78ee5 / 20261108-121 CP`)다. **M2.2b 배선을 구현했고 다음은M2.3/1ah**다.1ah/1bh 전체 수신/등록·선택적 기록Open은 미연결이며 M2 전체 완료가 아니다.

원문4b2df0(1)은 Zone 예약이0이면1..8을 끊고9를 남기며, 비영이면9를 끊고1..8을 남긴다. 원문4e2620은 현재 연결의 준비값이비영이면 주소 해석 전에1을 반환한다. 이 준비값은 단순 연결 성공 상수가 아니며-1도 비영이다. 새 일반 전송이 다른 TCP owner를 만들면 공유 표의9는 남아도 그 객체를 고를 수 없었다. MissionNetwork가 같은 TCP/UDP·전체 메타 callback owner를 pin하고, 새 CompositeMissionTransport는 그들의 동일 Registry/등록표를 확인해 socket/PacketConnection/depth/입출력 버퍼를 그대로 사용한다. 현재 World의 시계/기록/수명 hook만 재바인딩한다.

MetaConnectionRuntime/EditorRuntime은 실제 공유 소유권을 사용한다. Source Pump의 지역 strong owner가 UI 필드 반납 뒤까지 살아 있고 새 일반 전송도 전체 메타 callback owner를 보유한다. TCP/UDP 소멸자는 마지막 실제 owner에서 자기 노드만 반납한다. 기존 기본 소멸자/정상 cleanup의 분리를 유지하며 전송을 복제하거나 전체 표를 새로0으로 만들지 않는다.

4e2dc0의 선택된 일반 송신 owner1..8만 현재 Player base의+50 BYTE를 읽는다. 원문처럼 직접 가상 송신과9/10/11은 이 읽기를 건너뛴다. 메타/실제 Client 전송은 장기 ClientPlayerRecords의 현재 Records getter를 빌리고, 독립 장치의 기존 고정 span은 유지한다. cleanup이Player를 재할당해도 오래된 span을 읽지 않는다.

메타의 next7은 기존 Editor 분배기의 PollStarting, current7의 직접 Run은 같은 분배기의 RunStarting으로 실행한다. 별도 다음Tick command/대기실 ACK를 만들지 않는다. 기존 LAN의 일반 World 본체를 공통 함수로 옮겨 분모→현재7 캡처→cleanup1→CD→명령 숨김→ticks/source 복원→전투/음악→현재 이름/같은 서비스/일반 World→실제 전송/1152 Zone 소비자 순서로 연결했다. Native 메타는 캡처 Zone 예약으로 host/client 역할을 정하며 현재 State+80을 실제 장부에서 설치/반납한다.

수신 중 cleanup은 명시적으로 같은 현재 메타 shared owner를 넘긴 경우만 허용한다. 다른 World/메타 callback의 기존 정리 금지는 유지한다. Pool/Player 정리 순서나 실제 Native 효과를 생략하지 않는다. 같은 분배기의 즉시8에 실제 wall/progress/현재World Connect를 연결했다. 메타의 current+54는 C문자열임을 새 요청 필드로 구별하고, 바이트를 먼저 훑지 않은 bounded view를 전송에 넘긴다. TCP는 기존9 준비 조회 뒤에만 NUL/주소를 해석한다. 기존 일반 LAN의 binary20BYTE·저장 복구 경로는 그대로다.

열린UDP12도 인계할 수 있어 복합 전송의 같은 전역ID순회·send/observer·수신·open/원문 준비0 조회·close에 연결했다. 같은12 노드를 새로 열지 않는다. 기존 메타 단독 UDP Poll도 같은 per-ID 본체를 사용한다. 인계가 일어난 프레임은 옛 Pump가 실제 현재 표의 수신을 마친 뒤 새 World의 별도 Poll을 중복 실행하지 않는다. 원문 State7/즉시8 자체를 뒤로 미룬 것은 아니다. 실제 Login 완료 후 기존 일반 UI/입력 공개를 재사용한다.

4b6dd0/4b2df0/4e2620/4e2dc0 C/PE, Zone 예약에 따른9/1..8 정리·현재9 준비 비영 재사용 선행·같은 실제 transport/표·현재 Player BYTE·State7 캡처/cleanup/CD/명령/복원/음악/일반 설치·동기 RunStarting/즉시8·UDP12 전역ID·callback owner/중복 Poll 방지를 대조했다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0, 보호2800파일 불변이다. 새 C++ 단위/MP/게임/통신/전체/ZIP 실행0이다. 같은 노드 인계/실제 진입이 실행 검증됐다는 뜻이 아니다. 원시 C/ASM·빌드는 Git 제외 `extracted/cpp-resume-1153/`에 보존한다.

Netstorm.exe 13,134,848바이트/SHA-256 `15ed722115e14fc79e1b63b53f7abe095ce4c48d28f7b4b7793a39e3334f0c44` / R.exe 670,208바이트/SHA-256 `ce7efa72b4e7190750332cdf7fc7c528ca507a52386dbc8e7d88603170c87044`

마지막 전체 디컴파일2026-11-03/HJOW-Athlon, R전용2026-11-07/동일 호스트이며 생성 원문은 수정하지 않았다.

다음 M2.3은1ah의 checking/주소·현재 포트·가변복제·예약 증가·기록Open·10/11 정리, M2.4는1bh의server/예약 관문·checking·기록Open·현재 주소 복사/포트/current7·직접Run을 실제 등록한다. 추가 sender/phase/known 관문을 합성하지 않으며 checking은 원문 복귀 순서로 처리한다.3차 잔여는M2~M8의7묶음이다.
