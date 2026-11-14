# webpj 채팅 5모듈 원본 관측 연결 (WB04-E474, 2026-11-15)

3.8절 이월 목록의 채팅 계열 앞 다섯 항목을 원본 10.78 독립 관측 fixture에 대조했다: `RawChatFormat`(372행)·`RawChatMessage`(524행)·`RawChatPacket`(256행)·`RawChatReceive`(238행)·`RawChatSubmit`(383행), 합계 **1,773행**. 이미 이식한 모듈을 검증하는 단위이며 모듈 코드는 고치지 않았다(주석의 오기만 정정).

## 한 일

- `tests/chat-native-fixtures.test.mjs`를 새로 만들어 cpppj `ChatFormatTests`·`ChatMessageTests`·`ChatPacketTests`·`ChatReceiveTests`·`ChatSubmitTests`의 입력 구성(앞 열만 읽음)과 출력 직렬화를 그대로 옮겼다. 각 행에서 입력 열로 manager 2712바이트(원문 미작성 꼬리 `(0xa5+i*17)&255` 등)·수신자 배열·패킷·식별자·본문 작업 공간을 구성하고, 외부 장치(`memory`·`assertion`·`copyFormat`·`writeConfig`·`illegalFormat`·`nameForSubscriber`·`formatName`·`deliver`·`send`·`log`·`localReceive`·`read`·`ignored`·`config`·`owner`·`wake`·`sound`·`append`·`show`·`allocate`·`release`·`resolve`·`translate`·`records`·`dispatch` 등)의 호출 순서를 trace로 기록하며 mode 열의 부분 쓰기/선택 중단/현재값 변경을 같은 호출 번호에 적용했다. 관측 열(상태·전체 바이트·반환·trace)은 비교에만 썼다.
- manifest에 `chat-format`·`chat-message`·`chat-packet`·`chat-receive`·`chat-submit` 5항목을 연결했고 PORT_MAP 다섯 행 메모에 연결 사실을 덧붙였다. 다섯 모듈과 시험 파일 머리의 "cpppj에도 직접 시험이 없다"는 오기를 정정했다(E465 감사가 찾은 같은 오기).

## 결과

- **다섯 fixture 모두 첫 실행에서 전체 일치**했다(manager 메모리·캡처·수신자 배열·패킷 바이트·전역 변경·호출 순서·반환). 모듈 결함은 발견되지 않았다.
- 변형 확인(표본): 원본 관측만으로 `raw-chat-format`은 80개 중 64개, `raw-chat-receive`는 141개 중 93개를 잡았고, 기존 손 계산 시험과 합치면 각각 6개/9개만 남았다(이전 단위에서 동등으로 분류한 비교 경계·미도달 오류 경로). 나머지 세 모듈은 이전 단위의 변형 확인을 유지한다.

## 남은 것

채팅 계열의 나머지 fixture(`chat-callbacks`·`chat-completion`·`chat-input-key`·`chat-input-recipients`·`chat-input-toggle`·`chat-recipients`·`chat-state-clear`·`chat-window-base`·`chat-window`·`hud-chat-geometry`·`trace-chat`·`chat-recipient-identity`)는 후속 단위에서 같은 방식으로 연결한다. cpppj `ChatPacket_ActualOwnerMessageGrowthAndWireLength`·`ChatSubmit_ActualOwnerGrowthAndRetainedManager` 등 실제 소유자(`ClientChatManagerGlobals`) 조립 시험은 해당 모듈 이식분의 별도 시험(`client-chat-manager-globals`·`client-chat-runtime`)이 맡는다.
