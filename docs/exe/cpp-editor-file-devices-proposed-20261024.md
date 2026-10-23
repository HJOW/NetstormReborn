# Editor 작업 namespace의 실제 파일 장치 제안

2026-10-24, 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. AGENTS/공통·닷넷 인계를 다시 읽고 기존 `EditorFiles`/`BaseFileSystem`/`ConfigInterface`/FileSpec/원문 File·Load·State 자료를 먼저 대조했다. 공유 소스·CMake·LEFT/git·공용 build/library·게임/GUI는 바꾸거나 실행하지 않았다. 작업 산출물은 신규 문서와 extracted 사본/격리 콘솔뿐이다.

제안 [patch](../../extracted/cpp-editor-file-devices-20261024-proposed.patch)는 기존 `EditorFiles.h/.cpp` 사본 확장과 신규 `EditorFortFileDevices.h/.cpp`, `EditorFortFileDevicesTests.cpp`만 포함한다. Session/Client/전체 Editor 부모와 CMake 등록을 포함하지 않는다. [Storage LoadFile](cpp-template-storage-load-proposed-20261024.md) 및 현재 root가 통합하는 [전체 설치 수명](cpp-existing-editor-session-install-next-20261024.md)에 하위 장치로 공급한다.

## 실제 원문 분기와 재사용 범위

`0041ab10`은 FileSpec의 경로 부분이 있으면 **그 이름을 fopen("rb")→fclose하고 성공5/실패0**을 반환한다. 경로 없는 이름의 아카이브/기본/보조 탐색과 반환1/2/3/4는 별도다(`Netstorm.c:21022~21071`). `0041ad10`은 경로가 있으면 파일을 미리 열지 않고 그 이름과 출처5를 반환한다(21095~21105). 이 제안은 **검사된 절대 작업본 경로=출처5**만 다루므로 아카이브 출처2/4 판정을 복제하지 않는다.

`0041af80`은 빈 이름 진단0x480→현재 resolver→resolver0 진단0x484→출처2/4면 true→그 외 현재 경로의 CRT stat→실패 진단0x48e→mode 하위 BYTE 부호 판정이다(21194~21210). 그 부호비트는 `_S_IWRITE==0x80`이다. 신규 작업 장치는 **같은 CRT `_stat64`의 st_mode 쓰기 비트**를 읽는다. Windows readonly 속성을 반영하며 NTFS ACL의 모든 쓰기 가능성이나 실제 fopen("w+b") 성공을 예측하는 함수가 아니다.

원문 stat 실패 후 미초기화 mode를 읽는 반환을 임의 true/false로 채우지 않는다. 준비된 직접 경로는 resolver가 자연 출처5를 돌려주므로 없는 파일도 이 분기에서는 먼저 stat에 도달한다. stat 실패의 실제0x48e 진단을 호출한 뒤 host는 명시 중단한다. 빈 이름 역시0x480 진단 뒤 명시 중단한다. 이 미복원 물리 경계를 정상 native 반환이라고 주장하지 않는다.

실제 파일 읽기는 **기존 `BaseFileSystem::TryRead(검사된 절대 경로)`**를 그대로 사용한다. 원문 읽기 실패/빈 할당은 빈 벡터로 인계하고, 정상 false 뒤 manager 정리는 기존 `LoadTemplateFile`이 수행한다. 파서/서명/section body를 새로 쓰지 않는다. 절대 작업 경로가 없을 때 원본 기본 디렉터리/아카이브의 같은 이름으로 fallback하지 않는다.

## 추가 API와 설치 부모 바인딩

