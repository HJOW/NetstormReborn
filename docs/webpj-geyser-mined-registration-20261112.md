# webpj 간헐천 채집 통지 0x5b 분배기 등록과 실제 연결(RawGeyserMined 완료) — WB04-E323 (2026-11-12, 4-C)

3.1절 4-C 의 **RawGeyserMined**(`o/`, partial, 기준선 142줄 = `.cpp` 81 + `.h` 61)를 마쳤다. 처리기 본체(004266c0: 소리 난수·수량 WORD cost 차감·bridge assert·고갈 삭제·emptyGeyser 생성/Pop)는 WB04-E108 에서 옮겼고([E108 문서](webpj-contained-geyser-mined-20261108.md)), 이번 단위는 그때 `RawPacketDispatch` 이식 전이라 남겼던 **패킷 분배기 등록 `RegisterGeyserMined`** 와 cpppj 의 나머지 세 시험(실제 분배기·factory·공통 삭제·Pop 연결, 소리 뒤 현재 boss 재확인, 소리 뒤 현재 nugget 번호)을 옮겼다. PORT_MAP 상태를 partial → port 로 올렸다. 게임·브라우저 실행과 디컴파일은 하지 않았고, cpppj 단위 시험 실행 파일(`cpppj/build/bin/Release/netstorm_tests.exe --filter rawGeyserMined`, 게임 실행 아님)만 돌렸다. 이 모듈은 DOM 을 쓰지 않는다.

## 1. 규칙

- `registerGeyserMined(dispatch, handler)`(`core/raw-geyser-mined.js`): 원본 두 판본 초기화처럼 0x5b 에 대해 ① observer 를 비운다(`observe(0x5b, null)` — 원본 observer 는 효과 없는 빈 함수라 cpppj 도 빈 `std::function` 으로 등록하며, 단일 분배는 observer 를 부르지 않는다. 방향 전용 등록이라 같은 슬롯의 패킷 관찰기도 지운다) ② 버전 1/불일치 정책 0(버전이 달라도 계속 분배) ③ 일반 수신기를 등록한다(직접 수신기는 지운다). 수신기는 sender/연결 인자를 읽지 않고 `handler.handlePacket(packet)`(packet+4 WORD SID 의 현재 raw 슬롯)만 부른다. 길이는 등록 표 기본값 6(`raw-packet-lengths.js` PATCH 표)을 그대로 쓴다.
- 웹 전용 계약: `handler` 가 `RawGeyserMined` 가 아니면 등록 표를 바꾸기 전에 `TypeError` 로 거부한다(C++ 는 참조 타입으로 막는다).
- 상수 `GEYSER_MINED_MESSAGE = 0x5b` 를 내보낸다(Session 조립에서 쓸 수 있게).

## 2. 기준선 뒤 변경 판정

- `git diff --numstat 1f00a25a HEAD -- cpppj/src/o/RawGeyserMined.cpp cpppj/src/o/RawGeyserMined.h` 는 비어 있다(3.6절 ①: 변경 없음). cpppj 시험 `GeyserMinedTests.cpp` 도 기준선 뒤 변경이 없다(마지막 변경 `0d82e448d`, 2026-11-05). PORT_MAP base 는 `1f00a25a` 그대로 둔다.

## 3. 검사 (`tests/geyser-mined.test.mjs` 5건, 이번에 3건 추가)

