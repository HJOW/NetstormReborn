# webpj Client 채팅 연결 계층 (WB04-E455, 2026-11-14)

기준선 `1f00a25a`의 `ClientChatRuntime.cpp/.h`(191줄) 전 함수와 `RawChatWindowContext.h`를 이식했다. 제품 모듈은 `www/core/client-chat-runtime.js`, `www/ui/raw-chat-window-context.js`다. 같은 장기 `ClientChatManagerGlobals`·`ClientChatWindowGlobals`·`ClientDotCommandGlobals`·`ClientCoreDataGlobals`와 기존 RawChat/Identity/명령 본체를 빌린다. 별도 월드·수신자·명령표·Config를 만들지 않는다.

## 범위와 변경 판정

기준선 뒤 네 커밋을 읽고 다음과 같이 판정했다(대응표 base `c0f7843a`).

| 커밋 | 판정과 구현 |
| --- | --- |
| `8efc76eb4` | **②** 서버 뉴스 키/시각과 Config 저장 경계. 신규 `SystemNewsTime.cpp/.h`(54줄)를 `system-news-time.js`로 함께 옮겼다. 뉴스 저장은 `UberHud.inc:108`의 일반 `RawChatReceive.setNewsTime`에서도 호출하므로 메타 이름만 보고 제외하지 않았다. Load 자체의 State19 진입 호출은 웹 제외다. |
| `c1c8bd0ad` | **②** 공통 Core/현재 owner 연결. 기존 `ClientCoreDataGlobals`에 rank 조회를 연결했다. 해당 커밋의 State19 소비자 자체는 **①** 웹 제외다. |
| `9f7c0604d` | **①** 실제 CS 메타 참가자 지연 조회. 로컬/일반 전투/편집/Chat 경로는 그대로 이식하며 선택하지 않은 CS를 선행 요구하지 않는다. 로컬 일치 후의 메타 검색은 웹 제외 오류로 남긴다. |
| `c0f7843ae` | **①** 메타 로비 전용 Battle 접두/채널8 갱신. 실제 호출은 `UberMeta.inc:44`의 State19, `ClientMetaPacketReceivers.cpp:117`의 메타 수신, `ClientRendererFrame.cpp:124`의 RefreshMetaChatMode다. 웹에서 메타 참가자/CS나 LAN을 새로 구성하지 않는다. |

`RawChatWindowContext`는 기준선 뒤 변경이 없다. 신규 `SystemNewsTime`은 기준선 984단위에 없어서 현재 기준선 대응표의 Runtime 의존 파일로 연결했다. 기존 대응표 범위 자체를 현행 C++ 전체로 바꾸지 않았다. 디컴파일 소스·보호 원본·cpppj 제품 코드는 변경하지 않았고 게임/브라우저 실행도 없었다.

## 실제 연결과 플랫폼 경계

명령 등록/manager 재초기화·두 서식/현재 signed ID·rank 현재 Core와 보호 icon TYPE·현재 Playerbase/Chat100행 Identity·대상 검사/자동완성·실패 진단·서식 Config·이름 실패 SOMEONE·say→실제 패킷·제출→공유 명령 분배/issue·clear/quit/오류 출력이 기존 본체를 호출한다. 현재 창0·manager0·Core active0 관문과 bind 시 콜백 값 캡처, 선택된 장치 미연결 시 명시 실패를 보존한다. `desc`는 C++ Runtime 자체가 description 장치를 공급하지 않으므로 같은 선택 오류다. 종료 명령의 실제 웹 정책은 UI/앱 소유자 조립 때 정하며 여기서는 요청만 기존 메뉴 장치에 전달한다.

Client 본체가 아직 미이식이므로 명시 최소 장치 계약을 사용한다. `chatManagerGlobals()`·`chatWindowGlobals()`·`coreDataGlobals()`·`dotCommandGlobals()`는 이미 이식한 실제 장기 owner, `configuration()`은 기존 Config, `gameplayGlobals()`는 player/world/energy와 `localAddress()`, `chatRecords()`는 현재4800BYTE다. menus는 `chatGeometryBackend().setInputVisible`·`invokeChatCommand`·`printChat`을 제공한다. 뉴스는 `nsInfoGlobals().resolveServerName`과 `translate`를 빌린다. 테스트 장면의 Client/UI 효과 계약은 제품용 대체 구현이 아니다.

