# 단위67 실제 Client 수명의 Chat/현재 AI 기본이름 연결

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**로 같다. 작업 전 AGENTS와 양 LEFT의 최신 기록을 읽었다. 이 단위의 기존 소스 기준은 **81fae7c(단위65)**이며 원본/클론 게임, GUI, 새 디컴파일은 실행하지 않았다. 보호 원본·AGENTS·dotnetpj를 변경하지 않았다.

## 실제 수명과 현재 subscriber

`Client::ChatState`가 구현 파일의 pimpl 안에 원본 `00558ed0`에 해당하는 **100×0x30=4,800바이트**를 한 번0초기화하여 소유한다. PATCH는 같은 저장소를 빌리는 `RawChatChannels` 하나를 함께 만든다. 미션별 Session/AI 서비스는 이 기록을 복제하거나 다시 초기화하지 않는다. Client가 메뉴/월드보다 오래 소유하므로 Session 해제 중의 Chat 호출에도 기록 저장소는 살아 있다. CD Client 자체의 생성은 유지하되 미확정 CD Chat 몸체를 PATCH로 대신하지 않고 `ChatChannels()` 호출을 명시 거부한다.

자기 subscriber는 `RawChatChannels::Add` 첫 비교마다 다음 실제 경로로 읽는다.

`Client::CurrentSubscriber()` → 현재 `UberGump::CurrentSubscriber()` → 현재 `GameWorld::CurrentSubscriber()` → 같은 `RawCampaignSession::CurrentSubscriber()` → **`impl_->globals.mySubscriber`**

subscriber 사본·고정1·MissionPlayers 로컬 번호를 추가하지 않았다. 실제 UI의 GameWorld 생성자 안에서 `Session::Load()`를 실행하는 동안에는 아직 UberGump의 `world_`에 설치되지 않았으므로 getter는0이다. 메뉴/월드 부재도0이며, `ClearWorld`는 파괴 중인 월드를 getter가 따라가지 않도록 Kernel 제거 전에 설치 포인터를 비운다. 현재 아직 로그인 경로가 연결되지 않아 Session의 실제 `loginSubscriber`/`mySubscriber`도 기본0이다. 이 단위가 로컬 로그인 성공을 합성하지 않는다.

`Client::ChatRecords()`는 동일 물리 저장소의 읽기 전용 span이다. Chat의 이름·flags·subscriber 쓰기는 실제 `RawChatChannels`만 수행한다.

## AI가 실제 선택하는 서비스

장치를 요구하지 않는 `BindCampaignAiClientServices(Client&,CampaignAiServices&)`가 다음3개만 연결한다. 실제 `MakeCampaignClientServices`도 반환 전에 동일 binder를 호출하므로 검사 전용 분기와 별도 제품 callback이 생기지 않는다.

| 경계 | 실제 대상 |
| --- | --- |
| `participation.clearChat(channel)` | 같은 Client의 `ChatChannels().Clear(channel)` |
| `participation.addChat(sub,name,channel)` | 같은 Client의 `AddToChannel(sub,name,channel)` |
| `defaultName()` | 현재 `Configuration().PathSpec("defaultAiName")` → `EncodeOriginalText` → 원본 좁은 문자열 |

AI 호출의8인자 중 실사용1/2/6인자는 기존 Chat 모듈이 원본 필드에 적용한다. 자기 subscriber의 Add는 그 모듈의 자연 `-1` 반환을 그대로 사용한다. 원본 이름 정제 `0042b0e0`과 실제 빈 `004437f0`의 두 진입도 같은 모듈에서 실행한다. observer는 실제 Client에 설치하지 않는다.

기본이름은 `Translate`/Xlat으로 조회하지 않는다. 원본 `004410f0("defaultAiName",0,0,0)`에 해당하는 현재 설정 조회·치환을 매 요청 다시 수행한다. 실제 보호자료 경로는 `setup.cfg`의 `languageSpec={DataDir}\\config.{local.1}` → `LoadLanguage(currentLanguage)`이며 `config.english`의 `Evil Duke`, `config.german`의 `Fürst des Bösen`가 원본 CP1252로 전달되어야 한다.

실제 AI 할당 직후 바이트, nonzero Color, localTransport, 연결 번호 재배치/거부/해제, 동맹 송신은 이 binder가 채우지 않는다. 기존 Session의 실제 brain 소멸/반납 연결도 유지한다. 자동 AI/Stage3/승패 및 미션 완주를 활성화하지 않았다.

## 검사와 현재 검증 경계

기존 Chat native **2,448개**는 [몸체 문서](cpp-chat-channels-20261015.md)와 `cpppj/chat-channels.json`의 독립 관찰이며 새 고유입력으로 다시 합산하지 않는다. fixture SHA는 **2e8bbcdcf0ff3d053cc1e721991cd8e1aa1a97ea6e9e28c4eceb3fdf99d9bb6d**로 유지한다. 검사 소스 변경 뒤 보고서의 현재 SHA만 갱신하고 `--verify`로 다시 감사한다.

`ChatChannelsTests.cpp`에는 기존3개 외에 실제 Client를 `Run()` 없이 만드는 검사2개를 추가했다. 이 두 검사는 창/화면/음향 장치를 만들거나 설정을 저장하지 않는다.

1. 메뉴/월드 부재의 현재 subscriber0, self0의 자연 무쓰기, 실제 이름 정제, 여러 AI 서비스가 같은 Client 기록/주소에 쓰는 동작, BYTE 채널 OR/Clear와 미복원 경계의 미충전을 검사한다.
2. 현재 언어 Config의 기본이름 치환·독일어 CP1252의 실제 바이트·언어 교체·상위 전역 설정 변경을 같은 callback으로 다시 읽는지 검사한다.

제품과 검사를 CMake에 등록한 뒤 Release 빌드 경고0·오류0, Chat5개·전송3개·실제 Session15개·Client 정지/프레임5개·참여2개·요구에너지3개의 **집중33개 실패0**을 확인했다. 전체 CTest **1/1·내부1,264개 실패0·210.62초**도 통과했다. 로그는 `extracted/goal-chat-transport-final-{build,focused}.log`, `goal-chat-transport-{ctest,tests}.log`다. 게임/GUI 실행은0이다.

첫 집중 실행에서는 headless 검사에서 Startup의 전역 Config 버퍼 설치를 생략해 기본이름의2개 CHECK가 실패했다. 원본 Get은 자기 버퍼가 없으면 등록부 조회 전에 반환하므로 검사에 실제 전역 Config 버퍼를 먼저 설치했다. 제품 PathSpec을 바꾸거나 기대 바이트를 약화하지 않았다. 처음 잘못 지정한 `ClientMain_` 필터도 일치 검사0을 실패로 보고했으며 최종 실행에서는 실제 `ClientPause_`/`FrameInterval_` 필터를 사용했다. 초기 로그 `extracted/goal-chat-transport-focused.log`를 보존한다.

현재 소스 SHA는 `cpppj/client-ai-chat.json`에 별도 기록한다. `session-ai.json` 및 `session-construction.json`은 각 완료 커밋 당시 역사 스냅샷이며 이 단위의 현재 SHA로 덮어쓰지 않는다. 초기2,448입력 fixture와 TYPE2,352입력은 변하지 않았으며 SHA 감사를 통과했다.