- 기존 2건(E108): 원본 관측 `geysermined` fixture 의 10.78 1156행 전부 재생, 구성 거부·소리 뒤 현재 boss 로 고갈 삭제.
- **실제 연결**(cpppj `rawGeyserMined_real_packet_factory_destroy_and_space` 이식, 10.78 만): 같은 풀의 `SquidFactory` 로 간헐천(122, flags 2 → client 영역)을 만들고 `SquidPop` 으로 (40.75, 41.25)에 등록한 뒤 `RawPacketDispatch` 에 `registerGeyserMined` 로 등록하고 0x5b 패킷을 분배한다. 확인: 등록 표(길이 6·버전 1·정책 0·observer/패킷 관찰기/직접 수신기 없음·수신기 있음 — 등록 전에 패킷 관찰기와 직접 수신기를 미리 두어 등록이 지우는지 본다), 분배기의 시각 723·기록 1회·소리 콜백 안의 연결 깊이 1·분배 뒤 깊이 0, 고갈 삭제가 `RawSquidDestroy` 공통 몸체(pre/post 만 완료, 다른 파생 효과는 이 장면의 미실행 경계라 오류)로 간헐천을 free 로 만들고, `SquidFactory` 가 서버 영역(`>= serverFirst`)에 emptyGeyser(84)를 만들어 `SquidPop` 이 같은 좌표 비트로 해시 1단계 머리에 등록하는 것. 짧은 패킷·효과 없는 구성 거부, 처리기가 아닌 값의 등록 거부(표 미변경)도 본다.
- **소리 뒤 현재 boss**(cpppj `rawGeyserMined_shared_boss_rechecked_after_sound` 이식): boss 를 getter 로 두고 소리 콜백이 끄면 수량만 0 이 되고 삭제/생성/배치가 없으며, 처음 값이 거짓이어도 소리 콜백이 켜면 고갈 흐름이 일어난다.
- **소리 뒤 현재 nugget 번호**(cpppj `NuggetTypeNumber_MiningReadsOwnerChangedBySound` 이식): 소리 콜백이 바꾼 번호의 단가(20.75 → 79, 30.25 → 69)로 줄이고, 번호가 표 밖(188)이면 소리·난수 전진 뒤 수량 기록 전에 실패해 수량 100 이 남는다.
- **cpppj 시험 결함(웹 시험에서 보정):** cpppj `rawGeyserMined_real_packet_factory_destroy_and_space` 는 cpppj 실행 파일(2026-11-08 빌드)에서 `현재Squid Pop의실제전송미연결` CHECK 로 **실패한다**(나머지 두 cpppj 시험은 통과). 서버 영역 객체의 Pop 은 첫 활성화에서 송신 writer 를 요구하는데(`bd36f6e64` 2026-11-04 `20261104-27 CP Activate와 원문 상태 패킷 writer 및 공통 버퍼 연결`, 기준선 이전), 이 시험은 `SquidPop::BindTransmission` 을 묶지 않는다. 기준선 판 `SquidPop.cpp` 와 웹 `squid-pop.js` 모두 같은 검사를 가지므로 모듈 차이가 아니라 시험 장면의 누락이다. 웹 시험은 surface-lifecycle 시험처럼 기록용 전송 장치를 묶고, client 영역 간헐천 Pop 은 전송하지 않으며 서버 영역 emptyGeyser Pop 만 `[born, 9]` 한 번 요청함을 단언한다(flags 9 는 Pop flags 0 의 첫 활성화가 `SquidPop` 에서 낸 송신 flags). cpppj 파일은 이 단위에서 고치지 않았고, 같은 날 사용자 요청으로 cpppj 단위 1304(`2f5333171 / 20261112-95 CP`)가 같은 방식(기록 장치 연결, 10.78 은 `[born, 9]`·10.72 는 송신 없음)으로 교정했다([1304 문서](exe/cpp-test-local-transport-accept-20261112.md)).
- 첫 실행 결과: 새 3건 중 실제 연결 시험이 위 전송 미연결로 실패했고, 전송 장치를 묶은 뒤 통과했다.

## 4. 변형 확인과 남은 경계

- 등록 함수 변형 11개(타입 0x5c·버전 0·정책 1/4·observer 빈 함수/생략·버전 등록 생략·수신 몸체 비움/`handle(sender)`·처리기 종류 검사 생략·`registerDirect` 로 등록) 중 첫 실행에서 3개가 남았다(시험이 모듈 상수를 그대로 써서 타입 변경이 안 보임·빈 표라 observer 생략이 안 보임·직접 수신기 칸 미확인). 시험을 원본 번호 0x5b 직접 사용·등록 전 관찰기/직접 수신기 선배치·`directReceive` 확인으로 보강한 뒤 **0개 남음**. 처리기 본체 변형은 E108 에서 확인했다.
- 실제 Session 연결(분배기 소유자에 등록하는 시점, 소리 효과를 실제 소리 시스템에, 삭제를 가상 삭제 분배기에, 생성/Pop 을 월드 Factory/Pop 에 잇는 것)은 4-D Session 조립에서 한다. 현재 `RawGeyserMined` 의 앱 수명 소비자는 없다(PORT_MAP session 0).
- 3.1절 4-C 는 20/30 이다. 다음: `zacket/` LocalMissionTransport(단독 미션의 Zone/로그인 바이트와 로컬 두 연결을 같은 패킷 분배기에 연결).
