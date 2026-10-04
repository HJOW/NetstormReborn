# 원본 다국어 체계 (`xlat.*`, `config.*`, 언어별 미션 파일)

> 분석 상태: **포맷 확인 완료** — 12단계(다국어) 설계의 기반 자료

원본 게임은 영어 문자열을 코드에 하드코딩하고, 실행 시 언어별 번역 테이블로 치환하는 구조다.
현재 언어는 `options.cfg` 의 `currentLanguage` (예: `"english"`) 로 정해지며, 이 값이 곧 파일 확장자다.

## 1. `xlat.<언어>` — UI 문자열 번역 테이블

위치: TAFF 아카이브 `\d\xlat.<언어>` (english 는 빈 자리표시자 `This file is a simple placeholder`).

| 언어 | 블록 수 | 고유 원문 수 |
|---|---|---|
| german | 871 | 774 |
| french / spanish / portuguese | 746 | 664 |

블록 수와 고유 원문 수가 다른 것은 `NEW TRANSLATIONS` 구간과 `PRESERVED TRANSLATIONS` 구간에 같은 원문이 다시 들어 있기 때문이다
(예: `Unrecoverable Exception!`, `None`). 원본은 뒤에 나온 번역으로 덮어쓴다.

형식 (텍스트, Windows-1252):

```
// 번역자용 주석 (선택)
***
영어 원문 (여러 줄 가능)
---
번역문 (여러 줄 가능)
===
```

* 파일 머리의 `oCompXlat.pl output: ...`, `+++++++++++ NEW TRANSLATIONS +++++++++++++`, `+++ PRESERVED TRANSLATIONS +++` 줄은 제작 도구가 남긴 구분선이다.
* 원문에는 `printf` 서식(`%s`, `%d`)과 게임 서식 코드(`~r`, `~.`, `~w` 등: 색상/스타일)가 들어 있으며, 번역문도 같은 서식 코드를 유지해야 한다.
* 원문 문자열이 곧 키다 → **독일어 파일의 고유 원문 774개가 번역 대상 UI 문자열 목록**이 된다 (패치판에서 추가된 문자열은 빠져 있을 수 있음 — 실행 파일 문자열과 대조 필요).

### 원본 해석 규칙 (Xlat.cpp 정적 분석, 2026-09-28)

클론 구현: `dotnetpj/src/Netstorm.Assets/XlatTable.cs`

* 파일 경로: `d` + `xlat.<언어>` (파일 조회 순서는 [vfs.md](vfs.md)). 읽은 뒤 파일 전체에서 **CR 을 지운다**.
* 줄의 **첫 글자**만 본다 (FUN_004de260):
  1. `*` 로 시작하는 줄을 찾으면 다음 줄부터 원문
  2. `-` 로 시작하는 줄 앞까지가 원문, 다음 줄부터 번역문
  3. `=` 로 시작하는 줄 앞까지가 번역문. 원문·번역문 끝의 줄바꿈 하나는 떼어 낸다
  * 그 밖의 줄(주석 `//`, 구분선 `+++`, 제작 도구 머리말)은 상태 1·2 가 아닐 때 무시된다. 끝나지 않은 마지막 블록은 버린다.
* 원문 비교는 **대소문자 구분 완전 일치** (FUN_004de1e0). 같은 원문이 다시 나오면 번역문을 덮어쓴다.
* 번역 함수(FUN_004de9a0)는 현재 언어 번호가 0·1(english)이면 원문을 그대로 돌려주고, 번역이 없어도 원문을 돌려준다.
* 언어 표 (VA 0x5435b8): 0 `english`, 1 `english`(영국 영어), 2 `french`, 3 `german`, 4 `spanish`, 5 `japanese`, 6 `portuguese`.
  `currentLanguage` 값을 이 표에서 찾고 없으면 0번을 쓴다 (FUN_004de420). 첫 실행 시에는 `GetUserDefaultLangID` 로 고른다 (FUN_004de390).
* FUN_004dea00 은 `xlat.german`(3), `xlat.english`(1), `xlat.portuguese`(6) 세 개만 명시적으로 불러온다. french·spanish 를 불러오는 경로는 확인하지 못함.
* 클론 언어 목록은 `dotnetpj/src/Netstorm.Assets/GameLanguage.cs` — 원본 언어에 `korean` 을 더하고, 언어별 파일이 없으면 영어 파일로 대체한다(원본은 대체하지 않음).

## 2. `config.<언어>` — 용어 치환표 및 UI 설정

`키 = "값"` 형식. 두 가지 용도가 섞여 있다.

1. **용어 치환**: 문자열 속 `{키}` 를 값으로 바꾼다. 예: `vortex = "Temple"` → `{vortex}` 가 "Temple" 로 표시.
   개발 당시 명칭이 확정되지 않아 도입된 구조로, 내부 명칭과 표시 명칭의 대응표 역할을 한다.

   | 내부 명칭 | 표시 명칭 |
   |---|---|
   | factory | Workshop |
   | vortex | Temple |
   | stormpower | Storm Power |
   | technology | Knowledge |
   | priest | High Priest |
   | influence | Energy |

2. **UI 문구**: `OkButton`, `WindowName` 등. `{local.1}`, `{fort.mySubHandle|Not Registered}` 같은 런타임 변수 치환(`|` 뒤는 기본값)을 사용한다.

## 3. 언어별 미션·도움말 파일

미션 스크립트와 도움말은 `<이름>.<언어>` 파일로 언어마다 따로 존재한다 (예: `enemyterritory.english`, `enemyterritory.german`).
느슨한 `d/` 폴더와 TAFF 아카이브 양쪽에 있다.

## 클론의 다국어 설계에 대한 시사점

* 원본 체계(영어 원문 = 키, `{용어}` 치환, 언어 확장자 파일)를 그대로 따르면 원본 번역 자산(독일어 등)을 재활용할 수 있다.
* 한국어 추가 시: `xlat.korean`(또는 동등한 UTF-8 번역 파일), `config.korean`, `<미션>.korean` 을 만든다.
* 원본 파일은 Windows-1252 이므로 로드 시 UTF-8 로 변환하고, 신규 한국어 파일은 UTF-8 로 작성한다.
* 원본 비트맵 폰트(`.chfnt`)에는 한글이 없으므로 한국어 표시에는 D2Coding 을 쓴다 (LEFT_JOBS.md 1.5절).
