# 원본 게임의 네트워크 포트와 Windows 방화벽 경고

> 2026-09-30 · Netstorm.exe(10.78) 정적 분석(디컴파일·설정 파일) + Linux/Wine(`vm-debian-codex`)에서 `ss` 로 실제 리슨 소켓을 대조했다.
> 계기: Windows PC에서 사용자 직접 조작(수동 분석) 중 방화벽 경고 창이 떴다는 보고. **Windows 방화벽 자체는 이 PC에 없어 경고 창은 재현하지 못했다** — 아래 "Windows 방화벽" 절의 조치는 문서·구조 근거이며 Windows 에서 검증하지 않았다.

## 1. 결론

* **분석 도구(analyzeManager)는 포트를 쓰지 않는다.** 소켓·리스너·HTTP 코드가 없고, MCP 는 표준 입출력(stdio) 전송이며, 의존 패키지(`ModelContextProtocol`, `Microsoft.Extensions.Hosting`)도 이 구성에서는 포트를 열지 않는다. 안내 창·녹화(화면·소리·입력 후킹)도 네트워크와 무관하다.
* **경고를 만든 것은 원본 게임 `Netstorm.exe`(세션 복사본)다.** 게임은 **전투가 시작되면**(데모·튜토리얼·미션) 스스로 게임 서버가 되어 **TCP 6799 를 모든 인터페이스(0.0.0.0)에서 리슨**한다. 싱글 플레이도 "로컬 서버 + 로컬 클라이언트" 구조다. Windows 는 프로그램이 처음 리슨할 때 경고를 띄운다.
* **분석 도구는 세션마다 게임을 새 경로로 복사한다**(`extracted\analyzeManager\<세션>\game\Netstorm.exe`). Windows 방화벽 규칙은 프로그램 경로 단위이므로 **세션마다 경고가 다시 뜰 수 있다.**
* **결정(1-1절)**: 이 경고는 6799 방화벽 예외를 `PREPARE.ps1` 로 미리 등록해 해결한다(7절).
* **포트는 이미 고정되어 있다**(기본 6799). 설정 `gameServerPort`(`d\setup.cfg`)로 바꿀 수 있고 실제로 바뀌는 것을 확인했다. 다만 **경고는 포트가 아니라 프로그램 경로 기준이라 포트를 고정해도 경고는 없어지지 않는다.** 포트 고정은 포트 기준 방화벽 규칙을 만들 때 도움이 된다.

## 1-1. 프로젝트 결정 (2026-09-30, 사용자)

* **클라이언트 유효성 검사는 구현하지 않는다.** 원본이 시작할 때 띄우는 "Not Validated" 안내 창(자동 업데이트 검증 실패, "멀티플레이 불가, 싱글·LAN 가능")과 그 검증·업데이트 기능은 클론에 넣지 않는다. 클론의 시작 흐름은 팁 창(구현 여부는 9단계에서 정함) → 메인 메뉴다.
* **멀티플레이는 후순위**이며 나중에 구현한다(13단계). 이 문서의 8998(LAN 탐색)·6800/6802(루트 서버)·`R.exe` 관련 내용은 그때를 위한 참고 자료다.
* **6799 포트 열림에 따른 방화벽 경고는 임시로 6799 포트를 방화벽 예외로 미리 등록해 해결한다.** 분석용 Windows PC 에서 `PREPARE.ps1` 의 "방화벽 예외 (TCP 6799)" 항목으로 등록·점검한다(7절). 관리자 권한이 필요하며 관리자 권한이 아니면 그 항목은 "사용 불가"로 표시된다.

## 2. 분석 도구가 네트워크를 쓰지 않는 근거

