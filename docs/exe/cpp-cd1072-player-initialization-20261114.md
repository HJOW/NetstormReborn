# CD 10.72 Player 초기화 독립 API (제품 1309, 2026-11-14)

마지막 디컴파일 PC: **HJOW-Athlon**. 기존 양판본 디컴파일과 CD 실행 파일의 정적 x86 명령을 대조했다. 재디컴파일·원문 변경·게임/통신 실행은 없다. 보호 원본 `originalCD/NETSTORM.EXE` SHA-256: `613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233`.

## 원문 범위와 고정 값

| 역할 | CD 원문 | 실제 크기/의미 |
| --- | --- | --- |
| 초기화 부모 | `00406200~004062a4` | 165바이트, PATCH `004910a0` 대응 |
| 한 행 생성 | `004062b0~004062d6` | 39바이트 |
| 한 행 factory 반납 | `004062e0~004062eb` | 12바이트 |
| 방향별 동맹 | `00406130~004061f7` | 200바이트, 본 패치는 실제 호출 경계만 공급받음 |
| Player 현재 주소 | `0050f834` | 순수 현재 base getter/setter |
| 초기 카운터 | `0050f6d8/0050f6dc` | Fort 수/마지막 로그인 Player, 모두0 |
| 기록 수·간격·전체 크기 | 9·`0xac`·`0x60c` | 172바이트×9=1548바이트 |
| 배열 할당/반납 | `0048ef10/0048ef20` | malloc(1548)/free |
| factory 할당/반납 | `004f1650/004f1a50` | new(120)/delete, 포인터가NULL이어도 반납 호출 |

## 실제 순서와 PATCH와의 차이

1. 부모 `00406201`이 현재 Player base를 읽어 EAX에 캡처한 뒤 두 카운터를0으로 쓴다.
2. 캡처한 base가 비영이면 현재 EAX+행 위치를 `004062e0`에 전달한다. 자식은 행+0의 factory 포인터를 읽어 `004f1a50`에 전달한다. **각 반납 복귀 뒤** `00406228`에서 현재 base를 다시 읽는다. 마지막 반납 뒤의 조회값도 보존해 배열 free의 인자로 쓴다. 이후 현재 base에NULL을 쓴다.
3. malloc(1548) 반환 주소를 현재 base에 기록하고 같은 캡처 주소를 REP STOSD `0x183`(387) DWORD로0 초기화한다. NULL 반환이면 현재 base에NULL을 쓴 뒤 최초 실제 메모리 쓰기에서 실패한다.
4. 각 행의 주소는 new 직전에 현재 base+owner×172로 캡처한다. `004062b0`의 순서는 **new(120) → +0=factory → +4=30 → +0x70=127 → +8=0 → +0x2c=0**이다. 할당된 factory 메모리 자체는 초기화하지 않는다. new의NULL 반환도 같은 필드 초기화를 계속한다.
5. 자기 동맹 mask=1<<owner를 만들고 **new 뒤의 현재 base**+행+0x54에 쓴다. 원문 EDX의 같은 mask를 바로 `00406130(owner,mask)`에 전달한다. +0x54를 다시 읽지 않는다.

PATCH의 추가0쓰기 +0x64/+0x58, region +0x78,180바이트 간격, 동맹 인자 재독출을 CD에 복사하지 않는다. 기존 `InitializePlayerRecords(globals,hooks)`의 코드/서명은 보존하고, 독립 `InitializeCdPlayerRecords(globals,hooks)`와 `kCdInitializedPlayerStride=172`·`kCdInitializedPlayerBytes=1548`만 추가한다. count9와 factory120은 기존 공개 상수를 재사용한다. 모든 장치는 기존 메모리/할당/반납/동맹 인터페이스로 실제 호출 위치에 연결하며 빈 callback이나 성공값을 만들지 않는다.

## 검사

새 `PlayerInitialization_Cd*` 비MP 초기화 검사는1548바이트 전체·양쪽4바이트 canary·이전 두 물리 보존·주소 조회/쓰기/반납/할당/동맹의 순서와 인자, free 뒤 base 교체, new 전 캡처 행과 new 뒤 현재 동맹 base, 첫/마지막 new의NULL, malloc NULL·짧은 DWORD 물리·5번째 new 외부 예외의 부분효과를 확인한다. 동맹 장치는 원문이 요청한 인자를 관측할 뿐이며 실제 동맹/Chat/패킷/통신 본체는 실행하지 않는다.

초기화의 새 기대값은 정적 원문에서 고정했다. 아래 통합 결과와 구별하여, 새 CD 기계어 오라클 실행은 아직 하지 않았다. Client 소유자·최초 known0 동맹·메뉴 연결은 같은 단위에 포함했으며 GameWorld 연결은 후속이다.

## CD 기계어 기대값 생성 가능 여부

기존 `tools/decomp_player_initialization.py`는 PATCH 원본·`004910a0`/동맹/Chat 주소·1620바이트를 고정하고, `--observe`는 기존 제품 파일의 부재를 요구한다. 따라서 **현행 파일을 그대로 CD 관측에 실행할 수 없다**. `decomp_player_factory_init.py`도 PATCH/1620으로 고정되어 있다. `decomp_player_state_initialization.py`는 다른 State getter의28바이트 초기화 관측이므로 이 부모의 대체 도구가 아니다.