표시 인코딩은 `encodeDisplayText(UTF16) → Uint8Array`, `decodeDisplayText(Uint8Array) → UTF16`를 명시 요구한다. 좁은 부품 내부의 입력/이름/번역은 원문 바이트 문자열이다. 검사는 기존 엄격한 `encodeOriginalText`/`decodeWindows1252`로 영어·1252를 확인하고 표현 불가 한국어는 명시 실패한다. cpppj의 사설4BYTE 한국어 부호화는 E269의 웹 제외 정책을 유지한다. 한국어 UTF16/D2Coding 실제 UI와 이 경계의 결선은 후순위6차/해당 UI 단위에 남으며 **이번 단위로 한국어 채팅 완성을 주장하지 않는다.**

서버 뉴스는 같은128BYTE 키·DWORD 시각을 쓰고 getter보다 먼저 시각을 캡처한다. 첫 NUL·`.`치환·키/로컬256BYTE 한계·NUL뒤 꼬리·signed 십진 표현식·기존 `applyScriptConfig`를 유지한다. 외부 서버에 접속하거나 파일을 강제 저장하지 않는다.

## 검증과 최초 실패

신규 집중 **28건 통과**(Runtime/문맥21·뉴스7). `ClientChatRuntimeTests.cpp`의 초기화/서식/재초기화/현재Core/icon 시험과 `ChatWindowClientTests.cpp`의 현재이름/주소/Playerbase/일반본문/송수신·`IssueCommandsClientTests.inc`의 치트/명령 효과를 창 없이 대조했다. 실제 웹 장기 owner·보호 icon 자산·RawPlayerIdentity·RawChatSubmit/Message/Packet/Relay/Receive를 공유한다.

`cpppj/tests/fixtures/chat-recipient-identity-x86.tsv` **원본10.78 48행**을 Runtime→현재Identity→ChatRecipients에 공급해 반환·전체60BYTE·외부 호출 순서가 모두 일치했다(manifest `chat-recipient-identity-runtime`). 이 기존 PE 관측을 새로 실행/수정하지 않았다. 뉴스/Runtime 자체의 별도 PE fixture는 없어 나머지는 소스 계약/독립 입력 조립이다.

최초 실행은 27건 중3건 실패했다. 뉴스 키 NUL의 검사 인덱스를10에서11로, rank 나눗셈 오류 검사를 실제 `ChatManagerDivideError`로 고쳤다. 전투 송수신 결합은 기대한 `hello`와 달리 `hell`이 됐다. 기준선과 현행 C++ `RawChatPacket::SendChatPacket`은 전투 문자열의 마지막 NUL을 message 크기에 넣지 않고, `RawChatRelay`/수신은 마지막BYTE를 NUL로 바꾼다. 이 **C++ 현재 결합 효과를 그대로 검증**했으며 웹에서만 NUL을 덧붙여 다르게 고치지 않았다. 원본 게임의 실제 전투 채팅 결합 의미는 전용 PE/실행 대조가 없어 확정하지 않는다. 새 검사 한 건으로 뉴스 설정문256BYTE 실패 때의 앞선 키 쓰기도 확인했다.

대응표 줄 수는 기준선 blob의 실제 줄(`splitlines`)을 전984단위 다시 세어 현재 상태로 재집계했다. 앞선 누적 요약의 port/partial/todo 줄 분배 오차를 바로잡았으며 대상 총129,653줄은 같다. 전체 회귀는 Node v26.13.0·v22.23.3에서 각각 **4,719건 통과, 실패0**, 대응표984행 오류0이다. Node v24는 이 PC에 없어 실행하지 못했다. 실제 Console Enter·메뉴/Gump 로그·원본 글꼴 픽셀/기하·실제 transport Poll·Client 수명과 일반 앱의 연결은 RawMenuControls/Client 본체 단위에서 마무리한다. 검사에서 명시 큐를 통해 Poll 전 미표시/무시 목록 공유를 확인한 것은 실제 transport 연결 완료가 아니다. 실제 미션 플레이는 여전히 불가다.

## 다음 시작점

HtmlWindow 계열 partial과 `ClientHtmlWindowGlobals`를 기존 실제 HTML/Text/Style 장치 위에서 마무리한 뒤 RawMenuControls 허브를 조립한다. Runtime이 남긴 Client/메뉴 효과 계약·표시codec과 기존 chat 메뉴/로그는 그 허브에서 같은 실제 owner에 연결한다.