* `analyzeManager/*.cs` 에 `Socket`·`TcpListener`·`UdpClient`·`HttpListener`·`HttpClient`·`NamedPipe`·`localhost` 등이 없다(검색 확인).
* `Netstorm.AnalyzeManager mcp` 는 `WithStdioServerTransport()` 만 쓴다([Program.cs](../../analyzeManager/Program.cs)).
* 게임은 `UseShellExecute=false` 에 표준 입출력을 리디렉션해 시작한다(파이프 상속 차단, [AnalysisEngine.cs](../../analyzeManager/AnalysisEngine.cs)). 이 파이프는 네트워크가 아니다.
* 앱 매니페스트는 `asInvoker`(관리자 권한 요청 없음)이며 DPI 설정만 있다.

## 3. 게임이 쓰는 포트

설정은 `d\setup.cfg` 에 있다(`python tools/nscfg.py show originals/d/setup.cfg`). 값은 원본 기준이다.

| 포트 | 프로토콜 | 방향 | 설정 키 | 열리는 때 | 근거 |
|---:|---|---|---|---|---|
| **6799** | **TCP** | **리슨(0.0.0.0)** | `gameServerPort` = 6799 | **전투 시작**(데모·Auto-Demo·튜토리얼·미션, `FUN_004e2430`). 메인 메뉴에서는 열리지 않고, 한번 열리면 **게임 프로세스가 끝날 때까지** 유지(메뉴로 돌아와도 열려 있음) | 정적 + **Wine 실측** |
| 6799 | SPX(IPX) | 리슨 | 같음 | 위와 함께 시도. IPX/SPX 스택이 없는 현대 Windows 에서는 열리지 않고 무해 | 정적(`spx,*:%d,sn`) |
| — | `zlocal`(프로세스 내부) | — | — | 위와 함께. 싱글 플레이 클라이언트가 이 내부 전송으로 자기 서버에 붙는다 | 정적(`zlocal,*:%d,s/c`) |
| 8998 | UDP(IPX 옵션 있음) | 리슨 | `rootServerBroadcastPort` = 8998 | Multiplayer 의 **Find Local Server**(LAN 서버 탐색, 상태 0x22, `FUN_004e2bd0`) | 정적. 메뉴·데모에서는 열리지 않음(실측) |
| 6800 | TCP/UDP | 리슨(루트 서버 `R.exe`), 바깥으로는 접속 | `officialBasePort` = `rootServerBasePort` = 6800 | LAN 에서 서버가 없어 **내가 로컬 서버가 될 때**(상태 0x25, `R.exe` 실행). 온라인 접속은 `!rootservers.dat`·`netstorm.game-host.org`·`play.netstormworld.com` 등으로 나가는 연결 | 정적 |
| 6802 | TCP | 리슨 | `rootTelnetPort` = 6802 | 루트 서버의 텔넷 관리 포트 | 정적 |
| 80 | TCP | 리슨 | `httpPort` = 80, `httpServer` = **0** | **꺼져 있음**(켜면 내장 웹 서버) | 정적 |
| 6800·6799 | TCP | 바깥으로 | `diagTCPXmitPort` 등 | Multiplayer 진단(방화벽 검사, `diag.netstorm.activision.com`) | 정적 |

* **`R.exe` 의 정체**: 기존 문서의 "자동 업데이트/보조 도구 추정"은 맞지 않다. 문자열이 `NETSTORM Root Server Version %d.%d`·`Mode is … smROOT/smCHAL/smLOCAL`·`bound udp %d`·`tcp,*:%d,s` 이고 `setup.cfg` 의 `rootServerExe = "r.exe"` 로 클라이언트가 부른다 → **루트(로컬 LAN) 서버**다. 온라인 접속 실행기는 `nsLaunchC.exe`.
* 바깥으로 나가는 연결(루트 서버·핑·진단)은 방화벽 **경고를 만들지 않는다**(경고는 인바운드 리슨에서 뜬다). Wine 실측에서도 메뉴·데모 중 바깥 연결 시도는 관찰되지 않았다.

