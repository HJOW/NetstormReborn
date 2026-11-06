# LAN TCP 전송 부품 — 단위982

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 4번 활성 비멀티 범위980·7번 와이드 덱981 뒤8번의 TCP 전송 부품을 컴파일했다. **현재 Login·로비·일반 전투·메인 Multiplayer UI에는 미접합이며 멀티플레이 완료가 아니다.** 사용자 지시에 따라 신규 단위/패킷/소켓/통신/게임 검사는 작성·실행하지 않았다.

## 원문과 구현 범위

원본은 WSOCK32 TCP를 직접 사용한다. DirectPlay 변환이나 새 wire 프로토콜을 만들지 않는다. `LocalTcpTransport`는 현재 `RawConnectionRegistry`와 원문 분배 부품을 공유할 수 있도록 실제 노드·소켓·부분 입력/출력·pending 토큰을 소유한다. 생성 시에는 소켓을 열지 않는다.

| 원문 | 현재 부품 |
| --- | --- |
| 004e2430/004e02d0 | 명시 Listen·실제 accept 뒤31개 pending 슬롯 등록. accepted 노드 선생성 없음. |
| 004e2620/004e2b70 | 명시 IPv4/20BYTE 주소 Connect·nonblocking select/SO_ERROR 완료. bit2 buffered와 접속 완료를 분리. |
| 004dfc00 | version/type/little endian WORD 길이의4BYTE 프레임을 TCP stream에서 부분 수신. 단일 observer0를 원문 위치에서 호출. |
| 004dff00/004e0150 | 현재 Player/연결 관문→mutable observer1→선언 길이 송신. type4 집합 확정·partial send 접두 보존·backlog100000. |
| 004df9f0/004df440/004e03d0 | type1/2 통지·현재 flags/depth·실제 Registry close/free와 최종 C++ 자기 소유 반납을 분리. |

단일/집합의 기존 `LocalMissionTransport` 분배를 Open 없이 사용한다. 별도 length-prefix는 붙이지 않는다. mutable observer 뒤 현재 선언 길이/폐기 패킷을 다시 확인한다. 7f/80의 구 CD/클라이언트 검증 요청·응답은 송수신에서 제외한다. 미지원 중첩 집합·지연 큐를 base 성공으로 바꾸지 않는다.

직접 접속/수락은 loopback127/8·RFC1918 사설·IPv4 link-local169.254/16의 숫자 주소로 제한한다. bind는0.0.0.0 또는 같은 로컬 범위다. DNS/공식 서버·인터넷 디렉터리·ping/검증 서비스를 만들지 않는다. 공인 주소를 사용하는 특수 LAN/IPv6는 현재 범위 밖이며 이는 LAN 전용 이식 정책이다. unbuffered short-send도 대기 큐에 보존하고, 원문의 SO_REUSEADDR bind 재시도 대신 실제 bind 오류를 전달한다.

## 빌드와 명시한 미검증

새 `client/LocalTcpTransport.h/.cpp`를 netstorm 정적 라이브러리에 등록하고 Windows `ws2_32` 링크 의존성을 추가했다. Release 첫 명령 종료0·경고0·오류0이다. `LocalTcpTransport.obj`가 생성됐고 게임도 다시 링크됐다. **아직 실제 caller가 없어 현재 EXE에는 WS2_32 import가 없다.** 따라서 이것은 부품의 컴파일/제품 재링크 결과이며 모든 부품 기호의 실제 활성 링크·소켓 동작 성공으로 확대하지 않는다. 후속 Client owner 접합 때 실제 기호/의존 링크를 확인한다.

현재 제품982/표시·파일·제품11.01의 정확한 크기/해시와 보호 결과는 [단위 보고서](../../cpppj/lan-tcp-transport-20261106.json)를 따른다. 마지막 GUI는981이며982의 게임 실행 결과가 아니다. 새 Native CPU/fixture/MP 검사/소켓/통신/게임/전체 CTest/34미션/ZIP 실행은 없다.

## 다음 연결

1. 같은 registration 표를 local/TCP 분배기가 빌리고, 각 전송의 실제 close/free 장치는 분리한다. 수신 callback은 현재 Session 소유자를 호출 종료까지 유지하고 detach 뒤에는 다음 Session을 겹쳐 등록하지 않는다.
2. 두 전체 Poll을 단순 연결하지 않고 accept→pending 각각1회→normal0..21의 전역 원문 순서를 per-ID coordinator로 조립한다. host 내부 자기 끝과 원격 TCP가 공존해야 한다.
3. 실제 Client 장기 owner/Session 대여→Login의 ID별 Status/Open/address/송신·simulconnect→일반 roster/State6·7·Fort 수신·Commander→LAN UI를 연결한다. `!missionWorld || !missionPost` guard를 강제 단독 값으로 우회하지 않는다.
4. 일반 전투 저장/복구·메타맵/Challenge/Descend·랭크/의식/Outpost·checking8과 각 writer/decoder는8번 잔여로 유지한다. 원문 근거의 구현과 사용자 제외 검사를 구별한다.

준비/원문 발췌는 `extracted/cpp-tcp-preparation/PLAN.md`, `extracted/cpp-resume-977/audit-mp/transport-plan.md`다. 다음 공유표/복합 owner 반영안은 `extracted/cpp-mp-login-preparation/pending-new/`에서 준비 중이다.
