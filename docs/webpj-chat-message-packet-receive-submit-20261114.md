# webpj Chat 본문 조합·패킷·수신·제출 RawChatMessage/RawChatPacket/RawChatReceive/RawChatSubmit (WB04-E446, 2026-11-14)

RawMenuControls 선행 **RawChatMessage 122줄·RawChatPacket 95줄·RawChatReceive 101줄·RawChatSubmit 142줄(기준선 `1f00a25a`의 .cpp/.h 줄 수, 합계 460줄, todo→port)**을 새 모듈 [`www/ui/raw-chat-message.js`](../webpj/www/ui/raw-chat-message.js)·[`raw-chat-packet.js`](../webpj/www/ui/raw-chat-packet.js)·[`raw-chat-receive.js`](../webpj/www/ui/raw-chat-receive.js)·[`raw-chat-submit.js`](../webpj/www/ui/raw-chat-submit.js)로 이식했다. 기준선 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손 계산 바이트로 한 것이며 원본 직접 관측 수치가 아니다.

## 기준선 뒤 변경 (3.6절)

네 단위 모두 기준선 뒤 변경이 없다.

## 구현

- **`RawChatMessage`(0042d640/0042dc30/0042f1e0)**: manager(2712바이트)의 +0x298 2048바이트 출력에 `formatChatMessage`가 본문 빈 관문 → 출력 첫 BYTE 지우기 → 아이콘(+16 비트 0x8/0x40/0x80 은 D36/D37/D39 를 이어 붙이고 0x20 은 D41 을 처음부터 덮어쓰며, +12 비영이면 수신자 1명이면 D38 아니면 D40) → back 서식(+0x14) → 이름 서식 → front 서식(+0x50, 직접 대상이 아닐 때) → 본문 → `~0`~`~6` 슬롯 제거(기존 `stripChatFormatSlots`) → NUL 포함 길이로 전달 순서로 쓴다. 직접 대상(수신자 +12)이면 첫 비자기 수신자를 signed count 로 찾아 이름 조회(0 반환이면 이름 지움)하고, 빈 현재 이름은 `++Nameless++`, 작업 이름 버퍼는 256바이트 이상이어야 한다. 세 고정 서식(`%s> `·`%s `·`%s (to %s)> `)만 `writeChatNameFormat`이 쓰며 이름 안의 `%s`는 다시 해석하지 않는다(그 밖은 Error). `formatChatInput`은 선행 `!`/직접 대상/일반 서식을 고르고 `sayChatMessage`는 현재 manager 0·앞 공백을 무시하고 빈 본문이면 입력줄을 숨기며 항상 0 을 반환한다. 모든 물리 접근은 `hooks.memory(token)`의 같은 Uint8Array 를 다시 빌려 범위 밖이면 RangeError 다.
- **`RawChatPacket`(0042d280/0040a8d0)**: `sendChatPacket`은 현재 이름을 고정하고 자기 subscriber 가 수신자에 없으면 추가를 요청한 뒤(추가 뒤 count 를 다시 읽음), 전투 모드면 flags(+16, 직접 대상이면 |0x100)를 세 자리 이상 십진 앞 0 패딩으로 본문(첫 NUL 앞) 앞에 붙이고, 15바이트 헤더(종류 3·+1 전역 BYTE·+2 길이 WORD=max(전역&0xffff,4)·+5 1·+6 subscriber·+10 이름 크기·+12 본문 크기·+14 수신자 수)와 이름·본문·수신자 배열을 0xff 로 채운 패킷에 쓴 뒤 채널을 고른다(전역 594fb8 비영→1, 메타맵→11, 전투→9, 모두 0 이면 오류 로그·영 주소·지역 수신). 이름 크기 0x7fff/본문 0xffff/수신자 배열 크기를 넘으면 RangeError 다. `observeChatPacket`은 송신이 비영일 때만 길이를 합산해 +2 WORD 에 쓰며 이름 0xffff/수신자 0xff 는 진단 0x81d/0x81f 를 알리고 계속한다.
- **`RawChatReceive`(0042ed30/0042f0e0)**: `receiveChatPacket`은 버전>3·비ASCII 를 로그하고 끝내며, 본문 마지막 BYTE 에 NUL 을 쓰고(무시 뒤에도 유지) 시스템 뉴스(상위 비트+발신자 0, 이름 길이>1이고 첫 BYTE 1 이면 시각 비교/갱신 후 뉴스 400,30)와 무시 목록을 처리한 뒤 비전투는 알림/소리/본문 추가/표시, 전투는 열 개 옵션을 원문 순서로 모두 읽고 채널 마스크·관계표(`0x595200+(owner+현재*9)*4`를 두 번 읽음)·중립표 필터와 앞 세 글자 제거+개행을 적용한다. `dispatchChatPacket`은 현재 Chat 이 0 이면 다른 장치를 건드리지 않는다.
- **`RawChatSubmit`(0042da00/0042d1b0)**: `addChatRecipient`는 signed count 로 중복을 찾고 capacity≤count 이면 capacity×4+128 새 할당·capacity 칸 전체 복사·이전 배열 반납·capacity+32 로 확장한다(할당 실패는 진단 0x112 후 계속). `submitChatInput`은 수신자 count/직접 표시를 0 으로 쓰고 입력을 `:` 로 나눠 본문(앞 공백 제거)/접두(인접 2048 캡처가 본문 상한 NUL 을 덮어쓰는 원문 효과 보존)로 쓰고, 접두의 각 이름(40바이트 복사·전체 길이 전진)을 식별자 resolve → 실패하면 여덟 채널 키(빈 키 포함) 번역 비교로 해석한다. 미식별은 수신자 0·마스크 0x20·입력 전체를 본문으로 되돌린다. 마스크가 비영이면 Chat 기록 100행의 플래그가 겹치는 subscriber 를 추가하고 마지막에 빈 본문은 입력줄 숨김, 아니면 분배한다.
- 웹 적응 표기: C++ `out_of_range`/`length_error`→`RangeError`, `logic_error`/`runtime_error`→`Error`, `std::span<uint8_t>`→`Uint8Array`, `std::string_view`→바이트 문자열, 현재 전역 참조→읽는 시점의 속성(getter 가능).

