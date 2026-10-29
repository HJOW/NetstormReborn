# 실제 일반 Enter·중계·수신 연결 — 단위586

원본10.78의 **0042dc90 중계**를 복원하고 실제 Console의 대상 해석→명령 분배→say→본문 조합→패킷 작성→송신 observer→기존 로컬 큐→중계→수신→공유 스타일 로그를 연결했다. 새 배포본의 TEST02 실제 창에서 평문/행동/스타일/채널 메시지4개와 실제41h 패킷4개를 확인했다. **전체 게임·캠페인 동일성 완료는 아니다.**

현재 PC와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 일치하며 정밀 산출물은 변경하지 않았다.0042dc90은 기존10.72 정밀 자료의 대응이 없는 함수다. 연결한 수신/송신의 CD 대응 기록은 [584](cpp-chat-packet-20261030.md)·[585](cpp-chat-receive-20261030.md)를 따른다. 이번 기준은10.78이며 CD 실행은 없다.

## 중계와 같은 현재 상태

중계는 현재 전송 ID의20BYTE 주소를 먼저 준비한다. 전투이면 같은 패킷 마지막 본문 BYTE에NUL을 쓰고 처음3자의 좁은 CRT 십진수를548710 마스크에 저장한다. 비전투는 이전 마스크를 유지한다. 서버는 처음 읽은 수신자 수를 보관한 뒤 packet+14를1로 바꾸며, 원래 각 항목을 첫 슬롯으로 복사한다. 자기 subscriber를 만났을 때만 마지막 로컬 수신을 실행한다. 빈 서버 수신자 배열도 count1 쓰기 뒤 돌아가는 원문 효과를 유지한다.

`ClientChatWindowGlobals`가 마스크548710·뉴스548748·무시 목록548750/754를 같은 Client 수명으로 소유한다. 객체 메뉴의 별도 무시 벡터를 없애 동일 목록을 읽게 했다. 실제 현재 관계는 `ClientPlayerRecords::Alliances()`, 관전자 BYTE는 Session `HiddenPlayers()`에서 읽으며 같은 모드/닉네임/색을 장면 전환 전 값으로 저장하지 않는다.

주소는 실제 연결 노드의20BYTE를 등록표에서 읽고, 없는 ID는 원문의 zero IPv4 주소를 구성한다. 로컬 두 끝에는 기존 Login이 쓰는 같은 zlocal/포트 형식을 보관했다. 원격 Player 조회·연결 선택·전송은 정적으로 작성했고 이번 새 관측/검사에는 넣지 않았다. 원격 socket의 전체 연결 상태/주소 구현은 후순위다.

**앞선 범위 표현 정정:**584 기록에서 채널1과11을 함께 MP로 표현한 부분은 부정확했다.00594fb8은 요새(편집) 모드이며 이 모드가 채널1을 선택한다. 실제 getter는 이번에 기존 `SquidOwnerMode::fort`와 연결했다.00594fc8은 메타맵의 채널11이다. 앞선 원문 fixture·최초 보고서는 수정하지 않았으며 이번 실제 일반 제출 검사는 단독 전투의9번 경로다.

## 실제 패킷 observer와 큐

기존 observer 등록은 방향 인자만 표현했으므로 패킷 길이를 쓰는0040a8d0에 실제 this를 공급할 수 없었다. `RawPacketDispatch::ObservePacket`과 `PacketRegistration::NotifyObserver`로 같은 원문 슬롯의 mutable 패킷 표현을 추가했다. 두 호스트 표현을 동시에 실행하지 않으며 재등록은 이전 종류를 교체한다. 콜백이 표를 바꿔도 이미 읽은 함수의 수명은 유지한다.

기존 완성 패킷 writer의 전송 계약은 유지하고, 새 `SendMutable` 경계에서 실제 연결 선택 뒤 observer가 같은 패킷을 확정하고 그 현재 WORD만 큐에 복사한다. 일반 Login과 Edit의 같은 등록표에41h observer/중계를 설치하고, Session 반납에서는 자신을 빌린 receive만 해제한다. 이 경로를 직접 로컬 로그 호출로 바꾸지 않았다.

