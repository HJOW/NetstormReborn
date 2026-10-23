# 실제 Session의 전체 Fort 저장과 Client 상태 연결

2026-10-24 단위382. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 마지막 수행2026-10-23**이다. 기반은 단위381 `fa7762f9`의 전체2464검사 통과 제품이며 기존 원본/CD와 AGENTS.md는 변경하지 않는다.

## 실제 저장 경로

`RawCampaignSession::SnapshotFort(slot)`가 원본004bf9a0 부모를 호출한다. 같은 Storage의 TypeNames·기술·전체Objects·Territory·Deck·Money·Mission·Badges를 순서대로 기록한 뒤 같은 `RawPlayerTemplateState::Changed`에 통지한다. 현재 Fort 모드와 선택 owner를 임의로 변경하지 않는다.

Technology·Money·Mission의 새 섹션은 전체 부모가 한 번만 생성한다. 생성 당시 manager/노드/세대를 가진 `TemplateWriteStream`을 호출 내 registry에 보관한다. 기술 본문만 기존 `ContentWriter`의 내부 토큰1로 연결하여 외부 registry 토큰과 섞지 않는다. Money는 같은 Session 난수를 사용하며 현재 SP를 복사하지 않는다. Mission은 호출별 쓰기 함수만 새 stream에 연결하고 지역 캡처를 서비스에 남기지 않는다. Badges는 같은 장부의 실제 getter와 인라인 writer를 사용한다.

`BindFortFileSave(readOnlyForts,files)`는 장기 const DWORD와 실제 `EditorFiles`를 빌린다. 마지막 Notify가 실제 이미지 생성·현재 owner/숨김/readOnly 관문·작업본 파일 쓰기를 수행한다. 이미지 생성 전에 readOnly로 저장 전체를 건너뛰지 않는다. 연결한 장치와 DWORD는 Session보다 오래 살아야 한다. 선택된 필수 장치가 없으면 해당 위치의 앞선 효과를 보존한 채 실패한다.

## 실제 Client 상태

Client가 `RawFortBadgesGlobals` 한 개를 소유한다. 생성에서 원본 BSS0→CRT를 한 번 적용하고, Run의 시작 지도 초기화 위치에서 count reset을 한 번 실행한다. 매 미션·Session 생성에서는 reset하지 않는다. `FortBadges()`를 통해 같은 소유자를 실제 Session 서비스에 공급한다. generic Fort 로더의 실제 Badges 갱신은 아직 후속이다.

Client의 실제 readOnly DWORD는 누락 설정에서 현재값을 보존하고 signed 전체 비트를 유지한다. 초기 옵션 해석과 동기 설정 알림이 같은 helper를 사용한다. 소멸 첫 위치에서 알림을 해제한다. 원본 전체 InterpretOptions callback의 다른 언어·화면·음향 효과까지 복원한 것은 아니다. [옵션 제안과 격리 근거](cpp-fort-file-save-options-proposed-20261024.md)에 그 차이를 남겼다.

UberGump의 실제 MissionRuntime은 포인터를 절단하지 않는 비영 수명 토큰을 가진다. `FortMissionSaveEffects()`는 guided 여부와 무관하게 현재 Runtime 유무·NUL 포함 `init.name`·같은 `state.sequence/flags`를 읽는다. 미션 교체 뒤 옛 토큰의 필드 접근은 거부한다. 현재 getter만 장기 보관하고 고정 Runtime 포인터를 잡지 않으며 소멸 중 자동 Snapshot을 추가하지 않는다. 읽기 검토에서 생성·반납·현재값 재조회와 명백한 수명 결함이 없음을 확인했다.

## 검증 범위와 상태

[Session 제안](cpp-fort-snapshot-session-proposed-20261024.md)은 단위381 ABI를 읽기 링크한 격리 Release에서 신규3검사/실패0·경고오류0을 통과했다. 그 최초 patch와 원시 로그는 보존한다. 공용 등록 뒤에는 첫 왕복을 실제 Client의 Config 알림으로 readOnly `80000000→0`을 변경하고 같은 Client 장부를 공급하도록 확장했다. 각 파일 장치와 관문은 Session보다 먼저 생성하여 더 오래 유지한다.

