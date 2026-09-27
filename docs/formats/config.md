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
