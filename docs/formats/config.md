# 설정 파일 및 색상 테이블

> 분석 상태: **포맷 확인 완료**, 개별 키의 의미는 일부만 확인

## `d/options.cfg`, `d/setup.cfg` — 인코딩된 설정 파일

* 파일 전체를 TAFF 와 같은 XOR 키 `"mydoghasfleas"` 로 인코딩한다 (키 위치는 파일 시작 기준 0부터 순환, [taff.md](taff.md)).
* 복호화한 평문은 5바이트 서명 **`mQdsT`** 로 시작하고, 그 뒤는 텍스트다.
* 원본 텍스트 인코딩은 Windows-1252 (Latin-1 범위), 줄바꿈 CRLF.

### `options.cfg` (사용자 옵션, 게임이 기록)

형식: `키 = "값"` 한 줄에 하나. 확인된 키:

| 키 | 예시 | 의미 |
|---|---|---|
| `InstallDir`, `CDDir` | `"I:\...\originals"` | 설치 경로 |
| `SCREENW`, `SCREENH` | `"1024"`, `"768"` | **화면 해상도** (원본은 640×480 고정이 아님) |
| `startInFullScreen` | `"1"` | 전체화면으로 시작 (아래 "전체화면 재실행 오류" 참고) |
| `workingFullScreenFlags`, `windowScreenFlags`, `screenModeFlags` | | 화면 모드 플래그 (전체화면 / 창 모드) |
| `soundVolume`, `musicVolume` | `"3"`, `"2"` | 음량 |
| `currentLanguage` | `"english"` | 언어. 미션·번역 파일 확장자 선택에 사용 ([xlat.md](xlat.md)) |
| `intro`, `welcomeGiven`, `tipNumber` | | 첫 실행·팁 진행 상태 |
| `Done<미션이름>` | `DoneTutorial1 = "1"` | 미션 완료 기록 (미션 스크립트의 `<$Config,Done{mission.filename}=1>` 가 기록) |
| `lastMultiplayerFort`, `curServer`, `registerdSubId` | | 멀티플레이 관련 |
| `useIPAsLanProtocol`, `hasIP`, `hasIPX` | | 네트워크 프로토콜 |

### 전체화면 재실행 오류 (AGENTS.md 1.4절 관련)

시작 코드(`Clientmain.cpp`, 디컴파일 40714행 부근) 흐름:

1. `startInFullScreen` 이 참이면 `workingFullScreenFlags` 모드로 화면 초기화 시도
2. 실패하면 **초기화 함수가 그대로 반환되어 게임이 시작되지 않는다**
3. 거짓이면 `windowScreenFlags` 모드(창 모드)로 초기화

옵션에서 전체화면으로 바꾸면 `startInFullScreen = "1"` 이 저장되고, 다음 실행 시 Windows 10/11 에서 DirectDraw 독점 전체화면 초기화가 실패해 오류가 나는 것으로 추정한다.
오류 후 창 모드로 돌아오는 것은 실패 처리 과정에서 플래그가 초기화되기 때문으로 보인다 (동적 분석으로 확인 필요).
**복구 방법(추정)**: `options.cfg` 를 복호화해 `startInFullScreen = "0"` 으로 고친 뒤 다시 인코딩한다.
원본 로그 메시지에도 "init screen mode; if this fails, set the "screenModeFlags" in options.cfg to 0" 라는 안내가 있다.

### `setup.cfg` (패치판 서버·클라이언트 설정)

`키=값` 형식, `//` 주석 허용. 패치판 버전(`gamemaster="10"`, `gameminor="78"`), 채팅·메뉴 색상(팔레트 인덱스), 요새 자원 이름, UI 서식 코드(`~w`, `~r`, `~B~E` 등) 등을 담는다.

## 설정 조회 규칙 (Config.cpp 정적 분석, 2026-09-28)

클론 구현: `src/Netstorm.Assets/ConfigText.cs` (조회), `ConfigStore.cs` (층·치환)
실행 프로젝트 연결: [`GameResources`의 설정 구성](../runtime-resources.md) — options → setup을 한 텍스트에
이어 붙여 첫 일치 우선 규칙을 유지하고, 언어 용어표·선택 언어·미션 머리 값 공급자를 연결한다.

### 파일 읽기 (FUN_00440380)

* 파일의 **첫 바이트와 세 번째 바이트가 0** 이면 XOR 인코딩된 파일로 보고 복호화한다 (`"mQdsT"` 와 키 `"myd…"` 의 XOR 결과가 `00 ?? 00`).
  그렇지 않으면 평문 그대로 쓴다 → `config.english` 같은 용어표는 평문이다.
* 읽은 텍스트는 설정 객체의 버퍼 **끝에 이어 붙인다** (FUN_0043ffa0). 파일마다 `[이름]` 섹션 줄이 앞에 붙기도 하지만 조회에는 영향이 없다.

### 시작 시 불러오는 순서 (Configinterface.cpp, 디컴파일 46,860행 부근)

명령줄 인자 → `MAJOR_VERSION=<주>;MINOR_VERSION=<부>;version="v<주>.<부>"`(`;` 는 줄바꿈으로 바뀜) → `<IDENTITY 또는 USER>.cfg` → `user.cfg` → **`options.cfg`** → `dev.cfg` → `guild.cfg` → **`setup.cfg`**.
그 뒤 `InstallDir`, `CDDir` 를 설정한다(기존 줄을 모두 지우고 끝에 추가).

### 키 조회 (FUN_0043fd70, FUN_00440150)