신규 공용 검사는 전체 저장/파일 재파싱/별도 Session 객체 재로드 두 pass, 선택된 네 누락 지점, 현재 Mission0 및 preventSave 관문을 확인한다. 실제 Client 검사5개는 동기 설정·입력 큐 Config·Startup/누락 재적재·중복/새 수명·같은 장부 보존을 확인한다. 직접 count를 입력한 검사는 실제 게임에서 Badges가 획득됐다는 증거가 아니다.

공용 Release 제품/검사 빌드는 종료0이다. 신규8검사와 기존 관련30을 합친 **38검사/실패check0**을 통과했다. 제품 SHA256은 `ba96540c2532d11702a9d0f254a054157291d909b0db9b3682797b64a1e5dda8`, 검사 SHA256은 `b725e2be52be14f0cd36f4333847741c84a006d0fe3ffb1eac4d134fd29aab71`이다. 기존 판본 공유 검사 합성파일의 경로도 실행 cwd와 무관한 `cpppj/build/test-data`로 정리했다.

같은 제품의 양 판본 설정 CLI 회귀가 통과했고 각 options.cfg를 복원했다. 기존 기본 산출물은 `...-config-before`로 보존하고 새 결과는 `...-config`에 별도로 복사했다. 실제 `...-dialog-exit`에서 No의 계속 실행/기존 크기 보존과 Yes의 실제800×600 저장 후 정상 조기 종료를 확인했다. `...-game-menu`는1024×768/1280×720의 실제 메뉴/ESC와1280×720의 음량·해상도No·QWASZX 여섯 선택을 통과했다. 해당 두 GUI 보고서의 모든 제품SHA가 위와 같고 각 originalsUnchanged=true다. 입력 큐 검증을 OS 키 메시지 발생 자체 또는 원본 전체 HUD 동일성으로 확대하지 않는다.

최종 전체 CTest는 **2472검사/실패check0·종료0**, 310.01초(전체310.05초)다. 외부 CTest 결과와 보존한 내부 LastTest의 실제 검사 수를 함께 확인했다. 원본1394파일의 전체 manifest도 이전 실행 후와 같고 추가·누락·SHA 변경0이다. 엄격UTF8·문서 링크와 소스 공백을 확인했다. 기존 CRLF 소스의 CR과 보존 patch 문맥의 의미 있는 공백은 유지한다. 원시는 `extracted/cpp-fort-snapshot-session-20261024-*`에 보존한다. 제품/검사 소스는 최종 빌드 이후 변경하지 않았다.

최초 stage 두 번은9분 된0BYTE index.lock 때문에 실패했다. 실행 Git 프로세스가 없고 해당 파일의 배타 열기가 가능함을 확인한 뒤 `...-stale-index.lock`으로만 이동 보존했다. 실제 index는 변경·삭제하지 않았으며 재시도 stage가 통과했다.

## 계속 남는 작업

실제 사용자 작업 디렉터리의 목록·읽기·이름·저장·삭제와 Editor 메뉴에서의 FileSave 바인딩, generic Editor 최초 설치 및 TypeNames/Deck/필수 후처리는 미완료다. 이번 왕복 검사는 캠페인 설치 뒤 명시 Fort 입력을 공급하며 Editor 최초 설치로 표현하지 않는다. Battle CoreData, 전체 UI·음향 동일성 및 모든 미션의 사용자 완주도 별도 잔여다. [generic 설치 제안](cpp-generic-editor-install-20261024.md)은 아직 적용하지 않았다. 다른 PC의 인수인계에도 남도록 [동결 patch](patches/cpp-generic-editor-install-20261024.patch)를 함께 보존한다(SHA256 `5c8030c290bdab8605bddd8e00024c76b7247c2e19660925aea149fb0fcece90`). 새 Session 변경 이후의 문맥을 검토한 뒤 필요한 실제 하위 모듈과 함께 적용해야 한다.
