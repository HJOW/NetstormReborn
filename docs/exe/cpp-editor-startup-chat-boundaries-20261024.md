# Editor의 Chat 랭크 갱신과 startupStateCheck 실제 경계

2026-10-24 **단위384 당시 읽기 조사·후속385 제안**이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 기존 복원 코드·fixture·정밀 C와 실제10.78 PE를 대조했다. 최초 조사 단계의 새 C++/원문 실행·빌드·GUI·공유 코드/CMake/LEFT/git 변경은 없었다. 아래 미복원 표현은 그 시점의 경계이며, 이후 독립 복원 결과는 마지막 절로 구별한다. 원문 정적 근거는 `extracted/refined/originals/Netstorm.c`와 같은 `functions.tsv`다. 앞선 문서/주석의0042f540 ‘선택정보 Gump’ 표현은 아래 실제 의미로 정정한다.

## 0042f540은 Chatmanager의 랭크 그림 prefix 갱신이다

PE0042f540~0042f581(66BYTE)와 정밀C38652~38668의 순서는 다음과 같다.

1. 현재00548714가0이면 반환한다. 이것은 **Chat.cpp의 TheChatSystem manager 포인터**이며 ChatWindow root/생산 패널/현재 선택 SID가 아니다.
2. 현재00595348(coreDataActive)가0이면 반환한다. PlayerKnown00595344와 다르다.
3.0048f0d0으로 **현재00595350 CoreData 포인터**를 얻는다. metamap00594fc8이 비영이면 cachedCoreData005953f0와 같은지 진단하며, 진단 복귀 뒤 현재 포인터를 재조회한다.
4. 반환 주소+4의 unsigned BYTE 랭크를 읽고0042f480으로256BYTE 결과를 만든다.
5. formatter 복귀 뒤 **현재00548714를 다시 읽어**0042d620을 호출한다. 이 setter는 그 manager의 **+244**에 NUL 포함 순방향 복사한다. 처음 캡처한 manager를 마지막에도 사용하면 callback 변경을 놓친다.

현재 `services.fortTechnologyUi` 이름은 호환성을 위해 유지할 수 있지만 의미는 위 부모다. 실제 Technology 섹션이 없어도 `RawFortMetaLoad`의 마지막 UI callback은 실행된다. 생산 `RefreshProduction()`이나 선택 재지정으로 바꾸지 않는다. Chat0/CoreData활성0은 진짜 현재 owner를 읽은 경우에만 자연 반환이다.

### 기존 재사용과 미복원 범위

현재 전체 `cpppj/src/tests/docs` 및 제안 사본 검색에서 `ChatPrefixIcons`/`PrefixIcons` 구현은 찾지 못했다. `RawGameMenus::rankPicture`는0042f480을 위한 **외부 hook**이며 제품 바인딩은 없고 기존 `GameMenusTests`도 명시 서식 stub이다. 신규 작은 formatter를 복원하면 이 hook에도 같은 몸체를 재사용한다.

`ChatWindow`/`HudChatNode`는 다른 실제 객체다. ChatWindowState.current는00548740, 로그파일 포인터는0054874c, 입력창 접두는HudChatNode+b8의128BYTE다. 현재 private `ChatWindow::Prefix`는 그 **입력 접두**만 쓰므로 Chatmanager+244 setter를 대신할 수 없다. 기존 ChatWindow 생성/기하/입력 fixture는 유지하며 큰 자식/버퍼 생성 몸체를 중복 작성하지 않는다.

`Client::ChatState`의 기존4800BYTE/`RawChatChannels`도 정적100×48 채널표다. manager0xa98 할당과 별개이며 이 표 주소를00548714로 사용하지 않는다. 현재 `CampaignObjectMenuClient::chatInput`도 미연결 편집 입력의 별도 보조 문자열이므로 manager 랭크 prefix로 가장하지 않는다.

## manager는 공통 Client 시작에서 실제로 생성된다

PE00439840은 `init map mode` 로그→004437f0의 빈 ret 뒤에0042e050을 호출한다. 다음은00422490→0047d860 count reset이다. 정상 공통 시작 흐름이므로 단독 Campaign/Editor라고 생략하지 않는다. 현재 `Client::Run`은 이 공통 manager 초기화를 생략한 상태다. BSS의00548714 최초0과 원문 Client 시작이 끝난 뒤 비영 객체를 구별해야 한다.

실제 다른 호출 위치는0048fc51의 tail jump(재접속/현재 CoreData 갱신),004b4485,004c9670/004c96f0의 포맷 변경이다. 정밀 C에 중복으로 보이는 Client 시작 절과 별개이며 실제 PE call/jump 다섯 지점을 확인했다.

0042e050(393BYTE)은 다음을 수행한다.