## 4. 실제 관찰 (Linux/Wine, 2026-09-30, `ss -ltnup` 로 기준 목록과 비교)

| 시점 | 새로 생긴 리슨 소켓 |
|---|---|
| 게임 시작 직후(팁 창), 메인 메뉴 | **없음**(12초·70초 시점 모두) |
| 자동 데모 "The Storm Rages!" 시작 | **`0.0.0.0:6799` TCP** (창 제목이 `NetStorm Demo`로 바뀐 순간) |
| 데모를 Exit Demo 로 나가 메인 메뉴로 복귀 | 6799 **그대로 유지** |
| 세션 종료(게임 종료) | 6799 닫힘 |
| 8998/6800/6802/80 | 어느 시점에도 없음 |
| 복사본 `setup.cfg` 에 `gameServerPort = "6899"` 추가 후 같은 절차 | 데모 시작 시 **`0.0.0.0:6899` 만** 열림, 6799 는 열리지 않음 |

* 소켓 소유자는 Wine 에서 `wineserver32` 로 보이는데, Wine 이 소켓을 서버 프로세스가 들고 있는 방식이라서다. 게임 자체 소켓이다.
* 튜토리얼·캠페인 미션을 직접 시작해 보지는 않았다. 코드상 전투 시작 상태(0x7 + `simulconnect`, 0xf, 0x1d)가 모두 같은 리슨 함수를 부르므로 같다고 본다(정적 추론).
* 복사본 설정 변경은 저장소 밖 임시 폴더의 `originals` 복사본에서만 했고 원본과 저장소는 바뀌지 않았다.

## 5. 리슨 코드 (정적 근거)

* `FUN_004e2430(port)` (`Ztrans.cpp`, 로그 "Serving on port %d"): `tcp,*:%d,sn`, `spx,*:%d,sn`, `zlocal,*:%d,s` 리스너를 만든다. 호출: 전투 시작 상태 7(`simulconnect` ≠ 0 일 때, 기본값 `simulconnect = 1`), 상태 0xf(로컬 전투 시작: `DAT_00540bc0 = DAT_00540bc4 = 1` 뒤 `zlocal,*:%d,cn` 클라이언트 연결), 상태 0x1d(`State.cpp`).
* 포트 값: `FUN_00441270("gameServerPort", &DAT_005318e8)` 로 설정 키와 변수가 묶이고 그 변수가 `FUN_004e2430` 에 넘어간다.
* `FUN_004e2bd0`: 상태 0x22(LAN 서버 찾기)에서만 `udp,*:%d,s`(포트 = `rootServerBroadcastPort`)를 연다.
* 바인드 실패 시 `WARNING: Bind failed (port %d), retying with reuseaddr option.` 로 재시도한다(같은 포트를 쓰는 인스턴스가 둘이면 두 번째는 재시도한다 — 분석 도구는 한 번에 한 세션만 허용한다).

## 6. 포트를 고정·변경하는 방법

* 기본이 6799 로 **이미 고정**이다. 바꾸려면 `d\setup.cfg` 의 `gameServerPort` 를 고친다.

```bash
python tools/nscfg.py set <게임 폴더>/d/setup.cfg gameServerPort 6899
```

  (`set` 은 파일 끝에 새 값을 덧붙였고 그 값이 적용됐다. 원본 파일을 고치지 않도록 반드시 복사본에서 한다.)
* **분석 도구는 세션 복사본의 `options.cfg`·`setup.cfg` 에서 `InstallDir`·창 모드·해상도만 바꾼다**([SessionStore.cs](../../analyzeManager/SessionStore.cs)). 포트는 건드리지 않으므로 기본 6799 를 쓴다.
* 바인드 주소(0.0.0.0 대신 127.0.0.1)를 정하는 설정은 없다 — 주소는 `*` 로 고정이다.
* 포트를 바꿔도 **방화벽 경고는 없어지지 않는다**(경고는 프로그램 경로 기준).

