# WinHelp 도움말 (`help/*.HLP`)

> 분석 상태: 원본 HLP 5개에서 본문 토픽 150개와 그림 77개를 추출했다. 토픽 간 탐색 정보의 재컴파일은 검증하지 않았다.
> 도구: [`tools/hlp.py`](../../tools/hlp.py), 별도로 빌드한 `helpdeco` 2.1.4.

## 파일별 역할

| 원본 | 실제 본문 토픽 | BMP | 내용 |
|---|---:|---:|---|
| `GAME.HLP` | 13 | 71 | 게임 규칙, 튜토리얼 안내, 조작, 멀티플레이, 유닛·주문 설명 |
| `HELP.HLP` | 34 | 3 | 설치·장치·화면·소리 문제 해결 |
| `readme.hlp` | 39 | 3 | 설치 안내, 기술 지원, 요구 사양, 전체화면 설정 |
| `VENDOR.HLP` | 54 | 0 | 당시 하드웨어 제조사와 연락처 목록 |
| `VOCAB.HLP` | 10 | 0 | BIOS·CMOS·PCI 등 컴퓨터 용어 풀이 |

`*.CNT`는 WinHelp의 차례 항목과 다른 도움말 파일의 색인을 담는다. 예를 들어
`game.CNT`는 게임 규칙 장과 `Readme.hlp`·`Vocab.hlp`·`Vendor.hlp`·`Help.hlp` 색인을
나열하며 제목은 *The Book of Nimbus*다. 아카이브의 `help.english`는 게임 안의 F1
도움말 스크립트이므로 이 WinHelp 책과 구분한다. `GAME.HLP` 내부에는
`|TOPIC`·`|Phrases`·`|CONTEXT`·`|FONT`와 `bm0`~`bm70`
항목이 있다. 그림은 추출 후 BMP로 확인할 수 있다.

## 재현 방법

`helpdeco` 소스는 이 저장소의 형제 폴더 `../helpdeco/`에 있다. Visual Studio 2022
Build Tools의 **Win32 Release** 구성으로 작업 공간 안에 복사해 빌드했다.
기존 `vs2026/x64/Debug/helpdeco.exe`는 `GAME.HLP /l`의 14번째 토픽에서 종료 코드 1로
중단됐으며, Win32 빌드는 같은 명령을 끝까지 처리했다.

```powershell
New-Item -ItemType Directory -Force extracted/helpdeco-build | Out-Null
Copy-Item ../helpdeco/src extracted/helpdeco-build/src -Recurse
Copy-Item ../helpdeco/vs2022 extracted/helpdeco-build/vs2022 -Recurse
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' extracted/helpdeco-build/vs2022/helpdeco.vcxproj /p:Configuration=Release /p:Platform=Win32
python tools/hlp.py --helpdeco extracted/helpdeco-build/vs2022/Release/helpdeco.exe
```

도구는 `originals/help`의 HLP를 각각 `extracted/helpdeco/<파일명>/`에 풀어 RTF·BMP·
프로젝트 파일을 보관한다. RTF의 서식표·각주·숨긴 링크 대상을 제외하고 페이지별
본문을 UTF-8 `*.txt`로 만든다. 전체 결과와 경고 여부는 `manifest.json`에 기록한다.
`extracted/`는 Git에 포함되지 않으므로 다른 환경에서는 위 명령으로 다시 생성한다.

## 추출 결과의 범위

* 다섯 파일 모두 `helpdeco /l`의 마지막에 **본문이 없는 `untitled` 가상 토픽 한 개**가
  붙는다. 위 표는 이를 제외하고 RTF의 실제 페이지를 센 값이다.
* `helpdeco`는 모든 파일에서 `Browse start ... not found`와 재컴파일 결과에 관한 경고를
  출력한다. 따라서 토픽 간 이동·링크·원본 화면 재현은 아직 완전하다고 볼 수 없다.
  본문 텍스트와 추출된 그림은 모두 확보했다.
* UTF-8 텍스트는 서식과 링크 대상의 식별자를 버리고 그림 참조를 `{bml bmN.bmp}` 같은
  자리 표시자로 남긴다. 특정 그림이나 링크가 중요한 경우 같은 폴더의 RTF·BMP와
  원본 HLP를 함께 확인해야 한다.
* `GAME.HLP`의 규칙은 당시 도움말의 설명이다. 현재 저장소의 실행 파일은 후대 패치
  빌드이므로, 구현 수치와 경계 조건은 실행 파일·원본 플레이 자료로 다시 확인한다.
  추출한 규칙의 요약은 [도움말의 게임 규칙](../gameplay/help-manual.md)에 정리했다.
