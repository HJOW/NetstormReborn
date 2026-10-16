# PATCH Player 메뉴 전체와 실제 공통 메뉴 연결

2026-10-17, `HJOW-Athlon`. 마지막 전체 정밀 디컴파일도 **HJOW-Athlon / 2026-10-16**이다. 보호 PE/기존 정밀 소스를 읽었으며 새 Ghidra 내보내기·원본/클론 게임·GUI 창 실행은 없다. 기계어는 격리 메모리에서만 실행했다.

## 복원한 동작

`RawPlayerMenu`는 **PATCH004c9700 전체 부모 몸체**를 같은 `RawPlayerIdentity`의 현재 전역/9 Player/40 metadata와 방향별 동맹81DWORD·숨김9BYTE·사제 ownerState9DWORD에 연결한다. 공통 모드를 별도 bool 사본으로 저장하지 않는다. Identity의 판본/기록/현재 getter를 읽기 전용으로 빌리는 접근자를 추가했다.

실제 subscriberDWORD를 받아 owner가0이면0을 반환한다. 나머지는 원본 현재 모드에 따라 Player 제목·연결/사제 상태·양방향/일방향 동맹·버전/신뢰도/게임 수·정보 보기·Ignore/Whisper·사제 찾기·SP 송금·동맹 요청/해제를 작성하고 실제 controlled 스택에서 메뉴를 꺼낸다.

주소 해석 `004e0cd0`, 신뢰도 `00478310`, Ignore/Whisper 목록 `0042e390/0042f640`, 분산 SP 및 번역/문자열 CRT/GUI 생성·작성은 명시 의존 경계다. 선택 시 실제 연결을 요구하며 빈 목록/성공으로 메우지 않는다. 주소 parser의 결과는 부모에서 읽지 않지만 호출 효과를 생략하지 않았다. 게임/네트워크 명령을 실제 실행하는 소비자는 별도다.

## 기계어 관찰과 보존한 차이

`decomp_player_menu.py`가 부모 전체와 실제 Identity/사제/번호/동맹/SP 비교 helper를 **292고유 입력**으로 실행했다. x87 53/64비트 합계 **584정상 반환·정밀도 차이0·실제 명령684,032회**다. 게임/OS 호출은0이다.

- 로컬/다른/없는 subscriber, 모든32개 모드 조합, Player flags/사제 상태, 방향별 동맹, 숨김/허용, 부족 SP·NaN, metadata, 주소 문자열, Whisper BYTE 및 재진입을 포함한다.
- 번역이 metadata/이름을 바꾸거나 SP 조회가 현재 연결/허용 상태를 바꾸고 신뢰도 조회가 metamap 모드를 바꾸는 입력을 포함한다. 번역과 서식 출력을 분리해 실제 조회 순서를 유지했다. 버전/신뢰도/동맹 이름을 번역 전에 미리 읽지 않는다.
- 원본의 세 송금 경로를 동일한 표시 목록으로 축약하지 않았다. 숨긴 로비 대상의 앞4행은 include0/enabled0이고 마지막 행도 include0이다. 캠페인의 마지막 행은 별도 Tell 명령과 정적 문자열 토큰을 사용한다. SP 부족은 색을 바꾸며 연결/허용에 따른 활성화와는 별개다.
- `asksend(%d)`는 번역하지 않는다. 원본005cad98/005cae00의 두 정적 영역을 각각104바이트 관찰하고 **208바이트 전체**를 비교했다. NUL 뒤 표식도 지우지 않는다. 토큰은 호스트 포인터로 역참조하지 않으며 `TextParameter`가 같은 토큰의 최신 문자열을 반환한다. 임시80칸 고리로 수명을 바꾸지 않는다.
- 사제 상태 조회004922e0/00492340의 flags22·ownerState2/1, 번호 조회0048f550의 metamap 진단, 동맹 mask0048f790, 실제 SP 비교00425e90을 관찰에 포함했다. 이 계산은 같은 현재 원문 표를 읽는다.
- 첫 C++ 대조에서 체크 값4건이 달랐다. 디컴파일의 char를 signed로 해석했지만 실제 Whisper BYTE 인자는 **zero-extend**였다. 이를 교정했으며 기존 기대값은 바꾸지 않았다. 첫 빌드의 이름 가림 경고1건도 교정했다.