## 검사와 발견

새 시험 파일 4개, **47건 통과**(`raw-chat-message.test.mjs` 11건·`raw-chat-packet.test.mjs` 10건·`raw-chat-receive.test.mjs` 11건·`raw-chat-submit.test.mjs` 15건). 손 계산 바이트(15바이트 헤더·전투 접두 `005`/`261`/`0-1`·수신자 배열), 각 채널 선택·한도·진단, 전투 필터의 마스크 비트/관계표/중립표, 제출의 수신자 확장·접두 캡처·채널 마스크·100행 기록을 값으로 확인한다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류):** 전달 길이를 NUL 제외로 센 것(NUL 포함이 맞다), 본문 길이가 정확히 맞는 4바이트 서식 버퍼 경계, 전투 모드의 본문이 NUL 없이 들어오면 원문처럼 RangeError 라는 점, 비ASCII 판정이 하위 4비트(`0x11`은 정상)라는 점, 본문 NUL 이 본문 크기 칸의 마지막 BYTE 라는 점, 시각 검사에는 +19 부터 네 BYTE 가 필요하다는 점, 접두 캡처가 본문 2048 상한 NUL 을 덮어써 본문이 2049바이트가 되는 원문 효과를 시험 기대값에 반영했다.

**변형 확인:** 연산자/상수 변형 565개와 의미 변형 23개, 합계 588개 중 541개를 잡았다. 처음 생존 73개(message 26·packet 17·receive 30)는 큰 토큰/플래그의 네 바이트, count 0/1/경계, 시작 위치, 아이콘 비트 이웃값, 이름 첫 NUL, 한 글자 이름, 전투 직접 대상 비트(0x100 정확 값), 길이 WORD 최소 4, 실제 패킷의 +15/+14 인접 BYTE, 채널 마스크 이웃 값과 관계표 재조회, 이름 길이 2 의 시각 검사, 식별자 지역 초기화 필드(+0/+4/+48), 채널 비교의 ASCII 접기 경계(`Z`/`z`/`@`/`[`)와 인덱스 0 일치, 확장 복사의 capacity 전체 복사 등을 시험해 잡았다. 남은 47개는 동등 변형이다: 읽기/쓰기 범위 검사의 `>`↔`>=`(같은 위치에서 다른 조건이 먼저 거부)·도달 불가능한 범위 검사·`undefined` 읽기가 0 이 되는 범위 끝 루프(`<=`)·바로 뒤에서 덮어쓰이는 값(+4 의 기본 폭, 이름 첫 BYTE, 작업 공간 첫 BYTE 등)·16비트로 잘리는 폭 확대(이름/본문 크기 읽기 3바이트)·`-1`↔`-2` 반환·NUL 이 없는 값만 들어오는 방어 자르기.

## 남은 경계

- Chat 입력이 이 부품들을 부르는 연결(ChatWindow 본체)·실제 전송 채널(멀티플레이)·무시 목록 본체·본문 추가(42eca0)·분배 본체 0043f6f0 연결(점 명령 분배는 E443 에서 이식)은 별도 단위다. 앱은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
