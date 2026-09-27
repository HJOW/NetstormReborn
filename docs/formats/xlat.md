# 원본 다국어 체계 (`xlat.*`, `config.*`, 언어별 미션 파일)

> 분석 상태: **포맷 확인 완료** — 12단계(다국어) 설계의 기반 자료

원본 게임은 영어 문자열을 코드에 하드코딩하고, 실행 시 언어별 번역 테이블로 치환하는 구조다.
현재 언어는 `options.cfg` 의 `currentLanguage` (예: `"english"`) 로 정해지며, 이 값이 곧 파일 확장자다.

## 1. `xlat.<언어>` — UI 문자열 번역 테이블

위치: TAFF 아카이브 `\d\xlat.<언어>` (english 는 빈 자리표시자 `This file is a simple placeholder`).

| 언어 | 항목 수 |
|---|---|
| german | 871 |
| french / spanish / portuguese | 746 |

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
* 원문 문자열이 곧 키다 → **독일어 파일의 원문 871개가 번역 대상 UI 문자열 목록**이 된다 (패치판에서 추가된 문자열은 빠져 있을 수 있음 — 실행 파일 문자열과 대조 필요).

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
