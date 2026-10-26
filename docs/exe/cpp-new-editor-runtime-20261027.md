# 신규 Fort 설치의 실제 Client 연결 — 단위472

Edit의 새 맵 요청이 파일 부재를 만나면 실제 새 manager와 작업 파일을 만들고, 같은 Client의 초기 자료를 저장한 뒤 빈 편집 월드를 설치하도록 연결했다. 자동 Add Island 명령의 입력 큐 예약까지 검사했으며, Add Island 전용 창과 실제 섬 배치는 후속이다. 작업일은2026-10-27, 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 마지막 수행일2026-10-23**이다. 디컴파일 산출물은 변경하지 않았다.

## 연결한 동작

`RawCampaignNewFort.inc`는 원문 `004b3920`의 NewData를 Session의 실제 장치에 연결한다. Subscriber 읽기·등록·저장이 같은 Client의 ID와 CRT 설정을 사용하고, handle은 실제 현재 버퍼를 빌린다. SP는 기존 저장 본체와 `AddSp`, Core는 같은10BYTE 저장소와 활성 값, seed는 같은 난수 소유자를 사용한다. Territory120BYTE와 Money도 같은 manager의 typed stream에 쓴다. State 변경과 Notify가 실제 작업 파일을 생성·저장하므로 임시로 만든 빈 Fort 파일을 읽지 않는다.

파일 부재 부모 `004b5ca0`은 단위470의 실제 manager 생성 장치를 호출하고 NewData가 정상 종료한 뒤 alternate1을 유지한다. 기존 파일은 최초 exists 검사를 반복하지 않고 기존 부모의 그 다음 위치부터 실행한다. 기존 `InstallExistingEditorFile` 진입점은 그대로 보존했다.

alternate1은 실제 일반 Fort 월드 적재로 전달한다. 원문대로 Objects/Money/Badges/Technology 읽기를 생략하고, 빈 Territory에서 섬 개수가0이므로 population을 요구하지 않는다. population을 빈 성공 콜백으로 대체하지 않았다. 같은 Player·Deck·Snapshot·EditorPlayers, State Run/확정과 최종 공개를 거쳐 실제 입력 큐에 `02000022`(Add Island)를 예약한다.

호스트는 `LOADLOADFORT`가 요청하는 `NewFort` 명령79를 인식한다. mode0 이름 창은 World가 생기기 전에도 작업 파일 장치를 준비하며, 이후 State 분배기를 별도로 처음 한 번 구성한다. 설치 전에는 작업 부모 디렉터리만 만들고 파일이 있을 때만 기존 Fort를 파싱한다. 이 호스트 경로는 빌드했지만 실제 GUI로 실행하지 않았다.

## 검증과 한계

[실행 보고서](../../cpppj/new-editor-runtime-20261027.json)에 명령, 종료값, 원시 로그와 SHA를 기록했다. 최종 Release build4는 종료0·경고/오류0이며, 최종 게임 SHA256은 `b5c59950d0778bbb8c458c62576159717c21bbe9d088bfe4c597694395cc3506`이다.

- 신규3검사는 실제 작업본 생성·저장·전체 재파싱, 같은 Client의 SP/Core/Subscriber/난수, 첫 제목 콜백 실패의 부분 효과 보존, 빈 월드 설치·State 확정·Add Island 큐 예약을 확인한다. UI 꼬리는 기존 검사와 같은 동기 장치 범위이며 실제 창·HUD 픽셀 검사와 구별한다.
- 고정 time32=946684800 입력에서 seed 하위30897, Money502, 자료 작성 후 공유 난수 상태2857353399를 확인했다. SP는 원문 `0040eec0`의 Fort/Editor 고정 모드에 따라194162다. NewData의 차감 호출을 편의상 SP0 쓰기로 바꾸지 않았다.
- 최종 집중148개(ExistingEditor58 + NewFort12 + Template47 + DataManager21 + FortSubscriber4 + SaveFortName6), 전체 C++ **3,233개, 실패0**이다. CTest wrapper 대신 `netstorm_tests.exe` 전체를 직접 실행했다. 기존3,230개 등록 이름은 모두 유지했다.
- 기존 보호 입력·검사3,161파일은 바이트동일이다. 기존 검사 파일 한 개는 원래116,454BYTE 전체를 접두부로 보존하고 새 include만 덧붙였다. 원본/CD/AGENTS와 기존 fixture는 변경하지 않았다.
- 최초 build1의 신규 검사 헤더 누락, build2 뒤 신규 검사의 State 할당 입력 누락, build3 뒤 신규 검사의 미정 SP 입력 실패는 원시 로그에 보존했다. 신규 검사에 선언 헤더와 명시 A5 할당 입력·시작 SP0을 공급했고 실제 Client의 기존 할당 장치와 원문 부모는 유지했다. 최종 build4 이후 소스를 동결해 집중·전체 검사를 마쳤다.

원시 로그와 소스 manifest는 `extracted/cpp-new-editor-472/`에 있다. 이 단위에서는 원본 PE CPU 검증을 새로 실행하지 않았으며, 단위470에서 검증한 부모를 실제 Session에 채택했다. 게임 프로세스·GUI·음향 장치를 실행하지 않았다.

다음은 Add Island 전용 MetaImage·Bitmap·목록/창 수명과 실제 배치·population 연결이다. 실제 Create New Map 이름 입력부터 자동 Add Island까지의 GUI 확인, Test Battle/언어별 시나리오, 전체 미션 자연 완주와 화면·음향 동일성은 계속 남아 있다. 새 맵의 전체 사용자 플레이 완료로 평가하지 않는다.
