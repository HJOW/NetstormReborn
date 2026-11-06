# 986 원격 Fort 이미지 접합 설계 — 중지 인계

2026-11-06 / HJOW-Athlon. 작업 전 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽었고 마지막 전체 정밀 디컴파일 **HJOW-Athlon/2026-11-03**과 현재 호스트가 일치한다. 제품은 **985 / 커밋1f00a25a / 표시11.01**을 유지한다. **정식986 산출물은 원문 대조·접합 설계·함수표이며 실제 제품 구현은 미반영이다.** 에이전트 빌드/게임/GUI/소켓/통신 실행·신규 MP/단위 검사 작성은 모두0이다. 원본/fixture/WB/CMake/LEFT/제품 소스를 변경하지 않았다. 사용자 중지 지시에 따라 새 구현이나 월드 sync 분석 확장을 진행하지 않는다.

## 보존 범위와 상태

- 이 PLAN.md와 FUNCTION_MAP.md는 완료한 정적 조사 결과이다. manifest.json은 문서와 참고 제품 파일 SHA를 담는다.
- 설계 후 별도 scratch에 생성되던 CampaignTemplateStorage.h/.cpp 두 파일 스케치는 중지 지시 도착 시 즉시 멈췄다. root의 보존 지시에 따라 원문 바이트를 `unvalidated-draft/pending/`에 두고 출처/미검증 경계를 표시했다. **제품 미반영·미컴파일·전체 최종리뷰 미완료·적용 금지 초안**이다. 임시 패치의 적용 점검 성공을 구문/수명/행동 검증으로 읽지 않는다.
- 후속 구현을 재개하려면 먼저 현재 제품과 아래 계약을 다시 대조해야 한다. 스케치의 ReadImage 조각만 적용하여 원격 Fort 전체가 끝났다고 쓰면 안 된다.

## 실제 원문 사슬

1. **004a6c50**은 Player(owner)+5c에 현재 fortCount, +60에 현재 predictableCount를 쓰고 fortCount 증가/lastLoginPlayer=owner를 먼저 수행한다. 현재 RawMissionLogin의 previous0 블록이 이 순서와 자기 파일/원격 분기를 이미 인라인으로 구현했다. 서버이고 owner가 현재 myPlayer이면00460680→004bf550의 기존 자기 파일 Load이다. 그 외는004606d0(owner)→004bfba0(header,path,1)→004bfc60(header,image,size)이다. 원격을 LoadCurrentFort로 대체하지 않는다.
2. **004606d0**은0040da40의 현재00540c70을 읽고 같은 owner이면 `playerId != myPlayerId()` 진단(Fortspec.cpp:0x2d) 후 계속한다. 005035b8의 실제 문자열 `%d`로 owner를 지역256BYTE에 작성한 뒤 `Config.PathSpec("BattleFortSpec",현재 curFort,owner문자열,0)`을 반환한다. 현재 Fort 이름이나 고정 FileSpec 선택 포인터를 변경하지 않는다. 00460680의 selectFileSpec 동작과 합치지 않는다. 0040da40은 현재 owner를 그냥 반환하며 known 성공 상태를 만들지 않는다.
3. 실제 originals/d/setup.cfg를 기존 XOR 키로 **읽기만 해 확인한** 기본식은 `battleFortSpec = "{InstallDir}{DataDir}\{local.1}.player{local.2}"`이다. 원본 PE에서005035b8=`%d`, Template 진단 문자열00514770=`!tm`,0051476c=`tm`을 읽기만 해 확인했다. 추측 이름/확장자나 `.fort`를 추가하지 않는다.
4. **004bfba0**은 현재 header.manager 비영 진단→header+4=0/+16=argument→파일 존재 재조회→존재하면 임시 manager276 생성/그 실제 가상 filename 삭제/소멸/free→최종 manager276 생성/현재 header 공개→35섹션 초기화이다. header+8/+12는 임의 초기화하지 않는다. 동일 몸체는 이미 `RawNewFort::CreateNewFortManager`와 `CreateNewEditorFortManager`/CampaignTemplateStorage의 실제 owner에 구현되어 있으므로 재구현하지 않는다. Install/Clear/LoadFile로 대체하면 header 부분쓰기/삭제/임시 수명/공유 version0이 달라진다.
5. **004bfc60**은 현재 manager0/inBattle0 진단→지역 MarkStream20(first16=0,mark=-1)→0044cc20(size) 실제 할당/0→0044cce0(image,size) 복사/pos·end→pos=0→현재 manager 가상+c 서명004bce10→valid0 진단(반환하면 계속)→그 시점 현재 manager의0044d380 이름표/섹션 병합→같은 고정 header의004bd360 Notify→지역 수신 할당 free이다. 예외에서도 지역 할당을 반납하지만 앞서 바꾼 header/서명/부분 섹션을 rollback하거나 자기 파일로 재로드하지 않는다.

## 가장 작은 실제 후속 접합

### 경로와 사용자 작업 namespace

004606d0의 Config 현재값/인자 순서와 반환 이름을 먼저 그대로 구한다. 그 뒤 원본과 별개인 호스트 경계에서 현재 명시 user workspace로 매핑한다. 기존 `EditorCatalogWorkspace::Resolve`는 source 내부 절대 경로/작업 내부 절대 경로/상대 경로를 전체 상대 키로 매핑하고 원본·외부 링크를 검사한다. **Resolve만 빌리고 Import/Prepare로 원격 데이터를 설치 자산에서 복사하지 않는다.** 원격 원문은 실제 수신 이미지이다.

