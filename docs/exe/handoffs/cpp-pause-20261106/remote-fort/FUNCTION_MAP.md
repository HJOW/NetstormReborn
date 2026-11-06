# 986 원격 Fort 및 기 확보한 sync 원문 함수표

기준: refined originals/Netstorm.c, 보호 원본 Netstorm.exe 읽기 전용 / 마지막 전체 정밀 디컴파일 HJOW-Athlon(192.168.0.94),2026-11-03. 제품985는 미변경이다. 아래 표는 정적 대조/설계이며 동작 검증 결과가 아니다.

| 원문10.78 | 현재 실제 부모/owner | 확인한 효과·접합 상태 |
| --- | --- | --- |
| 004a7800→004a6c50 | RawMissionLogin::Receive previous0 | Player+5c/+60,fortCount++,lastLoginPlayer 후 자기 Reload 또는 원격 loadFortImage 선택. 원격 hook 미연결 |
| 00460680 | Login::SelectFile/Reload,EditorFortFileDevices::CurrentPath | 현재 curFort·고정 FileSpec 선택·Config path, 자기 파일 Load. 원격 대체로 사용 금지 |
| 004606d0 | 전용 현재 경로 장치 필요 | 현재 myPlayerId 비교진단→'%d' owner→PathSpec('BattleFortSpec',curFort,ownerText,0). 이름/selectedFileSpec 변경 없음 |
| 0040da40 | 같은 PlayerWorldState.battleOwner | 실제00540c70 단순 getter. known 성공 강제 없음 |
| 004a89c0 / 005035b8 | 기존 포매터/CRT,PE '%d' | 지역256BYTE 현재 owner 문자열. 추측 prefix/확장자 없음 |
| 004410f0 | ConfigInterface::PathSpec | local1=현재 curFort/local2=owner 문자열/local3미제공. 기존 Config 치환 결과 사용 |
| 004bfba0 | RawNewFort::CreateNewFortManager + CreateNewEditorFortManager | 이미 실제 구현. 같은 header+4=0/+16=1,존재 재조회/임시 manager 삭제/소멸/free/최종 공개/35섹션 초기화 |
| 004bcc60 | ConstructTemplateDataManager/Storage::ConstructEmptyManager | manager276의 실제 ctor/filename256/공유 serializationVersion0. 기존 owner 재사용 |
| 0041ab10 / 0041aef0 | EditorFiles::Exists/Delete | 명시 checked namespace의 실제 파일 장치. 설치/보호 경로에 직접 전달 금지 |
| 0044d870 | Storage::InitializeEmptyManager/InitializeDataManager | 실제 mutable SNART 표1회 조회와 고정 manager Create 순회. 기존 body 재사용 |
| 004bfc60 | CampaignTemplateStorage ReadImage 부모 필요 | 지역 MarkStream20/복사/서명/현재 manager 섹션 병합/Notify/지역 반납. 미구현·스케치 적용 금지 |
| 0044cc20 / 0044cce0 | AllocateMemStream/WriteMemStream | alloc 선행쓰기·실제 요청 할당0·현재 pos/end. 기존 raw body 재사용 |
| 004bce10 | ReadTemplateFileSignature | 비영data/end→현재두BYTE/pos→manager+10c/+110→F 판정/실제 현재 filename 삭제 요청 |
| 0044d380 / 0044cdb0 | ReadDataManagerImage/AdvanceDataManagerMark | 실제 이름표·섹션 새 할당·복사·빈섹션 병합·기존제거·목록추가·mark순서. 기존 Image 어댑터 접합 필요 |
| 004bd360 | RawPlayerTemplateState::Changed | filename·+8/+12·mission·manager·+16 재조회,선택저장. 같은 Session 실제 body 사용 |
| 004b3d30 | RawMissionClientPackets::Fort | 원격 client openFortImage/readFortImage 다음 buildFort. 자기 ordinary localFort와 server revision 자연반환은 별도 |

## 이미 확보한 004a7350 후속 함수표 — 구현 확장 없음

| 원문10.78 | 기 확보한 현재 코드·필수 후속 범위 |
| --- | --- |
| 004a7350 | clientSidAllowed005c7a78=1→압축 같은owner 모드1→sync target005c7a7c=owner→실제 고정 서버 SID15000..23000 두 순회→모드0→프로세스 가상+48/+4c 선택→clientSidAllowed=0. 현재 synchronizePeer hook 누락 |
| 00433100 | sameOwnerEnabled0054d8e8=인자,lastOwner0054d8e4=-99. 실제 RawSquidTransmissionState에 두 필드가 존재하지만 전용 부모 호출 접합 필요 |
| 004ac1e0 | 현재 TYPE 속성0x800 판정. 첫 순회 set,둘째 clear를 분리. 현재 SidPool/TYPE 조회 body를 명시 빌려야 함 |
| 004ac260 | 같은 실제 +54 writer와 대상 owner/selector=-3/writer arg1/flags 선택 송신. 현재 RawSquidTransmission::Pop은 일반 broadcast(-1,-1,0,flags) 범위이므로 그것을 호출하여 대상 송신 완료로 가장하면 안 됨 |
| 004a6d00 | 현재child WORD+6→sibling WORD+4 순회. server SID15000..23000은 송신/재귀,그 외 client SID의 현재 TYPE<70 및00541164일 때만 선택 송신. 실제 대상 전송/동일 writer 인자와 수명 검토 필요 |
| 004ac200 | 두 서버 SID 순회 뒤 현재 process TYPE 속성0x10000 검사. 현재 Pool limit/활성 슬롯을 실제 재조회해야 함 |
| process VT+48/+4c | 원문0 인자로 virtual 메시지 두 번. 현재 전용 가상 분배가 각 실제 객체를 처리하는지 함수별 대조가 필요하며 빈 성공 불가 |

첫 두 순회는 같은 슬롯의 flags+0b bit1 없음/TYPE+0a>0x45를 검사한다. flags+28의 bit0에 따라1 또는2,bit3이 있으면4를 선택하여 대상 전송 후 child 사슬을 보낸다. 고정 SID 한도는호스트 전체 limit로 임의 변경하지 않는다. 원문 sync target과 기존 clock sync token005c8bf4는 별개이다. 예외시 전역을 임의 정상화하는 정책은 원문에 없으므로 차후 실제 host owner/실패 경계와 함께 명시 검토해야 한다. 이 표 이후 원문 분석/새 코드 작성으로 확장하지 않았다.