## 7. Windows 방화벽 경고와 사전 등록 (임시 조치)

* 경고 창의 "경로" 항목이 `…\extracted\analyzeManager\<세션>\game\Netstorm.exe` 이면 이 글의 원인이다.
* 세션마다 경로가 달라 프로그램 기준 규칙은 반복되므로 **포트 기준 규칙**(TCP 6799 인바운드 허용)을 미리 등록한다. 포트는 기본값 6799 로 고정이라 이 규칙이 모든 세션에 맞는다(6절). 게임 설정 `gameServerPort` 를 바꾸면 `PREPARE.ps1` 의 `$GameServerPort` 도 같이 바꾼다.

### 7.1 `PREPARE.ps1` 방화벽 항목

| 항목 | 내용 |
|---|---|
| 이름·그룹 | "방화벽 예외 (TCP 6799)", 권장. **기본 선택은 해제**(보안 설정을 바꾸므로 명시적으로 고른다. 기본을 켜려면 항목의 `Default` 를 `$true` 로) |
| 규칙 | 이름 `NetStorm Reborn - Game Server TCP 6799`, 인바운드, TCP, 로컬 포트 6799, 허용, 프로필 Any(도메인·개인·공용), 원격 범위 `LocalSubnet` |
| 점검 | 같은 이름의 규칙이 켜져 있고 인바운드 허용·TCP·포트 6799 인지 확인(`Get-FirewallRuleStatus`). 조건에 맞는 규칙이 없거나 꺼져 있으면 "없음" |
| 등록 | 같은 이름의 낡은 규칙을 지운 뒤 새로 만들고 다시 점검한다 |
| 관리자 권한 | 시작할 때 한 번 확인(`Test-IsAdministrator`). **관리자 권한이 아니면 선택 화면에 "[사용 불가] … 관리자 권한 필요"로 표시되고 선택·점검·등록을 하지 않으며 결과 요약에 "사용 불가"로 남는다**(`-All`·`-CheckOnly` 도 같음) |
| 되돌리기 | 관리자 PowerShell: `Remove-NetFirewallRule -DisplayName "NetStorm Reborn - Game Server TCP 6799"` |
| 사용 | 관리자 권한 PowerShell 에서 `powershell -ExecutionPolicy Bypass -File .\PREPARE.ps1` → 항목 선택, 또는 `-All` / `-CheckOnly` |

* **원격 범위를 `LocalSubnet` 으로 제한한 이유**: 원본 게임 서버는 인증이 없다. 분석은 이 PC 안에서만 하므로 같은 서브넷까지만 허용하고, 프로필은 새 네트워크가 "공용"으로 분류돼도 경고가 다시 뜨지 않도록 Any 로 했다. 모든 곳을 허용하려면 `$FirewallRemoteScope = 'Any'`, 개인·도메인만 열려면 `-Profile` 을 `Private,Domain` 으로 바꾼다.
* 리슨 소켓은 전투 중(데모·튜토리얼·미션)에만 열린다(3·4절). 규칙이 있어도 게임을 하지 않으면 6799 를 듣는 프로그램이 없다.
* 이 항목은 **임시 조치**다. 멀티플레이를 구현할 때 다시 정한다(8절).
* 다른 방법(참고): `Set-NetFirewallProfile -Profile Private,Public -NotifyOnListen False`(알림 자체를 끔, 확실하나 PC 전체 설정), 경고 창의 "취소"(허용 안 함 — 싱글 플레이는 내부 전송 `zlocal` 로 동작하므로 분석에는 영향이 없어야 함, 미검증).

### 7.2 검증 상태 — Windows 에서 일부 확인됨

