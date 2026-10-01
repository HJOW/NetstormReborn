Netstorm - Reborn
-------------------------------------------------------------------------

# 개요 
Activision 사에서 1997년도에 개발 후 한참 전에 손 놓은
Netstorm - Islands at war 게임을
AI 를 이용해 되살리는 프로젝트입니다.

# 현재 개발 진행상황 관련

현재 실행하면 **메인 메뉴 → 캠페인 → 자유를 위한 투쟁 → 1-1 전쟁의 시작!**을 플레이할 수 있습니다.
옵션에서 해상도·창/전체화면·효과음/음악 볼륨을 바꿀 수 있으며 다른 메뉴·미션은 잠금 표시합니다.
`dotnet run --project src/Netstorm.Game -c Release -- --language korean`으로 시작합니다(영어: `--language english`).
원본 데이터가 필요하며 임시 AI·건설 규칙과 검증 한계는 [캠페인 1-1 구현](docs/gameplay/campaign-one.md)에 정리했습니다.

# 빌드 및 실행 파일 만들기

아래 명령은 프로젝트 루트(`NetstormReborn`)에서 실행합니다. 게임 본체는 C# / MonoGame DesktopGL이며 대상 프레임워크는 `net10.0`입니다.

## 준비

- **.NET 10 SDK**를 설치하고 `dotnet --version`으로 확인합니다. `global.json`은 10.0.100 이상인 .NET 10 SDK의 최신 기능 버전을 허용합니다.
- 첫 빌드·배포 시 NuGet 패키지와 대상 OS의 런타임을 내려받을 인터넷 연결이 필요합니다.
- 원본 게임 데이터인 `originals/`와 한국어 글꼴 `fonts/D2Coding-Ver1.3.2-20180524-all.ttc`를 준비합니다. 글꼴은 빌드·배포 출력에 자동으로 복사되고, 원본 데이터는 따로 배치합니다.
- 실행 대상은 **Windows 10/11 x64** 또는 **GUI 환경의 x64 Linux(glibc 기반)**입니다. OpenGL을 지원하는 그래픽 드라이버가 필요합니다.

## 개발용 빌드와 실행

```sh
dotnet restore Netstorm.sln
dotnet build Netstorm.sln -c Release --no-restore
dotnet run --project src/Netstorm.Game -c Release --no-build -- --language korean
```

영어로 실행하려면 `--language english`를 사용합니다. 빌드 결과는 `src/Netstorm.Game/bin/Release/net10.0/`에 생성됩니다. 일반 빌드는 빌드한 OS용 실행 파일을 만들며, 다른 OS용 바이너리는 아래의 `-r` 옵션으로 만듭니다.

## 런타임을 포함한 배포용 바이너리

게임 프로젝트에 `dotnet publish`를 실행합니다. 다음 명령은 Windows PowerShell과 Linux 셸에서 모두 사용할 수 있으며, Linux에서도 Windows exe를 만들 수 있습니다.

```sh
dotnet publish src/Netstorm.Game/Netstorm.Game.csproj -c Release -r win-x64 --self-contained true -o dist/win-x64
dotnet publish src/Netstorm.Game/Netstorm.Game.csproj -c Release -r linux-x64 --self-contained true -o dist/linux-x64
```

| 대상 | 출력 폴더 | 실행 파일 |
| --- | --- | --- |
| Windows 10/11 x64 | `dist/win-x64/` | `NetstormClone.exe` |
| Linux x64 | `dist/linux-x64/` | `NetstormClone` |

`--self-contained true`는 .NET 런타임까지 포함하므로 실행할 PC에 .NET SDK나 런타임을 따로 설치할 필요가 없습니다. SDL2·OpenAL 네이티브 라이브러리와 `fonts/`도 출력에 포함됩니다. 실행 파일은 함께 생성된 DLL·런타임 파일·글꼴을 사용하므로 **출력 폴더 전체를 복사해 배포**합니다. 그래픽 드라이버와 Linux GUI 환경 등 OS의 실행 환경은 별도로 필요합니다.

실행할 PC에 .NET 10 런타임이 이미 설치되어 있다면 더 작은 배포물을 만들 수 있습니다. 위 명령의 `--self-contained true`를 `--self-contained false`로 바꾸고, 출력 폴더도 `dist/win-x64-fdd` 또는 `dist/linux-x64-fdd`처럼 구분합니다.

## 원본 데이터 배치와 바이너리 실행

배포 폴더 안에 **`originals/` 폴더를 통째로 복사**합니다. `netstorm.tarc`와 함께 `d/`, `music/`, `sound/` 및 나머지 파일을 유지합니다. Windows 배포 폴더는 다음과 같은 구조가 됩니다(Linux도 동일하며 실행 파일 이름은 `NetstormClone`입니다).

```text
dist/win-x64/
  NetstormClone.exe
  NetstormClone.dll
  ... (함께 생성된 DLL·런타임·설정 파일)
  fonts/
    D2Coding-Ver1.3.2-20180524-all.ttc
  originals/
    netstorm.tarc
    d/
    music/
    sound/
    ... (원본 데이터의 나머지 파일)
```

Windows PowerShell에서 복사 후 실행:

```powershell
New-Item -ItemType Directory -Path .\dist\win-x64\originals -Force | Out-Null
Copy-Item -Path .\originals\* -Destination .\dist\win-x64\originals -Recurse -Force
.\dist\win-x64\NetstormClone.exe --language korean
```

Linux에서 복사 후 실행:

```sh
cp -a originals dist/linux-x64/
chmod +x dist/linux-x64/NetstormClone
./dist/linux-x64/NetstormClone --language korean
```

원본 데이터를 다른 위치에서 공유하려면 `NETSTORM_DATA`에 **`netstorm.tarc`가 들어 있는 폴더의 절대 경로**를 지정합니다. 이 값이 없으면 실행 파일 폴더와 현재 작업 폴더 및 그 상위 폴더의 `originals/`를 자동으로 찾습니다.

```powershell
$env:NETSTORM_DATA = "D:\Games\Netstorm\originals"
.\dist\win-x64\NetstormClone.exe --language korean
```

```sh
NETSTORM_DATA="/절대/경로/originals" ./dist/linux-x64/NetstormClone --language korean
```

# License

MIT License
Copyright (c) 2026 HJOW