- new0xa98 → 성공이면0042e000 생성자 → 결과를 **현재00548714에 대입**, 실패면 같은 슬롯0.
- 생성자0042e000(78BYTE)은 내부 new0x80을+8에 쓰고 실패 진단 뒤 원문을 계속한다. +0/+c DWORD0,+14/+50/+244 첫BYTE0,+4 DWORD0x20만 쓴다. 나머지0xa98/내부0x80을 임의0으로 채우지 않는다.
- `format`/`back`→0042ddf0,`front`→0042def0을 flag1로 등록한다.
- 각 사용 위치의 현재 subscriber00540c74를 signed `%d`로 서식화한 `<id>chatFormat`/`<id>chatFormat2`를 실제 설정에서 읽는다.
- 현재 manager가0이면 `TheChatSystem != NULL` 진단 후 원문 위치로 계속하고, 현재 manager+14에strncpy60/끝+4f=0, **다시 현재 manager**+50에strncpy500/끝+243=0을 쓴다.

따라서 향후 Client의 실제 소유자는 current manager 슬롯·각0xa98 할당·내부0x80 할당을 함께 관리하고, 위 공통 시작 시점에 실제 생성/명령등록/설정 읽기를 수행해야 한다. 빈 manager 포인터만 새로 두어 자연 생략시키거나 랭크prefix만 별도 문자열로 복제하는 것은 이 연결의 완료가 아니다. 생성자·초기화·setter·formatter·갱신 wrapper를 원문 PE에서 먼저 독립 관측한 뒤 신규 작은 모듈로 묶는 단위385가 필요하다. 전체 Chat 전송/로그/ChatWindow까지 완료했다는 뜻은 아니다.

CoreData의 현재 포인터/활성 DWORD도 Client 공유 수명이 필요하다. 현재 제품의 두 값은 `RawCampaignSession::Impl::Login`의 `currentCore`/`coreActive`에만 있고 Editor에는 Login이 없다. 새 `ClientCoreDataGlobals` 같은 **실제 포인터 역할의 span 슬롯·활성 DWORD·동일10BYTE 캐시**를 Client 한 개가 소유하고, 기존 Login/원문 패킷/Template/새 Chat wrapper가 같은 참조를 빌리게 하는 방향이 맞다. 이때 span은 실제 Player+6c 또는 같은 캐시를 빌려야 하며 임의10BYTE 복사본을 현재 포인터라고 부르지 않는다. 초기0/선택 쓰기·재접속·metamap 진단을 별도로 보존한다. getter 호출 때마다 현재 Player를 추정해 포인터 선택 효과를 생략하지 않는다.

## 004b5ba0 startupStateCheck는 별도 저장 상태 로더다

현재 제품에는 큰 외곽의 필수 `startupStateCheck` hook만 있다. 기존 `ExistingEditorInstallOuterTests`의258관측은 **4b5890을 명시 장치로 경계화한 외곽 원문**이며 제품의 실제 저장 상태 로더가 아니다. 전체 src/tests에서 실제249BYTE 부모와772BYTE reader 구현은 찾지 못했다.

PE/정밀C144495~144535에서 **GetInt00441090("startupStateCheck")**가0일 때만 즉시0을 반환한다. Editor/missionPost 관문은 없다. 비영이면004b5890을 반드시 호출하고 반환 signed 저장시각을 보존한 뒤00460d70의 매호출 **벽시각**을 읽는다. 앞선 문서의 ReadInt/게임시각 표현은 이 의미로 구별한다.

x87 명령은 savedTime을 signed DWORD로fild→현재 double 시각에서 빼기→double 상수0050c130(1/60) 곱하기→**float32 경과분 저장** 순서다. 정밀 C가 표시한 두 피연산자의 선행 float 변환을 그대로 구현하지 않는다. savedTime 비영 및 엄격한0<경과분<45일 때만 다음 UI를 실행한다. unordered/경계0·45는 생략한다.

1. Yes 번역→No 번역.
2. `$Button=%s,StateReconnect,0 $Button=%s,DelStateFile,0 `를 먼저 서식화한다.
3. 원래 reconnect 질문을 번역하고 float32 경과분을 double로 승격한 값/앞선 버튼 문자열을 `%2.1f`/`%s`에 서식화한다.
4.004cf550(text,0,0)으로 실제 임시 Tell을 열고1을 반환한다. 외곽은 반환값을 사용하지 않고 다음 현재 Editor 분기를 계속 실행한다.

기존 `ConfigInterface::GetInt`, 번역 `Client::Translate`+원문 인코딩, 실제 Tell 임시 본문/`ParseDialogPage`/80칸 문자열 고리/40칸 입력 고리는 재사용 대상이다. 이 질문을 `MessageBox`나 별도 Yes/No bool로 바꾸지 않는다. `StateReconnect`와 `DelStateFile` 명령은 현재 제품 registry/분배에 구현돼 있지 않아 해당 두 실제 선택 효과도 후속이다.

