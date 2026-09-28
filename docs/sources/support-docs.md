# 설치·지원 문서 요약 (`readme.hlp`, `README.DOC`, `HELP.HLP` 등)

> 원문: `originals/help/readme.hlp`·`HELP.HLP`·`VENDOR.HLP`·`VOCAB.HLP` (helpdeco 추출 `extracted/helpdeco/*/`),
> `originals/help/README.DOC`(Word 95 문서, 1997-10-23), `originals/Readme.txt`·`TMaker.txt`·`disclaimer.txt`·`steam_appid.txt`.
> 대부분 Windows 95 시절 설치·장치 문제 해결 안내다. 클론에 쓸 수 있는 사실만 남긴다.

## 1. 원판 실행 환경 (readme.hlp)

* 대상 OS: **Windows 95 전용** (3.1·NT·OS/2·DOS 미지원). 영어판 Windows 95 필요. **DirectX 5** (DirectDraw·DirectSound).
* 최소 사양: Pentium 90MHz, RAM 16MB, **256색 640×480** 그래픽(1MB), Sound Blaster 호환, 2배속 CD-ROM, 마우스.
  패치판 `Readme.txt`: Windows 9x 이상, RAM 24MB, 디스크 25MB, DirectX 5 이상.
* 멀티플레이: 14.4Kbps 모뎀, LAN(**IPX**), 인터넷(**TCP/IP**). 두 명마다 CD 1장.
* 설치 크기 2가지(최소·최대). 레지스트리 `HKEY_LOCAL_MACHINE\SOFTWARE\Titanic Entertainment\NetStorm`.
* 음악 `.mus` = 이름만 바꾼 `.wav` (우리 분석과 일치).

## 2. 화면 모드 (readme.hlp "Full Screen/Direct Draw Mode")

* 전체화면 = DirectDraw. **Alt+Enter** 로 전체화면 ↔ 창 모드 전환 (언제든).
* **Edge Scrolling 은 전체화면에서만** 켜진다. 단 바탕화면 해상도가 640×480 이면 창 모드가 전체화면을 흉내 내고 이때도 가장자리 스크롤이 된다.
* 전체화면 옵션(Options → Adjust Direct Draw):
  * Safe & Slow Mode — 가장 호환성 높음, 느림
  * Safe & Jumpy Mouse — 커서 잔상 해결, **F12** 로 전환 (README.DOC 에서는 "Software Mouse")
  * **Parallax Clouds** — 구름을 멀리 있는 것처럼 움직여 높이감 표현, 비디오 메모리 4MB 미만이면 자동 꺼짐
  * Reduce Tearing — 스크롤 시 화면 찢김 감소
* 일부 그래픽카드(S3 Trio 64 등)는 전체화면을 고르면 자동으로 "흉내 전체화면"(640×480 창 모드)으로 바뀐다 (README.DOC).
* 작업표시줄 "항상 위"가 켜져 있으면 실행이 안 되거나 창 일부를 가린다.
* 글꼴 관리자(Adobe 등)나 큰 글꼴 설정이 게임 글꼴을 바꾼다 → 원본은 **Windows GDI 글꼴**로 글자를 그린다 (`.chfnt` 캐시 분석과 일치, [chfnt.md](../formats/chfnt.md)).

→ 클론 요구사항과의 관계: 원본은 가장자리 스크롤을 전체화면(또는 640×480 창)에서만 켰다. AGENTS.md 는 풀스크린에서의 가장자리 스크롤을 요구하므로 원본과 같다. 창 모드 동작은 설계 선택.

## 3. 멀티플레이 연결 (README.DOC 1997-10-23, readme.hlp)

* **BattleMaster 가 서버 역할**을 하고, 서버가 끊기면 **다른 참가자가 다음 서버**로 선택된다 (14.4Kbps 방장은 권장하지 않음).
* 방화벽 뒤 플레이어는 BattleMaster 가 될 수 없다.
* Ready 체크박스 점이 검은색이면 BattleMaster 와 완전히 연결되지 않은 상태.
* LAN TCP/IP 서버 수동 추가 시 기본 **포트 6800**. `Find Local Server > TCP/IP` 로 루트 서버 자동 탐색.
* IPX/SPX 는 불안정하니 LAN 에서도 TCP/IP 권장. Kali 같은 IPX 에뮬레이터 불필요(자체 TCP/IP).
* 매뉴얼 변경: "Friends" 옵션 삭제, BattleMaster 옵션 **Breakable Alliances**(기본 OFF = 동맹이 맺어지면 깨지지 않음) 추가.
* 패치판 `Readme.txt`: 온라인은 `nsLaunchC.exe` 로 실행, 캠페인·LAN 은 `Netstorm.exe`. 서버 소식은 netstormhq.com.

→ 13단계 참고: 원본은 방장 PC 가 서버인 구조 + 서버 인계. 락스텝 여부는 exe 분석 필요 (`zacket` = 패치 문서에 나오는 네트워크 패킷 용어).

## 4. README.DOC 의 유닛 수치 변경 (1997-10-23)

Sun Barricade 200sp · 1400 hits · Level 1(Sun 1), Whirligig hits 25, Dust Devil damage 12, Acid Barricade hits 1400, Ice Tower 600sp, Man o' War Pool 600sp, Bulf hits 800, Arc Spire hits 1600.
이후 변화는 [patch-history.md](patch-history.md) 5절.

## 5. 기타 파일

| 파일 | 내용 |
|---|---|
| `HELP.EXE` | "Activision Help Utility" — 도움말 파일 이름을 받아 여는 작은 실행기 |
| `*.GID` | WinHelp 가 만든 색인 캐시 (원본 자료 아님) |
| `*.CNT` | 도움말 목차. `game.CNT` 제목 "The Book of Nimbus", 장 9개. `README.CNT` 에 "Information_for_Dark_Reign_Multi_play" 같은 다른 Activision 게임 템플릿 흔적 |
| `VENDOR.HLP` | 1997년 하드웨어 제조사 연락처 54개 — 클론과 무관 |
| `VOCAB.HLP` | BIOS·CMOS·ISP·PCI·QEMM·RAM·URL·VESA·VLB 용어 풀이 — 무관 |
| `HELP.HLP` | Windows 95 장치 문제 해결 34개 토픽. 참고할 점: 게임 글꼴이 Windows 글꼴 설정 영향을 받음, 16비트 오디오 필요, 레지스트리 키 |
| `TMaker.txt` | `TMaker.exe` 로 모든 레벨을 가진 요새(fort)를 만들어 멀티플레이 Save Game 으로 고르는 방법 |
| `disclaimer.txt` | NSHQ(netstormhq) 배포 고지: 권리는 Activision, 방치 소프트웨어(abandonware)로 배포 |
| `steam_appid.txt` | `12185140` — Steam 앱 번호로 보임 (Steam 배포판 흔적) |
| 제작진 (readme.hlp Credits) | Titanic Entertainment(Zack Simpson, Ken Demarest 등), 그래픽 코드 John Miles(→ Miles 셰이프 형식), 음악 Mark Morgan, 배급 Activision |