* 버퍼를 앞에서부터 줄 단위로 훑는다. 줄 앞 공백·탭을 건너뛰고, **키 길이만큼 대소문자 무시 비교** → 공백·탭 → `=` 이면 일치.
  * 키에 공백을 넣을 수 있다 (`Quick Help Spec`, `a factory`, `crash HTML`).
  * `//` 로 시작하는 줄은 키와 비교가 실패하므로 자연히 무시된다. 섹션 `[..]` 구분은 없다.
* **처음 일치한 줄을 쓴다.** 먼저 불러온 `options.cfg` 가 나중에 불러온 `setup.cfg` 의 기본값을 덮어쓰는 구조다.
  (이전에 "마지막 값 우선"으로 기록했던 것은 틀림 — `ConfigFile.Get` 도 첫 일치로 수정함)
* 값: `=`·공백·탭을 건너뛴 뒤 줄 끝(CR/LF)까지. 따옴표 밖의 `//` 부터는 주석.
  큰따옴표는 따옴표 모드를 켜고 끄며 값에서 빠진다. 단 바로 앞 글자가 `` ` `` 이면 `` `" `` 두 글자가 그대로 남는다.
* 값 쓰기(FUN_004404e0): 같은 키의 줄을 **모두 지운 뒤** 끝에 `키 = "값"` 을 추가한다.

### 설정 객체 층 (FUN_0043fc20 / FUN_0043fcd0)

* 설정 객체는 전역 배열(최대 1000개)에 쌓이고, 조회(FUN_00440760)는 **마지막에 쌓은 것부터** 한다.
* 이름 있는 객체(예: `local`)는 이름 뒤에 `.` 을 붙여 접두어로 쓰며, `local.1` 처럼 접두어가 맞는 키만 받는다.
  경로 지정값 치환(FUN_004410f0)은 `local` 객체에 `1="인자1"`, `2="인자2"`, `3="인자3"` 를 넣고 키를 치환한 뒤 걷어 낸다.
* 언어 용어표는 `languageSpec = "{DataDir}\config.{local.1}"` 로 따로 불러온다 (FUN_00441db0, 언어를 바꿀 때마다 다시 읽음).

### 치환 규칙 (FUN_00440b60, FUN_00440a00, FUN_00440760)

특수 문자 (VA 0x532538): `` ` `` 이스케이프, `{` `}` 치환 괄호, `|` 기본값 구분, `[` `]` 섹션, `.` 객체 접두어.

| 표기 | 결과 |
|---|---|
| `{키}` | 값을 찾아 **다시 치환**한 결과. 키 안의 `{..}` 는 먼저 치환한다 (`{{which}}`) |
| `{키\|기본값}` | 키가 없으면 기본값을 치환한 결과 (기본값이 비어 있으면 없는 것과 같음) |
| 둘 다 없음 | `{Not Found:키}` — 키는 **대문자**로 표시된다 (최상위 조회 API 는 빈 문자열) |
| `{@미션.키}` | `missionSpec` 으로 해당 미션 스크립트를 임시 설정 객체로 읽어 키를 찾는다 |
| `{&레지스트리 경로#값}` | Windows 레지스트리 조회 (URL 실행기 설정용, 클론은 지원하지 않음) |
| `{_환경변수}` | 환경 변수 (특정 플래그가 켜진 설정 객체에서만) |
| `` `n `` `` `r `` `` `t `` | 줄바꿈 / CR / 탭 |
| `` `그밖의문자 `` | 그 문자 자체 (`` `" `` → `"`, `` `{ `` → `{`) |

* 치환 결과 끝의 공백·탭은 잘라 낸다 (첫 글자는 남김).
* 자기 자신을 참조하는 값은 원본에서 무한 재귀가 된다 → 클론은 중첩 32단계에서 멈추고 "찾지 못함"으로 처리한다.
* 예: `missionSpec = "{DataDir}\{local.1}.{currentLanguage}"` 에 `local.1 = tutorial1` → `\D\tutorial1.english` ([vfs.md](vfs.md) 의 조회 순서로 연다)

## `d/!rootservers.dat` — 온라인 서버 목록 (평문)

한 줄에 `호스트:포트;이름;0`. 원본 서버는 현재 운영되지 않는 것으로 추정.

## `d/!color.dat` — 색상 변환(리맵) 테이블

* 2304 바이트 = **256바이트 테이블 9개**. 테이블 `t` 의 `i` 번째 값은 팔레트 인덱스 `i` 를 변환한 인덱스.
* 확인된 테이블:

| 번호 | 특징 | 추정 용도 |
|---|---|---|
| 0 | 불규칙 | 미상 (플레이어 색 또는 효과) |
| 1, 2 | 대부분 어두운 색으로 매핑 | 그림자 / 어둡게 |
| 3, 7 | 불규칙 | 미상 (색조 변환) |
| 4 | 항등 (`t[i] = i`) | 변환 없음 |
| 5 | 전부 0 | 검정 실루엣 |
| 6 | 전부 0xFE | 단색(흰색 추정) 실루엣 — 선택·피격 깜빡임 등 |
| 8 | 불규칙 | 미상 |

정확한 용도는 실행 파일 분석(4단계)에서 확인한다.

## 게임 팔레트

**`d/GIFCLOUD.COL`** — `setup.cfg` 의 `fortPal`/`battlePal`/`chalPal = "gifcloud"` 와 `GamePalSpec = "{DataDir}\{local.1}.COL"` 로 지정된다.
자세한 내용은 [shp.md](shp.md) "색상" 참고. 타이틀 화면은 `TITLE00.COL`, `TITLE01.COL`, `TITLE06.COL` 을 쓴다.

## 도구

[`tools/nscfg.py`](../../tools/nscfg.py): `show`(복호화 출력), `set <키> <값>`(백업 후 값 변경), `decode`/`encode`(텍스트 파일과 상호 변환).
복호화 → 재인코딩 결과가 원본과 바이트 단위로 같음을 확인했다.