정밀 표의 CD 대응 후보는 `004c4cd0`이다. 이번 구현/관찰은 주 목표 **10.78**에 집중했고 CD 부모 몸체는 이식하지 않았다. 생성자는 CD Identity를 명시 거부한다. 이전 식별자 모듈의 두 판본1,702입력은 그대로 보존한다.

## 실제 Gump 조립 검사

`BindPlayerMenuControls`는 기존 fancy/제목/머리/체크/항목/하위 메뉴/스택을 사용한다. `BindObjectPlayerMenu`는 기존 객체 메뉴의 subscriberDWORD를 전달한다. 검사에서 임시 Player 하위 본문을 만들던 경계를 이 실제 부모 몸체로 교체했다.

공통 트리 검사는 보호10.78 TYPE/SHP/글꼴과 **사제 기본 호출→Carrier→회수/객체→Player** 사슬을 사용한다. 실제 입력으로 Cast/Type About을 선택하고 새 수명에서 Player 하위 메뉴를 펼쳐 `Find Priest!`의 **020000b8/owner1**을 확인했다. Parser/목록/번역 등의 공급값은 명시 검사 입력이며 실제 Client Session/네트워크 명령 실행이나 OS 화면 증거는 아니다.

집중 원문292·정적 토큰 갱신/필수 의존 검사와 실제 Gump 입력 검사는 통과했다. 최종 Release 빌드2는 **종료0·경고0·오류0**이다. 전체 CTest는 **내부1,472개 모두 통과·실패0·227.40초·종료0**이며 전체 내부 출력을 `extracted/goal-player-menu-20261017-ctest-output.log`에 보존했다. 시스템 기본 커서도 이번 전체 실행에서 통과했으나 이전 간헐 실패 원인을 수정한 증거로 합산하지 않는다. 로그 접두사는 `extracted/goal-player-menu-20261017-`이다.

```powershell
python tools/decomp_player_menu.py --verify
cmake --build cpppj/build --config Release --parallel 3 -- /p:CL_MPcount=3
cpppj/build/bin/Release/netstorm_tests.exe --filter PlayerMenu_
cpppj/build/bin/Release/netstorm_tests.exe --filter RawMenuControls_Carrier
ctest --test-dir cpppj/build -C Release --output-on-failure
```

새 원문/제품 계보는 `cpppj/player-menu.json`과 `player-menu-x86.tsv`에 보존했다. 기존 Identity1,702·객체1,434·Carrier628·TYPE2,352 fixture는 변경하지 않았다. 실제 바뀐 공유 소스/CMake 핀만 이전/현재 이력과 갱신한다.

## 남은 실제 플레이 연결

이제 Player 메뉴 부모는 복원됐지만 실제 Session의 같은 현재 전역/기록·SP/목록/주소 의존을 Client 메뉴와 연결해야 한다. 정적 문자열/게임 명령 소비자와 객체 GumpForm 부착도 남아 있다. 내려놓기 위치00426a70·커서445b30/445440/4458e0·일반 건설 확정/배치가 미연결이어서 **Construct는 아직 비활성**이다.

Early Missions1~6·캠페인1-1/1-2·데모3개의400프레임 검사를 사용자 자연 완주로 합산하지 않는다. 생산/다리·경제·AI 전략·GUI 미션 시작/단계·승패/저장/다음 미션과 전체 그래픽/음향 동일성 검증은 미완료다. 목표는 active이며 전체화면 DirectDraw는 이번1차 범위에서 제외한다.