새 CD 전용 관측기를 준비하면 독립 기대값 생성은 가능하다. 기존 `decomp_oracle.py`의 판본별 PE 매핑·Unicorn·허용 함수 범위 방식을 재사용할 수 있고 CD의 함수표도 존재한다. 검토 범위는 부모165바이트+행 생성39바이트+행 반납12바이트이며 외부 malloc/new/delete/free를 명시 장치로만 대체하면 된다. 동맹은 이번 API 검사와 같이 호출 경계로 관측하거나, `00406130`의 known0(0050f828=0) 가지를 함께 실행해 실제81DWORD행 쓰기를 확인할 수 있다. known!=0은 Chat 장치를 추가로 필요로 하므로 별도 범위다. 최초 CD fixture는 기존 PATCH fixture와 다른 이름으로 만들고, 전체 메모리/trace/부분 실패와 원본·관측기 SHA를 보존해야 한다. 이번 작업에서는 관측기를 실행하지 않았으며 CD 기계어 기대값 생성 완료로 표기하지 않는다.

## CD 동맹 known0의 후속 최소 경계

`00406130`은 mask에 자기 비트를 OR하고 현재 Player+owner×172+0x54에 기록한 뒤 allies[owner×9+index] 9DWORD에 mask 비트의0/1을 쓴다. `0050f828`이0이면 여기서 끝난다. 숨김 표·역방향 행·CurrentCore는 쓰지 않는다. known!=0이면 ClearChat(0x40) 뒤 localOwner의 양방향 동맹인 참가자의 현재 subscriber(+0x2c)/이름(+0x0c)을 AddChat에 전달하고 매 Add 복귀 뒤 현재 base/localOwner를 다시 읽는다. CD 초기화에 필요한 known0 전용 앞단은 PATCH RawPlayerParticipation 호출 없이 독립 메모리/base/81DWORD표 장치로 구현할 수 있다. 이 동맹 구현 자체는 본 패치의 소유 범위 밖이다.

## Client 소유자와 메뉴 후반 프레임 연결

ClientPlayerRecords는 선택 판본을 소유하고 CD1548/PATCH1620의 실제 할당과 각172/180BYTE 행을 사용한다. 최초 Client 시작이 두 판본 모두 이 소유자를 초기화한다. CD00406130의 known0 앞단은 자기 비트 OR→현재 Player+54 저장→현재 방향9DWORD 동맹 표 쓰기 순서이며 known이 비영으로 바뀌면 Chat 후속 미연결을 명시한다. 현재 Core/기록 주소와 factory 목록은 같은 소유자를 계속 빌린다.

CD00484140은 메뉴에서도 Player1..8의 SID 쌍을 검사한다. 새 RawCdBattleCompletion은 +74/+78, +80/+7c, +84/+88, +8c/+90 순서와 dais.cpp:0x5ef~0x5f2 진단 뒤 battle·missionPost·현재 서버 객체·State14 관문을 복원한다. 현재 최소 범위 밖의 일반 CD 전투 활성 종료 본문은 명시 오류로 남긴다. 10.78 후반 프레임은 기존 실행기를 유지한다.

집중 대상 netstorm_cd_player_startup_tests를 추가했다. 새 Client 소유자 재할당/마지막 행 factory 검사와 비전투 메뉴의64DWORD 조회·진단 복귀 뒤 재조회·짧은 물리 거부를 검사한다. MP 실행 검사는 하지 않았으며 실제 CD 메뉴 실행 결과는 다음 절과 같다.

## 행 주소 캡처 교정과 통합 결과

CD00484157의 `mov edi,[0050f834]`와 뒤의 `add edi,esi`는 한 행에서 한 번만 실행한다. 그 뒤 MOV/CMP는 +78→+74, +7c→+80, +88→+84, +90→+8c 순서로 같은 EDI에서 읽는다. 진단이 현재 base를 바꿔도 그 행은 유지하고 다음 행에서 새 base를 읽는다. 처음 작성한 DWORD마다 재조회하는 구현은 `MenuRetainsCapturedRowUntilNextOwner` 반례에서 1건 실패했다. 행별 캡처로 고친 뒤 이 검사와 전체 집중 19건이 모두 통과했다. 메뉴의 DWORD 읽기는64회, base 조회는8회다.

Release `NetstormCpp`와 `netstorm_cd_player_startup_tests` 빌드는 종료 코드0이다. 집중 19건에는 새 CD 순수 초기화5·소유자1·메뉴 진단4와 기존 PATCH9(독립 원문202관측 포함)이 들어 있다. 새 CD 기계어 오라클을 실행한 것으로 세지 않는다.

실제 CD 자료의 **메인 메뉴가 640×480으로 표시**되었고 저장한 화면도 직접 확인했다. 로컬 전체 자료, NETSTORM.TARC+D/setup.cfg만 둔 설치 사본의 상대 한글 CDDir, 저장된 CDDir 재실행의 세 경로가 모두 종료 코드0·stderr 없음·화면 생성에 성공했다. 잘못된 CDDir은 SHP 부재로 실패했다. 원본 CD 전 파일의 실행 전후 SHA-256은 일치했다. 10.78 일반 메뉴40프레임 대조도 종료 코드0이다. 원시는 `extracted/cpp-cd1072-1309/`에 보존했다.

튜토리얼1은 단순 진입만 시도했으며 `RawMenuHtml.inc:3`의 `HTML 실제10.78 Client 전역/벽 시각 필요` 판본 검사에서 종료했다. 따라서 **튜토리얼 진입·미션 플레이는 미완료**다. 이 HTML 차단이 이전 정적 조사에서 예상한 SessionAssets 거부보다 앞에 있다. CD HTML·Init/Start·실제 월드 접합을 후속으로 진행한다. 일반 CD 전투의 활성 종료 본문·known 비영 Chat 후속·MP 실행·사제 포획/희생·특정 적 공격·승리·사용자 직접 확인 검사는 완료로 표시하지 않는다.
