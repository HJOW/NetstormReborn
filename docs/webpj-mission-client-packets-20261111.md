# webpj 미션 클라이언트 패킷(RawMissionClientPackets) — WB04-E314 (2026-11-11, 4-C)

3.1절 4-C 의 **RawMissionClientPackets**(`o/`, 기준선 294줄 = `.cpp` 186 + `.h` 108, 기준선 뒤 +59줄)를 `core/raw-mission-client-packets.js` 로 옮겼다. 로그인 서버가 보낸 실제 8종 패킷(0x29 LoginBegin·0x67 Options·0x2b Player·0x2a Fort·0x28 Reply·0x81 SP·0x84 시간·0x6e Ready)의 현재 Player/State/SP/Template 수신 처리(10.78: 004b3b40·004b4900·004912d0·004b3d30·004b8940·004b3e90·004a6f30·004a7050), 송신 observer(40a540/40a590/40ae80/40b460), 이름 정제(0042b0e0)이다. **체크 목록 순서를 ClientPackets → ServerPackets 로 바꿨다**: 서버 쪽(`RawMissionServerPackets`)의 부모와 시험이 이 모듈의 `sanitizeMissionPlayerName`·`observeMissionClientPacket` 을 쓴다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawMissionClientPackets(players, participation, sp, state, hooks)`: `players` 는 Player9 1620바이트, `participation` 은 `RawPlayerParticipation`(방향별 동맹), `sp` 는 `ScrambledSpStore`, `state` 는 원본 현재 전역 묶음(속성으로 읽는 시점마다 현재값 조회·쓰는 필드는 직접 갱신), `hooks` 는 도달한 위치에서만 요구하는 큰 외부 소비자이다. `hidden` 은 `battleOptions` 의 `+0x3e` 9바이트와 같은 메모리여야 한다.
- 수신(`receive`/`receiveFixed`): 패킷 종류 BYTE 를 고정 진입값으로 바꿔 본문을 실행하며(다른 종류는 거부) 등록 분배기는 같은 `receiveFixed` 를 부른다. 본문마다 packet 필드는 쓰는 위치에서 다시 읽는다(콜백이 packet 을 바꿀 수 있음).
  - **LoginBegin**: 서버는 번호 불일치 진단, 번호 9 이상 거부, 두 owner 설정, owner 0 이면 이전 CoreData 를 캐시에 보관하고 선택을 비운 뒤 active 0, 아니면 알려진 Player 로 두고 CoreData 를 선택(메타면 캐시, 아니면 Player `+0x6c` 10바이트). 시작 seed·zone 6바이트·전투 seed(공유 자원 상자가 있으면 int32 로 그곳에)를 쓴다.
  - **Options(67h)**: `receiveStateBattleOptions` — 길이 진단(87 초과, 복귀 뒤 현재 길이 재조회) → DWORD 단위 복사 → 나머지 0..3 BYTE → (CS 가 있으면 자식) → 범위 → 비용(float) → 환불 비율 → (전투·미션 아님·저장 경로면) 재접속 UI. 87바이트 밖 복사는 앞선 쓰기를 남기고 거부.
  - **Fort**: owner 9 이상 거부. 로컬 owner 는 (미션 후처리가 아니면) 로컬 설치, 다른 owner 는 서버면 revision 만 올리고 반환, 클라이언트면 이미지 열기. 클라이언트는 미션 후처리이거나 숨김이 아니면 이미지(`+17`, 크기 `+12`)를 읽어 배치. revision 증가.
  - **Player**: 이름 복사 → (미션 후처리의 자기 번호) 자기 이름 → 정제 → 구독자 → `+0x71` → (클라이언트) 주소 복사·`*` 주소는 sender 연결 주소로·flags(기존 0x40 보존 | 패킷)·동맹 mask → 채널 추가/제거 → 방향별 동맹 → (클라이언트) 필드들 → (미션 후처리 아님·구독자 있음) CoreData 복원 → UI 가 바쁘지 않으면 저장 요청과 알림 순서.
  - **Reply(28h)**: 버전(10.0x4e 미만이고 안내 억제가 꺼져 있으면 GameVerDiff 안내·상태 12) → 남은 시간(double 비트) + 벽시각 → 현재 phase 16/10 이면 State 요청, 아니면 틱 0·다음 상태 3. **SP(81h)**: 각 반복 끝에 signed count 를 다시 읽고 음수 SP 는 전달하지 않으며 편집기에서는 194162.0, 로컬이면 갱신 요청. **시간(84h)**: 기준 시계 = (현재 clock − 틱×1000) 의 낮은 DWORD. **Ready(6eh)**: 클라이언트는 mask 를 받아 자기 0x40 응답을 별도로 보내고, boss 는 보낸 owner 에 0x40·전체 분배 요청·로그인 타이머가 0 이면 시작 응답.
- `sanitizeMissionPlayerName(subscriber, name)`: 금지 문자(0xa0 과 `% . ? = ( ) [ ] ~ " \` ; :` 공백)를 NUL 까지 당겨 지우고 연속 금지문자도 제거, 이름이 비면 `Cheat<구독자 & 0xff>`.
- `observeMissionClientPacket(type, packet, sending, assertion)`: 송신에서만 현재 필드로 선언 WORD 를 다시 쓴다 — Fort `크기 WORD + 0x11`, Player `이름 길이 + 0x3b`, Options `dataLen + 7`, SP `((count WORD + 2) & 0xffff) × 4`; 센티넬(-1) 값은 진단(복귀하면 현재 packet 을 다시 읽어 길이 기록). 나머지 4종은 무효과.
- `registerMissionClientPackets(dispatch, packets)`: 8종을 버전 1/정책 0 으로 등록하고 가변 4종(0x2a·0x2b·0x67·0x81)만 mutable 송신 observer, 나머지는 빈 observer 를 건다.

