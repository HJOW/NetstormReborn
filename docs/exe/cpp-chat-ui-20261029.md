# 실제 채팅 대상 검사·현재 입력 연결 — 단위572

원본 대상 검사기를 **실제 Client/Console/ChatWindow**에 연결하고 객체 메뉴의 별도 빈 입력을 실제 편집 버퍼 조회로 교체했다. 원문720입력 재관측 차이0, 최종 고유집중80개/82실행/실패0·등록3440, Release 검사/게임 빌드 경고·오류0이다. 같은 제품의4:3 창/16:9 호환 전체화면에서 Chat HUD→자기 사제 우클릭/Player 하위 생성→Leave/Main/Quit 두 실행을 확인했다. [원문](../../cpppj/chat-input-recipients-native-20261029.json)·[집계](../../cpppj/chat-ui-runtime-20261029.json)를 따른다.

## 실제 연결과 수명

- `BindClientChatRecipients`가 현재 manager·번역·Identity를 같은 ChatWindow와 Console Draw에 공급한다. Identity마다 현재 이름 getter를 다시 읽고 `ClientCoreDataGlobals::Players()`의 현재 base를 빌린다. Session 교체 전의 이름/Player span을 캡처하지 않는다. 실제 Uber는 기존 맵 목록과 같은 `world.Session.CurrentHandle()`/장기 `mapUserName_`를 사용한다.
- `005953fc`는 보호 PE의 최초BSS0인 실제 Client 주소DWORD로 장기 소유하고 객체 메뉴의 private0을 제거했다. 현재 주소를 PlayerFinder와 메뉴가 함께 읽는다. 실제 네트워크 주소 writer/메타맵 기록은 후순위 MP 경계이며 새 통신 기능이나 성공값을 만들지 않았다.
- 이름 입력 창이 Chat보다 먼저 공통 TextEntry를 생성해도 기존 처리기에 대상 검사만 추가한다. 버퍼/시계/clipboard를 다시 만들거나 이전 문맥의 다른 장치를 지우지 않는다. 같은 UI 소유자의 재생성은 기존 getter를 계속 사용한다.
- `ChatWindow::ContainsRecipient`가 `0042e9e0`의 CopyText128→콜론 앞 유효 대상 검사→대소문자 구분 `strstr`를 수행한다. 유효하지 않거나 콜론이 없는 입력은 빈 접두다. 빈 대상 이름은 빈 접두에서도 참이며 이름 경계/대소문자 무시 검색을 추가하지 않는다. NULL 이름의 원문 미정 동작은 지원하지 않는다.
- 객체 메뉴의 `0042f640` 경로는 현재 Identity를 먼저 조회하고 성공한 이름만 현재 Chat 포인터/편집 버퍼에서 찾는다. 이전 `chatInput` 빈 문자열과 콜론 미연결 예외를 제거했다. 조회 실패 진단은 같은 현재 trace 장치로 전달한다. 송신·자동완성은 여전히 선택 시 명시 경계다.

## 원문 관측과 C++ 검사

[도구](../../tools/decomp_chat_input_recipients.py)는 제품 작성 전에720행을 고정했다. `0042e9e0/0042f640`, 실제 채팅 검사 부모·Regexp 생성/검색/캡처·`004841c0`을 함께 실행한다. 현재 Console의 CopyText128 결과, malloc/CRT, 영어 번역만 명시 장치다. 로컬/일반 전투/편집3모드×manager0/1×12입력×직접 대상6/Subscriber4를 교차했다. 빈/미지/부분이름·대소문자·40BYTE·복수 토큰·중간NUL을 포함한다. 원시EAX와 실제 소비되는 하위BYTE를 구별하고 재관측 차이는0이다. GUI/송수신을 실행한 관측은 아니다.