* **Windows 에서 사용자 확인(2026-09-30)**: TCP 6799 방화벽 허용 기능이 정상 동작했고, 관리자 권한이 없을 때 해당 항목을 선택할 수 없는 것도 확인했다.
* **아직 확인할 것**(Windows PowerShell 5.1 과 PowerShell 7 모두):
  1. 비관리자 실행의 `-All`·`-CheckOnly` 결과 요약에 "사용 불가"가 남는지, 다른 항목은 계속 동작하는지.
  2. 관리자 실행에서 기본 선택 해제 및 `-CheckOnly`의 규칙 없음/이미 설치됨 표시가 맞는지. 규칙 필터가 TCP·로컬 포트 6799·프로필 Any·원격 범위 `LocalSubnet`인지.
  3. 두 번 실행해도 규칙이 하나만 남는지(낡은 규칙 삭제 후 재생성).
  4. **등록한 뒤 분석 도구로 게임을 실행해(데모 또는 튜토리얼 시작) 방화벽 경고가 뜨지 않는지.** 사용자 확인 내용에는 경고 억제 여부가 포함되지 않아 아직 미검증이다. 원격 범위 `LocalSubnet` 규칙이 경고를 막는지가 핵심이며, 막지 못하면 `$FirewallRemoteScope = 'Any'` 로 다시 확인한다.
  5. 되돌리기 명령으로 규칙이 지워지고, 지운 뒤 `-CheckOnly` 가 "없음"으로 돌아오는지.
* **사전 검사 기록**: Linux 의 PowerShell 7.6 파서에서 `PREPARE.ps1` 문법 오류가 없음을 확인했고 파일은 UTF-8 BOM·LF 를 유지한다. Linux cmdlet 모의 검사는 다른 필수 항목이 Linux 전용이 아니라 끝까지 실행되지 않아 Windows 동작 검증으로 세지 않는다.
* `PREPARE.sh`(Linux)에는 이 항목이 없다 — Linux 에서는 경고 창이 없다.

## 8. 클론 설계 메모

* 클론의 싱글 플레이(튜토리얼·캠페인·데모·Test Battle)는 **소켓을 전혀 열지 않는다**(세션이 프로세스 안에서 돌아가며 외부 서버 구조가 필요 없다) → 방화벽 경고를 만들지 않는다. 6799 사전 등록(7절)은 **원본 게임을 분석할 때만** 필요한 임시 조치다.
* **클라이언트 유효성 검사(자동 업데이트 검증, "Not Validated" 안내)는 구현하지 않는다**(사용자 결정 2026-09-30). 그에 딸린 온라인 서버 접속·버전 확인도 없다.
* **멀티플레이는 후순위**(13단계, 사용자 결정 2026-09-30). 구현할 때만 네트워크를 쓴다. 원본 호환 기본값 참고: 게임 서버 TCP 6799, 루트 서버 6800, LAN 탐색 UDP 8998. 원본과 다른 프로토콜을 쓸 예정이면 포트를 설정 가능하게 하고 호스트 만들기 시점에만 리슨하며, 리슨 전에 사용자에게 알린다.
* 메인 메뉴의 Multiplayer 버튼을 멀티플레이 구현 전까지 어떻게 보일지(생략·비활성)는 9단계 UI 에서 정한다.
* 인터넷 바깥 연결(루트 서버 등)은 원본 서버가 사라져 의미가 없다.

## 9. 재현 방법

```bash
# Linux(Wine, vm-debian-codex 등 실행 예외 시스템에서만): 세션 시작 → 팁/인증 창 닫기 → 45초 무입력(자동 데모) → 리슨 소켓 확인
bash analyzeManager/linux-wine.sh call start_session '{"label":"포트 확인"}'
ss -ltnup | grep -E ':(6799|8998|6800|6802|80)\b'
```

* 안내 창 클릭 좌표는 창 크기(1600×828 → 1024×768로 바뀌는 시점이 있다)에 따라 달라지므로 클릭 전에 `capture_state`로 확인한다(이번 관찰에서도 첫 클릭이 빗나간 사례가 있었다).
