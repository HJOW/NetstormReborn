# webpj 원문 경로 이름 작성기(RawFileName) 이식 — WB04-E28

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 스물여덟 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawFileName`(10.78)을 옮겼다. 베이스 파일 IO·옵션 저장·VFS 읽기·CD 경로·시작 파일 이름이 모두 같은 10칸 고리를 이어 쓰므로, 이 작성기는 그 소비자 단위의 공통 바탕이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-file-name.js` — `RawFileName`과 `FileNameView`. OS 경로나 유니코드로 다시 해석하지 않는 원본 바이트 작성기이다.

- 256바이트 칸 10개의 고리. 모든 호출은 번호를 먼저 1 올리고(10 나머지) 그 칸을 0으로 비운 뒤 쓴다. `index`는 조회만 하며 고리를 전진시키지 않는다. `currentBytes()`는 NUL 뒤 남은 바이트까지 현재 칸 256바이트를 빌린다.
- 반환값은 사본이 아니라 칸의 한 구간을 가리키는 `FileNameView`(`slot`·`offset`·`length`·`text`)이다. `text`는 항상 현재 바이트를 읽어 열 번째 후속 호출이 같은 칸을 덮으면 내용도 바뀐다(원문 포인터와 같음). 이전 뷰를 입력으로 되넘기면 출력 칸을 먼저 비운 다음 그 뷰의 현재 바이트를 읽는다(원문 별칭 동작).
- `format(parts, separator)` [459c60]: 부품 `{kind, text}`(-1 루트·-2 디렉터리·-3 파일·-4 확장자, 0이면 그 뒤는 보지 않음)를 순서 규칙대로 잇는다. `/`·`\`는 현재 구분자 바이트로 치환하고, 선행 같은 문자는 제거하며 후행은 마지막 한 문자만 지운다. 고정 접두 `.\`와 연결 문자 `\`는 현재 구분자와 무관하게 역슬래시이다. 부품 순서 위반은 `TypeError`이고 그때까지 쓴 부분 출력과 번호는 보존한다.
- `device(fileSpec)` [459900]: UNC면 서버 끝, 아니면 콜론 다음(없으면 0)까지의 장치 접두를 반환하고 전체 복사본의 해당 위치에 NUL을 쓴다. NUL 뒤 원래 FileSpec 바이트는 칸에 남는다.
- `name(fileSpec, extension)` [459b20]: 전체를 칸에 복사한 뒤 마지막 구분자/콜론 다음 내부 뷰를 반환(없으면 빈 뷰). `extension=false`는 선두 dot이 아닌 끝 확장자만 지운다.
- `directory(fileSpec, level)` [459990]: 전체(-1)/단계별 디렉터리를 반환한다. 파일 시스템의 `parent_path`와 다른 dot/UNC/단계 경계를 가지며, 원문 NULL 읽기는 현재 칸(부분 버퍼)을 보존한 `TypeError`로 거부한다.
- `full(fileSpec, prefix)` [459890]: 같은 칸을 비운 뒤 전체 또는 무조건 앞 두 바이트를 건너뛴 나머지를 첫 NUL까지 복사한다.
- 256바이트 밖 쓰기는 절단/성공으로 합성하지 않고 `RangeError`로 거부한다(원문은 배열 밖 미정 쓰기).

## 2. 검증 (`tests/file-name.test.mjs`, 8건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음) 656행:

- `file-name-formatter-x86.tsv` 352행: 부품 순서 규칙·구분자 치환·접두/연결 문자·예외 뒤 부분 출력과 번호·현재 칸 256바이트 전체.
- `file-name-leaf-x86.tsv` 64행(`name`: extension 0/1·두 구분자·bare/빈/콜론/UNC/선두 dot/255바이트의 반환 위치와 칸 전체), `file-name-device-x86.tsv` 32행.
- `file-directory-x86.tsv` 192행(정상 162 + 원문 NULL 읽기 30의 부분 버퍼), `file-full-x86.tsv` 16행(flag 0/1·+2 물리 입력).

cpppj 계약 검사 이식: 열 칸 고리 순환과 입력 별칭의 선행 clear, 256바이트 초과 거부와 앞선 칸 보존, 디렉터리/전체의 별칭 clear·NULL 읽기·+2 접두 거부, 부품 순서 위반 `TypeError`와 종료 0 이후 무시.

최초 실패와 교정: 원본 관측 재생은 처음부터 모두 통과했다. 처음 쓴 `directory` 계약 검사의 기대값(`d\a\b\file.dat` → `d\a\b\`)은 제가 추측한 값이었고 틀렸다(실제 원문은 첫 구분자부터의 접미를 쓴다). 추측 기대값을 지우고 같은 입력 192행의 원문 관측 재생과 cpppj의 별칭/범위 계약 검사로 바꿨다. 모듈은 그대로이다.

## 3. 경계

- 실제 Client 옵션 저장(`SaveClientOptionsInGameDirectory`)·VFS 읽기/Probe/ReadOnly·CD 경로(`MakeWindowsCdPresenceHooks`)·`SavedStateFilePath`·`ModuleDirectory`·`PortableGameDirectory`가 같은 고리를 이어 쓰는 검사와 `ClientStaticStrings` 소유자는 해당 소비자 단위를 옮길 때 대조한다. 그 소비자들은 이 클래스를 그대로 빌려 쓴다.
- `file-directory-x86.tsv`에 관련된 모듈 부모(`file-module-directory-x86.tsv`의 무인자 5행)와 CD 디렉터리(`cd-directory-x86.tsv`)는 해당 단위에서 한다. CD 10.72는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등). 이제 이 고리를 쓰는 `RawBaseFileOpenPolicy`·`ConfigFileRead`·`StartupFiles` 계열 소비자와, 작은 후보 `RawEnergySupply`·`RawTotalMade`·`RawMemStream`을 옮길 수 있다.
