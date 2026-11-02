# 디스크 목록의 FileSpec·가상 게임 CWD 연결 (단위752)

2026-11-02~03, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 이번에는 디컴파일 산출물을 변경하지 않았다. 작업 전에 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽었고 원격 fetch를 수행했다. 다른 세션의 dotnetpj 변경은 이 작업에 포함하지 않는다.

## 수정한 계약

원문10.78 `00459610`의 FileSpec 재대입과 `004597c0`의 다음 검색을 기존 `FileSpecQuery`가 이미 복원하고 있었다. 그러나 `BaseFileSystem::MatchDisk`에는 별도 경로 정규화/Win32 검색이 남아 있어 선행 루트·UNC·비패턴 접두를 제거하고, '/' 패턴의 결과에도 하위 폴더 접두를 합성했다. 이번 수정은 별도 구현을 제거하고 같은 FileSpec 객체의 생성→현재 문자열 복사→Next→소멸 수명을 사용한다.

비패턴은 실제 파일이 없어도 한 이름을 반환한다. 첫 NUL까지만 사용하고, 원래 입력에 backslash나 colon이 없으면 `.\`를 붙인 뒤 '/'를 backslash로 바꾼다. UNC·루트·드라이브 입력을 상대 이름으로 재해석하지 않는다. 패턴은 정규화된 문자열이 아닌 원래 입력으로 FindFirst를 호출하고, 원래 입력의 마지막 backslash까지만 결과 앞에 보존한다. 따라서 `d/*.bin`의 결과는 파일명만이고 `d\*.bin`은 `d\파일명`이다. Next 실패 뒤 마지막 문자열/검색 핸들의 수명도 공통 FileSpec에 맡긴다. 디렉터리 후보와 OS 순서는 그대로 둔다.

`FileSpecDirectoryHooks`는 원본 게임의 chdir 상태를 현재 VFS의 명시 게임 폴더로 대체한다. 상대 패턴과 같은 드라이브의 드라이브 상대 이름은 그 폴더에서 검색하며, 루트 상대 이름은 게임 드라이브의 루트를 사용한다. 절대 드라이브·UNC 이름과 다른 드라이브 상대 이름은 그대로 OS에 전달한다. 프로세스 CWD를 바꾸지 않고 반환 FileSpec에 게임 폴더를 덧붙이지 않는다. 비패턴에는 폴더 절대화나 OS 검색을 시작하지 않는다. 원문의 ANSI 대신 W API를 사용하는 이유는 Windows10/11의 한글·보충 평면 경로 호환이며, 검색/Next/Close는 같은 핸들을 사용한다.

현대 VFS와 편집 장부가 허용한 '/' 입력은 `MatchFinder`·`EditorFiles`·`EditorCatalogWorkspace`·독립 `SoloMapCatalog`의 호출 경계에서 native 구분자로 바꾼다. 특히 편집의 절대 작업 경로는 FileSpec이 폴더 접두를 보존하도록 전달해야 하며, 기존 canonical 경계·링크 거부·원본 보호·장부 키는 유지한다. 원문 FileSpec 자체에 현대 '/' 반환 규칙을 섞지 않는다. 기존 맵 목록 검사의 두 직접 열거 입력도 실제 native 경계에 맞춘다. 원문 fixture의 기대 바이트는 바꾸지 않았다.

## 검증

실제 실행 이름·수치·로그와 산출물 SHA는 `cpppj/file-spec-disk-20261103.json` 및 Git 제외 `extracted/cpp-resume-752/`를 따른다. 기존 집중 대상에 `SoloMapCatalogTests.cpp`를 연결했으며 새 실행 대상은 만들지 않았다.

새 집중 검사는 비패턴10입력의 명시 접두·UNC·루트·빈 이름·첫NUL·드라이브 상대 계약을 확인한다. 자체 임시 폴더의 한글·이모지 이름과 디렉터리 후보는 제품의 FileSpec을 사용하지 않은 별도 Win32 열거와 순서를 비교한다. 기본 게임 폴더가 C드라이브이고 프로세스 CWD가 H드라이브인 상태에서 상대·절대·루트 상대·같은 드라이브 상대 검색을 확인하고, CWD/공유 파일명 고리의 비소비도 확인한다. UNC 비패턴은 서버 접속이나 존재 조회를 하지 않는다.

기존 `decomp_startup_archives.py --verify`의54+16행/140관측과 `decomp_config_path_load.py --verify`의12행/24관측은 기존 fixture와 일치했다. 실제 원문 FileSpec/재대입/Next/소멸과 전체 고리를 재실행하며, OS FindFirst/Next/Close와 등록 등은 기존 도구의 명시 장치 경계다. 새164개의 독립 fixture를 만들었다는 의미가 아니며, 실제 Windows 경로/파일 검사는 C++ 집중 검사에서 따로 수행한다.

최종 Release 빌드는 경고/오류0이다. ring60개·editor45개·search27개의 실행132회에서 서로 다른100개가 모두 통과했다. 새2개와 기존98개이며, 같은 BaseFile/편집 파일 검사가 여러 집중 대상에 함께 포함된 실행을 새 성공 수로 중복 집계하지 않는다. 실제 맵 목록 검사는 보호 Fort433개의 분류/캐시와 현재 writer·Config/Template·언어 경로를 포함하지만 게임이나 시나리오 전투를 실행하지 않는다.

실행 제품752의 EXE SHA-256은 `f467d9b4261f8a19280306e6586bd7bc615e29da4d32ae0116d5340254a03f6d`이다. 기존 보호2662개(AGENTS1·원본1394·CD425·fixture842)는 모두 불변이며 새 fixture는0이다. 다음 보호 기준은 `extracted/cpp-resume-752/protected-after.json`이다.

## 남은 범위

전체264BYTE FileSpec/FILE·예외 객체 ABI와 다른 쓰기/키/스트림 소비자, 실제 부분I/O/할당 실패 수명은 남는다. UNC 실제 서버와 다른 드라이브의 per-drive CWD는 이번 검사 대상이 아니다. CD10.72는 `0049c6d0` 재대입과 `0049c880` Next가 정적 대응하며, 원래 패턴 FindFirst/마지막 backslash 접두와 +100 핸들/+104 유효값을 같은 방식으로 사용한다. 이번 독립 실행은10.78에 한정하므로 CD의 전체 FileSpec 복원/관측으로 주장하지 않는다.

새 게임/GUI·독립 CLI·물리 키/PCM·공중3종 전용 행동·포획/희생·특정 적 공격·시나리오 승리·자연 완주·멀티플레이 검사는0이다. 마지막GUI746, 전체34/ZIP645·644, 전체 C++ 마지막 실행549는 별도 제품의 근거로 유지한다.

다음 원문 대조에서 Pause 키의 정지 앞 큐 예약과 T/t 게임 시간 표시의 같은 Client 시계·메시지 소비자 누락을 확인했다. 각 항목을 독립 단위로 이어 가며, 나머지 비멀티 UI·시나리오·파일·수명·호환성과 후순위3~8차를 계속 진행한다. 전체 목표는 미완료다.
