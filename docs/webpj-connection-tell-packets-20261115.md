# 연결 종료·연결 감시·원격 Tell·개발자 명령 패킷 이식 (WB04-E559)

`o/` 아래 연결 수명 두 단위와 문자열 패킷 두 단위를 한 묶음으로 옮겼다. 네 단위 모두 원본 관측 fixture 가 없고 cpppj 에도 전용 시험이 없어(상위 클래스가 간접으로 쓸 뿐이다),
cpppj 소스 계약(원문 주소·필드 폭·순서·경계)을 웹 시험이 직접 단언한다. 기준선 `225553df` 과 현재 cpppj 소스가 같음을 확인했다(기준선 뒤 변경 없음).

| 단위 | 웹 모듈 | 원문 | 시험 |
| --- | --- | --- | --- |
| `RawConnectionEnded` | `www/core/raw-connection-ended.js` | 004b6660 | 10건 |
| `RawConnectionHealth` | `www/core/raw-connection-health.js` | 004b9010 | 11건 |
| `RawRemoteTellPacket` | `www/core/raw-remote-tell-packet.js` | 004c9150·004c9200(쓰기)·0040b090(관찰)·004cf980(수신) | 7건 |
| `RawDeveloperCommandPacket` | `www/core/raw-developer-command-packet.js` | 0040a6b0(관찰)·0044dba0→0040a6f0(수신) | 5건 |

## 옮긴 범위

- **연결 종료 통지(`receiveConnectionEnded`)**: 현재 동기화 token 과 같은 연결이 끝나면 동기화 DWORD 를 0 으로 되돌린다. 서버가 아니면 서버(9번) 끝에서 서버 실패 저장 뒤 서버가 되었는지 다시 읽어 되었으면 즉시 반환하고, 아니면 연결 종료를 요청한다. 서버이고 9번이 아니면 연결 종료만 하고 9번 자기 끝은 닫지 않는다.
  서버이고 sender 가 1..8 이면 Player flags(+0x50)를 세 번 따로 store(0x100 설정 → 비트 0 해제 → 비트 0x40 해제, 매번 현재 Player base 를 다시 빌림)하고 Player 송신 뒤에만 활동 프레임을 0 으로 지운다. sender 10·단계 23·다음 상태가 24/12 가 아니면 다음 상태를 20 으로 예약만 한다. 읽기 전용 전역은 상자·함수·단순 값을 받아 각 원문 위치에서 다시 읽는다.
- **연결 감시(`pollConnectionHealth`)**: 연결 확인 상태(29/33)에서 연결 11 이 없으면 반환 → 상태 14 가 아니면 다음 활동 끝을 벽시각+1.0 → 끝이 지났으면(<=) 송신하고 송신 효과 뒤 서버 권한을 다시 읽어 3.0/10.0초 간격을 새 벽시각에서 계산 → 현재 owner 와 프레임 벽시각을 이 순서로 캡처 →
  서버면 Player 1..8 의 flags 1·15초 초과·활동 프레임·로컬 owner 아님·전투 종료 아님일 때만 종류 2 안내(매 반복 현재 9행을 빌림) → 클라이언트면(서버 이전 중이 아니면) 연결 9 의 8초 초과 안내(종류 1)와 연결 없음/20초 초과+프레임 차(unsigned) 20 초과의 자동 서버 이전 저장(true 반환). 활동 초는 signed DWORD 로 해석한다.
- **원격 Tell(70h)**: `writeRemoteTellPacket`(264바이트 0xff 채움·헤더·길이 DWORD 선기록·255 초과 진단 복귀·256 초과 물리 범위 오류·NUL 복사)·`observeRemoteTellPacket`(송신에서만 sentinel 진단 뒤 현재 길이를 다시 읽어 길이+8 하위 WORD)·`receiveRemoteTellPacket`(+8 의 첫 NUL 문자열을 소비자에 전달)·`registerRemoteTellPacket`(버전 1/정책 0·가변 길이·관찰기·수신기).
- **개발자 명령(31h)**: 같은 규약의 송신 관찰기(`len != (int)-1`, 줄 0x74b)와 수신, 등록. 수신은 sender/연결/선언 길이를 읽지 않고 해석기가 없으면 물리 검사 뒤 명시 오류이다.
- 문자열은 바이트 값(문자 코드 0..255)의 JS 문자열로 주고받는다. 이 모듈들은 새 Native 고정 수신기(canonical) 등록을 만들지 않는다(웹 분배기에 없음).

## 시험과 변형 확인

- `tests/raw-connection-ended.test.mjs` 10건: 동기화 token(unsigned 비교), 클라이언트/서버 종료 선택과 9번 서버 실패 저장·서버 승격 반환, 서버 flags 세 번 store(서로 다른 현재 base·인접 바이트 보존·비정렬), 방송 실패 시 활동 프레임 보존, 다음 상태 20 예약 조건, 현재값 재조회, 선택 장치만 요구, flags 물리 경계.
- `tests/raw-connection-health.test.mjs` 11건: 상태별 조기 반환, 활동 송신 경계(<=)·송신 인자와 간격·새 벽시각, 서버 Player 안내 조건 전부와 signed 활동 초·물리 9행, 클라이언트 서버(9) 안내와 자동 서버 이전의 경계(경과 8/20초·프레임 차 20·unsigned 감김), 캡처 순서, 선택 장치 요구, 상자·함수·단순 값.
- `tests/raw-remote-tell-packet.test.mjs` 7건·`tests/raw-developer-command-packet.test.mjs` 5건: 쓰기 규격·길이 경계(254/255/256자), 관찰기 sentinel 재조회와 32비트 감김, NUL 문자열 수신과 오류 종류(물리/NUL/소비자), 실제 `RawPacketDispatch` 등록과 분배.

**변형 확인:** 연결 종료 47종·연결 감시 60종·원격 Tell 50종·개발자 명령 32종, 총 189가지를 넣어 동등 4가지를 뺀 모두 시험에서 잡혔다(처음 살아남은 13가지 중 동등이 아닌 9가지는 함수 형태의 거짓 값·연결 9 조회 번호·8바이트 헤더 경계·오류 종류 메시지 시험을 보강해 잡았다).
동등 변형 4가지: 연결 종료의 flags 쓰기 폭 3바이트(마스크가 건드리는 비트는 모두 낮은 3바이트 안이라 네 번째 BYTE 는 같은 값을 되쓴다)·세 store 의 `(값|설정)&~해제` 순서(설정/해제 비트가 겹치지 않는다)·owner `>>> 0` 대 `| 0`(sender 1..8 에서 같다), 원격 Tell 쓰기의 `& 0xff`(`Uint8Array` 저장 절삭과 같다).

## 남은 경계

- 실제 Client 연결 객체 표(`connectionExists`·`disconnect`)·Player 송신(`broadcastPlayer`)·서버 실패 저장(`saveServerFailure`)·전투 종료 판정과 안내 표시(`status`)는 세션 단계에서 연결한다. 원격 Tell/개발자 명령의 실제 소비자(Tell 표시·개발자 해석기)도 같다.