|API|역할|
|---|---|
|`EditorFiles::ResolvePath(path)`|기존 같은 Checked/root/보호 루트 규칙을 공개된 작업 경로 해석에도 사용한다. 파일 생성/권한 변경 없음.|
|`PreparePath(path,bytes)`|기존 Prepare의 기존 일반파일 보존/새 파일만 생성/실제 Write를 그대로 재사용한다. 기존 `Prepare(name,bytes)`의 이름 검사와 대소문자/경로 반환 계약은 유지한다.|
|`Exists(path)`|경로를 다시 검사하고 실제 fopen rb 성공5/실패0. 서명·metadata·manager 성공값이 아니다.|
|`ReadOnly(path,assertion)`|현재 파일의 CRT stat 쓰기 비트. header+8의 입력이며 Config readOnlyForts와 별개.|
|`EditorFortFileDevices::CurrentPath()`|현재 Fort 이름의 첫 NUL 진단39→같은 FileSpec 버퍼 선택→현재 key/name 조회→기존 Config.PathSpec→기존 BaseFileSystem과 동일한 상대 경로 해석→작업 절대 경로. 파일 복사 없음.|
|`PrepareCurrent()`|**명시 호스트 namespace 준비**다. 열리는 기존 작업본이면 원본도 재읽지 않는다. 없으면 실제 기존 source.TryRead 뒤 작업본만 준비한다. 원본도 없으면 새 파일을 만들지 않는다.|
|`Read(path)` / `BindLoad(hooks)`|Read는 같은 기존 VFS 절대경로 본문. BindLoad는 read/readOnly/실제 CRT stricmp 비교만 채우고 현재owner/TYPE/제목/진단/거부 요청은 caller hook 그대로 남긴다.|
|`ExistingEditorTemplateValid(storage,slot)`|원문0041aad0처럼 **현재 header 첫DWORD 비영만** 반환한다. 비영 unknown 토큰을 임의 map lookup/서명 오류로 재해석하지 않는다.|

Config가 `\D\Map.fort`를 돌려줄 수 있다. 기존 BaseFileSystem은 `is_absolute()==false`이면 같은 `NormalizeGamePath`로 leading slash/`.`/대소문자를 처리한다. 신규 devices도 **그 함수를 그대로 재사용**해 작업 namespace에 연결한다. `EditorFiles::Prepare` 자체의 기존 파일명 반환을 모두 소문자로 바꾸지 않는다. 실제 absolute Config 경로는 그대로 Checked에 넘겨 작업root 밖/보호 원본이면 거부한다. DataDir 전체나 원본 VFS를 작업root로 바꾸지 않는다.

전체 부모의 `path=[&]{return devices.CurrentPath();}`와 `exists=[&](p){return devices.Exists(p);}`, `valid=[&](slot){return ExistingEditorTemplateValid(storage,slot);}`를 공급한다. load hook은 **현재 slot과 부모가 전달한 path**를 기존 Storage.LoadFile에 그대로 넘긴다. 새 devices는 전체 File/World/Deck/Snapshot/UI/State/loaded 부모를 실행하지 않는다.

기존 수정 작업본이 있으면 `Read`가 읽는 실제 바이트뿐 아니라 **LoadFile의 보조 FortTemplate도 그 작업본에서 기존 FortTemplate::Parse로 얻은 자료**여야 한다. 원본에서 캐시한 parsed fort를 사용하면 Objects/지형 보조 자료가 옛 상태로 남을 수 있다. 이번 장치가 새 파서를 추가하거나 raw 성공 뒤 parsed metadata 일치까지 자동 보장하지 않는다. 초기 파싱/실제 raw read 사이 파일 변경의 수명은 전체 부모에서 다룬다.

`CurrentPath/PrepareCurrent`의 반환 view는 다음 경로 조회 또는 장치 해제 전까지만 유효한 같은 scratch다. bind callback은 장치의 `this`를 빌리므로 devices와 EditorFiles/Config/BaseFileSystem 및 이름/key owner가 모든 사용보다 오래 살아야 한다. 장치 생성 시 미선택 이름/진단을 선행 요구하지 않는다. 파일 접근/쓰기 직전마다 기존 Checked를 다시 적용한다. 보호 originals/CD는 source의 읽기만 허용하고 작업 경로 읽기/쓰기 장치로 넘기면 거부한다.

## 기존 이름·FileSpec·dirty 소유자와 필요한 최소 공개 경계