### 004b5890은 파일 존재/시각 조회 한 번으로 치환할 수 없다

state 파일 경로004b5470은 `!state.dat`를 실제 파일 부모00459c60에 전달한다. 실제 reader는 존재 여부→MemStream/파일 객체→서명0x000a004e→256BYTE Fort 이름/점 없음→pending 이름 복사→저장 owner/시각/인접 DWORD→8개 Player 레코드 순회→추가 시드/source/gameTicks/마지막 검증DWORD를 읽는다. 원문 파일 객체의 생성/읽기/해제는 기존 장치를 재사용해야 한다.

각 Player는 subscriber+2c, NUL 이름+0c/설정+30, flags+50(0x40만 제거),+58,+6c의10BYTE 기술 CoreData를 직접 쓴다. 선택 저장 owner에서는 **새 이름을 쓰기 전에 이전 Player 이름을005c84c0으로 복사**한다.0048f350이 현재 캐시/기술 획득 상태를 갱신한다. 끝 검증0이고 유효 참가자가2명 이상일 때만 저장시각을 반환하지만, 반환0이어도 앞선 pending/Player/cache/ticks 효과는 이미 남을 수 있다. 파일 없음/서명 불일치/점 있는 이름의 조기 실패와 이 늦은 실패를 구별한다.

최소 부모 제안 API는 `configInt(key)`, **실제 `readSavedState()`**, `wallSeconds()`, 현재 번역/원문 서식, `tell(text,0,0)`의 필수 hooks다. `readSavedState`에는 기본0을 넣지 않는다. 현재 실제 설정0이면 reader와 UI 없이 자연 반환할 수 있으나, 그것을 Editor 전체에서 설정을0으로 강제하는 우회로 사용하지 않는다. 활성값이 비영인데 reader가 없으면 도달 위치에서 명시 오류로 남기는 것이 현재 정확한 경계다.

실제 외곽 바인더는 같은 Client Config/현재 Session의 Player·CoreData·pending 이름·같은 State owner를 reader에 공급해야 한다. reader가 현재 pending/owner를 바꿀 수 있으므로 뒤의 Editor/lastEditFort/EditorPlayers가 모두 원래 callback 뒤 현재값을 다시 읽어야 한다. 실제 Store 사본을 reader/조회 전역이라고 포장하거나 파일에서 기대값만 읽어 상태 효과를 생략하지 않는다.

## 후속 검증 경계

단위385 Chat 독립 모듈은 원문 PE의 실제 생성/부분 쓰기·비0 초기 메모리·할당 실패·재초기화·명령등록/Config 후 현재값 재조회·rank clamp/나눗셈·현재 manager 교체·CoreData 비활성 자연반환을 먼저 고정한다. 기존 ChatWindow/채널/메뉴 fixture는 별개로 보존한다. 실제 Client.CoreData/manager 소유자 연결과 공통 Run 시점은 그 다음 통합 검사다.

startup 부모는 옵션0/비영, 저장시각0/음수, 엄격 경계와 float32 반올림, 번역/format/Tell 순서 및 reader의 부분 효과를 독립 관측해야 한다. 실제 reader 없이 부모 hook stub 검사만 통과한 것을 파일 재접속 복원 완료로 세지 않는다. 실제 Editor 마지막 loaded/UI 성공은 두 경계를 포함한 전체 외곽·State 정상 반환 뒤에만 판정한다.

## 단위385 독립 복원 후속

위 최초 조사 뒤 Chatmanager의5몸체를 제품 작성 전에 실제 PE로 **322입력×2=644관측** 고정했다. 정상524·물리/CPU fault94·명시 중단26과 전체220442명령이며, 이후 신규 `RawChatManagerPrefix`와 신규3검사를 공용 라이브러리 없이 격리 빌드해 **3검사/실패0**을 확인했다. [독립 복원과 남은 Client 접합](cpp-chat-manager-prefix-20261024.md)에 최초 실패·교정과 자료 경계를 기록했다. 이는 실제 공통 Client.Run 초기화/현재 CoreData 소유자 연결의 완료가 아니다.

root가 별도 후속으로 `RawStartupStateCheck` 부모와 도구 `tools/decomp_startup_state_check.py`를 복원했다. root 인계 기준 **195입력×2=390관측·12430명령·정밀도 차0**, 격리2검사/실패0·두번째 빌드 경고오류0이다. 여기서 그 실행을 재수행하지 않았다. 위 ‘249BYTE 부모 미복원’은384 당시 이력으로 유지하며 현재385 독립 부모는 준비된 상태다. **004b5890의772BYTE reader는 계속 필수 외부 hook**이고, 실제 파일 재접속이나 전체 Editor 외곽/loaded 성공으로 확대하지 않는다.
