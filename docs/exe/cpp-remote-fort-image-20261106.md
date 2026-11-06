# 987 — 원격 Fort 경로와 실제 수신 이미지 수명

2026-11-06, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의 2026-11-03 자료다. 이번 사용자 재개 지시에 따라 986의 중지 상태를 해제하고 제품985에서 LAN 잔여 구현을 이어 간다. 멀티플레이 동작/통신/신규 단위 검사는 사용자 지시대로 하지 않는다.

## 원문과 실제 연결

- PATCH `004606d0` / CD `004ca440`: 자기 owner 진단 뒤 signed `%d` owner 문자열과 그 시점의 curFort로 `PathSpec("BattleFortSpec",curFort,ownerText,0)`을 조회한다. `SelectRemoteFortPath`는 이름/선택 FileSpec을 변경하지 않는다. 실제 Client가 조회 결과를 기존 `EditorCatalogWorkspace::Resolve`로 사용자 홈 CustomMaps namespace에 매핑한다. Import/Prepare/설치 자산 복사는 호출하지 않는다.
- PATCH `004bfba0`: 기존 `CreateNewEditorFortManager`와 같은 저장소·파일 장치를 재사용해 argument1로 새 manager를 공개하고 35개 섹션을 초기화한다. 존재/삭제/임시 manager 소멸·최종 공개 순서와 +8/+12 보존을 유지한다. 삭제 경로의 `.string()`을 UTF-8 변환으로 고쳐 한글 사용자 홈이 같은 파일 장치에 전달되게 했다.
- PATCH `004bfc60` / CD `00429bb0`: 지역 MarkStream20의 첫16BYTE=0/mark=-1, 실제 MemStream 할당·0 초기화·복사·pos0, 가상 서명, valid 진단, 현재 manager의 섹션 병합, 같은 고정 헤더의 Changed, 지역 할당 반납을 연결했다. 패치판의 잘못 복원된 지역 스택 변수는 CD판 및 기존 공통 MemStream 부모와 대조했다.
- 서명의 manager this는 진입 시 고정하고, 서명/valid 진단 뒤 본문 manager는 같은 헤더에서 다시 읽는다. 잘못된 서명은 현재 manager 이름의 실제 삭제 요청을 전달한다. 삭제 장치는 검사된 사용자 작업 namespace만 허용한다.
- 파일 읽기/수신 읽기가 `ImageReadDevices`를 함께 사용해 같은 노드 생성, 원래 섹션 제거, 빈 섹션 유지, 이름표와 버퍼 장치를 사용한다. 본문 실패 뒤에도 이미 연결한 노드/현재 커서를 공개하고 앞선 manager 효과를 되돌리지 않는다. 지역 입력 할당은 본문/Changed 예외에서도 C++ 수명으로 반납한다.
- Session 서버의 `loadFortImage`는 준비→읽기, client packet2a의 `openFortImage`와 `readFortImage`는 각각 준비/읽기에 연결했다. `Changed`는 빈 콜백 대신 기존 `RawPlayerTemplateState::Changed(owner)` 전체를 호출한다. preventSave=1의 자연 저장 관문을 생략하지 않는다. 자기 서버 Fort의 기존 Reload는 유지한다.

## 검증과 한계

Release 제품과 기존 `netstorm_mission_entry_tests` 대상이 각각 종료0으로 빌드됐다. 공통 파일 읽기의 기존 비멀티 `CampaignTemplateLoadFile_` 9검사/실패0을 확인했다. 신규 MP 검사, 실제 소켓/통신, 게임/GUI 실행은 하지 않았다. 컴파일 성공을 LAN 플레이 가능으로 표현하지 않는다. AGENTS.md·원본/CD·fixture2800파일은 시작 때 새로 고정한 SHA와 모두 일치했다. 제품 EXE는11,538,944바이트, SHA-256 `004886fd483073b017d8ebd9616ca9a838b3d542ae0bc59af661861eac1656fb`다. 기계 판독 기록은 `cpppj/remote-fort-image-20261106.json`이다.

다음 잔여는 `synchronizePeer(004a7350)`와 대상별 객체 전송, 원격 Fort 실제 배치 `0046d9b0→0046da60→0046e2c0`, 자기 ordinary client의 `localFort`, 중앙섬/월드 생성, 일반 State/Commander/roster/LAN UI/재접속/메타/Outpost/checking8이다. 원격 이미지 읽기 연결만으로 이들을 완료로 세지 않는다. 공식 서버·구 유효성/CD검증을 재활성화하지 않았다.
