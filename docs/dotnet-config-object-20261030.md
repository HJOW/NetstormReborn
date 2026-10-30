# dotnetpj 설정 객체 층 이식 — 2026-10-30

2026-10-30, HJOW-Athlon. 사용자 요청("AGENTS.md, LEFT_JOBS.dotnetpj.md 를 읽고 dotnetpj 다음 작업 진행해 줘")으로
R3k 다음 순서(전체 ConfigInterface 조립)를 재개했다. 시작 HEAD는 `b1c2261d`이고 작업 트리는
깨끗했다(병렬 CP·WB 세션의 cpppj·webpj·LEFT_JOBS.md 변경은 건드리지 않았다). 현재/마지막 전체 정밀 디컴파일 PC는
HJOW-Athlon이다. cpppj `o/Config.h/.cpp`와 `ConfigOracleTests.cpp`를 읽고 설정 객체 층을 이식했다.
게임/창 실행과 디컴파일은 하지 않았다.

## 1. 이식 범위

| C# 파일 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `Original/Config.cs` (신규) | `o/Config.h/.cpp` 전체 | `ConfigText`(버퍼·XOR·따옴표 판독)·`ConfigRegistry`(등록·이름 조회·환경/저장소/미션 갈고리)·`Config`(버퍼·치환·조회·쓰기·섹션·변경 표시)·자유 함수(`ConfigFindKey`·`ConfigGetRaw`·`ConfigFindSection`) |

- `ConfigText`·버퍼·치환·섹션 규칙은 원본 그대로 옮겼다. 고정 길이 버퍼(패치 8KB·CD 4KB)는 C# 동적 문자열로 바꿨다.
- 바이트 단위 해석은 Latin1 1문자 1바이트로 보존한다. `DecodeOriginalText` 자리는
  `OriginalText.DecodeWindows1252`와 바이트 단위로 같음을 대조했다(UTF-8 유효성 검사를 하는 `Decode`가 아니라 byte→코드포인트→UTF-8인코딩 쪽이다).
  XOR 키(`mydoghasfleas`)는 기존 `XorCipher.Apply`를 쓴다.
- `FindSection`은 원본의 `matchedName` nullptr 분기를 지킨다. 이름이 필요 없을 때는 `]` 탐색을 건너뛰고 위치만 계산하는
  별도 overload(`FindSection(text, from, header)`)를 두었다. 이름 있는 overload와 위치가 달라지는 경우를 fixture로 확인했다.
- 기존 `Assets/ConfigText`·`ConfigFile`·`ConfigStore`와 이름·역할이 겹친다. 이번 단위에서는 손대지 않는다.
  `Config` 쪽이 R4 세션 조립의 소유 구조가 되며, 기존 판독기와의 재대조(0.4 판정표)는 ConfigInterface 단위에서 함께 다룬다.
- `EncodeOriginalText`(저장 인코딩)는 `ConfigInterface.SaveText` 쪽이라 이번 범위에서 제외한다.

## 2. fixture 연결

| fixture | 행 | 근거 |
| --- | --- | --- |
| `config-x86.tsv` (기존 등록, 소비자 `Assets`) | 2440 | 치환·조회·삭제·쓰기·섹션머리·섹션 6종의 원본 재생. 새 등록 없이 기존 `Link`·manifest 항목을 그대로 쓴다. reflected-commit은 유지한다 |

- `PORT_MAP.tsv`에 `Config.h/.cpp`→`Config.cs` 2행을 `L1`로 추가했다. `--ports` 기준 바뀐 쌍은 40개(`L1` 26·`port` 14)이며 이번 단위와 무관하다.
- `options` 16행(`ConfigSave`)은 `ConfigInterface.SaveText`가 필요해 다음 단위로 미룬다.
- 검사 위치는 fixture 소비자 규칙에 따라 `Assets.Tests`에 두었다(`X86ConfigObjectTests`). 대상 클래스(`Netstorm.Core.Original.Config`)만
  Core에서 빌리며, 이를 위해 `Assets.Tests`에 Core 프로젝트 참조 1줄을 추가했다. 테스트 프로젝트→제품 프로젝트 참조라 순환은 없다.

## 3. 검증

- 새 검사 `X86ConfigObjectTests.ConfigObjectLayerMatchesNative` 1개가 2440행 전부(치환 495·조회 77·삭제 272·쓰기 143·섹션머리 1045·섹션 408,
  Def 2행 제외)를 통과했다. 기대값을 C# 결과로 바꾸지 않았다.
- 검사 작성 중 실패 4건은 모두 테스트의 오류였고(조회 오프셋·쓰기 오프셋·섹션 위치 계산·`out` 변수 확정, 소스 수정 없음) 원본과 대조해 고쳤다.
  섹션 1건은 제품의 오류처럼 보였으나(`FindSection` 이름 계산 분기) 원본 nullptr 분기의 재현이었고 overload 분리로 원본과 일치시켰다.
- 변이 1건(`Set`의 변경 표시 제거)에 L1 기대값 실패를 확인하고 원복했다.
- 전체 **2,004개(Assets 364 + Core 1030 + Client 610) 통과**, xUnit 실패/건너뜀 0, Release 경고/오류 0이다.
- `HtmlTextParserTests`·`HtmlTextParserNativeTests` 2개 파일에 기존 `Assets.ConfigText` 지칭용 using 별칭 1줄씩을 추가했다.
  새 `Core.Original.ConfigText`와 이름이 겹쳐 생긴 모호성 해소이며 동작 변경은 없다.
- Git 제외 로그: `extracted/dotnet-config-object-*.log`에 빌드·집중·전체 검사 로그를 남겼다.

## 4. 남은 것

- `ConfigInterface`(Startup·GetInt·PathSpec·ReadInt/ReadString·IntEquals·Set/SetInt·SubstituteLocal·LoadLanguage·SaveText·SaveFile)와
  `ConfigInterfaceTests` 4종(시작 순서·타입 접근·미션 조회·ParseLong/ListItem은 기존 `ConfigNumbersTests`로 이미 대조),
  `ApplyScriptConfig`의 `ConfigInterface` 직접 연결과 두 번째 검사 상당(`ConfigOverwriteAndQueueLifetime`).
- `ParseDialogPage` 명령 추출·HTML 합성은 대화창 조립 단위에서 잇는다.
- 목록 owner·메시지/대화창·상위 메뉴/전체 자산/호스트 조립과 R3 전체 완료는 남는다. 전체 목표를 완료로 세지 않는다.
- cpppj·원본·CD·AGENTS.md·공통 LEFT_JOBS.md·webpj 변경은 없다.
