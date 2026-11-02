# BaseFile 전체 열기 부모·현재 FILE/키 (단위757)

2026-11-03, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 산출물 변경은 없다.756을 `3477db51`로 로컬 커밋한 뒤 별도 단위로 진행했다.

## 전체 출처 선택

새 `RawBaseFileOpenPolicy`는 원문 `0041b4c0`을 기존 FileSpec/Device/10칸·일반 열기 `0041a5b0`·부분파일 `0041b460`·키 `0041a620` 몸체에 연결한다. FileSpec은 부모 정상/예외 이탈까지 소유하고, 실제 열기에 넘기는 원래 이름은 FileSpec 결과로 덮어쓰지 않는다.

상대 이름은 기본 디스크→기본TAFF→보조 디스크→보조TAFF다. 디스크의 mode/create는 각 위치까지 그대로 전달한다. 찾은 TAFF의 부분파일 열기가 실패하면 그 archive 이름의 예외를 발생시키며 다른 출처의 성공으로 숨기지 않는다. 부분파일 성공 뒤 encrypted가 비영인 경우에만 고정키 `mydoghasfleas`를 설치한다.

장치 이름은 직접 디스크를 먼저 연다. 비읽기 mode의 직접 실패는 즉시 같은 이름의 예외다. mode0 실패만 기본root와의 첫 불일치 BYTE까지 원래 이름 포인터를 전진시킨 뒤 기본TAFF→보조디스크→보조TAFF를 시도한다. 구분자/전체접두나 대소문자 관문을 추가하지 않는다. UTF-8 중간 BYTE에서 잘린 OS 이름은 Windows 호환 경계에서 디스크 실패로 처리하고 뒤 TAFF의 원시 이름 조회를 보존한다.

실제 `BaseFileSystem::Open`은 같은 VFS/고리·현재 기본/보조 root와 caller 소유 FILE/키 상태를 빌린다. 호스트의 Unicode/가상CWD 변환만 장치 경계에서 수행하고 경로 고리를 추가 소비하지 않는다. 선택된 필수 장치가 없으면 앞선 쓰기를 유지하고 명시 진단한다. 원문 비복귀 예외 장치가 정상 복귀하는 것도 성공으로 처리하지 않는다.

## 등록 메타데이터와 소비자

TAFF 등록 목록에 원래 실제 파일명을 같은 인덱스로 보관한다. 이름/offset/length/xor 조회는 등록 시점 메타데이터를 반환하며 payload를 읽거나 디스크를 다시 열지 않는다. 등록 경로·아카이브·중복키의 세 vector 확보가 끝난 뒤 같은 인덱스로 확정한다. 실제 Open은 그 메타데이터로 현재 TAFF 파일을 열고 기존 부분파일/키·길이/읽기·반납을 사용한다.

객체 덤프는 재읽기와 재쓰기를 이 공통 부모에 연결했다.755의 정상5칸은 유지하고, 실제 상대 writer 실패는 보조 경로까지 시도하는6칸으로 강화했다. 별도 직접3인자 디스크 API의 절대 이름/typed 실패 계약은 유지한다.

기존 `TryRead`는 무진단 optional 미발견과 등록 후 snapshot을 유지하는 값형 API이며 `ReadFileBytes`/TAFF 등록 helper는 VFS를 적용하지 않는 OS 물리 경계다. 이들을 새 Open으로 일괄 대체하면 snapshot 계약과 등록 재진입이 달라지므로 유지했다. 명시적 실시간 읽기/원문41baa0 부모와 다른 실제 consumer는763에서 잇는다.

## 검증 범위

원문1364입력은 mode/create·각 출처 성공/실패·archive open·encrypted·고리 시작0~9·생성 재시도·키 할당 실패를 포함한다. 두 정밀도2728관측과 별도2728회 `--verify`가 같은 fixture로 일치했다. 실제41b4c0/FileSpec/Device/Format/일반열기/부분/키를 실행하고 OS fopen/fseek/free/new·archive metadata는 명시 경계다. 실패는41a0d0 진입 또는 operator_new 예외에서 멈춘다.256BYTE 예외객체·SEH unwind/실제 OS FILE ABI를 기계어 전체 반환으로 주장하지 않는다.

새 집중 검사6개는 원문15열/여섯 상태 필드·모든 장치 인자·현재 고리256BYTE를 대조한다. 실제 Unicode scratch에서는 metadata 비I/O, 등록 뒤 바뀐 현재 암호 payload, 부분파일 열기 실패의 즉시 중단, 상대mode2의 보조 성공/직접mode2 실패 무fallback, FileSpec의 같은 검색 핸들 예외 반납을 확인한다. 기존 객체덤프 검사와 값형 VFS 회귀도 같은 새 라이브러리로 확인한다. 최종 수치/실제 로그·SHA는 `cpppj/basefile-open-policy-20261103.json`과 `extracted/cpp-resume-757/`을 따른다.

최종 Release 제품/집중 대상 빌드는 경고/오류0이다. FILE/새부모/덤프17개와 기존파일27개, 서로 다른44개가 모두 통과했다. 실패 이름의 `unable to open` 출력은 명시 실패 입력의 원문 진단이며 검사 실패가 아니다. 새 대상은 만들지 않고 기존 FILE 집중 대상에 새6개를 추가했다.

실행 제품757의 EXE SHA-256은 `e67b4ceb13be995fd0fb39f60aa5dfbb1428c18cfbff20c3acad6555039ef3fd`이다. 기존 보호2666개는 모두 불변이고 새 fixture1개를 더한 현재 기준은2667개/fixture847이다. 다음 기준은 `extracted/cpp-resume-757/protected-after.json`이다.

## 남은 경계

MSVC32의 FILE24BYTE·FileSpec264BYTE·예외payload256BYTE와 SEH unwind ABI를64비트 호스트 포인터/표준 소유자로 복제했다고 주장하지 않는다. 파일명 metadata의 host 소유 복사와 std::function 할당도 원문 allocator ABI와 다르다. 실제 부분I/O/닫기 반환 실패와 모든 할당 실패 수명, 다른 consumer의 실시간 이관은 별도다. 이번 새 API를 MP 전체에 이관하지 않았다.

CD10.72의 일반 열기 `004ba5d0`/부분파일 `004ba630`은 기존652의 정적 대응을 유지하며 자동 정밀 후보의 반대 매핑을 따르지 않는다. 이번 실행 관측은10.78에 한정한다.

새 게임/GUI·독립 CLI·물리/PCM·공중3종 전용 행동·포획/희생·특정 적 공격·승리·자연 완주·MP·전체 C++/34/ZIP 검사는0이다. 마지막GUI746·전체34/ZIP645·644는 별도 제품 근거로 유지한다. 다음은758~764 준비본과 남은 비멀티/호환성이고 전체 목표는 미완료다.