신규 실제 UI 검사는 같은720행을 Client의 현재 Player/Chat 저장소·공통 ChatWindow·실제 TextEntry 버퍼에 대조한다. 반환과 원본 입력 보존을 비교하며, 원문 관측의 외부 호출 trace 전체를 UI에 새 계측한 것은 아니다. 별도 신규 검사는 늦은 TextEntry 바인딩·현재 이름/주소 변경·Player 재할당 뒤 새 base 사용·실제 Console Draw를 확인한다.40BYTE Chat 행은 원문 경계와 같은 명시 검사 입력이며 기본 채널 Add 정책을 바꾸지 않는다.

| 최종 필터 | 사례 / 실패 |
| --- | --- |
| ChatWindow / ChatRecipients / ClientChatRuntime | 11 / 0, 3 / 0, 6 / 0 |
| PlayerIdentity / PlayerMenu / ObjectMenu | 4 / 0, 3 / 0, 20 / 0 |
| TextEntry / SubscriberGlobals / CampaignSharedGameplay | 9 / 0, 10 / 0, 2 / 0 |
| HudChatGeometry / HudDock / ClientStateWorldGlobals / ClientPlayerRecords | 1 / 0, 1 / 0, 8 / 0, 4 / 0 |

중복2개를 제외한 고유80개/최종82실행이다. 선행 `ChatWindowClient`10개 성공은 별도로 보존했다. 최초 묶음의 `ClientGameplayGlobals`는 실제 등록명과 다른 필터라0사례/1CHECK·종료1이었다. 이를 제품 실패/통과로 합산하지 않고 원시 로그를 보존한 뒤 실제 `CampaignSharedGameplay`·`ClientStateWorldGlobals`를 선택했다. 검사 기대값이나 프레임워크를 약화하지 않았다.

## 실제 두 화면비

[GUI 도구](../../tools/cpp_chat_ui_smoke.py)는 기존UI 사본을 새 디렉터리에 복사하고 `.english`의 Go 앞에 원문 ToggleBottomGump2 버튼 한 줄만 추가했다. Fort와 나머지 시나리오는 source와 같으며500000SP/기술 설정은 이 검사 사본의 입력이다. 기본 게임 규칙이나 자연 완주 근거로 세지 않는다.

| 장면 | 실행 결과 |
| --- | --- |
| 1024×768 창 | 17.369초/종료0·Chat 하단96·자기 사제와 Player 하위 생성·메인 복귀/Quit |
| 1280×720 호환 전체화면 | 17.671초/종료0·같은 경로와 하단96·메인 복귀/Quit |

두 실행의 원본/사본 설정 복원 뒤 해시는 같다. 사제의 실제 메뉴와 Player 하위 제목을 보고서에서 확인했다. `RawObjectMenu→RawPlayerMenu`의 자기 Subscriber 가지가 현재 Chat 조회를 호출하는 코드와 함께 판정하며 별도 함수 호출 카운터를 GUI에 계측한 것은 아니다. 두 캡처에서 메뉴/Chat 하단을 직접 검토했지만 원본 픽셀 비교는 하지 않았다.

**GUI 입력은 원문 기본 빈 접두다. 직접 게임 창에서 타이핑한720사례로 주장하지 않는다.** 여러 접두의 동작은 창 없는 실제 UI 검사다. 입력줄 표시 요청 `0054db18→프레임 말0042f210`과 실제 키 연결은 미복원이며 다음 작업이다. 일반 싱글플레이의 원래 숨김 조건을 변경하지 않았다.

## 인계

게임 SHA는 `ac8c9e12cc3f1e9cdf71fde6daa1a42ec38b1281353b9db64bc7dd8f2a325225`다. 보호2,585파일/기존fixture725의 차이0이며 현재fixture726이다. 원시는 `extracted/cpp-chat-ui-572-20261029/`다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이고 현재도 같으며 산출물 변경은 없다.

입력줄 표시/실제 타이핑, 자동완성·제출·전송과 일반 UI/Edit·시나리오·수명·호환성은 계속 남는다. 새 MP/실제 통신, 포획/희생·특정 공격·승리·자연 완주·전체C++·34경로·Windows11 실기·원본 픽셀/음향은 실행하지 않았다. 전체 목표는 진행 중이다.
