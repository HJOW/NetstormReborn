# 클론 게임 데이터

`game-data/`는 클론에서 직접 관리하는 실행 데이터 소스다. 분석 중인 Netstorm 10.78의 `originals/`에서 필요한 파일을 **일반 파일로 복사**해 분리했다. 원본을 가리키는 심볼릭 링크나 빌드 중 원본을 복사하는 단계는 없다. `originals/`는 원본 분석이 끝날 때까지 별도로 유지한다.

| 경로 | 내용 |
| --- | --- |
| `game-data/netstorm.tarc` | 타입 정의·기본 지도·미션·언어표 246개를 담은 TAFF 데이터 아카이브 |
| `game-data/d/` | 1,102개 파일: 셰이프, 팔레트, 메뉴 GIF, 지도 432개, 영어·독일어 미션/도움말, 글리프 자료, 기본 설정. 2026-10-03에 원본 분석용 커스텀 맵 `TEST01.fort`·`TEST01.english`(`originals/d/`의 복사본)를 더했다 — `--test-battle TEST01`과 통합 테스트가 쓴다 |
| `game-data/sound/` | 효과음 WAV 218개 |
| `game-data/music/` | RIFF WAV 형식의 음악 MUS 9개 |
| `migration-manifest.json` | 최초 이관한 1,327개 파일의 상대 경로·크기·SHA-256 |

이관 데이터는 약 **152 MB(145 MiB)**다. 현재 메뉴에서 잠긴 미션의 지도·스크립트·소리도 이후 구현에 사용할 수 있도록 보존했다. 원본 프로그램(`Netstorm.exe` 등), DLL, 설치/패치/압축 도구, 서버 PHP, 설치 도움말 PDF/HLP, 원본 사용자 등록·설치 경로·캠페인 완료 기록은 클론 데이터에 포함하지 않았다. TAFF는 실행 코드가 없는 데이터 아카이브다.

셰이프·팔레트·지도·기존 텍스트의 포맷과 인코딩은 로더 호환을 위해 보존했다. `d/options.cfg`는 원본 사용자 설정을 복사하지 않고 UTF-8 클론 기본 파일로 만들었다. 클론 화면·오디오 설정은 사용자 설정 폴더의 `settings.json`에서 관리한다. 글꼴은 별도 `fonts/` 소스를 사용한다.

## 빌드와 배포

`src/Netstorm.Game/Netstorm.Game.csproj`가 이 폴더의 모든 실행 데이터를 개발용 출력과 `dotnet publish` 출력의 `game-data/`로 복사한다. 필수 아카이브·셰이프·기본 팔레트가 없거나 실행 파일·DLL·PHP가 섞이면 빌드 오류를 낸다. 실행 파일과 `game-data/`, `fonts/`, 함께 생성된 런타임 파일을 포함한 출력 폴더 전체를 배포한다. 명령은 [루트 README](../README.md)에 있다.

실행 시 유효한 `NETSTORM_DATA`를 먼저 확인하고, 그다음 실행 파일/현재 작업 폴더 및 상위의 `game-data/`, `assets/game-data/`를 찾는다. 분석용 `originals/`를 자동으로 찾지 않는다. 별도 데이터를 사용할 때만 환경 변수를 지정하면 된다.

## 수정과 검증

이후 클론 자산·지도·언어 변경은 `game-data/`에서 관리한다. `migration-manifest.json`은 **최초 이관본의 출처 기록**이며 이후 수정본을 원본으로 되돌리거나 동기화하는 도구가 아니다. 원본 분석 도구의 `originals/` 경로와 원본 분석 문서는 계속 유지한다.

Assets·Core 통합 테스트도 이 데이터를 사용하며 누락 시 실패한다. 원본 exe를 읽던 다리 표 회귀 검사는 독립적으로 추출한 [고정 JSON 자료](../tests/Netstorm.Core.Tests/Fixtures/bridge-tables.json)로 대체했다. 자료에 원본 exe의 SHA-256과 추출 주소를 기록했으며, 클론 구현에서 기대값을 다시 생성하지 않는다.