## 2. 기준선 뒤 변경 판정 (+59줄)

- **옮김(② 적용)**: 67h 공통 State 부모(`RawStateBattleOptions`, 대응표 밖 신규 파일이므로 동작을 이 모듈에 `receiveStateBattleOptions` 로 두었다), 송신 observer(`aa91f53b1`), 고정 진입값 직접 수신 `receiveFixed`(`ede61f05d`/`eeae45c69`), 일반 CS 수신 연결(`3f942573d`), 참가자 동맹 사제 관문(`46d64b2a7`).
- **옮기지 않음 ③(4-D)**: Native Login 고정 수신기를 분배기에 대여하는 등록 분기(고정 Native 수신기 묶음, E306 의 ③). Ready 의 boss 서버 경로가 부르는 `sendReady` 의 본체는 `RawMissionLogin` 이다.
- CD(10.72)는 옮기지 않았다.

## 3. 검사 (`tests/raw-mission-client-packets.test.mjs` 8건)

- **원본 독립 관측 `mission-client-packets-x86.tsv` 157행**: 8종 수신 × 서버/boss/미션 모드 × 알려진 Player × 변형 × 호출 도중 변경(wall 로 phase 바뀜·채널로 서버 모드 바뀜·SP 갱신으로 packet 바뀜·송신으로 타이머 바뀜)에서 전체 Player 1620바이트·방향별 동맹 81DWORD·현재 전역 20개(double 비트 포함)·옵션/zone/cache/TYPE 쓰기·SP 9칸·외부 효과 순서를 대조한다. **Ready boss 경로 6행은 실제 `RawMissionLogin.SendReady` 몸체가 필요해 E314 에서는 건너뛰었고(151행 재생), E319 에서 실제 로그인을 공용 시험 장면(`tests/mission-client-scene.mjs`)에 연결해 157행 전부를 재생한다.**
- 손 시험(cpppj `MissionClientPacketsTests.cpp` 이식 + fixture 밖 경계): 이름 정제, 송신 observer 네 종과 진단, 옵션 복사·순서·미연결 소비자(앞선 쓰기 보존), 필수 효과 누락·8종 등록·잘린 입력, 구성 검사, 비용 float 반올림, LoginBegin/Fort 범위·공유 seed·로컬 설치·짧은 이미지, flags 0x40 보존·CoreData 복원 조건, Reply 기본 경로, SP signed count·소유자 0, Ready boss 경로(시작 응답 바이트·타이머 관문)와 시간 응답, 분배기 sender 전달.
- 변형 확인 127개 중 **9개만 남았고 모두 동등**하다: 이름 정제의 마지막 NUL 쓰기(이미 당겨진 NUL)·SP observer 의 16비트 감김(낮은 WORD 만 씀)·Player 범위의 길이 검사(NUL 누락 검사가 먼저 거부)·주소 NUL 여분(첫 NUL 에서 복사 종료)·동맹 mask 중복 쓰기(setAlliance 가 덮음)·SP 값 부호(음수는 앞에서 걸러짐)·SP 소유자 선택(로컬이면 같은 번호)·시간 응답의 signed 변환 둘(낮은 32비트는 모듈러 산술로 같음).

## 4. 남은 경계

- 3.1절 4-C 는 11/30 이다. 다음: RawMissionServerPackets(서버 시험 187행이 이 모듈의 정제·observer 를 쓴다).