`EditorFiles`의 같은 checked namespace/공유 RawFileName owner를 존재·삭제·쓰기 장치에 사용해야 한다. 실제 파일 자동 생성은 사용자 홈 Netstorm 하위여야 하며 보호 originals/originalCD/설치 폴더에 쓰지 않는다. 별도 remote cache namespace를 선택할지 기존 user workspace를 빌릴지는 상위 Client owner가 명시한다. 상대키를 filename/첫점으로 축약하거나 임의 경로를 설치 폴더 fallback으로 허용하지 않는다. 경로 생성 자체가 파일/폴더를 미리 만들 필요는 없다.

기존 `CreateNewEditorFortManager`를 같은 storage/files/slot/path/argument1로 호출한다. 현재 NewFortManagerDevices.cpp의 erase 장치는 `ResolvePath(name).string()`을 쓰므로 새 경로 접합 전 UTF-8 사용자 경로에 맞춰 `platform::PathToUtf8` 사용을 검토해야 한다. 기존 코드의 재사용과 별개로 이 한글 경로 손실 후보를 놓치지 않는다. 제품은 이번에 변경하지 않았다.

### 실제 저장소의 ReadImage 부모

CampaignTemplateStorage에 선택된 현재 slot의 원문 ReadImage API가 필요하다. 이미지 입력을 FortTemplate::Parse 편의 사본으로 Install하지 않고, 기존 `ReadTemplateFileSignature`/`ReadDataManagerImage`/MemStream allocator·writer·destructor/동일 Image 목록 및 버퍼 장치를 사용한다. 현재 ReadFile 내부의 실제 stream 토큰/버퍼 생성/기존 section 제거·병합/BindFileStreams 장치를 최대한 재사용한다.

서명 진입의 manager는 그 가상 this로 고정한다. 서명/진단 뒤 원문이 다시 읽는 header.manager를 본문 진입에서 재조회하고, 실제 섹션 읽기 동안 manager의 같은 물리를 보유한다. 잘못된 서명은 해당 manager의 실제 파일 삭제 요청과 valid 진단을 보존한다. 원본 보호를 우회하지 않도록 삭제 대상도 같은 검사된 사용자 namespace여야 한다. 지역 source 할당은 성공·서명 거부·본문 예외·Notify 예외에서 한 번만 반납하고 부분 manager 효과를 되돌리지 않는다.

Notify는 독립 빈 callback을 쓰지 않고 Session의 실제 `RawPlayerTemplateState::Changed(slot)`에 연결한다. 이 본문은 현재 filename 첫BYTE, +8 isMine/+12 transient, inMission, manager, +16 preventSave를 다시 읽는다. 원격 NewManager의 argument1은 +16=1이므로 올바른 현재 조건에서는 실제 저장 부모를 자연 생략하지만, 그 앞 진단/관문까지 통째로 생략하지 않는다.

### 실제 Session 소비자

현재 RawCampaignSessionLogin.inc의 serverHooks.loadFortImage(owner,image)를 위 경로→기존 NewManager(argument1)→ReadImage→같은 templateState Changed에 연결해야 한다. 현재 serverHooks.loadCurrentFort(Reload)는 자기 서버 파일 분기로 유지한다. client packet의 openFortImage는 같은 경로/NewManager만, readFortImage는 같은 현재 manager ReadImage/Notify만 연결해야 한다. 두 선택을 합쳐 불필요한 manager를 재생성하지 않는다.

Client packet2a의 owner==현재 local인 일반 경로는 먼저 localFort00460680/Flags/Player/metadata 사슬을 요구한다. 원격 이미지 구현만으로 그 경계를 없애면 안 된다. 또한 원격 이미지 읽기 뒤 buildFort0046d9b0→0046da60→0046e2c0은 별도 실제 생성 부모이며 아직 미연결이다.

## 현재 첫 missingNeed와 다음 재개 판단

- 현재 제품985 server 일반 원격 신규 owner: 원문 prefix/CheckLogin/Begin/Options/Player/metadata 뒤 previous0의 `loadFortImage`가 아직 serverHooks에 없다. 중앙 metadata가 선택되면 그 기존 중앙섬 경계가 더 먼저 발생할 수 있다.
- 자기 owner는 기존 실제 Reload 경로이며, 원격 작업을 그 경로로 우회시키지 않는다.
- client packet2a 원격 owner: server이면 기존 revision 증가 후 자연 반환, client이면 `openFortImage`가 현재 첫 누락이다. 이미지 선택 조건(missionPost 또는 !hidden) 뒤에는 `readFortImage`가 누락이며, 연결 후 다음 실제 누락은 `buildFort`이다.
- client packet2a 자기 owner·missionPost0: `localFort`가 먼저 누락이다. open/read를 추가해도 이 부모를 대체하지 않는다.
- 원격 로그인 이미지 이후 다른 owner 전체월드 `synchronizePeer`004a7350은 현재 별도 누락이다. 이번 중지 이후 새 구현으로 확장하지 않았다. 기 확보한 원문 함수표만 FUNCTION_MAP.md에 보존했다.
- 일반 UI Multiplayer NotImplemented, roster/subscriber/Zone, ordinary State6/7/8/9/13, metadata 중앙섬, battle-island builder, 재접속 저장/Commander, checking8의8번 이월은 유지한다. 공식 서버/DNS/유효성/R.exe/root6800/7f80은 재활성화하지 않는다.

## 재개 전 필요한 확인

현재 제품/관련 파일 SHA를 재확인하고 이 설계의 source·workspace·Client/Session 실제 owner를 확정한다. 적용 금지 스케치의 서명/부분 목록/예외/manager 재조회도 새로 최종 리뷰해야 한다. 그 뒤 한 유한 단위의 실제 접합·root 빌드와 문서/커밋을 진행할 수 있다. 이번 인계는 구현·검사·MP 동작 완료 선언이 아니다.
