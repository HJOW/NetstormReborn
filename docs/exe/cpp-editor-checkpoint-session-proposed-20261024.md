# 기존 Editor checkpoint의 실제 Session 접합 제안

현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 이 제안은 공유 소스·CMake·LEFT·공용 빌드·라이브러리·GUI·git을 변경하지 않은 사본이다. 004b743b까지의 부모이며 최종 HUD/State/loaded/UI와 전체 Editor 완료는 포함하지 않는다.

- 제안: [patch](../../extracted/cpp-editor-checkpoint-session-20261024-proposed.patch), [검증](../../extracted/cpp-editor-checkpoint-session-20261024-proposed/verification.json), [사본 안내](../../extracted/cpp-editor-checkpoint-session-20261024-proposed/README.md).
- 선행 의존: [파일 장치](cpp-editor-file-devices-proposed-20261024.md)의 동결 patch `ba7a9cda55e66e341ae5715c646b46c9518556b1c433d889f0532a69c6d6bc48`, 공유의 실제 `RawStartupStateCheck`, 단위384 owner/Subscriber/Storage이다.
- 범위는 기존 Session/Runtime 네 파일과 신규 검사 초안 한 파일이다. 원문 `RawExistingEditorInstallOuter`와 `RawStartupStateCheck` 본문을 복제하지 않는다.

`RawCampaignSessionServices::deferExistingEditorFile`은 기본 false다. 명시 ExistingEditor에서만 true를 허용하고 ctor의 기존 Install/InstallFile을 미룬다. Campaign/LocalLogin 및 기존 단계별 Editor 입력의 설치 순서는 그대로다. 지연 ctor는 보조 FortTemplate과 실제 요청 이미지의 수명만 보유하며 기존 Storage의 manager/cursor/version을 정리하지 않는다. WorldRuntime 생성은 기존 lazy 경계를 유지한다.

`InstallExistingEditorCheckpoint(effects, client.ReadOnlyForts(), editorFiles)`은 한 번만 호출한다. 실제 장치와 Client/Config/EditorFiles는 Session보다 오래 살아야 한다. `effects`의 현재 이름/pending/이름 변경/Guide/Config/Title/오류 분기는 현재 owner를 빌리는 동기 callback이며 새 Client 이름·dirty DWORD를 만들지 않는다. 이름 변경이 선택되면 실제 00460980 부모가 필요하다.

1. 원문 이름 비교/선택된 이름 변경보다 먼저 같은 Session의 `BindFortFileSave`를 연결한다. 첫 File Notify도 실제 이미지 생성·readOnly 관문·작업 파일 writer에 도달한다.
2. 원문 File 부모가 현재 path/owner를 읽고 실제 Storage.LoadFile을 실행한다. 실제 TYPE 수·현재 슬롯·첫 raw Title을 연결한다. 정상 false는 원문 manager 정리 뒤 InvalidFile 분기로 가며 예외는 공개된 부분효과를 보존한다.
3. valid는 현재 header 첫 DWORD만 읽는다. 현재 State 첫 BYTE의 `|1`, 같은 Notify, 실제 Subscriber, 현재 handle의 Config, 두 번째 raw Title이 따른다. 서명 검사나 parsed Subscriber로 대체하지 않는다.
4. 기존 실제 Session World→같은 부모가 인자로 고정한 슬롯의 Deck→같은 RawCampaignWorldLoad의 FinishMissionPriests→실제 camera/production callback을 실행한다.
5. CoreWrite는 선택 위치의 필수 실제 부모다. 이어 private SnapshotFortBody가 같은 Storage/현재 owner/기술/Objects/섹션/난수/장부를 사용한다. 공개 SnapshotFort의 최종 loaded 요구는 유지한다.
6. 현재 State `&fe`→같은 Notify→실제 CheckStartupState→현재 editor 관문의 Config/실제 EditorPlayers·맵 tail까지 실행한다. 옵션0만 reader를 자연 생략하며 옵션 비영의 실제 772BYTE reader 누락은 중단한다.

checkpoint 성공은 `ExistingEditorCheckpointReady()`만 true로 만든다. loaded를 쓰거나 기존 Load를 호출하지 않는다. 예외/정상 false 뒤 자동 재시도·rollback을 하지 않는다. 지연 Session의 공개 단계별 Subscriber/World/Deck/Players는 이 동기 부모 안에서만 허용한다. 기본 false의 기존 호출 계약은 그대로다.

실제 파일 바이트와 ctor에 넘긴 보조 FortTemplate은 같은 작업본에서 준비해야 한다. 실제 raw 섹션은 LoadFile.read가 읽는다. 이 제안은 별도 파서·섹션 복사·캐시된 parsed Content 성공을 추가하지 않는다. 작업본이 바뀌면 상위 파일 준비 단계에서 기존 FortTemplate::Parse로 보조 입력도 다시 준비해야 한다.

실제 장치가 없는 범위를 숨기지 않는다.

- 0042f540은 Chat 랭크 prefix다. 실제 Client Chat/Core owner가 없는 경계를 0이나 빈 callback의 성공으로 통과시키지 않는다.
- 004923e0은 기존 실제 FinishMissionPriests를 Runtime forwarding으로 호출한다. missionPost의 자연 ret는 그 본문이 판정하며 다른 분기의 미복원 사제 효과는 여전히 필수 중단이다.
- 004beb30은 후속 독립 복원 대상이다. 이 patch에는 성공 CoreWrite나 Editor 전용 임의 생략을 넣지 않는다.
- Camera/Production은 같은 기존 서비스의 실제 callback이다. UI에서 최종 loaded 전 장치를 가로막는다면 설치용 연결을 먼저 마련해야 하며 loaded를 앞당겨 해결하지 않는다.

신규 검사 초안 5개는 명시 지연의 기존 manager 보존/기본 false 호환, Campaign 지연 거부, 실제 파일 읽기 정상 false의 manager 정리, OS 파일 공유 거부로 첫 Notify의 실제 writer 도달, 실제 Storage/ID/Config/두 Title 뒤 Chat 필수 경계를 다룬다. 실제 Client(ReadOnly/Badges/CRT), Storage, 파일 장치를 사용하며 게임 성공은 대역하지 않는다. 전체 checkpoint 성공을 기대하는 검사는 없다.

**검증:** 최초 배치 환경 설정 실패 로그를 보존했다. 수정 뒤 MSVC x64 `/Zs /std:c++20 /utf-8 /W4`로 Session/Runtime/신규 검사 세 소스의 구문 검사 종료0, 경고·오류0이다. 공용 라이브러리 링크·검사 실행은 하지 않았다. 공유 네 파일의 SHA 불변, patch `git apply --check --whitespace=error` 종료0, UTF-8/Python AST/공백/문서 링크를 확인했다.

후속 실제 통합에서는 Chat/CoreWrite를 연결한 뒤 config0의 정상 checkpoint, config 비영/reader 누락의 Snapshot·State 부분효과, 실제 reader/EditorPlayers 정상 복귀를 검사해야 한다. 그 결과도 최종 HUD/State/loaded와 구별한다.