`BindClientChatMessage`는 현재 이름/manager로 기존 say·조합·패킷을 실행한다. 실제 Uber UI는 현재 Session 전송과 같은 수신 부모를 사용한다. 수신 알림은 기존 `PulseRegisteredIfIdle`, 음향은 기존 `WorldAudio().Sounds()`, 로그/표시는 같은 `AppendChatLog`/`ShowChatWindow`다. 시스템 뉴스의 실제 서비스/UI는 선택 시 명시 미연결로 남긴다. 다른 미복원 dot 명령을 성공으로 처리하지 않는다.

## 검증

- 원문 중계132행을 제품보다 먼저 고정하고 2회 재관측 차이0, 총396관측을 확인했다. 전체256BYTE 패킷·현재 마스크·주소/수신 순서와8개 중단 부분효과를 비교했다. 자기 subscriber/로컬 경로만 사용했고 원격 Player/다른 연결 송신은 실행하지 않았다. 최초 도구의 클래스 설명에서 CRT atol 경계를 명시 장치로 정정했고, 계산AST 동일과 마지막 재관측 일치를 확인했다. 최초 도구·보고서는 원시에 보존했다.
- 채팅 집중45개＋분배/전송/연결 활동/Edit 체크포인트 회귀76개＝**121개, 실패0**이다. 현재 등록은3472개다. 실제 Console Enter 뒤 Poll 전에는 로그가 비어 있고, 실제 mutable observer·큐·중계를 처리한 뒤 표시되는 접합 검사도 통과했다. 같은 목록의 무시와 행동 본문도 실제 큐 수로 검증했다.
- 최초 빌드는 지역 변수 이름 충돌1원인/진단3개, 두 번째는 새 검사에서 상속형 Client 연결표의 정의 헤더 누락으로 실패했다. 각 로그를 보존하고 수정했다. 이후 제품 빌드와 기본 패키지 빌드는 경고/오류0이다. Player의 원문 진단이 복귀하면 검색을 계속하는 정적 경계도 최종 제품에 반영했다.
- 실제 배포본 TEST02는22.074초/종료0이다. 실제 문자 큐로 `hello`, `!waves`, `.front ~g`, `styled`, `EVERYONE: team`을 제출했다. 최종 로그는 `You> hello`, `You waves`, `You> styled`, `You> team`이고 실제 Session의41h 수신은4개다. 기존 입력 접두 `EVERYONE:`도 남았다.
- 기본 본문의 색246→front 적용 본문의 색134를 실제 저장 조각에서 확인했고 [최종 화면](../../extracted/cpp-chat-live-message-586-20261030/live/channel.bmp)도 시각 확인했다. 새 프로세스의 원문 Config reader는 저장된 `1chatFormat2=~g`를 읽었다. options.cfg는 기존 암호화 형식이며 원시 텍스트 키 추정 실패를 제품 저장 실패로 합산하지 않는다.
- ZIP1365항목/VC 런타임3DLL이며 사용자 options.cfg를 제외했다. 압축 해제본 게임 SHA는 현재 제품과 같고 동봉 원본 리소스 Netstorm.exe도 보호 원본과 같다. 실제 실행은 클론이었다.

게임 SHA-256은 `e4fe47f4782f411ee9435c2a8c1a5e498546c4a9bad02c3cfecc36c19a729e52`, ZIP SHA-256은 `15c5a1660433de63f436427dbe63ee4ba25a17c9697db2f24216d1782ecd21ca`다. 보호2596파일(원본/CD/AGENTS/정밀 자료1861＋기존fixture735)은 변경0이며 현재fixture736개다.

고정 원문은 [chat-relay-x86.tsv](../../cpppj/tests/fixtures/chat-relay-x86.tsv), 최초 관측은 [native 집계](../../cpppj/chat-relay-native-20261030.json), 최종 빌드/집중·실제 실행·해시는 [runtime 집계](../../cpppj/chat-live-message-runtime-20261030.json), 원시는 `extracted/cpp-chat-live-message-586-20261030/`다. 실제 입력 검사는 [도구](../../tools/cpp_chat_message_smoke.py)로 재생할 수 있으며 색 비교에는 설정이 없는 새 게임 자료 사본을 사용한다.

전체34경로 재검증은 다음587 단위로 분리했다. 이번에 전체 C++·자연 완주·승패/새 캠페인 해제·Win11 실기를 검사했다고 세지 않는다. 포획/희생/특정 적 공격/시나리오 승리·공중3종 전용 재현, 신규 MP/통신 검사는 하지 않았다. 전체 사용자 플레이/원본 동일성과 남은 UI/Edit·수명·호환성·후순위MP/4~6차 목표는 계속 진행 중이다.