- `UberGump::pendingFortName_`는 기존 Client UI 수명의 pending 저장소다. `SetPendingFortName`은 원문 점/NUL/진단 순서를 재사용한다. current 이름과 합치지 않는다.
- `UberGump::mapFileSpec_`의 `SoloMapFileSpec::Current()`는 같은9BYTE 주소를 빌린다. 원문00460680/00460980의 선택은 **005327e0 포인터를 이 고정 버퍼로 고르는 효과**이며 그 내용을 매번 fortSpec으로 되돌리는 효과가 아니다. 현재 key가 SOLOSpec이면 그대로 사용한다. 글로벌 선택 포인터의 실제 owner/public API는 전체 Client 연결에서 필요하다.
- **dirty00542678은 이미 `SoloMapCatalog::Impl.cache.dirty`에 존재**한다. `Cached()`는 const이고 `Invalidate()`는1 대입이다. 최소 추가 공개는 같은 cache의 `uint32_t& Dirty()`이며 `RawFortCurrentNameGlobals.dirty`에 그 참조를 공급해야 한다. 새 dirty DWORD나 Invalidate 대입으로 원문 dirty++/wrap을 대체하지 않는다.
- persistent **curFort0055b490 owner는 현재 제품에 없다**. `mission_->FortName()`와 Compose의 지역 current 사본은 실제 Mission 수명이며 Editor current owner가 아니다. 원문00460610은 current/pending256BYTE를 각각 필요할 때만 할당하고 첫BYTE0만 쓴다. 기존 pending을 보존하면서 current의 동일한 장기 쓰기 버퍼/순수 getter가 필요하다. 이 제안은 새 owner를 중복 생성하지 않고 callback으로 그 실제 주소/현재 값을 받는다.
- manager 이름 setter는 기존 `RawFortCurrentName::SetFortManagerFileName`, 전체 current setter는 `SetCurrentFortName`을 사용한다. InstallFile 재설치/파일명에서 basename 추론/수동 dirty 변경으로 대체하지 않는다. 해당 setter에 필요한 실제 manager+12 쓰기 span은 별도 Storage 공개 경계로 root가 통합해야 한다.

## 격리 결과와 실제 검증 환경

현재 **H:는 exFAT, C:는 NTFS**임을 Windows 논리볼륨 조회로 확인했다. 빌드/소스/로그는 작업의 extracted 아래이며, 실제 파일 검사는 `C:\Users\BUZZ\AppData\Local\Temp\cpp-editor-fort-file-devices-20261024\<고유이름>`에 새 합성파일만 생성했다. readonly 검사도 이 소유파일의 실제 `SetFileAttributesW` 속성만 설정/복구했다. H: 결과를 NTFS 관측으로 세지 않는다. 기존 임시자료/사용자 파일은 삭제하지 않았다.

최종 격리 build3/run2 **종료0/경고0/오류0, 신규9검사/실패0**이다. 현재 Config/FileSpec 재조회, 기존 수정본/mtime 보존, 실제 raw LoadFile의 수정 State 소비, 실제 NTFS readonly stat/header+8, 별도 save DWORD gate와 현재 같은 raw 버퍼 저장, valid의 첫DWORD, 읽기 실패와 stat 진단, 빈 current 복구, rooted 게임경로, 작업root/보호경로 거부를 확인했다. 실제 Client callback→readOnlyForts owner의 전체 공급은 기존 root의 별도 검사이며 여기서는 SaveFile이 빌리는 같은 DWORD를 명시 입력했다.

최초 build1의 Microsoft fopen 교체권고 C4996 두 경고는 원래 로그로 보존했다. 격리 CMake에 원문 fopen 사용 목적의 `_CRT_SECURE_NO_WARNINGS`만 명시했고 제품 코드를 다른 파일 API로 바꾸지 않았다. build2 후 실제 경로 정규화 보완을 build3으로 검증했다. 공유 library를 읽거나 링크하지 않고 현재 Storage를 support 사본으로 고정했다. 비선택 전체 월드 링크를 피하려고 기존 RiftTypeTable 네 조회 메서드의 동일 몸체만 추출해 컴파일했다.

원문 새 실행/독립 native fixture/GUI0이다. 이 결과는 작업 파일 장치의 실제 Windows 효과와 C++ 연결 검사이며 전체 Editor 최초 설치·사용자 편집/저장/별도 재로드 성공 판정이 아니다. 전체 실제 파일 삭제 요청/잘못된 서명 삭제 장치는 BindLoad의 caller 필수 경계로 남기며 보호 원본 삭제 기본hook을 제공하지 않는다.
