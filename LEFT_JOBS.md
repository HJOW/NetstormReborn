# LEFT_JOBS — NetStorm 클론 프로젝트 작업 계획 및 인수인계

> 최종 갱신: 2026-10-10 (**음악 버퍼 생성·링 버퍼 갱신과 DirectSound 경계 복원 완료.** 실제 채널 Read/Stop에 첫 채우기·커서 기반 두 구간 갱신·복구/음량·Unlock/반복 Play를 연결했다. 각 COM 실패와 EOF/부분 읽기·호스트 예외의 잠금 반납을 확인했다. 새 독립 원본 **5,088개**, 누적 **435,733개**. Release 경고/오류 **0**, CTest 내부 **503개·실패 0**(123.11초, 전체 123.20초), 원본 근거 감사 **77종 모두 통과**. 음악 파일 열기·format/duration·경로/fallback·두 채널/이벤트/스레드·실제 오디오/GUI GameWorld 연결은 후속이다. 영어=원본 글꼴·한국어/D2Coding/화면 요구사항/MCP 4차·outpost/LAN 3차 유지. 게임/창/실제 장치 실행·AGENTS.md/보호 파일/dotnetpj 변경·커밋/푸시 없음.)
> **마지막 디컴파일 수행 PC: `HJOW-Athlon` (IP 192.168.0.94), 2026-10-10.** 최신 `musicstream` 목록 **2/4/4개**를 같은 PC에서 읽기 전용으로 내보냈다. 음악 버퍼 생성/갱신 독립 입력 **5,088개**, 최신 감사 **77종 모두 통과**. 이후 musiccontrol 이하 기록은 앞 단계 이력이다. 최신 `musiccontrol` 목록 **11/13/13개**를 같은 PC에서 읽기 전용으로 내보냈다. 음악/공유 음소거 독립 입력 **5,292개**, 최신 감사 **76종 모두 통과**. 이후 soundcontrol 이하 기록은 앞 단계 이력이다. 최신 `soundcontrol` 목록 **6/6/6개**를 같은 PC에서 읽기 전용으로 내보냈다. 기존 `soundplay` 몸체도 실행해 제어 독립 입력 **3,189개**, 최신 감사 **75종 모두 통과**. 이후 sounddevice 이하 기록은 앞 단계 이력이다. 최신 `sounddevice` 목록 **7/7/7개**를 같은 PC에서 2026-10-10 읽기 전용으로 내보냈다. 실제 WAVE 열기/읽기/닫기의 독립 입력 **1,305개**, 최신 감사 **74종 모두 통과**. 이후 soundplay 이하 기록은 앞 단계 이력이다. 앞 단계 `soundplay` 목록 **19/16/16개**를 같은 PC에서 2026-10-10 읽기 전용으로 내보냈다. 소리 재생 계층의 독립 입력 **14,472개**, 최신 감사 **73종 모두 통과**. 이후 soundprocess 이하 기록은 앞 단계 이력이다. 앞 단계 `soundprocess` 목록 **10/7/7개**를 같은 PC에서 2026-10-10 읽기 전용으로 내보냈다. 소리 프로세스/이름 표의 독립 입력 **18,381개**, 최신 감사 **72종 모두 통과**. 이후 forcefieldpostpop 이하 기록은 앞 단계 이력이다. 앞 단계 `forcefieldpostpop` 목록 **7/6/6개**를 같은 PC에서 2026-10-10 읽기 전용으로 내보냈다. 실제 보호막 후처리/Regular/좌표 타입 finder의 독립 입력 **3,624개**, 최신 감사 **71종 모두 통과**. 이후 priestpop 이하 기록은 앞 단계 이력이다. 최신 `priestpop` 목록 **24/18/18개**를 같은 PC에서 2026-10-10 읽기 전용으로 내보냈다. 실제 공통 Pop/firstPop/Activate→사제 전체 wrapper의 독립 입력 **3,840개**, 최신 감사 **70종 모두 통과**. 이어지는 frameadvance 이하 기록은 앞 단계 이력이다. 이번에는 `frameadvance` 목록 2/4/4개를 같은 PC에서 2026-10-10 읽기 전용으로 내보내고 진행→실제 지정의 독립 입력 23,040개를 만들었다. 감사 69종 모두 통과. 기존 `carriercheck` 원본 2,028개/감사도 유지하며 누적에 포함한다. 앞 단계 `priestfall` 목록 15/15/15개를 2026-10-10 같은 PC에서 읽기 전용으로 내보내고 낙하 요청·실제 공유 생성자·0x25b 처리의 독립 입력 1,368개를 만들었다. 감사 67종 모두 통과. 앞 단계 `priestpostpoptail` 목록 18/20/20개·독립 입력 6,144개도 같은 PC에서 만들었다. 아래 2026-10-09까지의 내보내기 이력은 당시 기록이다. `VM-W11-CODEX`에서 바뀐 디컴파일 소스(새 함수 목록 14개)를 pull(`438e749`)한 뒤 이 PC에서 `tools/ghidra/export_functions.ps1 -All`로 내보내기를 다시 만들었다(기존 Ghidra 프로젝트 재사용·읽기 전용, 게임 실행 없음). `tools/decomp_*_oracle.py --verify` 감사 **37개 모두 통과**. 이어 새 `prieststate` 목록(10.78 11개·CD/추가 10.37 각각 12개)을 `HJOW-Athlon`에서 읽기 전용으로 내보내고 독립 대조 입력 20,256개를 만들었다. 후속 새 `priestregen` 목록(10.78 10개·CD/추가 10.37 각각 11개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 실제 회복 0x25a의 독립 입력 4,539개를 만들었다. 이어 새 `priestdestroy` 목록(세 판본 4/4/4개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 사제 preDestroy의 독립 입력 6,552개를 만들었다. 이어 `priestforcefield` 목록(10.78 12개·CD/추가 10.37 각각 10개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 실제 보호막 조회/삭제 준비의 독립 입력 8,928개를 만들었다. 이어 `carrierpredestroy` 목록(세 판본 7/7/7개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 Carrier/실제 contained 조회의 독립 입력 2,880개를 만들었다. 이어 `damageablepredestroy` 목록(세 판본 9/9/9개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 Damageable 효과/소리 접두·종속 순회/Carrier의 독립 입력 4,896개를 만들었다. 이어 `damageablerelease` 목록(17/15/15개)을 같은 `HJOW-Athlon`에서 2026-10-08 23:58~59에 읽기 전용으로 내보내고, 자정 뒤 해방 구간 실제 실행의 독립 입력 4,512개를 만들었다. 이어 2026-10-09 `priestspawn` 목록(8/7/7개)을 같은 PC에서 읽기 전용으로 내보내고 실제 사제 나선 생성/Carrier 검사 래퍼의 독립 입력 1,236개를 만들었다. 이어 `priestplacement` 목록(3/4/4개)을 같은 PC에서 읽기 전용으로 내보내고 실제 배치 접두의 독립 입력 3,024개를 만들었다. 이어 `priestpreview` 목록(9/7/7개)을 같은 PC에서 읽기 전용으로 내보내고 실제 로컬 미리보기/표면/관계의 독립 입력 4,860개를 만들었다. 이어 `priestcollision` 목록(5/4/4개)을 같은 PC에서 읽기 전용으로 내보내고 실제 무시 helper/한 충돌 후보 구간의 독립 입력 11,520개를 만들었다. 이어 `priestgeometry` 목록(10/9/9개)을 같은 PC에서 읽기 전용으로 내보내고 실제 비패턴 decoder/범위/한 모양 finder 인자의 독립 입력 4,352개를 만들었다. 이어 `priestshape` 목록(6/4/4개)을 같은 PC에서 읽기 전용으로 내보내고 전체 비패턴 픽셀 getter/실제 SHP 조회의 독립 입력 8,064개를 만들었다. 이어 `priestterrain` 목록(7/8/8개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 실제 후보/모양 종료/최종 구간·지역 getter의 독립 입력 6,720개를 만들었다. 이어 `typehotspot` 목록(3/3/3개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 숫자 속성/실제 CRT ftol 독립 입력 1,404개를 만들었다. 이어 `canontype` 목록(10/9/9개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 타입별 decoder 전체 생성/순회/범위의 독립 입력 3,927개를 만들었다. 이어 `canonshape` 목록(8/6/6개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 전체 일반/패턴 픽셀 getter의 독립 입력 5,950개를 만들었다. 이어 `canongeometry` 목록(12/11/11개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 전체 decoder/각 모양 finder 인자·snap/CRT의 독립 입력 5,967개를 만들었다. 이어 `canonplacement` 목록(3/4/4개)을 같은 PC에서 읽기 전용으로 내보내고 일반 타입/별도 패턴 인자의 독립 입력 3,360개를 만들었다. 이어 `canonpreview` 목록(9/7/7개)을 같은 PC에서 읽기 전용으로 내보내고 일반 타입/별도 패턴 인자의 로컬 미리보기 독립 입력 4,860개를 만들었다. 이어 `canoncollision` 목록(5/4/4개)을 같은 PC에서 읽기 전용으로 내보내고 일반 타입 후보의 독립 입력 22,680개를 만들었다. 이어 `canonterrain` 목록(7/8/8개)을 같은 PC에서 읽기 전용으로 내보내고 일반 타입 초기 권한/후보/모양 종료/지면 누적/지역 getter의 독립 입력 11,220개를 만들었다. 이어 `canonpermission` 목록(3/2/2개)을 같은 PC에서 읽기 전용으로 내보내고 실제 후보 권한/패치 방향 관계 helper의 독립 입력 16,890개를 만들었다. 이어 `playeranchor` 목록(14/15/15개)을 같은 PC에서 읽기 전용으로 내보내고 실제 Player/거리/그래프/contained 정상 반환의 독립 입력 4,794개를 만들었다. 이어 `canonsurrounding` 목록(25/21/21개)을 같은 PC에서 읽기 전용으로 내보내고 실제 flag 8 finder/주변 권한 구간의 독립 입력 1,572개를 만들었다. 이어 `canonrelations` 목록(6/2/2개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 최종 구간/실제 정상 반환의 독립 입력 6,921개를 만들었다. 이어 `canonmayplace` 목록(56/51/51개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 전체 MayPlace 정상 반환 독립 입력 432개를 만들었다. 이어 `outpostlifecycle` 목록(4/2/2개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 두 목록 접두의 정상 반환 독립 입력 2,304개를 만들었다. 최신 감사 63종 모두 통과. 정밀 디컴파일(`refine_all.ps1`)은 이 PC에서 다시 하지 않았다. 바로 앞 수행: `VM-W11-CODEX`(IP 10.0.0.17), 2026-10-08 — 그 PC에서 한 것: 세 판본 기본 Ghidra 프로젝트 위의 읽기 전용 내보내기 전체(`tools/ghidra/export_functions.ps1 -All`과 새 목록 `bridgeconnect`·`setframe`·`islandlifecycle`·`regiongraph`·`pop`·`display`·`postpop`·`graph`·`lifecycle`·`islandpostpop`·`pathanimation`·`priestowner`·`priestpostpop`·`carrierpostpop`), 정밀 디컴파일(`tools/ghidra/refine_all.ps1`, 10.78·CD). 결과는 Git 제외 `extracted/`에 있어 **다른 PC에서 cpppj/디컴파일 분석을 이어가려면 그 PC에서 다시 만들어야 한다**([절차](docs/exe/ghidra-exports.md)). 10.62·10.82(V10/V12) 판본은 이 PC에서 디컴파일하지 않았다. 이어 `regionownership` 목록(10/8/8개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보내고 전체 지역 투표/getter/재귀의 독립 입력 1,320개를 만들었다. 최신 감사 **64종 모두 통과**. 이어 KST 2026-10-09 자정 전에 `priestshield` 목록(15/13/13개)을 같은 `HJOW-Athlon`에서 읽기 전용으로 내보냈으며 2026-10-10 전체 보호막 생성 wrapper/lookup/소유자 대조 2,496개를 만들었다. 최신 감사 **65종 모두 통과**.
> 프로젝트 목표(AGENTS.md): 원본 NetStorm: Islands at War 를 디컴파일/분석하여 클론 코딩하고,
> **Windows 10/11** 과 **GUI 환경의 Linux** 에서 동작하며 **여러 언어를 지원**하는 게임을 만든다.
> **모방 범위(2026-10-03 AGENTS.md 변경)**: 기존 게임의 **사운드·그래픽·애니메이션 등 거의 모든 요소를 가능한 한 동일하게** 최대한 모방한다.
> **1차 목표 언어: 영어, 한국어** (그 외 언어는 이후 확장). cpppj의 한국어 지원은 4차 목표에서 추가한다(AGENTS.md 2026-10-10 변경).
> **변경된 목표 순서(AGENTS.md, 2026-10-10 반영):** cpppj는 1차 **10.78 싱글플레이 최대한 복원**, 2차 **Windows 10/11 오류 없는 실행**, 3차 **TCP/IP LAN 멀티플레이/outpost**, 4차 **요구사항 반영·MCP·한국어 지원 추가**다(한국어 지원은 AGENTS.md 2026-10-10 00:20 변경으로 4차에 명시됐다). outpost 전용 후속 구현은 2차 뒤로 미루며 공식 서버 접속 복원은 하지 않는다. cpppj는 Windows 전용이고 MCP는 이후 dotnetpj 개발을 위한 분석에 사용한다. dotnetpj는 cpppj 완성 후 분석하여 원본과 완전히 동일한 동작을 먼저 구현하고 그 뒤 요구사항을 반영하며 Linux는 후순위다.
> **주석 규칙(AGENTS.md 2026-10-10 변경, cpppj·dotnetpj 공통):** 새로 만드는 코드의 **상수·함수·클래스·멤버함수·반복문**에 모두 한국어 주석을 단다. 주석의 주 내용은 **그것이 무엇이고 어떻게 쓰는지(설명과 사용법)**이며, **언제·왜 추가/수정했는지는 주 내용 뒤**에 적는다. 적용 방법과 순서는 [cpp-build.md 5절](docs/cpp-build.md#5-다시-만든-코드의-규칙)을 따른다. 기존 주석은 **그 주석이 설명하는 대상을 수정할 때** 새 규칙에 맞춘다(2026-10-10 사용자 결정).
> **근거 우선순위·글꼴(AGENTS.md 2026-10-08·09 변경):** 기존 게임에서 재현되는 동작과 **디컴파일로 알아낸 정보가 팬게임보다 우선**한다. 한국어 글꼴은 `fonts/`의 D2Coding이며 마이너 업그레이드판 `D2Coding-Ver1.4.0-20261003-all.ttc`가 같은 폴더에 추가됐다(1.5절). 새 판으로의 교체는 중요도가 낮다(2026-10-10 사용자 결정).
> **dotnetpj의 의도적 차이(2026-10-10 사용자 결정):** dotnetpj가 원본과 일부러 다르게 둔 구조(24Hz 고정 틱·입력·설정 저장 등)는 원본처럼 동작하도록 바꾼다. **지금 구조를 둔 채 원본 게임의 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다.** 대상과 판정 절차는 [LEFT_JOBS.dotnetpj.md 9-1](LEFT_JOBS.dotnetpj.md#original-parity-plan).
> **영어 글꼴(2026-10-10 사용자 결정):** **언어가 영어일 때의 글꼴은 기존 게임의 글꼴(원본 `.chfnt` 비트맵 글꼴)을 따른다.** 언어가 한국어이면 `fonts/`의 D2Coding을 쓴다. dotnetpj의 적용 상태는 [LEFT_JOBS.dotnetpj.md 5-3·10절 2번](LEFT_JOBS.dotnetpj.md)에 있다. cpppj는 이미 원본 글꼴을 쓰며 한국어 지원(4차) 때 같은 기준을 따른다.
> **화면 요구사항(2026-09-28 AGENTS.md 추가, 2026-10-03·10-05 변경)**: 풀스크린 모드와 화면비 **16:9 · 16:10 · 4:3** 지원, **기존 게임 수준의 프레임으로 먼저 만들고 이후 60·120프레임 지원**(2026-10-05 변경. 원본 수준 = `maxFPS` 75·14ms 루프; 당시 C# 적용 기록은 별도 문서), 풀스크린에서 **마우스를 화면 끝에 대면 화면 이동**(원본도 옵션에서 켰을 때 지원), 원본의 **전체화면 전환 뒤 재실행 오류는 클론에서 발생하지 않아야 한다** — 1.4·1.7절

> **문서 분할 정리(2026-10-07, DESKTOP-HJOW):** 기존 dotnetpj 전용 구현·검증·기술 스택·개발 계획을 [LEFT_JOBS.dotnetpj.md의 이관 이력](LEFT_JOBS.dotnetpj.md#legacy-dotnet-history)으로 옮겼다. 기존 절 제목은 이관 링크를 위해 유지했다. 이번에는 두 인수인계 문서만 정리했으며 게임/창 실행·빌드/테스트를 하지 않았다.
> **공통사항의 적용 범위:** 원본 10.78 동작·판본 비교·디컴파일·포맷·그래픽/소리/영상·글꼴·사용자 게임 요구사항과 tools/analyzeManager/PREPARE 기록은 두 프로젝트의 공통 근거로 여기에 남긴다. 공용 분석기가 C#이어도 dotnetpj 게임 전용이 아니다. Netstorm.Core/Assets/Game·MonoGame·게임 .cs 변경·xUnit/clone 스모크 완료는 dotnetpj 기록이다. **원본 분석 완료가 두 게임의 구현 완료를 뜻하지 않는다.**
> **과거 기록 읽기:** 프로젝트 분리 전 “클론” 구현/화면 확인은 C# 클론이다. 옛 테스트 수/단계 체크는 당시 이력이고 최신 dotnetpj 상태는 별도 문서 맨 위, cpppj 상태는 이 문서 맨 위를 따른다. 궁극적인 Linux/다국어 요구사항은 공통이지만 현재 cpppj 구현 범위는 Windows용이며 dotnetpj Linux 지원은 후순위다.

---

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 음악 버퍼 생성·링 버퍼 갱신 (HJOW-Athlon)

- [x] **착수/호스트:** AGENTS.md·두 LEFT_JOBS·cpppj README/playable-plan·음악 제어/소리 장치 근거를 확인했다. 시작 트리 깨끗함, 현재/마지막 디컴파일 PC 일치. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** 새 `musicstream` 목록 **2/4/4개**를 읽기 전용으로 내보냈다. 기존 musiccontrol·soundplay 몸체도 실제 명령으로 실행했다. 게임/복사본·클론 창·실제 장치/스레드 실행, AGENTS.md·보호 경로·dotnetpj 변경, 커밋/푸시 없음.
- [x] **구현:** [`MusicChannel`](cpppj/src/client/SoundMusic.h)에 `EnsureBuffer`·`FillBuffer`, `SoundMusic::Update`와 `MusicBufferHooks`/두 잠금 구간을 추가했다. 기존 버퍼 보존·고정 생성(flags 0xe2, 176,000바이트), 상태/손실 복구→음량(실패는 계속)→재생 커서 기반 갱신→Lock/실제 Read→쓰기 위치 modulo→Unlock/필요 시 반복 Play를 원본 순서로 수행한다. EOF/부분 IO/COM 실패는 중첩 실제 Stop으로 정리하고 두 판본 패딩/반복 비트 보존을 유지한다. 호스트 IO 예외/잘못된 구간도 실제 잠금을 반납한다. [`SoundDevice`](cpppj/src/client/SoundDevice.h)의 `BindMusicBuffers`·`MusicBuffers`를 실제 COM/토큰 소유권에 연결했다. 소유하지 않는 추가 코덱 바이트는 거부한다. [주소·계약·검증 범위](docs/exe/cpp-music-stream-reconstruction.md).
- [x] **독립 원본:** 10.78 **1,704개**, CD/추가 10.37 각 **1,692개**, 총 **5,088개**. 두 x87 제어값에서 생성/갱신 직접 호출 **10,176회**, 실제 초기 Lookup까지 root **30,528회** 정상 반환. 실제 채우기/Read/Stop/helper를 실행하고 COM/파일 IO/잠금 관찰/기록/패치 CRT 로캘/초기 배치만 대체했다. 활성·준비·status 0~7·앞/뒤/같은 커서·일곱 COM 실패·잠금 구간 0/분할·파일 0/1/8/16/64와 EOF/부분 읽기/되감기 실패를 교차했다. 전체 raw96/output128·사건/반환·파일과 ABI/스택/보존 레지스터/SEH/x87·허용 쓰기·잠금 균형·OS 0·SHA/정확한 입력/행/진입/반환을 감사한다. **10.78 생성 12개는 native 대조, CD 생성은 공개 시작 안의 정적 대조**다. CD 갱신은 음악 준비 검사도 포함하며 void 반환값을 기대값으로 주장하지 않는다. 기존 fixture/감사 도구 미수정, 누적 **435,733개**.
- [x] **검증:** 새 C++ 검사 **7개**, Release 경고/오류 **0**, 전체 CTest 내부 **503개·실패 0**(123.11초, 전체 123.20초), 원본 근거 감사 **77종 모두 통과**. 최종 `--filter SoundMusicStream_` 7개도 통과했다. 실제 시작→첫 채우기/Play→커서 갱신→링 끝 두 구간→짧은 IO→Unlock/Stop, IO 예외/손상 구간의 잠금 반납·미준비/비활성/기존 버퍼·장치 없는 생성 실패와 파일 경계 보존을 확인했다. UTF-8/noBOM·AST/JSON·문서 링크/diff 확인. 추가 변이/실제 장치 검사는 하지 않았다. 기록 `extracted/musicstream-{export,smoke,oracle,build,build-final,tests,tests-final,ctest,ctest-details}.log`·`musicstream-audits.json`.
- [x] **앞 인계 해소/정정:** 버퍼 생성과 링 버퍼 갱신/COM 경계를 완료했다. **CD 00439260은 파일 존재 검사이고 버퍼 생성은 004393b0 내부**이므로 앞 문서/인계의 주소를 수정했다. 실제 음악 파일/작업 스레드/청취 완료는 아니다. 장치는 아직 단일 스레드 전용이며 음악 채널은 장치 Shutdown 전에 Stop해야 한다.
- [ ] **다음 싱글플레이:** ① 음악 파일 열기/헤더·duration `004aa220` / CD `004393b0` 내부, 음악 디렉터리/기본·보조 경로·demo.mus fallback·공개 시작을 실제 파일 토큰 경계에 연결한다. 기존 ReadSoundWave는 표본 전체 적재 방식이므로 음악의 열린 파일/부분 읽기 수명과 원본 float/x87 길이 계산을 별도로 다뤄야 한다. ② 두 채널·이벤트/작업 스레드 초기화 `004aadd0` / CD `00439000`·종료 `004aaf00` / CD `00439130`, 장치 토큰 표 동기화와 종료 대기(원본 강제 중단 검토)를 연결한다. ③ 기존 무음 `netstorm_tests.exe --inspect-sound-device originals` 및 실제 청취/장치 소실/음질 하향/반복 실행은 미검증. ④ HWND/장치 수명·GameWorld 프레임/벽시계·시야/카메라·옵션/경로·Recount 시점·나머지 소리 호출자, raw GUI 건설·경제·전투·AI·승패. **미션 완주는 아직 불가능하다.** outpost/LAN 3차·한국어/D2Coding/화면 요구사항/MCP 4차 유지.

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 음악 채널 제어·공유 음소거 (HJOW-Athlon)

- [x] **착수/호스트:** AGENTS.md·두 LEFT_JOBS·cpppj README/playable-plan·소리 근거를 다시 확인했다. 시작 트리 깨끗함, 현재/마지막 디컴파일 PC 일치. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** 새 `musiccontrol` 목록 **11/13/13개**를 읽기 전용으로 내보냈다. 기존 `soundcontrol`/`soundplay` 호출 몸체도 실제 명령으로 실행했다. 게임/복사본·클론 창·실제 장치/스레드 실행, AGENTS.md·보호 경로·dotnetpj 변경, 커밋/푸시 없음.
- [x] **구현:** [`SoundMusic`](cpppj/src/client/SoundMusic.h)의 `MusicChannelState`(0x60 바이트)·`MusicChannel`(재귀 잠금)·공유 음소거 계층을 추가했다. 읽기/되감기·상태 시작·파일 닫기·중첩 정지/버퍼 참조 해제·채널/전체 음악 음량과 효과음/음악 예약·PushMute/PopMute를 복원했다. `SoundState` 음악 준비/현재·예약 음량을 같은 효과음 제어와 공유한다. 파일 끝을 한 번만 되감기·한 번 재생의 끝을 0으로 채우기·부분 IO 실패 시 출력/파일 토큰/앞 구간 위치 보존·두 판본 +0x3a 패딩 보존·음수/넘치는 깊이·COM 재진입의 판본 분기를 유지했다. [주소·계약·대조 한계](docs/exe/cpp-music-control-reconstruction.md). 실제 파일/COM 경계 연결은 후속이며 Start 자체는 버퍼 Play를 하지 않는다.
- [x] **독립 원본:** 10.78 **1,780개**, CD/추가 10.37 각 **1,756개**, 총 **5,292개**. 두 x87 제어값에서 스크립트 **10,584회**·음악 직접 호출 **44,244회**, Lookup/효과음 음량 포함 root **66,132회** 정상 반환. 음악/공유 음소거·기존 효과음 몸체는 실제 명령이다. COM/Winmm read·seek·close/잠금 관찰/기록·예정 assert·패치 CRT 로캘/초기 배치·명시 COM 재진입만 대체한다. 전체 raw96·출력128·효과음 표·사건/반환·파일 위치/열림·공유 전역과 ABI/스택/보존 레지스터/SEH/x87·허용 쓰기·잠금 균형·OS 0·SHA/전체 입력/직접 반환·몸체 진입을 감사한다. **CD 시작은 파일 열기 끝 인라인의 정적 대조이고, 직접 시작/독립 되감기 입력은 10.78 24개에만 포함한다.** CD Read 내부 되감기는 실제 명령이다. 기존 fixture/감사 도구 미수정, 누적 **430,645개**.
- [x] **검증:** 새 C++ 검사 **6개**, Release 경고/오류 **0**, 전체 CTest 내부 **496개·실패 0**(118.83초, 전체 118.87초), 원본 근거 감사 **76종 모두 통과**. `--filter SoundMusic_` 6개도 통과했다. 실제 시작→파일 끝 통과→부분 IO 실패→정지·중첩 예약값 갱신→최종 복원·잘못된 길이/필수 경계 누락·IO 예외 뒤 잠금 해제를 확인했다. UTF-8/noBOM·AST/JSON·문서 링크/diff 확인. 실제 장치와 추가 변이 검사는 실행하지 않았다. 기록 `extracted/musiccontrol-{export,oracle,build,tests,ctest,ctest-details}.log`·`musiccontrol-audits.json`. 처음 CD 공개 정지의 쓰기 폭을 DWORD로 읽은 구현은 대조에서 실패했다. `004399f4`의 WORD 쓰기와 패딩 보존을 확인해 수정한 뒤 전체 회귀를 완료했다.
- [x] **앞 인계 해소:** 직전의 음악 음량/예약·파일 읽기/되감기·상태 시작/정지·공유 보류 깊이 증감 API를 완료했다. **음악 전체 수명이나 실제 재생은 완료한 것이 아니다.** 원본 실패 경로의 stale File 토큰/재닫기는 후속 실제 자원 경계에서 관리해야 한다.
- [ ] **다음 싱글플레이:** ① 음악 파일 열기/format·duration (`004aa220` / CD `004393b0`)를 실제 파일 토큰 경계에 연결한다. **버퍼 생성 (`004aa040` / CD `004393b0` 내부)·링 버퍼 갱신/COM은 후속 musicstream에서 완료했다.** CD `00439260`은 파일 존재 검사로 주소를 정정한다. 이어 두 채널·이벤트/스레드 초기화 `004aadd0` / CD `00439000`·종료 `004aaf00` / CD `00439130`(원본 강제 중단을 안정 구동 목표에 맞게 검토). ② 기존 무음 `netstorm_tests.exe --inspect-sound-device originals` 및 실제 청취/장치 소실/음질 하향/반복 실행은 아직 미검증. ③ 실제 HWND/장치 수명·GameWorld 프레임/벽시계·시야/카메라·옵션/경로·Recount 호출 시점·나머지 소리 호출자. ④ raw GUI 건설·경제·전투·AI·승패. **미션 완주는 아직 불가능하다.** outpost/LAN 3차와 한국어/D2Coding/화면 요구사항/MCP 4차는 유지한다.

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 소리 제어 여섯 함수 (HJOW-Athlon)

- [x] **착수/호스트:** AGENTS.md·두 LEFT_JOBS·cpppj README/playable-plan·소리 근거를 확인했다. 시작 트리 깨끗함, 현재/마지막 디컴파일 PC 일치. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** 새 `soundcontrol` 목록 **6/6/6개**를 읽기 전용으로 내보냈다. 기존 `soundplay`의 호출 몸체도 실제 실행한다. 게임/복사본·클론 창·실제 장치 실행, AGENTS.md·보호 경로·dotnetpj 변경, 커밋/푸시 없음.
- [x] **구현:** [`SoundPlayer`](cpppj/src/client/Sound.h)에 Recount·StopByName·IsPlayingByName·SetNamePlaying·StopLoops·SetMasterVolume을 복원했다. 자연 종료는 재계산으로 재생 한도를 회복한다. 이름 정지는 진단 뒤 원본 항목만 정지하며 복제는 유지한다. 켜기는 성공 여부와 무관하게 on을 그대로 반환한다. 반복 일괄 정지는 상태를 두 번 조회한 뒤 기존 정지/잃음 검사를 수행한다. 전체 음량은 미초기화에서도 저장하고 적재 버퍼 모두에 범위를 자르지 않고 적용하며 실패 HRESULT를 무시한다. 보류 깊이/예약 음량 두 signed DWORD와 CD 이름 검사/보고 줄 차이를 보존했다. [주소·계약·검증 범위](docs/exe/cpp-sound-control-reconstruction.md).
- [x] **독립 원본:** 각 PE **1,063개**, 총 **3,189개**. 두 x87 제어값에서 스크립트 **6,378회**·직접 함수 호출 **76,458회** 정상 반환. 재계산/이름/반복/음량과 기존 재생·정지·조회·ASCII 몸체는 실제 명령이다. COM/적재/기록/예정 assert/CRT 로캘/표 확보만 명시 대체한다. 상태 비트·세 복제·자연 종료·옵션/초기화·silent/null 이름·on 반환·음량 넘침/보류·조회 사이 장치 변화 입력을 대조했다. ABI/스택/보존 레지스터/SEH/x87·허용 쓰기·표 전체/사건/반환/전역·OS 0·SHA/정확한 입력/진입/정상 반환 감사. 기존 fixture/감사 도구 미수정, 누적 **425,353개**.
- [x] **검증:** 새 C++ 검사 **6개**, Release 경고/오류 **0**, 전체 CTest 내부 **490개·실패 0**(134.34초), 원본 근거 감사 **75종 모두 통과**. 선택 검사 `--filter SoundControl_`도 6개 통과. 실제 계층의 자연 종료→재생 한도 회복·옵션 꺼짐→반복만 정지·복제 유지·표 전체 바이트 보존·보류/범위 미보정·재진입 음량 갱신을 검사했다. UTF-8/noBOM·AST/JSON·문서 링크/diff 확인. 추가 변이 검사와 실제 장치 검사는 실행하지 않았다. 기록 `extracted/soundcontrol-{export,oracle,build-final,tests-final,ctest-final}.log`·`soundcontrol-audits.json`. 초기 보조 검사에서 한도 실패 전 상태 조회 사건 Q를 누락한 기대값을 수정한 뒤 전체 회귀를 완료했다.
- [x] **앞 인계 해소:** 직전 Sound.cpp 효과음 제어 여섯 함수 완료. `Recount` 자동 호출 시점은 원본 클라이언트 호출자를 따라 후속 연결해야 한다. 당시 미완료였던 음악 공유 보류 깊이 증감 API는 후속 musiccontrol 단계에서 완료했다.
- [ ] **다음 싱글플레이:** ① **음악 채널 읽기/상태 시작/정지·음량/예약·공유 음소거는 후속 musiccontrol 단계에서 완료.** 음악 파일 열기·버퍼 생성/스트리밍 갱신과 초기화 `004aadd0`·종료 `004aaf00`는 아직 남았다. ② 기존 무음 명령 `netstorm_tests.exe --inspect-sound-device originals` 및 실제 청취/장치 소실/음질 하향/반복 실행은 아직 미검증이다. ③ 실제 HWND/장치 수명·GameWorld 프레임/벽시계·화면/카메라·옵션/언어/소리 경로·Recount 호출 시점·나머지 소리 요청/프로세스 호출자. ④ raw GUI 건설·경제·전투·AI·승패. **미션 완주는 아직 불가능하다.** 영어 원본 글꼴 유지, outpost/LAN 3차와 한국어/D2Coding/화면 요구사항/MCP 4차를 앞당기지 않는다.

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: DirectSound 장치 코드·WAVE 적재 (HJOW-Athlon)

- [x] **착수/조건:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md와 cpppj README/playable-plan/소리 복원 문서를 다시 읽었다. 시작 트리 깨끗함. 현재/마지막 디컴파일 PC는 HJOW-Athlon으로 일치한다. 앞 단계의 기본 방식인 DirectSound 직접 호출로 장치 코드를 작성했고, 실제 장치 실행은 답이 없으면 인계한다는 앞 방침을 따랐다. 게임/복사본·클론 창·실제 오디오 장치 실행, AGENTS.md·보호 파일·dotnetpj 변경, 커밋/푸시 없음.
- [x] **호스트/내보내기:** **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** 새 `sounddevice` 목록 **7/7/7개**, 읽기 전용 내보내기. 초기화/종료·항목 파일 선택·버퍼 적재와 WAVE 열기/읽기/닫기를 확인했다. [주소/계약/차이/검증 범위](docs/exe/cpp-sound-device-reconstruction.md).
- [x] **실제 장치 코드:** [`SoundDevice`](cpppj/src/client/SoundDevice.h)의 동적 dsound.dll/DirectSoundCreate→DSSCL_PRIORITY→주 버퍼/음질 하향→0xe2 효과음 버퍼/잠금/복사/해제와 상태/재생/위치/음량/좌우/정지/복제를 `SoundDeviceHooks`에 연결했다. 32비트 토큰으로 64비트 COM 포인터를 보관하며 원본의 0/소리 없음 표식을 유지한다. 종료는 이름/사슬/감쇠/이력을 보존하고 버퍼만 비워 재초기화/기존 복제 항목 복원이 가능하다. 소유 버퍼·DLL·mmio를 실패 경로에서도 반납한다. **장치 코드 실행과 GameWorld/음악 연결은 아직 미검증/미완료다.**
- [x] **파일/WAVE:** 언어/기본·주/보조의 여덟 패턴 순서와 첫 일치·전체 경로의 원본 sscanf 감쇠를 복원했다. 실제 Winmm 읽기에서 짧은 fmt의 bits/cbSize 보정과 18바이트 헤더를 보존한다. cbSize=20인 원본 PCM 네 파일을 처음에는 거부했으나 전수 읽기에서 발견해 수정했다. Winmm의 data 크기 축소도 확인했다. 큰/잘린 청크는 Winmm이 반환한 안전한 크기만 읽고 실제 디스크 범위를 추가 검사한다. 비PCM 추가 코덱/0 분모/불완전 fmt는 거부한다. 원본 ANSI 대신 Unicode 디스크 경로를 받는다.
- [x] **독립 원본:** 세 PE 각 **435개·총 1,305개**. 실제 열기/헤더 435·표본 읽기 288·닫기 288회씩, 총 **3,033회 정상 반환**. mmio 다섯 API만 독립 메모리 RIFF 경계로 대체한다. 실제 명령/허용 쓰기·cdecl 스택·보존 레지스터·진입/반환·assert/OS 0·SHA/정확한 입력/행/API 횟수를 감사한다. 정수 처리여서 x87 정밀도 대조는 없다. **DirectSound 초기화/COM/파일 검색은 이 native 기대값의 범위 밖이다.** 기존 fixture/감사 도구 미수정, 누적 **422,164개**.
- [x] **검증:** 새 C++ 검사 **7개**, 최종 Release 경고/오류 **0**, 전체 CTest 내부 **484개·실패 0**(131.49초), 원본 근거 감사 **74종 모두 통과**. 원본 **218 WAV·9,758,513 표본 바이트·감쇠 24개**를 실제 mmio로 전수 읽고 data 바이트와 비교했다. 원본 파일은 읽기만 했다. `--filter SoundDevice_` 선택 검사도 통과했다. 이번 단계의 추가 변이 검사는 하지 않았다. 기록 `extracted/sounddevice-{export,oracle,build-final,tests,assets,ctest-final}.log`·`sounddevice-audits.json`. 초기 거부 기준 실패·실행 중 exe 잠금으로 인한 LNK1104는 수정 전 로그이며 완료 근거가 아니다. 검사 종료 뒤 다시 빌드하고 최종 전체 회귀를 수행했다.
- [x] **앞 인계 해소:** ① 장치 초기화/종료·WAVE 읽기·버퍼 적재 코드와 여덟 장치 경계를 완료했다. 장치 방식은 원본 DirectSound로 구현했다. 실제 실행을 선택할 수 있도록 `netstorm_tests.exe --inspect-sound-device originals`도 작성했다. **아직 실행하지 않았다.** 숨긴 협조 창/실제 장치를 열고 음량 -10000으로 적재·복제·상태/재생/정지·종료/재초기화/복제 항목 복원을 확인한다. 기본 CTest와 `--inspect-sound-files`는 장치를 열지 않는다.
- [ ] **다음 싱글플레이:** ① 준비한 **실제 장치 검사** 및 실제 청취/장치 소실/음질 하향/반복 실행 확인(앞 방침에 따라 실행 여부 답이 없으면 실행은 인계). ② **효과음 제어 여섯 함수는 2026-10-10 후속에서 완료**([최신 근거](docs/exe/cpp-sound-control-reconstruction.md)). 음악 `004a9fa0` 이후와 초기화/종료·공유 음소거는 계속 후속이다. ③ **GUI 클라이언트 연결**: 실제 HWND/장치 수명, GameWorld 프레임·벽시계·화면/카메라·옵션(`sound`·`maxSimulSounds`·`swapLeftRightSpeakers`)·언어/소리 디렉터리·주/보조 경로, 다른 소리 요청 지점/소리 프로세스 호출자. ④ raw 월드/GUI·건설·경제·전투·승패. 미션 완주는 아직 불가능하다.
- [ ] **3·4차/글꼴:** outpost/LAN 3차, 화면비·한국어·60/120프레임·요구사항/MCP 4차 유지. 영어=원본 `.chfnt`·한국어=D2Coding 정책. 한국어 출력/언어 선택은 후속이다.

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 소리 재생 계층 — 전역/위치 재생·정지·화면 위치 계산 (HJOW-Athlon)

- [x] **착수/조건:** AGENTS.md(변경 없음)·LEFT_JOBS.md를 다시 읽었다. 직전 작업은 사용자가 `99d66b0`(1010 - 11)으로 커밋했고 시작 트리는 깨끗했다. 마지막 디컴파일 PC와 현재 호스트는 `HJOW-Athlon`으로 같아 재디컴파일은 하지 않았고 새 내보내기도 같은 PC에서 했다. 앞 절의 "다음 싱글플레이 ① 소리 장치 계층" 가운데 **재생 계층의 논리**를 진행했다. 장치 방식(DirectSound 직접 호출 여부)은 앞 세션에서 물었으나 답이 없어 **문서의 기본 방침(원본 방식)**을 전제로 했고, 장치 호출은 경계(`SoundDeviceHooks`)로 두어 방식을 바꿀 여지를 남겼다. 게임/복사본·클론 창 실행 없음, **소리 장치도 열지 않음**, AGENTS.md·보호 파일·dotnetpj 변경, 커밋/푸시 없음.
- [x] **재생 계층:** [`SoundPlayer`](cpppj/src/client/Sound.h)가 전역 재생 `004a9550`/CD `004383f0`·정지 `004a97e0`/CD `004386f0`·재생 여부 `004a9810`/CD `004387a0`·이름 기반 재생 `004a9cb0`·`004a9d70`/CD `00438b50`·`00438c00`를 복원한다. 우선이 0일 때만 `maxSimulSounds`(기본 8) 한도를 보고, 재생 중인 소리를 다시 재생하면 사슬에서 쉬는 항목을 쓰거나 버퍼를 복제해 새 항목을 만든다. 음량 = 인자 + 전체 음량 − 소리별 감쇠, [-10000, 0]. 복제 항목의 감쇠는 0으로 남는다(원본 그대로). 재생 수는 정지를 호출할 때만 준다. [주소/계약/판본 차이](docs/exe/cpp-sound-play-reconstruction.md).
- [x] **위치 재생/화면 계산:** 위치 반복 `004a9860`/CD `00438800`·위치 한 번 `004a9c30`/CD `00438ae0`, 월드→화면 `00497220`(×16/×11 + 0.5 절삭 − 카메라)·화면 안 `004c6b30`(좌우 40·상하 30 여유)·좌우 `004c6a40`·음량 `004c6ac0`(32비트 넘침 포함)을 복원했다. 위치 반복은 다른 이름이면 현재 소리를 멈추고, 화면 안에서 `Play(소리, 반복, 0, 0, 우선 없음, 한도 1)`로 시작하며, 화면 밖이면 멈추고, 재생 중이면 매번 음량/좌우를 다시 정한다. CD판은 그 재생을 함수 호출로, 패치판은 인라인으로 하지만 관찰이 같다.
- [x] **인자 뜻 확정·고의 고장:** 전역 재생의 둘째 인자 = 반복, 다섯째 = 우선, 여섯째 = 같은 소리 한도임을 기계어로 확정했다(앞 단계의 "추정" 해소). 소리 프로세스의 위치 + 한 번 조합은 `flags & 8`이 반복 자리에 들어간다(원본 그대로). 패치판은 변조 감지 값 `005318ec`가 7이면 null 버퍼로 재생을 불러 일부러 멈춘다 — **옮기지 않았다.**
- [x] **연결:** `MakeSoundProcessHooks`(소리 프로세스 → 재생 계층), `MakePriestShieldNoticeHooks`(`ourPriestImmobile.wav` 우선 전역 재생), `MakePriestFallSound`(`priestFall.wav` 위치 재생). 두 raw 배치에서 실제 보호막 생성 wrapper → 이름 표 → 소리 프로세스 → 재생 계층 → 장치 호출(적재 → 반복 재생 → 위치 갱신 → 화면 밖 정지 → 재진입 재시작 → 자기 삭제 때 정지/확인)을 검사했다. **장치는 상태표 대체다.**
- [x] **독립 원본:** `soundplay` 목록 **19/16/16개**, `HJOW-Athlon`, 2026-10-10 읽기 전용 내보내기. 세 실제 PE 각 **4,824개·총 14,472개**(연산 묶음 1,065/월드→화면 399/화면 점 3,360), 두 x87 정밀도 일치, 각 PE 정상 반환 35,002회. 패치와 CD의 관찰은 기록 문장의 파일 이름·줄 번호와 보고 줄 번호만 다르다. 버퍼 COM 호출(가짜 가상 표로 가로챔)·버퍼 복제·항목 적재·기록 출력·assert 보고·패치 CRT 로캘 선택·표 초기화만 명시 대체. ABI/비영 보존 레지스터/SEH/x87·허용 실행/쓰기·OS 0·SHA/정확한 입력/진입 수 감사. 앞 도구/fixture 미변경, 누적 **420,859개**.
- [x] **변이 확인:** `tools/cpp_mutation_check.py`에 재생 계층 변이 15개(`soundplay-*`)를 추가해 저장소 밖 사본(`extracted/mutation-work`)에서 실행했다. **15개 모두 검출, 미검출 0.** 모든 변이에서 세 판본의 재생 검사가 실패했고, 다섯은 통합/거부 검사도 함께 실패했다. 변이 전 사본은 빌드·검사 통과. 기록 `extracted/soundplay-mutation.log`. 저장소 소스는 변이하지 않았다.
- [x] **최종 검증:** Release 경고/오류 **0**, CTest 내부 **477개·실패 0**(133.26초 — 변이 확인과 함께 돌아 평소보다 길다), 원본 근거 감사 **73종 모두 통과**. 새 C++ 검사 **5개**(첫 빌드/첫 실행에서 통과). 기록 `extracted/soundplay-{export,oracle,build,ctest-final,audits,mutation}.log`·`soundplay-audits.json`.
- [x] **앞 인계 해소:** 앞 절 ①의 재생/정지/재생 여부·위치 재생·화면 안 판정/음량/좌우·이름 기반 재생(`004a9cb0`·`004a9d70`)을 끝냈다. ①의 나머지(장치 초기화·wav 읽기·음악)는 아래로 남는다.
- [x] **후속 구현 완료(실행/음악 제외):** 아래 ①의 장치/WAVE 적재 코드는 맨 위 sounddevice 단계에서 완료했다. 다음은 당시 계획이며 최신 작업 순서는 맨 위 절을 따른다. ① **실제 소리 장치** — DirectSound 초기화 `004aa600`(dsound.dll을 읽어 DirectSoundCreate)·종료 `004a8ef0`, 항목 적재 `004a9170`(소리/언어 폴더에서 `이름-숫자.wav`·`이름.wav` 찾기, 숫자 = 감쇠. 실제 `originals/sound`에 `beamOver-1000.wav` 등 24개) → `004a8f70`(버퍼 만들기·잠금·읽기) → `004a8b50`(mmio WAVE). 이것을 `SoundDeviceHooks`에 채워야 소리가 난다. ② `Sound.cpp`의 나머지: 재생 수 다시 세기 `004a8e60`, 이름 정지/조회/켜고 끄기 `004a9d20`·`004a9da0`·`004a9de0`, 반복 소리 모두 정지 `004a9e50`, 전체 음량 `004a9f10`, 음악(`004a9fa0` 이후). ③ 클라이언트 연결: `GameWorld`가 프레임 카운터·벽시계·화면 영역/카메라·옵션(`sound`·`maxSimulSounds`·`swapLeftRightSpeakers`)을 공급, 다른 소리 요청 지점(`004a9d70` 45곳·`004a9cb0` 59곳) 연결, 소리 프로세스의 다른 다섯 호출자. ④ raw 월드/GUI·저장 맵 비표면 객체/미션 목록 수명, GUI 건설·경제·전투·승패. 미션 완주는 아직 불가능하다.
- [x] **장치 방식/실행 인계 갱신(후속 완료):** 실제 DirectSound 장치 코드는 맨 위 sounddevice 단계에서 완료했다. 실제 장치를 여는 검사는 계속 인계한다. 다음은 당시 결정 요청 이력이다. (1) 소리 장치 방식 — 지금은 원본과 같은 **DirectSound 직접 호출**을 전제로 했다. 다른 방식(예: waveOut/XAudio2)을 원하면 ①을 시작하기 전에 알려 달라. (2) **장치를 실제로 여는 검사** — 소리가 나거나(무음 음량 −10000으로도 가능) 오디오 장치를 점유한다. 이 PC에서 해도 되는지, 클론 창 실행과 함께 다른 PC로 넘길지 정해 달라. 답이 없으면 ①은 장치 코드를 쓰되 장치를 여는 실행은 하지 않고 인계한다.
- [ ] **3·4차/글꼴:** outpost/LAN 3차, 화면비·한국어·60/120프레임·요구사항/MCP 4차 유지. 영어=원본 `.chfnt` 캐시·한국어=D2Coding 정책, Unicode/D2Coding 출력/언어 선택은 후속이다.

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 소리 프로세스·소리 이름 표와 보호막 소리 수명 (HJOW-Athlon)

- [x] **착수/조건:** AGENTS.md·LEFT_JOBS.md를 읽고 LEFT_JOBS.dotnetpj.md 머리 절(다른 PC의 dotnetpj 인계, cpppj와 무관)을 확인했다. 시작 트리 깨끗함. 마지막 디컴파일 PC와 현재 호스트는 `HJOW-Athlon`으로 같아 재디컴파일은 하지 않았고 새 내보내기도 같은 PC에서 했다. 앞 절의 "다음 싱글플레이" 첫 항목(보호막 SfxProcess/파일 조회·소리 재생/정지 수명)을 진행했다. 설명/사용법 우선 한국어 주석·UTF-8·10.78 싱글플레이 우선순위 유지. 게임/복사본·클론 창 실행, AGENTS.md·보호 파일·dotnetpj 코드/인계 변경, 커밋/푸시 없음.
- [x] **소리 프로세스:** [`SoundProcess`](cpppj/src/client/SoundProcess.h)(원본 `Soundprocess.cpp`, 타입 이름 `soundProcessType`·번호 47, 앞 문서들의 "SfxProcess")의 생성자 `004ab240`/CD `00453800`·실행 `004ab070`/CD `00453900`·form 삭제 통지 `004ab030`/CD `004538b0`를 복원했다. flags 1 위치·2 대체 소리(부모 상태 단어)·4 한 번·8 playPriority(값을 그대로 넘기며 재생 함수 안의 뜻은 미확정). 위치 반복은 재생 중이면 매 프레임 재요청하고 아니면 "이 타입이 이번 프레임에 아직 소리를 내지 않았을 때"만 시작한다. 한 번 재생은 재생하지 못해도 끝난다. form이 지워질 때 반복 소리만 정지한다. 벽시계가 시작 시각(+0x20) 전이면 실행하지 않는다. `SoundProcessSystem::Add`가 실제 `SquidProcessHost::Attach`(타입 전역, flags 0x10)로 붙인다. [주소/계약/판본 차이](docs/exe/cpp-sound-process-reconstruction.md).
- [x] **소리 이름 표:** [`SoundList`](cpppj/src/client/Sound.h)(원본 `Sound.cpp`의 bigSoundList 0x8000바이트)의 찾기/추가 `004a8d50`/CD `00437b29`를 복원했다. C 로캘 `_stricmp`(ASCII만 접음)·한도 0x7fb0 이상이면 대체 소리 `nonexistant.wav`·null → 0·표 안 포인터 → 포인터 − 0x1f(뜻 미확인, 값만 재현). `SoundHandle`은 `0x15000000 + 오프셋`(호스트 주소 아님)이어서 표 전체 바이트를 원본 관찰과 그대로 비교한다. 빈 이름/표 끝을 넘는 이름은 원본과 달리 거부한다.
- [x] **판본 차이 보존:** NaN 시각에서 패치판은 실행하지 않고 CD판은 실행한다. 패치판만 debug 전역에서 flags 조합(줄 53·54)을 보고한다. CD판만 이름 표 순회에서 끝 글자가 `v`가 아닌 항목(대문자 `.WAV` 포함)을 `s->isRobust()`(줄 438)로 보고한다. assert 보고는 선택 훅으로 전달하고 원본처럼 실행을 계속한다.
- [x] **보호막 연결:** `MakePriestShieldSoundHooks`가 `RawPriestShield`의 `loadSound`·`attachSound` 경계를 실제 이름 표/소리 프로세스로 바꾼다. 두 raw 배치에서 실제 생성자/소유자/Pop/두 Regular 위에 `priestForceField.wav` 조회(오프셋 44)→**새 보호막에** 소리 프로세스 부착(Kernel 3개)→첫 프레임 위치 반복 재생 요청·타입 프레임 기록→재생 중 매 프레임 재요청→사제 Unpop 뒤 보호막 자기 삭제의 종속 form 정리에서 정지→SID/Kernel 반납→소리 확보 실패 경로를 검사했다. **소리 장치 계층은 기록 대체다.**
- [x] **독립 원본:** `soundprocess` 목록 **10/7/7개**, `HJOW-Athlon`, 2026-10-10 읽기 전용 내보내기. 세 실제 PE 각 **6,127개·총 18,381개**(실행 5,808/통지 144/생성 168/이름 표 묶음 7·조회 124회), 두 x87 정밀도 일치. 각 PE 정상 반환: 실행 11,616·통지 288·생성 336·조회 248회. BaseProcess 부착·벽시계·세 재생·정지/재생 여부·Kill·assert 보고(기록 뒤 계속)만 명시 대체. 패치 CRT의 스레드 로캘 선택은 C 로캘로 넘기고 ASCII 비교 본문은 실제 실행, 표 초기화(확보/0 채움/대체 소리 대입)는 Python 대체. ABI/비영 보존 레지스터/SEH/x87·허용 실행/쓰기·OS 0·SHA/정확한 입력/진입 수 감사. 앞 도구/fixture 미변경, 누적 **406,387개**.
- [x] **변이 확인:** `tools/cpp_mutation_check.py`에 소리 변이 10개(`sound-*`)를 추가해 저장소 밖 사본(`extracted/mutation-work`)에서 실행했다. **10개 모두 검출, 미검출 0.** 패치 전용 두 변이(NaN 시각 규칙·CD 전용 보고)는 패치판 재생 검사만, 나머지 여덟 개는 세 판본 재생 검사가 모두 실패했다. 변이 전 사본은 빌드·검사 통과. 기록 `extracted/soundprocess-mutation.log`. 저장소 소스는 변이하지 않았다.
- [x] **최종 검증:** Release 경고/오류 **0**, CTest 내부 **472개·실패 0**(128.32초 — 변이 확인용 사본 빌드와 함께 돌아 평소보다 길다), 원본 근거 감사 **72종 모두 통과**. 새 C++ 검사 **5개**(첫 빌드/첫 실행에서 통과). 기록 `extracted/soundprocess-{export,oracle,build-final,ctest-final,audits,mutation}.log`·`soundprocess-audits.json`. 소스 대응표 재생성: `Sound.cpp`·`SoundProcess.cpp` "있음"(클라이언트 10 → 12개).
- [x] **앞 인계 해소:** 앞 절의 "보호막 SfxProcess/파일 조회·소리 재생/정지 수명" 가운데 프로세스·이름 표·재생/정지 **요청**까지 연결했다. 실제 소리 출력은 아래 항목으로 남는다.
- [x] **후속 완료(2026-10-10):** 아래 ①의 재생/정지/재생 여부·위치 재생·화면 안 판정/음량/좌우·이름 기반 재생은 맨 위 단계에서 복원했다. 남은 장치 초기화·wav 읽기·음악과 ②~④는 그 단계의 인계를 따른다.
- [ ] **다음 싱글플레이(당시 기록):** ① **소리 장치 계층(`Sound.cpp`의 나머지)** — DirectSound 초기화 `004aa600`, wav 읽기/버퍼 복제, 위치 반복 `004a9860`·위치 한 번 `004a9c30`·전역 재생 `004a9550`·정지 `004a97e0`·재생 여부 `004a9810`, 화면 안 판정/음량/좌우, 전역 안내 소리 `004a9cb0`, 음악. 이것이 붙어야 소리가 난다(외부 라이브러리 없이 원본 방식을 기본으로 하되 방식은 그 단계에서 정한다 — cpp-build.md 6절). ② 소리 프로세스의 다른 다섯 호출자(지연 재생 포함)와 저장/전송 가상 함수. ③ 사제/보호막 raw 모듈과 소리 프로세스를 실제 월드/GUI(`GameWorld`)의 프레임 카운터·벽시계에 연결, 저장 맵 비표면 객체/미션 목록 수명. ④ GUI 건설·경제·전투·승패. 미션 완주는 아직 불가능하다. 실제 자산 전수·장시간 변이·최대 지도/SID 소진·창 픽셀·Windows 반복 실행은 인계한다.
- [ ] **3·4차/글꼴:** outpost/LAN 3차, 화면비·한국어·60/120프레임·요구사항/MCP 4차 유지. 영어=원본 `.chfnt` 캐시·한국어=D2Coding 정책, Unicode/D2Coding 출력/언어 선택은 후속이다.

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 보호막 일반 Pop·Regular 애니메이션·공간 삭제 수명 (HJOW-Athlon)

- [x] **착수/조건:** AGENTS.md·두 LEFT_JOBS·cpppj README/계획/빌드·관련 복원 문서를 확인했다. 시작 트리 깨끗함. 마지막 디컴파일 PC와 현재 호스트는 `HJOW-Athlon`으로 같으며 새 내보내기도 같은 PC에서 했다. 설명/사용법 우선 한국어 주석·UTF-8·10.78 싱글플레이 우선순위 유지. 게임/복사본·클론 창 실행, AGENTS.md·보호 파일·dotnetpj 코드/인계 변경, 커밋/푸시 없음.
- [x] **보호막 일반 Pop/예약:** [`RawForcefieldPostPop`](cpppj/src/o/RawForcefieldLifecycle.h)을 `SquidPostPop::SetForcefieldPostPop`에 등록하면 실제 타입 167/가상 표의 일반 Pop→최초 Regular 사건 1(0.08f)/2(0.01f) 독립 new→공통 장부/깊이를 한 번 처리한다. 첫 확보 실패여도 두 번째와 base를 계속하며 기존 사건을 검색하지 않는다. 미연결/해제·다른 풀·잘못된 SID/free/contained/타입/가상 표를 효과 전에 거부한다. [주소/사용 계약/범위](docs/exe/cpp-forcefield-lifecycle-reconstruction.md).
- [x] **실제 Regular/좌표 타입 finder:** [`RawForcefieldRegular`](cpppj/src/o/RawForcefieldLifecycle.cpp)은 사건 1에서 실제 진행(1,0) 뒤 0.08f, 사건 2에서 현재 좌표 0방향 절삭/현재 사제 타입 DWORD·일반 점 finder의 첫 타입 일치를 조회한다. 소유자/HP/state 별도 필터와 거의 올림을 추가하지 않는다. 존재하면 0.01f, 없으면 자기 가상 destroy(0) 뒤 -1.0f다. 다른 사건은 0(패치 debug=true이면 예외 진단), count/payload는 읽지 않는다. 예약/프레임 진행 어댑터를 실제 ProcessHost/Advance에 연결했다.
- [x] **공간/삭제 수명:** 두 raw 배치에서 실제 보호막 생성자·owner→생성 wrapper의 실제 Pop→회복과 별개의 두 Regular/form/Kernel·중복 생성 방지→실제 프레임 지정/진행·Display→사제 실제 Unpop→존재 검사에서 자기 삭제→공통 pre/postDestroy 장부·해시/Unpop/SID·실행 중인 Regular를 포함한 두 form/Kernel 제거→재생성/외부 삭제를 검사했다. 보호막 프레임/SHP/타입/비용과 사제 등록은 합성 입력이며 사제 vtable/보호막 생성자는 실제 대응이다. 소리 확보 실패 경로이며 오디오/GUI 완료는 아니다.
- [x] **독립 원본:** `forcefieldpostpop` 목록 **7/6/6개**, `HJOW-Athlon`, 2026-10-10 읽기 전용 내보내기. 세 실제 PE 각 **1,208개·총 3,624개**(접두 128/Regular 1,080). 두 x87 정밀도 각 PE 접두 256회·Regular 2,160회, 총 **7,248회** 정상 반환. 실제 좌표 타입 helper/finder 시작 각 720회·Next 1,080회. new/Regular 생성·common postPop·진행·가상 destroy만 명시 대체. 수정 없는 실제 보호막 가상 표·후처리/처리기/좌표/finder/CRT/반환 상수와 효과/raw/float/ABI/보존 레지스터/SEH/x87·허용 실행/쓰기·assert/OS 0·SHA/입력/진입 감사. 앞 도구/fixture 미변경, 누적 **388,006개**.
- [x] **최종 검증:** Release 경고/오류 **0**, CTest 내부 **467개·실패 0**(121.04초), 원본 근거 감사 **71종 모두 통과**. 새 C++ 검사 **5개**. 초기 통합 검사의 SHP loaded=false 입력 오류를 수정한 뒤 최종 전체 검사를 통과했다. 기록 `extracted/forcefieldpostpop-{export,oracle,build-final,ctest-final}.log`·`forcefieldpostpop-audits.json`. 새 파일 UTF-8/BOM 없음/LF·JSON/AST·diff/추가 문서 링크/보호 경로를 확인했고 소스 대응표 재생성 변경 없음.
- [x] **앞 인계 해소:** 일반 보호막 Pop의 전용 후처리/Regular·사제 부재 자기 삭제/공통 삭제 공간 수명을 이번에 연결했다. 일반 사제 Pop/Activate와 낙하/진행의 기존 완료 근거 유지. 아래 과거 단계의 보호막 Pop/destroy 인계는 당시 이력이다.
- [x] **후속 완료(2026-10-10):** 보호막 SfxProcess(소리 프로세스)·파일 조회(소리 이름 표)와 재생/정지 **요청** 수명은 맨 위 단계에서 복원·연결했다. 소리 장치의 실제 재생/정지는 그 단계의 인계를 따른다.
- [ ] **다음 싱글플레이(계속):** 사제/보호막 raw 모듈의 실제 월드/GUI 연결·저장 맵 비표면 객체/미션 목록 수명, GUI 건설·경제·전투·승패. 미션 완주는 아직 불가능하다. 실제 자산 전수·장시간 변이·최대 지도/SID 소진·창 픽셀·Windows 반복 실행은 인계한다.
- [ ] **3·4차/글꼴:** outpost/LAN 3차, 화면비·한국어·60/120프레임·요구사항/MCP 4차 유지. 영어=원본 `.chfnt` 캐시·한국어=D2Coding 정책, Unicode/D2Coding 출력/언어 선택은 후속이다. 앞 단계 영어 18캐시/4,608자/302,480픽셀 대조와 브리핑 굵기 수정은 유지한다.

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 일반 사제 Pop/Activate 연결·영어 원본 글꼴 확인 (HJOW-Athlon)

- [x] **착수/범위:** AGENTS.md·두 LEFT_JOBS·cpppj README/플레이/빌드·관련 복원 문서를 확인했다. 마지막 디컴파일 PC와 현재 호스트는 `HJOW-Athlon`으로 같다. 10.78 싱글플레이 우선순위·한국어 설명/주석·UTF-8을 유지했다. 원본 게임/복사본·클론 창 실행, 보호 파일·AGENTS.md·dotnetpj 코드/인계 변경, 커밋/푸시 없음.
- [x] **사제 일반 Pop/Activate:** [`SquidPostPop::SetPriestPostPop`](cpppj/src/o/SquidPostPop.h)에 같은 풀의 전체 `RawPriestPostPopTail`을 명시 등록한다. 실제 타입 158/가상 표 일치 때 공통 Pop/firstPop→Activate→사제 접두/Carrier/base/후반을 실행하며 공통 장부를 추가 호출하지 않는다. 접두 타입/가상 표/목록 개수는 공간 쓰기 전 검증하고 반환 깊이는 실행 뒤 검사한다. 미연결/해제 상태의 일반 사제 Pop 제한은 유지한다. Carrier는 해당 PostPop 인스턴스의 `PostPopBase`에 연결해야 한다. [주소/사용 계약/검증 범위](docs/exe/cpp-priest-pop-reconstruction.md).
- [x] **실제 모듈 연결:** 두 raw 배치에서 일반 Pop→실제 HP/회복 예약→SharedRegular·낙하 예약→권한 있는 0x800 중첩 Pop→J 진행/실제 프레임 지정·Display·Unpop/Pop→SHP 크기 단계 해시 1→3 이동→착지/Kernel 예약 제거를 검사했다. 비용/개수/사제 목록은 중복 갱신되지 않는다. 생성자는 합성 입력이며 실제 사제 vtable을 사용한다. 보호막/소리는 호출 경계다.
- [x] **독립 원본:** 새 `priestpop` 목록 **24/18/18개**, `HJOW-Athlon`, 2026-10-10 읽기 전용 내보내기. 세 실제 PE의 수정 없는 사제 가상 표·공통 Pop/firstPop/Activate/사제 전체 wrapper를 실행했다. 각 판본 **1,280개·총 3,840개**, 두 x87 정밀도·총 **7,680회** 정상 Pop 반환. 각 PE Pop/Activate/사제 wrapper 2,560회·firstPop 1,280회. Carrier는 flags 기록/base 깊이 감소, Regular/낙하/보호막은 명시 대체. 합성 타입/프레임/SHP/지면·비멀티플레이·표시 억제 경로다. flags/효과 순서/raw 전체/해시/목록/소유자 표·ABI/보존 레지스터/SEH/x87/깊이·허용 실행/쓰기·assert/OS 0·SHA/입력 누락/중복 감사. 앞 fixture/원본 도구 미수정, 누적 **384,382개**.
- [x] **영어 원본 글꼴:** 원본 `.chfnt` 캐시 사용을 확인했다. [원본 캡처 대조](docs/dotnet-reconstruction-20261010.md)에 맞춰 브리핑/안내 기본 본문을 슬롯 5(Arial 14/0)에서 **슬롯 0(Arial 14/700)**으로 수정했다. 제목 3·버튼 0 유지, 이후 실제 도움말 문서 본문은 슬롯 5다(현재 Help는 About 안내만 지원). `cpp_renderer_smoke.py --console-only` 추가/실행으로 **18개 캐시·4,608자·302,480픽셀**의 폭/픽셀 일치·두 PE 커서 표 각 18개·원본 1,394파일 해시 불변을 확인했다. 창/설정 변경 없음. 영어를 D2Coding으로 대체하지 않았다.
- [x] **최종 검증:** Release 경고/오류 **0**, CTest 내부 **462개·실패 0**(115.39초), 원본 근거 감사 **70종 모두 통과**. 새 C++ 검사 **5개**. 첫 빌드의 테스트 타입 이름 충돌을 수정한 뒤 최종 검사를 진행했다. 기록 `extracted/priestpop-{export,oracle,build-final,ctest}.log`·`priestpop-audits.json`·`cpp-renderer-smoke/console-report.json`. 새 파일 UTF-8/BOM 없음/LF·JSON/AST·전체 변경 UTF-8·diff/추가 문서 링크/보호 경로를 확인했고, 소스 대응표 재생성 변경 없음.
- [x] **앞 단계 인계 해소:** 일반 사제 Pop/Activate의 전체 파생 후처리 분배를 이번에 연결했다. Carrier +0xcc/+0xc8·공통 프레임 진행/낙하 지정은 기존 완료 근거를 유지한다. 아래 과거 단계의 일반 Pop 미지원/인계 표기는 당시 이력이다.
- [ ] **다음 싱글플레이:** 보호막 실제 Pop/destroy/소리 수명, 월드/GUI의 raw 사제 연결·저장 맵 비표면 객체/미션 목록 수명, GUI 건설·경제·전투·승패. 미션 완주는 아직 불가능하다. 실제 자산 전수·장시간 변이·최대 지도/SID 소진·창 픽셀·Windows 반복 실행은 계속 인계한다.
- [ ] **한국어/3·4차:** 한국어는 `fonts/D2Coding-Ver1.4.0-20261003-all.ttc`를 사용하는 Unicode 레이아웃/렌더링·언어별 선택이 남았다. 현재 cpppj는 CP1252/256문자 경로다. 영어=원본 캐시·한국어=D2Coding 정책 유지. outpost/LAN(3차), 화면비·한국어·60/120프레임·요구사항/MCP(4차)는 앞당기지 않는다.

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 공통 프레임 진행·사제 낙하 프레임 연결 (HJOW-Athlon)

- [x] **착수/범위:** AGENTS.md·두 LEFT_JOBS·cpppj README/플레이 계획을 다시 확인했다. 호스트 `HJOW-Athlon`은 마지막 디컴파일 PC와 같다. 10.78 싱글플레이 복원 우선순위와 설명/사용법 우선 한국어 주석을 유지했다. 원본 게임/복사본·클론 창 실행, 보호 파일·AGENTS.md·dotnetpj 변경, 커밋/푸시 없음.
- [x] **공통 프레임 진행:** [`RawFrameAdvance`](cpppj/src/o/RawFrameAdvance.h)는 현재 글자 첫 번호/전체 개수·signed DWORD 증분/넘침·한 번 감싸기·보정 여부 반환→실제 `SquidFrame::Set`을 복원했다. 큰 증분을 반복 보정하지 않으며 비연속 글자 배열도 원본 첫 번호/개수로 계산한다. 현재 코드 표를 참조하고 풀/지정기/프레임/방향 오류를 진단한다. [주소/계약/범위](docs/exe/cpp-frame-advance-reconstruction.md).
- [x] **표시/공간/낙하 연결:** `MakePriestFallFrameHooks`는 시작/착지 지정과 J 진행을 같은 실제 지정기로 연결한다. 실제 Factory·SHP 메타·Display·Unpop/Pop의 한 호출 늦은 해시 1→3→1 이동과, 실제 사제 vtable/HP 조회·0x25b·SharedRegular 타입 61/form/Kernel의 J 3→4→5→3→착지/예약 제거를 두 배치에서 검사했다. 사제 검사는 같은 크기 단계/비권한 낙하이며 보호막·소리는 호출 관찰 경계다. 일반 사제 Pop/Activate를 자동 허용하지 않았다.
- [x] **독립 원본:** 새 `frameadvance` 목록 2/4/4개를 `HJOW-Athlon`, 2026-10-10에 읽기 전용으로 내보냈다. 세 PE 각각 7,680개·총 **23,040개**, 각 입력 x87 53/64비트 일치. 실제 진행/지정 각 PE 15,360회, 실제 글자 표 생성 각 PE 4회, 구판 방향 각 15,360회. 정상 진행 반환 46,080회·표 반환 12회. 표시/Unpop/Pop만 기록 대체이며 나머지 몸체/헤더/구간 표는 실제 원본 명령이다. 정수 반환/순서/인자/raw 전체·스택/비영 보존 레지스터/SEH/x87·허용 실행/쓰기·assert/OS 0·SHA/입력 누락/중복 감사. 이전 fixture/원본 감사 도구 미변경, 누적 **380,542개**(기존 Carrier 2,028개 포함).
- [x] **최종 검증:** Release 경고/오류 **0**, 새 C++ 검사 **6개**, CTest 내부 **457개·실패 0**(111.87초), 감사 **69종 모두 통과**. 패치 전용 현재 프레임 범위 검사와 CD의 물리 범위 계약을 구별했다. `cpp_source_map.py` 재생성 결과 원본 소스 대응표 변경 없음. 기록은 `extracted/frameadvance-{export,oracle,build,ctest}.log`·`frameadvance-audits.json`이다.
- [x] **앞 단계 인계 해소:** Carrier +0xcc·사제 +0xc8은 [기존 구현/원본 2,028개](docs/exe/cpp-carrier-check-reconstruction.md)와 감사가 이미 완료돼 있으며 이번 전체 회귀도 통과했다. 프레임 설정/진행 자체와 낙하의 지정/진행 훅은 이번 단계에서 실제 모듈에 연결했다.
- [ ] **다음 싱글플레이:** 일반 사제 Pop/Activate의 파생 postPop 연결·보호막 Pop/destroy/소리 수명. 이어 저장 맵/raw 비표면 객체·미션 목록 수명/GUI 건설·경제·전투·승패. 실제 자산 전수·장시간 변이·최대 지도/SID 소진·창/픽셀·Windows 반복 실행은 계속 인계한다.
- [ ] **3·4차:** outpost/LAN(3차), 화면비·한국어/새 글꼴·60/120프레임·요구사항/MCP(4차)는 앞당기지 않는다.

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 사제 낙하 요청·0x25b·공유 Regular 연결 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 인계·cpppj README/playable-plan/build·관련 복원 문서를 다시 읽었다. 시작 트리 깨끗함. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** 새 `priestfall` 15/15/15개를 읽기 전용으로 내보냈다. 새 코드의 상수/함수/클래스/멤버/반복문은 한국어 설명·사용법을 먼저 적고 변경한 기존 주석도 정비했다. 원본/복사본 게임·클론 창 실행, AGENTS.md·보호 파일·dotnetpj 코드/인계 변경과 커밋/푸시 없음.
- [x] **낙하 요청/공유 예약:** [`RawPriestFall::Begin`](cpppj/src/o/RawPriestFall.h)은 보호막→new 성공 때 SharedRegular(타입 61, 0x25b, payload 0.01f)→생성 다음 현재 권한의 Unpop(0)/Repop(0x800)→현재 좌표 priestFall.wav 요청 순서를 복원한다. 확보 실패여도 공간/소리 요청을 계속하며 기존 예약은 조회하지 않는다. [`SquidProcessHost::AddSharedRegular`](cpppj/src/o/SquidProcess.h)은 기존 Regular의 실행 몸체·실제 form/부모 체인·Kernel을 재사용하고 부착 다음 현재 시각/count 0을 설정한다. 공유 타입의 네트워크 저장/복원 가상 함수는 미복원이다.
- [x] **0x25b/연결:** 현재 이동 불가이면 보호막 요청→효과 다음 현재 signed 방향이 J이면 (1,0) 진행, 아니면 타입 +0x154 프레임 설정→현재 좌표의 0.9999f 거의 올림 WORD 지도→지면이 있으면 현재 타입 +0x140 설정/이동 불가 재조회/보호막 삭제 요청과 0 반환, 없으면 0.1f 반환. count/payload는 읽지 않는다. 실제 회복 분배와 fallback으로 합성하며 미복원 사제 사건은 거부한다. 일반 Pop/Activate의 사제 지원은 확대하지 않았다. [주소/계약/검증 범위](docs/exe/cpp-priest-fall-reconstruction.md).
- [x] **실제 모듈 검사 추가:** 두 raw 배치의 실제 factory/owner/보호막 finder→공유 form 타입 61·현재 시각 부착/검색→중복 낙하 요청/새 예약 제거 후 이전 예약 유지→Kernel 첫 실행/재예약/이른 시각 생략→지면 도착의 실제 보호막 조회/삭제 요청→form 삭제/Kernel 제거를 검사한다. 공간/프레임/보호막 Pop/destroy 경계는 명시 입력이며 전체 GUI/오디오 수명 완료가 아니다.
- [x] **독립 원본:** 세 PE 각 456개·총 **1,368개**, 두 x87 정밀도 각 PE 912회·총 2,736회 정상 반환. 요청·실제 SharedRegular 생성자/vtable·0x25b 분배/이동 불가/HP/방향/지도/CRT는 실제 명령이다. new·BaseProcess 부착·보호막·Unpop/Repop·프레임·위치 소리·패치 보호막 find/destroy만 명시 대체한다. 각 PE 실제 요청 192회·생성 96회·0x25b 몸체 720회·이동 불가 1,110회·방향/지도 720회, 패치 유효 보호막 가상 삭제 114회. ABI/보존 레지스터/SEH/x87·실행/쓰기 제한·전체 raw/사건/반환 비트/권한·assert/OS 0·SHA/행/입력 감사. 기존 fixture/감사 도구 미수정. 누적 인계 **355,474개**.
- [x] **최종 검사:** 새 C++ 검사 5개 포함 최종 Release 경고/오류 **0**, CTest 내부 **446개·실패 0**(118.93초), 원본 근거 감사 **67종 모두 통과**. UTF-8/LF/AST/JSON·문서 링크/보호 경로/diff 검사도 통과했다. 로그 `extracted/priestfall-export.log`·`priestfall-oracle.log`·`priestfall-build-final.log`·`priestfall-ctest-final.log`·`priestfall-audits.json`. 처음 Release 빌드도 성공했으며 편집 완료 뒤 최종 빌드/전체 CTest를 수행했다. 약 26분 범위로 이번 단계를 완료했고 장시간/창 검사는 인계한다.
- [ ] **다음 싱글플레이:** Carrier +0xcc의 실제 상태 검사·일반 사제 Pop·프레임 설정/진행의 공간 효과·보호막 Pop/destroy/소리 수명. 이어 저장 맵/raw 비표면 객체·미션 목록 수명/GUI 건설·경제·전투·승패. 자산 전수·장시간 변이·최대 지도/SID 소진·창/픽셀·Windows 반복 실행은 계속 인계한다.
- [ ] **3·4차:** outpost/LAN(3차), 화면비·한국어/새 글꼴·60/120프레임·요구사항/MCP(4차)는 앞당기지 않는다.

---

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 사제 postPop 후반·전체 wrapper 연결 (HJOW-Athlon)

- [x] **다시 읽은 지침/우선순위:** AGENTS.md·두 인계·cpppj README/playable-plan/build 주석 규칙과 관련 복원 문서를 다시 읽었다. 한국어·요구사항/MCP는 4차, outpost/LAN은 3차다. 기존 주석은 설명 대상을 바꿀 때만 정비하고 새 코드의 클래스/구조체·멤버함수/상수/함수/반복문에는 설명·사용법을 먼저 한국어로 적었다. dotnetpj의 의도적 차이/글꼴 교체 결정은 보존하고 해당 코드는 수정하지 않았다.
- [x] **호스트/내보내기:** 현재/마지막 PC 일치, 시작 트리 깨끗함. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** 새 `priestpostpoptail` 18/20/20개를 읽기 전용으로 내보냈다. 원본/복사본 게임·클론 창 실행, AGENTS.md·보호 파일·dotnetpj 코드/인계 변경과 커밋/푸시 없음.
- [x] **전체 wrapper 조합:** [`RawPriestPostPopTail`](cpppj/src/o/RawPriestPostPopTail.h)이 실제 목록/회복 접두→Carrier/Damageable/공통 장부→현재 extra/이동 불가→0.9999f 거의 올림 지면/프레임 J→낙하/보호막 생성·삭제 요청을 연결한다. Carrier는 extra 차단과 무관하게 항상 같은 flags로 호출한다. `0x800`은 지면/프레임 조회 자체를 생략하며 낙하 뒤 extra 변경에도 선택한 보호막 요청은 유지한다. signed 프레임 side와 HP 조회와 낙하 조건은 같은 0.9999f 거의 올림 칸을 읽으며, 중간 합을 float으로 좁히거나 단순 절삭으로 바꾸지 않는다. [계약/주소/후속 범위](docs/exe/cpp-priest-postpop-tail-reconstruction.md).
- [x] **실제 모듈/미복원 경계:** ProcessHost/Form/Kernel·공통 비용/통계/깊이·factory/owner/보호막 조회/중복 방지→이동 가능한 보호막 삭제 요청→강제 이동 불가 재생성을 두 판본에서 통합 검사한다. Pop/destroy 경계는 등록/해제 입력을 공급하며 낙하는 전체 외부 효과다. 같은 풀/표 크기·누락 효과 검사를 넣었고 일반 Pop의 사제 가상 표를 확대하지 않았다. 전체 공간/GUI/오디오 완료가 아니다.
- [x] **독립 원본:** 세 PE 각 2,048개·총 **6,144개**, 두 x87 정밀도 일치. 전체 wrapper/실제 접두 목록·HP·소유자 칸/이동 불가·지면·프레임·CRT를 정상 ret 4까지 실행한다. Regular 검색/new/생성과 Carrier·낙하·보호막 생성/삭제만 명시 대체한다. 각 PE 정상 반환/Carrier 4,096회·실제 이동 불가 1,696회·지면/HP 상태 864회·방향 400회, 낙하 264회·보호막 생성 1,440회·삭제 256회. ABI/보존 레지스터/x87/SEH·허용 실행/쓰기·raw/목록/소유자 칸·assert/OS 0·SHA/행/입력/진입 수 감사. 누적 인계 **354,106개**, 기존 fixture/감사 도구 미수정.
- [x] **최종 검사:** 새 C++ 검사 6개 포함 Release 경고/오류 **0**, CTest 내부 **441개·실패 0**(119.45초), 원본 근거 감사 **66종 모두 통과**. UTF-8/AST/JSON/LF·문서 링크/보호 경로/diff 검사도 통과했다. 초기 단순 절삭 구현은 원본 fixture 재생에서 실패했고 실제 fadd(0.9999f)를 확인하여 수정한 뒤 전체 회귀가 통과했다. 원본 기대값은 변경하지 않았다. 최종 로그 `extracted/priestpostpoptail-export.log`·`priestpostpoptail-oracle.log`·`priestpostpoptail-build-final.log`·`priestpostpoptail-ctest-final.log`·`priestpostpoptail-audits.log`. 이전 `priestpostpoptail-ctest.log`는 수정 전 실패 기록이며 완료 근거로 쓰지 않는다.
- [x] **낙하 예약/처리 후속 완료(2026-10-10):** 맨 위 최신 단계의 `RawPriestFall`·공유 Regular 연결로 요청/0x25b 분기·예약/종료를 복원했다. 프레임/공간/소리 전체 효과·Carrier +0xcc·일반 사제 Pop와 raw GUI/싱글플레이 완주는 계속 인계한다.
- [ ] **3·4차:** outpost/LAN(3차), 화면비·한국어/새 글꼴·60/120프레임·요구사항/MCP(4차)는 앞당기지 않는다.

---

## 2026-10-10 ✅ 완료(문서만): AGENTS.md 최근 변경을 문서·진행 계획에 반영 (HJOW-Athlon)

- [x] **요청/범위:** "AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 먼저 읽고 AGENTS.md 최근 변경을 문서와 cpppj·dotnetpj 진행 계획에 반영". **문서만** 고쳤다. 코드·fixture·원본·AGENTS.md 변경, 빌드/검사, 게임/창 실행, 새 디컴파일, 커밋/푸시는 없다. 현재 PC와 마지막 디컴파일 PC는 `HJOW-Athlon`으로 같다. 아래 cpppj 구현 상태(검사 435개·감사 65종)는 바로 아래 절 그대로다.
- [x] **확인한 AGENTS.md 변경(git 이력)과 반영 위치:**

  | 커밋(KST) | 변경 | 이번 작업 전 | 반영한 곳 |
  |---|---|---|---|
  | `7e24648` 2026-10-10 00:20 | cpppj **4차 목표에 한국어 지원 추가**가 명시됐다 | 목표 이름은 "요구사항/MCP"였고 한국어는 세부 항목으로만 있었다 | 이 문서 머리말, [README.md](README.md), [cpppj/README.md](cpppj/README.md), [cpp-playable-plan.md](docs/cpp-playable-plan.md), [cpp-roadmap.md](docs/cpp-roadmap.md), [cpp-build.md](docs/cpp-build.md) |
  | `7e24648` 2026-10-10 00:20 | **주석 규칙 강화**: 대상에 클래스·멤버함수 추가, 설명·사용법이 주 내용, 추가/수정 시점·이유는 그 뒤, cpppj·dotnetpj 공통 | 미반영(옛 "상수·함수·반복문") | 이 문서 머리말, [cpp-build.md 5절](docs/cpp-build.md#5-다시-만든-코드의-규칙), cpp-roadmap, cpp-playable-plan, [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md) 12절 |
  | `f77a6f9` 2026-10-09 23:45 | cpppj 목표를 1~4차로 구분, dotnetpj는 원본과 완전히 동일한 동작 먼저·Linux 후순위 | 이 문서·README·cpppj/README·cpp-playable-plan에는 반영돼 있었다(바로 아래 절). **cpp-roadmap·cpp-build·cpp-screen-reconstruction·LEFT_JOBS.dotnetpj.md에는 미반영** | 그 네 문서, [README.en.md](README.en.md), 이 문서 2026-10-05 인수인계 절의 갱신 메모와 5절 4번(멀티플레이 범위) |
  | `8e35aec` 2026-10-09 15:22 | 기존 게임을 디컴파일해 알아낸 정보도 팬게임보다 중요하다 | 계획 문서에 명시가 없었다 | 이 문서 머리말, LEFT_JOBS.dotnetpj.md 맨 위 절 |
  | `559f290` 2026-10-08 16:48 | D2Coding 마이너 업그레이드판을 `fonts/`에 넣었다 | 문서·코드 모두 1.3.2만 언급 | 이 문서 머리말·1.5절, LEFT_JOBS.dotnetpj.md 맨 위 절·10절 |

- [x] **cpppj 계획에 반영한 것:** (1) 4차 목표 = 요구사항 반영 + MCP + **한국어 지원**. playable-plan 8단계와 roadmap 7·8단계에 대응을 적었다. 한국어 작업 항목은 영어·한국어 선택, `<미션>.korean` 확장자, 용어표·UI 문구·줄바꿈, 원본 `.chfnt` 영어 글꼴 + D2Coding 한국어 폴백이다. 1~3차가 끝나기 전에는 앞당기지 않는다. (2) roadmap의 단계 표에 목표 단계 대응과 2차(Windows 10/11 안정 실행)·3차(TCP/IP 로컬 멀티플레이·outpost) 행을 넣었다. (3) **다음 싱글플레이 구현 순서는 바뀌지 않는다** — 바로 아래 절의 "다음 싱글플레이 구현"을 그대로 잇는다.
- [x] **dotnetpj 계획에 반영한 것:** [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md) 맨 위 2026-10-10 절과 0·1·9·10·12절. "원본과 완전히 동일한 동작 → 요구사항 → Linux" 순서, 9절의 의도적 차이 재검토 대상, 주석 규칙, 글꼴 1.4.0, cpppj MCP(4차)가 생기기 전까지의 대조 수단.
- [ ] **주석 적용 현황(cpppj·dotnetpj 공통):** 이번 cpppj 후속의 새 코드/수정 대상에는 적용했다. dotnetpj 다음 코드 변경에도 계속 적용한다. 새로 만들거나 고치는 코드의 클래스/구조체·멤버함수에도 한국어 주석을 달고, 설명·사용법 → 원본 근거 → 추가/수정 이력 순서로 쓴다.
- [x] **사용자 결정(2026-10-10) — 기존 코드의 주석 정비:** **그 주석이 설명하는 대상(상수·함수·클래스·멤버함수·반복문)을 수정할 때** 새 규칙에 맞춘다. 일괄 소급 정비는 하지 않는다. 참고로 어림 조사(2026-10-10, 정규식 기반이라 정확한 수는 아니다)에서 `cpppj/src` 헤더 103개의 class/struct 선언 258개 가운데 약 197개가 바로 위/같은 줄에 주석이 없었고, dotnetpj는 타입 선언 322개 중 2개만 없으며 날짜로 시작하는(이력이 설명보다 앞선) 주석이 약 25줄이었다.
- [x] **사용자 결정(2026-10-10) — D2Coding 1.4.0 교체: 중요도가 낮다.** 당분간 dotnetpj는 1.3.2를 그대로 쓴다(파일명이 코드 3곳에 고정돼 있다). 교체할 때 확인할 것은 LEFT_JOBS.dotnetpj.md 10절 7번에 있다. `analyzeManager/GuidedForm.cs`는 `fonts/D2Coding*.ttc` 가운데 처음 열거된 파일을 쓰므로, 두 파일이 함께 있는 지금은 어느 판이 선택될지 열거 순서에 달려 있다.
- [x] **사용자 결정(2026-10-10) — dotnetpj의 의도적 차이:** 원본처럼 동작하도록 바꾼다. **지금 구조를 둔 채 원본 게임의 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다.** 대상(24Hz 고정 틱·입력·설정 저장·프레임 대기·전체 다시 그리기·창 동작)과 항목별 판정 절차·제안 순서는 [LEFT_JOBS.dotnetpj.md 9-1](LEFT_JOBS.dotnetpj.md#original-parity-plan)에 적었다. **아직 어느 항목도 판정하지 않았다.** [main-loop.md](docs/exe/main-loop.md) 7절의 2026-10-05 판단("충돌하지 않는다")에는 재판정 메모를 달았다.
- [ ] **참고:** AGENTS.md 68·70행의 "Outpust"는 Outpost의 오타로 읽었다. AGENTS.md는 에이전트가 수정하지 않는다.

---

## 2026-10-10 ✅ 완료 / ⏭ 부분 인계: 변경 목표 반영·사제 보호막 생성 (HJOW-Athlon)

- [x] **목표/우선순위 반영:** 수정된 AGENTS.md와 두 인계·cpppj 문서를 먼저 읽었다. cpppj는 **10.78 싱글플레이 최대한 복원 → Windows 10/11 오류 없는 실행 → TCP/IP LAN/outpost → 요구사항/MCP** 순서를 따른다. outpost는 멀티플레이 전용이며 전용 후속 구현은 3차로 옮겼다. 공식 서버 접속 복원은 대상이 아니다. cpppj Windows 전용·dotnetpj 원본 동일 동작 우선/이후 요구사항·Linux 후순위·MCP의 dotnetpj 분석 목적도 README/계획에 반영했다. 기존 outpost 코드/근거를 제거하지 않았다.
- [x] **호스트/내보내기:** 현재 PC와 마지막 분석 PC는 `HJOW-Athlon`으로 일치하며 시작 작업 트리는 깨끗했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** KST 자정 전에 새 `priestshield` 15/13/13개를 읽기 전용으로 내보내고 자정 뒤 구현/대조를 마쳤다. 게임/창 실행·보호 파일/AGENTS.md/dotnetpj 코드 변경·커밋/푸시는 없다.
- [x] **보호막 생성 wrapper:** [`RawPriestShield`](cpppj/src/o/RawPriestShield.h)가 실제 보호막 조회→현재 타입 flags 2 생성→생성 뒤 좌표/소유자 캡처→가상 Pop 0→소리 new(0x28) 성공/실패→새 보호막 SID의 소리 프로세스→현재 로컬 owner DWORD/로딩/double 0 조건·패치 번역/창 요청을 복원한다. 이미 보호막이 있으면 모든 효과를 생략한다. NaN/부호 있는 0 좌표 전달·+0/-0/NaN 시각·소리 조회의 실제 cdecl ABI를 보존한다. [주소·계약·미복원 경계](docs/exe/cpp-priest-shield-reconstruction.md).
- [x] **실제 모듈 연결:** 같은 풀의 `SquidFactory::Create`/원본 타입 167 생성자·공통 초기화와 `SquidOwner::Set`을 연결했다. 실제 lookup→중복 생성 방지→사제 preDestroy의 보호막 조회/삭제 요청→재생성·소리 확보 실패/성공을 두 판본에서 검사했다. 통합의 Pop/destroy 경계는 등록/해제 입력을 공급하며 전체 가상 공간 몸체의 완료는 아니다. 일반 Pop 가상 표 지원을 넓히지 않았다. 누락 경계/잘못된 생성 SID/다른 풀도 거부한다.
- [x] **독립 원본 관찰:** 세 PE 각각 832개·총 **2,496개**, 두 x87 정밀도 일치. 전체 wrapper·실제 lookup/finder/좌표/CRT·공통 owner는 정상 반환까지 원본 명령이며 생성/Pop·소리 할당/조회/프로세스·전역 소리·번역/창만 명시 대체한다. 각 PE 정상 반환/lookup 1,664회·실제 owner/생성/Pop/new 각각 1,280회·소리 조회/프로세스 640회·로컬 소리 224회, 패치 번역/창 각 224회(CD 없음). ABI/x87/SEH FS:[0]·허용 코드/쓰기·해시 불변·raw/전역/사건·assert/OS 0·SHA/입력/행/실제 진입 감사. 누적 인계 **347,962개**, 기존 fixture/감사 도구 미수정, 새 근거 UTF-8/LF.
- [x] **최종 검사:** Release 경고/오류 **0**, CTest 내부 **435개·실패 0**(118.77초), 감사 **65종 모두 통과**. 새 C++ 검사 5개. 전체 빌드 완료 후 새 실행 파일로 검사했으며 조기 중단한 `priestshield-ctest.log`는 완료 근거로 사용하지 않는다. 최종 로그 `extracted/priestshield-export.log`·`priestshield-oracle.log`·`priestshield-build.log`·`priestshield-ctest-final.log`·`priestshield-audits.log`. 변경 24개 파일의 UTF-8/AST/JSON·신규 fixture/근거 LF·문서 링크·보호 경로·git diff 검사도 통과했다.
- [x] **후속 완료:** 위 2026-10-10 전체 wrapper 조합에서 실제 Carrier/이동 불가·지면·프레임·낙하 경계/보호막 생성·삭제 요청 결합을 완료했다. **남은 구현:** 실제 낙하 몸체·Carrier +0xcc·일반 사제 Pop·보호막 자체 공간 수명/소리 프로세스. 이어 저장 맵/raw 객체·공간 장부/미션 수명·GUI 건설·경제·전투·승패. 생성 wrapper 복원을 전체 사제/보호막 플레이 완료로 해석하지 않는다.
- [ ] **3차 목표로 연기:** outpost 전용 일반 Pop/삭제·별도 목록 수명과 TCP/IP LAN 멀티플레이. 공통 작업장/지역 처리는 싱글플레이에 필요한 범위에서만 이어 간다.
- [ ] **장시간 검증:** 자산 전수·장시간 변이·최대 지도/SID 소진·창/픽셀 회귀·Windows 10/11 반복 실행은 계속 인계한다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: outpost·작업장 지역 소유 투표 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 인계·cpppj 문서를 읽고 현재 호스트/마지막 분석 호스트 일치와 깨끗한 기준선을 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `regionownership` 10/8/8개 읽기 전용 내보내기. 게임/창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시는 없다.
- [x] **투표 결정 복원:** [`RawRegionOwnership`](cpppj/src/o/RawRegionOwnership.h)이 현재 추가 outpost/작업장 목록과 raw 좌표/소유자·중복 SID·소유자 0~8 표·동률 중립을 처리한다. 패치 mode 0의 마지막 일치 작업장 테마/옵션·mode 10의 중복 영향 지역 누적 표/1단계 재귀·두 도장/revision DWORD 감김/dirty와 CD 단일 도장을 구별한다. [주소·계약·남은 경계](docs/exe/cpp-region-ownership-reconstruction.md).
- [x] **실제 모듈 연결:** 원본 0.99999 float snap을 실제 `RawCanonPlacementTerrain::RegionAt`의 WORD SID/genus/다리 sentinel/빈 칸 127에 연결했다. 도장 좌표는 편향 없이 절삭한다. outpost 생성자→첫 등록→작업장과 동률/중립→실제 Damageable 삭제 준비→남은 작업장 소유자/테마 복귀를 두 판본에서 검사했다. 다른 풀/누락 경계/손상 입력도 검사한다. 일반 Pop 지원 가상 표는 바꾸지 않았다.
- [x] **독립 원본 관찰:** 세 PE 각각 440개·총 **1,320개**, 두 x87 정밀도 일치. 전체 wrapper/재귀·실제 getter/CRT/clear/drawer 생성자를 cdecl 정상 반환까지 실행하며 **지형 flood/도장과 theammode 조회만 명시 대체**한다. 각 PE 정상 반환 880회, 패치 실제 wrapper 1,168회(재귀 288)·도장 2,336회·옵션/테마 getter 984회·지역 getter 13,152회, CD/10.37 도장 880회·지역 getter 8,592회. assert/OS 0·ABI/x87·raw/목록 불변·순서/전역·SHA/입력/행/실제 진입 감사. 누적 fixture **345,466개**, 기존 fixture/감사 도구 미수정, 새 근거 LF 고정.
- [x] **최종 검사:** Release 경고/오류 **0**, CTest 내부 **430개·실패 0**(120.90초), 감사 **64종 모두 통과**. 새 C++ 검사 5개. 로그 `extracted/regionownership-export.log`·`regionownership-oracle.log`·`regionownership-build.log`·`regionownership-ctest.log`·`regionownership-audits.log`. UTF-8·Python AST·JSON·LF·신규 문서 링크·보호 경로·git diff 검사도 통과했다.
- [ ] **우선순위 변경 인계:** 지역 flood/도장 몸체·실제 타입 +0x98 테마/옵션 자료 연결·작업장 postPop/Regular·미션 목록 확보/해제·저장 맵/raw outpost 일반 Pop/삭제 연결. outpost 전용 Pop/삭제·목록 수명은 3차 목표로 연기한다. 공통 작업장/지역 처리는 싱글플레이에 필요한 범위에서 진행한다. 이어 사제 Pop·보호막 생성/회복 예약·Carrier와 GUI 건설·경제·전투·승패. wrapper 자체 revision만 복원했으며 도장 내부 증가/지형 변화·실제 플레이 완료를 주장하지 않는다.
- [ ] **장시간 검증:** 여러 판본 자산 전수·장시간 변이·최대 지도/SID 소진·창/픽셀 회귀는 계속 인계한다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: outpost의 배치 추가 목록 등록·삭제 준비 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 인계·cpppj 문서를 읽고 호스트 일치/깨끗한 기준선을 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `outpostlifecycle` 4/2/2개 읽기 전용 내보내기. 게임/창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시는 없다.
- [x] **별도 목록 정체/접두 복원:** [`RawOutpostLifecycle`](cpppj/src/o/RawOutpostLifecycle.h)이 실제 타입 126 outpost의 postPop/preDestroy를 복원한다. 배치 추가 목록 `0055a70c`/CD `005670e0`와 별도 거리 후보 `0059a9d8`/CD `00565ae0`를 factories와 구분한다. 첫 Pop/extra 조건·용량/중복 없는 추가·모든 중복 제거/순서/비활성 꼬리·지역 통지 뒤 같은 flags의 부모 호출을 유지한다. [주소·계약·남은 경계](docs/exe/cpp-outpost-lifecycle-reconstruction.md).
- [x] **실제 모듈 연결:** 등록/중복 등록/삭제/재등록에 따라 같은 additional 목록을 읽는 실제 Player 종속·그래프 기준점의 반환이 즉시 변한다. 실제 outpost 생성자 쓰기와 Damageable 붕괴·소리·공통 부모 호출을 합성 장면에서 연결했다. 다른 풀/필수 훅/손상 두 번째 목록/부모 사전 거부/조기 읽기 생략도 검사한다. 일반 Pop 가상 표 지원 범위는 바꾸지 않았다.
- [x] **독립 원본 관찰:** 세 PE 각각 768개·총 **2,304개**, 두 x87 정밀도 일치. wrapper/실제 Array는 ret 4까지 실행하며 지역 소유 투표·작업장 postPop·Damageable 부모 진입만 명시 대체한다. 각 PE 정상 반환/부모 대체 1,536회·지역 대체 672회, 패치 실제 Array 추가 576회/제거 768회(CD 인라인). assert/OS 0·ABI/x87·목록 전체/꼬리/raw/사건·SHA/입력/행/실제 진입 감사. 누적 fixture **344,146개**, 기존 fixture/감사 도구 미수정, 새 근거 LF 고정.
- [x] **최종 검사:** Release 경고/오류 **0**, CTest 내부 **425개·실패 0**(112.07초), 감사 **63종 모두 통과**. 새 C++ 검사 5개. 최초 검사에서는 테스트가 미할당 SID에 AllocatedBytes로 쓰려다 실패해 독립 raw 입력 어댑터로 바로잡았다. 원본 관찰값을 수정하지 않았다. 로그 `extracted/outpostlifecycle-export.log`·`outpostlifecycle-oracle-final.log`·`outpostlifecycle-build-final.log`·`outpostlifecycle-ctest-final.log`·`outpostlifecycle-audits-final.log`.
- [x] **후속 부분 완료:** 지역 소유 투표 결정은 위 regionownership 단계에서 복원했다. 지형 도장 몸체는 후속이다.
- [ ] **우선순위 변경 인계:** 지역 지형 flood/도장·작업장 postPop/Regular·미션 목록 확보/초기화/반납과 실제 저장 맵/raw 세계의 outpost 일반 Pop/삭제 연결. outpost 전용 Pop/삭제·목록 수명은 3차 목표로 연기한다. 공통 작업장/지역 처리는 싱글플레이에 필요한 범위에서 진행한다. 이어 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사와 GUI 건설·경제·전투·승패. 이번 목록 접두 완료를 전체 객체/공간 수명이나 실제 플레이 완료로 해석하지 않는다.
- [ ] **장시간 검증:** 여러 판본 자산 전수·장시간 변이·최대 지도/SID 소진·창/픽셀 회귀는 계속 인계한다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 전체 MayPlace 연결·독립 원본 대조 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 인계·cpppj 문서를 읽고 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canonmayplace` 56/51/51개를 읽기 전용으로 내보냈다. 게임/창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시는 없다.
- [x] **전체 모듈 연결:** [`RawCanonPlacementPipeline`](cpppj/src/o/RawCanonPlacementPipeline.h)이 실제 모양·decoder·미리보기·finder·충돌·지형·Player/주변·최종 관계를 연결한다. 현재 표면/해시 0단계 공유와 공통 관계/편집기/graphReady/noIsland 동기화, 내부 주소를 보존하는 복사/이동 금지를 반영했다. [주소·자료 계약·한계](docs/exe/cpp-canon-mayplace-reconstruction.md).
- [x] **전체 원본 독립 실행:** 세 PE 각각 144개·총 **432개**, 두 x87 정밀도 일치. 진입부터 ret 24/보존 레지스터/FS/x87/메모리 균형까지 확인한다. 픽셀/decoder/finder/충돌/지형/Player/주변/최종 helper 몸체는 실제 실행하고 **임시 160바이트 확보/반납만 대체**한다. assert/OS 0, 누적 fixture **341,842개**, 기존 fixture/감사 도구 미수정, SHA/입력/진입/출력 감사.
- [x] **실제 자산 제한 연결:** 새 콘솔 `--inspect-canon-mayplace`와 독립 [`cpp_canonmayplace_smoke.py`](tools/cpp_canonmayplace_smoke.py)로 sunArcher/sunBlocker/sunFactory/priest/bridge/noIsland/island 7종의 실제 TYPE·논리 코드·SHP·hotspot·패턴을 검사했다. 두 판본 각각 **480개·합계 960개** 일치하며 누적 fixture에는 포함하지 않는다. 주변은 내장 타입 10/11/12의 합성 입력이다. 실제 자산용 관찰 도구의 긴 프레임 표/헤더 중첩을 수정했고, CD 긴 다리의 지도 밖 spot 읽기는 검사 범위에서 제외하여 공통 경계를 250으로 제한했다.
- [x] **최종 검사:** Release 경고/오류 **0**, CTest 내부 **420개·실패 0**(122.18초), 감사 **62종 모두 통과**. 새 C++ 검사 3개가 세 PE 전체 432행을 실제 파이프라인으로 재생한다. 로그 `extracted/canonmayplace-export.log`·`canonmayplace-oracle-final.log`·`canonmayplace-build-final.log`·`canonmayplace-ctest-final.log`·`canonmayplace-audits-final.log`·`canonmayplace-assets.log`·`cpp-canonmayplace-assets-report.json`.
- [x] **후속 부분 완료:** 별도 Player 추가 목록의 outpost 등록/삭제 준비 접두는 위 outpostlifecycle 단계에서 복원했다.
- [x] **후속 부분 완료:** 지역 소유 투표 결정은 위 regionownership 단계에서 복원했다.
- [ ] **우선순위 변경 인계:** 지역 지형 flood/도장·작업장 postPop/Regular·미션 목록 확보/해제와 실제 맵/raw outpost 일반 Pop/삭제 연결, outpost 전용 Pop/삭제·목록 수명은 3차 목표로 연기한다. 공통 작업장/지역 처리는 싱글플레이에 필요한 범위에서 진행한다. 이어 사제 Pop·보호막/회복 예약·Carrier와 GUI 건설·경제·전투·승패. 전체 객체 수명·실제 플레이는 후속이다.
- [ ] **장시간 검증:** 여러 판본 실제 자산 전수·변이 전수·최대 지도·창/픽셀 회귀·0 발자국/비정상 입력 전수는 계속 인계한다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 배치의 최종 표면 소유 관계·특수 지역·거부 조건 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 인계·cpppj 문서를 읽고 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canonrelations` 목록 6/2/2개를 읽기 전용으로 내보냈다. 게임/창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시는 없다.
- [x] **복원/연결:** [`RawCanonPlacementRelations`](cpppj/src/o/RawCanonPlacementRelations.h)은 원래 요청 좌표+0.9999 현재 표면 조회·번호 범위 검사·내부 관계 거부/표시 차단·패치 전용 특수 지역·마지막 직접 방향 동맹·최종 반환을 처리한다. `RawCanonPlacementPermission::Alliance`를 공유하고 실제 일반/3×3 픽셀 getter→decoder→미리보기→finder→지형→Player/주변 권한→최종 반환에 연결했다. [주소/자료 계약/한계](docs/exe/cpp-canon-relations-reconstruction.md).
- [x] **원본 차이:** genus 0x4200의 표시 초기화는 내부 거부를 해제하지 않는다. 마지막 직접 관계는 편집기/동일 owner/동맹 활성 조건을 보지 않는다. 지면/권한 우회도 관계 거부를 해제하지 않는다. 10.78만 signed 지역/sentinel·IslandList 제한을 읽고 CD만 최종 소유자 거부를 이전 지역 변수에 쓴다. 통합 장면의 누락 extra 비트와 0x4000 표시값 기대를 원본 계약대로 보완했으며 원본 관찰값을 바꿔 실패를 숨기지 않았다.
- [x] **독립 대조:** 새 [`decomp_canonrelations_oracle.py`](tools/decomp_canonrelations_oracle.py), 세 PE 각 2,307개·총 **6,921개**·두 x87 정밀도 일치. 앞부분 누적 지역 변수만 입력하여 최종 구간/실제 helper/성공·실패 에필로그/ret 24/보존 레지스터까지 실행했다. **함수 대체/OS 호출/assert 0**이며 전체 MayPlace/decoder/finder/Player는 이 생성기에서 실행하지 않는다. 방향 한 칸·좌표 보정·SID 경계·signed 지역·우회/조기 반환과 모든 출력/checksum·SHA/입력 순서/실제 진입을 감사한다. 누적 fixture **341,410개**, 기존 fixture/감사 도구는 미수정이다. 새 근거는 LF로 고정해 Git 줄바꿈 변환에 따른 SHA 차이를 방지한다.
- [x] **최종 C++ 검사/감사:** Release 경고/오류 **0**, CTest 내부 **417개·실패 0**(109.58초), 감사 **61종 모두 통과**. 새 검사 6개(세 PE 전체 재생·실제 일반/3×3 배치 합성·필수 자료/잘못된 지역/다른 풀/조기 읽기 보호)가 통과했다. 로그 `extracted/canonrelations-export.log`·`canonrelations-oracle-final.log`·`canonrelations-build-final.log`·`canonrelations-ctest-final.log`·`canonrelations-audits-final.log`.
- [x] **후속 완료:** 전체 MayPlace의 제한 입력 독립 실행/정상 반환과 대표 실제 자산 연결은 위 canonmayplace 단계에서 완료했다.
- [ ] **다음 작은 구현:** 실제 저장 맵/raw 세계·공간 장부·별도 Player 추가 목록의 생성/삭제 수명, 이어 사제 Pop·보호막/회복 예약·Carrier 및 GUI 건설·경제·전투·승패. 실제 자산 전수·미션 완주는 후속이다.
- [ ] **장시간 검증:** 여러 판본 실제 자산 전수·변이 전수·최대 지도/공간·창/픽셀 회귀는 계속 인계한다. 이번 작업은 30분 범위의 정적/제한 기계어/콘솔 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 배치 모양의 주변 표면 탐색·권한 누적 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 인계·cpppj 문서를 읽고 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canonsurrounding` 목록 25/21/21개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창·OS 실행, 보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시는 없다.
- [x] **복원/연결:** [`RawCanonSurfaceWalk`/`RawCanonPlacementSurrounding`](cpppj/src/o/RawCanonPlacementSurrounding.h)은 현재 모양 생성자→실제 flag 8 지도→float 발자국/중심/접합·중복 WORD 캐시와 바깥 소유 관계/실제 후보 helper를 처리한다. 중립도 바깥 관계 조건을 통과해야 하며 권한을 얻어도 같은 모양의 나머지 표면을 계속 조회한다. 실제 Player 기준점→주변 helper→EndShape 권한 누적으로 연결했다. [주소/자료 계약/한계](docs/exe/cpp-canon-surrounding-reconstruction.md).
- [x] **동적 읽기/경계:** 원래 모양 x/y를 쓰고 source 프레임 번호는 고정하되 코드 표는 후보마다 다시 읽는다. 첫 반환/조회 뒤 raw·지도·source/후보 코드 표 변경을 실제 원본과 대조했다. 1×1/3×2/12×12·소수 좌표·지도 끝/clamp·중복/내부/dead/abstract·방향 관계/초기 true 생략을 검사했다. 새 주변 경로는 양수 1..12 발자국/정상 유한 좌표 계약이며 0 발자국/비정상 입력 전체 동등성을 주장하지 않는다.
- [x] **독립 대조:** 새 [`decomp_canonsurrounding_oracle.py`](tools/decomp_canonsurrounding_oracle.py), 세 PE 각 524개(finder 140·주변 구간 384), 총 **1,572개**·두 x87 정밀도 일치. finder 전체/필터/기하/접합/CRT는 정상 반환/ABI·보존 레지스터까지 실제 실행하고 주변 중간 구간은 decoder Advance 전에 종료한다. **Player locator 진입만 지정 결과/미래 입력 변화로 명시 대체**한다. 각 PE 실제 생성 664회·Next 1,506회·후보 helper 970회·Player 대체 256회, assert/OS 0. 커서/캐시/인자·raw/지도/spot checksum·SHA/행/진입/대체 수 감사. 누적 fixture **334,489개**, 기존 fixture/감사 도구는 미수정이다.
- [x] **최종 C++ 검사/감사:** Release 경고/오류 **0**, CTest 내부 **411개·실패 0**(114.98초), 감사 **60종 모두 통과**. 새 검사 5개(세 PE 전체 재생·실제 Player/주변 권한/지형 연결·필수 자료/다른 풀/프레임 보호) 통과. 로그 `extracted/canonsurrounding-oracle-final.log`·`canonsurrounding-build-final.log`·`canonsurrounding-ctest-final.log`·`canonsurrounding-audits-final.log`, 판본별 내보내기 로그는 `extracted/canonsurrounding/`이다.
- [x] **후속 완료:** 최종 표면 소유 관계·특수 지역·최종 거부는 위 `canonrelations` 단계에서 복원하고 실제 파이프라인에 연결했다.
- [ ] **다음 작은 구현:** 전체 MayPlace 제한 대조/대표 자산 연결은 위 canonmayplace에서 완료. 후속은 실제 맵/공간 장부·별도 Player 추가 목록 수명. 이어 사제 Pop·보호막/회복 예약·Carrier 및 raw GUI·건설·경제·전투·승패다. 이 단계의 임시 최종 정책 true는 전체 원본 배치 성공 근거가 아니다.
- [ ] **장시간 검증:** 여러 판본 실제 자산 전수·변이 전수·최대 지도/공간·창/픽셀 회귀는 계속 인계한다. 이번 작업은 30분 범위의 정적/제한 기계어/콘솔 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: Player 작업장·그래프 배치 기준점 조회 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 인계 문서·cpppj 내부 문서를 읽고 현재/마지막 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `playeranchor` 목록 14/15/15개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창·OS 실행, 보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시는 없다.
- [x] **복원/연결:** [`RawPlayerPlacementAnchor`](cpppj/src/o/RawPlayerPlacementAnchor.h)은 `0048fdb0`/CD `00406f80`의 현재 작업장·별도 추가 목록·종속 조건·40개 순서/중복·하나/거리 선택, 좌표/SID 그래프를 복원한다. 공통 ownerFactories를 공유하고 별도 `0055a70c`/CD `005670e0` 목록은 factories와 구분한다. 실제 SquidOwner 등록/제거→Player 조회→후보 권한→지형에 연결했다. [주소/계약/한계](docs/exe/cpp-player-anchor-reconstruction.md).
- [x] **원본 차이/회귀 수정:** 디컴파일에 생략된 0.9999 fadd를 지도 경계 fixture 실패로 찾아 원본 상수/명령대로 반영했다. sqrt(5) 동률의 뒤 후보 51 선택을 두 개의 추가 원본 입력으로 확인하여 float 최소값 저장을 검증한다. NaN은 패치 C0/C2가 제외하지만 CD C0만 검사하여 채택한다. CD fixture 실패를 원본 명령에 따라 수정했다. 초기 실패를 숨기거나 기존 기대값을 바꾸지 않았다.
- [x] **독립 대조:** 새 [`decomp_playeranchor_oracle.py`](tools/decomp_playeranchor_oracle.py), 세 PE 각각 1,598개(collector 1,296·좌표 60·SID 180·거리 62), 총 **4,794개**·두 x87 정밀도 일치. Player/거리/그래프/contained 몸체와 CD 정리 래퍼는 정상 반환까지 실제 실행한다. **임시 160바이트 확보/반납 진입만 명시 대체**하며 메모리 부족/예외 전파는 검증하지 않는다. 반환/후보 목록·raw/지도 checksum·cdecl ESP/보존 레지스터·FS/x87·코드/쓰기·assert/OS 0·SHA/행/실제 진입/메모리 균형 감사. 누적 fixture **332,917개**, 기존 fixture/감사 도구는 미수정이다.
- [x] **최종 C++ 검사/감사:** Release 경고/오류 **0**, CTest 내부 **406개·실패 0**(113.03초), 감사 **59종 모두 통과**. 새 검사 5개(세 PE 전체 재생·실제 작업장 장부/종속/후보 권한/지형 연결·자료/풀/조기 읽기/SID 보호)가 통과했다. 로그 `extracted/cpp-playeranchor-export.log`·`cpp-playeranchor-oracle-final.log`·`cpp-playeranchor-verify-final.log`·`cpp-playeranchor-build-final.log`·`cpp-playeranchor-ctest-final.log`·`cpp-playeranchor-audits-final.log`.
- [x] **후속 완료:** 모양 기반 flag 8 주변 finder/바깥 권한 관계는 위 후속 단계에서 복원했다.
- [x] **후속 완료:** 최종 표면 소유 관계/특수 지역/거부 조건은 위 `canonrelations` 단계에서 복원했다.
- [ ] **다음 작은 구현:** 전체 MayPlace 제한 대조/대표 자산 연결은 위 canonmayplace에서 완료. 후속은 실제 맵/공간 장부·별도 추가 목록의 생성/삭제/공간 수명. 이어 사제 Pop·보호막/회복 예약·Carrier 및 raw GUI·건설·경제·전투·승패다.
- [ ] **장시간 검증/정밀도:** 여러 판본 실제 자산 전수·창/픽셀·최대 지도/공간 회귀는 계속 인계한다. 거리 중간값은 double/최소값은 float이며 임의 80비트 반올림 경계 전체의 동등성을 주장하지 않는다. 이번 단계는 30분 범위의 정적/제한 기계어/콘솔 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 일반 배치 후보 권한·방향 소유 관계 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 인계 문서·cpppj 내부 문서를 읽고 현재/마지막 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canonpermission` 목록 3/2/2개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창·OS 실행, 보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시는 없다.
- [x] **복원:** [`RawCanonPlacementPermission`](cpppj/src/o/RawCanonPlacementPermission.h)은 후보 helper `00462cb0`/CD `0045bae0`의 그래프 준비/요청 owner 0 조기 반환·현재 raw 소유자 BYTE·편집기/동일 owner/방향 동맹·중립/다른 owner 허용과 Player 조회 DWORD 반환을 보존한다. 별도 관계 `004629e0`은 방향 관계만 검사한다. DWORD 인덱스 감김을 유지하며 실제 표 밖 읽기를 진단한다. [주소/계약/한계](docs/exe/cpp-canon-permission-reconstruction.md).
- [x] **필수 경계/지형 연결:** Player 작업장·그래프 locator `0048fdb0`/CD `00406f80` 몸체는 필수 조회 경계다. 현재 raw 타입/좌표 비트·요청 owner를 전달한다. `MakeCanonPermissionTerrainHooks`는 원래 요청 DWORD를 유지하고 helper owner만 하위 BYTE 부호 확장한다. 실제 두 판본 3×3 decoder/미리보기/finder/지형에서 첫 조회 0/두 번째 DWORD 허용 후 권한 유지·후속 조회 생략·비활성 그래프/모양별 주변 조회를 검사했다. 주변/최종 정책의 공급값을 원본 전체 MayPlace 결과로 해석하지 않는다.
- [x] **독립 대조:** 새 [`decomp_canonpermission_oracle.py`](tools/decomp_canonpermission_oracle.py), 10.78 후보 5,520개+방향 관계 330개·CD/추가 10.37 후보 각각 5,520개·총 **16,890개**, 두 x87 정밀도 일치. helper 전체 정상 반환/cdecl ABI/보존 레지스터·실제 조기 분기/방향 표 읽기·raw/전역/조회 인자를 대조한다. **Player 진입에서 반환/변화만 명시 대체하고 그 몸체는 실행하지 않는다.** 각 PE 실제 후보 11,040회/대체 3,768회, 패치 실제 관계 660회, assert/OS 0회·SHA/행/실제 진입 수 감사. 누적 fixture **328,123개**, 기존 fixture/감사 도구는 미수정이다.
- [x] **C++ 검증/감사:** Release 경고/오류 **0**, CTest 내부 **401개·실패 0**(101.73초), 감사 **58종 모두 통과**. 새 검사 6개(세 PE 전체 재생·패턴 지형 합성·signed owner/현재 변화/조기 SID 생략·누락 경계/다른 풀/표 밖 주소)가 통과했다. 함수 목록 설명을 작업장·그래프 조회로 정확히 정리한 뒤 새 fixture/근거를 재생성하고 새 SHA 감사도 통과했다. 로그 `extracted/cpp-canonpermission-export.log`·`cpp-canonpermission-oracle.log`·`cpp-canonpermission-build.log`·`cpp-canonpermission-ctest.log`·`cpp-canonpermission-audits.log`.
- [x] **후속 완료:** Player 작업장·그래프 조회/40개 목록/거리 선택은 위 후속 단계에서 복원하고 임시 메모리만 대체한 원본 실행과 대조했다.
- [x] **후속 완료:** 실제 주변 표면 finder/권한 누적은 위 후속 단계에서 복원했다.
- [ ] **남은 구현:** 최종 표면 관계/특수 지역/거부·별도 추가 목록 수명 연결. 이어 사제 Pop·보호막 생성/회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하·raw GUI·건설·경제·전투·승패다. 일반 타입/패턴 전체 MayPlace는 아직 완료되지 않았다.
- [ ] **장시간 검증:** 여러 판본 실제 자산 전체 합성·변이 전수·창/픽셀·최대 지도/공간 회귀는 계속 인계한다. 이번 단계는 30분 범위의 정적/제한 기계어/콘솔 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 일반 타입 지형·지역 누적과 권한 경계 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 인계 문서·cpppj 내부 문서를 읽고 현재/마지막 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canonterrain` 목록 7/8/8개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창·OS 실행과 보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시는 없다.
- [x] **복원:** [`RawCanonPlacementTerrain`](cpppj/src/o/RawCanonPlacementTerrain.h)이 그룹 10/genus/bridge 초기 권한, 현재 raw 프레임/noIsland·다리/섬·실제 지역 getter, 열 간격 12/후보 8×8·signed BYTE/sentinel·모양 사이 groundComplete=false·bridge 우선 최종 지면 누적을 처리한다. 0 발자국과 빈 decoder의 배열 유지를 보존한다. [주소/계약/검증 한계](docs/exe/cpp-canon-terrain-reconstruction.md).
- [x] **필수 경계/연결:** 후보 권한은 허용 지면이 맞고 permission=false일 때, 주변 권한은 모양 종료 뒤 permission=false일 때만 조회한다. 최종 관계는 실제 지면 누적 뒤 필수 정책에 위임한다. 원래 argument/owner/mode와 모양 전체 정보를 캡처하여 실제 geometry/미리보기/finder와 연결했다. 기존 사제 지형 구현은 유지했다. 임시 최종 정책의 true를 원본 배치 성공으로 해석하지 않는다.
- [x] **독립 대조:** 새 [`decomp_canonterrain_oracle.py`](tools/decomp_canonterrain_oracle.py), 세 PE 각 3,740개·총 **11,220개**, 두 x87 정밀도 일치. 초기화 28·후보 3,200·모양 종료 144·지역 getter 128·최종 지면 누적 240개/판본이다. 중간 구간 입력·후보 permission=true·주변 권한 진입 전 종료·최종 관계 분기 전 종료이며 전체 MayPlace/권한 helper/관계/최종 반환 검증이 아니다. 지역 helper 정상 반환·ESP/x87·보존 레지스터·허용 코드/쓰기·raw/배열·assert/OS 0회·SHA/행/실제 호출 수를 감사한다. 누적 fixture **311,233개**, 기존 fixture/감사 도구는 미수정이다.
- [x] **C++ 검증:** Release 경고/오류 **0**, CTest 내부 **395개·실패 0**(106.75초). 새 검사 6개(세 PE 재생·실제 3×3 받침/미리보기/finder/지역 지도와 권한 누적/거부·주변 라벨/빈 decoder·필수 경계/계약)가 통과했다. 테스트 장면의 표면 단계 0 등록/그룹 0을 명시하여 서로 다른 섬 SID와 초기 무권한 조건을 보존했다. 로그 `extracted/cpp-canonterrain-export.log`·`cpp-canonterrain-oracle.log`·`cpp-canonterrain-build-final.log`·`cpp-canonterrain-ctest-final.log`.
- [x] **최종 감사/기존 근거 갱신:** 새 지형을 포함한 감사 **57종 모두 통과**. 최근 일본 유통판 추가로 바뀐 `run_script.ps1` 때문에 기존 destroygraph SHA가 달랐다. 세 PE/두 x87 정밀도의 제한 원본 실행 1,728개를 다시 만들고 **fixture 바이트·실제 호출 수·관찰/한계 전부 동일**을 확인했다. 기존 `recovery-destroygraph-evidence.json`은 해당 도구 SHA 하나만 갱신했다. 기존 fixture/감사 도구는 미수정이며 누적 입력 수에도 다시 더하지 않는다. 로그 `extracted/cpp-canonterrain-audits-final.log`·`cpp-canonterrain-destroygraph-refresh.log`·`cpp-canonterrain-destroygraph-verify.log`.
- [x] **후속 완료:** 후보 권한 helper의 조기/소유 관계 판정과 DWORD 반환 계약은 위 후속 단계에서 복원했다(Player 내부 조회는 후속).
- [x] **후속 완료:** Player 내부 조회와 실제 주변 표면 finder는 위 후속 단계에서 복원했다.
- [ ] **남은 구현:** 최종 표면 소유 관계/특수 지역/최종 거부 조건. 이어 사제 Pop·보호막 생성/회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하·raw GUI·건설·경제·전투·승패다. 일반 타입/패턴 전체 MayPlace는 아직 완료되지 않았다.
- [ ] **장시간 검증:** 여러 판본 실제 자산 전체 합성·변이 전수·창/픽셀·최대 지도/공간 회귀는 계속 인계한다. 이번 단계는 30분 범위의 정적/제한 기계어/콘솔 검증이다.

---

## 2026-10-09 ✅ 완료: 10.37 일본 유통 CD(original1037JP) 디컴파일 (HJOW-Athlon)

- [x] **보관 정책:** `original1037JP/`는 **Git에 커밋하지 않고(.gitignore) `HJOW-Athlon`과 `HJOW-X3D` 두 PC에만 파일로 보관**한다. 다른 PC에는 폴더가 없다. `tools/compare_original1037jp.py`는 폴더가 없으면 건너뛰고 정상 종료한다. 문서/근거 JSON/스크립트만 Git에 있다.
- [x] **디컴파일:** `tools/ghidra/run_decomp.ps1 -Edition original1037JP`(`run_script.ps1`도 같은 판본 지원 추가). 별도 프로젝트 `extracted/original1037JP/ghidra/NetStorm.gpr`, 결과 `decomp/NetStorm.c`·`functions.tsv`, **3,711개 함수·실패 0**(Git 제외). 마지막 디컴파일 수행 PC: `HJOW-Athlon`.
- [x] **핵심 결과:** `original1037JP/NetStorm.exe`는 `originalCD/NETSTORM.EXE`와 **바이트 단위로 동일**(SHA-256 같음)이고, 디컴파일 C 파일도 CD와 SHA까지 같다. `original1037/netstorm.exe`와는 `0x3314c` 한 바이트(`75`↔`eb`)만 다르다. 새 코드 배치/규칙 차이는 없으므로 CD/10.37 비교 해석을 그대로 쓴다. 기준은 10.78 유지.
- [x] **자료:** 3,402개 파일(약 696MB). `originalCD`와 공통 362개 중 361개 동일(`autorun.inf`만 다름), `netstorm.tarc` 동일. 번들 자료: `directx/` 3,036개, `demos/` AVI 3개(Heavy Gear·Zork GI·Dark Reign), `spart/` 다른 게임 로고 BMP. 상세: [비교 문서](docs/exe/original1037jp-comparison.md), [근거](cpppj/recovery-original1037jp-evidence.json).
- [x] **판본 성격(사용자 판단):** 일본어 리소스가 없고(영어/독일어 `nsenglishres.dll`·`nsgermanres.dll`, `d/lang.english`) 실행 파일이 CD와 같으므로, **일본에서 유통만 했을 뿐 실제로는 영문판일 가능성이 높다.** 문서의 "일본판"은 폴더/유통 이름이다. 일본어 자료로 쓰지 않는다. 언어 확정은 게임 실행이 필요해 하지 않았다.
- [x] **보호/범위:** 폴더 3,402개 파일 SHA가 작업 전후 동일(`extracted/original1037JP/protected-before.json`). 게임/설치 프로그램/클론 창은 실행하지 않았다. AGENTS.md·기존 원본·C#·`LEFT_JOBS.dotnetpj.md` 변경과 커밋/푸시는 없다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 일반 타입 후보 충돌·실제 패턴/finder 연결 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS/내부 문서를 읽고 현재/마지막 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canoncollision` 목록 5/4/4개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **공통 후보:** [`RawCanonPlacementCollision`](cpppj/src/o/RawCanonPlacementCollision.h)이 일반 genus·0 발자국·현재 무시 전역·클라이언트 SID·로컬 표시·extra/mode 거부·실제 finder의 조기 중단을 복원한다. 사제는 매 후보의 기존 genus 계약/순회를 유지하고 공통 후보 본문에 위임한다. 미리보기/실제 decoder 범위 어댑터와 원래 owner/mode를 캡처하는 필수 지역 정책 factory를 연결했다. 무시한 후보의 지형 효과도 유지하며 거부 뒤의 후속 효과를 생략한다. [주소/계약/검증 한계](docs/exe/cpp-canon-collision-reconstruction.md).
- [x] **독립 대조:** [`decomp_canoncollision_oracle.py`](tools/decomp_canoncollision_oracle.py), 세 PE 각 7,560개·총 **22,680개**, 두 x87 정밀도 일치. 원본 중간 후보 구간에 지역 변수/현재 SID·타입을 공급하고 지형 진입/거부 분기에서 관찰 종료한다. helper/현재 타입/CRT/표시·거부는 대체 없이 실제 실행한다. 전체 MayPlace/접두/decoder/finder/지형 실행 근거는 아니다. ABI/ESP/x87·허용 코드/쓰기·assert/OS 0회·SHA/행/호출 수를 감사한다. 누적 fixture **300,013개**, 기존 감사/fixture 미수정.
- [x] **최종 검증:** Release 경고/오류 **0**, CTest 내부 **389개·실패 0**(103.33초), 감사 **56종 모두 통과**. 새 검사 7개(세 PE 재생·실제 finder 조기 거부·현재 전역/저장 next·실제 받침/미리보기/모양/finder·계약)와 기존 사제 배치·미리보기·충돌·지형·모양/생성 통합이 통과했다. 공통 무시/후보 본문은 이름/진단 문자열 외에 기존 본문과 같음도 확인했다. 로그 `extracted/cpp-canoncollision-export.log`·`cpp-canoncollision-oracle.log`·`cpp-canoncollision-build.log`·`cpp-canoncollision-build-final.log`·`cpp-canoncollision-ctest-final.log`·`cpp-canoncollision-audits.log`.
- [x] **후보 지형/지역 누적:** 위 최신 단계에서 일반 타입의 후보 다리/섬·지역 배열·모양 종료·최종 지면 누적과 필수 권한/관계 경계를 연결했다.
- [ ] **다음 작은 구현:** 후보 권한 helper·주변 관계 finder·최종 표면 소유 관계/특수 지역·최종 거부 조건. 이어 일반 사제 Pop·보호막 생성/회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하, 비표면 raw 월드/GUI·건설·경제·전투·승패다. 후보 충돌 통과를 전체 MayPlace/일반 배치 성공으로 해석하지 않는다.
- [ ] **장시간 검증:** 여러 판본 실제 자산 전체 합성·변이 전수·창/픽셀·최대 지도/공간 회귀는 계속 인계한다. 이번 단계는 30분 범위의 정적/제한 기계어/콘솔 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 일반 타입·패턴의 로컬 배치 미리보기 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS/내부 문서를 읽고 현재/마지막 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canonpreview` 목록 9/7/7개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **공통 미리보기:** [`RawCanonPlacementPreview`](cpppj/src/o/RawCanonPlacementPreview.h)가 타입/별도 argument·로컬 배열 초기화·역방향 순회·고정 표면/current genus·소유 관계를 공유한다. 실제 decoder 범위 어댑터를 추가하고 기존 사제 API/생성/미리보기 경로도 같은 본문에 위임했다. 패치 y≥254 조기 거부·CD 선형 spot 읽기·부호 BYTE·방향 관계·조회 중 변경을 유지한다. 표시 배열과 실제 배치 반환은 독립이다. [주소/계약/검증 한계](docs/exe/cpp-canon-preview-reconstruction.md).
- [x] **독립 대조:** [`decomp_canonpreview_oracle.py`](tools/decomp_canonpreview_oracle.py), 세 PE 각 1,620개·총 **4,860개**, 두 x87 정밀도 일치. 접두/미리보기/표면/관계/반환 실제 실행, 모양/decoder/범위 함수 진입·후반 충돌 구간 명시 대체다. ABI/보존 레지스터/ESP/x87·허용 코드/쓰기·assert/OS 0회·SHA/행/호출 수를 감사한다. 누적 fixture **277,333개**, 기존 감사/fixture는 미수정이다.
- [x] **최종 검증:** Release 경고/오류 **0**, CTest 내부 **382개·실패 0**(104.87초), 감사 **55종 모두 통과**. 새 검사 5개(세 PE 재생·실제 3×3 받침/비로컬 연결·입력 계약)와 기존 사제 배치/미리보기·후보·지형·모양/생성 통합이 통과했다. 공통 본문은 이름/진단 문자열 외에 기존 사제 본문과 같음도 정적으로 확인했다. 로그 `extracted/cpp-canonpreview-export.log`·`cpp-canonpreview-oracle.log`·`cpp-canonpreview-build.log`·`cpp-canonpreview-build-final.log`·`cpp-canonpreview-ctest-final.log`·`cpp-canonpreview-audits.log`.
- [x] **일반 후보 충돌:** 위 최신 단계에서 공통 후보 정책과 실제 패턴/finder 연결을 완료했다.
- [ ] **다음 작은 구현:** 일반 타입/패턴의 후보 지형·지역 효과/모양 종료·최종 관계 판정. 이어 일반 사제 Pop·보호막 생성/회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하, 비표면 raw 월드/GUI·건설·경제·전투·승패다. 로컬 미리보기 통과를 전체 MayPlace/일반 배치 성공으로 해석하지 않는다.
- [ ] **장시간 검증:** 여러 판본 실제 자산 전체의 합성·변이 전수·창/픽셀·최대 지도/공간 회귀는 계속 인계한다. 이번 단계는 30분 범위의 정적/제한 기계어/콘솔 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 일반 타입·패턴 배치 접두 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS/내부 문서를 읽고 현재/마지막 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canonplacement` 목록 3/4/4개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **일반 접두:** [`RawCanonPlacement`](cpppj/src/o/RawCanonPlacement.h)가 별도 타입/argument·진입 차단 초기화·즉시 허용·모양 조회 전 부호 BYTE 소유자 캡처·판본별 x87 여백/지도 경계를 복원한다. 빈 모양의 음수 여백도 보존한다. 실제 `RawCanonPixelShape` 연결 어댑터와 `--inspect-canon-placement` 콘솔 명령을 추가했고 기존 사제 접두도 공통 구현으로 위임한다. [주소/계약/검증 한계](docs/exe/cpp-canon-placement-reconstruction.md).
- [x] **독립 대조:** [`decomp_canonplacement_oracle.py`](tools/decomp_canonplacement_oracle.py), 세 PE 각 1,120개·총 **3,360개**, 두 x87 정밀도 일치. 접두/CRT/반환 실제 실행, 모양 함수 진입·후반 미리보기/충돌/지형/관계 중간 구간은 명시 대체다. ABI/보존 레지스터/ESP/x87·허용 코드/쓰기·assert/OS 0회·SHA/행/호출 수를 감사한다. 누적 fixture **272,473개**, 기존 감사/fixture는 미수정이다.
- [x] **실제 자산:** [`cpp_canonplacement_smoke.py`](tools/cpp_canonplacement_smoke.py), 10.78 **116개 자산·512개 조합·4,096개 관찰**, CD **101개 자산·497개 조합·3,976개 관찰** 일치. TYPE/SHP 독립 파싱·실제 전체 getter 관찰을 접두의 모양 진입에 공급한다. getter/접두는 별도 원본 실행이며 모양/후반 정책 대체·즉시 허용 비트 공급을 명시한다. 결과 `extracted/cpp-canonplacement-assets-report.json`.
- [x] **최종 검증:** Release 경고/오류 **0**, CTest 내부 **377개·실패 0**(108.26초), 감사 **54종 모두 통과**. 새 검사 5개와 기존 사제 배치·미리보기·후보·지형·모양/생성 통합이 통과했다. 로그 `extracted/cpp-canonplacement-export.log`·`cpp-canonplacement-oracle.log`·`cpp-canonplacement-build-final.log`·`cpp-canonplacement-ctest-final.log`·`cpp-canonplacement-audits-final.log`·`cpp-canonplacement-assets-final.log`.
- [x] **일반 미리보기:** 위 최신 단계에서 타입/별도 argument와 실제 decoder 범위 연결을 완료했다.
- [ ] **다음 작은 구현:** 일반 타입의 충돌 정책·지형/지역/관계 판정. 이어 일반 사제 Pop·보호막 생성/회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하, 비표면 raw 월드/GUI·건설·경제·전투·승패다. 접두 통과를 전체 MayPlace/일반 배치 성공으로 해석하지 않는다.
- [ ] **장시간 검증:** mutations 전수·여러 판본·창/픽셀·최대 지도/공간 회귀는 계속 인계한다. 이번 단계는 30분 범위의 콘솔/정적/제한 기계어/실제 파일 자료 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 일반/패턴 배치의 모양 순회·finder 사각형 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS/내부 문서를 읽고 현재/마지막 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canongeometry` 목록 12/11/11개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **공통 모양 순회:** [`RawCanonPlacementGeometry`](cpppj/src/o/RawCanonPlacementGeometry.h)가 별도 타입/패턴 인자와 모든 타입별 패턴·비패턴/default/명시 모드를 처리한다. 첫 미리보기 정수 범위와 각 칸의 원래/절삭 좌표·현재 발자국의 finder 사각형을 공급한다. 지역 초기화→칸 준비→finder/후보→칸 종료→Advance→최종 순서를 유지하고 후보/칸 종료 거부 뒤 후속 칸/최종 판정을 생략한다. 빈 모양에는 지역 초기화/최종만 수행한다. [주소/산술/검증 한계](docs/exe/cpp-canon-geometry-reconstruction.md).
- [x] **사제/실제 연결:** 사제 geometry/미리보기·후보·지형·나선 생성 통합도 공통 순회를 사용하며 기존 사제/비패턴·양수 발자국 계약을 유지한다. 실제 `RawSquidFinder`/해시/슬롯과 다중 칸 후보 거부를 두 판본에서 검사했다. 일반 계산은 실제 0×0 `dude` 발자국을 허용하며 방향 메타가 없는 기본 프레임의 정보용 side는 0이다. 일반 배치 전체 정책을 허용한 것은 아니다.
- [x] **독립 대조:** [`decomp_canongeometry_oracle.py`](tools/decomp_canongeometry_oracle.py), 10.78 **3,159개**, CD/추가 10.37 각 **1,404개**, 총 **5,967개**·두 x87 정밀도 일치. 전체 decoder/Bounds·셀/검색/방향은 정상 반환까지 실제 실행한다. 모든 모양의 사각형/snap/CRT는 MayPlace 중간 구간에 좌표/현재 발자국 입력을 공급하고 **finder 몸체 진입 전에 관찰 종료**한다. 전체 MayPlace/finder/후보·지형/게임/OS 실행 근거가 아니다. 유효 칸 **12,393 / 5,508 / 5,508개**, ABI/보존 레지스터/스택/x87·허용 코드/쓰기·assert/OS 0회/SHA를 구별해 검사한다. 칸 사이 발자국 변경과 0 발자국·절삭 경계도 포함한다. 누적 fixture **269,113개**, 기존 감사 도구/fixture는 변경하지 않았다.
- [x] **실제 자산:** [`cpp_canongeometry_smoke.py`](tools/cpp_canongeometry_smoke.py), 10.78 **116개 자산·512개 조합·2,132개 유효 칸**, CD **101개 자산·497개 조합·2,117개 유효 칸**의 최초 범위/모든 칸 출력이 원본과 일치한다. 실제 TYPE 독립 파싱의 코드/default/발자국과 새 `--inspect-canon-geometry` 콘솔 출력을 대조한다. 결과 `extracted/cpp-canongeometry-assets-report.json`.
- [x] **최종 빌드/회귀:** 최종 구현 소스 이후 Release 경고/오류 **0**, CTest 내부 **372개·실패 0**(114.35초), 두 판본 실제 자산 대조 통과. 기존 사제 geometry/픽셀/후보/지형·나선 생성 통합도 전체 CTest에서 통과했다. 로그 `extracted/cpp-canongeometry-export.log`·`cpp-canongeometry-oracle.log`·`cpp-canongeometry-build-final.log`·`cpp-canongeometry-ctest-final.log`·`cpp-canongeometry-audits-final.log`·`cpp-canongeometry-assets-final.log`.
- [x] **최종 감사/문서 검사:** 새 근거의 전체 SHA·행/칸 수·모든 실제 구간/CRT 횟수를 재생/확인했고 감사 **53종 모두 통과**했다. UTF-8/추가 문자·차이 공백 검사도 통과했다. 성공 뒤 구현 소스 변경/불필요한 재검사는 하지 않았다.
- [x] **일반 타입 배치 접두:** 위 최신 단계에서 별도 패턴 인자와 실제 픽셀 getter 연결을 완료했다.
- [ ] **다음 작은 구현:** 일반 타입 미리보기·충돌 정책·지형/지역/관계 판정과 일반 사제 Pop·보호막 생성·회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하, 이어 비표면 raw 월드/GUI·건설·경제·전투·승패다.
- [ ] **장시간 검증:** mutations 전수·원본 여러 판본·창/픽셀·최대 지도/공간 회귀의 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어/실제 파일 자료 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 일반 자산과 특수 패턴의 픽셀 범위 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS와 내부 문서를 읽고 현재/마지막 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canonshape` 목록 8/6/6개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **전체 픽셀 getter:** [`RawCanonPixelShape`](cpppj/src/o/RawCanonPixelShape.h)가 일반 자산과 영역 68·다리 26·섬 2·받침 1·공유 전투 패턴의 모든 유효 칸/SHP 범위를 합친다. 현재 프레임/defaultFrame·물리 헤더·기준점·배율을 읽고 판본별 float32 저장·동률·부호 있는 0·DWORD 차·빈 범위를 보존한다. 패치 frameCheck/해제 표식과 CD 직접 참조를 구별한다. [주소/동작/검증 한계](docs/exe/cpp-canon-shape-reconstruction.md).
- [x] **공통 연결:** 사제 전용 getter는 기존 사제/비패턴 계약을 유지하고 공통 계산기로 위임한다. `--inspect-canon-shapes` CLI가 같은 실제 자산 자료로 일반 타입 기본 프레임과 모든 특수 패턴/네 짝수 방향의 여섯 출력 비트를 조회한다. 일반 타입의 전체 MayPlace/지형·관계/Pop/GUI 연결은 후속이다.
- [x] **독립 대조:** [`decomp_canonshape_oracle.py`](tools/decomp_canonshape_oracle.py), 10.78 **3,150개**, CD/추가 10.37 각각 **1,400개**, 총 **5,950개**·두 x87 정밀도 일치. 전체 getter·ctor/Advance·셀/FindFrame·패치 SHP helper를 대체 없이 정상 반환까지 실행한다. ABI/보존 레지스터/스택/x87·허용 코드/쓰기·assert/OS 0회/SHA를 검사한다. 유효 칸 **12,510 / 5,560 / 5,560개**의 헤더를 조회했다. 기대값은 실제 명령 관찰에서만 저장한다. 누적 독립 fixture **263,146개**. 기존 감사 도구/fixture는 변경하지 않았다.
- [x] **실제 자산:** [`cpp_canonshape_smoke.py`](tools/cpp_canonshape_smoke.py), 10.78 **116개 자산·512개 조합**, CD **101개 자산·497개 조합**의 여섯 float 비트가 원본 전체 getter와 일치한다. 실제 TYPE/SHP 독립 파싱·원본 숫자 변환 기준점·모든 특수 타입/패턴/짝수 방향·나머지 일반 타입 기본 프레임을 대조했다. SHP 주소 표만 분석용으로 재배치하며 실제 물리 메타/순서는 보존한다. 결과 `extracted/cpp-canonshape-assets-report.json`. 전체 로더/MayPlace/Pop/GUI 플레이 검증은 아니다.
- [x] **최종 검증:** 최종 소스 이후 Release 경고/오류 **0**, CTest 내부 **366개·실패 0**(101.77초), 감사 **52종 모두 통과**. 기존 사제의 원본 8,064개/배치 연결/오류 검사도 전체 CTest에서 통과했다. 로그 `extracted/cpp-canonshape-export.log`·`cpp-canonshape-oracle.log`·`cpp-canonshape-build-final.log`·`cpp-canonshape-ctest-final.log`·`cpp-canonshape-audits-final.log`·`cpp-canonshape-assets-final.log`. 성공 뒤 구현 소스 변경/불필요한 재검사는 하지 않았다.
- [x] **일반/패턴 모양 순회:** 위 최신 단계에서 타입/패턴 인자·전체 칸 순회·실제 finder 사각형을 완료했다.
- [ ] **남은 일반 배치:** 전체 MayPlace의 접두/미리보기·충돌 정책·지형/관계와 일반 사제 Pop·보호막/회복 예약·Carrier·raw GUI는 최신 후속 항목으로 인계한다.
- [ ] **장시간 검증:** mutations 전수·원본 여러 판본·창/픽셀·최대 지도/공간 회귀의 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어/실제 파일 자료 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: CanonDecoder 타입별 패턴 선택/전체 순회 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS와 내부 문서를 읽고 현재/마지막 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `canontype` 목록 10/9/9개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **타입별 패턴 복원:** [`CanonTypeDecoder`](cpppj/src/o/CanonTypeDecoder.h)와 누락 섬 2개·받침 1개·공유 전투 1개 표를 추가했다. 앞 네 전역의 선행 비교/별칭 우선순위·뒤 네 타입의 공유 표·argument×72·패턴 explicit 무시·비패턴 default/명시 선택·signed 방향/CD 홀수 진단을 보존한다. 새 표 생성기는 세 PE 전체 레코드 바이트 일치를 확인한다. [주소/선택/검증 한계](docs/exe/cpp-canon-type-reconstruction.md).
- [x] **공통 연결:** 기존 사제 geometry/shape도 `DecodeCanonType`를 사용하며 사제 genus/비패턴 계약은 유지한다. 실제 비사제 특수 타입의 전체 MayPlace/지역·관계/픽셀 연결은 후속이다. 새 `--inspect-canon-patterns` CLI는 실제 자산과 전역 번호로 모든 패턴/네 짝수 방향과 일반 사제를 콘솔에서 순회한다.
- [x] **독립 대조:** [`decomp_canontype_oracle.py`](tools/decomp_canontype_oracle.py), 10.78 **2,079개**, CD/10.37 각 **924개**, 총 **3,927개**·두 x87 정밀도 일치. 생성/전체 Advance·실제 셀/프레임 검색/방향·발자국/범위/CRT를 대체 없이 정상 반환까지 실행한다. 전체 칸의 프레임/좌표 비트/라벨/방향과 끝 상태/범위·선택된 원본 패턴 포인터·ABI/ESP/보존 레지스터/x87·허용 코드/쓰기·assert/OS 0회/SHA를 대조한다. 누락 검색은 원본 assert 경로이므로 정상 표/역순·중복 표를 사용했다. 누락 C++ 검사는 별도이며 과거 bridge의 assert 보고 대체 fixture를 기존 근거로 유지한다. 누적 독립 fixture **257,196개**, 기존 감사 도구/fixture/영역·다리 표는 변경하지 않았다.
- [x] **실제 자산:** [`cpp_canontype_smoke.py`](tools/cpp_canontype_smoke.py), 두 판본 각각 **408개 조합·2,028개 유효 칸**의 전체 출력이 실제 PE decoder와 일치한다. 독립 parser의 실제 코드/defaultFrame/발자국을 공급하고 원본 생성/전체 진행/범위/방향을 두 정밀도로 대조했다. 결과 `extracted/cpp-canontype-assets-report.json`. 전체 MayPlace/일반 사제 Pop/GUI 플레이 검증은 아니다.
- [x] **최종 검증:** Release 경고/오류 **0**, CTest 내부 **362개·실패 0**(111.86초), 감사 **51종 모두 통과**, 두 판본 실제 패턴 자산 및 기존 사제 픽셀 자산 회귀 통과. 로그 `extracted/cpp-canontype-export.log`·`cpp-canontype-oracle-final.log`·`cpp-canontype-build-final.log`·`cpp-canontype-ctest-final.log`·`cpp-canontype-audits-final.log`·`cpp-canontype-assets-final.log`·`cpp-canontype-priestassets-final.log`. UTF-8/추가 줄·차이 공백 검사도 통과했다. 성공 뒤 소스 변경/불필요한 재검사는 하지 않았다.
- [x] **일반 자산/패턴 픽셀:** 위 최신 단계에서 전체 getter와 실제 자산 연결을 완료했다. 일반 타입의 지형·관계/전체 MayPlace와 사제 Pop·보호막/회복 예약·Carrier·raw GUI는 최신 후속 항목에 인계한다.
- [ ] **장시간 검증:** mutations 전수·원본 여러 판본·창/픽셀·최대 지도/공간 회귀의 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어/실제 파일 자료 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 타입 기준점과 실제 사제 배치 자산 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md와 두 LEFT_JOBS를 읽고 현재/마지막 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `typehotspot` 목록 3/3/3개를 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **기준점 복원:** `RiftTypeRecord::hotspotX/Y`와 `TypeHotFootOffset`를 추가했다. 초기값 0·default_hotspot 비트·float32 AST→고정 16/11→중간 float 저장 없는 곱셈→0쪽 절삭·속성 순서/중복·다른 축 보존을 반영한다. 문자열 비율/원본이 assert하는 hotFootX/Y와 분석 계약 밖 정수 범위는 예외로 보고한다. [근거/주소/한계](docs/exe/cpp-priest-assets-reconstruction.md).
- [x] **실제 자산 공급:** [`PriestPlacementAssets`](cpppj/src/client/PriestPlacementAssets.h)가 전체 타입 번호의 프레임 코드/defaultFrame·기준점·실제 SHP 물리 헤더와 8개 패턴 번호를 소유한다. `GameWorld::BuildSurfaces`도 같은 자료를 사용한다. 새 `--inspect-priest-assets` CLI는 실제 사제 shape getter를 콘솔에서 호출한다. 패턴 몸체/일반 사제 Pop/GUI 배치의 완성은 후속이다.
- [x] **독립 숫자 변환:** [`decomp_typehotspot_oracle.py`](tools/decomp_typehotspot_oracle.py), 각 PE 468개·총 **1,404개**, 두 x87 정밀도 일치. 실제 숫자 속성/CRT ftol을 대체 없이 실행하고 각 PE 1,872회 호출·지역 스택/기준 레지스터/x87·허용 코드/쓰기·assert/OS 0회·SHA를 감사한다. 전체 로더 ABI/파싱을 실행한 근거는 아니다. 누적 독립 fixture 입력 **253,269개**, 기존 fixture/감사 도구 유지.
- [x] **실제 자산 대조:** [`cpp_priestassets_smoke.py`](tools/cpp_priestassets_smoke.py), 독립 Python parser와 실제 숫자 속성/전체 getter를 사용했다. 10.78 **116개 타입·3,783개 물리 프레임 개수**, CD **101개 타입·3,167개 물리 프레임 개수** 및 코드/defaultFrame/기준점/패턴 전역이 일치한다. 실제 사제 158의 기본 프레임 **18**·기준점 **(8,2)**에서 범위 **(-11,-23,13,4)**·보정 기준점 **(11,23)**의 여섯 float 비트가 두 x87 정밀도/원본 전체 getter와 일치한다. SHP 주소 표는 분석용 재배치, 프레임 순서/헤더 값은 실제 자료다. 조회 대상은 기본 프레임 18이며 모든 프레임 조회/전체 로더/MayPlace/일반 Pop/GUI 플레이 검증은 아니다. 결과 `extracted/cpp-priestassets-report.json`.
- [x] **최종 검증:** Release 경고/오류 **0**, CTest 내부 **358개·실패 0**(110.19초), 감사 **50종 모두 통과**, 두 판본 실제 자산 검사 통과. 로그 `extracted/cpp-typehotspot-export.log`·`cpp-typehotspot-oracle-final.log`·`cpp-typehotspot-build-first.log`·`cpp-typehotspot-build-final.log`·`cpp-typehotspot-ctest-final.log`·`cpp-typehotspot-audits-final.log`·`cpp-priestassets-smoke-final.log`. 성공 뒤 소스 변경/불필요한 재검사는 하지 않았다.
- [x] **타입별 패턴 decoder:** 위 최신 단계에서 선택/표/전체 순회를 완료했다. 일반 자산의 픽셀/지형·관계 연결·일반 사제 Pop/보호막/Carrier와 raw GUI는 후속이다.
- [ ] **장시간 검증:** mutations 전수·원본 여러 판본·창/픽셀·최대 지도/공간 회귀의 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어/실제 파일 자료 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 일반 사제 배치의 다리·섬·지역 효과 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md와 두 LEFT_JOBS를 읽고 현재/마지막 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `priestterrain` 목록 7/8/8개를 같은 PC에서 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창 실행과 보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **실제 후보 지형:** [`RawPriestPlacementTerrain`](cpppj/src/o/RawPriestPlacementTerrain.h), `0049bb1a`/CD `004457f7` 이후 bridge/섬/noIsland 상태·프레임별 허용 지면·좌표 절삭/지역 조회와 배열을 복원했다. 원본 배열은 **144바이트·열 간격 12·후보 쓰기 8×8**이다. 지역 지도는 미리보기 표면 지도와 별도 입력이며 getter는 **raw +8의 unsigned WORD**, 다리는 현재 sentinel, 지도 SID 0/범위 밖은 **127**이다. [주소/계산/범위](docs/exe/cpp-priest-terrain-reconstruction.md).
- [x] **모양/최종 판정:** 일반 사제는 초기 permission=true여서 후보 권한 helper/주변 관계 탐색을 우회한다. 첫 signed BYTE와 sentinel DWORD 비교·현재 발자국 전체·groundComplete의 누적 false를 보존한다. 최종 canPlaceGround와 `flags1 & 0x400`의 permission 효과를 계산한 뒤 사제 genus의 true 반환을 보존하며 앞선 충돌 거부를 허용으로 바꾸지 않는다. 유효 모양이 없으면 이전 지역 배열을 유지한다. bridge/패치 특수 지역 `0x200` 타입 조합과 일반 자산/패턴의 관계 탐색은 별도 범위다.
- [x] **생성/탐색 연결:** geometry의 선택 `beginShape`를 추가해 finder 전에 모양 원점/배열을 준비한다. 일반 사제의 지역 외부 경계를 실제 구현으로 교체했다. 기존 SHP/decoder/미리보기/후보/나선 생성/생성자 통합에서 첫 **21,22** 거부→**21,21** 생성, SID **15001/6001**·WORD **0x8101**·미리보기 **16칸**을 유지한다. 일반 Pop/가상 Carrier 몸체는 입력 경계다.
- [x] **독립 대조:** [새 도구](tools/decomp_priestterrain_oracle.py), 각 PE **2,240개**, 총 **6,720개**, 두 x87 정밀도 일치. 후보 1,920/모양 종료 144/지역 조회 128/최종 판정 48개다. 후보/종료/최종은 **MayPlace 중간 구간의 지역 변수/레지스터 입력**이며 finder Next/주변 관계 또는 반환값 설정 뒤 종료한다. 전체 MayPlace ABI/원본 finder 실행 대조가 아니다. 지역 조회는 실제 thiscall 정상 반환/스택/보존 레지스터 대조다. 전체 지역 배열/raw/상태/반환, 허용 코드/쓰기·x87·assert/OS 0회와 SHA를 감사한다.
- [x] **검증:** Release 경고/오류 **0**, CTest 내부 **353개·실패 0**(99.35초), 감사 **49종 모두 통과**, 누적 독립 x86 **251,865개**. 로그 `extracted/cpp-priestterrain-export.log`·`cpp-priestterrain-export-cd.log`·`cpp-priestterrain-oracle-final.log`·`cpp-priestterrain-build-final.log`·`cpp-priestterrain-ctest-final.log`·`cpp-priestterrain-audits-final.log`. 기존 fixture/감사 도구는변경하지 않았다. 성공 뒤 코드 변경/불필요한 재검사는 하지 않는다.
- [x] **타입 기준점/실제 자산 연결:** 위 최신 단계에서 완료했다. 특수 패턴/타입 조합은 후속이다. 이어 일반 사제 Pop/보호막·회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하, 일반 자산 수명과 비표면 raw 월드/GUI, 건설/경제/전투/승패를 복원한다.
- [ ] **장시간 검증:** mutations 전수·여러 판본 원본 자료·창/픽셀·최대 지도/공간 회귀의 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 사제 비패턴 픽셀 범위·기준점·SHP 프레임 조회 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS와 현재/마지막 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `priestshape` 목록을 읽기 전용으로 내보냈다(6/4/4개). 정밀 디컴파일·원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **실제 픽셀 조회:** [`RawPriestPlacementShape`](cpppj/src/o/RawPriestPlacementShape.h), 전체 getter `0043cbd0`/CD `004ed7c0`의 비패턴 사제 경로다. 기존 실제 decoder와 SHP 추가 헤더 메타 자료를 재사용했다. 기본/명시 프레임·원점 (0,0) 순회·빈 프레임의 (1000,1000,0,0)/배율 기준점·signed WORD/DWORD 차의 low DWORD·현재 타입 기준점과 패치/CD의 float 저장/덧셈/최댓값 선택 순서를 보존한다. SHP helper `00419850`의 논리 프레임/이중 레이어/해제 표식 검사와 CD 직접 물리 프레임 접근을 구분한다. 손상된 물리 접근/assert는 C++ 예외로 진단한다. [주소/산술/검증 한계](docs/exe/cpp-priest-shape-reconstruction.md).
- [x] **배치/생성 통합:** `MakePriestShapePlacementHooks`는 같은 풀의 초기 shape 경계만 실제 getter로 연결한다. 두 판본 생성 통합의 합성 픽셀 반환을 SHP 헤더 조회로 교체했다. 실제 픽셀 범위→여백→decoder 범위/모양→미리보기→finder/후보→나선 생성→사제 생성자/postCreate다. 첫 **21,22** 거부→다음 **21,21** 생성, SID **15001/6001**·WORD **0x8101**·미리보기 **16칸**이다. 160×110 프레임으로 좌표 9를 거부하고 현재 기본 프레임 변경 뒤 geometry 검사에 진입하는 연결도 확인했다. 지형·일반 Pop/가상 Carrier 몸체는 입력 경계다.
- [x] **독립 대조:** [새 도구](tools/decomp_priestshape_oracle.py), 패치 **4,032개**·CD/10.37 각 **2,016개**, 총 **8,064개**·두 x87 정밀도 일치다. **전체 getter/decoder/패치 SHP helper를 대체 호출 없이 정상 반환까지 실행**하고 cdecl/ESP·보존 레지스터·x87·출력 6개 float 비트·허용 코드/쓰기·assert/OS 0회/SHA를 검사했다. 비연속 역순 SHP 주소 표/프레임별 다른 메타 자료·기본/명시/빈/이중 프레임·CD의 논리 개수 밖 기본 프레임·signed 극값/정수 넘침·2^24/1000/0 경계·음수 배율/부호 있는 0·signed 방향을 포함한다. 패치 두 정밀도 실제 getter/생성/비패턴 조회 8,064회·Advance 14,112회·타입 getter 16,128회·SHP helper 6,048회다. CD/10.37 각각 getter/생성/조회 4,032회·Advance 7,168회다. 타입/SHP/기준점/배율은 분석 입력이며 자산 로더/전체 MayPlace/지형/게임/OS 원본 실행 대조가 아니다.
- [x] **검증:** Release 경고/오류 **0**·CTest 내부 **348개·실패 0**(108.73초), 감사 **48종 모두 통과**, 누적 독립 x86 **245,145개**. 로그 `extracted/cpp-priestshape-export.log`·`cpp-priestshape-oracle-final.log`·`cpp-priestshape-build-first.log`·`cpp-priestshape-ctest-first.log`·`cpp-priestshape-audits-final.log`. 기존 fixture/감사 도구는 유지한다. 성공 뒤 소스 변경/불필요한 재검사는 하지 않았다.
- [x] **후속 일반 사제 지형 완료:** 후보별 bridge/섬/지역 배열·조회·모양 종료/최종 사제 우회는 위 최신 기록에서 완료했다. 일반 자산/특수 타입 조합의 관계 탐색은 후속이다.
- [x] **타입 기준점/실제 자산 연결:** 위 최신 단계에서 완료했다. 특수 패턴 분기는 후속이다. 이어 일반 사제 Pop/보호막·회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하·일반 자산 수명, 파편/소리·전역 후처리·보행/GUI를 진행한다.
- [ ] **장시간 검증:** mutations 전수·여러 판본 원본 자료·창/픽셀·최대 지도/공간 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 사제 비패턴 CanonDecoder·미리보기 범위·모양별 finder 사각형 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS와 현재/마지막 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `priestgeometry` 목록을 같은 PC에서 읽기 전용 내보내기(10/9/9개)했다. 정밀 디컴파일·원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **실제 비패턴 decoder/범위:** [`CanonDecoder`](cpppj/src/o/CanonDecoder.h)에 패턴 없는 생성/진행·정수 Bounds를 추가했다. 실제 생성 `00425c20`/CD `0041fcf0`, Advance `00425860`/CD `0041feb0`, 범위 `00425b90`/CD `004202e0`다. 타입 발자국과 별개로 순회 1×1·기본 frame 또는 명시 인자 선택·기본 -1의 Valid=false·다음 칸에서 끝/마지막 좌표 유지다. signed 방향/2를 보존하고 패치 홀수/0xffffffff→회전 0과 CD 홀수 assert의 별도 C++ 예외를 구분한다. 기본 프레임 개수 검사도 임의로 추가하지 않았다. 미리보기는 trunc(x)-footX/trunc(y)-footY..trunc(x),trunc(y)이며 frame=-1도 계산한다. 기존 영역/다리 패턴 경로를 유지했다. [주소/계산/한계](docs/exe/cpp-priest-geometry-reconstruction.md).
- [x] **실제 모양 사각형/연결:** [`RawPriestPlacementGeometry`](cpppj/src/o/RawPriestPlacementGeometry.h), `0049b8c0`→`0049b95a`/CD `004455ab`→`00445641`. 모양 좌표를 CRT 절삭하여 float 저장한 뒤 left/top=절삭 좌표-(foot-1), right/bottom=원래 float 좌표+단정도 0.99999의 넓은 덧셈/절삭이다. 현재 특수 패턴 번호/사제 genus를 검사하며 패턴 경로는 미지원 계약이다. 실제 decoder 생성→지역 초기화 경계→유효 칸의 현재 발자국/실제 사각형→기존 실제 finder/후보→모양 후처리 경계→Advance→최종 지역 경계 순서를 연결했다. 초기화 중 기본 frame 변경은 이미 고른 frame을 덮지 않고 현재 발자국은 반영한다. 누락 frame은 finder/모양 후처리 없이 초기화/최종 지역 경계만 호출한다. 값 인자를 복사하고 같은 풀/판본만 어댑터로 연결한다.
- [x] **생성/회귀 통합:** 두 판본 실제 decoder 범위/순회·미리보기·접두·finder/충돌·나선 생성·사제 생성자/postCreate를 연결했다. 첫 **21,22** 거부→다음 **21,21** 생성, 새 SID **15001/6001**·WORD **0x8101**이다. 초기화 2회/모양 후처리 1회/최종 지역 1회/Pop 요청 1회와 **미리보기 16칸**을 확인했다. 이전 합성 bounds의 9칸과 구분한다. 초기 픽셀 모양/SHP·실제 지형/지역·일반 Pop/가상 Carrier 몸체는 입력 경계다. 기존 패턴·후보 통합도 전체 회귀에서 통과했다.
- [x] **독립 대조:** [새 도구](tools/decomp_priestgeometry_oracle.py), 패치 **2,304개**·CD/10.37 각 **1,024개**, 총 **4,352개**·두 x87 정밀도 일치다. ctor/Advance/Bounds는 실제 thiscall 정상 반환/ABI/보존 레지스터/x87 대조다. **충돌 사각형은 MayPlace 중간 구간에 현재 좌표/타입 지역 변수를 공급하고 finder 진입에서 인자를 관찰해 종료한다. 전체 MayPlace/finder/후보/지형 원본 실행 대조가 아니다.** 생성/비패턴 조회/진행/발자국/snap/CRT는 대체하지 않는다. 패치·두 정밀도 실제 생성/비패턴 조회 4,608회·Advance/Bounds/발자국 9,216회·getter 15,552회·snap/구간/finder 경계 3,456회·CRT 39,168회다. CD/10.37 각 생성/조회 2,048회·Advance/Bounds/발자국 4,096회·snap/구간/finder 경계 1,536회·CRT 17,408회다. 현재 frame/Valid/좌표/범위·허용 코드/쓰기·assert/OS 0회/SHA를 검사했다. 패턴 전역/패치 프레임 디버그 전역=0 입력 범위이며 assert 입력은 C++ 별도 검사다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **343개·실패 0**(97.86초), 감사 **47종 모두 통과**, 누적 독립 x86 **237,081개**. 로그 `extracted/cpp-priestgeometry-export.log`·`cpp-priestgeometry-oracle-final.log`·`cpp-priestgeometry-build-final.log`·`cpp-priestgeometry-ctest-final.log`·`cpp-priestgeometry-audits-final.log`. 기존 fixture/감사 도구는 유지한다.
- [x] **후속 픽셀 조회 완료:** 초기 비패턴 픽셀 모양 조회/SHP 연결(`0043cbd0`/CD `004ed7c0`)은 위 최신 기록에서 완료했다.
- [ ] **다음 작은 구현:** 후보별 bridge/표면/지형/지역/관계 몸체(`0049bb1a`/CD `004457f7` 이후)를 경계에서 복원한다. 특수 패턴 분기·실제 프레임/SHP 메타 로더와 최종 지역 판정도 남아 있다. 이어 일반 사제 Pop/보호막·회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하·일반 자산 수명, 파편/소리·전역 후처리·보행/GUI를 진행한다.
- [ ] **장시간 검증:** mutations 전수·여러 판본 원본 자료·창/픽셀·최대 지도/공간 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 사제 배치 충돌 후보 무시·발자국 표시/실제 finder 연결 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS와 현재/마지막 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `priestcollision` 목록을 같은 PC에서 읽기 전용 내보내기(5/4/4개)했다. 정밀 디컴파일·원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **실제 무시/후보:** [`RawPriestPlacementCollision`](cpppj/src/o/RawPriestPlacementCollision.h), 무시 함수 `0049ade0`/CD `00444900`, 후보 `0049b9c4`→`0049bb1a`/CD `004456ad`→`004457f7`와 실제 거부 분기다. mode/자기 타입·후보 genus·자기 surface flag·현재 전역의 순서/전체 DWORD 반환을 보존한다. CD bridge 마스크 0x208000와 패치 0x200000, 전역 반환 CD 0x210000와 패치 0x218000 차이를 반영했다. 현재 client mode에서 SID 5..14999/5999를 충돌에서 무시하고, 무시되지 않은 로컬 후보의 넓은 좌표 차 CRT 절삭/발자국→기존 mask[dx×12+dy+13]=1→extra&1/mode DWORD로 거부한다. 배열을 지우거나 표시 자체로 거부하지 않는다. [주소/분기/검증 한계](docs/exe/cpp-priest-collision-reconstruction.md).
- [x] **실제 finder/생성 연결:** `InspectArea`는 실제 RawSquidFinder flag 0/default filter Begin/Next를 사용하고 첫 거부 뒤 Next를 읽지 않는다. `inspectCandidateTerrain` 경계는 무시한 후보도 받고 그 뒤 다음 후보로 진행한다. 첫 후보의 충돌 무시→지형 경계에서 현재 next 지움/전역 변경→finder가 저장한 두 번째 SID→현재 전역에 의한 실제 충돌 거부를 확인했다. `MakePriestPlacementCollisionHooks`로 미리보기/접두/나선 생성에 연결했다. 두 판본 통합은 첫 21,22 충돌 거부→다음 21,21 실제 서버 SID **15001/6001**·사제 생성자/postCreate→WORD 0x8101→기록 소유자/Pop/Carrier 검사다. 모양/지형·일반 Pop/가상 검사 몸체는 경계다. 별도 buried 제외/거부 후 next=0xffff 미조회/지역 경계 거부 유지도 검사했다.
- [x] **독립 대조:** [새 도구](tools/decomp_priestcollision_oracle.py), 세 PE 각 **3,840개**(무시 helper 2,160 + 후보 1,680), 총 **11,520개**·두 x87 정밀도 일치다. 무시 helper는 실제 thiscall 정상 반환/ABI/전체 DWORD 대조다. **후보는 MayPlace 중간 구간에 지역 변수/현재 SID를 공급하고 지형 진입 또는 실제 거부 분기에서 관찰을 종료한다. 전체 MayPlace/모양/finder/preview/지형 원본 실행 대조가 아니다.** 후보 helper/타입/CRT/표시/거부는 대체 없이 실제 명령이다. 각 PE·두 정밀도 실제 helper 7,008회·후보 진입/종료 3,360회·현재 타입 3,360회, 패치 getter 3,360회·CRT 3,400회/CD·10.37 CRT 4,352회다. 전체 mask/raw/반환·허용 코드/쓰기·helper 보존 레지스터·스택/x87·assert/OS 0회/SHA를 검사했다. 실제 finder 연결/후보 사이 경계 변화는 별도 C++ 통합 검사다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **337개·실패 0**(103.23초), 감사 **46종 모두 통과**, 누적 독립 x86 **232,729개**. 로그 `extracted/cpp-priestcollision-export.log`·`cpp-priestcollision-export-patch.log`·`cpp-priestcollision-oracle-final.log`·`cpp-priestcollision-build-final.log`·`cpp-priestcollision-ctest-final.log`·`cpp-priestcollision-audits-final.log`. 기존 fixture/감사 도구는 유지한다.
- [x] **후속 비패턴 모양 완료(위 기록):** 실제 CanonDecoder 사제 비패턴 생성/진행·정수 범위·모양별 finder 사각형을 복원해 실제 미리보기/후보/생성에 연결했다.
- [ ] **다음 작은 구현:** 초기 픽셀 모양/SHP 연결과 후보별 bridge/표면/지형/관계 처리(`0049bb1a`/CD `004457f7` 이후)를 경계에서 복원한다. 특수 패턴 타입은 후속 범위다. 사제의 최종 일반 지면/관계 우회가 앞선 충돌을 항상 허용한다는 뜻은 아니다. 이어 일반 사제 Pop/보호막·회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하·일반 자산 수명, 파편/소리·전역 후처리·보행/GUI를 진행한다.
- [ ] **장시간 검증:** mutations 전수·여러 판본 원본 자료·창/픽셀·최대 지도/공간 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 사제 배치 로컬 미리보기·고정 표면/소유 관계 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS와 현재/마지막 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `priestpreview` 목록을 같은 PC에서 읽기 전용 내보내기(9/7/7개)했다. 정밀 디컴파일·원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **실제 미리보기:** [`RawPriestPlacementPreview`](cpppj/src/o/RawPriestPlacementPreview.h), `0049b661`→`0049b825`/CD `0044537c`→`0044550e`. 비로컬이면 배열 유지, 로컬이면 144바이트 초기화→CanonDecoder 생성/정수 사각형 경계→left/top 최소 1→bottom+1..top-1의 y 감소/각 행 right+1..left-1의 x 감소→px×12+py 차단 표시다. spot&0x10/고정 표면 SID 0/현재 표면 genus 0x2와 관계로 표시하며 raw free/dead/void/contained/extra 필터는 추가하지 않는다. 편집기/동일 소유자/방향 동맹 표 조건과 현재 지도/genus/관계를 사용한다. low BYTE 부호 확장 요청자 -1도 실제 인덱스가 표 안이면 조회한다. 빈 축의 불필요한 반복만 생략한다. [주소/범위/제한](docs/exe/cpp-priest-preview-reconstruction.md).
- [x] **판본/연결:** 패치는 x≥256 또는 y≥254 등 지도 밖 미리보기에서 바로 false다. CD는 조기 거부 없이 linear spot을 먼저 읽고 표면에만 x/y 범위를 검사한다. x=256의 다음 행 spot·y=254/255 차이를 정상 fixture로 대조했다. C++ 할당 지도/배열/관계 표 밖 읽기는 별도 예외다. `MakePriestPlacementPreviewHooks`로 같은 풀의 접두에 연결했다. 두 판본 생성 통합은 표시 9칸이 모두 1이어도 충돌 경계가 허용하면 실제 서버 SID/사제 생성자/postCreate로 생성함을 검사한다. 새 SID 15000/6000·좌표 21,22·WORD 0x8101이다. **표시 배열은 배치 결과와 독립**이며 모양/충돌/소유자·일반 Pop/Carrier 가상 몸체는 기록 경계다.
- [x] **독립 대조:** [새 도구](tools/decomp_priestpreview_oracle.py), 세 PE 각각 **1,620개**, 총 **4,860개**·두 x87 정밀도 일치. 배치 진입/여백/미리보기/표면/genus/관계/반환은 실제 명령이다. 모양·CanonDecoder 생성/범위 진입 대체와 **미리보기 뒤** `0049b825`/CD `0044550e`부터 충돌/지역 구간을 명시 대체한다. 각 PE 실제 진입 3,240회·CRT 2,880회, 패치 고정 표면 35,882회·genus/타입 23,772회·관계 14,742회다. CD/10.37 genus 각 26,460회·타입 번호 각 2,880회, 표면/관계는 본문 실제 인라인이다. 모양 대체 각 2,880회·CanonDecoder 생성/범위 각 2,520회·이후 충돌 구간 2,250/2,880/2,880회다. C++ 전체 mask/현재 관계·genus/표면 raw/지도 Adler/반환·사건과 원본 ABI/보존 레지스터/x87/허용 코드·쓰기·assert/OS 0회/SHA를 검사한다. 실제 충돌/모양/지도 수명 완성은 아니다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **330개·실패 0**(96.68초), 감사 **45종 모두 통과**, 누적 독립 x86 **221,209개**. 로그 `extracted/cpp-priestpreview-export.log`·`cpp-priestpreview-export-patch.log`·`cpp-priestpreview-oracle-final.log`·`cpp-priestpreview-build-final.log`·`cpp-priestpreview-ctest-final.log`·`cpp-priestpreview-audits-final.log`. 기존 fixture/감사 도구는 유지한다.
- [x] **후속 후보 충돌 완료(위 기록):** 타입별 무시 함수/client SID 조건/후보 발자국 표시/extra·mode 거부와 실제 finder 연결을 복원했다.
- [ ] **다음 작은 구현:** `0049b825`/CD `0044550e` 이후 실제 CanonDecoder 모양 순회/모양별 finder 범위와 후보별 지형/지역 처리를 복원한다. 사제 genus가 마지막 일반 자산 지면/관계 조건을 우회해도 앞선 충돌은 실패할 수 있으므로 무조건 허용하지 않는다. 이어 실제 모양/주변 관계·일반 사제 Pop/보호막·회복 예약·Carrier +0xcc/+0xc8 상태 검사/낙하·일반 자산 생성/소유자/Pop 수명을 진행한다. 파편/소리·전역 후처리·보행/GUI도 남아 있다.
- [ ] **장시간 검증:** mutations 전수·여러 판본 원본 자료·창/픽셀·최대 지도/공간 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 사제 배치 초기화·모양 여백·지도 경계 접두 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS와 현재/마지막 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `priestplacement` 목록을 같은 PC에서 읽기 전용 내보내기(3/4/4개)했다. 정밀 디컴파일·원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **배치 접두:** [`RawPriestPlacement`](cpppj/src/o/RawPriestPlacement.h), `0049b510`→`0049b661`/CD `00445200`→`0044537c`. 차단 관계 DWORD 초기화→전역 또는 flags2 0x02000000 즉시 허용→부호 확장 owner BYTE와 전체 로컬 DWORD 비교 캡처→모양 조회→판본별 여백/CRT 절삭→x/y 및 256-x/y 경계→후반 처리 경계다. 패치는 가로를 넓게 유지/세로 float 저장, CD는 두 축 float 저장이다. left=2^-20/right=32/top=bottom=0에서 패치 여백 1/CD 여백 2가 된다. 조회 중 로컬/허용/genus 변화를 다시 평가하지 않는다. 원본 값 인자를 복사해 조회 중 호출자 요청 변화도 차단한다. 사제 genus 전용이며 C++ 비유한/잘못된 타입/자료 입력 계약은 별도 진단한다. [주소/정밀도/제한](docs/exe/cpp-priest-placement-reconstruction.md).
- [x] **생성 연결:** `MakePriestPlacementHooks`는 같은 풀의 사제 생성 mayPlace만 실제 접두로 연결한다. 두 판본 spot=6 통합에서 후반 첫 거부/두 번째 성공→실제 서버 SID/사제 생성자/postCreate→WORD 0x8101→기록 소유자/Pop/Carrier 요청을 확인했다. 새 SID 15000/6000·선택 좌표 21,21이다. 기존 빈 공간 검사 생략과 실제 사제 소유자 통합은 유지한다. 모양/후반 충돌·관계/일반 Pop/가상 Carrier 몸체는 경계다.
- [x] **독립 대조:** [새 도구](tools/decomp_priestplacement_oracle.py), 세 PE 각각 **1,008개**, 총 **3,024개**·두 x87 정밀도 일치. 진입 초기화/즉시 허용/타입 번호/여백/지도 조건/정상 반환은 실제 명령, 모양 진입과 **후반 중간 구간** `0049b661`→`0049bfbb` 또는 거부 반환/CD `0044537c`→`00445ce2` 또는 거부 반환을 명시 대체한다. 각 PE·두 정밀도 실제 진입 2,016회·CRT 1,440회, CD/10.37 타입 번호 1,440회다. 모양 대체 각 1,440회·후반 구간 대체 730/700/700회다. 경계·강제 허용·BYTE 부호/상위 비트/전체 로컬 DWORD·조회/후반 전역 변화와 float 판본 차이를 포함한다. C++ 반환/사건/현재 전역/genus, 원본 thiscall EIP/ESP·보존 레지스터/x87·출력 포인터/초기 차단값/허용 코드·쓰기·assert/OS 0회 및 SHA를 검사한다. 실제 충돌/관계/모양/지도 수명 검증으로 해석하지 않는다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **325개·실패 0**(95.21초), 감사 **44종 모두 통과**, 누적 독립 x86 **216,349개**. 로그 `extracted/cpp-priestplacement-export.log`·`cpp-priestplacement-oracle-final.log`·`cpp-priestplacement-build-final.log`·`cpp-priestplacement-ctest-final.log`·`cpp-priestplacement-audits-final.log`. 기존 fixture/감사 도구는 유지한다.
- [x] **후속 미리보기 완료(위 기록):** 로컬 배열 초기화/주변 칸·고정 표면/genus/관계와 판본 가장자리 처리를 복원하고 접두/생성에 연결했다. 모양 생성/순회·실제 충돌 후보/지역 본체와 일반 Pop/보호막·회복/Carrier 상태 검사는 위 다음 항목에 남긴다.
- [ ] **장시간 검증:** mutations 전수·여러 판본 원본 자료·창/픽셀·최대 지도/공간 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: 사제 나선 생성·실제 WORD·Carrier 검사 래퍼 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS와 현재/마지막 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** 새 `priestspawn` 목록을 같은 PC에서 읽기 전용 내보내기(8/7/7개)했다. 정밀 디컴파일·원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **실제 생성 제어:** [`RawPriestSpawn`](cpppj/src/o/RawPriestSpawn.h), `0044b2e0`/CD `00461430`. `0.99999f` 스냅→북/동/남/서 길이 1,1,2,2,...,11,11의 **132개 후보**→0<x,y<255 공간 조회다. spot==0이면 배치 검사 생략, 아니면 `(type,x,y,0,WORD&0x7f,0)` 배치 결과와 **검사 전** spot&0x10으로 생성 여부를 결정한다. 생성(type,0)→값 인자인 전체 WORD 실제 쓰기→현재 다리 타입이면 표면 검사→고정 low 7비트 소유자→선택 정수 위치 Pop→현재 새 타입 genus 0x30000이면 가상 +0xcc 상태 검사다. 패치의 debug 탐색 실패/비 Carrier assert는 C++ 예외, CD는 각각 조용한 반환/검사 생략이다. [주소/범위](docs/exe/cpp-priest-spawn-reconstruction.md).
- [x] **최종 호출 의미/연결:** 마지막 래퍼 `00426230`/CD `004e38f0`는 현재 Carrier genus를 확인해 가상 +0xcc를 요청한다. 실제 사제의 +0xcc는 `00426fc0`/CD `004e50d0`이며 상태를 조회하는 몸체다. 활성화로 해석하지 않는다. `MakePriestSpawnHooks`로 같은 풀의 종속 해방 spawnPriest만 실제 몸체로 연결한다. 별도 두 판본 통합에서 실제 서버 SID·사제 생성자/postCreate·RawPriestOwner/SquidOwner를 실행했다. 부모/종속/새 사제의 freeCount -3·WORD 0x8101→로컬 표식 0x8181·소유자 1·선택 좌표 21,22를 확인했다. 일반 Pop/가상 상태 검사와 빈 공간 이외의 배치 검사 내부는 외부 경계다.
- [x] **독립 대조:** [새 도구](tools/decomp_priestspawn_oracle.py), 10.78 **408개**·CD/10.37 각 **414개**, 총 **1,236개**·두 x87 정밀도 일치다. 스냅/CRT·나선/경계/공간·WORD/Carrier 래퍼·현재 genus·정상 반환은 실제 실행, 배치/생성/가상 소유자/Pop/상태 검사/표면 함수 진입만 대체한다. 각 PE·두 정밀도 배치 대체 **40,468회**, 실제 WORD/Carrier 래퍼 480/492/492회·genus 960/492/492회다. 빈 공간·첫/세 번째/132번째 성공·전체 실패·범위 밖 후보·검사 중 공간 변경·생성 중 다리 전역·Pop 뒤 타입/genus 변경을 포함한다. C++ 현재 다리 전역/사건/새 raw 전체/지도 Adler와 cdecl EIP/ESP·thiscall 인자/보존 레지스터/x87·허용 코드/스택/WORD 쓰기를 검사한다. 정상 패치 탐색 실패는 debug=0이며 assert/OS 0회다. debug/Carrier 실패 진단은 C++ 별도 검사다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **320개·실패 0**(95.78초), 감사 **43종 모두 통과**, 누적 독립 x86 **213,325개**. 로그 `extracted/cpp-priestspawn-build-final.log`·`cpp-priestspawn-ctest-final.log`·`cpp-priestspawn-oracle-final.log`·`cpp-priestspawn-audits-final.log`. 기존 사제/종속 해방 fixture는 유지한다.
- [x] **후속 접두 완료(위 배치 기록):** 사제 배치의 초기화/즉시 허용/소유자 비교/모양 여백/지도 경계를 복원하고 생성에 연결했다. 후반 미리보기/충돌/지역·관계 본체와 일반 Pop/보호막·회복/Carrier 상태 검사는 위 다음 항목에 남긴다.
- [ ] **장시간 검증:** mutations 전수·여러 판본 원본 자료·창/픽셀·최대 지도/공간 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-09 ✅ 완료 / ⏭ 부분 인계: Damageable 종속 해방 좌표·타입·생성 요청 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·두 LEFT_JOBS와 현재/마지막 호스트 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-08.** 새 `damageablerelease` 목록을 같은 PC에서 23:58~59에 읽기 전용 내보내기(17/15/15개)했다. 작업 중 한국 시각 자정이 지났다. 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **실제 해방 몸체:** [`RawDamageableRelease`](cpppj/src/o/RawDamageableRelease.h), `0044b626`~`0044b80c`/CD `0046174e`~`0046195a`. 부모 footX=1이면 현재 좌표/아니면 현재 중심→`0.99999f` 스냅→yesIKnow=1 contained 재순회→후보 BYTE 0x20·현재 전투/허용 genus 조건이다. genus `0x200000`은 정확히 **사제 분기**이며 부모 현재 genus `0x80400000`이면 현재 중심+0.5, 아니면 현재 raw 좌표로 사제 생성/자리 탐색을 요청한다. 일반 자산은 고정 스냅 위치의 spot&0x10==0 && spot&6!=0에서 생성(kind,0)→**생성 뒤 현재 후보 WORD +0xc 실제 복사**→현재 다리 타입이면 표면 알림→소유자 0→Pop(고정 x,y,0)이다. 상위 boss/extra/flags 조건은 다시 평가하지 않는다. [주소/범위](docs/exe/cpp-damageable-release-reconstruction.md).
- [x] **연결/변경 관찰:** `MakeDamageableReleaseHooks`는 같은 풀의 releaseContained만 실제 몸체로 교체한다. 타입 표/공간/전역을 사용 시점에 읽고 finder의 saved next를 유지한다. 기존 두 판본 × server/client 사제 삭제 통합에 실제 해방 순회를 연결했다. 회복 form 46은 기본 finder 타입 6에서 제외된다. 이 통합에는 해방 생성 후보가 없으며 일반 자산 생성/배치의 실제 공간 수명 검증은 아니다.
- [x] **독립 대조:** [새 도구](tools/decomp_damageablerelease_oracle.py), 세 PE 각각 **1,504개**(Damageable/Carrier 각 752개), 총 **4,512개**·두 x87 정밀도 일치다. **해방 중간 구간 대체 0회**, 중심/스냅/발자국/genus/WORD·contained·바깥 분기·정상 반환은 실제 명령이다. 사제 생성/자리 탐색·일반 생성·가상 소유자/Pop·표면 검사와 기존 파편/소리/공통 함수 진입만 기록 대체한다. 각 PE·두 정밀도 실제 해방 2,944회·중심/발자국 2,928회·스냅 2,944회·WORD/부모 genus 각 1,984회다. 생성 중 WORD/next·부모 좌표/boss·다음 후보, 또는 전투/허용 genus/finder/다리 전역·공간 변경을 포함한다. C++ 사건/현재 여섯 전역/부모·후보 5개와 새 슬롯 4개의 전체 raw, EIP/ESP/this·인자/보존 레지스터/x87·허용 코드/스택/WORD 쓰기를 검사한다. assert/OS 0회다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **315개·실패 0**(95.76초), 감사 **42종 모두 통과**, 누적 독립 x86 **212,089개**. 로그 `extracted/cpp-damageablerelease-build-final.log`·`cpp-damageablerelease-ctest-final.log`·`cpp-damageablerelease-oracle-final.log`. 기존 Damageable 4,896행·Carrier 2,880행과 실제 사제 삭제 통합도 통과했다.
- [x] **후속 완료(위 사제 생성 기록):** 사제 생성 함수의 거의 올림·132개 나선·빈 공간 검사 생략·배치 검사/고정 WORD/소유자/Pop·Carrier genus 래퍼 요청을 복원하고 두 판본의 실제 서버 SID/사제 생성자·사제 소유자 통합을 연결했다. 배치 검사 내부·일반 Pop/보호막/회복·가상 Carrier 검사·일반 자산 수명·파편/소리/전역 후처리는 위 다음 항목에 남긴다.
- [ ] **장시간 검증:** mutations 전수·여러 판본 원본 자료·창/픽셀·최대 지도/공간 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: Damageable 효과·소리 접두와 실제 종속 순회 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 현재 PC/마지막 디컴파일 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-08.** 새 `damageablepredestroy` 목록 9/9/9개를 읽기 전용으로 내보냈다. 정밀 디컴파일·원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **접두/공통 호출:** [`RawDamageablePreDestroy`](cpppj/src/o/RawDamageablePreDestroy.h), `0044b4b0`/CD `004615e0`. 첫 extra&9 ordinary 조건→`0x200000` 붕괴/현재 좌표 collapse 소리→`0x100000` 폭발/현재 좌표 explosion 소리→spot&6일 때 contained 사제 kind마다 전역 priestFree 소리→현재 boss이고 flags&0x800이 없으면 해방 요청→항상 같은 flags의 공통 pre다. 두 효과 비트는 독립이며 extra가 효과 중 바뀌어도 첫 조건을 다시 평가하지 않는다. **파편/소리 출력·해방 생성/배치·Carrier 전역 후처리 내부는 외부 경계**다. [주소/범위](docs/exe/cpp-damageable-predestroy-reconstruction.md).
- [x] **좌표/공용 커서:** spot은 raw float에 `0.9999f`를 x87 정밀도로 더한 뒤 절삭한다. `20.0001f`의 중간 float 반올림 오답을 방지한다. [`RawContainedFinder`](cpppj/src/o/RawContainedFinder.h)는 현재 후보 next를 반환 전에 저장하고 아직 읽지 않은 타입/next 및 현재 타입 DWORD를 나중에 읽는다. 후보 상태를 추가로 거르지 않는다. Damageable의 yesIKnow=1은 패치 dead 부모 assert를 허용하고 Carrier 조회는 기본 false를 유지한다. 기존 Carrier 조회도 공용 커서로 옮겼으며 독립 2,880행 회귀가 통과했다.
- [x] **실제 사제 연결:** `MakeDamageablePreDestroyHooks`로 같은 풀의 Carrier 두 경계를 연결했다. 기존 두 판본 server/client의 실제 사제 생성자·보호막 조회·회복 Regular/ProcessForm/Kernel·공통 삭제 장부/깊이·SID 반납 합성에도 새 접두를 사용한다. 붕괴/위치 소리/해방 요청이 각 1회이며 회복 form 타입 46은 기본 finder 타입 6에서 제외된다. 자산/해시는 합성 void 입력이며 일반 공간 해제·파생 보호막 수명의 완료가 아니다.
- [x] **독립 대조:** [새 도구](tools/decomp_damageablepredestroy_oracle.py), 세 실제 PE 각각 **1,632개**(Damageable/Carrier 각 816개), 총 **4,896개**. 두 x87 정밀도의 결과가 같다. 효과/소리/공통 pre/Carrier 전역 후처리는 함수 진입 대체, **해방 구간은 중간 주소 `0044b626`→`0044b80c`/CD `0046174e`→`0046195a`를 명시 대체**한다. 해방 내부가 실제 실행되었다고 해석하면 안 된다. 각 PE·두 정밀도 합계 실제 Damageable 3,264회·Carrier 1,632회·CRT 1,920회·Begin 1,408회·Next 2,448회·true 필터 1,656회·Carrier 조회 832회. 기록 대체 붕괴/폭발/각 위치 소리 640회씩·priestFree 736회·해방 구간 320회·공통 pre 3,264회·Carrier 후처리 616회다. EIP/ESP·thiscall/cdecl 인자·보존 레지스터·x87·허용 코드/스택 쓰기·assert/OS 0회 및 C++ 사건/현재 전역/5개 raw 전체를 검사했다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **311개·실패 0**(91.03초), 감사 **41종 모두 통과**, 누적 독립 x86 **207,577개**. 로그 `extracted/cpp-damageablepredestroy-build.log`·`cpp-damageablepredestroy-ctest.log`·`cpp-damageablepredestroy-audits.log`. 첫 빌드의 namespace 닫힘 누락을 수정한 뒤 전체 검사를 통과했다.
- [x] **후속 완료(위 해방 기록):** footX 조건·중심/스냅·후보 0x20/현재 전투/허용 genus·사제 genus 0x200000 분기·일반 생성 뒤 현재 WORD 복사/다리 표면 알림/소유자 0/고정 위치 Pop 요청을 복원했다. 사제 생성/자리 탐색과 일반 생성/소유자/Pop 하위 몸체·파편/소리·Carrier 전역 후처리·보호막/낙하/전체 공간 수명은 위 다음 항목에 남긴다.
- [ ] **장시간 검증 인계 유지:** mutations 전수·여러 판본 원본 자료·창/픽셀·world/surface 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: Carrier 삭제 준비·실제 contained 조회 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 현재 PC/마지막 디컴파일 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-08.** 새 `carrierpredestroy` 목록 7/7/7개를 읽기 전용으로 내보냈다. 정밀 디컴파일·원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **Carrier와 조회:** [`RawCarrierPreDestroy`](cpppj/src/o/RawCarrierPreDestroy.h), `00426890`/CD `004e43b0`. 같은 flags의 Damageable→현재 boss→`FindContained(1,0,0)`→SID가 0이 아니면 인자 없는 전역 후처리다. 조회 `004ac9f0`/CD `004ac560`는 WORD head/next→타입 BYTE와 전역 DWORD(기본 6) 일치→kind WORD/flags WORD 조건→index개의 일치 건너뛰기다. 후보 상태/extra를 추가로 거르지 않고 DWORD 타입/조건 상한을 BYTE/WORD로 줄이지 않는다. 패치의 권한 없는 dead 부모 debug assert·범위/순환 오류를 명시적으로 거부한다. [주소/범위](docs/exe/cpp-carrier-predestroy-reconstruction.md).
- [x] **사제 연결:** `MakeCarrierPreDestroyHooks`로 validateCarrier/carrierPre만 같은 풀의 실제 몸체에 연결했다. 기존 두 판본 server/client 사제 생성자·실제 보호막 조회·회복 Regular/ProcessForm/Kernel·공통 장부/깊이·SID 반납 합성도 실제 Carrier 조회를 사용한다. 회복 form 타입 46은 기본 finder 타입 6에서 제외되고 이후 실제 종속 정리에서 제거된다. Damageable은 공통 lifecycle만 부르는 합성 경계이며 보호막/일반 공간 수명 완성이 아니다.
- [x] **독립 대조:** [새 도구](tools/decomp_carrierpredestroy_oracle.py), 세 실제 PE 각각 **960개**(조회/Carrier 각 480개), 총 **2,880개**. 직접 조회 대체 0, Carrier의 Damageable/전역 후처리만 대체다. 각 PE 조회/Begin 720회·Next 1,497회·true 필터 988회·Carrier 480회, Damageable 대체 480회·전역 후처리 146회다. Damageable 중 권한 0→1/1→0 각 24행과 머리/WORD 변경을 포함한다. 전체 정상 반환/EIP·ESP·thiscall 인자·보존 레지스터·허용 코드/스택 쓰기를 검사하고 assert/OS 호출 0회다. C++ 반환·사건·현재 권한·입력 5개 raw 슬롯 전체가 일치한다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **307개·실패 0**(101.70초), 감사 **40종 모두 통과**, 누적 독립 x86 **202,681개**. 로그는 `extracted/cpp-carrierpredestroy-build.log`·`cpp-carrierpredestroy-ctest.log`·`cpp-carrierpredestroy-audits.log`. 첫 빌드의 namespace 닫힘 누락을 수정한 뒤 전체 검사를 통과했다.
- [x] **후속 완료(위 Damageable 기록):** 붕괴/폭발·위치/해방 소리 요청, +0.9999 spot/실제 contained 순회, 현재 boss/flags의 해방 요청과 같은 flags의 공통 pre 호출을 복원했다. 공용 커서와 사제 연결도 완료했다. 파편/소리 출력·해방 내부 생성/배치·Carrier 전역 후처리·보호막/낙하/전체 공간 수명은 위 다음 항목에 남긴다.
- [ ] **장시간 검증 인계 유지:** mutations 전수·여러 판본 원본 자료·창/픽셀·world/surface 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: 사제 보호막 실제 조회 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 현재 PC/마지막 디컴파일 PC 일치를 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-08.** 새 `priestforcefield` 목록 12/10/10개를 읽기 전용으로 내보냈다. 정밀 디컴파일·원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **실제 조회:** [`RawPriestForcefield`](cpppj/src/o/RawPriestForcefield.h), `004918e0`/CD `0040bf00`. 좌표의 0방향 절삭→flags 0의 한 점 일반 finder→DWORD 타입 전역/owner BYTE 비교→첫 일치 SID 또는 0. 기본 true 필터/발자국/level·y·x·next 순서를 사용하고 매장 후보만 제외한다. free/dead/void/contained의 추가 후보 필터를 넣지 않았다. 타입 256/0xffffffff를 BYTE로 줄이지 않고 -1 좌표의 Begin sentinel도 보존한다. [주소/범위](docs/exe/cpp-priest-forcefield-reconstruction.md).
- [x] **삭제 준비 연결:** `MakePriestForcefieldHooks`로 같은 풀의 findForcefield만 실제 조회에 연결했다. 기존 두 판본 server/client의 실제 사제 생성자·회복 prefix·Regular/ProcessForm·Kernel·공통 삭제 장부/깊이·SID 반납 합성 검사도 실제 조회를 사용한다. hash 등록은 합성 void 자산 입력이며 일반 Pop/Unpop·해시 해제/보호막 파생 삭제의 완성은 아니다. 보호막 가상 삭제/Carrier·Damageable의 파생 효과는 외부 경계다.
- [x] **독립 대조:** [새 도구](tools/decomp_priestforcefield_oracle.py), 1,488장면에서 각 PE 조회/Pre **2,976개**, 총 **8,928개**. 두 x87 정밀도의 결과가 같다. 직접 조회에는 대체가 없으며 Pre에서 가상 삭제/Carrier만 대체한다. 실제 조회/좌표/Begin 3,970회·Next 19,050회·true 필터 17,008회(각 PE·두 정밀도 합계), preDestroy 2,976회. 반환/스택·보존 레지스터·x87·허용 코드/쓰기를 검사하며 assert/OS 호출 0회다. C++ SID·목록 전체·사건·모든 입력 raw 슬롯이 일치한다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **303개·실패 0**(93.06초), 감사 **39종 모두 통과**, 누적 독립 x86 **199,801개**. 로그는 `extracted/cpp-priestforcefield-build.log`·`cpp-priestforcefield-ctest.log`·`cpp-priestforcefield-audits.log`. 실제 true 필터/Pre/Carrier와 직접 조회 대체 0의 감사를 보강한 뒤 재생성한 fixture는 CTest가 검사한 바이트와 동일하다.
- [x] **후속 완료(위 Carrier 기록):** Carrier pre `00426890`/CD `004e43b0`의 Damageable 호출→현재 권한→실제 contained 조회 `(1,0,0)`→인자 없는 전역 후처리 요청을 복원하고 사제 삭제에 연결했다. Damageable 내부 효과·전역 후처리 몸체·보호막 생성/해제·낙하·전체 공간 수명은 위 다음 항목에 남긴다.
- [ ] **장시간 검증 인계 유지:** mutations 전수·여러 판본 원본 자료·창/픽셀·world/surface 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: 사제 삭제 준비·회복 form 정리 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 현재 PC와 마지막 디컴파일 PC가 같음을 확인했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-08.** 새 `priestdestroy` 목록(4/4/4개)을 읽기 전용 내보내기했다. 정밀 디컴파일·원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.
- [x] **삭제 준비 본문:** [`RawPriestPreDestroy`](cpppj/src/o/RawPriestPreDestroy.h), `004919b0`/CD `0040c330`. extra&9가 0이면 사제 목록의 모든 같은 SID를 순서 보존 압축→보호막 조회→1≤SID<capacity이면 가상 destroy(flags 0)→원래 flags의 Carrier다. 추상/매장은 Carrier만 호출하고 dead/void는 생략 조건이 아니다. 조회 뒤 extra 변경·예약 SID 1~4·비활성 꼬리도 유지한다. **보호막 공간 조회/가상 삭제의 파생 몸체와 Carrier→Damageable 내부 효과는 외부 훅**이다. [주소/범위](docs/exe/cpp-priest-destroy-reconstruction.md).
- [x] **실제 종속 정리 연결:** `MakePriestPreDestroyHooks`를 ProcessHost의 자산 훅에 연결했다. 두 판본 server/client 실제 사제 생성자·공통 postPop 장부·사제 회복 prefix→Regular/ProcessForm/Kernel→삭제→Kernel 제거·form/자산 SID 반납을 검사한다. 보호막은 합성 void 타입 167이며 파생 pre/post 효과를 대체하고 실제 공통 장부를 직접 부른다. 부모 dead 재삭제·보호막 중첩 삭제·깊이 균형·현재 수/비용 복구·누적 생산 수 보존을 확인했다. 일반 사제/보호막 공간 및 전체 파생 수명의 완성이 아니다.
- [x] **독립 대조:** [새 도구](tools/decomp_priestdestroy_oracle.py), 세 실제 PE 각각 **2,184개**, 총 **6,552개**. preDestroy 및 목록 압축/CD 삭제 helper는 실제 실행하며 조회/보호막 가상 삭제/Carrier만 기록 대체한다. 각 PE 목록 제거/helper 879회·조회 879회·삭제 요청 547회·Carrier 2,184회. 전체 정상 반환/EIP·ESP·thiscall 인자·보존 레지스터·허용 쓰기/코드를 검사했고 assert/OS 호출 0회다. C++ 사건/목록 전체/raw 슬롯 전체가 일치한다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **299개·실패 0**(100.12초), 감사 **38종 모두 통과**, 누적 독립 x86 **190,873개**. 로그는 `extracted/cpp-priestdestroy-build.log`·`cpp-priestdestroy-ctest.log`·`cpp-priestdestroy-audits.log`. 준비 코드가 free 슬롯의 쓰기 API를 다시 호출하던 보호 검사 오류를 수정한 뒤 전체 검사를 통과했다.
- [x] **후속 완료(위 보호막 실제 조회 기록):** `004918e0`/CD `0040bf00`의 좌표 절삭·point finder·DWORD 타입/owner 필터를 기존 RawSquidFinder에 연결했다. Carrier/Damageable 삭제 효과·보호막 생성/해제·낙하·전체 postPop/Unpop/Repop은 위 다음 항목에 남긴다.
- [ ] **장시간 검증 인계 유지:** mutations 전수·네 판본/원본 자료·창/픽셀·world/surface 회귀는 기존 인계를 유지한다. 이번 단계는 콘솔/정적/제한 기계어 검증이다.

---

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: 사제 실제 회복 0x25a와 Kernel 연결 (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 최신 회복 후보를 진행했다. 현재 호스트와 마지막 디컴파일 PC가 일치한다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-08.** 새 `priestregen`을 같은 PC에서 읽기 전용 내보내기(10/11/11개)했다. 정밀 디컴파일은 다시 하지 않았다. 원본 게임/복사본·클론 창 실행·보호 파일·AGENTS.md·dotnetpj/인계 변경·커밋/푸시 없음.
- [x] **회복 분기:** [`RawPriestRegen`](cpppj/src/o/RawPriestRegen.h)으로 `00494580`/CD `0040ccd0`의 **0x25a 분기**를 복원했다. 현재 HP < 최대 HP이면 타입 최대 HP/6의 signed 정수 증가량을 원본 ADD 감기로 계산하고 최대치로 제한해 실제 HP setter를 부른다. setter가 회복을 거부하거나 증가량이 0이어도 +0x88→damageable 요청은 실행한다. 가득 찬 HP의 두 요청은 생략하지만 중립 후처리는 계속한다. payload 비트는 반환하고 count는 읽지 않는다. [주소/범위](docs/exe/cpp-priest-regen-reconstruction.md).
- [x] **중립/패치 추적:** 권한·두 차단 전역 0·현재 소유자 0·현재 HP ≥ 최대 HP/2에서 공간 자료→점유→0이면 중립 동작이다. 가상/외부 효과 뒤 현재 HP/소유자/타입·전역을 다시 읽는다. 패치는 기존 GameRandom::Next(9)와 sequence(+3, >9600이면 한 번 빼기), 추적 비교/측정 >7000.0·남은 DWORD 감소·gate 0·**signed WORD sentinel -1**을 유지한다. CD는 난수/추적을 실행하지 않는다. 두 차단 전역과 추적의 게임 내 의미는 미확정이다. **공간/표시·damageable·중립 동작·patchAudit/측정 하위 몸체는 외부 훅**이다.
- [x] **실제 프로세스 연결:** `MakePriestRegenHandler`가 실제 사제 vtable의 0x25a만 분배한다. 다른 사제 사건은 fallback 없으면 미복원 예외이며 다리 처리기와 합성할 수 있다. 두 판본 server/client 실제 생성자→postPop prefix→ProcessForm/Kernel→HP **80→113→146→179→200**·첫 절반 경계의 Unpop/Repop 요청·8.25 payload 재예약·최대 HP 뒤 예약 유지·form 삭제/해제를 확인한다. 공간 효과는 기록 훅이며 일반 사제 Pop/GUI 허용을 추가하지 않았다.
- [x] **독립 대조:** [새 도구](tools/decomp_priestregen_oracle.py), 판본마다 **1,513개**, 총 **4,539개**. 실제 회복/HP getter/setter·중립 조건·난수/추적 필드 쓰기를 실행하고 미복원 외부 효과만 기록 대체한다. 두 x87 정밀도의 결과·정상 반환/thiscall/x87 스택·허용 쓰기와 assert/OS 호출 0을 확인한다. C++ 반환 비트·호출 시점 HP/소유자/좌표·사건·추적 전역·최종 난수·슬롯 전체가 일치한다. 중립 차단/점유/동작을 독립 교차하며 모든 판본에서 F/O/N 진입도 감사한다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **294개·실패 0**(기존 289 + 새 5, 90.47초), 감사 **37종 모두 통과**. 최초 CTest의 CD 호출 없는 결과 비교 표현을 고치고 중립 분기 입력을 보강한 뒤 전체 회귀를 다시 통과했다. 누적 독립 x86 **184,321개**. 로그 `extracted/priestregen-export.log`·`priestregen-generate-final.log`·`priestregen-build-final.log`·`priestregen-ctest-final.log`·`priestregen-audits-final.json`. [정적 증거](cpppj/recovery-priestregen-evidence.json).
- [x] **후속 완료(위 사제 삭제 준비 기록):** 사제 preDestroy `004919b0`/CD `0040c330` 본문의 사제 목록 제거·보호막 삭제 요청·Carrier 순서와 실제 회복 form 정리를 복원했다. 보호막 실제 조회/파생 삭제·Carrier/Damageable 효과·사제 낙하/공간·전체 postPop은 위 다음 항목에 남긴다.
- [ ] **계속 구현/장시간 검사:** 회복의 외부 하위 효과·패치 SP/추적 검증/측정, Carrier +0xcc/지면 공간 순회, PathProcess 진행/종료·파편/소리·배치·건설·경제·전투·승패. SharedRegular 0x25b를 일반 AddRegular로 바꾸지 않는다. 기존 약 40분 변이·최대 지도·SID 소진·로드 재시도는 계속 인계한다.

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: 사제 HP·지면 상태 조회와 HP setter (HJOW-Athlon)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 최신 다음 후보를 진행했다. 현재 PC와 마지막 내보내기 PC가 일치한다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-08.** 새 `prieststate` 목록을 같은 PC에서 읽기 전용으로 내보냈다(11/12/12개). 정밀 디컴파일은 다시 하지 않았다. 원본 게임/복사본·클론 창 실행·보호 파일·AGENTS.md·dotnetpj/인계 변경·커밋/푸시 없음.
- [x] **상태 조회:** [`RawPriestState`](cpppj/src/o/RawPriestState.h)의 현재 HP는 raw +26의 패치 signed DWORD/CD signed WORD다. genus 0x200000→HP < 최대 HP/2→x/y 거의 올림 spot & 6·extra 0x20 순서를 복원했다. wrapper는 extra를 먼저 검사한다. 지면 조회는 **0.9999f**를 더하고 중간 float 저장 없이 절삭한다. 저HP/강제 비트/비사제 genus의 사용하지 않는 NaN 좌표도 읽지 않는다.
- [x] **HP setter:** `004942b0`/CD `0040ca40`의 원본 폭 저장→절반 경계 전환→비권한 half/half-1 제한→소유자 허용 0이면 oldHP 복원→Unpop(0)→현재 HP/좌표 재조회→저HP의 **0.99999f** 좌표 보정→Repop(0)을 복원했다. Player +0x7c/CD +0x74의 게임 내 의미는 아직 확정하지 않았고 0 아님 조건만 표현한다. **두 가상 공간 효과는 외부 훅**이며 일반 사제 Pop은 계속 거부한다. 필요한 훅/소유자/vtable은 HP 쓰기 전 검사하지만 Unpop 뒤 좌표 오류의 원자적 복구는 보장하지 않는다. [동작/경계](docs/exe/cpp-priest-state-reconstruction.md).
- [x] **독립 대조:** [새 도구](tools/decomp_prieststate_oracle.py)로 판본마다 **6,752개**, 총 **20,256개**(지면 8,160·wrapper 8,160·setter 3,936). 두 x87 정밀도 결과가 같으며 정상 반환/thiscall 스택/x87 복구/허용 쓰기와 assert·OS 호출 0을 확인한다. 조회는 대체 함수 없음, setter는 Unpop/Repop만 기록/입력 변화 대체다. C++ 반환값·사건/호출 시점 HP/좌표·슬롯 전체가 일치한다. HP 폭/절반 경계/1/4 모드·권한/회복 허용·체커보드 spot·거의 정수 좌표·가상 효과 뒤 재조회를 검사했다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **289개·실패 0**(기존 284 + 새 5, 92.95초), 감사 **36종 모두 통과**. 최초 대조 실패로 좌표 보정의 별도 0.99999f 상수를 확인해 수정한 뒤 전체 회귀를 다시 통과했다. 누적 독립 x86 **179,782개**. 로그 `extracted/prieststate-build-final.log`·`prieststate-ctest-final.log`·`prieststate-audits.json`, 내보내기 `extracted/prieststate/`, 정적 증거 [JSON](cpppj/recovery-prieststate-evidence.json).
- [x] **당시 다음 작은 구현 완료(위 최신 기록):** 이벤트 처리기 `00494580`/CD `0040ccd0`의 회복 **0x25a** 분기를 실제 HP setter·기존 ProcessForm/Kernel에 연결했다. 독립 x86 4,539개로 전체 회복 분기 정상 반환을 대조했다. 공간/표시·damageable·중립 동작·패치 추적 검증/측정 하위 효과는 외부 훅이며 다른 사제 이벤트·일반 Pop/GUI는 후속이다.
- [ ] **계속 구현:** 실제 사제 Unpop/Repop 공간 효과·보호막/낙하·전체 postPop·preDestroy, Carrier +0xcc/지면 공간 순회, PathProcess 진행/종료와 비표면 raw GUI. SharedRegular 낙하 0x25b를 일반 AddRegular로 바꾸지 않는다. 이후 실제 파편/소리·배치·건설·경제·전투·승패다.
- [ ] **장시간 검사 인계 유지:** 기존 약 40분 프레임 지정/섬 삭제 변이, noIsland/표시/삭제/연결부 변이, 최대 지도·SID 소진·로드 재시도는 후속이다.

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: Carrier·Damageable postPop 직접 호출 (VM-W11-CODEX)

- [x] **지침/작업 범위:** 최신 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽었다. 사용자의 이번 **30분 이내 구현·검증 범위** 지시를 적용했다(과거 15분 기준보다 우선). 현재 `VM-W11-Codex`와 마지막 디컴파일 PC가 일치한다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08.** 새 `carrierpostpop`을 같은 PC에서 읽기 전용 내보내기(5/5/5개)했다. 원본 게임/복사본·클론 창 실행·보호 파일·AGENTS.md·dotnetpj/인계 변경·커밋/푸시 없음.
- [x] **복원:** [`RawCarrierPostPop`](cpppj/src/o/RawCarrierPostPop.h)으로 Carrier `00427720`/CD `004e6360`와 Damageable `0044bf80`/CD `004621c0`의 호출 순서를 복원했다. 비최초(flags & 1 없음)·비권한에서만 가상 +0xcc를 부르고, loadingDepth!=0·geyser 제외·extra & 9 없음·flags1 & 0x400이면 지면 소유자 갱신을 요청한다. 이후 항상 같은 flags로 공통 postPop이다. 가상 확인 뒤 현재 타입/extra/좌표/소유자를 다시 읽는다. 최초 flag의 genus & 0x130000 조회는 순수 읽기·반환값 무사용이며 HP 쓰기가 아니다. **+0xcc 몸체와 지면 공간 순회는 외부 훅**이다. [정확한 경계](docs/exe/cpp-carrier-postpop-reconstruction.md).
- [x] **공통/사제 합성:** `SquidPostPop::ValidateBase/PostPopBase`로 파생 몸체의 비가상 직접 공통 호출을 분리했다. 실제 공통 raw/Graph/비용/목록/AI/배치 보호와 비용·두 로컬/전역 통계·깊이 감소를 재사용한다. `MakeCarrierPostPopHooks`가 같은 풀인지 검사한다. 일반 Validate/PostPop/Activate/Pop의 사제 거부는 유지한다. 두 판본 server/client 사제 생성자→실제 소유자→회복 prefix/ProcessForm·Kernel→carrier/damageable→공통 장부를 합성했다. 기존 회복 유지·form 삭제·공통 억제/AI 거부·깊이 한 번 감소도 검사한다. **사제 전체 postPop/실제 HP 회복/GUI는 미완성**이다.
- [x] **독립 대조:** 새 [`decomp_carrierpostpop_oracle.py`](tools/decomp_carrierpostpop_oracle.py), 판본마다 **2,448개**, 총 **7,344개**. 실제 carrier/damageable/getter 몸체 전체·정상 반환/thiscall 스택을 실행하고 **가상 +0xcc·지면 갱신·공통 postPop만 대체**했다. flags/boss/로딩/타입·geyser/extra/flags1·소유자/상태·좌표 비트/외부 raw 변경에서 C++ 사건·인자·슬롯 전체가 일치한다. 실제 사제 flags1 0x69012에는 지면 갱신 비트 0x400이 없으며 합성 비트 입력도 별도 대조했다. assert/OS 호출 0. quiet NaN/음의 0은 이 호출부의 인자 전달만 검사하며 실제 지면 함수의 지원을 주장하지 않는다.
- [x] **검증:** Release 경고/오류 0·CTest 내부 **284개·실패 0**(기존 278 + 새 6, 58.19초). 관련 감사 **6종 통과**(carrierpostpop/priestpostpop/priestowner/postpop/process/owner). 누적 독립 x86 **159,526개**. 로그 `extracted/carrierpostpop-export.log`·`carrierpostpop-generate.log`·`build-carrierpostpop.log`·`ctest-carrierpostpop.log`·`audit-carrierpostpop.json`. UTF-8·변경 범위·diff 검사도 통과했다.
- [x] **당시 다음 작은 구현 완료(위 최신 기록):** 사제 이동 불가 조회 `00427030`→`00492090`/CD `004e5150`→`0040d7b0`와 HP setter `004942b0`/CD `0040ca40`를 복원했다. 조회는 대체 없음, setter 공간 효과 두 개는 기록 대체이며 독립 x86 20,256개와 대조했다. **이벤트 처리기 `00494580`/CD `0040ccd0`의 회복 0x25a 분기와 실제 공간 효과는 후속**이다.
- [ ] **보호막/낙하 인계·역할 정정:** **`00493d30`/CD `0040bfb0`은 경로 무효화가 아니라 보호막 생성·소리 처리**다. `004918e0`/CD `0040bf00`은 같은 소유자의 보호막 SID를 공간 조회한다. 보호막이 없으면 타입 DAT_005412f4/CD DAT_0051cbe0 생성→Owner/Pop→priestForceField.wav/ourPriestImmobile.wav로 이어진다. 낙하 `004941f0`/CD `0040c880`은 보호막 뒤 **SharedRegular 0x25b·payload 0x3c23d70a**와 권한 시 Unpop(0)/Repop(0x800), priestFall.wav다. 반대 분기 `00491980`/CD `0040c0d0`은 보호막 가상 삭제다. 이 내용은 같은 PC 정밀 디컴파일의 정적 확인이며 해당 효과 전체는 미복원이다. SharedRegular 전용 생성자/스케줄을 일반 AddRegular로 바꾸어 처리하지 않는다.
- [ ] **계속 구현:** +0xcc `00426fc0`/CD `004e50d0` 및 하위 가상 +0xc8, 지면 갱신 `0044bd20`/CD `00461f00`의 주변 표면/섬 받침/종유석 Owner/표시, 사제 preDestroy `004919b0`/CD `0040c330`, PathProcess 진행 `0048bb40`/CD `004805d0`·종료. 전용 postPop/preDestroy가 갖춰진 뒤 일반 Pop·비표면 raw GUI를 연결한다. 이후 실제 파편/소리·배치·건설·경제·전투·승패다.
- [ ] **장시간 검사:** 기존 약 40분 프레임 지정/섬 삭제 변이, noIsland/표시/삭제/새 연결부 변이, 최대 지도·SID 소진·로드 실패 재시도는 계속 인계한다. 이번 30분 범위를 넘는 작업은 시작하지 않았고 완료로 표시하지 않는다.

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: 사제 postPop 목록·회복 예약 prefix (VM-W11-CODEX)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 최신 인계의 다음 작은 구현을 진행했다. 현재 호스트와 마지막 디컴파일 PC가 같다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08.** 새 `priestpostpop` 목록을 같은 PC에서 읽기 전용 내보내기(5/6/6개)했다. 원본 게임/복사본·클론 창 실행·보호 파일·AGENTS.md·dotnetpj/인계 변경·커밋/푸시 없음.
- [x] **복원:** [`RawPriestPostPop`](cpppj/src/o/RawPriestPostPop.h)의 `Prefix`로 `004950f0`/CD `0040c110`의 **carrier 호출 전까지만** 복원했다. extra & 9 차단→flags & 1이면 용량/중복을 지키는 사제 목록→기존 회복 이벤트 `0x25a` 검색→확보 성공일 때만 HP/생성→같은 원래/현재 소유자 1~8 상태 칸 0 순서다. 중복/가득 찬 목록도 회복 검색을 계속하고, 예약 생성 뒤 현재 소유자를 다시 읽는다. [판본별 경계/계산](docs/exe/cpp-priest-postpop-reconstruction.md).
- [x] **회복 예약/통합:** 두 PE의 float 배율 50.0과 정수 HP/6 절삭→곱셈 후 float 저장→정수 최대 HP로 나누고 float 저장을 보존했다. 기존 `SquidReward`의 HP 조회를 재사용하며 SP 지급/저장을 호출하지 않는다. `MakePriestPostPopProcessHooks`로 실제 `SquidProcessHost::FindEvent/AddRegular`에 연결했다. 두 판본 server/client 실제 생성자·소유자→Form/Kernel 예약·중복 방지→Kernel 재예약→form 삭제/해제→HP 1/4 뒤 새 예약(8.25→8.0)을 검사했다. 부모의 회복 처리기는 기록/재예약 대체이므로 **실제 HP 회복 효과는 미복원**이다. 기본 연결의 호스트 메모리 부족은 C++ 예외로 전파하고 원본 new 실패는 reserve 훅으로 검사한다.
- [x] **독립 대조:** 새 [`decomp_priestpostpop_oracle.py`](tools/decomp_priestpostpop_oracle.py), 판본마다 **2,392개**, 총 **7,176개**. 두 x87 정밀도·목록/소유자/상태·이벤트 중복/확보 실패·큰/음수 HP에서 목록 저장소·소유자 칸 전체·사건/payload 비트·raw 슬롯이 일치한다. 목록/HP/소유자 helper는 실제 명령이며 **Regular 검색/new/생성자만 대체**, carrier 진입의 this/flags/x87 스택 확인 뒤 중단한다. 정상 반환/SEH 복구나 carrier 이후 실행을 주장하지 않는다. assert/OS 호출 0. 원본/Ghidra 없이 CTest fixture 재생 가능.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **278개·실패 0**(기존 272 + 새 6, 57.75초). 감사 **5종 통과**(priestpostpop/priestowner/process/reward/owner), 누적 독립 x86 **152,182개**. 최초 통합 검사의 client 생성에 필요한 flags 2 누락을 고친 뒤 재빌드/전체 회귀를 통과했다. 로그 `extracted/priestpostpop-export.log`·`priestpostpop-generate.log`·`build-priestpostpop-final.log`·`ctest-priestpostpop-final.log`.
- [x] **당시 다음 구현 중 호출부 완료(위 최신 기록):** Carrier/Damageable→공통 장부 직접 호출을 복원했다. +0xcc와 지면 공간 효과는 외부 훅이며, 사제 HP 회복/보호막/낙하·전체 postPop/preDestroy는 후속이다. 보호막 helper 역할 정정과 다음 함수 주소는 위 최신 인계를 따른다. 일반 사제 Pop은 계속 거부한다.
- [ ] **계속 구현:** 사제 preDestroy `004919b0`/CD `0040c330` 목록 제거·종속 처리·carrier 훅, PathProcess 진행 `0048bb40`/CD `004805d0` 및 종료. 이후 raw 비표면 유닛/GUI, 실제 낙하·파편/소리·배치·건설·경제·전투·승패를 진행한다. 기존 GUI walker/건물·12Hz 보행 타이머는 어댑터다.
- [ ] **장시간 검사 유지:** 기존 프레임 지정/섬 삭제 훅 변이 약 40분, noIsland/표시/삭제/새 연결부 변이, 최대 지도·SID 소진/로드 재시도는 계속 인계한다. 테스트 포함 개별 15분 이상 작업을 인계하는 기존 지침을 유지한다.

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: 사제 소유자 재정의 (VM-W11-CODEX)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 직전 다음 작은 구현의 소유자 지정을 진행했다. 현재 `VM-W11-Codex`와 마지막 디컴파일 PC가 일치한다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08.** 새 `priestowner` 목록을 같은 PC의 기본 Ghidra 프로젝트에서 읽기 전용으로 내보냈다(사제 몸체/알림, 판본마다 2개). 공통 owner 내보내기는 재사용했다. 원본 게임/복사본·클론 창 실행·AGENTS.md·dotnetpj/인계 변경·커밋/푸시 없음.
- [x] **복원:** [`RawPriestOwner`](cpppj/src/o/RawPriestOwner.h)으로 `00491790`/CD `0040bda0` 전체를 복원했다. 현재 소유자 BYTE와 원래 소유자 WORD(+12)의 low 7비트/로컬 표식을 구별한다. 비전투/요새 읽기 중은 원래 소유자 쓰기/알림→공통 지정, 전투 중은 요청 대신 원래 소유자 사용→공통 지정이다. 읽기 중첩 0 또는 전투이면 모드별 로컬 번호로 비트 7을 바꾼 뒤 알림한다. high byte(+13)는 보존한다. [`MakePriestOwnerDispatch`](cpppj/src/o/RawPriestOwner.h)는 기존 섬/종유석 `MakeOwnerDispatch`의 base로 합성할 수 있다. [정확한 판본 차이/범위](docs/exe/cpp-priest-owner-reconstruction.md).
- [x] **판본/보호 차이:** 패치의 갱신 분기 요청 0은 변경 전 예외다. 범위 밖의 0이 아닌 요청은 원래 WORD/표식/알림만 처리하고 공통 지정을 생략한다. CD 요청 0은 원래/현재 지정만 생략하며 마지막 표식 처리는 모드대로 한다. CD 공통 assert·free/예약 SID·타입/가상 표 불일치는 효과 전 거부한다. 무시되는 전투 요청 0/0xffffffff는 거부하지 않는다. 공통 소유자와 같은 0~8 지원 범위, challenge 9~39는 후속이며 challenge의 0 거부만 보호 검사했다.
- [x] **독립 대조/통합:** 새 [`decomp_priestowner_oracle.py`](tools/decomp_priestowner_oracle.py), 패치 **3,528** + CD/10.37 각 **1,944** = **7,416개**. 사제/공통 지정/타입 조회/알림을 **대체 함수 없이** 실제 PE 명령으로 실행하고 정상 반환/스택을 확인했다. 사건 인자·호출 시점 WORD/현재 소유자·슬롯 전체가 C++과 일치한다. assert/OS 호출 0. 두 판본 실제 사제 생성자→로딩→전투 요청 무시→로컬 표식→비전투 로딩 전환을 실제 공통 지정과 검사했다. 섬/종유석 분배 합성·공통 fallback/보호 입력도 통과한다. **Pop/postPop·Graph·GUI·포획 플레이는 미연결**이다.
- [x] **검증:** 최종 Release 경고/오류 0·CTest 내부 **272개·실패 0**(기존 266 + 새 6, 62.21초), 관련 감사 **4종 모두 통과**(priestowner/owner/bridgeconnect/pathanimation). 최초 빌드의 새 소스 namespace 닫기 누락을 수정한 뒤 통과했다. 누적 독립 x86 **145,006개**. 로그 `extracted/priestowner-export.log`·`priestowner-generate.log`·`build-priestowner-final.log`·`ctest-priestowner.log`.
- [x] **당시 다음 작은 구현 중 prefix 완료(위 최신 기록):** 사제 postPop `004950f0`/CD `0040c110`의 목록·회복 Regular(`0x25a`) 예약·소유자별 상태 칸 정리를 복원했다. carrier 이후/실제 회복 효과는 미완성이다. 읽기 전용 소스에서 carrier postPop `00427720`/CD `004e6360` 호출 뒤 지면/방향에 따라 낙하/경로 처리가 이어짐을 확인했다. 회복 payload의 타입/HP 조회·float 나눗셈과 두 판본 prefix는 이번 기계어 대조로 확인했다. 지금 소유자 모듈만으로 일반 Pop에 사제 가상 표를 허용하면 전용 효과가 빠진다.
- [ ] **계속 구현:** 사제 preDestroy `004919b0`/CD `0040c330`의 목록/종속 처리·carrier 훅, 보행 진행 `0048bb40`/CD `004805d0`과 도착 뒤 종료/명령 처리. 검증된 lifecycle/process/표시를 raw 유닛 월드/Kernel로 옮긴 뒤 실제 walker 낙하·파편/소리·배치·건설·경제·전투·승패를 진행한다. GUI walker/건물과 보행 중 12Hz 타이머는 기존 어댑터다.
- [ ] **장시간 검사 인계 유지:** 기존 프레임 지정/섬 삭제 훅 변이 약 40분, noIsland/표시/삭제/새 연결부 변이, 최대 지도·SID 소진/로드 실패 재시도. 테스트 포함 개별 15분 이상 작업은 인계한다는 기존 지침을 유지한다.

---

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: 방향 조회·보행 도착 프레임 접두 (VM-W11-CODEX)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 직전 다음 구현의 보행 애니메이션부터 범위를 좁혔다. 현재 `VM-W11-Codex`와 마지막 디컴파일 PC가 일치한다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08.** 새 `pathanimation` 목록을 같은 PC의 세 기본 Ghidra 프로젝트에서 읽기 전용으로 내보냈다(함수 각 3개). 원본 게임/복사본 실행·AGENTS.md·dotnetpj/인계 변경·커밋/푸시 없음. 허용된 클론 창만 실행했고 두 스모크가 originals/originalCD 전체 해시·존재 상태/설정 복구를 확인했다.
- [x] **복원:** [`RawPathAnimation`](cpppj/src/o/RawPathAnimation.h)으로 signed side `004ad680`/CD `004ae3f0`, direction `004ad710`/CD `004ae4b0`, 도착 접두 `0048bd90`/CD `00480580`을 복원했다. 타입 글자별 첫 물리 번호+1을 사용하며 같은 프레임이어도 Unpop(0)→쓰기→Repop(0x40)한다. 패치 DWORD/CD BYTE 폭과 extra 보존을 유지한다. free/예약 SID·타입/현재 프레임·A~P 표 밖 입력은 효과 전 거부한다. GUI 보행 종료 계산은 `RestFrame`으로 교체했다. [원본 주소·범위·재현](docs/exe/cpp-path-animation-reconstruction.md).
- [x] **독립 대조:** 새 [`decomp_pathanimation_oracle.py`](tools/decomp_pathanimation_oracle.py), 세 PE×(조회 608 + 접두 1,408) = **6,048개**. x87 두 정밀도, signed side 256값·A~P·비연속 코드/variant·owner·extra·이미 정지 프레임·BYTE 넘침을 대조했다. C++ 사건/슬롯 전체 일치, 예상 밖 assert/OS 호출 0. **Unpop/Pop은 기록 대체**, 패치 `0048bdd0`/CD `0047e220`에서 멈추므로 이후 종료/경로 처리와 원본 함수 전체 반환을 증명하지 않는다. 넘침 번호 256은 Pop 대체 상태의 필드 쓰기만 검사한다.
- [x] **통합/검증:** 실제 두 판본 Factory·Unpop·Pop의 단계 1→2 재등록/같은 프레임 반복/단일 체인·SID 여유를 검사했다(일반 bridgeConnector 155·합성 코드/크기, 표시/Graph 없음). Release 경고/오류 0·CTest 내부 **266개·실패 0**(기존 261 + 새 5, 60.46초). 관련 감사 **7종 모두 통과**(pathanimation/setframe/bridgeevent/process/display/pop/unpop). world 창 검사 1-1/TEST01 **20개 상태**, 1-1 동쪽 C 구간 도착 물리 프레임 17과 캡처 확인. surface 창 검사 예약/재개 삭제·픽셀 변화·미션 재진입/두 판본 5개 미션 로드 통과. 누적 독립 x86 **137,590개**. 로그 `extracted/pathanimation-export.log`·`pathanimation-generate.log`·`build-pathanimation-final.log`·`ctest-pathanimation.log`·`audit-pathanimation.json`·`gui-pathanimation-world.log`·`gui-pathanimation-surface.log`.
- [x] **당시 다음 작은 구현의 소유자 완료(위 최신 기록):** 실제 priest 타입 **158**의 소유자 재정의를 복원했다. 전용 postPop은 다음 구현으로 계속 남는다. 생성자 `004952a0`/CD `0040d790`, vtable `0050f210`/CD `005003e0`, postPop `004950f0`/CD `0040c110`, owner `00491790`/CD `0040bda0`, preDestroy `004919b0`/CD `0040c330`. postPop에 regen Regular(`0x25a`)·carrier/damageable·낙하/경로 무효화가 있어 공통 가상 표로 대체해 raw 표면 Pop에 넣으면 원본 효과를 잃는다. 합성 예전 fixture의 85는 실제 priest 번호가 아니다.
- [ ] **계속 구현:** 보행 진행 `0048bb40`/CD `004805d0`의 SHP float 이동량·정지/목표 판정·물리 프레임 진행과 도착 뒤 경로 종료/명령 처리를 별도 복원한다. GUI walker/건물과 보행 중 12Hz 타이머는 기존 어댑터다. 검증된 lifecycle/process/표시를 raw 월드·Kernel로 옮긴 뒤 walker 낙하·파편/소리·배치·건설·경제·전투·승패를 진행한다. 이번에 유닛 raw 전환/애니메이션 전체를 완료한 것은 아니다.
- [ ] **장시간 검사 인계 유지:** 기존 프레임 지정 6개·섬 삭제 훅 5개 변이(약 40분), noIsland/표시/삭제/새 연결부 변이, 최대 지도·SID 소진/로드 실패 재시도. 테스트 포함 개별 15분 이상 작업은 인계한다는 기존 지침을 유지한다.

---

## 2026-10-08 ✅ 완료 / ⏭ 부분 인계: raw 표면 월드·GUI·Kernel 게임 시각 연결 (VM-W11-CODEX)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 먼저 읽었다. 현재 `VM-W11-Codex`와 마지막 디컴파일 PC가 같으며 **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08(유지)**. 기존 내보내기/복원 모듈을 사용했고 새 디컴파일·원본 게임/복사본 실행 없음. 허용된 클론 창만 실행했다. AGENTS.md·dotnetpj·dotnet 인계 변경·커밋/푸시 없음. 스모크가 두 원본 디렉터리의 전체 해시/존재 상태를 확인하고 options.cfg/fullscreenStateFile.dat를 복구했다.
- [x] **실제 표면 월드:** [`RawSurfaceWorld`](cpppj/src/client/RawSurfaceWorld.h)가 같은 풀/4단계 해시/spot/타입/프레임/SHP에서 실제 생성·소유자·Pop/Unpop·Graph·가상 삭제·파생 pre/post·Regular를 구성한다. isle/noIsland/다리의 초기 등록→F/H 받침/종유석→연결 조각 생성 뒤 `Rebuild(true)` 한 번으로 전체 Graph를 만들고 활성화한다. 입력 필드 사전 검사, 적재 카운터 종료, 원본 비용/타입 장부를 유지한다. [범위/재현](docs/exe/cpp-raw-world-integration.md).
- [x] **GUI/게임 시각:** GameWorld는 현재 raw 표면 슬롯으로 스프라이트를 제출하고 기존 저장 다리/정적 받침은 표시하지 않는다. 현재 Renderer·카메라/viewport로 변경 요청을 전달하며 해상도 장치 교체를 견딘다. 기존 GroundGrid도 현재 raw 다리/받침 지면을 반영한다. 클라이언트 `Time()`의 고정 game/number를 미션 내부 Kernel에 공급하며 정지 중에는 Regular를 실행하지 않는다. 월드 해제 시 예약을 먼저 정리한다. 전역 Kernel은 GameWorld를 소유하는 호스트 구성이며 원본 전역 프로세스 구조 전체 복원은 아니다.
- [x] **실제 자산/창:** `cpp_surface_world_smoke.py` 추가. 10.78 1-1/Save the Island/TEST01 + CD 1-1/Save the Island **5개 raw 로드**에서 표면/Graph/다리 입력을 대조했다. 각각 표면 **1637/2413/6546/1637/2413**, 연결 조각 **2/31/77/2/31**. 1-1 클론 창에서 ESC 정지→(135,138) 받침 삭제(`surface-delete` 검사 경계)→예약 1개 보존→재개 시 (133,135) 새 끝 칸/(133,136) 연결 제거→140×150 영역 실제 픽셀 변화→메뉴 복귀/미션 재진입 초기화를 확인했고 캡처를 직접 보았다. 기존 world 스모크의 1-1/TEST01 조작도 통과했다.
- [x] **검증:** Release 경고/오류 0·CTest 내부 **261개·실패 0**(기존 257 + raw 월드 4). 기존 클론 창 회귀 **4종(window/renderer/menu/world)**과 새 표면 창 검사가 통과했다. 관련 근거 감사 **15종 모두 통과**(bridgedecay/bridgeevent/bridgeeffects/bridgeconnect/islandpostpop/islandlifecycle/process/setframe/display/rawgraph/graphrebuild/destroygraph/postpop/pop/unpop). 새 독립 x86 입력 없음·누적 **131,542개 유지**. 로그 `extracted/build-raw-world.log`·`ctest-raw-world.log`·`gui-raw-world-regression.log`·`gui-raw-cpp_*-smoke.log`·`audit-raw-world-*.log`, 새 GUI 결과 `extracted/cpp-surface-world-smoke/`.
- [ ] **다음 구현:** (1) 비표면 객체/선택/이동의 raw 전환과 원본 애니메이션 프로세스. (2) 실제 walker 가상 낙하·Flyingshrapnel/파편·소리와 공통 삭제 보상/UI 장치 연결. (3) 다리 배치/Construction→건설·생산 덱·경제/SP→전투·AI·승패. 이번에는 건물/walker·본섬 생성/변형·fringe/깊이 등 기존 어댑터를 유지했다. battle=true·mission=false의 초기 색 어댑터이며 원본 미션 전역 전환 순서 전체는 후속이다. 파편/소리/보상은 진단 요청, 네트워크 Transmit은 전송 없음. 받침 삭제 뒤 남는 종유석/noIsland·건물의 전체 붕괴/낙하 애니메이션은 아직 미복원이다.
- [ ] **계속 인계:** 기존 프레임 지정 6개·섬 삭제 훅 5개 변이(약 40분), noIsland/표시/가상 삭제/새 월드 연결 변이 검사, 최대 지도·SID 소진·불완전 받침 입력/로드 실패 재시도. 테스트 포함 개별 15분 이상 작업을 인계하라는 기존 지침을 유지한다. 이번 검증은 클론의 raw 상태/정적 픽셀 변화이며 원본 화면 전체 픽셀 일치나 낙하 애니메이션 검증은 아니다.

---

## 2026-10-08 ✅ 완료: Graph 활성 섬 생성·다리 가상 삭제·끝 칸 재등록 (VM-W11-CODEX)

- [x] **지침/호스트:** AGENTS.md·두 인수인계를 읽고 다른 AI가 추가한 다리 destroy 재정의 누락을 먼저 반영했다. 현재 `VM-W11-Codex`와 마지막 디컴파일 PC `VM-W11-CODEX`가 일치한다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08(유지).** 기존 내보내기만 읽었고 새 디컴파일·원본 게임/복사본·클론 창 실행 없음. 원본 파일·AGENTS.md·dotnetpj·dotnet 인계 변경·커밋/푸시 없음.
- [x] **호출 지점 확인/가상 분배:** 두 판본의 자기 삭제 이벤트·끝 칸 변환·연결 객체 정리가 가상 +0x10을 부르고, 다리 재정의의 마지막은 공통 destroy 직접 호출임을 확인했다. [`RawSquidDestroyDispatch`](cpppj/src/o/RawSquidDestroyDispatch.h)로 현재 raw 가상 표/graph byte/프레임과 모드를 읽어 `Bridge::DestroyProceeds`를 적용한다. 거부 시 풀·공간·장부·표시·Regular를 보존한다. 공통 `RawSquidDestroy` 직접 호출은 별도로 유지한다. 무효 Graph 254/표 밖 255·프레임 오류는 변경 전 예외, editor/권한/abstract/buried는 뒤 조회를 생략한다.
- [x] **Graph 활성 통합:** 같은 Graph를 postPop/삭제 공통 훅에 연결하고 graphsEnabled를 켰다. 빈 월드의 `Rebuild(true)`로 예약 0번을 먼저 초기화한다. 받침/종유석·소유자 생성→섬 삭제→Kernel 지연 낙하→끝 칸 생성/삭제/Pop→연결/ProcessForm 정리를 실제 프레임/표시 훅과 함께 검사한다. 정상/고립/hard/flags 0x1000 흐름에서 raw 번호별 그래프 표면 수·활성 표·깊이·비용/타입 수·해시/spot·풀 여유를 대조한다. hard는 실제 자기 삭제 이벤트도 거부하며 editor 전환 후에만 정리한다. [근거·재현](docs/exe/cpp-surface-graph-integration.md).
- [x] **실제 자산 읽기 전용:** 테스트 실행 파일의 `--inspect-surface-graph originals` / `--inspect-surface-graph originalCD --cd`로 실제 두 판본 타입·프레임 코드·SHP를 공급했다. **두 판본 × 네 흐름 = 8개 모두 통과**, 최종 삭제 구간 표시 전달 교체 4회/나머지 3회와 부분 Draw/Present 확인. 일반 CTest는 원본 파일 없이 동작한다. 실제 스프라이트 GUI 검사는 아니다.
- [x] **검증:** x64 Release 경고/오류 0·CTest 내부 **257개·실패 0**(62.12초)(기존 251 + 가상 삭제 5 + Graph 통합 1). 관련 감사 bridgedecay/bridgeevent/bridgeeffects/graphrebuild/destroygraph/rawgraph/postpop/islandpostpop/setframe/display **10종 모두 통과**. 새 독립 x86 입력 없음·누적 **131,542개 유지**. 최초 검사에서 Graph 예약 초기화·해시 단계 필드·Transmit 억제 훅 수 기대값을 수정한 뒤 회귀 통과. 로그 `extracted/build-surface-graph.log`·`ctest-surface-graph.log`·`surface-graph-originals.log`·`surface-graph-cd.log`·`audit-surface-graph.json`.
- [x] **당시 다음 구현 완료(위 최신 raw 월드 절):** 표면 SID/Graph/프레임/표시를 GUI에 연결하고 고정 게임 시각으로 미션 내부 Kernel을 실행했다. 1-1/TEST01에서 받침·색·연결 조각 표시와 기존 조작, 1-1에서 정지 중 예약→재개 끝 칸 교체/연결 제거→픽셀 변화→미션 재진입을 확인했다. 비표면 raw 전환과 실제 애니메이션·walker 낙하·파편/소리·배치·건설·경제·전투·승패는 최신 다음 구현으로 남긴다.
- [ ] **장시간 검사/남은 범위:** 기존 프레임 지정 6개·섬 삭제 훅 5개 변이 검사(약 40분), noIsland/삭제 분배기/표시/새 가상 삭제의 변이 검사는 인계한다. 테스트 포함 15분 이상 작업을 인계하라는 기존 사용자 지침을 유지한다. 이번은 Graph 활성 호스트 통합이며 새 독립 원본 실행 대조나 GUI raw GameWorld 검사는 아니다.

---

## 2026-10-08 ✅ 완료: SquidFrame 표시 갱신·실제 SHP 크기 공급 (VM-W11-CODEX)

- [x] **지침/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 직전 다음 구현 (1)을 진행했다. 현재 `VM-W11-Codex`와 마지막 디컴파일 PC `VM-W11-CODEX`가 일치한다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08(유지).** 같은 PC의 기존 함수 내보내기를 읽었고 새 디컴파일·원본 게임/복사본·클론 창 실행 없음. 원본 파일·AGENTS.md·dotnetpj·dotnet 인계는 변경하지 않았고 커밋/푸시하지 않았다.
- [x] **실제 연결:** [`SquidFrameBinding`](cpppj/src/o/SquidFrameBinding.h)의 `MakeSquidFrameHooks`로 실제 SHP 칸 크기 공급→SquidDisplay의 old/new Update→일반 Unpop/Pop 표시 갱신을 연결했다. `SquidRenderer::Shapes`가 기존 픽셀 크기/hotspot에 칸 크기 float 두 개를 함께 보존한다. 판본/타입 내용·풀/표시/해시/spot 공유를 구성 때 검사한다. 현재 프레임 기준의 원본 단계 계산/재등록 지연은 유지한다. [범위·재현](docs/exe/cpp-frame-display-integration.md).
- [x] **범위/표시 분기:** 패치 `00419850`의 logical/shadow 두 배 범위·SHP 유무·해제 흔적 검사를 크기 공급에 사용했다. CD에는 logical 검사를 추가하지 않고 물리 배열 밖 접근을 거부한다. 두 판본의 old/new 픽셀·해시 이동·그림자·선택 표식·alternate 갱신·같은 프레임 반복·전체 갱신 억제를 검사했다. 잘못된 현재 프레임/미연결 표시/다른 풀·타입·공간도 확인한다.
- [x] **실제 자산 읽기 전용:** 새 `--inspect-frame-binding` 콘솔 명령으로 10.78 자산 116타입/물리 **3,783개**/크기 공급 **3,779개**, CD 101타입/물리 **3,167개**/크기 공급 **3,167개** 통과. 합계 물리 **6,950개**/크기 공급 **6,946개**다. 패치 logical 범위 밖 물리 4개는 메타데이터만 검사했다. 실제 연결 객체의 프레임 변경과 같은 프레임 alternate 변경은 각 판본에서 영역 전달 4회→부분 Draw/Present 1영역으로 이어진다.
- [x] **검증:** x64 Release 경고/오류 0·전체 콘솔 CTest 내부 **251개·실패 0**(기존 246 + 새 5, 테스트 57.48초·전체 57.52초). 관련 감사 display/setframe/pop/unpop **4종 모두 통과**. 최초 선택 표식 기대값의 좌우 확장 계산 2건을 수정한 뒤 전체 회귀 통과. 새 독립 기계어 입력은 없으며 누적 제한 x86 **131,542개 유지**다. 로그 `extracted/build-frame-display.log`·`extracted/ctest-frame-display.log`·`extracted/frame-binding-originals.log`·`extracted/frame-binding-cd.log`·`extracted/audit-frame-display.json`.
- [x] **당시 다음 구현 (1) 완료(위 Graph 활성 절):** Graph 활성 상태의 끝 칸 생성/삭제/Pop 및 받침 생성 통합. 이번 실제 SHP/표시 훅을 재사용하고 Graph 번호/표/깊이·공통 장부·공간 쓰기를 함께 확인한다. (2) raw GameWorld/GUI와 Kernel 프레임·게임 시각 연결. 그 뒤 애니메이션 프로세스·walker 낙하·파편·소리·배치·건설·경제·전투·승패.
- [x] **✅ 위 절에서 연결 완료. 추가 당시 점검 인계 — 다리 destroy 재정의 연결(2026-10-08 코드 점검에서 추가, VM-W11-CODEX, 코드 변경·빌드/테스트 없음):** 다리 가상 표 +0x10은 공통 destroy(`004af780`)가 아니라 `004220f0`/CD `00449820`이다. 편집기가 아니고 권한 플래그(`00540bc4`)가 켜져 있으며 `(extra & 9) == 0`이고 그래프 표면 수가 5 이상일 때, 단단한 프레임(0x40)이거나 디버그 유지 플래그(`0054db80`)가 켜져 있으면 삭제하지 않고 돌아간다([근거 5절](docs/exe/cpp-bridgedecay-reconstruction.md)). 이 판정은 상태 모델 [`Bridge::DestroyProceeds`](cpppj/src/o/Bridge.h)에만 있고 raw 삭제 경로(`RawSquidDestroy`·[`RawSurfaceLifecycle`](cpppj/src/o/RawSurfaceLifecycle.h)·다리 이벤트 처리기의 destroy 훅)는 공통 destroy를 직접 부른다. Graph 비활성에서는 표면 수 조건이 걸리지 않아 결과가 같지만, **Graph 활성 통합에서는 다리 삭제 호출을 가상 표 기준으로 이 판정에 먼저 통과시켜야 한다.** [`SurfaceLifecycleTests`](cpppj/tests/SurfaceLifecycleTests.cpp)의 Hard 시나리오 마지막 강제 삭제(`destroy.Destroy(source,0,...)`)는 Graph 활성·표면 수 5 이상에서는 원본이 거부하는 호출이므로 그때 기대값을 다시 정한다. 원본의 어느 호출 지점이 가상 +0x10을 거치고 어느 지점이 공통 destroy를 직접 부르는지는 아직 확인하지 않았다(연결 전에 호출 지점부터 확인한다).
- [ ] **장시간 검사/남은 범위:** 기존 프레임 지정 6개·섬 삭제 훅 5개 변이 검사(약 40분), noIsland/삭제 분배기/표시 연결부 변이 검사는 인계한다. 테스트 포함 15분 이상 작업을 인계하라는 기존 사용자 지침을 유지한다. 이번 통합은 Graph 비활성·공통 표시 override 범위다. 원본 SHP 검사는 C++ 자료 전달/표시 영역 검증이며 새 x86 독립 대조나 GUI raw 스프라이트 동기화 검사가 아니다.

---

## 2026-10-08 ✅ 완료: 섬 삭제·다리 지연 낙하·연결 객체 정리 콘솔 통합 (VM-W11-CODEX)

- [x] **착수/호스트:** AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 직전 다음 구현 (1)을 진행했다. 현재 `VM-W11-Codex`는 마지막 디컴파일 PC `VM-W11-CODEX`와 일치해 같은 PC의 기존 내보내기를 사용했다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08(기존 기록 유지).** 새 내보내기·디컴파일·원본 게임/복사본·클론 창 실행 없음. AGENTS.md·원본 파일·dotnetpj·dotnet 인계는 변경하지 않았고 커밋/푸시하지 않았다.
- [x] **삭제 연결부:** [`RawSurfaceLifecycle`](cpppj/src/o/RawSurfaceLifecycle.h)가 실제 vtable로 다리 pre/post, 받침 pre, noIsland post를 분배한다. 다리 `DestroyLink`는 최종 ProcessHost 훅을 포함한 실제 재귀 삭제, Base는 공통 삭제/장부, walker·파편/소리는 외부 경계로 이어진다. 선택/form 계약 보존·미연결 훅/풀/타입 오류 거부·분배기 복사/이동 금지를 추가했다. [연결과 검사 범위](docs/exe/cpp-surface-lifecycle-integration.md).
- [x] **실제 raw 통합:** Factory/Pop으로 noIsland 9칸→받침/종유석 생성→K 다리와 연결 객체 생성→받침 실제 삭제→payload 7196의 Regular 예약→Kernel 다음 프레임→O 끝 프레임 56 새 다리 생성/옛 다리 삭제→dead 참조에 따라 연결 객체 삭제→실행 중 ProcessForm 정리까지 연결했다. 소유자/수명/부모 단어/0x10 비트·해시/spot·비용/통계·SID 여유 수·삭제/등록 깊이를 검사한다.
- [x] **분기/표면:** 10.78/CD 각각 정상 교체·고립 삭제·hard 유지·flags `0x1000` 보존·noIsland 삭제의 walker 낙하 분배를 확인했다. hard의 예약/연결은 유지되며 후속 실제 부모 삭제가 둘을 정리한다. 받침만 삭제한 단계에서는 표면 9칸과 종유석이 남는다. 포괄적인 섬 제거 명령/실제 낙하 몸체는 후속이다.
- [x] **빌드/검증:** x64 Release 경고/오류 0·콘솔 CTest 내부 **246개·실패 0**(기존 240 + 새 6, 테스트 60.22초·전체 60.27초). 관련 감사 7종(bridgeeffects/event/connect·islandlifecycle/postpop·setframe·process) 모두 통과. 새 독립 기계어 관찰 없음, 누적 제한 x86 **131,542개 유지**. 최초 SID 기대값의 받침 반납 누락을 수정한 뒤 최종 전체 검사가 통과했다. 로그 `extracted/build-surface-lifecycle.log`·`extracted/ctest-surface-lifecycle.log`·`extracted/audit-surface-lifecycle.json`.
- [x] **후속 완료:** SquidFrame 표시 콜백과 실제 SHP 크기 공급은 위 최신 기록에서 완료했다.
- [ ] **다음 구현:** (1) Graph 활성 상태의 끝 칸 생성/삭제/Pop 및 받침 생성 통합. (2) raw GameWorld/GUI와 Kernel 프레임·게임 시각 연결. 그 뒤 walker 낙하·파편·소리·배치·건설·경제·전투·승패.
- [ ] **장시간 검사:** 기존 프레임 지정 6개·섬 삭제 훅 5개 변이 검사(약 40분), noIsland/새 분배기 변이 검사는 인계한다. 테스트 포함 15분 이상 작업을 다음 작업으로 인계하라는 기존 사용자 지침을 유지한다. Graph 비활성·합성 SHP/비다리 프레임·표시/소리/보상 기록 훅 범위이며 GUI에서 이번 raw 흐름을 확인한 것은 아니다.

---

## 2026-10-08 ✅ 완료: noIsland 최초 등록·받침 소유자 전파, regiongraph 감사 복구 (VM-W11-CODEX)

- [x] **착수/호스트:** 사용자 요청에 따라 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 먼저 읽었다. 현재 호스트 `VM-W11-Codex`는 마지막 디컴파일 PC `VM-W11-CODEX`와 일치한다. 새 관련 함수만 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08.** AGENTS.md·원본 게임 파일·dotnetpj/C#·dotnet 인계는 수정하지 않았고 커밋/푸시하지 않았다.
- [x] **noIsland postPop:** `004423b0`/CD `004d2790`, 서/북 프레임 조회 `00442350`/CD `004d03a0`, 받침 묶음 소유자 `0044bd20`/CD `00461f00`를 [`RawIslandPostPop`](cpppj/src/o/RawIslandPostPop.h)으로 복원했다. 최초 등록 flags 1 → 중첩 값이 있으면 프레임 직접 선택 → F 받침 생성/소유자/Pop → H 3×3 표면/받침/종유석 소유자·표시 갱신 → 연결 순회 순서다. `SquidPostPop::SetSurfacePrefix`/`HandlesSurface`로 실제 가상 표에 접두를 공급한다. 누락된 접두는 Pop의 공간 쓰기 전에 거부한다. [근거](docs/exe/cpp-islandpostpop-reconstruction.md).
- [x] **소수 좌표 정정:** 표면 머리 조회의 진입 조건은 0.9999를 더해 절삭하지만 실제 3×3 반복 기준점은 보정 없이 절삭한다. H 왼쪽 위는 패치 `(float)(x−2)+0.9999`, CD x87의 `x−1.0001`이다. 서/북 및 소유자 순회 고정 지도와 현재 해시 지도도 별도로 받을 수 있게 했다. 디컴파일 C의 빠진 x87 연산을 실제 명령으로 확인했다.
- [x] **독립 제한 x86:** [`tools/decomp_islandpostpop_oracle.py`](tools/decomp_islandpostpop_oracle.py), 세 실제 PE × 552(접두 402 + 소유자 전파 150) = **1,656개**, 공유 장면 332개, x87 두 정밀도에서 같은 관찰만 저장. 사건 순서/인자·생성 슬롯 전체·전체 풀 Adler-32가 C++과 일치한다. 생성/가상 소유자/Pop/표시/공통 postPop과 진단 문자열/표시/assert는 기록 대체다. 판본별 예상 진단/assert 각각 130회, 예상 밖 assert 0. 새 Ghidra 목록 [`islandpostpop-functions.json`](tools/ghidra/islandpostpop-functions.json): 함수 5/4/4.
- [x] **실제 raw 통합(콘솔):** 10.78/CD에서 noIsland 9칸을 실제 Factory·Pop으로 배치 → 첫 F가 받침 생성 → 받침 postPop이 종유석 생성 → 마지막 H가 9칸/받침/종유석 소유자 3·색 프레임 2를 맞춤. 표시 훅 11회, 공통 통계 9/1/1, 중첩 깊이 0, 누락 진단 0. Graph 비활성·합성 프레임/표시 훅이며 GUI raw 월드는 미연결이다.
- [x] **이전 인계 2 해결 — regiongraph:** 준비된 내보내기로 `decomp_regiongraph_oracle.py` 재생성. 기존 **1,536개 fixture는 바이트 단위로 그대로**(`git diff` 없음), 증거 JSON의 현재 PC 디컴파일 SHA·누적 함수 경계 메타데이터만 갱신. `--verify` 통과. 기존 30개 감사 전부 통과했고 새 도구 포함 **31개 모두 통과**다. 재생성 로그 `extracted/regiongraph/regenerate-vmw11.log`.
- [x] **빌드/콘솔 검증:** x64 Release 경고/오류 0·CTest 내부 **240개·실패 0**(기존 234 + 새 6, 전체 100.04초). 누적 제한 x86 **131,542개**. `extracted/build-islandpostpop-final.log`·`extracted/ctest-islandpostpop.log`·`extracted/audit-islandpostpop-prior.json`.
- [x] **클론 창 회귀:** window·renderer·menu·world 스모크 4종 모두 통과. `extracted/gui-<종류>-islandpostpop-smoke.log`. 같은 PC의 앞 단계 창 허용과 AGENTS.md의 호스트 예외를 확인했다. 원본 게임/복사본은 실행하지 않았다. 설정 파일은 스모크가 기존 상태로 복구했다. raw 받침의 실제 GUI 표시를 확인한 것은 아니다.
- [ ] **변이 검사:** 앞 단계 프레임 지정 6개·섬 삭제 훅 5개(약 40분 예상)는 계속 인계한다. 새 noIsland 모듈 변이 검사는 아직 실행하지 않았다. 15분 이상 작업을 인계하라는 앞 단계 사용자 지침을 유지한다.
- [x] **후속 완료:** 섬 삭제 → 지연 낙하 → 다리 끝 칸 변환 → 연결 객체 정리 콘솔 통합은 위 최신 기록에서 완료했다.
- [ ] **다음 구현:** (1) `SquidFrame` 표시 갱신을 실제 `SquidDisplay::Update`에 연결하고 실제 SHP 크기 공급. (2) Graph 활성 상태의 끝 칸 생성/삭제/Pop 및 받침 생성 통합. (3) raw GameWorld/GUI와 Kernel 프레임·게임 시각 연결. 그 뒤 walker 낙하·파편·소리·다리 배치·건설·경제·전투·승패.

---

## 2026-10-08 ✅ 완료 / ⏭ 일부 인계: 다리/섬 연결·소유자 전파, 프레임 지정, 섬 삭제 훅 (VM-W11-CODEX, 클론 창 회귀 실행·원본 게임 실행 없음)

- [x] **지침/범위:** 사용자 요청 "cpppj 프로젝트 계속 진행, 이 PC에서는 창 띄우는 작업 허용"에 따라 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 바로 아래 절의 "다음 구현 (1)"을 진행했다. 호스트 `VM-W11-CODEX`(10.0.0.17, AGENTS.md의 게임 구동 확인 면제 시스템 2). **원본 게임·복사본·업데이터는 실행하지 않았다.** 클론 창(`NetstormCpp.exe --run`) 회귀만 실행했다. 원본 디렉터리 파일은 `git status`상 변경 없음(스모크가 `options.cfg`·`fullscreenStateFile.dat`를 복구). **커밋/푸시하지 않았다.**
- [x] **AGENTS.md 변경 확인(사용자 요청):** 작업 중(11:29) 사용자가 AGENTS.md에 두 규칙을 추가했다(HEAD 대비 2줄 추가, 그 밖은 그대로. **에이전트는 AGENTS.md를 수정하지 않았다**). ① cpppj 작업·디컴파일 소스 분석 전에 이 문서에서 마지막 디컴파일 PC 호스트명을 확인하고 현재 PC와 다르면 디컴파일을 다시 한 뒤 작업한다. ② 디컴파일 소스가 바뀌면 작업 내역 문서와 이 문서에 마지막 디컴파일 수행 PC 호스트명을 명시/갱신한다. **반영:** 이 문서 머리말에 "마지막 디컴파일 수행 PC" 줄을 만들었고 [내보내기 문서](docs/exe/ghidra-exports.md)와 새 근거 문서 셋에 호스트명을 적었다. 이번 세션은 직전 기록이 `HJOW-Athlon`이어서 이 PC와 달랐고, **이 PC에서 내보내기 전체를 다시 만든 뒤** 작업했다(기본 Ghidra 프로젝트 셋은 이 PC에 2026-10-07 17:09~17:18에 만들어진 것이 있었고, 그 위의 내보내기가 다른 PC의 기록 SHA와 일치함을 감사로 확인했다).
- [x] **사용자 지시 "15분 이상 걸리는 작업은 테스트 포함해 다음 작업으로 인계"(11:14):** 돌고 있던 변이 확인을 중단했고(아래 인계 1), 소요 시간을 모르는 `regiongraph` 재생성은 시작하지 않았다(인계 2). 그 뒤로는 각 15분 미만인 것만 실행했다.
- [x] **이 PC 준비:** `extracted/oracle-python`에 Unicorn 2.1.4 격리 설치(한글 주석 때문에 `PYTHONUTF8=1` 필요), `export_functions.ps1 -All`, CRLF 사본 21개를 LF로 다시 받음. 감사는 처음 27개 중 26개 통과(`regiongraph`만 실패). 기준선 Release 빌드·CTest 통과.
- [x] **다리/섬 연결·연결 객체 생성·소유자 전파:** `004213b0`·`004210f0`·`00421240`(CD `00448b00`·`004487c0`·`00448910`), 한 칸 타입 조회 `004b1fa0`, 소유자 색 프레임 `004421a0`, 섬 받침·종유석의 소유자 지정 재정의 `00442310`, 섬 받침 postPop 재정의 `004421c0`, 탐색기 flags 0~7과 Next 순회. C++ [`RawBridgeConnect`](cpppj/src/o/RawBridgeConnect.h)·[`RawSquidNeighborWalk`](cpppj/src/o/RawSquidNeighbors.h), `SquidPostPop::SetIslandPrefix`/`HandlesDerived`. 새 도구 [`tools/decomp_bridgeconnect_oracle.py`](tools/decomp_bridgeconnect_oracle.py): 세 PE × 3,600 = **10,800개**(장면 1,056개 공유, 판본마다 프로세스 하나). 대체: 생성·가상 소유자 지정·Pop·프레임 지정·표면 알림·base 소유자 지정·공통 postPop. **C++ 본체는 수정 없이 전부 일치.** 타입 번호: 연결 객체 155(bridgeConnector)·섬 받침 94(island)·종유석 95(islandStalag)·표면 칸 157(noIsland). [근거](docs/exe/cpp-bridgeconnect-reconstruction.md).
- [x] **앞 단계 코드 정정(원본 관찰로 확인):** 연결 필터의 "후보 기준점 spot"은 좌표를 자른 칸이 아니라 **0.9999를 더해 자른 칸**에서 읽는다. 처음 입력에서는 변이 확인이 이 차이를 검출하지 못해 전용 입력 32장면을 더했고, 세 PE 모두 "자른 칸에만 내부 비트 → 이웃으로 받음 / 더해 자른 칸에만 → 거부"였다. `RawSquidNeighbors::Accept`를 고쳤고 기존 `neighbor` 기대값 1,728개도 그대로 통과한다.
- [x] **Squid 프레임 지정:** `004acee0`/CD `004acbc0`. 현재 프레임의 해시 단계가 저장된 단계 바이트와 같으면 표시 갱신 → 프레임 쓰기 → 표시 갱신, 다르면(새 프레임이 현재와 다를 때만) Unpop → 프레임 쓰기 → 같은 자리에 Pop(flags | 0x50). 해시 단계는 한 번 늦게 옮겨진다. C++ [`SquidFrame`](cpppj/src/o/SquidFrame.h), 도구 [`tools/decomp_setframe_oracle.py`](tools/decomp_setframe_oracle.py): 세 PE × 1,924 = **5,772개**(사건마다 호출 시점의 프레임을 함께 대조), 첫 실행 일치. 다리/섬 연결 통합 검사의 프레임 지정 경계를 이 함수로 바꿨다. [근거](docs/exe/cpp-setframe-reconstruction.md).
- [x] **섬 받침 preDestroy·noIsland postDestroy:** `00442240`·`00442640`(CD `004d0250`·`004d2a40`). 섬 받침이 지워질 때 flags에 `0x1000`이 없으면 연결 이웃 가운데 죽지 않은 다리마다 받침 중심으로 지연 낙하(`0x2692`) 예약, 표면 칸이 지워질 때 그 칸(해시 0단계 제외)의 walker마다 가상 낙하. C++ [`RawIslandLifecycle`](cpppj/src/o/RawIslandLifecycle.h), 도구 [`tools/decomp_islandlifecycle_oracle.py`](tools/decomp_islandlifecycle_oracle.py): 세 PE × 520 = **1,560개**, 첫 실행 일치. 실제 프로세스 계층(ScheduleBridgeFall·Kernel)에 이은 통합 검사 포함. [근거](docs/exe/cpp-islandlifecycle-reconstruction.md).
- [x] **실제 raw 모듈 통합(콘솔):** 두 판본에서 중립 섬 받침 Pop → 종유석 생성(SquidFactory)·소유자(가상 표 분배 → SquidOwner)·Pop → 소유자 3의 다리 Pop → 다리 postPop 접두 → 연결 순회 → 연결 객체 생성/등록 → 섬 받침 소유자 3·색 프레임 2·종유석 프레임 2·발자국 안 표면 칸 둘의 소유자 3 → 다른 소유자의 다리가 닿아도 주인 있는 섬은 그대로. noIsland는 파생 postPop이 미복원이라 직접 배치했다.
- [x] **재현성 보강:** 목록 파일이 없던 `pop`·`display`·`postpop`·`graph`·`lifecycle`·`regiongraph`의 함수 목록을 문서의 명령과 증거 JSON의 누적 `function_ranges`에서 복원해 `tools/ghidra/<이름>-functions.json`으로 넣었다(새 내보내기의 주소·범위가 두 판본 모두 기록과 순서까지 같음을 확인). `export_functions.ps1`: `aliases`(pop의 `helpers-originals` 폴더), `powershell -File ... -Name a,b`의 쉼표 목록 처리. 이 PC에서 `refine_all.ps1`(정밀 디컴파일, 22분)도 실행해 `extracted/refined/`가 있다. **Ghidra 헤드리스는 같은 설정 폴더로 동시에 두 개를 돌리면 번들 캐시 잠금으로 멈춘다**(실제로 한 번 겪었다). [정리](docs/exe/ghidra-exports.md).
- [x] **테스트 지원:** `cpppj/tests/TestSupport.h`의 `CHECK`를 함수 호출로 바꿔, 테스트 전체를 다시 컴파일할 때만 보이던 C4127 경고 4건(상수 조건식)을 없앴다. 장면 준비 코드를 `cpppj/tests/RawSceneSupport.h`로 묶어 두 검사 파일이 함께 쓴다.
- [x] **최종 검증(모두 각 15분 미만):** x64 Release 경고/오류 0(테스트 전체 재컴파일 포함)·CTest 내부 **234개·실패 0**(218 + 다리/섬 연결 6 + 프레임 지정 5 + 섬 삭제 훅 5, CTest 전체 59.73초). 제한 x86 새 **18,132개**, 누적 **129,886개**. 감사 **30개 중 29개 통과**(실패는 `regiongraph` 하나 — 인계 2). **클론 창 회귀 4종 통과:** `cpp_window_smoke`(26초)·`cpp_renderer_smoke`(17초)·`cpp_menu_smoke`(8초)·`cpp_world_smoke`(15초), 로그 `extracted/gui-*-smoke.log`. 최종 CTest 로그 `extracted/final-ctest-vmw11.log`.
- [x] **변이 확인(실행한 것):** 다리/섬 연결 변이 19개 — 첫 실행 18개 검출·`neighbor-candidate-spot-no-bias` 미검출 → 전용 입력을 더한 뒤 다시 실행해 **검출**(19개 모두 검출). 프레임 지정 변이 7개 가운데 `frame-single-update` 하나만 실행해 검출. 로그 `extracted/bridgeconnect/mutation.log`·`mutation-second.log`.
- [x] **문서:** 새 근거 문서 셋(위 링크), `ghidra-exports.md`·`cpp-neighbor-reconstruction.md`·`cpppj/README.md`·`docs/cpp-build.md`·`cpp-roadmap.md`·`cpp-playable-plan.md`. dotnetpj에 넘길 확정 사항(연결 조각·섬 점령·색 프레임·섬 삭제 때의 다리/유닛)은 LEFT_JOBS.dotnetpj.md 8절에 적었다(문서만, C# 코드는 읽거나 고치지 않았다).
- [ ] **⏭ 인계 1 — 변이 확인 11개 미실행(사용자 지시로 중단, 약 40분 예상):** 프레임 지정 6개와 섬 삭제 훅 5개. 변이 하나에 빌드+전체 검사로 3~4분이 걸린다. 도구가 실행 중에 강제 종료돼 `extracted/mutation-work` 사본에 변이된 파일이 남아 있을 수 있으나 도구가 시작할 때 저장소와 다시 맞춘다(저장소 소스는 건드리지 않는다). 결과를 `cpp-setframe-reconstruction.md`·`cpp-islandlifecycle-reconstruction.md`의 "변이 확인" 절에 적는다. `island-pre-schedules-dead-bridge`는 연결 필터가 dead 후보를 먼저 걸러 미검출일 수 있다(그러면 그 조건이 도달하지 않는 중복 검사라는 사실을 적는다).
  ```powershell
  python -X utf8 tools/cpp_mutation_check.py --only frame-write-before-first-update --only frame-same-frame-still-repops --only frame-repop-plain-flags --only frame-bridge-level-by-size --only frame-level-of-stored-not-written --only frame-write-after-repop --only island-pre-ignores-keep-flag --only island-pre-schedules-dead-bridge --only island-pre-schedules-any-surface --only surface-post-includes-level0 --only surface-post-ignores-buried
  ```
- [x] **⏭ 인계 2 완료(후속 세션) — `regiongraph` 재생성·감사 통과, fixture 불변. 아래는 당시 준비 기록:** 이 PC에서 `decomp_regiongraph_oracle.py --verify`는 `extracted/regiongraph/<originals|originalCD>/creation.c` SHA에서 멈춘다(함수 범위 `functions.tsv`는 세 판본 모두 기록과 일치, 디컴파일 텍스트만 10.78·CD에서 다름 — 기록은 다른 PC의 예전 Ghidra 프로젝트 상태로 보인다). 재생성에 필요한 입력(`pop`·`display`·`postpop`·`graph` 내보내기, `extracted/refined/`)은 **이 PC에 모두 준비돼 있다.** 할 일: `python -X utf8 tools/decomp_regiongraph_oracle.py` → `git status --short cpppj/tests/fixtures/regiongraph-x86.tsv`가 비어 있는지(fixture가 바이트 단위로 그대로인지) 확인 → `--verify`. fixture가 바뀌면 증거를 되돌리고 원인을 먼저 본다. 같은 방법으로 `pop`·`display`·`postpop`·`graph`·`rawgraph` 도구의 재생성도 확인할 수 있다(하지 않았다). **다른 PC에서는** 먼저 `export_functions.ps1 -All`과 `refine_all.ps1`이 필요하다.
- [x] **⏭ 인계 3 완료(후속 세션) — noIsland postPop 재정의 `004423b0`/CD `004d2790`.** 최신 복원/대조는 [근거 문서](docs/exe/cpp-islandpostpop-reconstruction.md)를 따른다. 아래는 소수 좌표 보정이 빠져 있던 당시 정적 후보 기록이다. 아래는 **기계어를 읽은 정적 판독이며 x86 대조를 하지 않았다.** flags 비트 1(최초 등록)일 때만: (a) 중첩 카운터 `005c89b8`(요새 읽기/배치 중에 증가)가 0이 아니면 서쪽 칸 `(x−1, y)`와 북쪽 칸 `(x, y−1)`의 noIsland 프레임 글자를 표면 지도(`005c84bc` = 해시 0단계 배열)로 읽어(`00442350`) 자기 글자를 고른다 — 서쪽이 없으면: 북쪽도 없으면 F, 북쪽이 F면 B, B면 I, 그 밖은 F / 서쪽이 C→G, F→C, A→D, B→A, E→H, I→E, 그 밖은 북쪽 규칙. 3×3이 왼쪽 위부터 `F C G / B A D / I E H`로 채워진다. 프레임 = `FindNumber(글자, 'P', 1)`(`0049a940`). (b) 글자가 F면 섬 받침을 만들어(생성 flags 2) 자기 소유자를 주고 `(x + 발자국폭−1, y + 발자국높이−1)`에 Pop한다. (c) `005c85d4`가 0이고 글자가 H면 `(x−2, y−2)`의 표면 객체 소유자로 `0044bd20(x, y, 소유자)`를 부른다 — 3×3 칸의 표면 객체와 섬 받침에 가상 소유자 지정 + 표시 갱신을 하는 함수로 읽었다(끝부분 미판독). (d) 연결 순회 `004213b0(자기, 0, 0)`. 그 뒤 항상 공통 postPop. 대조 도구는 `decomp_bridgeconnect_oracle.py`의 `ConnectOracle`을 상속하면 장면 준비를 재사용할 수 있다(표면 지도 전역만 추가).
- [ ] **다음 구현(위 셋 다음):** (1) 섬 삭제 → 지연 낙하 → 다리 끝 칸 변환 → 연결 객체 정리(`RawBridgeLifecycle::LinkNeedsDestroy`)를 한 흐름으로 잇는 콘솔 통합. (2) `SquidFrame`의 표시 갱신 훅을 `SquidDisplay::Update`에 연결하고 프레임 크기를 실제 SHP 추가 헤더에서 공급. (3) Graph 활성 상태의 끝 칸 생성/삭제/Pop·연결 순회 통합 대조. (4) 실제 GUI raw GameWorld에서 이 모듈들을 쓰고 Kernel 프레임/게임 시각을 게임 루프에 잇는다. (5) walker 낙하·Flyingshrapnel·소리 출력, SharedRegular/다른 파생 프로세스·AI 부착 통지·SID 소진·다리 배치(미리보기 경로 호출자 `00445d87` 포함)·건설/경제/전투/승패.
- [ ] **창 검증:** 이 PC에서는 창 작업이 허용된다(사용자 지시, 이 PC 한정). 기존 GUI(임시 객체 모델)의 회귀 4종은 이번에 통과했다. **raw 월드를 GUI에 연결한 뒤** TEST01/1-1에서 다리 연결 조각·섬 점령 색 변화·다리 붕괴→끝 칸 변환→낙하를 화면으로 확인하는 일은 남았다.

---

## 2026-10-08 ✅ 완료: flag 0 첫 연결 이웃·다리 postPop 접두·끝 칸 raw 모듈 통합 (HJOW-Athlon, 게임/창 실행 없음)

- [x] **지침/범위:** 사용자 요청에 따라 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 먼저 읽고 최신 cpppj 인계의 첫 작업을 진행했다. 10.78 기준, CD/추가 10.37 비교. 읽기 전용 Ghidra·Unicorn·콘솔 빌드/검사만 실행했다. AGENTS.md·원본 파일·dotnetpj·dotnet 인계는 수정하지 않았으며 커밋/푸시하지 않았다.
- [x] **flag 설명 정정:** `004215d0`와 CD 인라인 `00449a20`이 넘기는 값은 **`004b23e0(source, 0)`**이다. 앞선 인계의 flag 8 설명을 정정했다. 생성 직후 +0x34가 실제 첫 결과임을 대조했다. flag 0은 네 단계 일반 해시+연결 필터이며 flag 8 지도 반복자로 대체할 수 없다.
- [x] **첫 이웃 복원:** `RawSquidNeighbors`가 source 내부 spot AND → 일반 finder → 축별 발자국 확장 교차 XOR → 저장 중심의 방향 → 프레임 접합 → dead/후보 내부 제외를 수행한다. 호출 시점의 raw 프레임/체인을 읽고 첫 반환 뒤 후보를 미리 읽지 않는다. `MakeBridgeNeighborHooks`로 실제 조회와 동일한 프레임 표를 끝 칸 변환에 연결했다.
- [x] **새 독립 x86:** `tools/decomp_neighbor_oracle.py`는 탐색 대체를 제거해 세 PE의 첫 결과 **864개**·실제 탐색을 넣은 이벤트 **864개**, 합계 **1,728개**를 저장했다. 두 슬롯 모든 바이트·반환 float·호출 순서/인자를 비교한다. 소수 좌표·가장자리·네 단계·큰 발자국·등록 순서·dead/buried/내부·권한 없음 등을 포함한다. 생성/삭제/소유자/Pop은 이 x86 묶음에서는 외부 대체다.
- [x] **다리 전용 postPop 접두:** 실제 다리 vtable +0x20은 `00422150`/CD `00449890`이다. flags 비트 1이면 extra 비트 2 기록 후 연결, 아니면 flags 비트 4이고 abstract 아님이면 연결 → 공통 postPop 순서를 복원했다. `tools/decomp_bridgepostpop_oracle.py`의 세 PE **240개** 접두 대조(연결/공통 함수는 호출 기록 대체)와 쓰기 전 거부·공통 통계 억제 순서 검사로 확인했다. 기존 공통 가상 표 목록은 보존하고 정확한 다리 표와 명시적 connector가 있는 인스턴스만 추가로 허용한다.
- [x] **실제 raw 통합:** 두 판본·보통/단단한/고립 칸에서 지연 낙하→Kernel→가상 처리기→첫 이웃→SquidFactory→SquidOwner→다리 삭제 훅/공통 장부→Unpop/반납/종속 ProcessForm 정리→SquidPop/postPop을 검사했다. 실제 bridge.type 60프레임, 소수 위치, 소유자/수명/부모 단어/플래그/좌표, 해시/spot/타입 수/비용, 실행 중 Regular 제거와 풀 카운터를 확인했다. **비전투 Pop·Graph 비활성·연결 생성/소유자 전파·파편/소리/보상/UI 외부 콜백** 범위다.
- [x] **최종 검증:** x64 Release 경고/오류 0·콘솔 CTest 내부 **218개·실패 0**(이전 211 + 새 7; CTest 전체 86.80초). 제한 x86 새 **1,968개**, 누적 **111,754 = 109,786 + 1,728 + 240**. 새 두 도구는 x87 53/64비트 동일 관찰·assert 0이다. 지원하는 `--verify` 감사 **27개 모두 통과**(`config`·`graphics`·`options`는 이 옵션이 없는 옛 생성기). 로그 `extracted/neighbor-ctest.log`, 증거 `cpppj/recovery-neighbor-evidence.json`·`recovery-bridgepostpop-evidence.json`, 새 내보내기 `extracted/bridgepostpop/<판본>/`.
- [x] **문서:** [복원 근거](docs/exe/cpp-neighbor-reconstruction.md)·[내보내기](docs/exe/ghidra-exports.md)·다리 이벤트/일반 finder·cpppj README·빌드/로드맵/실제 플레이 계획을 갱신했다. 아래 과거 수치는 당시 이력이다.
- [ ] **다음 구현:** (1) **✅ 2026-10-08 `VM-W11-CODEX`에서 완료(위 절):** 다리 연결 함수 `004213b0`/CD `00448b00`의 flag 4 이웃 순회, 연결 객체 생성 `004210f0`, 소유자 전파 `00421240`을 복원해 connector 기록 경계를 실제 효과로 바꿨다. 탐색기도 flags 0~7과 Next 순회를 제공한다. 남은 것은 (2) 이하다. (2) Graph 활성 상태의 끝 칸 생성/삭제/Pop 통합 대조. (3) 실제 GUI raw GameWorld에서 이 모듈을 사용하고 Kernel 프레임/게임 시각을 게임 루프에 잇는다. (4) walker 낙하·Flyingshrapnel·소리 출력, SharedRegular/다른 파생 프로세스·AI 부착 통지·SID 소진·건설/경제/전투/승패를 이어간다. 기존 일반 이웃/생성/Pop/삭제의 **콘솔 통합 인계는 완료**했다.

---

## 2026-10-08 ✅ 완료: Ghidra 내보내기 재현 문제 수정(감사 25개 통과)·소유자 지정·다리 처리기 분배 (HJOW-Athlon, 게임/창 실행 없음)

- [x] **지침/범위:** 사용자 요청 "이 PC에서 Ghidra 내보내기 문제 수정 후 cpppj 더 진행"에 따라 진행했다. AGENTS.md·LEFT_JOBS.md 확인. 호스트 `HJOW-Athlon`. **게임/복사본·업데이터/설치 도구·클론 창·`start_session`/`--live`를 실행하지 않았다**(읽기 전용 Ghidra와 Unicorn만). 원본 디렉터리 변경 없음. AGENTS.md·C# 코드는 수정하지 않았다. **에이전트는 커밋/푸시하지 않았다.**
- [x] **내보내기 목록을 저장소에 넣음:** 목록이 문서의 명령이나 다른 PC의 로그에만 있던 `bridgedecay`·`bridgeeffects`·`finder`·`graphrebuild`·`graphrecovery`·`sid`·`creation`을 `tools/ghidra/<이름>-functions.json`으로 만들고, 공용 스크립트 [`tools/ghidra/export_functions.ps1`](tools/ghidra/export_functions.ps1)(`-Name`/`-All`, 읽기 전용, 전체 약 10분)을 추가했다. 전체를 다시 내보내 기존 것과 비교하니 **86개 파일이 바이트 단위로 같았다**(내보내기는 결정적이다). `bridgeevent` 내보내기 폴더도 `extracted/bridgeevent/<판본>/`으로 맞췄다. 정리 문서: [docs/exe/ghidra-exports.md](docs/exe/ghidra-exports.md).
- [x] **`bridgeeffects`·`finder` 순서 복원:** 직접 호출 폐포(`finder`는 간접 호출되는 기본 true 필터 추가)가 문서의 함수 수와 같았고, **저장 순서는 기록된 `functions.tsv` SHA에 맞는 순열을 찾아 확정**했다. 다시 내보낸 `creation.c` SHA까지 기록과 일치한다.
- [x] **`bridgedecay` 목록 재정의:** 처음 기록(패치 51·CD 35개)의 목록이 남아 있지 않고 SHA에 맞는 구성을 찾지 못해, **기대값 생성 전체를 실제로 돌려 실행된 함수만** 추렸다(패치 48·CD/10.37 각 34개, 진입점 먼저 + 첫 실행 순서, 대체 진입점 함수 제외). 이 목록만으로 다시 생성한 `bridgedecay-x86.tsv`는 저장소의 것과 바이트 단위로 같다(입력 14,524개). 3개·1개 차이의 원인은 확인하지 못했다.
- [x] **줄바꿈에 좌우되던 SHA 수정:** `decomp_bridgeeffects_oracle.py`·`decomp_rawfinder_oracle.py`가 `newline` 없이 써서 Windows에서 CRLF 파일의 SHA를 기록하던 것을 고쳤고, `.gitattributes`에 `tools/decomp_*.py`·`tools/ghidra/*.java`·`tools/ghidra/*-functions.json`·`cpppj/recovery-*.json`을 `eol=lf`, `*.ps1`을 `eol=crlf`로 고정했다(`decomp_rawgraph_oracle.py`의 CRLF 사본 SHA가 여러 증거에 기록돼 있었다).
- [x] **이 PC의 오래된 사본 교체:** `extracted/sid/originalCD`·`extracted/creation/originalCD`가 예전 프로젝트 상태의 것이었다(잘못된 주소 한 줄·함수 하나 더). 새로 내보낸 파일이 다른 PC가 기록한 SHA와 일치한다(옛 사본은 `extracted/_stale/`에 보관).
- [x] **증거 재기록:** 영향받는 11개 도구(`bridgeevent`·`bridgedecay`·`bridgeeffects`·`rawfinder`·`graphremove`·`graphlookup`·`graphrebuild`·`graphrecovery`·`destroy`·`destroylifecycle`·`destroygraph`)를 이 PC에서 다시 실행했다. **모든 fixture가 바이트 단위로 그대로였고**(증거 JSON의 SHA 항목만 바뀜) 원본 assert 0이다. 그 결과 **`--verify` 감사 25개가 모두 통과**한다(처음에는 24개 중 11개 실패). 로그 `extracted/regen/`.
- [x] **소유자 지정 복원:** base Squid vtable +0x74(`004adf00`/CD `004aefa0`, 다리 vtable도 같은 함수). 범위 검사 → 요새/전투 모드이고 genus가 vortex·factory(`0x4200`)이면 이전 소유자의 작업장 목록에서 이 번호를 모두 제거 → 소유자 바이트 쓰기 → void·buried가 아니면 새 소유자의 목록 끝에 추가(가득 차면 생략). C++ [`SquidOwner`](cpppj/src/o/SquidOwner.h)는 범위 밖 번호(원본 assert)와 목록이 없는 9~39번을 쓰기 전에 거부한다. 새 도구 [`tools/decomp_owner_oracle.py`](tools/decomp_owner_oracle.py)·[`owner-x86.tsv`](cpppj/tests/fixtures/owner-x86.tsv)·[`recovery-owner-evidence.json`](cpppj/recovery-owner-evidence.json): 세 PE × 1,000 = **3,000개, 대체 함수 없음**, assert 0. C++는 첫 실행에서 전부 일치했다. [근거](docs/exe/cpp-owner-reconstruction.md).
- [x] **다리 이벤트 처리기 연결:** `MakeBridgeRegularHandler`가 부모 객체의 **가상 표 기록값**(다리 `005034c8`/CD `00501ab0`)으로 분배한다. 단위 검사 `bridge_event_runs_from_scheduled_fall_through_regular_process`가 지연 낙하 예약 → Kernel 프레임 → 분배 → 끝 칸 변환(소유자 지정은 실제 함수) → 권한 없을 때 예약 종료를 확인한다. 외부 효과(이웃·생성·삭제·Pop)는 여전히 기록 훅이다.
- [x] **확인한 사실:** (1) 글자 표 함수 `0049b060`은 타입 후처리 `0049b0d0`가 모든 타입에 대해 부른다(`0049b4b8`, CD `0044519d`). (2) 실제 `bridge.type`은 프레임 60개·글자 16종이고 끝 프레임은 L 47·M 50·N 53·O 56(보통 프레임), J·K에서 교체가 막히는 것은 번호 20(0x40) 둘뿐이다. (3) "표면 알림" `004214a0`은 상태를 바꾸지 않는 디버그 검사(수명 > 7이면 assert)이고 CD `00448c10`은 슬롯 주소만 돌려준다 — 연결하지 않아도 게임 상태는 같다.
- [x] **변이 확인 도구를 안전하게 고침:** `tools/cpp_mutation_check.py`가 저장소 소스를 직접 바꾸던 것을 **Git 제외 폴더 `extracted/mutation-work`의 사본**에서 별도 빌드로 하도록 바꿨다(변이 확인 중에 커밋해도 변이가 저장소에 들어가지 않는다). 변이 전 사본의 검사가 모두 통과해야 진행한다. 새 변이 5개(처리기 분배 1·소유자 지정 4)를 더해 정의는 18개다. 새 변이 5개는 **모두 검출**됐다(처리기 분배는 통합 단위 검사가, 소유자 지정 4개는 세 판본 fixture가 검출). 이번에는 앞선 13개를 다시 돌리지 않았다.
- [x] **빌드/회귀(최종):** x64 Release 경고/오류 0·CTest 내부 **211개·실패 0**(103.26초; 이전 205 + 소유자 지정 4 + 다리 이벤트 2), 누적 제한 x86 **109,786개 = 106,786 + 3,000**. 감사 25개 통과.
- [x] **문서:** 새 [소유자 지정 근거](docs/exe/cpp-owner-reconstruction.md)·[내보내기 정리](docs/exe/ghidra-exports.md), `cpp-bridgeevent/bridgedecay/bridgeeffects/rawfinder-reconstruction.md`, `cpppj/README.md`·`docs/cpp-build.md`·`cpp-roadmap.md`·`cpp-playable-plan.md`를 맞췄다. dotnetpj에 넘길 확정 사항(소유자 지정)은 LEFT_JOBS.dotnetpj.md 8절에 적었다.
- [ ] **⚠ 다른 PC(DESKTOP-HJOW 등)에서 받은 뒤 할 일(※ `VM-W11-CODEX`에서는 2026-10-08에 마쳤다 — 맨 위 절. 다른 PC에는 여전히 해당하며, AGENTS.md의 새 규칙대로 그 PC에서 디컴파일/내보내기를 다시 만든 뒤 작업하고 이 문서 머리말의 "마지막 디컴파일 수행 PC"를 갱신한다):** (1) `powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -All`로 내보내기를 다시 만든다 — 특히 `bridgedecay`는 목록이 바뀌어 예전 내보내기로는 `--verify`가 실패한다. (2) `.gitattributes`가 바뀌었으므로 CRLF로 남아 있는 도구 사본(`tools/decomp_rawgraph_oracle.py` 등)은 지운 뒤 `git checkout -- <파일>`로 다시 받는다. (3) 그 뒤 모든 `tools/decomp_*_oracle.py --verify`가 통과하는지 확인한다.
- [ ] **다음 구현 인계:** (1) **2026-10-08 후속 완료:** 실제 인자는 flag 0이며 첫 이웃·생성/Pop/삭제를 콘솔 통합했다(위 완료 기록). 다리 연결 객체/소유자 전파 함수와 Graph 활성 통합·GUI 월드 연결은 남았다. (2) walker 낙하·Flyingshrapnel 파편·`00422300`의 소리. (3) SharedRegular(61)·프로세스 전송 직렬화·다른 파생 프로세스와 DependForm/GumpForm/ContentForm. (4) Kernel 프레임 실행을 게임 루프/게임 시각(`0055b4d0`)에 연결. (5) 소유자 지정을 재정의하는 파생 vtable이 있는지 전수 확인(패치판에서 base 함수를 가리키는 칸은 73개), 지연 낙하(`0x2692`)를 예약하는 호출 경로(`0x4422db`). (6) AI 부착 통지(`004166e0`), 파생 vtable +0x80, 샘 생성 코드, raw GameWorld. 다리 배치/Construction·SID 소진·건설/경제/전투/승패·V12 비교 도구 확장도 남았다. GUI는 임시 객체 모델이며 이번 raw 계층을 게임 플레이 완료로 해석하지 않는다.
- [ ] **창 검증:** raw 월드 연결 뒤 TEST01/1-1 다리 붕괴→끝 칸 변환→파편/소리/낙하를 확인한다. 이번 단계는 월드에 연결하지 않아 창 검사를 하지 않았다.

---

## 2026-10-07 ✅ 완료: cpppj 다리 이벤트 처리기·끝 칸 변환·글자 표와 프로세스 보강 재생성·변이 확인 (HJOW-Athlon, 게임/창 실행 없음)

- [x] **지침/범위:** AGENTS.md·LEFT_JOBS.md(맨 위 인계 2건과 다음 구현 목록)를 읽고 cpppj를 이어서 진행했다. 호스트 `HJOW-Athlon`은 AGENTS.md의 게임 구동 확인 면제 시스템이지만 **게임/복사본·업데이터/설치 도구·클론 창·`start_session`/`--live`를 실행하지 않았다**(읽기 전용 Ghidra와 Unicorn 에뮬레이션만). 원본 디렉터리는 `git status`상 변경 없음. AGENTS.md·C#·LEFT_JOBS.dotnetpj.md는 수정하지 않았다. 기준은 10.78, CD/추가 10.37은 비교 자료다. **커밋/푸시하지 않았다.**
- [x] **인계 2 — 변이 확인(완료):** 이 PC의 `core.autocrlf=true` 때문에 변이 도구가 줄바꿈이 있는 원문 5개를 찾지 못했다. 도구가 파일의 줄바꿈에 맞춰 원문/변이문을 바꾸도록 고쳤다(`adapt`). 기존 7개 중 **6개 검출, `process-unlink-under-dead-parent`(부모가 지워지는 중인데 form을 체인에서 뺌) 1개 미검출**(실패 검사 0). 이 변이는 연산마다 끝 상태를 비교하는 기계어 관찰로는 구별되지 않는다(원인은 추정: 부모 정리 과정에서 체인 조정이 끝 상태에 남지 않음). **단위 테스트 `process_form_unpop_keeps_chain_while_parent_is_dying`을 추가했고 그 변이가 검출된다.** 이 확인은 재생성 전 fixture로 했고, 재생성한 fixture만으로 검출되는지는 따로 보지 않았다.
- [x] **인계 1 — 보강 재생성(완료):** 커버리지 패치를 적용해(`git apply`) `extracted/process/<판본>/`(읽기 전용 Ghidra, 이 PC에서 새로 내보냈고 기록된 SHA와 **같았다** — 내보내기가 결정적이다) 위에서 재생성했다(약 38분). **시나리오 360개·연산 5,893개**(패치 1,935·CD 1,979·10.37 1,979; 이전 6,044개), 제외 0·`--verify` 통과. 이벤트 조회 성공 패치 11→**77**회·CD 16→**71**회, 종료 요청 무시 분기는 CD/10.37 **2회씩** 도달했다. **패치판에서는 무시 분기가 여전히 0회**이며 원인은 확인하지 않았다(그 판본은 정적 판독 구현 그대로). **C++는 수정 없이 새 fixture 전체와 일치했다.** 패치 파일은 지웠고 `.gitattributes`의 그 줄도 지웠다.
- [x] **읽기 전용 디컴파일:** [`tools/ghidra/bridgeevent-functions.json`](tools/ghidra/bridgeevent-functions.json)으로 패치 **8개**·CD/10.37 각 **6개**를 `extracted/bridgeevent/<판본>/`에 내보냈다(Git 제외). CD와 10.37은 이 함수들의 주소와 디컴파일 결과가 바이트 단위로 같다.
- [x] **확정한 사실:** 다리 vtable `005034c8`/CD `00501ab0`의 +0x5c가 이벤트 처리기 `00422740`/`00449a20`이다. `0x2691`=권한 있으면 자기 destroy(-1.0), `0x2692`=payload를 `_ftol`해 x=하위 8비트·y=부호 있는 >>8로 풀고 끝 칸 변환(-1.0), 권한 없으면 둘 다 0.0, 그 밖은 payload 반환. 끝 칸 변환(패치 `004215d0`, CD는 처리기에 인라인): 현재 프레임이 J면 `objY < y`→N 아니면(NaN 포함) L, K면 `objX < x`→M 아니면 O → 임시로 새 프레임을 쓰고 flag 0 이웃 탐색기 첫 이웃(+0x34)을 본 뒤(2026-10-08 인자 정정) **곧바로 프레임 복원**, 이웃 없으면 destroy(0) → 수명 비트를 4로 쓰고 표면 알림 → **프레임 플래그 0x40 검사는 새 프레임이 아니라 복원한 옛 프레임에 대해** 하며 켜져 있으면 교체하지 않음 → 아니면 같은 타입 새 객체를 만들어 프레임·소유자(vtable +0x74)·수명 단어(다리 타입이면 표면 알림)·부모 단어·플래그 0x10을 옮기고 옛 객체 destroy 후 같은 위치에 Pop. 타입의 `+0x2c + 글자*4`는 `0049b060`/CD `00444da0`이 프레임 코드에서 만드는 **글자(A~P)별 첫 프레임 번호 표**(+0x130, 없으면 -1; 개수 +0x170, 글자 수 +0x12c)다.
- [x] **C++:** [`RawBridgeEvents.{h,cpp}`](cpppj/src/o/RawBridgeEvents.h)(`Handle`·`ConvertEnd`·`BridgeEventState`·`BridgeEventHooks`)와 `RiftTypeFrames::Run`/`LetterKinds`. 훅(이웃 탐색·표면 알림·생성·destroy·소유자·Pop·타입 프레임 조회)은 호출자가 연결하며 호출 순서·인자만 원본과 같다. 타입에 끝 프레임 글자가 없으면(원본은 -1을 프레임으로 써 정의되지 않는 동작) 쓰기 전에 예외로 거부한다.
- [x] **원본 x86/콘솔:** [`tools/decomp_bridgeevent_oracle.py`](tools/decomp_bridgeevent_oracle.py)·[`bridgeevent-x86.tsv`](cpppj/tests/fixtures/bridgeevent-x86.tsv)·[`recovery-bridgeevent-evidence.json`](cpppj/recovery-bridgeevent-evidence.json). 세 PE×x87 53/64비트(결과가 다르면 중단, 같을 때만 저장), **Handler 3,621·End 726(패치 직접 호출)·Table 306 = 4,653개**, 원본 assert 0·허용 밖 쓰기/실행 0. 실제 명령: 처리기·끝 칸 변환·방향 글자·타입 조회·단어 쓰기·`_ftol`·글자 표 생성. 대체(호출 사실과 인자 기록): 이웃 탐색기 생성(첫 이웃은 입력)·표면 알림·새 객체 생성(헤더만)·가상 destroy/소유자/Pop. 슬롯 쓰기는 허용한 필드만 받고 두 슬롯은 Adler-32로 비교한다. 이웃 탐색기가 **임시 프레임이 쓰인 채로** 불리는지도 사건에 프레임 값을 기록해 대조한다.
- [x] **변이 6개 추가(모두 검출):** 새 구현을 한 군데씩 틀리게 바꿔 확인했다(단단한 프레임 검사를 새 프레임으로, J 판정 NaN, 수명 비트, 플래그 복사, 권한 없음 반환, 글자 첫/마지막 프레임). 도구에 정의를 추가했다. 총 변이 13개 모두 검출.
- [x] **빌드/회귀(최종):** x64 Release 경고/오류 0·CTest 내부 **205개·실패 0**(104.23초; 이전 200 + 새 검사 4개 + 프로세스 단위 테스트 1개), 누적 제한 x86 **106,786개 = 102,133(프로세스 5,893 반영) + 4,653**. 새 도구 `--verify`·프로세스 `--verify` 통과. 로그 `extracted/process/`·`extracted/bridgeevent/`.
- [x] **문서:** 새 [근거 문서](docs/exe/cpp-bridgeevent-reconstruction.md), `cpp-process-reconstruction.md`(보강 재생성·변이 확인 결과)·`cpp-bridgedecay-reconstruction.md`·`cpppj/README.md`·`docs/cpp-build.md`·`cpp-roadmap.md`·`cpp-playable-plan.md`를 맞췄다. 루트 `README.md`/`README.en.md`에는 이 수치가 없어 바꾸지 않았다.
- [x] **✅ 해결: 커밋 주의.** 22:14의 커밋 `1007 - 15`에는 변이 확인 도구가 `RawBridgeEvents.cpp`에 적용해 둔 변이 한 줄(`letter = (objectY >= y) ? kEndL : kEndN;`)이 들어갔으나, 커밋 `1007 - 16`(a3fa059)에서 올바른 식 `(objectY < y) ? kEndN : kEndL`로 바뀌었다(`git diff HEAD`가 비어 있음을 확인). 변이 확인(`tools/cpp_mutation_check.py`)은 실행하는 동안 `cpppj/src`의 파일을 일시적으로 바꾸므로 그 사이에는 커밋하지 말고, 커밋 전에 `git diff cpppj/src`로 변이가 남아 있지 않은지 확인하라.
- [x] **Ghidra 내보내기를 이 PC에서 다시 만들었다(사용자 요청, `HJOW-Athlon`, 읽기 전용·게임 실행 없음). ※ 아래에 적은 `--verify` 실패 원인 두 가지와 `bridgedecay` SHA 문제는 위 2026-10-08 절에서 모두 해결했다.** `extracted/`는 Git에 없어 PC마다 다르다. 이 PC에는 이전 단계들의 내보내기가 없어 `tools/decomp_*_oracle.py`의 생성/`--verify`가 돌지 않았다. `tools/ghidra/run_script.ps1 -Script ExportCreation.java`로 아래를 만들었다(`extracted/<이름>/<판본>/creation.c`·`functions.tsv`, 로그 `extracted/ghidra-export-logs/`·`extracted/ghidra-export-run*.log`). **기록된 증거 SHA와 바이트 단위로 일치:** `process`·`reward`·`destroy`·`destroylifecycle`·`destroygraph`(각 `tools/ghidra/<이름>-functions.json`), `graphremove`(18/18, base/geometry/lookup), `graphrebuild`(6/6)·`graphrecovery`(28/28)(주소는 해당 문서의 명령), `bridgeeffects`(패치 12·CD/10.37 각 7)·`finder`(패치 8·CD/10.37 각 6)(직접 호출 폐포가 문서 개수와 같음을 확인한 뒤, **함수 저장 순서는 기록된 `functions.tsv` SHA에 맞는 순열을 찾아 확정**). 새로 만든 `bridgeevent`는 `tools/ghidra/bridgeevent-functions.json`. **`bridgedecay`는 기록된 목록·순서를 알 수 없어 SHA를 맞추지 못했다**(`export_sha256` 불일치 예상): 폐포(68/49개)는 실행되지 않는 함수까지 포함해 문서의 51/35개보다 크고, 실제로 오라클을 돌려 실행된 함수만 추려(대체 진입점 함수 제외) 패치 48·CD/10.37 각 34개로 내보냈다(문서의 51/35개와 3개·1개 차이, 원인 미확인; 진입점 먼저+첫 실행 순서/주소 정렬/폐포 순서 가설은 모두 SHA와 맞지 않았다). 그래도 **이 내보내기만으로 `decomp_bridgedecay_oracle.generate()`를 돌린 결과(372초)가 저장소의 `bridgedecay-x86.tsv`와 바이트 단위로 같고** 허용 밖 실행·assert가 없었다(파일은 저장소에 쓰지 않고 임시 경로에서 비교). **이 PC에서 아직 `--verify`가 실패하는 것과 원인(내보내기 문제가 아님):** (1) 증거가 줄바꿈에 의존한다 — 기록된 SHA는 `tools/decomp_rawfinder_oracle.py`가 LF 사본, `tools/decomp_rawgraph_oracle.py`·`cpppj/tests/fixtures/bridgeeffects-x86.tsv`·`rawfinder-x86.tsv`가 **CRLF 사본** 기준이라 저장소(LF)·이 PC 사본과 맞지 않는다(`graphremove`·`graphrebuild`·`graphrecovery`·`graphlookup`은 rawgraph 도구 SHA에서, `bridgeeffects`는 fixture SHA에서, `rawfinder`는 도구/fixture SHA에서 멈춘다). 해결안: 해당 파일을 `.gitattributes`에 `eol=lf`로 고정하고 증거를 다시 만들거나, 해시 전에 줄바꿈을 LF로 정규화하도록 도구를 고친다. 이번에는 기존 증거·도구를 바꾸지 않았다. (2) `extracted/sid/originalCD`·`extracted/creation/originalCD`(기존 PC-로컬 내보내기)가 destroy 계열이 기록한 SHA와 다르고 `extracted/creation/<판본>/sid.c`가 이 PC에 없다 — `destroy`·`destroylifecycle`·`destroygraph`의 `--verify`는 여기서 멈춘다(`sid`·`creation` 자체 `--verify`는 통과). 어느 PC의 프로젝트 상태가 달라진 것인지는 확인하지 못했다. `--verify`가 통과하는 것: `creation`·`derived`·`display`·`graph`·`pop`·`postpop`·`process`·`rawgraph`·`regiongraph`·`reward`·`sid`·`spatial`·`unpop`·`bridgeevent`. WSL/Linux 빌드는 하지 않았다. `core.autocrlf=true`라 작업 트리의 일부 파일은 CRLF다.
- [ ] **다음 구현 인계(처리기 연결·소유자 지정·글자 표 확인은 위 2026-10-08 절에서 완료, 나머지는 그쪽을 따른다):** (1) 이벤트 처리기를 `SquidProcessHost`의 `RegularHandler`에 부모 타입별로 연결(다리 타입이면 `RawBridgeEvents::Handle`, 그 밖은 payload)하고, 훅을 실제 `RawSquidFinder`/표면 이웃 탐색기·`SquidFactory::Create`·`SquidPop`·`RawSquidDestroy`·소유자 지정(`004adf00`, 아직 미복원)에 잇는 통합. (2) walker 낙하·Flyingshrapnel 파편·`00422300`의 소리. (3) SharedRegular(61)·프로세스 전송 직렬화·다른 파생 프로세스와 DependForm/GumpForm/ContentForm. (4) Kernel 프레임 실행을 게임 루프/게임 시각(`0055b4d0`)에 연결. (5) 타입 로더가 글자 표 함수(`0049b060`)를 부르는 위치와 실제 `bridge.type`의 L~O 프레임 번호 대조. (6) AI 부착 통지(`004166e0`), 파생 vtable +0x80, 샘 생성 코드, raw GameWorld. 다리 배치/Construction·SID 소진·건설/경제/전투/승패·V12 비교 도구 확장도 남았다. GUI는 임시 객체 모델이며 이번 raw 계층을 게임 플레이 완료로 해석하지 않는다.
- [ ] **창 검증:** raw 월드 연결 뒤 TEST01/1-1 다리 붕괴→끝 칸 변환→파편/소리/낙하를 확인한다. 이번 단계는 월드에 연결하지 않아 창 검사를 하지 않았다. 해당 PC의 AGENTS.md/사용자 지시를 확인한다.

---

## 2026-10-07 ✅ 구현·검증 완료 / ⏭ 보강 재생성·변이 확인 인계: cpppj 객체 부착 프로세스·Regular 이벤트 (DESKTOP-HJOW, 게임/창 실행 없음)

- [x] **지침/범위:** AGENTS.md와 두 인수인계 문서(LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md)를 읽고 cpppj 다음 항목을 진행했다. 작업 중 사용자가 AGENTS.md의 인수인계 규칙을 바꿨고(두 문서 확인, cpppj·공통은 이 문서, dotnetpj는 별도 문서) 그대로 따랐다. 기준은 10.78, CD/추가 10.37은 비교 자료다. 게임/복사본·업데이터/설치 도구·클론 창·`start_session`/`--live`를 실행하지 않았다. **커밋/푸시하지 않았다.** AGENTS.md·C#은 수정하지 않았다.
- [x] **항목 선택 이유:** 다리 지연 낙하(`0x2692`)를 따라가 보니 객체에 붙는 프로세스(ProcessForm)가 "종속 form의 파생 release/destroy", "0x2692 등록/실행/취소", 이후 모든 타이머/이동/건설 프로세스의 공통 기반이었다. 그래서 이 계층을 먼저 복원했다.
- [x] **읽기 전용 디컴파일:** `tools/ghidra/process-functions.json`으로 패치 **73개**, CD/추가 10.37 각 **71개**를 `extracted/process/<판본>/`에 내보냈다(Git 제외). 대조 도구의 `--trace`로 허용 목록 밖 진입점을 조사해 목록을 채웠다.
- [x] **확정한 구조:** 프로세스 = Kernel 슬롯의 객체 + 부모 객체 안의 form SID(타입 바이트가 프로세스 타입 번호 10~62, `+0x12`에 Kernel 번호). form은 부모의 종속 체인 머리에 들어가며 체인은 양방향이다(form: `+0xe` 이전·`+0x10` 부모, contained 자산: `+0xe` 부모·`+0x10` 이전 — 자산 쪽은 정적 확인). 프로세스를 지우는 길은 form의 공통 destroy뿐이고 form의 preDestroy가 Kernel에서 제거한다. 부모가 지워지면 종속 루프가 모든 form을 정리한다. Regular는 만든 다음 프레임에 처음 불리고, 부모의 이벤트 처리기(vtable +0x5c) 반환값이 양수면 그 초 뒤 재예약·0이면 종료·음수면 유지다. base 처리기는 payload를 그대로 돌려준다. geyser(122)에 붙은 Regular는 0.5초보다 먼 예약을 당긴다.
- [x] **판본 차이:** free/dead 부모 부착(패치: 조용히 건너뜀, CD: assert 뒤 진행), abstract 부모(패치만 건너뜀), NaN 반환(패치만 재예약), Kernel 슬롯 39999/3999, 패치만 등록 때 타입 순환 값(`0059af74`) 전진, 프로세스 타입 끝 63/62.
- [x] **C++:** `SquidProcess.{h,cpp}`(`SquidProcess`·`RegularProcess`·`SquidProcessHost`·`ScheduleBridgeFall`/`HasScheduledBridgeFall`), `Kernel::Get`, `RawSquidDestroy`의 form 루트 지원(`SquidDestroyHooks::unpopForm`이 있을 때만). `SquidProcessHost::Hooks()`가 ProcessForm 루트/종속의 pre/post/release/destroy/Unpop을 처리하고 나머지는 자산 훅으로 넘긴다. CD의 free/dead 부모 부착과 서버의 비로컬 flags(프로세스 전송 직렬화)는 등록 전에 거부한다.
- [x] **원본 x86/콘솔:** `tools/decomp_process_oracle.py`·`cpppj/tests/fixtures/process-x86.tsv`·`cpppj/recovery-process-evidence.json`. 세 실제 PE×x87 53/64비트, **시나리오 360개·연산 6,044개**(패치 1,974·CD 2,035·10.37 2,035), 정밀도 불일치 제외 0·원본 assert 0·허용 밖 실행/쓰기 0·정상 반환/ESP/x87 TOP/FS:[0] 확인. 부모의 가상 pre/post/이벤트 처리기·프로세스 new/free·삭제 로그·삭제 전파만 대체한다. 연산마다 풀 전체 Adler-32·삭제 기록·Kernel 슬롯·프로세스 필드·깊이·호출 기록을 비교한다. C++는 첫 비교에서 전부 일치했다.
- [x] **빌드/회귀(최종):** x64 Release 경고/오류 0·CTest 내부 **200개·실패 0**(97.58초, 기존 191개 + 새 9개), 누적 제한 x86 **102,284개 = 96,240 + 6,044**. `--verify` 감사 8종(프로세스·삭제 보상·장부·Graph·destroy·일반 탐색·다리 효과·삭제 준비) 통과, 원본 여섯 디렉터리 **2,782개 파일 SHA/목록 동일**. 로그 `extracted/process/build.log`·`ctest.log`.
- [x] **문서:** 새 [근거 문서](docs/exe/cpp-process-reconstruction.md), `cpppj/README.md`·`docs/cpp-build.md`·`cpp-roadmap.md`·`cpp-playable-plan.md`·루트 `README.md`/`README.en.md`·`cpp-reward/destroy-reconstruction.md`를 맞췄다. dotnetpj에 넘길 확정 사항(삭제 보상·지연 이벤트)은 규칙대로 [LEFT_JOBS.dotnetpj.md 8절](LEFT_JOBS.dotnetpj.md)에 적었다(C# 코드는 읽거나 고치지 않음).
- [x] **✅ 인계 1 완료(위 새 절, HJOW-Athlon) — 보강한 대조 입력 재생성·재검사(당시 이 PC에서 중단했던 내용):** 현재 fixture는 이벤트 조회가 실제로 찾은 경우가 적고(패치 11회·CD 16회), "권한 없음(boss=0) + 서버 번호 form + flag 0" 종료 요청이 무시되는 분기는 **한 번도 도달하지 않았다**(C++는 정적 판독대로 구현). 보강한 생성기 변경을 [`tools/decomp_process_oracle.coverage.patch`](tools/decomp_process_oracle.coverage.patch)로 남겼다. 절차: `git apply tools/decomp_process_oracle.coverage.patch` → `$env:PYTHONPATH='extracted/oracle-python'; python -X utf8 tools/decomp_process_oracle.py`(이 PC에서 15분 넘게 걸려 중단함; 세 PE의 읽기 전용 내보내기 `extracted/process/`가 없으면 `process-functions.json`으로 먼저 내보낸다) → `--verify` → 빌드/CTest. 불일치가 나오면 C++(`SquidProcessHost::Kill`·`Find`)를 고친다. 끝나면 패치 파일을 지우고 연산 수가 바뀐 만큼 이 문서와 근거 문서·README 등의 6,044/102,284 수치를 고친다. **재생성 전까지 도구는 현재 fixture를 만든 바이트 그대로다**(`--verify`가 도구 SHA를 확인한다).
- [x] **✅ 인계 2 완료(위 새 절, HJOW-Athlon) — 변이 확인(당시 설명):** 검사가 구현 차이를 실제로 잡는지 확인하는 절차다. 이 PC에서 시작했다가 결과가 나오기 전에 중단했고 **소스는 원래 바이트로 복구해 재빌드했다**(결과 없음). 도구: [`tools/cpp_mutation_check.py`](tools/cpp_mutation_check.py). `--list`로 일곱 변이(프로세스 5·삭제 보상 2)의 원문이 한 곳씩 있음만 확인했다. 실행: `python -X utf8 tools/cpp_mutation_check.py --cmake <cmake 경로>`(변이마다 빌드+검사 약 2~3분, 끝나면 원래 소스로 되돌려 재빌드). **일곱 개 모두 "검출"이어야 한다.** "미검출"이 나오면 그 분기를 덮는 대조 입력이 없다는 뜻이므로 인계 1의 생성기를 더 보강한다. 게임/창 실행은 없다.
- [ ] **다음 구현 인계(처리기 몸체·끝 칸 변환은 위 새 절에서 완료, 나머지는 그쪽을 따른다):** (1) ~~다리 이벤트 `0x2692`의 처리기 몸체·끝 칸 변환~~ → 완료. walker 낙하·Flyingshrapnel 파편. (2) SharedRegular(61)·프로세스 전송 직렬화·다른 파생 프로세스(이동·건설·전투·애니메이션)와 DependForm/GumpForm/ContentForm의 가상 함수. (3) Kernel 프레임 실행을 게임 루프/게임 시각(`0055b4d0`) 전진에 잇기. (4) AI 부착 통지(`004166e0`), 파생 vtable +0x80, 샘 생성 코드, raw GameWorld. 다리 배치/Construction·SID 소진·건설/경제/전투/승패·V12 비교 도구 확장도 남았다. GUI는 임시 객체 모델이며 이번 raw 계층을 게임 플레이 완료로 해석하지 않는다.
- [ ] **다른 PC 창 검증:** 호스트 **DESKTOP-HJOW에서는 원본/복사본·업데이터/설치 도구 실행과 모든 창 검사를 금지**한다. 다른 허용 PC에서 raw 월드 연결 후 TEST01/1-1 생성→선택→삭제(보상/SP 지급)/반납→재진입과 Graph/파편/소리/낙하·전체화면을 검사한다. 해당 PC의 AGENTS.md/사용자 지시를 확인한다.

---

## 2026-10-07 ✅ 완료: cpppj 삭제 보상(SP)·SP 저장소·샘 풀 복원과 Graph 문서 동기화 (DESKTOP-HJOW, 게임/창 실행 없음)

- [x] **지침/범위:** AGENTS.md·LEFT_JOBS.md를 먼저 읽고 다른 AI의 미완 항목(문서 동기화)과 다음 구현(보상/SP)을 이어서 진행했다. 기준은 10.78이며 CD/추가 10.37은 비교 자료다. 게임/복사본·업데이터/설치 도구·클론 창·`start_session`/`--live`를 실행하지 않았다. **작업 중 사용자가 AGENTS.md에 추가한 지침(커밋/푸시 금지)을 확인했고 커밋하지 않았다.** AGENTS.md·C#·LEFT_JOBS.dotnetpj.md는 내가 수정하지 않았다.
- [x] **읽기 전용 디컴파일:** `tools/ghidra/reward-functions.json`으로 패치 **27개**, CD/추가 10.37 각 **24개**를 `extracted/reward/<판본>/`에 내보냈다(Git 제외). 두 판본 코드 배치는 같다.
- [x] **의미 확정:** 보상 `0044c2f0`/CD `004626d0`는 처치한 소유자(flags 16..19비트)에게 비용×`DAT_005424bc`%(25)를 SP로 주고, 세 번째 인자가 참(해체)이면 남은 HP 비례 환불을 준다. 지급하지 않은 가치는 전역 정수(패치 `00595088`, CD `00512110`)에 쌓이며 샘 생성 코드가 비교하므로 **샘 풀**로 부른다(사용처 추정). 타입 번호 전역은 PE 초기값으로 확인했다(164 altar 제외·165 dais·147 rainBlocker·150 growingRainBlocker·154 nugget·158 priest). vtable +0x80은 base가 상태 바이트 상위 3비트를 돌려준다.
- [x] **패치판 SP 저장소 발견:** `0040eba0/0040ec50/0040edb0`은 소유자별 SP 32비트를 32개의 9칸 DWORD 배열에 비트 단위로 흩뿌려 저장한다. 쓰기마다 난수 잡음 160개(비트당 5개)를 섞고 **전역 난수 상태(`00532710`)를 전진**시킨다. 첫 접근에서 시각 기반으로 난수를 재시드한다(C++는 입력 주입). CD판은 로컬 SP만 전역 float이고 다른 소유자는 AI 지갑 정수만 갱신한다.
- [x] **C++:** `SpStore.{h,cpp}`(`GameRandom`·`ScrambledSpStore`), `SquidReward.{h,cpp}`를 추가했다. 판본별 x87 순서(패치: 같은 단가 누적/`float(HP×비용)÷최대HP`, CD: `(개수+1)×단가`/`float((HP÷최대HP)×비용)`)·SP 고정 모드(194162/100000)·AI 지갑 가산과 0x4a6 하한·샘 풀 가산을 복원했다. `SquidDestroyLifecycle`에 선택적 `SquidReward*`를 연결했다(없으면 기존 보상 사건 콜백 유지, 있으면 `Plan`으로 쓰기 전 검사 후 실제 지급). 최대 HP 0은 기계어로 확인한 대로 환불 0이다.
- [x] **원본 x86/콘솔:** `tools/decomp_reward_oracle.py`·`cpppj/tests/fixtures/reward-x86.tsv`·`cpppj/recovery-reward-evidence.json`. 세 실제 PE×x87 53/64비트, 입력 **7,200개**(각 2,400), 정밀도 불일치 제외 0·원본 assert 0·허용 밖 쓰기/실행 0·정상 반환/ESP/x87 TOP 확인. UI 갱신·시각 재시드·힙 할당·assert 보고·파생 vtable +0x80 개수만 대체한다. 샘 풀·소유자 0..8 지갑·SP(패치는 실제 읽기 함수)·전역 난수·32개 SP 배열 Adler-32·offset·UI/가상 개수 횟수를 비교한다.
- [x] **빌드/회귀:** x64 Release 경고/오류 0·CTest 내부 **191개·실패 0**(61.41초, 기존 182개 + 새 9개), 누적 제한 x86 **96,240개 = 89,040 + 7,200**. 새 검사: 세 PE fixture, 난수=지형 생성기 식, SP 저장소 왕복/보호, 입력 거부, 장부 연동 3개. 이전 `--verify` 감사(장부/Graph/destroy/일반 탐색/다리 효과/삭제 준비)와 원본 여섯 디렉터리 **2,782개 파일 SHA/목록 동일** 확인.
- [x] **문서 동기화 완료:** 직전 단계가 미룬 항목을 마쳤다. `cpppj/README.md`·`docs/cpp-build.md`·`cpp-roadmap.md`·`cpp-playable-plan.md`·루트 `README.md`/`README.en.md`·`cpp-destroylifecycle/destroy/rawfinder/destroygraph-reconstruction.md`에 Graph(182개/89,040개)와 보상(191개/96,240개)을 반영하고 새 [근거 문서](docs/exe/cpp-reward-reconstruction.md)를 추가했다. 과거 수치는 당시 이력으로 구분했다.
- [ ] **다음 구현 인계(종속 ProcessForm의 release/destroy와 0x2692 예약/조회·실행 틀은 위 최신 절에서 완료, 나머지는 그쪽을 따른다):** AI 부착 통지(`004166e0` 등, 지금은 AI 부착 소유자의 삭제를 거부), 종속 form/process 파생 release/destroy·참조 수명, 다리 전용 훅/장부/Graph 통합, Flyingshrapnel 파편·walker 낙하·0x2692 등록/실행/취소·끝 칸 변환, 파생 vtable +0x80(dais 전달 `0044a480`·프레임 기반 `004478f0`)과 raw GameWorld. 샘 풀을 읽는 샘 생성 코드와 SP 표시/UI(`0043dad0`)도 아직 없다. 다리 배치/Construction·SID 소진·건설/경제/전투/승패·V12 비교 도구 확장도 남았다. GUI는 임시 객체 모델이며 이번 raw 계산을 게임 플레이 완료로 해석하지 않는다.
- [ ] **다른 PC 창 검증:** 호스트 **DESKTOP-HJOW에서는 원본/복사본·업데이터/설치 도구 실행과 모든 창 검사를 금지**한다. 다른 허용 PC에서 raw 월드 연결 후 TEST01/1-1 생성→선택→삭제(보상/SP 지급)/반납→재진입과 Graph/파편/소리/낙하·전체화면을 검사한다. 해당 PC의 AGENTS.md/사용자 지시를 확인한다.

---

## 2026-10-07 ✅ 빌드/검사까지 완료·나머지 인계: cpppj 공통 삭제 Graph 연결 (DESKTOP-HJOW, 게임/창 실행 없음)

- [x] **종료 범위:** AGENTS.md·LEFT_JOBS.md를 먼저 읽고 기존 다음 Graph 작업을 진행했다. 마지막 사용자 지시 **“빌드 테스트까지만 진행하고 나머지는 인수인계 작성해줘.”**에 따라 Release/콘솔 CTest 완료 뒤 추가 기능 구현·창 실행·다른 문서 동기화를 멈추고 이 인수인계만 갱신한다. 커밋하지 않았다. AGENTS.md·C#·LEFT_JOBS.dotnetpj.md·원본 파일은 변경하지 않았다.
- [x] **읽기 전용 재디컴파일:** 새 `tools/ghidra/destroygraph-functions.json`으로 pre/post·위치/프레임·Free/Detach·발자국·finder/Add를 `extracted/destroygraph/<판본>/creation.c`·`functions.tsv`에 패치 15개·CD/추가 10.37 각각 17개 내보냈다. 세 read-only 완료 로그 확인. 기존 내보내기·독립 도구/fixture/기록은 보존했다. 10.78을 기준으로 유지하며 10.82/업데이터/DevLog 관련 기존 비교 기록을 따른다.
- [x] **C++ 연결:** `SquidDestroyLifecycle`의 선택 처리 뒤·장부 억제 앞에 Graph Detach/Free를, 실제 Unpop 뒤·비용 차감 앞에 현재 발자국의 RawSquidFinder→Surface별 순차 Add를 연결했다. `0x800` Free가 `0x2000000` noGraph보다 우선하며 raw root의 graph byte만 해제한다. noGraph는 pre 분할만 막고 post Add는 남긴다. buried는 두 Graph 효과를 막지만 abstract/장부 억제는 막지 않는다. Free는 앞 두 WORD만 지우고 reserved/raw 번호/스택을 유지한다.
- [x] **특수 분기/공간 계약:** 두 PE의 초기 영역 mask `0x50444200`·특수 타입 157·치환/9 감소 타입 162를 상태 기본값으로 넣었다. 특수 타입은 방향 `H`에서 삭제 위치 정보만 치환하며 raw 타입과 전체 풀 재구성 입력은 유지한다. 치환 발자국/프레임/flags2를 지역 연결에 사용한다. lifecycle 생성 시 Graph와 destroy/Unpop의 풀·타입·해시·spot 연결을 확인한다. post의 연속 Add는 풀 사본에서 계산하고 성공한 번호/표/스택만 반영한다. 상위 Destroy의 dead·콜백·후속 Unpop/post 실패까지 전체 롤백하는 것은 아니다.
- [x] **새 원본 대조:** `tools/decomp_destroygraph_oracle.py`·`cpppj/tests/fixtures/destroygraph-x86.tsv`·`cpppj/recovery-destroygraph-evidence.json` 추가. 세 실제 PE×x87 53/64비트 **1,728개**(직접 Pre 576·직접 Post 576·실제 Destroy/Unpop/Release 576). 교차/다중 칸·지도 1/255 경계·해시 단계/같은 위치의 다른 SID·254·dead/void/buried 후보·Free/noGraph·Graph/장부 억제·H/다른 방향·치환 발자국/9 감소·raw 타입 보존·깊이를 대조한다. 슬롯/255개 Graph 레코드 세 WORD는 직접 비교, 전체 풀/공간/장부/스택은 Adler-32다.
- [x] **실제/대체 경계:** pre/post·Graph·위치/발자국/finder·장부·destroy/Unpop/Release·CRT 기록 이동은 실제 x86이다. 각 PE Pre/Post 384·Destroy/Release 192·Unpop 180·Detach 220·Free 20·Add 360·Begin 480/Next 1,280·발자국 2,238·Allocate 504/Flood 512·AI null 래퍼 168, assert 0·정상 반환/ESP/x87 확인. 선택 UI 조회 576/해제 192·공통/Graph 로그 1,064는 명시적 대체다. 보상/SP·실제 소리·전파/종속 파생 효과는 외부 경계이며 이번 입력에서 발생하지 않는다. 합성 타입/FrameCode/SHP·정수·확보한 client 풀·동결 시계·표시 억제·AI null·비전투 null 큐·Graph 소진 전 범위다. 별도 FS 감사나 실제 게임/OS/창 검증은 없다. 내부 도달/준비/비용 접두 실행은 공개 입력 수에 더하지 않는다.
- [x] **빌드/회귀:** 최종 x64 Release 경고/오류 0. 전체 콘솔 CTest 1개 실행 파일의 내부 **182개·실패 0**, 테스트 87.53초(CTest 전체 87.55초). 새 7개 검사는 원본 Pre/Post/통합 Destroy 대조, Free 보존/번호 보호, 필요한 H 치환만 검증, 후보 오류의 쓰기 전 거부, 다른 공간/타입 연결 거부다. 누적 제한 x86은 **89,040개 = 87,312 + 1,728**다. 추가 구현/재빌드/테스트 확대는 하지 않는다.
- [x] **SHA/원본 보호:** 새 Graph와 기존 장부·공통 삭제·일반 finder·다리 효과/붕괴·Graph 소진 복구·삭제 준비의 `--verify` SHA/행 수/호출 경계 감사 통과. 여섯 원본 디렉터리 **2,782개 파일의 SHA/목록이 시작과 동일**함을 빌드/검사 단계에서 확인했다. `originals` 1,394·CD 425·10.37 261·Patches 7·10.82 358·10.82v12 337개. 새 로그는 `extracted/destroygraph/build.log`·`ctest.log`·`oracle.log`·각 `<판본>-ghidra.log`, 보호 기준은 `protected-before.json`이다.
- [x] **수정/추가 파일:** `Graph.{h,cpp}`·`RawGraph.{h,cpp}`·`SquidDestroyLifecycle.{h,cpp}`·`RawSquidDestroy.{h,cpp}`·`SquidUnpop.{h,cpp}`, `cpppj/tests/CMakeLists.txt`·`DestroyGraphTests.cpp`·새 fixture/증거 JSON, `.gitattributes`·새 oracle/Ghidra 목록. 중단 지시 전에 [삭제 Graph 근거 문서](docs/exe/cpp-destroygraph-reconstruction.md)도 작성했다. `extracted/destroygraph/make_tests.py`는 초기 1회 생성 보조이며 최종 테스트를 재생성하는 도구가 아니므로 다시 실행하지 않는다.
- [x] **문서 동기화 인계(위 최신 절에서 완료):** `cpppj/README.md`, `docs/cpp-build.md`·`cpp-roadmap.md`·`cpp-playable-plan.md`, 루트 `README.md`/`README.en.md`, 이전 `docs/exe/cpp-destroylifecycle-reconstruction.md`·`cpp-destroy-reconstruction.md`·`cpp-rawfinder-reconstruction.md`에 최신 Graph 완료·182개/89,040개·검증 범위/남은 작업과 새 근거 링크를 반영한다. 새 Graph 근거 문서에도 최종 빌드/CTest/원본 보호 수치를 보충한다. **이번에는 사용자 중단 지시로 이 동기화를 진행하지 않았으므로 기존 문서의 175개/87,312개·Graph 미연결 표현은 이전 단계 상태다. 현재 상태는 이 문서 맨 위를 따른다.** dotnetpj 전용 기록과 과거 독립 fixture/SHA는 그대로 보존한다.
- [ ] **다음 구현 인계(보상/SP는 위 최신 절에서 완료, 나머지는 그쪽을 따른다):** 공통 pre/post Graph 연결은 완료했다. 실제 삭제 보상 `0044c2f0`/CD `004626d0`의 가상 비용 조회·소유자 비율·SP/누적 수입, AI 부착 통지, 종속 form/process 파생 release/destroy·참조 수명, 다리 전용 훅/장부/Graph 통합, Flyingshrapnel 파편·walker 낙하·0x2692 등록/실행/취소·끝 칸 변환과 raw GameWorld를 잇는다. 다리 배치/Construction·SID 소진·건설/경제/전투/승패·V12 비교 도구 확장도 남았다. GUI는 임시 객체 모델이며 이번 raw Graph 연결을 게임 플레이 완료로 해석하지 않는다.
- [ ] **다른 PC 창 검증:** 호스트 **DESKTOP-HJOW에서는 원본/복사본·업데이터/설치 도구 실행과 모든 창 검사를 금지**한다. 다른 허용 PC에서 raw 월드 연결 후 TEST01/1-1 생성→선택→삭제/반납→재진입과 Graph/파편/소리/낙하·전체화면을 검사한다. 해당 PC의 AGENTS.md/사용자 지시를 확인한다.

---

## 2026-10-07 ✅ 완료: cpppj 공통 pre/postDestroy 장부 연결 (DESKTOP-HJOW, 게임/창 실행 없음)

- [x] **지침/기준:** AGENTS.md·최신 LEFT_JOBS.md와 이전 근거를 읽고 공통 삭제의 다음 장부 처리를 복원했다. 1차 기준 10.78·CD/추가 10.37 비교를 유지했다. AGENTS.md·C#·LEFT_JOBS.dotnetpj.md 변경/커밋은 없다. 이전 일회성 창 허용을 적용하지 않았다.
- [x] **읽기 전용 재디컴파일:** 공통 pre/post·선택 복구·목록/타입 감소·AI null 래퍼·삭제 보상 경계를 새 `extracted/destroylifecycle/`에 내보냈다. 패치 14개·CD/10.37 각각 20개, 세 완료/read-only 로그 확인. 기존 내보내기·독립 도구·fixture는 보존했다.
- [x] **C++ 장부/연결:** `SquidDestroyLifecycle`가 선택/abstract 배치 복구→소유자 작업장→보상 사건→현재 로컬/전역 타입 수 감소→unitLost 사건/좌표→공급→AI(null)→전역 작업장→깊이 감소, post ordinary 비용/깊이 감소를 복원했다. 생성과 같은 `SquidPostPopState`를 공유하고 실제 RawSquidDestroy/Unpop/SID Release에 연결했다. 중복 전부 제거·순서/inactive 꼬리·소유자 0 목록 보존·누적 made 유지·DWORD 감김·패치 silentType의 소리만 제외를 대조했다. post 비용은 장부 억제와 무관하며 패치 정수 비용/CD float 차감 뒤 절삭 차이를 유지한다.
- [x] **의미/범위 정정:** 기존 “경로” 후보 `0044c2f0`/CD `004626d0`는 가상 비용·보상 비율·SP/수입 계산이다. 이번에는 sid/flags 16..19비트/마지막 0을 외부 보상 사건으로 전달하며 실제 SP를 갱신하지 않는다. AI 미부착·Graph 비활성·표시 억제·비전투 null 큐 범위다. 손상 목록/누락 효과·Graph/AI·비유한 비용·다른 풀/root는 해당 장부 쓰기 전에 거부하나 상위 Destroy의 dead·콜백/이후 오류까지 전체 롤백하지 않는다. raw GameWorld는 미연결이다.
- [x] **원본 x86/콘솔:** 세 실제 PE×x87 53/64비트, 새 **1,728개**(직접 pre/post 쌍 1,152·실제 삭제/공간 해제/반납 576). 훅·목록 압축·선택 복구·통계/비용·AI null 래퍼·destroy/Unpop/Release/CRT 기록 이동은 실제 명령, 보상/SP·소리·선택 UI·전파/로그는 명시적 대체다. 직접 쌍의 깊이 DWORD underflow와 통합 삭제의 균형을 구별하며 건물 통합 입력은 이미 void라 건물 Unpop 완료로 세지 않는다. 각 PE 내부 Pre/Post 576·Destroy/Release 192·Unpop 104·owner 제거 232/provider 60/factory 58/AI wrapper 140, 원본 assert 0·정상 반환/ESP/x87 확인. 별도 FS 감사는 없다.
- [x] **최종 검사/보호:** Release 경고/오류 0·CTest 실행 파일 1개 안의 내부 **175개·실패 0**(60.85초), 누적 제한 x86 **87,312개**. 생성→삭제 공유 장부·누적 made 유지·억제/비용·미지원 효과 보호와 기존 전체 회귀를 통과했다. 새/기존 공통 삭제·일반 탐색·다리 훅·붕괴 SHA/행 수/호출 감사 통과. 여섯 원본 디렉터리 **2,782개 파일 SHA/목록이 시작과 동일**하다. 게임/복사본·업데이터/설치·클론 창 실행 없음. 로그 `extracted/destroylifecycle/build.log`·`ctest.log`·`oracle.log`.
- [x] **문서:** [장부 복원/원본 주소/비용 차이/재현](docs/exe/cpp-destroylifecycle-reconstruction.md), cpppj README·빌드/로드맵/플레이 계획·앞선 복원 문서·한국어/영어 루트 README와 현재 인수인계를 맞췄다. dotnetpj 인수인계는 보존했다.
- [x] **다음 Graph 연결 완료:** 공통 pre의 표면 Graph 분할/직접 Free와 post의 주변 finder→표면 Add를 위 후속에서 연결했다. 새 범위/검증과 보상/SP·AI·파생 효과·raw GameWorld 등 남은 구현은 최신 인수인계를 따른다.
- [ ] **다른 PC 창 검증:** 호스트 **DESKTOP-HJOW에서는 원본/복사본·업데이터/설치 도구 실행과 모든 창 검사를 금지**한다. 다른 허용 PC에서 raw 월드 연결 후 TEST01/1-1 생성→선택→삭제/반납→재진입과 Graph/파편/소리/낙하·전체화면을 검사한다. 원본 비교 실행은 해당 AGENTS.md/사용자 지시를 확인한다.

---

## 2026-10-07 ✅ 완료: cpppj 공통 destroy·실제 Unpop/SID 반납·다리 훅의 중첩 삭제 (DESKTOP-HJOW, 게임/창 실행 없음)

- [x] **지침/기준:** 현재 AGENTS.md·LEFT_JOBS.md를 먼저 읽고 기본 삭제의 후속을 진행했다. 기준은 10.78이며 CD/추가 10.37은 비교 자료다. AGENTS.md·C#·LEFT_JOBS.dotnetpj.md 변경/커밋은 없다. 이전 일회성 창 허용은 적용하지 않았다.
- [x] **읽기 전용 디컴파일:** 공통 destroy·client SID·공통 pre/post·파편 효과를 새 `extracted/destroy/<판본>/`에 패치 9개·CD/10.37 각 6개 내보냈다. 세 완료/read-only 로그를 확인했다. 디컴파일의 `unaff_retaddr`를 실제 flags 전달로 정정해 복원했다. 이전 도구/내보내기/fixture는 보존했다.
- [x] **C++:** `RawSquidDestroy`의 dead 선표시·서버/client flags·훅 깊이·pre 이후 head·release 전 next 저장·종속 free 재조회·전파/선택·실제 `SquidUnpop`/`SidPool::Release` 순서를 구현했다. form/contained 종속의 파생 release/destroy와 공통 Pre/Post 전체는 호출자 계약이며 완료 콜백은 깊이 감소만 수행한다. 지원 밖 root/권한/풀은 변경 전, 손상 종속/완료 누락은 해당 효과 경계에서 거부하며 전체 롤백을 보장하지 않는다.
- [x] **다리 중첩 연결:** 실제 일반 `RawSquidFinder`·`RawBridgeLifecycle`에서 DestroyLink가 공통 destroy에 재진입해 공간 해제/반납을 수행한다. 같은 버킷의 next/슬롯이 바뀐 뒤 탐색·다음 참조 판단·post의 walker 호출 순서를 대조했다. 기본 Graph 분할/목록/통계·파편/소리/낙하는 여전히 효과 대체다. raw GUI GameWorld는 미연결이다.
- [x] **의미 정정:** 기존 “제거 통지 목록” `00460600` → `004604a0`는 **Flyingshrapnel 파편/입자 생성**이다(0x68 할당·생성자 `004603a0`·vtable `0050a4f0`·위치/목록 `00460000`). 기존 API 사건 이름 NotifyRemoval/N과 독립 fixture는 유지하며 실제 파편 구현은 후속으로 구분했다.
- [x] **독립 x86/콘솔:** 세 실제 PE×x87 53/64비트, 새 **1,992개**(공통 Destroy 1,800·다리 중첩 192). 실제 common destroy/다리 훅/finder/Unpop/Release와 CRT 삭제 기록 이동이며 Pre/Post·종속·전파/선택·로그·파편/소리/낙하는 명시적 대체다. 슬롯 바이트·전체 풀/네 해시/spot/두 삭제 기록·목록/카운터·선택/깊이를 대조했다. 각 PE 내부 Destroy 712/Unpop 440/Release 600/BridgePre/Post 각각 48/Begin 96/Next 704, assert 0·정상 반환/ESP/x87 확인. 별도 FS 감사는 추가하지 않았다.
- [x] **최종 검증/보호:** x64 Release 경고/오류 0·콘솔 CTest 실행 파일 1개 안의 내부 **169개·실패 0**(68.81초), 누적 제한 x86 **85,584개**. 재삭제·훅 완료 누락·권한/다른 풀·손상 종속을 검사했다. 최초 C++ 합성 입력의 판본 타입 개수/비종속 next를 수정했으며 독립 기계어 기대값은 보존했다. 새/기존 탐색·삭제 훅·붕괴 감사 통과. 로그 `extracted/destroy/build.log`·`ctest.log`. 여섯 원본 디렉터리 **2,782개 파일 SHA/목록이 시작과 동일**하다. 원본/복사본·업데이터/설치·클론 창 실행 없음.
- [x] **문서:** [공통 삭제 복원](docs/exe/cpp-destroy-reconstruction.md), cpppj README·빌드/로드맵/실제 플레이 계획·앞선 훅/탐색 문서·한국어/영어 루트 README에 최신 범위와 파편 함수 정정을 반영했다. dotnetpj 인수인계는 보존했다.
- [x] **후속 일부 완료(위 최신 장부 절):** 선택/abstract 배치 복구·공급/작업장/소유자 목록·현재/누적 통계·unitLost 사건/좌표·비용을 실제 공통 삭제에 연결했다. `0044c2f0`/CD `004626d0`는 삭제 보상/SP 경계로 정정했다. Graph 분할/Free·주변 Add·실제 보상/SP·AI·종속 파생 메서드·파편/낙하/소리·raw GameWorld는 최신 남은 작업을 따른다.
- [ ] **창 검증 인계:** 호스트 **`DESKTOP-HJOW`의 기존 창 제한을 유지**한다. 다른 허용 PC에서 raw 월드 연결 후 다리 붕괴→Graph/공간/반납→파편/소리/낙하와 TEST01/1-1 생성·삭제·재진입을 검사한다. 원본 실행은 해당 AGENTS.md/사용자 지시를 확인한다.

---

## 2026-10-07 ✅ 완료: cpppj 일반 공간 탐색·다리 삭제 훅 연결 (DESKTOP-HJOW, 게임/창 실행 없음)

- [x] **지침/기준:** 현재 AGENTS.md·LEFT_JOBS.md와 앞선 복원 근거를 읽고 다음 항목인 일반 탐색을 진행했다. 기준은 10.78이며 CD/추가 10.37은 비교 자료다. 이전 작업의 일회성 창 허용은 이번에 적용하지 않았다. AGENTS.md·C#·LEFT_JOBS.dotnetpj.md를 변경하지 않았고 커밋하지 않았다.
- [x] **읽기 전용 디컴파일:** 패치 Begin/Next·버킷 범위·타입/발자국·CRT·기본 true 필터 8개, CD/10.37 각 6개를 `extracted/finder/<판본>/`에 새로 내보냈다. 세 완료 로그를 확인했고 프로젝트 변경을 저장하지 않았다. 이전 삭제/붕괴 내보내기·도구·fixture는 보존했다.
- [x] **C++ 일반 탐색:** `RawSquidFinder`가 네 단계의 y/x·raw +4 WORD next 순회, 오른쪽/아래 -1·버킷 끝 +1, flag 1/4, 소수 좌표 절삭/발자국/확장 높이, buried만 제외하는 기본 필터를 복원했다. 반환 전에 next를 저장하며 뒤 후보의 extra/좌표/타입과 미방문 버킷 머리는 그 시점에 다시 읽는다. 원본과 달리 손상 체인/범위/타입은 예외로 거부한다. 다른 파생 가상 필터는 후속이다.
- [x] **삭제 훅 연결:** `MakeBridgeLifecycleHooks`로 실제 raw 일반 Begin/Next를 preDestroy/postDestroy에 연결했다. 일반 탐색 결과 목록을 합성 콜백으로 대신하지 않고 두 버킷 등록 순서·앞선 링크 삭제/다음 walker 타입 변경을 대조했다. 기본 삭제·통지·소리·낙하 효과는 여전히 호출자 계약이며 raw GUI GameWorld는 미연결이다.
- [x] **기계어/콘솔:** 세 실제 PE×x87 53/64비트의 새 **1,776개**(Find 1,752/Pre 12/Post 12). Begin/Next·버킷·발자국·CRT·기본 필터는 원본 명령이다. 커서 15 DWORD와 마지막 0 반환 전체를 비교했다. 원본 assert 0·정상 반환/스택/x87/FS 복구, 일반 탐색 쓰기는 finder/스택/FS만 허용하고 풀/타입/해시는 읽기 전용이다. 합성 버킷 등록 입력과 destroy/fall/base/소리/통지 효과 대체 경계를 [독립 기록](cpppj/recovery-rawfinder-evidence.json)에 남겼다. 기존 삭제/붕괴 SHA 감사도 통과했다.
- [x] **최종 검증/보호:** x64 Release 경고/오류 0·CTest 실행 파일 1개 안의 내부 **163개·실패 0**(56.38초), 누적 제한 x86 **83,592개**. 풀/네 해시 배열 불변·필터 변경 시 next 유지·뒤 버킷 변경·손상 보호를 검사했다. 로그는 `extracted/finder/build.log`·`ctest.log`. 여섯 원본 디렉터리 **2,782개 파일 SHA/목록이 시작과 동일**하다. 게임/복사본·업데이터/설치 도구·클론 창 실행 없음.
- [x] **문서:** [일반 탐색 복원 문서](docs/exe/cpp-rawfinder-reconstruction.md), cpppj README·빌드/로드맵/실제 플레이 계획·이전 삭제/해시 근거와 한국어/영어 루트 README의 cpppj 상태를 갱신했다. 과거 검사 수와 창 허용은 당시 기록으로 구별했고 dotnetpj 인수인계는 보존했다.
- [x] **후속 일부 완료(위 최신 절):** 공통 destroy의 순서·종속 체인·실제 Unpop/SID 반납과 다리 훅 내부의 링크 삭제를 연결했다. `00460600`의 의미는 Flyingshrapnel 파편 생성으로 정정했다.
- [ ] **남은 공통 삭제:** pre/post의 Graph·보상/SP·AI(목록/통계/비용은 위 후속 완료)·종속 파생 메서드·파편/낙하/소리·이벤트 실행/끝 칸 변환·raw GameWorld, 다리 배치/Construction·SID 소진/form/process·건설/경제/전투/AI/승패·V12 비교 도구 확장은 남았다.
- [ ] **창 검증 인계:** 호스트 **`DESKTOP-HJOW`에서는 기존 창 제한을 유지**하며 후속에 명시적인 새로운 허용이 있는지 확인한다. 다른 허용 PC에서 raw 월드 연결 후 다리 붕괴/낙하/소리·TEST01/1-1 생성/삭제/재진입을 확인한다. 이번 결과는 실제 삭제/낙하/게임 플레이 완료의 증명이 아니다.

---

## 2026-10-07 ✅ 완료: cpppj 다리 삭제 전후 훅·지연 낙하 좌표 (당시 작업의 클론 창 허용)

- [x] **현재 지침/다른 AI 작업 확인:** AGENTS.md와 최신 LEFT_JOBS.md를 다시 읽었다. 다른 AI가 추가한 다리 붕괴 스캔/수명 감소·V12 비교를 보존하고 그 다음 경로를 복원했다. 기준은 계속 10.78이다. 인수인계 분리 지침을 유지하며 AGENTS.md/C#/LEFT_JOBS.dotnetpj.md는 변경하지 않았다. 커밋하지 않았다.
- [x] **읽기 전용 디컴파일:** 새 `extracted/bridgeeffects/<판본>/`에 패치 12개·CD/추가 10.37 각 7개를 내보냈다. pre/postDestroy·연결 helper·낙하 wrapper/포장·주변/칸 위 좌표·타입 genus/CRT를 포함하며 세 완료 로그를 확인했다. 프로젝트 변경은 저장하지 않았다.
- [x] **C++:** 새 `RawBridgeLifecycle`이 +12/+8 참조의 경계/첫 참조 extra/두 dead 판단, ±1 주변 탐색→특수 타입 destroy(0)→공통 preDestroy, 제거 통지→원본 좌표 `bridgeFall.wav`→칸 위 walker→공통 postDestroy를 연결한다. 순서 있는 begin/next·emit 콜백의 변경 뒤 다음 raw 필드를 다시 읽는다. 기본 삭제/가상 낙하/일반 탐색/소리/통지의 내부 구현은 호출자 계약이다.
- [x] **디컴파일 누락 교정:** preDestroy의 좌표는 `trunc(float좌표 + 0.9999899864196777f)`이며 상수 `3f7fff58`은 표면 조회의 0.9999와 다르다. C 디컴파일에 없는 fadd를 실제 명령에서 확인했다. 최초 무보정 C++의 범위 비교 504건 실패를 고치고 보정 경계 위/아래 입력을 추가했다. postDestroy는 무보정 절삭이다. 중간 덧셈은 float로 좁히지 않는다.
- [x] **낙하 payload:** `0x2692`의 좌표 `(ftol(x)&255)|(ftol(y)<<8)`를 signed DWORD 수치→float로 바꾼다. bit_cast로 float 비트 재해석하는 경로가 아니다. wrapper/직접 함수도 같은 결과다. 이벤트 프로세스의 실제 생성/실행/취소·끝 칸 변환은 복원 완료로 세지 않는다.
- [x] **기계어/콘솔:** 세 실제 PE×x87 53/64비트의 새 **6,170개**(Link 882/Pre 2,520/Post 1,800/Delay 726/Pack 242). patch helper/직접 Pack만 패치판에 존재하며 CD는 인라인 경로다. 실제 패치 내부 Link 1,410/Pre 840/Post 600/Delay 242/Pack 484, CD/10.37 각각 Pre 840/Post 600/Delay 242는 fixture 상위 수에 더하지 않는다. 원본 assert/OS 0·정상 반환/스택/x87/FS 복구, 입력 슬롯 전체 및 효과 순서 대조. 일반 탐색 결과·destroy/fall/base/소리/통지/할당/등록은 대체했다. 기존 붕괴 기록 SHA 감사 유지. 최종 Release 경고/오류 0·CTest 내부 **157개·실패 0**(51.89초), 누적 제한 x86 **81,816개**.
- [x] **이번 호스트의 창 검증:** 사용자 명시적 허용으로 `DESKTOP-HJOW`에서 `cpp_world_smoke.py`를 실행했다. 캠페인 1-1 진입/선택·1.8칸/초 이동·일시정지·우클릭/닫기·허공 거부·카메라/Home·복귀/재진입 15개와 TEST01 브리핑/SP 50,000/선택·이동·도착 5개, 총 **20개 상태** 통과. 초기 자료 6개·393,216 마스크 바이트·2,593 객체를 대조하고 선택 화면을 확인했다. 새 raw 다리 훅의 게임 플레이 검증은 아니다. 보고서/이미지는 `extracted/bridgeeffects/world-smoke-report.json`·`selected.bmp`·`TEST01.bmp`다. 클론 창 종료, 원본/복사본 게임·업데이터/설치 도구 실행 없음.
- [x] **원본 보호/문서:** 여섯 원본 디렉터리 **2,782개 파일 SHA/목록이 시작과 동일**하고 허용된 설정도 원래 바이트/존재 상태로 복구했다. 새 [복원 문서](docs/exe/cpp-bridgeeffects-reconstruction.md)와 cpppj README·빌드/계획·기존 붕괴 후속을 갱신했다. 별도 dotnetpj 인수인계는 보존했다.
- [x] **일반 탐색 후속 완료:** 다음 작업에서 실제 raw 일반 Begin/Next·4단계 해시/next·기본 필터를 복원하고 삭제 훅에 연결했다. [후속 기록](docs/exe/cpp-rawfinder-reconstruction.md).
- [ ] **남은 작업:** 기본 Squid destroy/pre/postDestroy의 공통 삭제·Graph 분할·Unpop·참조/의존 객체·SID 반납과 `00460600` 제거 통지 목록, walker vtable +200/칸 위 carrier 처리, `0x2692` 프로세스 실행/취소·`004215d0` 끝 칸 변환 뒤 raw GameWorld에 연결한다. 이후 다리 배치·소유자/Construction·건설/경제/전투/AI/승패. V12 비교 도구 확장과 이전 SID 소진/form/process 등도 유지한다.
- [ ] **후속 실행 범위:** 이번 창 허용은 **이번 작업에만** 적용한다. 다음 작업은 호스트 `DESKTOP-HJOW`의 기존 제한과 최신 사용자 지시를 확인한다. raw 월드 연결 후 다리 붕괴 화면·낙하·소리 및 TEST01/1-1 생성/삭제/재진입을 검사해야 한다. cpppj/디컴파일 인수인계는 LEFT_JOBS.md, 별도 dotnetpj 작업은 LEFT_JOBS.dotnetpj.md를 쓴다.

---

## 2026-10-07 ✅ 완료: cpppj 다리 붕괴 스캔·한 칸 처리·수명 감소 전체와 V12 구조 확인 (호스트 `DESKTOP-HJOW`, 게임/창 실행 없음)

- [x] **요청/보호:** "디컴파일 소스 분석 및 cpppj 더 진행". AGENTS.md/LEFT_JOBS.md를 먼저 읽었다. 원본/복사본 게임·업데이터·클론 창·GUI 스모크·`start_session`/`--live`를 실행하지 않았다. AGENTS.md/C#/원본 변경 없음. 보호된 여섯 디렉터리 **2,782개 파일 SHA/목록이 시작 스냅샷과 동일**하다. 커밋하지 않았다.
- [x] **디컴파일:** 읽기 전용 Ghidra `ExportCreation.java`로 새 `extracted/bridgedecay/<판본>/`에 패치 **51개**, CD/추가 10.37 각 **35개** 함수를 내보냈다(완료 로그 확인, 프로젝트 변경 폐기). CD/10.37은 범위표가 같다.
- [x] **분석으로 새로 확정한 것:** (1) 스캔 `00422bc0`의 프레임당 개수는 디컴파일에 없는 x87 식 `trunc(delta / 10.0f × (끝 − 시작))`이다(14ms에서 11개). 남은 시간이 0 이하인 프레임에 남은 번호를 한꺼번에 처리하고 커서를 되돌리며 다음 시각 = 그 프레임 시각 + 10이다. (2) 범위는 서버 SID 영역 + 예측 머리(패치 15000..23001, CD 6000..14000)다. (3) 수명 0에서 제거를 가르는 `00540bc0`은 "전투 모드"가 아니라 **서버 플래그**다(`ReduceLife` 인자 이름 정정, 값/기대값 불변). (4) 다리 vtable `005034c8`(CD `00501ab0`)의 +0x10은 destroy 재정의 `004220f0`이며 큰 그래프(표면 수 ≥ 5)의 단단한 다리는 destroy되지 않는다. (5) CD판은 한 칸 처리·열린 방향·금 간 프레임 전환이 스캔/수명 함수에 인라인돼 있다. (6) 그래프 254인 칸은 한 칸 처리가 NULL 레코드를 읽는다(스캔에 try/catch가 있다. 실행 검증 없음).
- [x] **C++:** `Bridge::CrackedFrame`/`NormalFrame`/`Weaken`/`Restore`/`DestroyProceeds`/`ApplyLife`/`DecayCell`과 `BridgeDecayScan::Reset`/`Advance`/`Eligible`, `SurfaceFinder::Map`을 추가했다. `BridgeLifeChange`에 금·이동체 확인 조건을 넣었다. 실제 삭제·화면 갱신·소리·이동체 확인은 사건(`BridgeDecayEvent`)으로 돌려준다. 접합 고리·그래프 254·수명 7 초과 결과는 상태를 바꾸기 전에 구분/거부한다(원본에 없는 보호).
- [x] **기계어/콘솔:** 세 실제 PE×x87 53/64비트. **x86 입력 14,524개**(한 칸 처리 4,241·수명 3,264·destroy 재정의 1,152·프레임 전환 1,914·스캔 초기화/프레임 3,953), 원본 assert 도달 0. 패치판은 직접 호출과 스캔 경유 결과가 같고, CD/10.37은 스캔 범위를 번호 하나로 좁혀 실제 스캔 함수로 실행한 결과가 패치판과 같다. 대체한 진입점은 할당/해제·소리·칸 위 이동체 탐색·기본 destroy(dead 표시만)·공통 화면 갱신 다섯 가지다. CD판 delta 0.3 한 프레임은 64비트 정밀도에서 개수가 달라(240/239) 제외·기록했다. 최종 Release 경고/오류 0·CTest 내부 **152개·실패 0**(64초). 목표 수명을 일부러 틀리게 바꾼 확인 실행에서 실패 1,004건이 나왔고 되돌린 뒤 0건이다. `--verify` 통과.
- [x] **10.82 V12 후속(정적):** V10에서 검토한 Graph 함수 다섯 개의 V12 주소를 찾았다(모두 +0x500). **V12는 SID 슬롯 75바이트·graph +40/DWORD·타입 구조체 508바이트로 V10(77·+42·504)과도 다르다.** 그래프 한도 50,000은 같다. DevLog V11/V12 항목에 대응하는 새 문자열(요새 백업, Reset to Rank 1, Shift Z 템플·`defaultTemple`, `sendMoney1/2`, `disableFenceCheck`)을 확인했다. [표·근거](docs/exe/cpp-reference-versions.md).
- [x] **문서:** 새 [cpp-bridgedecay-reconstruction.md](docs/exe/cpp-bridgedecay-reconstruction.md), `bridge-pieces.md` 8.1(반올림→절삭·주기 끝 일괄 처리·서버 플래그 정정), `cpp-bridge-reconstruction.md`·`cpp-reference-versions.md`·`cpp-playable-plan.md`·`cpp-roadmap.md`·`cpp-build.md`·`cpppj/README.md`, `.gitattributes`(새 도구/기록 LF 고정), `LEFT_JOBS.dotnetpj.md`(스캔 항목 갱신).
- [x] **후속 일부 완료(위 최신 절):** 다리 pre/postDestroy의 참조 판단·좌표/효과 순서와 지연 낙하 payload를 복원했다. 실제 일반 탐색·삭제/낙하/소리·이벤트 실행·raw 월드는 남았다.
- [ ] **다음 작업:** (1) 붕괴 사건을 raw GameWorld에서 실제 효과로 소비 — destroy → 삭제 준비 분할·Unpop·SID 반납, 다리 postDestroy `00422300`(`bridgeFall.wav`·walker 낙하), preDestroy `004221b0`. (2) 칸 위 이동체 처리(`004202f0` → `00427de0`/`00426120`/`00427e00`)와 지연 낙하 `00421f90`/`00421530`. (3) 다리 배치 `0049b510`·소유자 전파 `00421240`/`004213b0`·Construction `00442c80`(다리 조각의 SID 할당 인자 확인 포함). (4) 이전 절의 SID 소진·form/process·건물 부착 등. (5) V12: `tools/compare_reference_versions.py`에 V12 추가, 편집기 128개 한도·새 기능의 코드 경로는 미확인.
- [ ] **다른 PC 창 인수인계:** raw 월드 연결 후 다리가 금 가고 무너지는 화면·소리와 TEST01/1-1 생성·표시·선택·이동·해제·재진입을 `DESKTOP-HJOW` 이외의 PC에서 검사한다. 원본 비교 실행은 AGENTS.md/사용자 지시를 확인한다.

---

## 2026-10-07 ✅ 완료: `original1082v12/` (10.82 V12) 식별·전체 디컴파일 (호스트 `DESKTOP-HJOW`, 게임 실행 없음)

- [x] **요청/보호:** "original1082v12 쪽도 디컴파일 진행". AGENTS.md/LEFT_JOBS.md를 먼저 읽었다. 원본/복사본·업데이터·클론 창을 실행하지 않았다. AGENTS.md/C#/원본 변경 없음. 보호된 `originals`/`originalCD`/`original1037`/`originalPatches`/`original1082`/`original1082v12`의 **2,782개 파일 SHA/목록이 시작 스냅샷과 동일**하다.
- [x] **식별:** `DevLog.txt` 맨 위가 `10.82 V12`다. 기존 `original1082`는 DevLog 상 **V10**이고 v12에는 V11(단축키 Shift+Q/W/A/S/X/Z, 오래된 `.tarc` 읽기 수정)·V12(멀티 요새 메뉴, `\d\backups\` 백업, Whirligig 복귀, Send Money 옵션, Fence check 기본 활성, 편집기 요새 128개)가 추가돼 있다. `netstorm.ver`는 두 폴더 모두 **10.64**로 판본 근거가 아니다. v12에는 `README.txt`·`disclaimer.txt`가 있고 `file.list.txt`/`auto.list.txt`/`remote*.txt`/`trace.txt`/`libbz2.dll`/`d/options.cfg` 등 업데이터 부산물과 일부 음악·gif는 없다.
- [x] **PE:** 본체 `netstorm.game` 2,333,696바이트(SHA-256 `4c4a52ac…`, 10.82 V10은 `a8c15ef1…`, 2,330,112바이트), 업데이터 `Netstorm.exe` 603,136바이트(`4e3074c5…`, V10은 `5b1c7a84…`, 크기 동일·내용 다름). 두 PE 모두 ImageBase 0x400000이고 본체 .text는 1,628,672→1,631,744바이트, EntryPoint는 `0x135840`→`0x136367`로 바뀌었다.
- [x] **TARC:** `netstorm.tarc` 291개 항목 중 추가 0·삭제 0·**변경 18개**(`help.english`, `tell.english`, rain/sun/thunder/wind 계열 `.type` 16개). 내용 SHA-256 기준(`tools/taff.py`)이다.
- [x] **디컴파일:** `tools/ghidra/run_decomp.ps1`/`run_script.ps1`에 `-Edition original1082v12`/`original1082v12-launcher`를 추가했다(기존 10.82 프로젝트/출력과 분리). 읽기 전용 Ghidra 12.1.4로 **본체 10,141개·실패 0**(5분 20초)과 **업데이터 2,618개·실패 0**(약 1분)을 `extracted/original1082v12/decomp/`·`launcher/decomp/`에 내보냈다(Git 제외). 기존 10.82는 10,130/2,618개라 본체만 11개 늘었고 업데이터 함수 수는 같다. GIF/PNG/MinGW 분석 경고는 남아 있으며 함수 의미 복원 완료를 뜻하지 않는다.
- [x] **정적 비교(대략):** 주소·전역·지역 이름을 정규화한 함수 본문 SHA-1이 10.82 V10과 같은 것은 약 **6,306개**, 다른 것은 약 3,830개다. 주소가 밀리면서 생기는 차이가 섞여 있어 **실제 변경 함수 수의 상한**일 뿐이다. 변경이 큰 후보는 `FUN_004bbd80`, `FUN_00509820`, `FUN_00500750`, `FUN_004db150`, `FUN_0055aa9a`, `FUN_0052a4d3`, `FUN_00455280`, `FUN_005298a8`이다. 개별 의미는 확인하지 않았다.
- [x] **후속 일부 완료(위 최신 절):** (1) V12의 Graph 함수 다섯 개 주소를 찾았고 **SID 폭 75·graph +40·타입 508바이트로 V10과 다름**을 확인했다(한도 50,000은 유지). (2) DevLog V11/V12 항목에 대응하는 새 문자열을 확인했다.
- [ ] **다음 작업:** `tools/compare_reference_versions.py`에 v12를 추가(현재는 10.82 V10만 대상). 편집기 128개 한도·Whirligig 복귀 등 문자열로 확인되지 않는 항목의 코드 경로. cpppj 1차 기준은 계속 10.78이며 v12는 교차 확인 자료다.

---

## 2026-10-07 ✅ 완료: cpppj 상위 Add·Detach·Pop의 자동 Graph 소진 복구

- [x] **추가 지침/보호:** AGENTS.md/LEFT_JOBS를 먼저 읽고 사용자가 추가한 인수인계 분리 지침을 확인했다. 이번 cpppj/디컴파일 결과는 이 문서에 기록하며 `LEFT_JOBS.dotnetpj.md`와 C#은 변경하지 않았다. 원본/복사본·업데이터/설치/SFX/배치·클론 창·GUI 스모크·`start_session`/`--live`를 실행하지 않았다. AGENTS.md 변경 없음. 보호된 다섯 디렉터리 **총 2,445개 파일 SHA/목록이 시작 스냅샷과 동일**하다.
- [x] **디컴파일:** 새 `extracted/graphrecovery/<판본>/`에 Add/Detach/Allocate/Rebuild/Pop/postPop을 각 **6개**씩 읽기 전용 Ghidra로 다시 내보냈다. 세 완료 로그를 확인했으며 10.78을 1차 기준으로 유지한다. CD/추가 10.37은 같은 코드 배치의 비교 자료다.
- [x] **C++ 연결:** `GraphRecovery::FullPool`을 선택한 전체 자산 풀에서 내부 할당 소진→전역 재구성→새 번호 확보→원래 flood/감소/해제를 연결했다. Add/Detach가 보관한 레코드/객체 참조는 같은 복사본에서 유지한다. 기본 `Reject` 정책과 기존 지역 소진 보호는 보존한다. 같은 RawGraph를 연결한 Pop/공통 postPop도 이 정책을 공유한다.
- [x] **원본 순서/보호:** 다른 void/contained 표면도 전체 순회에 포함하며 Pop의 새 좌표/void 해제/hash/spot을 사전 예측한다. 다른 표면의 손상 프레임/내부 타입은 공간·풀·표·스택·비용·깊이 쓰기 전에 거부한다. 재구성 이후 크기 0의 inUse를 임의로 지우지 않는다. Detach는 최초 조회 레코드와 원래 연결 목록을 유지하며 현재 번호를 다시 읽고 최초 레코드에 1/특수 9 감소·해제를 수행한다. 전역 재구성으로 dead 원천의 graph도 변경될 수 있다.
- [x] **기계어/콘솔:** 세 PE×두 x87 정밀도×8입력×네 상위 경로 **192회**(Add/PostPop/Pop/Detach 각각 48), 각 호출에서 실제 **32768슬롯 전체 재구성 1회**를 확인했다. 각 PE 내부 Rebuild 64/Allocate 308/Flood 244/Add 48/PostPop 32/Pop 16/Detach 16과 준비 Reset 2/Create 14는 상위 수에 더하지 않는다. 7개 슬롯/255개 레코드/dirty 직접 비교·전체 풀/hash/spot/스택/통계 Adler-32. 대체 함수/OS 0·최종 assert 0. 이전 특수 감소 타입의 생성 준비 상태를 초기화했고 원본 assert 몸체는 실행하지 않았다. 기존 graphrebuild/graphlookup/graphremove SHA·호출 수 유지. 최종 Release 경고/오류 0·CTest 내부 **145개·실패 0**(43.59초). 추가 보호 검사의 genus 0 일반 표면 hash 단계를 실제 x86 출력인 1로 정정했으며 독립 fixture는 유지했다.
- [ ] **다음 작업:** 미복원 form/process 타입을 포함한 실제 전체 풀 계약·`Rebuild(false)` 중첩 소진·재구성 중 재소진·SID 소진의 파생 삭제/서버·클라이언트 통지를 이어 복원한다. 건물 부착/dirty/grid·fencemark/windArcher 특수 위치 조회·파생 삭제/참조/반납 수명·상위 수신 목록 뒤 raw GameWorld·다리 배치/Construction·덱/자원/SP·경제/전투/AI/승패로 연결한다. 자동 복구는 전체 자산 타입(74 이상)·SID 5..32767·정수·비전투 null 큐 계약이며 실제 플레이 전체 복구로 세지 않는다.
- [x] **새 비교 자료:** 작업 중 나타난 미추적 `original1082v12/`는 수정/실행하지 않았다. 이번 구현 기준에 도입하지 않았다. (후속: 10.82 V12로 식별하고 본체/업데이터를 전체 디컴파일했다 — 위 최신 절. 10.78 정적 비교는 아직 미실시.)
- [ ] **다른 PC 창 인수인계:** raw 월드 연결 후 TEST01/1-1 생성·표시·선택·이동·해제·재진입 및 renderer/menu/world·전체화면·오디오 회귀를 `DESKTOP-HJOW` 이외의 PC에서 검사한다. 원본 비교 실행은 해당 AGENTS.md/사용자 지시를 확인한다. cpppj/디컴파일 인수인계는 이 문서, 별도 dotnetpj 작업은 `LEFT_JOBS.dotnetpj.md`를 사용한다.

---

## 2026-10-07 ✅ 완료: cpppj 전역 Graph 재구성·소진 할당

- [x] **조건/보호:** AGENTS.md/최신 인수인계를 읽고 10.78을 기준으로 후속 작업을 진행했다. 원본/복사본 게임·업데이터/설치/SFX/배치·클론 창·GUI 스모크·`start_session`/`--live`를 실행하지 않았다. AGENTS.md/C# 변경은 없다. 보호된 원본/CD/10.37/패치/10.82의 **총 2,445개 파일 SHA와 목록이 시작 스냅샷과 모두 동일**하다.
- [x] **재디컴파일:** 읽기 전용 Ghidra로 `00463110` ↔ `0045bd60` 전역 재구성 및 초기화/할당/iterator를 새 `extracted/graphrebuild/`에 내보냈다. 10.78 **5개**, CD/추가 10.37 각각 **7개**, 완료 로그 확인. SID `004af1d0`의 최대 50개 다리 파생 삭제 루프를 DevLog 10.73/74(줄 1122)와 대조했지만 SID 소진 복구 완료로 세지 않는다.
- [x] **C++ API:** `Graph::Rebuild`/`AllocateWithRecovery`와 `RawGraph::Rebuild`/`Allocate`를 추가했다. 전체 raw 풀의 두 SID 오름차순 순회·free 제외·기준점 내부 제외·무효 번호 수집/전체 초기화·임시 그래프·원본 flood·임시 해제를 복원했다. dead/void/contained/buried는 수집 제외 조건이 아니며, 비표면 graph/state/next/좌표/payload는 보존한다.
- [x] **세부/보호:** 251개 레코드 앞 두 WORD만 초기화하고 0번 예약/reserved low byte 1·254 sentinel·251..253·기존 reserved/4096 DWORD 스택 흔적을 보존한다. 여유 번호 경로는 풀/프레임을 읽지 않는다. 손상·소수 좌표·누락 membership·재소진은 기존 번호/표/스택/공간 쓰기 전에 거부한다. 249개 독립 무리 성공/250개 거부는 C++ 보호 검사이며 해당 원본 재귀 경로는 실행하지 않았다. `Rebuild(false)`는 최초 여유 번호가 있는 입력 범위다.
- [x] **기계어/회귀:** 세 실제 PE×두 x87 정밀도×16입력×세 경로 **288회**(직접 재구성 192·소진 할당 96), 실제 **32768슬롯 전체 순회**, 대체 함수/assert/OS 0. 각 PE의 내부 Rebuild 96/Allocate 406/Flood 278 및 준비 Reset 2/Create 14는 상위 수에 더하지 않는다. 7개 슬롯/255개 레코드/dirty 직접 비교, 전체 풀/네 단계 해시/spot/스택/통계 Adler-32. 새/기존 graphlookup/graphremove SHA/호출 수 확인 통과. 최종 Release 경고/오류 0·CTest 내부 **143개·실패 0**(40.73초). 최초 fixture의 free 슬롯 재입력은 정상 Reset/Create 준비로 수정했고 기대값은 유지했다. 기존 void 원천 직접 Flood 계약도 검사했다.
- [x] **후속 일부 완료:** 전체 자산 풀 계약의 Add/Detach/Pop·공통 postPop 자동 전역 재구성 연결은 위 최신 절에서 완료했다. form/process를 포함한 실제 전체 월드 계약·중첩 소진·SID 소진·건물 부착/dirty/grid·특수 조회·파생 삭제/참조 수명·raw GameWorld는 후속이다.
- [ ] **다른 PC 창 인수인계:** raw 월드 연결 후 TEST01/1-1 생성·표시·선택·이동·해제·재진입과 renderer/menu/world·전체화면·오디오를 `DESKTOP-HJOW` 이외의 PC에서 검사한다. `DESKTOP-HJOW`에서는 원본/복사본·업데이터/설치 도구 실행과 모든 창 검사가 금지된다. 다른 PC 원본 비교 실행도 해당 AGENTS.md/사용자 지시를 확인한다.

---

## 2026-10-07 ✅ 완료: 10.62/10.82·DevLog 정적 비교와 같은 위치 그래프 조회

- [x] **기준/보호:** AGENTS.md/최신 인수인계를 읽었다. 사용자의 후속 결정대로 10.78을 cpppj 1차 목표로 고정하고 다른 판본은 비교 자료로 사용했다. 원본/복사본·설치/SFX/배치/zpatch·업데이터·클론 창을 실행하지 않았고 업데이트 서버에도 접속하지 않았다. AGENTS.md/C# 변경은 없다. 보호된 `originals` 1,394/CD 425/10.37 261/패치 7/10.82 358개, **총 2,445개 파일의 최종 SHA/목록이 시작 스냅샷과 모두 동일**하다.
- [x] **10.62 복원:** `update.bat`의 CD 복사→패치 1/2 적용 순서를 확인했다. 패치 도구 148개 함수·실패 0에서 체크섬/삭제·삽입 형식을 분석해 Python으로 `extracted/original1062/Netstorm.exe`(1,695,744바이트)와 TARC(1,021,546바이트)를 복원했다. CD 기준 체크섬 일치·저장 결과 전체 바이트 재계산·손상 입력 자체 검사·TARC 266개 항목 경계/길이 확인. 공급 실행 도구는 사용하지 않았다.
- [x] **디컴파일:** 복원 10.62 **3,729개**, 10.82 본체 **10,130개**, 10.82 업데이터 **2,618개** 함수·실패 집계 0. `-Edition original1062/original1082/original1082-launcher/patch1062` 출력/프로젝트를 분리했다. 10.82 본체는 `netstorm.game`이고 업데이터는 `_spawnl`로 본체를 시작한다. `netstorm.ver=10.64`와 업데이터 `v10.82+`를 구분한다. MinGW/PNG/GIF/pcode 경고는 남아 전체 의미 복원이 끝난 것으로 세지 않는다.
- [x] **판본/DevLog:** Graph 역할 다섯 개를 각 새 본체에서 읽기 전용으로 다시 내보냈다. 10.82의 SID 77바이트·graph DWORD(+42)·한도 50,000/무효 50,003은 10.78의 50바이트·graph byte(+30)·251/254와 다르므로 도입하지 않았다. Windows-1252 일지의 10.73/74 소진 처리·10.81 다리 재변경·Sun Generator·난이도/옵션 추가를 대조했다. Sun Generator/easy·hard 자료는 후대 요소다. `bridge.type` 자체는 10.78/10.82 동일하지만 코드 동일성으로 확대하지 않는다. 실제 file.list 355개 일치/3개 불일치·auto.list 1개 일치를 기록했다.
- [x] **C++ 후속:** `RawGraph::Detach`가 같은 위치의 정상 다른 표면 SID의 번호를 조회하고, `Graph::DetachAt`이 원천 타입/프레임/genus와 조회 graph를 분리한다. 원천의 graph/state/payload는 바꾸지 않는다. 조회 254/미사용의 조기 반환과 손상 SID/위치/상태/번호·소진/순환의 변경 전 거부를 유지했다. fencemark/windArcher 특수 조회는 후속이다.
- [x] **검증:** 세 실제 PE×두 x87 정밀도의 새 Detach **384회**를 대체 함수/assert/OS 없이 실행했다. 각 PE 내부 Allocate/Flood 216회는 상위 수에 더하지 않는다. 슬롯/dirty 직접 비교, 전체 풀/해시/spot/표/스택/통계 Adler-32. 최종 Release 경고/오류 0·CTest 내부 **139개·실패 0**(37.81초). 새/기존 graphremove SHA·호출 수와 정적 비교/패치 전체 재계산 확인을 통과했다. 이전 독립 도구/fixture/기록은 보존했다.
- [x] **후속 일부 완료:** 전역 Graph 재구성·소진 할당 API는 위 최신 절에서 추가했다. Add/Detach/Pop 자동 전역 재구성·SID 소진 복구·건물 부착/dirty/grid·특수 타입 위치 조회·참조/파생 삭제/반납 수명·form/process·상위 수신 목록·raw GameWorld는 후속이다.
- [ ] **다른 PC 창 인수인계:** raw 월드 연결 후 TEST01/1-1 생성·표시·선택·이동·해제·재진입 및 renderer/menu/world·전체화면·오디오 회귀를 `DESKTOP-HJOW` 이외의 PC에서 검사한다. 호스트 `DESKTOP-HJOW`에서 `--run`·GUI 스모크·원본/복사본/업데이터 실행·`start_session`/`--live`는 실행하지 않는다. 다른 PC의 원본 비교 실행도 해당 AGENTS.md/사용자 지시를 확인한다. 막힌 업데이트 서버 접속을 복원하려 시도하지 않는다.

---

## 2026-10-07 ✅ 완료: cpppj 삭제 준비 분할·일반 다리/섬 Unpop과 10.37 재디컴파일

- [x] **작업 조건/보호:** AGENTS.md/LEFT_JOBS.md와 이전 복원 기록을 읽었다. 사용자 최신 지시대로 원본/복사본 프로세스와 클론 창을 실행하지 않았다. 원본 `originals` 1,394개/CD 425개/10.37 261개 파일이 시작 SHA와 동일하다. AGENTS.md·기존 원본·C# 변경은 없다. 작업 도중 나타난 별도 `originalPatches/` 미추적 자료는 건드리지 않았다.
- [x] **10.37 전체 재디컴파일:** 별도 `extracted/original1037/ghidra/netstorm.gpr`에서 **3,711개 함수·실패 0**을 다시 내보냈다. GIF 분석 경고는 있었지만 분석/내보내기/저장 완료. 전체 C/함수 표 SHA는 기존 기록과 동일하며 새 독립 기록에도 저장했다. CD/새 PE 전체 차이 `0003314c`/VA `00433d4c`의 `75→eb` 하나와 두 `netstorm.ver=10.37`을 확인했다. [비교](docs/exe/original1037-comparison.md).
- [x] **삭제 helper 분석:** `004637b0` ↔ CD/10.37 `0045bfd0`와 일반 finder/기하/Graph/Unpop/표시 등을 읽기 전용 Ghidra로 새 `extracted/graphremove/`에 통합 내보냈다. 패치 **95+2**, CD/10.37 각각 **90+2+1**개, 완료 로그/불연속 몸체 확인·프로젝트 변경 폐기. 과거 SHA 기록/도구/fixture는 수정하지 않았다. [재현 주소 목록](tools/ghidra/graphremove-functions.json).
- [x] **C++ 분할:** `Graph::Detach`/`RawGraph::Detach`·사전 검사를 추가했다. dead 원천·원본 일반 탐색의 네 단계/y/x/next·경계 접촉 XOR·현재 이웃 번호 재판독·일반/rebuild 분할·1/특수 9 감소·reserved WORD/스택 보존을 복원했다. 위치 조회는 0단계 머리가 없으면 자연 반환한다. 동일 위치 다른 SID·잘못된 프레임/체인·소진은 부분 변경 전에 거부한다.
- [x] **일반 Unpop:** 일반 다리/섬의 비전투 null 알림 큐 자연 반환 범위에서 void→spot AND→머리/이전 next→공통 표시 순서를 허용했다. Graph 분할·Unpop·반납은 원본처럼 별도 단계다. 건물 부착·파생 destructor·참조 수명과 전투 큐를 복원한 것은 아니다.
- [x] **해석 정정:** 패치 사각형의 `(작음 != 같음)`도 정수 좌표에서는 `<=`다. 실제 ECX/스택 인자로 세 판본 모두 경계 접촉을 인정함을 확인했다. `GetGridSid`의 전역은 해시 객체 +12/0단계 머리여서 별도 grid가 아니었다. 초기 대조 실패를 기대값 변경 없이 C++ 조회/교차 해석을 수정하여 해결했다. 발자국의 1..255 보정/무효 초기 점 재설정을 포함하고, CD의 0 좌표 후보 `pos.isValid()` assert는 oracle에서 즉시 중단/C++에서 쓰기 전 예외로 거부한다.
- [x] **기계어:** 세 실제 PE×두 x87 정밀도·6시퀀스 **1,158회**(Detach 384/Unpop 768/반납 6), 대체/assert 0. 각 PE의 비전투 통지 86회와 내부 Allocate/Flood(패치 각각 38, CD/10.37 각각 52)는 상위 수에 더하지 않는다. 가장자리 입력은 패치의 0 후보/CD의 최소 1 후보로 구별하며 도달 수를 같은 입력의 규칙 차이로 해석하지 않는다. 전체 7개 raw 슬롯/dirty 직접 비교, 풀/4단계 해시/spot/표/스택/통계 Adler-32. 원본/새 도구/부모 도구/내보내기/전체 10.37/fixture SHA·호출 수 `--verify` 통과. [독립 기록](cpppj/recovery-graphremove-evidence.json).
- [x] **검사/문서:** x64 Release 경고/오류 0·CTest 내부 **137개·실패 0**. 그래프 소진·active 원천·체인 순환·다른 위치 SID의 변경 전 거부, 미사용 그래프가 손상 이웃 탐색 전에 반환하는 순서·정상 마지막 무리 유지/WORD 흔적과 전체 기존 fixture 회귀를 확인했다. README·빌드/로드맵/실제 플레이 계획·개별 복원/신뢰도/비교 문서를 맞췄다. [범위·명령](docs/exe/cpp-graphremove-reconstruction.md).
- [x] **후속 일부 완료:** 같은 위치의 정상 다른 SID 조회는 위 최신 절에서 추가했다. 전역 Graph/SID 소진 복구·건물 부착/dirty/grid·특수 조회·참조/파생 삭제/반납 수명·form/process·상위 수신 목록·raw GameWorld와 실제 플레이는 남았다.
- [ ] **다른 PC GUI 인수인계:** raw 월드 연결 후 TEST01/1-1 생성·표시·선택·이동·해제·재진입과 renderer/menu/world 창 회귀를 검사한다. 현재 PC에서는 `--run`·window/renderer/menu/world 스모크·원본/복사본 실행·analyzeManager `start_session`/`--live`를 실행하지 않는다. 원본 실행 허용은 해당 PC에서 AGENTS.md와 사용자 지시를 확인한다.
- [ ] **과거 검증 재현 조건:** 현재 PC에는 과거 `extracted/regiongraph/` 등 일부 Git 제외 내보내기가 없어 과거 `compare_original1037.py --verify` 전체 통과를 새로 주장하지 않는다. 이번 확인은 새 통합 내보내기와 `decomp_graphremove_oracle.py --verify`로 수행했다. 과거 명령을 재현하려면 해당 문서의 과거 추출물을 먼저 준비한다.

---

## 2026-10-07 ✅ 완료: cpppj 영역 그래프·일반 다리/섬 Pop과 10.37 비교

- [x] **진행/원본 보호:** AGENTS.md/최신 인수인계를 읽고 바로 다음 영역 helper부터 복원했다. AGENTS.md/기존 원본/C# 변경과 원본/복사본 게임 프로세스 실행은 없다. 추가 10.37도 전체 261개 파일이 시작 SHA와 동일하다. 당시 클론 창 허용 기록은 위 최신 창 금지 지시로 대체한다.
- [x] **10.37 전체 디컴파일:** 새 `-Edition original1037` 프로젝트/출력을 기존 것과 분리했다. `extracted/original1037/decomp/netstorm.c`/`functions.tsv`, **3,711개 함수·실패 0**. 내장 GIF 분석 경고가 있었지만 분석/내보내기/저장은 성공했다. [SHA·자료 목록·검토 함수·비교 근거](cpppj/recovery-original1037-evidence.json).
- [x] **구버전 비교:** CD와 새 실행 파일은 1,647,616바이트이며 전체 차이는 `0003314c`/VA `00433d4c`의 `75→eb` 하나다. `InsertCD` 창 생성 구간을 건너뛰는 분기로 해석하며 전체 CD 검사 우회를 주장하지 않는다. 두 `netstorm.ver`는 모두 10.37이다. 배포판 설명 10.72와 실행 파일 표시를 구분하고 기존 `Cd1072` 레이아웃 식별자는 유지한다. 공통 자료 236개와 TARC 동일, 변경 8개·신규 17개 목록을 기록했다. [비교·재현](docs/exe/original1037-comparison.md).
- [x] **영역 재디컴파일:** 읽기 전용 Ghidra로 패치 13+3/CD 14+1/10.37 14+1개 helper를 새 `extracted/regiongraph/`에 내보냈다. 새 PE의 검토 함수 바이트도 CD와 동일하다. 기존 함수 대응 2,496쌍/앵커 24쌍은 유지한다.
- [x] **C++:** `RawGraph::InvalidateRegion`이 일반 finder의 네 단계/y/x/next 순서, 넓은 끝 버킷, 교차 발자국·매몰 제외·dead 포함·내부 spot Surface 무효화/Remove를 복원한다. postPop에서 영역→Add를 같은 복사본으로 계산한다. 일반 다리/섬 Pop은 비전투 null dirty 큐의 자연 반환 범위로 허용했다. 건물 부착/전투 알림 큐는 후속이다.
- [x] **보호:** Pop의 새 좌표·void 해제·해시 체인·genus OR spot을 미리 조회한다. 순환/잘못된 후보·다른 연결·영역 이후 Add 소진을 쓰기 전에 거부하며 번호/표/스택/슬롯/해시/spot/비용/깊이를 보존한다.
- [x] **기계어:** 세 실제 PE×x87 두 정밀도, 6시퀀스 **1,536회**(영역 480/직접 postPop 480/Pop 288/Add 288), 대체 함수/assert 0. 구버전 두 PE는 같은 그래프 코드이며 별도 시대 규칙으로 세지 않는다. Patch의 Pop 충돌 32회도 부분 좌표/spot 쓰기와 void 유지까지 대조했다. 슬롯/dirty 항목 직접 비교, 전체 풀/4단계 해시/spot/표/스택/통계는 Adler-32다. [SHA·범위·공개/내부 호출](cpppj/recovery-regiongraph-evidence.json).
- [x] **컴파일/검사:** 최종 x64 Release 경고/오류 0·CTest 내부 **134개·실패 0**(50.88초). 최초 대조에서 항상 등록 성공을 기대한 테스트 오류를 실제 x86 출력의 void 상태로 정정했다. 새/기존 raw Graph/Graph/postPop/display/pop/unpop/derived/creation/SID SHA/호출 수 확인 통과. 기존 표면 x86 3,450회도 재생성 결과가 기존 기록과 동일하다.
- [x] **범위/후속 기록:** 합성 타입/FrameCode/SHP·client 풀 32768·정수 좌표·비전투 null dirty 큐·AI/배치 선택 없음 범위다. raw GUI 월드는 미연결이므로 이번에는 새 창 검사를 실행하지 않았다. [세부·재현·남은 작업](docs/exe/cpp-regiongraph-reconstruction.md).
- [x] **후속 일부 완료:** 일반 다리/섬 Unpop과 삭제 준비 Graph 분할은 위 최신 절에서 완료했다. 전역 소진 복구·건물 옆면 부착·dirty/grid·참조 수명과 raw GameWorld 연결은 남았다. 이후 다리 배치/Construction·생산 덱/자원/SP 차감·건설/경제/전투/AI/승패로 이어 간다.
- [ ] **raw 월드 연결 후 클론 창:** TEST01/1-1 raw 생성·표시·선택·이동·해제·재진입을 다른 PC에서 검사한다(현재 PC 창 금지). 원본 실행은 AGENTS.md의 해당 단계 확인/예외를 따른다.

---

## 2026-10-06 ✅ 완료: cpppj raw SID 그래프·공통 postPop/지원 Pop 연결

- [x] **진행 파악/원본 분석:** AGENTS.md/최신 LEFT_JOBS를 읽고 기존 읽기 전용 Ghidra의 Graph·postPop·표면/SID 내보내기를 재검토했다. 새 함수 대응 없이 기존 2,496쌍/검토 앵커 24쌍을 유지한다.
- [x] **C++:** `RawGraph.h/.cpp`를 추가했다. 실제 풀의 graph/state/판본별 프레임과 타입 FrameCode, 0단계 기준점 머리·spot에서 매번 스냅샷을 구성한다. 성공한 연산의 graph byte만 반영하며 reserved WORD와 전체 flood 스택의 미사용 흔적도 다음 계산에 보존한다. 표시 SHP 헤더와 프레임 코드를 구별한다.
- [x] **후처리 연결/보호:** 공통 postPop의 surface Add 분기를 선택 연결하고 기존 비용/목록/통계/noGraph 리셋을 유지한다. Pop의 최종 좌표·void 해제·해시/spot 등록을 사전 예측하여 소진/소수 좌표를 공간 쓰기 전에 거부한다. 풀/타입/해시/spot이 다른 연결도 거부한다. 일반 표면/매몰 다리 Pop 범위이며 정상 다리/섬의 영역·부착 효과는 미복원이다.
- [x] **기계어:** 두 판본×x87 53/64비트 4시퀀스 **1,536회**(직접 postPop 640/Pop 128/직접 Add 512/Flood 256), 대체 함수/assert 0. 실제 Reset/Create로 7개 SID를 준비한다. 7개 raw 슬롯·dirty 표는 직접 비교, 전체 풀/해시/spot/그래프 표/스택/세 통계 표는 Adler-32 대조다. 준비 Reset 2/Create 14회(판본당), 패치 비용 인코딩 896회 접두 구간과 내부 도달 수는 공개 호출 수에 더하지 않는다. [근거](cpppj/recovery-rawgraph-evidence.json).
- [x] **검사:** 최종 x64 Release 경고/오류 0·CTest 내부 **132개·실패 0**, 새/기존 graph/postPop/display/pop/unpop의 SHA/호출 수 확인 통과. 최초 빌드 완료 전 검사로 실행 파일이 잠겨 링크가 실패했으며 검사 종료 후 빌드→전체 검사 순서로 해결했다. 프레임/지도/raw membership 재판독·소진/소수·손상 후보·다른 연결·미복원 영역 통지를 검사했다. SOURCE_MAP은 도구로 갱신했고 파일 존재 30/136이며 모듈 전체 완료 수가 아니다.
- [x] **실행 범위/보호:** 새 GUI 검사는 실행하지 않았다. raw Graph가 GUI GameWorld에는 아직 미연결이므로 raw 단위/기계어 검사로 완료했다. 기존 클론 창 허용은 유지한다. 원본/복사본 게임 프로세스 실행·AGENTS.md/기존 원본 자료/C# 수정은 없다. [재현·범위·다음 순서](docs/exe/cpp-rawgraph-reconstruction.md).
- [x] **후속 일부 완료:** 주변 영역 통지 `00462d40` ↔ CD/10.37 `0045c210`과 일반 다리/섬 비전투 Pop은 위 2026-10-07 절에서 완료했다. 건물 부착·일반 다리/섬 Unpop·삭제 분할/소진 복구·raw GameWorld는 최신 남은 작업을 따른다.
- [ ] **raw 월드 연결 후 클론 창:** TEST01/1-1의 raw 생성·표시·선택·이동·해제·재진입을 다른 PC에서 검사한다(현재 PC 창 금지). 원본 실행은 AGENTS.md의 해당 단계 확인/예외를 따른다.

---

## 2026-10-06 ✅ 완료: cpppj 정수 표면 Graph 연결·flood·병합·표 관리

- [x] **진행 파악/디컴파일:** AGENTS.md/최신 LEFT_JOBS·postPop을 읽고 읽기 전용 Ghidra에서 Graph 관련 패치 13개/CD 16개를 내보냈다. `extracted/graph/<판본>/creation.c`, `functions.tsv`와 최종 완료 로그를 확인했다. CD의 추가 SID 조회 helper도 내보내어 실제 실행에 포함했다. 대응 2,496쌍/검토 앵커 24쌍은 유지했다.
- [x] **C++:** 새 `Graph.h/.cpp`를 빌드에 추가했다. 원본 6바이트 레코드/정상 251개/무효 254, 첫 빈 번호·앞 두 WORD 반납/감소·reserved 보존·LIFO flood/중복 push·WORD 감김·최대 이웃 그래프/동률 첫 탐색·패배 무리 병합을 복원했다. 기존 `SurfaceFinder`를 공유하는 정수 스냅샷 계산이며 raw postPop 활성 연결 완료가 아니다.
- [x] **해석 정정:** 그래프 크기는 발자국 칸 수가 아니라 방문 표면 객체마다 1 증가한다. 다중 칸 객체 수 1을 검사하고 `docs/exe/bridge-pieces.md`의 모든 섬을 칸 단위로 센다는 설명을 정정했다. 기존 C#의 칸 근사를 원본 복원 완료로 세지 않는다.
- [x] **기계어:** 두 판본×x87 53/64비트 4시퀀스 **2,560회**(등록/flood/감소/반납/할당 각 512회), 대체 함수/assert 0. 실제 탐색기/기하/가상 필터/Graph를 실행하며 기존 표/스택·로그 조기 반환·소진 전 범위다. 모든 membership graph/state, 전체 표/스택 Adler-32를 대조했다. [근거](cpppj/recovery-graph-evidence.json), 원본/도구/부모 도구/fixture SHA와 호출 수 `--verify` 통과.
- [x] **검사:** x64 Release 경고/오류 0, CTest 내부 **128개·실패 0**. 동률 첫 이웃·병합·다중 칸 수 1·WORD 경계·미사용 WORD·소진/누락/손상 번호의 변경 전 거부를 확인했다. [재현·기대값·제한](docs/exe/cpp-graph-reconstruction.md).
- [x] **실행 범위:** 이번 단계는 GUI에 미연결인 Graph의 단위/기계어 검사로 완료했다. 새 클론 창 회귀는 실행하지 않았다. 원본/복사본 프로세스·AGENTS.md/기존 원본 자료/C# 수정도 없다. 이전 클론 창 허용은 유지한다.
- [x] **후속 일부 완료:** raw SidPool의 graph/state/실제 프레임·0단계 해시/spot 스냅샷과 graph byte 반영·공통 postPop surface 분기는 위 최신 절에서 완료했다. 주변 영역 통지·정상 다리/섬 Pop·삭제 분할/소진 복구·raw GameWorld·생산/건설/경제/전투/AI/승패는 최신 남은 작업을 따른다.
- [ ] **연결 후 클론 창:** TEST01/1-1 raw 생성·표시·선택·이동·해제·재진입을 다른 PC에서 검사한다(현재 PC 창 금지). 원본 실행은 AGENTS.md의 단계 확인/예외를 따른다.

---

## 2026-10-06 ✅ 완료: cpppj 공통 postPop 비용·목록·통계와 noGraph 리셋

- [x] **진행 파악:** AGENTS.md/최신 LEFT_JOBS를 읽고 공통 후처리부터 이어 복원했다. 이번 PC의 창 허용을 유지하며 AGENTS.md/기존 원본 자료/C#은 수정하지 않았다. 원본/복사본 게임 프로세스 실행도 없다.
- [x] **디컴파일:** 읽기 전용 Ghidra에서 패치 10개/CD 13개 공통 postPop·목록/통계·AI/선택 입구·그래프 리셋·비용 관련 함수를 내보내고 완료 로그를 확인했다. 출력은 `extracted/postpop/<판본>/creation.c`, `functions.tsv`다. 대응 2,496쌍·검토 앵커 24쌍은 유지했다.
- [x] **C++:** 새 `SquidPostPop`/상태/용량 고정 목록과 `.type` float cost를 추가하고 `SquidPop`의 선택적인 다섯 번째 인자에 연결했다. 공급 중복 제거/작업장 중복 허용·full 목록 메모리 보존·알림 증가·소유자 0·타입 통계·depth를 보존한다. 공간 변경 전에 미지원 그래프/AI/배치 선택·손상 목록/비용을 거부한다. 그래프 리셋 byte는 패치 +30/CD +28이다.
- [x] **판본 차이:** 패치 비용은 `trunc(cost+type*23)-type*23`, CD는 이전 signed 집계에 float cost를 더한 뒤 절삭한다. 비용 -0.5/기존 집계 0은 패치 -1/CD 0이다. 전역 비용 집계이며 현재 SP 차감 복원이 아니다.
- [x] **기계어:** 두 판본×x87 53/64비트 4시퀀스 **984회**(직접 postPop 768/Pop 144/Unpop 72), 대체 함수/assert 0. 전체 통계/목록 Adler-32, raw 슬롯 전체와 dirty 전 항목을 대조했다. 판본당 내부 postPop 456·firstPop 32회와 목록 함수 도달은 상위 호출에 포함되며 준비 Reset/Create 각 4회·패치 비용 인코딩 192회 접두 구간은 별도다. 원본/도구/부모 도구/fixture SHA와 호출 수 `--verify` 통과. [근거](cpppj/recovery-postpop-evidence.json).
- [x] **검사:** x64 Release 경고/오류 0, CTest 내부 **124개·실패 0**. 비용 문법의 테스트 입력 오류를 수정한 뒤 전체 통과했다. 첫 등록/재등록·중복·미지원 그래프/AI/배치 선택·비유한 비용/손상 목록·다른 풀 연결을 확인했다. 이전 display/pop/unpop/derived/creation/SID의 SHA/호출 수도 재확인했다.
- [x] **클론 창:** `cpp_world_smoke.py`에서 기존 1-1/TEST01 선택·사제 이동·정지·카메라·복귀/재진입 20개 상태, 두 판본 초기 자료 회귀를 통과했다. 설정 복구/원본 자료 해시 동일. 보고서 `extracted/cpp-world-smoke/report.json`, 빌드/창 로그 `extracted/cpp-postpop-*.log`. GUI는 기존 임시 GameWorld이며 새 raw 월드 연결의 증명이 아니다. [범위·재현·후속](docs/exe/cpp-postpop-reconstruction.md).
- [x] **후속 일부 완료:** Graph의 정수 표면 연결/flood/병합/표 관리와 raw postPop/graph byte 연결을 위 후속 절들에서 완료했다. 영역 통지/정상 다리·섬 Pop/삭제 분할/소진 복구·AI/배치 선택·생산 계산·월드 연결은 최신 남은 작업을 따른다.
- [ ] **raw 월드 연결 후 창 검사:** 다른 PC에서 TEST01/1-1의 raw 생성·표시·선택·이동·해제·재진입을 검사한다. 원본 게임 실행은 AGENTS.md의 해당 단계 확인 규칙/예외를 따른다.

---

## 2026-10-06 ✅ 완료: cpppj raw 공통 표시 활성·SHP 추가 헤더·Renderer 연결

- [x] **진행 파악:** AGENTS.md/최신 인수인계를 확인했다. 이번 PC의 창 허용을 유지했으며 원본/복사본 프로세스·AGENTS.md/기존 원본 자료/C# 수정은 없다.
- [x] **디컴파일:** 표시/update88·update8c/main·shadow 경계/dirty 관련 패치 10개·CD 7개를 읽기 전용 Ghidra로 내보냈다. `extracted/display/<판본>/creation.c`, `functions.tsv`와 완료 로그 확인. 프로젝트 변경은 버렸다.
- [x] **C++:** `SquidDisplay`가 별도 SHP 헤더·signed DWORD/byte frame·Q16 크기/hotspot·viewport clamp→선택 확장→그림자 순서와 패치 15/CD 9픽셀 여유를 보존한다. Pop의 firstPop/void 해제 전, Unpop의 공간 제거 뒤 호출한다. `SquidRenderer`가 실제 Renderer 변경 표에 연결하며 o/는 client/를 포함하지 않는다. 기존 nullptr 억제 경로도 보존한다.
- [x] **기계어:** 두 판본×x87 53/64비트 4시퀀스 **1,728회**(표시 400/경계 368/Pop 480/Unpop 480), 대체 함수/assert 0. 모든 dirty 항목 좌표/플래그와 raw 슬롯을 대조했다. 원본/도구/부모 도구/fixture SHA와 호출 수 `--verify` 통과. 준비 Reset/Create 각 4회와 내부 표시 호출은 상위 수치에 더하지 않는다. [근거](cpppj/recovery-display-evidence.json).
- [x] **검사:** x64 Release 경고/오류 0, CTest 내부 **120개·실패 0**. raw→실제 Renderer의 이전/새 위치 부분 Draw/Present 픽셀, 추가 헤더 signed short/float와 순수 VFX 거부, 잘못된 표시 연결의 변경 전 거부를 확인했다. `--inspect-assets`에서 패치 3,783/CD 3,167개, 합계 **6,950개** 실제 SHP 표시 헤더 연결을 확인했다.
- [x] **창 회귀:** `cpp_renderer_smoke.py`, `cpp_world_smoke.py`를 다시 실행했다. 기존 글꼴/커서/화면, 1-1/TEST01 선택·사제 이동·정지·카메라·재진입 회귀 통과. 설정 복구/원본 자료 해시 동일. 이 GUI는 기존 임시 GameWorld 검사이며 raw SID 월드 연결의 증명이 아니다. [재현·결과·제한](docs/exe/cpp-display-reconstruction.md).
- [x] **후속 일부 완료:** 공통 postPop 비용/공급·소유자별 작업장 목록/통계와 noGraph 리셋을 위 최신 절에서 완료했다. 그래프/영역 통지·AI/배치 선택·생산 계산·dirty/grid·원본 월드 연결은 최신 남은 작업을 따른다.
- [ ] **raw 월드 연결 후 실행:** 다른 PC에서 TEST01/1-1의 원본 SID 생성·표시·선택·이동·해제·재진입을 검사한다. 원본 게임 실제 실행 단계는 AGENTS.md와 해당 단계의 사용자 허용을 따른다.

---

## 2026-10-06 ✅ 완료: cpppj raw 일반 Pop·원본 기계어 대조·Release 컴파일·창 회귀

- [x] **진행 파악/허용:** AGENTS.md와 최신 LEFT_JOBS·실제 플레이 계획을 읽었다. 이번 사용자의 "이 PC에서는 창 뜨는 작업 해도 돼"를 적용해 이전 PC의 창 금지 인수인계를 해소했다. 원본/복사본 게임 프로세스는 실행하지 않았고 AGENTS.md/기존 원본 자료/C#은 수정하지 않았다.
- [x] **디컴파일:** 읽기 전용·헤드리스 Ghidra로 Pop 관련 패치 22개+보조 2개/CD 18개 함수를 내보냈다. 이 PC에 없던 SID/기본 생성 내보내기도 재생성했다. `extracted/pop/<판본>/creation.c`, `functions.tsv`, `helpers-originals/`. 완료 로그와 내보내기 수를 확인했고 프로젝트 변경은 버렸다. 기존 대응 2,496쌍·검토 앵커 24쌍은 유지했다.
- [x] **C++:** `SquidPop`은 같은 raw SidPool/SquidHash/spot의 좌표·화면 캐시→spot OR→현재 SHP 크기의 level/next/머리→최초 등록 extra→void 해제를 연결한다. 패치의 overlap 부분 변경·CD의 충돌 후 계속 등록·재등록을 보존한다. PE 표 151행 중 공통 firstPop/postPop·표시 두 경로를 확인한 패치 26개/CD 16개 vtable을 지원한다. 일반/매몰 경로이며 미복원 영역·건물 부착·파생 후처리는 변경 전에 거부한다.
- [x] **해석 정정:** CD 디컴파일의 `(int)param_2 < 1`은 실제로 float 비트값의 signed 비교다. 좌표 절삭이라고 해석한 변경은 x86/기존 fixture에서 실패해 되돌렸다. 양 판본의 0.25 좌표 보존을 추가 검사하고 SquidSpatial 주석에 근거를 남겼다.
- [x] **기계어:** 8시퀀스·**4,600회**(Create 752/Pop 1,712/Unpop 1,384/Release 752), 대체 함수/assert 도달 0. 풀 32,768/65,535·x87 두 정밀도·네 단계·실제 Pop으로 만든 3객체 체인의 중간/머리/꼬리·소수/보정 좌표·부분 충돌·SID 65,534·재등록을 포함한다. 실제 firstPop 패치 208/CD 216, 성공 postPop 패치 688/CD 696회는 상위 Pop 호출 수 안에 포함된다. Reset 8회/vtable 151행은 별도다. raw 슬롯 전체와 풀/해시/spot Adler-32를 대조한다. [고정 근거](cpppj/recovery-pop-evidence.json).
- [x] **컴파일/콘솔:** Release 빌드 경고/오류 0, CTest 한 실행 파일 내부 **116개 검사 통과**. 새 Pop과 기존 Unpop/derived/creation/SID의 원본·도구·fixture SHA-256/호출 수 `--verify`를 확인했다. 제한 x86 누적은 **50,756행**이며 완성도가 아니다.
- [x] **이번 PC의 클론 GUI:** `cpp_window_smoke.py`, `cpp_renderer_smoke.py`, `cpp_menu_smoke.py`, `cpp_world_smoke.py` 통과. 영역 488요새/2,607줄, 타입 화면 1,266,432픽셀, 18글꼴/4,608글리프, 메뉴 54상태와 창 해상도/옵션 재실행, 1-1/TEST01 선택·사제 이동·정지·카메라·복귀/재진입을 확인했다. 새 모듈 빌드 후 월드 스모크도 재검사했다. 허용된 설정 두 파일을 복구했고 원본 자료 해시는 동일하다.
- **제한:** 새 raw Pop은 **표시·공통 postPop 효과 억제/비전투·빈 grid/dirty** 입력이다. 합성 타입/SHP/기존 파생 vtable payload이며 새 oracle에서 파생 ctor/월드 초기화를 실행하지 않았다. 고위 SID Claim은 목록/Allocate 복원이 아니다. GUI는 기존 임시 GameWorld 회귀이며 새 raw SID 연결의 실행 증명이 아니다. [근거·재현·제한](docs/exe/cpp-pop-reconstruction.md).
- [x] **후속 일부 완료:** raw 공통 표시 활성의 경계/선택/그림자 dirty 갱신과 실제 Renderer 연결을 위 최신 절에서 완료했다. postPop 영역/소유자/생산·grid/표면 부착·파생 표시/공간 효과·삭제/참조·SID 상위 수명·GameWorld는 최신 남은 작업을 따른다.
- [ ] **raw 월드 연결 후 실행:** 현재 허용된 이 PC 또는 다음 PC에서 TEST01/1-1의 원본 SID 생성·표시·선택·이동·해제·재진입을 확인한다. 원본 게임을 실제 실행하는 단계는 AGENTS.md와 그 단계의 사용자 허용을 따른다.

---

## 2026-10-06 ✅ 완료: cpppj raw 일반 공간 해제·non-void Take·firstPop 플래그 (당시 창 검증 제외)

- [x] **제한:** AGENTS.md·최신 LEFT_JOBS를 확인하고 후속 복원을 진행했다. 원본/복사본 게임 프로세스·클론 GUI·창 스모크 실행 없음. 원본/AGENTS.md/C# 수정·커밋 없음.
- [x] **디컴파일:** Unpop·공통 firstPop·표시 갱신·Renderer 조기 반환·섬 번호/좌표 보조 등 패치 22개/CD 17개 함수를 읽기 전용·헤드리스 Ghidra로 내보냈다. `extracted/lifecycle/<판본>/creation.c`, `functions.tsv`. 정의가 부족한 함수는 메모리에서 정의하고 변경을 버렸으며 완료 로그/행 수를 확인했다. 기존 대응 2,496쌍·검토 앵커 24쌍은 그대로다.
- [x] **C++:** 새 `SquidUnpop`이 기존 raw SidPool·4단계 SquidHash·spot의 void→genus 해제→체인 머리/이전 next를 갱신한다. 일반 자산/매몰 객체의 표시 비활성 경로이며 dead는 허용, free/void는 조기 반환한다. 제거 대상 next/저장 level/좌표/HP/섬 번호를 보존한다. 가상 함수 주소 표 151행으로 미복원 override를 거부한다. `SquidFactory`에 같은 풀의 선택적 Unpop 어댑터를 연결하여 non-void Take→기존 ctor/postTake를 지원하고 공통 `FirstPopFlags`를 추가했다.
- [x] **판본/보호 차이:** CD판의 체인 미발견에도 void 전환하는 경로를 보존했다. 패치 미발견·미복원 섬/다리/건물 부착 효과·unknown vtable·contained/form·잘못된 좌표/발자국·순환 검색 경로는 변경 전에 거부한다. 패치 원본 assert 이전의 부분 변경/오류 UI는 재현하지 않는다.
- [x] **필드 정정:** 기존 생성자 문서의 `+8 frame word`는 **섬 번호 word**다. 실제 프레임은 패치 +36 DWORD/CD +34 byte이며 `SetIsland 004acd20`↔CD `004aca80`과 표시 함수에서 확인했다. 기존 ctor 쓰기와 fixture는 올바른 오프셋/폭/값이므로 보존하고 현재 주석/문서만 정정했다. 고정 해시의 옛 감사 도구 frame 라벨은 역사적 표현이다.
- [x] **기계어:** 18시퀀스·**3,618회**(Create 34/firstPop 896/Take 768/Unpop 1,536/Release 384). 양 판본·풀 32,768/65,535·서버/클라이언트·x87 53/64비트·SID 50,000/65,534·네 단계/머리/중간/꼬리·소수 좌표·다른 저장 level·반복 void/free를 포함한다. 실제 non-void Unpop 패치 384/CD 384회, CD 미발견 활성 입력 16개, firstPop extra 전수 512회. 대체 함수/assert 도달 0. Reset 18회와 vtable 151행/생성자 359행은 호출 수에 더하지 않았다. raw 슬롯 모든 바이트와 전체 풀/삭제 기록/86,272머리/65,536 spot의 Adler-32를 대조한다. [기록](cpppj/recovery-unpop-evidence.json).
- [x] **검증:** x64 Release 빌드 경고/오류 0, CTest 한 실행 파일 내부 **112개 검사 통과**(추가 4개). 새 unpop과 기존 derived/creation/SID의 원본/도구/표/fixture SHA-256·행 수 `--verify` 통과. 누적 **46,156행**은 제한이 있는 입력 수이며 실제 월드/GUI나 미션 완주를 증명하지 않는다.
- **남은 계약:** 합성 타입/기존 배치 상태와 표시 억제 입력이다. 실제 Pop·표시 활성/dirty·grid/표면 알림·섬/다리/건물 부착/파생 효과·파생 destructor/의존 객체·참조/패킷 수명·GameWorld는 미연결이다. Take는 free list/freeCount를 조정하지 않으므로 상위 수신/할당 흐름도 남았다. 단순 void 반납 검증을 삭제 전체 복원으로 세지 않는다. [근거·재현·제한](docs/exe/cpp-unpop-reconstruction.md).
- [x] **후속 일부 완료:** 일반 raw Pop/좌표·실제 firstPop/Activate·억제된 공통 postPop 연결은 위 최신 절에서 완료했다. 표시 활성/영역/소유자·dirty/grid·섬/다리/건물 효과·삭제/참조·월드 연결은 최신 남은 작업으로 유지한다.
- [x] **기본 GUI 보류 해소:** 최신 사용자 허용으로 위 window/renderer/menu/world 회귀를 이번 PC에서 실행했다. raw 월드 연결 뒤의 원본 SID 동작 검사는 아직 미완료이며 맨 위 다음 작업으로 인계한다. 이전 "여기서 실행하지 않는다"는 당시 PC의 제한이었다.

---

## 2026-10-06 ✅ 완료: cpppj 자산 파생 생성자·void Take 확장 (창 검증 제외)

- [x] **제한:** AGENTS.md·최신 LEFT_JOBS를 확인하고 후속 복원을 진행했다. 원본/복사본 프로세스·클론 GUI·창 스모크 실행 없음. 원본/AGENTS.md/C# 수정·커밋 없음.
- [x] **디컴파일:** 자산 생성자 패치 84개/CD 75개(CD Bomb 보조 포함), 패치 HP 보조 1개를 읽기 전용·헤드리스 Ghidra로 내보냈다. `extracted/derived/<판본>/creation.c`, `functions.tsv`, `patch-helpers/`. 부족한 함수는 메모리에서 정의하고 변경을 버렸다. 전체 대응 2,496쌍·검토 앵커 24쌍은 그대로다.
- [x] **C++:** `SquidFactory`에 패치 82개/CD 71개 생성자·98/81개 타입 연결의 순서 있는 raw 쓰기를 연결했다. 플래그 OR·섬/anim 섬 번호 word 초기화·anim HP의 패치 4/CD 2바이트·CD Bomb/outpost의 중간 vtable을 보존한다. `Construct`는 생성자만, `Create`는 기존 할당/공통 postCreate, `Take`는 free 또는 같은 타입·최종 vtable·void 객체의 실제 공통 Unpop 경로를 지원한다. null/assert(153/159/169)·알 수 없는 주소는 변경 전에 거부한다. 다른 가상 메서드와 유닛 플레이 동작의 복원 완료는 아니다.
- [x] **기계어:** 8시퀀스·총 **6,028회**(Create 716/Construct 1,432/postCreate 716/postTake 716/Take 2,448). 모든 179개 지원 타입 연결·서버/클라이언트·mana 옵션·오염 payload·반복 void Take를 포함한다. 실제 void Unpop 패치 948/CD 784회, 대체 함수·assert 도달 0. 준비 생성자 감사 306회/Reset 10회는 6,028회에 더하지 않았다. 슬롯 전체 바이트·풀 Adler-32·카운터/머리/꼬리를 대조한다. [기록](cpppj/recovery-derived-evidence.json).
- [x] **검증:** x64 Release 빌드 경고/오류 0, CTest 한 실행 파일 내부 **108개 검사 통과**(추가 4개). 새 derived와 기존 creation/SID의 원본/도구/표/fixture SHA-256·행 수 `--verify` 통과. 누적 **42,538행**은 제한이 있는 입력 수이고 별도 주소 표는 359행이다. 실제 월드/GUI 결과를 증명하지 않는다.
- **주의할 계약:** 공통 postTake는 HP를 보존하지만 **anim 생성자가 먼저 HP를 지운다**. Take는 기존처럼 free list/freeCount를 조정하지 않으므로 일반 Allocate와 혼용하는 상위 흐름은 미완료다. form/process와 다른 가상 메서드·실제 참조/패킷·non-void 공간 해제·삭제/의존 객체→SidPool 반납·GameWorld 연결은 남았다. [근거·재현·제한](docs/exe/cpp-derived-reconstruction.md).
- [x] **후속 일부 완료:** `004afe50`↔CD `004ad0b0`의 표시 비활성 일반 non-void Unpop·Take와 공통 firstPop 플래그·void 반납은 위 최신 절에서 완료했다. 섬/다리/건물/파생 가상·영역 효과, 실제 Pop/삭제/의존 객체/참조 수명·목록/카운터·GameWorld는 남았다. 다음 순서는 맨 위 인수인계를 따른다.
- [ ] **다른 PC 실행 인수인계:** raw SID/월드 연결 후 TEST01/1-1 생성·선택·이동·정지·카메라·재진입을 `cpp_world_smoke.py`로 확인한다. 창/표시/메뉴 회귀는 `cpp_window_smoke.py`, `cpp_renderer_smoke.py`, `cpp_menu_smoke.py`. 이 도구들과 `NetstormCpp.exe --run`은 **여기서 실행하지 않는다**. 현재 raw SID와 월드는 별개이므로 기존 GUI 기록을 이번 생성자 연동 검증으로 세면 안 된다. 원본 실행이 필요한 단계는 다른 PC에서 AGENTS.md/사용자 허용 범위를 확인한다.

---

## 2026-10-06 ✅ 완료: cpppj 생성자 주소 표·기본 생성/가상 초기화·free/void Take (새 창 없음)

- [x] **요청/제한:** AGENTS.md·최신 인수인계를 다시 확인하고 "새 창이 뜨지 않는 범위"에서 cpppj를 이어 갔다. 원본/복사본·클론 GUI·창 스모크 실행 없음. AGENTS.md/원본/C# 수정·커밋 없음.
- [x] **정적 디컴파일:** `ExportCreation.java`로 패치 8개/CD 13개 함수를 읽기 전용·헤드리스로 내보냈다. `extracted/creation/<판본>/creation.c`, `functions.tsv`. CD `004ad0b0`은 기존 프로젝트에 함수 정의가 없어 메모리에서만 정의하고 변경을 버렸다. 전체 대응 2,496쌍·검토 앵커 24쌍은 그대로다.
- [x] **생성자 표:** 타입 초기화의 생성자 대입 두 구간만 실제 x86으로 실행했다. 패치 188타입/122개 연결, CD 171타입/105개 연결. CD 마지막 대입까지 포함한 총 359행을 `TypeConstructors.inc`·`constructors-x86.tsv`에 저장했다. 주소는 기록값이며 호스트 함수/포인터로 사용하지 않는다. 원본 타입 로딩·이름 복사·초기화 전체를 실행한 검증은 아니다.
- [x] **C++ 연결:** `SquidFactory`의 constructor=0 생성, base postCreate/postTake, free/이미 void인 base `Take`; `RiftTypeTable`의 생성자 주소·최대 HP·숫자/이름 깊이. raw owner/깊이 위치와 HP의 패치 4바이트/CD 2바이트, 서버/클라이언트·dais abstract·약화 mana 정수 /4를 보존했다. Take는 payload·목록·카운터를 보존한다. 미복원 파생/공간 해제·잘못된 상태는 변경 전에 거부한다.
- [x] **기계어 대조:** 4시퀀스·두 판본 합계 **964회**(Create 192/postCreate 304/postTake 304/Take 164). 대체 함수 없음. 이미 void인 기존 Unpop도 실제 호출(각 판본 74회) 후 조기 반환했다. assert 0회, 함수 몸체/쓰기/시간/명령/EIP/ESP 제한. 대상 슬롯의 모든 바이트·풀 Adler-32·머리/꼬리·카운터를 대조했다. 생성자 359행과 준비 Reset 4회는 964회에 더하지 않았다.
- [x] **검증:** VS 2026 Insiders/MSVC 19.51 x64 Release 빌드 경고/오류 0, CTest 한 실행 파일의 **104개 검사 통과**(새 검사 5개). 새 생성/Take와 기존 SID의 원본/도구/fixture SHA-256·행 수 `--verify` 통과. 누적 36,510행은 제한을 포함하는 입력 수이며 완성도가 아니다. SOURCE_MAP 파일 존재 수는 29/136으로 동일하다.
- [x] **실제 자료 회귀:** 창 없는 `python -X utf8 tools/cpp_fort_smoke.py` 통과. 두 판본 합계 **488개 요새·320,634객체·326,169줄**의 구조/타입 이름·플래그·해시·미션 머리 값을 대조했다. 아카이브/낱개 요새의 전후 해시 동일. `extracted/cpp-fort-smoke/report.json`. 새 HP/깊이 원본 파서 전체나 실제 생성자/월드 동작을 검증한 수치는 아니다.
- **제한:** 실제 파생 생성자 dispatch·non-void/파생 Unpop·삭제/반납·네트워크 패킷/참조 수명·GameWorld 연결은 남았다. Take는 free list/freeCount를 고치지 않으므로 일반 서버 Allocate와 바로 혼용하는 상위 흐름을 완료한 것으로 보면 안 된다. 속성 로더 연결은 C++ 단위 검사이며 원본 파서 전체의 x86 검증은 아니다. [근거·재현·제한](docs/exe/cpp-creation-reconstruction.md).
- [x] **후속 중 완료:** 자산 파생 생성자·공통 postCreate/postTake·파생 void Take는 위 후속에서 완료했다. form/process·다른 가상 메서드·non-void 공간/삭제/참조 수명·GameWorld는 남았다. 현재 다음 작업은 맨 위 인수인계를 따른다.
- [ ] **다른 PC 실행 인수인계:** 파생/월드 연결 후 `python tools/cpp_world_smoke.py`로 TEST01/1-1의 생성·선택·이동·정지·카메라·재진입을 확인한다. 창/표시/메뉴 회귀는 `cpp_window_smoke.py`, `cpp_renderer_smoke.py`, `cpp_menu_smoke.py`. 이 도구들과 `NetstormCpp.exe --run`은 **여기서 실행하지 않는다**. 아직 월드와 raw SID가 별개이므로 기존 GUI 기록을 새 생성/Take 연동 검증으로 세면 안 된다. 원본 실행은 다른 PC의 해당 단계에서 AGENTS.md와 사용자 허용 범위를 확인한다.

---

## 2026-10-06 ✅ 완료: cpppj SID 풀·번호 할당/반납 복원 (게임/클론 GUI 실행 없음)

- [x] **요청/제한:** AGENTS.md·최신 LEFT_JOBS를 읽고 정적 디컴파일과 cpppj를 이어 갔다. 원본/복사본 실행과 창이 뜨는 cpppj 검사는 금지했다. 호스트 실행 예외보다 이번 사용자 제한을 우선 적용했다.
- [x] **디컴파일:** `ExportSid.java`로 읽기 전용·헤드리스 Ghidra에서 패치 7개/CD 6개 함수(초기화·할당·반납·서버 목록·next·삭제 기록·CRT 복사)를 다시 디컴파일했다. 결과: `extracted/sid/<판본>/sid.c`, `functions.tsv`. 어셈블리/기계어도 대조했다. 기존 전체 대응 2,496쌍·검토 앵커 24쌍은 그대로다.
- [x] **C++:** `o/SidPool`에 raw 50/36바이트·pool+0/pool+14 머리·판본별 번호 경계·FIFO 예약 꼬리·예측 커서/서버 next·void 반납의 payload 초기화/타입 보존·영역별 20개 삭제 기록·서버 목록 재구성의 카운터 누적을 복원했다. `Sid`를 임시 월드 `SquidId`와 구별했다. **이 할당/반납 경로에는 세대 비트가 없다**. 원본에 없는 세대를 추가하지 않았으며 참조 수명은 별도 후속이다.
- [x] **기계어:** 16시퀀스·두 판본 합계 **3,168회**(초기화 32/할당 1,552/반납 1,552/재구성 32). 겹친 CRT 삭제 기록 복사까지 실제 실행했다. 대체 함수/assert 도달 0. Ghidra 몸체·쓰기 범위·정상 EIP/ESP/cdecl ret 0·명령/시간을 제한했다. raw 풀/삭제 기록 전체의 Adler-32와 카운터/머리/꼬리를 대조했으며 체크섬은 바이트별 동일성 증명과 구별한다.
- **검증 제한:** Init은 기존 메모리 경로, Allocate는 소진 전 경로만 실행했다. malloc/실패·소진 UI/다리 파괴 복구는 미검증이며 C++는 소진/잘못된 상태에서 풀을 바꾸지 않고 예외를 반환한다. 합성 생성자 payload·SID 계산 검사이며 실제 파생/공간/삭제 효과·참조/Take·GameWorld 연결은 남았다. [근거·판본 경계·재현·제한](docs/exe/cpp-sid-reconstruction.md).
- [x] **검증:** VS 2026 Insiders/MSVC 19.51·내장 CMake의 VS 18 x64로 Release 구성/빌드 완료, 최종 빌드 경고·오류 0. CTest 한 실행 파일 안의 **99개 검사 통과**. SID와 기존 spatial의 기록/fixture/원본/도구 SHA-256·행 수 `--verify` 통과. 누적 35,546개는 제한을 포함하는 입력 행 수다. SOURCE_MAP 파일 존재 29/136으로 동일하며 완료율이 아니다.
- **환경:** 기존 `cpppj/build`·정밀 분석 산출물·oracle Python 패키지가 없어 원래 Ghidra 프로젝트에서 SID만 별도로 내보냈고 지정 분석 패키지를 `extracted/oracle-python`에 설치했다. Ghidra/구성/빌드/fixture/CTest 로그 쓰기의 샌드박스 제한은 해당 명령의 자동 승인 후 해결했다. VS 2022 프리셋과 기존 공통 oracle는 수정하지 않았다.
- **변경 범위:** SidPool·SidTests·sid-x86.tsv·recovery-sid-evidence.json·decomp_sid_oracle.py·ExportSid.java, CMake·.gitattributes·Squid 설명, SID 문서·README·빌드/로드맵/플레이 계획·신뢰도·이 문서. 원본/AGENTS.md/C# 수정·게임/클론 GUI 실행·커밋 없음.
- [x] **후속 중 완료:** 생성자 주소 표·constructor=0 기본 생성/가상 초기화·free/void Take는 위 후속에서 완료했다. 파생 생성자·non-void/파생 Take·참조 수명·삭제/공간 해제→SidPool 반납·GameWorld 연결은 남았다. 다음 순서는 위 최신 인수인계를 따른다.
- [ ] **다른 PC 실행 인수인계:** SID/파생/월드 연결 후 `python tools/cpp_world_smoke.py`로 TEST01/1-1의 선택·이동·정지·카메라·재진입을 확인한다. 표시/메뉴 회귀 명령은 `cpp_window_smoke.py`, `cpp_renderer_smoke.py`, `cpp_menu_smoke.py`다. 이 도구들과 `NetstormCpp.exe --run`은 창을 띄우므로 **여기서는 실행하지 않는다**. 지금은 SID 풀과 월드가 별개라 기존 GUI 기록을 새 SID 연동 검증으로 세면 안 된다. 원본 실행이 필요하면 다른 PC의 해당 단계에서 AGENTS.md/사용자 허용 범위를 확인한다.

---

## 2026-10-06 ✅ 완료: cpppj 객체 공간 등록/해제·CD판 충돌 차이 복원 (게임 실행 없음)

- [x] **요청:** "cpppj 다음 작업도 진행". 최신 AGENTS.md/인수인계를 확인하고 Pop/Unpop의 공간 갱신을 이어서 복원했다. C++ 최대 복원·Windows/Win32 우선순위를 유지했다.
- [x] **C++:** 새 `o/SquidSpatial`은 이미 할당된 void 객체 번호 입력→좌표/화면 캐시→spot OR→기준점 버킷의 next 삽입→firstPop 한 번/비전투 활성화, void 설정→spot AND→4단계 체인 검색/제거를 구현한다. 제거한 next·캐시/단계는 보존한다. 실제 등록한 0단계/spot을 정수 SurfaceFinder에 전달한다. SID 할당기나 raw 50/36바이트 풀 자체를 복원한 것은 아니다.
- [x] **분석 정밀화:** 등록 발자국의 float→int→편향 덧셈과 EffectiveGenus의 float+편향을 구별했다. 화면 캐시는 x×16/y×11+0.5의 short이며, 섬 번호 무효 표식은 127이다. 건물 양옆 표면 word의 bit 2/4 갱신·조회/알림 순서도 보존했다.
- [x] **판본 차이:** 패치판은 spot bit 충돌에서 이전 칸의 부분 변경을 남긴 채 반환하고, CD판은 OR/등록을 계속한다. 격리 시퀀스 중 **32개 전이**의 차이를 각각의 결과 열과 C++ 판본 모드로 보존했다. 전체 배치 판정으로 사용하지 않는다.
- [x] **기계어:** 두 판본 **238개 시퀀스·878회 전이(Pop 463/Unpop 415)**. 실제 타입/좌표/SHP/genus/hash/체인/SetIsland/Activate를 정상 반환까지 실행했다. 전체 86,272머리·65,536 spot·공간 필드·사건 순서를 대조했다. x87 53/64비트 전체 시퀀스 재실행 결과 일치, assert 0/0, 코드/공간 필드 쓰기·명령 수·EIP/ESP/ret N·x87 상태를 제한했다.
- **검증 제한:** 가상 갱신·firstPop·postPop, 영역 조회·표면 알림은 **입력 반환/사건 기록 계약으로 대체**했다. dirty 큐·Battle 복제는 비활성이고 별도 grid 조회 입력은 비어 있다. 실제 파생/화면/소유자/부착 프레임 효과는 미실행이다. Pop/Unpop을 전체 함수 검토 앵커에 추가하지 않아 대응 **2,496쌍·검토 24쌍**은 그대로다. [상세 근거·후속 순서](docs/exe/cpp-spatial-reconstruction.md).
- [x] **검증/보호:** Windows Release 빌드 경고·오류 0, CTest 내부 **93개 검사** 통과. 누적 **32,378개 제한 x86 입력 사례**에 새 조건부 878개를 구분해 기록했다. 기록/fixture/원본/도구 SHA-256·행 수 검사 `decomp_spatial_oracle.py --verify`를 추가했다. SOURCE_MAP 파일 존재는 29/136으로 동일하며 완료율이 아니다. 원본/복사본/클론 GUI 실행·AGENTS.md/원본/C# 수정·커밋은 없었다.
- **변경 범위:** 새 SquidSpatial·SpatialTests·spatial-x86.tsv·recovery-spatial-evidence.json·decomp_spatial_oracle.py, CMake·.gitattributes, 복원 문서·README·신뢰도/빌드/로드맵/플레이 계획·LEFT_JOBS.
- [x] **후속 일부 완료:** 위 최신 절에서 SID raw 풀·할당/반납·FIFO/예측·삭제 기록·서버 free list를 복원했다. 이 경로에는 세대 비트가 없다.
- [ ] **다음:** SID 소진 복구·참조/Take·파생 생성자→firstPop/postPop·영역/소유자・dirty/grid·표면 변경/부착 프레임 실제 효과→섬/지면/noIsland/받침 생성과 GameWorld 연결·소수 좌표/일반 공간 탐색→`0049b510` 배치/소유자 전파·Construction→전체 수명/붕괴/삭제→다리 칸/커서/배치 UI→Construct·건설/경제/전투/AI/승패.

---

## 2026-10-06 ✅ 완료: cpppj 공간 해시·발자국 점유 계산과 x87 검증 보강 (게임 실행 없음)

- [x] **요청:** "cpppj 프로젝트 더 진행". AGENTS.md/인수인계를 읽고 표면 계산의 다음 기반인 SquidHash 네 단계 배열/버킷 주소·현재 SHP 크기의 객체 단계·Squid 유효 genus를 복원했다. Win32/Windows 전용 및 C++ 최대 복원 우선순위를 유지했다.
- [x] **C++:** 새 `o/SquidHash`는 1/2/4/16 크기의 버킷, 전체 86,272개 short 머리, 선택 보존 초기화, 단계별 읽기/쓰기 조회를 제공한다. `Squid::EffectiveGenus`는 건물군 `0x50444200`·발자국 네 변·roof 중심 열의 윗부분·`0.9999f` 편향을 보존한다. 원본 객체 배열 크기 설명도 패치 50/CD 36바이트로 정정했다.
- [x] **표면 관계 확인:** `005c84bc`/CD `0052d590`는 **0단계 hash 머리 배열**이다. `004b02d0`는 기준점의 버킷에 번호를 연결하며 spot은 별도 발자국 반복에서 OR한다. 앞선 여러 셀에 같은 번호를 적은 테스트는 합성 탐색 입력이며 실제 등록이 전체 발자국을 채운다는 근거가 아니다. 새 [해시 복원 문서](docs/exe/cpp-hash-reconstruction.md)와 앞선 표면 문서를 보완했다.
- [x] **기계어:** 두 판본 **4,632개 새 입력 사례**(기존 배열 초기화 4·칸 주소 200·float 주소 607·객체 단계 605·genus 3,216). 실제 타입/기하·SHP 헤더·CRT 변환·전체 정상 반환을 사용했고 assert 보고 호출 0/0이다. 초기화의 malloc 분기는 실행하지 않았다. 주소·메모리 쓰기·명령 수·EIP/ESP/ret N을 제한했다.
- [x] **x87 보강:** Unicorn 기본 제어 워드 0이 만드는 소수 경계 결과를 판본 차이로 오인하지 않도록 정상 반올림/예외 마스크·53/64비트 제어 워드 `0x027f`/`0x037f`를 명시했다. float 입력을 두 상태/두 판본 모두 실행하여 같은 결과를 확인했다. 매 호출의 제어 워드 보존·TOP 복구도 검사한다. 원본 시작 함수 전체의 FPU 설정을 동적으로 확인한 것은 아니다.
- [x] **대응/재현:** 원본/기대값/생성·공통 도구 SHA-256·x87·assert 0·함수 존재/ret N을 확인하는 5개 검토 앵커를 추가했다. malloc을 제외한 Init은 앵커에 넣지 않았다. 현재 대응 **2,496쌍·검토 24쌍**, strong 1,738/medium 707/weak 51이다.
- [x] **검증/보호:** Windows Release 빌드 경고·오류 0, CTest 내부 **87개 검사** 통과. 누적 **31,500개 제한 x86 입력 사례**다. SOURCE_MAP 파일 존재 29/136이며 전체 모듈 완료 수가 아니다. 원본 EXE SHA-256은 동일하다. 원본/복사본/클론 GUI 실행·AGENTS.md/원본/C# 수정·커밋은 없었다. 새 코드/주석/문서는 UTF-8·한국어다.
- **변경 범위:** SquidHash·Squid genus/배열 설명, HashTests·hash-x86.tsv·검증 기록·CMake·.gitattributes, decomp_hash_oracle.py·decomp_refine 앵커, SOURCE_MAP·복원/신뢰도 문서·README·계획·인수인계.
- [x] **후속 일부 완료:** 위 최신 절에서 next 체인·등록/해제 hash·spot OR/AND·충돌 차이·표면 word·비전투 상태·정수 SurfaceFinder 전달을 복원했다. SID 할당/세대·실제 가상/영역/부착 프레임 효과·GameWorld 연결과 나머지 플레이 항목은 최신 다음 순서에 남긴다.

---

## 2026-10-05 ✅ 완료: cpppj 표면 이웃·다리 연결·붕괴 방문 목록 (원본 게임 실행 없음)

- [x] **요청:** "cpppj 프로젝트 더 진행". 최신 AGENTS.md와 인수인계를 확인하고 원본 C++ 최대 복원 목표에 따라 다리 계산의 다음 범위를 옮겼다. 새 `o/SquidFinder`는 정수 발자국의 flag 8 이웃 탐색, `Bridge` 확장은 양쪽 타입/프레임 연결과 붕괴 방문 목록 재귀다.
- [x] **이웃:** 한 칸 확장/모서리 제외·y/x 순서·자기 사각형/타입 surface/죽음/spot 내부 필터·중복 제거·중심 방향 동률을 보존했다. 큰 섬을 여러 칸에서 발견해도 한 번만 반환한다. 지도 가장자리의 spot AND 좌표 1..255 제한도 원본과 같다. 정수 발자국을 읽는 스냅샷이며 일반 공간 해시/소수 좌표 경로는 후속이다.
- [x] **재귀:** 부모 제외·접합 재귀·짧은 접합 플래그·판자/섬 경계·전체 중복 삭제·방문 목록 용량을 옮겼다. 목록이 차도 재귀는 계속한다. 원본의 접합 고리는 새 C++에서 `complete=false/canDecay=false`로 중단한다. 실제 수명/금/낙하/삭제는 실행하지 않는다.
- [x] **검증:** 두 판본의 실제 생성자/가상 필터/기하/순회/재귀를 정상 반환까지 실행한 **3,450개 사례**(연결 3,016·이웃 125·방문 목록 309)가 일치했다. 이웃이나 재귀를 대체하지 않았으며 assert 보고 대체 호출은 두 판본 모두 0회다. 코드/메모리 쓰기·명령 수·EIP/ESP/ret N을 제한했다. Windows Release 빌드 경고·오류 0, CTest 내부 **81개 검사** 통과. 기존 포함 누적 **26,868개 x86 입력 사례**이며 수명 480개는 접두 구간이다.
- [x] **대응 보강:** 원본·기대값·생성/공통 도구 SHA-256과 ret N을 확인하는 7개 검토 앵커를 추가했다. 현재 대응 **2,496쌍·검토 19쌍**(strong 1,735/medium 710/weak 51). 일반 공간 해시 다음 함수 `004b1810(ret 0)`↔CD 생성자 `004eae20(ret 16)` 오대응을 금지했다. 이 일반 경로의 새 완전 대응은 추정하지 않았다.
- [x] **계획 정정:** 이전의 `004215d0` "배치 판정" 표기는 잘못이었다. 실제로는 폭발 가장자리의 J/K→L~O 변환이다. 실제 배치는 `0049b510`↔CD `00445200`, 실제 조각 생성/품질은 Construction `00442c80`부터 이어 간다. 최신 계획과 [표면 복원 문서](docs/exe/cpp-surface-reconstruction.md)를 수정했다.
- [x] **기록/보호:** 새 문서·코드/주석은 UTF-8·한국어다. AGENTS.md·원본 자료·C# 소스를 수정하지 않았고 원본/복사본/클론 GUI를 실행하지 않았다. SOURCE_MAP 파일 존재는 28/136이며 모듈 전체 완료 수가 아니다. 커밋하지 않았다.
- **변경 범위:** Bridge·RiftType surface 비트, 새 SquidFinder·SurfaceTests·surface-x86.tsv/검증 기록, CMake·.gitattributes, decomp_surface_oracle.py·decomp_refine 검토 대응, SOURCE_MAP·복원 문서·README·계획·인수인계.
- [ ] **다음:** 실제 표면/SID·오브젝트 번호 지도·spot 생성/변경을 복원해 GameWorld에 연결→`0049b510` 다리 배치 경로/법적 위치→소유자 전파·Construction 실제 생성→`004227e0`/`00422bc0` 스캔·수명 함수 나머지·금/소리·낙하/삭제→다리 생산 칸/커서·배치 UI. 이어 Construct·덱·에너지·채집/SP·전투·AI·승패를 복원한다.

---

## 2026-10-05 ✅ 완료: cpppj 다리 계산 복원·CD판 차이·디컴파일 검증 보강 (원본 게임 실행 없음)

- [x] **요청:** AGENTS.md·LEFT_JOBS.md를 읽고 현재 디컴파일/cpppj 진행을 확인한 뒤 신뢰도를 높일 방법과 cpppj를 더 진행했다. 1차 목표는 원본의 C++ 최대 복원이며 Windows 10/11 실행·요구사항·MCP가 뒤따른다. dotnetpj의 새 개발은 cpppj 완성 뒤다. 현재 메뉴→기본 월드/선택·이동 연결을 확인하고 다음 대상인 다리 계산을 복원했다.
- [x] **C++:** CanonDecoder에 두 판본의 26개 다리 모양·추첨·전체 회전/프레임 반복을 추가했다. 새 `o/Bridge`는 한 방향/전체 열린 끝과 수명 비트 갱신·제거 플래그의 접두 부분을 구현했다. `--inspect-bridges`는 자산을 읽기만 하는 콘솔 검사다. GameWorld의 배치 기능과 전체 붕괴는 아직 연결하지 않았다.
- [x] **CD 비교:** 모양/셀/회전 표는 같지만 조각 22/23/24의 가중치가 패치 1/1/0, CD 10/10/1이다. 총합 287/306을 판본별로 보존했다. 기존 오대응 `004217f0`↔CD `00449e30`을 제거하고 실제 보조 검사 `00421770`↔CD `00449e30`을 연결했다. 전체 CD 판단은 `00449250`에 인라인이다.
- [x] **정밀 해석:** 표면 조회의 실제 상수는 `0.9999f`(비트 `3f7ff972`)이며 일반 반올림이 아니다. x87 중간 합을 float로 좁히지 않도록 옮기고 경계 입력을 검증했다. 자료형·ECX/ESP 원형을 적용한 `typed-bridge.c`는 패치 9개/CD 8개 함수에 생성했다. `_ftol`의 ST0 입력 미복구로 출력 C에 이 덧셈은 아직 빠진다. 이 부분은 어셈블리/기계어 결과가 근거다.
- [x] **기계어:** 새 18,053개 입력 사례(추첨 10,005·난수 1,056·반복자 752·열린 방향 5,760·수명 접두 480). 균일 지도 4개 구성에 칸마다 번호가 다른 지도 입력을 더해 내부 셀 주소도 대조했다. 정상 함수의 코드/쓰기 범위·명령 수·복귀/스택을 제한한다. 누락 프레임 assert 보고만 각 판본 111회 대체했다. 수명은 금/낙하/삭제 효과 전에 멈추므로 전체 함수 검증이 아니다. 전체 열린 끝 함수는 패치판만 기계어 검증했다.
- [x] **대응/재현:** 바이너리·기대값·생성 도구 SHA-256을 확인한 7개 완전 함수 대응을 검토 앵커로 추가했다. 전체 대응 **2,494쌍·검토 12쌍**, strong 1,733/medium 710/weak 51. Ghidra의 추정 인자 일괄 잠금 대신 확인한 원형/구조체만 적용하고 원본 실행값으로 재검증한다. [방법·공식 자료·재현 명령](docs/exe/cpp-bridge-reconstruction.md).
- [x] **검증:** Windows Release 빌드 경고·오류 0, CTest 내부 **75개 검사** 통과. 누적 x86 **23,418개 입력 사례**에는 위 접두 구간 480개가 포함된다. 두 판본 각각 26모양·104회전·468셀(합계 936셀)이 실제 PE 표/타입 자산과 독립 대조로 일치했다. 영역 회귀 **488요새·320,634객체·326,169비교 행**도 통과했다. 이번 GUI 조작 검사는 추가하지 않았다.
- [x] **보호/기록:** 원본 두 폴더 1,819파일의 해시·존재 상태 유지. 원본 게임·복사본·클론 GUI를 실행하지 않았고 AGENTS.md·원본 자료·C# 소스를 수정하지 않았다. 새 소스/주석/문서는 UTF-8·한국어 규칙을 따른다. SOURCE_MAP의 파일 존재 표시는 27/136이며 전체 모듈 완료 수가 아니다. 커밋하지 않았다.
- **변경 범위:** o/Bridge, CanonDecoder/생성 표, app 검사 명령·CMake, BridgeTests/기대값/검증 기록, decomp_bridge_oracle.py·cpp_bridge_smoke.py, 검토 대응 보강, ApplyBridgeTypes/recover_bridge_types, PowerShell 5의 Java stderr 경고 처리, 복원 문서·계획·README·SOURCE_MAP.
- **후속 반영:** `004218b0` 방문 목록과 표면 이웃 계산은 위 최신 절에서 완료했다. 당시 `004215d0`를 배치로 적은 것은 폭발 끝 칸 변환의 오기이며 실제 배치는 `0049b510`이다. 실제 표면/SID·spot→배치/소유자 전파→전체 수명·붕괴/삭제→다리 UI의 최신 순서는 위 절을 따른다. 기존 bridge bool만으로 원본 연결 그래프를 대체하지 않는다.
- [ ] **이어서:** 사제 Construct→템플/워크샵·덱·에너지→채집/SP→전투·AI·포획/희생·승패/결과·소리. 1-1 완주는 전체 복원의 중간 기준이다. 별도 분석 보강으로 CRT `_ftol`의 ST0 입력/EDX:EAX 반환 규약을 좁혀 typed C 식을 복구한다.

---

## 2026-10-05 ✅ 완료: cpppj 기본 실제 지형·객체·선택/사제 이동·카메라·정지 연결 (원본 게임 실행 없음)

- [x] **요청:** "cpppj 다음 사항도 이어서 진행". 현재 계획 3단계의 기본 월드/조작을 연결했다. 메뉴의 정적 InspectView를 새 GameWorld로 교체하고, 원본 저장 Tutorial 미션의 초기 지형·객체/점유·플레이어 상태를 생성한다. 기본 실행은 메인 메뉴이며 `--mission TEST01`은 동일 State→Loading→Briefing→월드 검사 진입점이다. `--view` 독립 검사는 유지한다.
- [x] **지형:** 원본 Terrainbuilder의 전체 통로→청크별 시드/목표량 성장→두 차례 빈틈 보정, 원소별 isle·AA 3×3 그림·fringe y+4칸, 완전한 noIsland 3×3 저장 받침의 island/islandStalag. 표시와 경로의 지지/점유 격자를 공유한다. 개별 변형/받침 소유 외관은 고정 시드·정적 어댑터다.
- [x] **초기 상태:** 저장 좌표·프레임·수량·작업장 상태·내용물 보존, 소유자 정규화/중립 처리, 타입 최대 HP/속도, 미션 myStartMoney·시작 지식/허용 표·동맹·색. 1-1은 저장 Money 100000 대신 3000 SP, TEST01은 50000 SP다. 저장 기술/Deck은 별도로 보존하며 생산 덱/AI는 실행하지 않는다.
- [x] **입력/갱신:** 몸통 좌클릭 선택→땅 좌클릭 이동→선택 해제, 허공 거부, 우클릭 객체 메뉴, 원본 1.8칸/초 사제·A~H 걷기 프레임. 8방향 경로·대각선 모서리 제한, 화살표/Alt·가운데 버튼 카메라·F4/H 신전·F5 사제 보기·P/R 사제 선택. 메뉴/대화상자는 정지/재개한다. 미복원 Construct/About 항목은 비활성이다.
- [x] **수명/표시:** Kernel이 월드를 소유하고 입력→커널 이동→현재 객체/지형 목록→Renderer 순으로 갱신한다. 입력 참조→커널 월드→미션 설정 해제와 재진입 초기화를 검사했다. 실제 조작 화면은 정적 SceneImage를 쓰지 않으며, SceneImage는 정지한 대화상자 배경에만 쓴다. 투명 선택 표시 수명도 Renderer에 연결했다.
- [x] **검증:** Windows Release 빌드 경고·오류 0, CTest **69개 검사**(새 10개). 패치판 1-1/Save the Island!/tutorial1/TEST01 및 CD판 1-1/Save the Island! **6개 초기 자료 사례·393216마스크 바이트·2593객체**가 독립 Python 판독과 일치한다. 클론 창 **20개 조작 상태**에서 1-1 `(94,108)`→`(98,108)`, TEST01 `(171,202)`→`(174,202)`, 속도/정지/재개·우클릭/허공·카메라/홈·복귀 후 재생성을 확인했다. 새 월드 함수의 x86 대조는 추가하지 않았으며 기존 **5365개 기대값**을 유지한다.
- [x] **회귀/보호:** 메뉴 **54개 상태**·GIF/배경·해상도/설정 재실행, Renderer **18글꼴/4608글리프/302480픽셀·실제 글꼴 창786432픽셀·커서 각18개·미변경4프레임**, 창/영역 **488요새/2119영역** 검사 통과. 설정/시작 표시를 성공·실패 모두 복구하며 두 원본 폴더의 전체 파일 해시·존재 상태를 대조했다. 새 NetstormCpp만 실행했고 AGENTS.md·원본 자료·C# 소스는 수정하지 않았다. 커밋하지 않았다.
- **변경 범위:** 새 o/TerrainBuilder·Player·Squid 부분 구현, 새 client/GameWorld·부분 UserInput, UberGump/ClientMain/Renderer/진입점·CMake, WorldTests·cpp_world_smoke.py, 월드 복원 문서·계획·README·메뉴/빌드 문서·SOURCE_MAP. 원본 모듈 파일 존재 표시는 136개 중 26개이며 전체 모듈 완료 수가 아니다.
- **3단계 미완료:** 원본 생성자/가상 함수·SID/세대/좌표 해시·파생 프로세스·내용물 객체화, Battle/Normal 재배치/옵션·생산 덱·튜토리얼 단계/동적 가이저, 동적/불완전 받침·소유자 전파·edgeFarm, 전체 zorder·그림자·상황별 커서·정지 객체 애니메이션/전역 난수, 원본 경로 알고리즘·재탐색/충돌/낙하/기절/비행/수확·출발 지연. 기본 HP는 타입 최대값이며 QA/QB의 타입별 실행 상태는 아직 해석하지 않는다. 카메라/선택 표시/HUD·프레임 주기는 임시 어댑터다.
- [ ] **바로 다음(4단계 첫 부분):** 원본 다리 조각/배치·점유/연결/붕괴→사제 Construct 건설→템플/워크샵·생산 덱/에너지→가이저 채집/SP 갱신. 이어서 전투·AI·포획/운반/희생·승패/결과/재시작·소리를 붙여 1-1 완주로 진행한다. 전체 플레이 가능 상태라고 표시하지 않는다.

---

## 2026-10-05 ✅ 완료: cpppj 메뉴→캠페인→브리핑→정적 미션 표시/메뉴 복귀 (원본 게임 실행 없음)

- [x] **요청:** "cpppj 다음 단계도 진행". 실제 플레이 계획의 2단계 탐색 경로를 구현했다. `--run originals --window` 기본 화면이 원본 타이틀/8개 버튼의 메인 메뉴로 바뀌었다. 기존 `--view` 검사는 유지한다.
- [x] **메뉴/입력:** 원본 GIF의 색 번호·fortGump 질감/모서리·원본 비트맵 글꼴. 돌 버튼 캡처/같은 영역 뗌·아래 판정 +1픽셀, 목록 누름 즉시 실행·호버·비활성 항목 차단. 다음 프레임의 UI State에서 명령을 실행한다.
- [x] **원본 자료 연결:** tell/offical 스크립트의 제목·파일 목록·조건·잠금/완료 표시·Back·시간 제한. 1-1 A. 브리핑과 튜토리얼 1 A./A1. MORE/BACK·정지/확인, 스크립트/요새 로드·정적 표시·Game/ESC·Stay/메인 메뉴 복귀.
- [x] **옵션/표시 삭제:** 원본 키 저장, 640×480/800×600/1024×768 장치·창·Renderer 전환, 음량/소리 설정의 재실행 유지. `Client::ClearFullScreenState`를 원본 `004cebf0`의 메인 버튼 활성 사건에 연결했다. 오른쪽 클릭/일반 종료에서는 지우지 않는다(A6b 완료).
- [x] **검증:** Windows Release 빌드 경고·오류 0, CTest 내부 **59개 검사**. 메뉴 스모크 **54개 프레임 상태**·1-1 **172개**/튜토리얼 **3개** 요새 레코드 로드, 두 판본 GIF 각각 **831,008픽셀**, 실제 메뉴 배경 **774,432픽셀**, 세 해상도·브리핑/복귀·잠금·옵션 재실행. 새 메뉴 함수의 기계어 대조는 추가하지 않았으며 기존 x86 **5,365개 기대값**은 유지한다.
- [x] **자료 보호:** 새 `NetstormCpp.exe`만 구동했다. 메뉴 스모크는 설정/시작 표시를 `finally`에서 복구하고 두 원본 폴더의 전체 파일 해시·존재 상태가 같음을 검사한다. AGENTS.md·C# 소스는 수정하지 않았다. 커밋하지 않았다.
- **변경 범위:** Gump 헤더, State/조건 파서, 새 UberGump, VFS 목록 검색, 미션 본문, Client 루프/메뉴/해상도·검사 연결, Renderer 정적 장면 합성, InspectView·진입점·CMake, MenuTests, cpp_menu_smoke.py, 메뉴 복원 문서·계획·README·기반 문서·SOURCE_MAP.
- **완료 범위의 제한:** 이 경로는 **원본 UI 전체/미션 타입 객체/실제 월드 복원 완료가 아니다.** 정적 미션 표시는 지형도 생성하지 않는다. 전체 StyleText(이미지/링크/스크롤/색)·정확한 패널/부모 유지 하위 메뉴·전체 Help/기술·인라인 명령·팁/자동 데모·음향이 남았다. 옵션의 소리/가장자리 이동 등은 설정 저장만 연결했다. 미복원 명령은 비활성화하거나 원본 NotImplemented 창으로 표시한다.
- [x] **후속 기본 연결 완료(3단계):** 맨 위 최신 절의 실제 지형/객체·시작 상태·선택/이동·카메라/정지로 교체하고 TEST01/1-1을 검사했다. 원본 생성자/SID/미션 종류·덱·전체 표시 등의 완전 복원은 계속 남으며, 다음은 다리/건설/경제·전투/AI·승패/사운드다.

---

## 2026-10-05 ✅ 완료: cpppj 실제 플레이 복원 목표 문서화·1단계 표시 기반

- [x] **요청 반영:** cpppj도 기존 게임이 온전히 동작하고 실제 게임 플레이 가능하도록 복원한다. 메인 메뉴→캠페인→브리핑→선택·이동·건설·경제·전투→승패·결과→재시작/메뉴 복귀가 첫 플레이 이정표이며, 이후 원본 전체 기능·미션으로 확장한다. 화면비 확장·한국어·Linux·60/120프레임·MCP·추가 기능은 후순위다. Win32 직접 호출·Windows 전용 범위는 유지한다. AGENTS.md는 수정하지 않았다.
- [x] **계획:** 새 [cpp-playable-plan.md](docs/cpp-playable-plan.md)에 현재 작업 순서와 완료 기준을 정리하고 README·빌드/기존 로드맵을 연결했다. 이번 "1단계"는 표시 기반이며 기존 로드맵의 "설정·경로" 1단계와 다르다. dotnetpj 기록을 cpppj 완료로 세지 않는다.
- [x] **Renderer 기반(B1 일부):** 100항목 변경 표·무시/불투명 플래그·면적 병합, 깊이/y/x 정렬, 프레임 해독 캐시, 투명·소스 변환·대상 변환 그림자, 변경 영역 합성/출력. `Client::draw`를 제거하고 Frame/WM_PAINT를 연결했다. 변화 없는 화면은 다시 그리거나 출력하지 않는다. `InspectView`는 장면 목록을 공급하는 어댑터로 바꿨다.
- [x] **글꼴·커서(B5 일부):** `.chfnt` 네 정수 표·256개 VFX 글리프·공백·원본 슬롯/스타일·글자/그림자/외곽선. 캐시가 없거나 손상됐으면 같은 GDI 글꼴로 메모리에서 생성하고 원본 파일에 쓰지 않는다. 원본 커서 18개·핫스팟·WM_SETCURSOR·8비트 소프트웨어 프레임·이동 흔적 복구를 연결했다. 실제 상황별 커서 선택은 후속 UserInput 범위다.
- [x] **검증:** Windows Release 빌드 경고·오류 0, CTest 내부 **52개 검사 통과**. Renderer 정렬 261·변경 표 시퀀스 133 = **394개**가 두 판본 기계어와 일치하고 기존 기대값과 합계 **5,365개**다. 글꼴 **18개·4,608글리프·302,480픽셀**, 실제 글꼴 창 **786,432픽셀**이 독립 Python 판독 결과와 일치한다. 5프레임에서 그리기/출력 각 1회, 뒤 4프레임 출력 없음. 두 판본 실제 리소스로 소프트웨어 커서 각각 18개 생성·검사 창 구동도 통과했다.
- [x] **회귀·자료 보호:** `cpp_assets_smoke.py`의 217타입·6,777이미지·15,940,472픽셀, `cpp_window_smoke.py`의 488개 요새·2,119영역과 타입 표 픽셀 검사 통과. 설정/시작 표시를 검사 후 복구했으며 원본 해시·목록이 유지됐다. 원본 게임 프로세스는 실행하지 않았다. **커밋하지 않았다.**
- **수정 범위:** 새 `client/Renderer`·`BitmapFont`·`Cursor`, ClientMain·InspectView·검사 진입점·CMake·RendererTests. 새 `tools/decomp_renderer_oracle.py`·`cpp_renderer_smoke.py`, `renderer-x86.tsv`·`recovery-renderer-evidence.json`, 표시 복원/플레이 계획 문서, README·빌드/로드맵·chfnt 분석·SOURCE_MAP.
- **미완료:** 원본 `004994b0` 전체 함수, Squid·지형·Gump 목록 수집, 객체별 `00498220`, 실제 프레임/소유자·그림자 표 선택, 구름·시차·카메라 복사·DirectDraw. 원본 qsort 동률 순서는 미확정이며 현재 안정 정렬이다. **메인 메뉴·실제 월드·게임 플레이는 아직 없다.** 표시 기반 완료를 전체 Renderer/게임 완료로 처리하지 않는다.
- [x] **후속 연결 완료(현재 계획 2단계 탐색 경로):** 메뉴→캠페인→브리핑→정적 미션 표시/취소와 `Client::ClearFullScreenState` 사건을 연결했다(맨 위 최신 절). 실제 월드/조작은 3단계, 1-1 완주는 4단계다.

---

## 2026-10-05 ✅ 완료: cpppj 옵션 저장·전체화면 시작 표시 복원 (원본 게임 실행 없음)

- [x] **원본 저장 시점·경로(A1·A2):** 시작 `004359f0` → `00441d10(0)`에서 설정 해석 전 저장. 메인 루프 `00439ad3` → `00441de0`는 변경 표시가 켜졌을 때 저장한다. 종료 전용 저장 호출은 없다. 경로 포인터는 설치 폴더가 아니라 빈 문자열과 `d`다. CD판 대응과 실제 호출 지점을 함께 확인했다.
- [x] **저장 연결(A3):** `Client::Run`의 시작 자리와 입력 뒤·커널 갱신 전 변경 검사에서 같은 `<게임 폴더>/d/options.cfg`에 저장한다. `ConfigInterface::SaveFile`은 쓰기·인코딩 실패 때 변경 표시를 보존한다. `global.dd…` 등 실행 중에 쓴 설정도 원본과 같이 저장한다.
- [x] **바이트 대조(A4):** UTF-8 BOM 추가를 제거하고 내부 UTF-8을 Windows-1252로 되돌린다. `decomp_options_oracle.py`가 두 판본의 저장·섹션 읽기·XOR 쓰기 기계어를 격리 실행한다. 16개 기대값(비ASCII·4KB 경계·실제 설정 버퍼 포함), 실제 설정 저장 834/114바이트가 C++와 일치했다. 파일 이름 섹션이 없으면 원본은 서명을 추가하지 않는 예외도 보존했다. **원본 게임 프로세스를 실행한 대조는 아니다** — 같은 버퍼의 원본 저장 기계어와 대조했다.
- [x] **검사 보호(A5):** `--config-save`는 게임 폴더에서 `d/options.cfg`만 허용한다. 대소문자·`..` 표기로 다른 원본 파일에 쓰는 것도 거부한다. 설정·창 스모크는 두 허용 파일을 보관하고 `finally`에서 바이트·존재 여부를 복구한다. 실패 경로의 복구도 임시 폴더에서 검사했다. 요새 스모크는 읽기 명령만 실행하므로 쓰기 예외가 필요 없다.
- [x] **시작 표시 분석·조회/생성(A6 일부):** `fullscreenStateFile.dat`는 게임 폴더에 두며, 내용을 읽지 않고 존재만 검사한다. 전체화면 시도 전 `r+b`/없으면 `w+b`로 열고 닫는다. 조회·생성과 삭제 함수는 복원했다. **삭제 함수의 UberGump 버튼 사건 연결은 남음**(B6와 함께).
- **분석 정정:** `004395df`는 별도 반환 함수가 아니라 WinMain의 분기 목적지다. 표시 파일이 있거나 전체화면 초기화가 실패하면 원본도 플래그를 0으로 하고 창 모드로 이어 간다. 이전 절의 “복구는 원본과 달라서 이후 단계로 미룬다”는 해석을 취소한다. 재실행 오류의 전체 원인은 DirectDraw·UberGump 복원 뒤 확인한다.
- **검증:** Windows Release 빌드 경고·오류 0, CTest 실행 파일 내부 **46개 테스트**, x86 기대값 **4,971개**. 실제 자료 스모크 5종(recovery·assets·config·fort·window) 전부 통과. 창 스모크는 전체화면 시작 표시 생성·남은 표시의 창 모드 재실행도 검사했다. 원본 파일 해시·목록과 사용자 설정이 검사 전후 동일하다. AGENTS.md·C# 소스는 수정하지 않았다. **커밋하지 않았다.**
- **수정 범위:** `ClientMain`, `ConfigInterface`, `OriginalText`, 검사 진입점·파일 출력 주석, 설정 단위/기계어 검사·CMake, 설정/창 스모크. 새 `tools/cpp_smoke_files.py`·`decomp_options_oracle.py`, `cpppj/tests/fixtures/options-x86.tsv`·`recovery-options-evidence.json`, [복원 문서](docs/exe/cpp-options-reconstruction.md). README·빌드/로드맵·설정/화면 복원 문서도 현 상태로 갱신했다.
- **당시 다음 작업:** 아래 B1 Renderer부터 — 표시 기반은 위 최신 절에서 완료했다. 현재 다음은 메인 메뉴·브리핑이다. UberGump 버튼 사건의 `Client::ClearFullScreenState` 연결과 DirectDraw는 남았다.

---

## 2026-10-05 📌 인수인계: cpppj 목표 변경과 `options.cfg` 결정, 앞으로 할 일

> 이 절은 당시 **문서만** 고친 인수인계다. 이후 A1~A5와 A6의 분석·조회/생성을 완료했다(바로 위 절). 아래 체크 상태와 정정은 후속 작업 결과를 반영한다.
> **2026-10-10 갱신:** 아래의 목표 순서 ①②③은 2026-10-05 당시 AGENTS.md 문구 기준이다. 현재 AGENTS.md는 **1차 10.78 싱글플레이 복원 → 2차 Windows 10/11 오류 없는 실행 → 3차 TCP/IP 로컬 멀티플레이·outpost → 4차 요구사항 반영·MCP·한국어 지원**이다. 아래 ②는 2차, ③은 4차에 해당하고 3차가 그 사이에 들어간다. 최신 순서는 이 문서 머리말과 [cpp-playable-plan.md](docs/cpp-playable-plan.md)를 따른다.

### 바뀐 것 (사용자 결정)

1. **AGENTS.md의 cpppj 목표가 바뀌었다**(사용자가 직접 수정, 커밋 `bfb780c`에 포함).
   - 지금 문구: "cpppj - C++ , 기존 게임을 디컴파일하여 C++ 코드로 최대한 복원하는 것이 1차 목표이다. 이후 윈도우10, 11에서 실행 가능한 수준으로 만들고, 요구사항 반영 및 MCP 추가한다. 윈도우용만 개발한다."
   - "dotnetpj - C# + MonoGame 기반으로 개발. cpppj 완성 후 이를 분석하여 개발."
   - 이전 문구는 "…최대한 복원하여, 실행 가능한 수준으로 만들고, 요구사항 반영 및 MCP 추가 (윈도우용만 개발)"였다. 달라진 점은 **복원이 1차 목표로 앞에 놓이고, 실행 가능 수준·요구사항·MCP가 "이후"로 순서가 정해진 것**, 그리고 **dotnetpj는 cpppj 완성 뒤에 그것을 분석해 개발한다**는 것이다.
2. **`options.cfg`는 원본 게임과 동일하게 처리한다.** AGENTS.md가 `options.cfg`와 `fullScreenStateFile.dat`의 수정·삭제를 예외로 허용하고 있다. cpppj는 따로 둔 저장 위치를 만들지 않고, 원본이 읽고 쓰는 `<게임 폴더>/d/options.cfg`를 원본과 같은 시점·같은 형식으로 읽고 쓴다. 직전 절의 "사용자 결정이 필요한 것: 설정 저장 위치"는 이것으로 정해졌다.

### 이 결정을 반영한 문서

[cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md) 1·3·5·7·8·12절, [cpp-build.md](docs/cpp-build.md) 1·6절, [cpp-roadmap.md](docs/cpp-roadmap.md) "목표와 기준"·1단계·"다음 작업 세션의 범위", [cpp-config-reconstruction.md](docs/exe/cpp-config-reconstruction.md) 검증 절, [cpppj/README.md](cpppj/README.md).

### 목표 순서를 작업에 적용하는 방법 (에이전트의 해석 — 틀리면 고쳐 주세요)

- **① 1차 목표(복원)** 동안에는 원본의 동작을 고치지 않고 그대로 옮긴다. 원본에 없는 기능(와이드 화면, 60·120프레임, 한국어, 가장자리 스크롤의 확장)과 원본 문제의 수정(전체화면 재실행 오류)을 앞당겨 넣지 않는다. **정정:** 전체화면 초기화 실패 후 창 모드로 이어지는 흐름은 역어셈블에서 원본 동작으로 확인했으므로 복원 단계에 포함한다(위 완료 절).
- **② Windows 10/11에서 실행 가능한 수준**: 복원한 코드가 지금의 Windows에서 돌지 않는 부분(DirectDraw 8비트 전체화면, 전체화면 재실행 오류 등)을 이 단계에서 다룬다.
- **③ 요구사항 반영과 MCP**: AGENTS.md의 화면비·프레임·언어 요구와 MCP.
- **dotnetpj**: 현재 AGENTS.md는 "cpppj 완성 후 이를 분석하여 개발"로 정한다. cpppj 작업을 진행하며 dotnetpj의 새 작업은 시작하지 않는다. 아래쪽 dotnetpj 이력·남은 일은 보존한다.

### 앞으로 할 일

**A. `options.cfg`를 원본과 동일하게 (A1~A5 완료, A6의 메뉴 사건 연결 남음)**

- [x] **A1. 저장 시점 정적 분석.** 시작 한 번과 메인 루프의 변경 후 저장. `00441d10`의 실제 호출은 인자 0 한 곳이다. 자세한 호출 주소는 [옵션 저장 복원](docs/exe/cpp-options-reconstruction.md).
- [x] **A2. 경로 조립 확인.** 포인터는 빈 문자열과 `d`. 게임 폴더의 `d/options.cfg`다.
- [x] **A3. 저장 연결.** 시작·변경 검사 자리에서 `SaveOptions` → `ConfigInterface::SaveFile`.
- [x] **A4. 원본 바이트 대조.** BOM 제거·Windows-1252 역변환. 원본 게임을 실행하지 않고 두 판본의 저장·XOR 쓰기 기계어로 같은 버퍼의 16개 입력 및 실제 저장 파일을 대조했다.
- [x] **A5. 검사 도구 설정 예외·복구.** 게임 폴더에서는 `d/options.cfg`만 설정 출력 허용. 설정/창 스모크는 설정·시작 표시 파일을 검사 후 복구한다. 요새 검사는 읽기 전용이라 추가 예외 불필요.
- [x] **A6a. `fullscreenStateFile.dat` 분석·조회/생성·삭제 함수 복원.** 존재 검사·생성·UberGump 사건의 삭제 용도와 창 모드 분기를 확인했다. 조회·생성은 실행 경로에 연결했다.
- [x] **A6b. UberGump 삭제 사건 연결.** 같은 날 메뉴 단계에서 `004cebf0`의 메인 버튼 활성 사건에 연결했다. 상세 검사는 맨 위 최신 절/메뉴 복원 문서를 본다.
- **주의:** 지금 `originals/d/options.cfg`는 `startInFullScreen = 1`, `SCREENW = 1024`, `SCREENH = 768`이다(2026-10-05 읽기 전용 확인). cpppj는 DirectDraw가 없어 창 모드로 뜨고 `global.ddFullScreen = 0` 등을 설정 버퍼에 쓴다. 저장을 켜면 이 값들이 `options.cfg`에 들어가 원본 게임의 다음 실행에 영향을 줄 수 있다 — 원본도 같은 키를 저장하는지 A4에서 함께 본다.

**B. 1차 목표: 디컴파일 코드 복원을 이어 간다** ([cpp-roadmap.md](docs/cpp-roadmap.md) "다음 작업 세션의 범위")

- [ ] **B1. Renderer 전체**(`004994b0`·`00499fe0`): **표시 기반은 완료**(맨 위 최신 절). `Client::draw` 제거·변경 영역·정렬·색/그림자 합성·프레임 캐시 완료. 실제 Squid/지형/Gump 목록·프레임/표 선택·구름/카메라와 `00498220`이 남아 전체 항목은 완료하지 않는다. `InspectView`는 목록 공급 어댑터로 유지한다.
- [ ] **B2. 오브젝트(Squid)**: 타입별 생성자 표, `.fort`를 읽으며 오브젝트를 만드는 경로, 소유자 결정, 프레임 선택(기본·난수·방향).
- [ ] **B3. 섬 지형**: `Islandbuilder`의 나머지(`0046da70` 이후)와 Terrainbuilder — [terrain-and-bridges.md](docs/exe/terrain-and-bridges.md). `CanonDecoder`의 다리·섬·그 밖의 패턴 표(`0052f998`, `00531410`, `005314a0`, `005314e8`).
- [ ] **B4. UserInput**(`004d62b0`): 화면 이동부터. 끝나면 임시 연결점 `Client::input`을 없앤다.
- [ ] **B5. 화면 장치의 나머지**: DirectDraw 표면(종류 3·4·5)·전체화면·플리핑·구름/시차가 남았다. **글꼴·원본 커서·WM_SETCURSOR·소프트웨어 프레임은 표시 기반 단계에서 완료**(맨 위 최신 절). 상황별 커서 선택은 UserInput 복원 뒤 연결한다.
- [ ] **B6. 메인 루프의 나머지**: **UI 상태 전환은 메뉴 단계에서 연결했다.** 원본 State 전체(`004b88c0`)·갱신 목록·튜토리얼 안내·플레이어 알림·소리("init sound" `004aa600`)·네트워크(후순위)는 남았다.
- [ ] **B7. 원본과 다르게 둔 것을 원본대로 되돌릴지 정한다**([cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md) 7절): 뮤텍스 `TitanicNetStormMutex`(지금은 만들지 않는다 — 원본과 함께 띄우기 위해서였다), 아이콘·로딩 그림을 원본 실행 파일에서 읽는 것, 레지스트리 읽기·CD 찾기, `WM_PAINT` 처리. 1차 목표의 원칙대로면 원본대로 옮기는 것이 기본이다.
- [ ] **B8. x86 기대값**: 입력 큐·사각형 자르기·패턴 반복자(지금은 실제 자료 대조만 있다). 회전된 영역은 실제 파일에 없으므로 x86 실행으로만 확인할 수 있다.

**C. 그 이후 (② 실행 가능 수준, ③ 요구사항·MCP)** — 순서만 적는다. 상세는 [cpp-roadmap.md](docs/cpp-roadmap.md) 5~8단계.

- [ ] Windows 10/11에서의 전체화면과 **전체화면 전환 뒤 재실행 오류 수정**, 16:9·16:10·4:3, 가장자리 스크롤, 60·120프레임, 한국어(D2Coding).
- [ ] MCP: 미션 로드·화면 캡처·상태 조회·입력 명령.
- [ ] dotnetpj: cpppj 완성 뒤 분석해 개발(위 "사용자 확인이 필요하다" 참고).

### 다음 세션의 시작 방법

1. AGENTS.md와 이 절을 읽는다. 빌드·검사 명령은 [cpp-build.md](docs/cpp-build.md) 3절(`cmake` → `ctest` 43개 → 스모크 5종)이다.
2. A1~A5·A6a는 완료했다. [옵션 저장 복원](docs/exe/cpp-options-reconstruction.md)의 역어셈블 정정과 검사 명령을 먼저 본다.
3. B1(Renderer)부터 진행한다. A6b는 UberGump와 함께 연결한다. 작업 방식은 [cpp-roadmap.md](docs/cpp-roadmap.md) "디컴파일 신뢰도를 높이는 작업 방식"을 따른다.

---

## 2026-10-05 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: cpppj Win32 창·화면 장치·입력 큐·영역 배치 복원

- [x] **요청:** "cpppj 계속 진행. Win32 로 진행, cpppj 는 기존 게임 원본을 되살리는 목적으로, 기능 변경을 최소화하고 이후 MCP를 추가해 기존게임 분석에 도움이 되려고 만드는 것이라 최대한 기존 게임과 동일한 방식으로 만들어야 한다." 그 앞의 결정: cpppj는 Linux를 지원하지 않고, 한국어 지원은 cpppj에서는 후순위다. 원본 게임 프로세스는 실행하지 않았다(띄운 것은 새로 빌드한 `NetstormCpp.exe`뿐이다). AGENTS.md·원본 파일·C# 소스는 수정하지 않았다. **커밋하지 않았다.** [근거·범위·검증](docs/exe/cpp-screen-reconstruction.md).
- [x] **플랫폼 결정:** Win32 API를 원본처럼 직접 부른다. 외부 라이브러리 없음. `cpppj/CMakeLists.txt`가 Windows가 아니면 구성에서 멈추고, `netstorm` 라이브러리가 `user32`·`gdi32`·`winmm`에 연결된다. CI의 `cpp-build-test`는 Windows만 돈다. "OS 호출은 `platform/`을 거친다"는 규칙을 바꿨다: `client/`는 원본이 부른 Win32 함수를 그 자리에서 부르고, `platform/`은 원본에 없는 보조 기능만 담는다([cpp-build.md](docs/cpp-build.md) 5절).
- [x] **창과 메인 루프(`client/ClientMain`)** — WinMain `00438dc0`의 순서대로: 창 클래스 `NetstormClient`(스타일 `0x23`), 창 만들기, 로딩 화면(`00436260`), 설정 시작·해석(`00435220`의 언어·`SCREENW`/`SCREENH`·`highPriorityValue`·`initWindowPos`·`sleepPerLoop`·`maxFPS`), 화면 초기화 → 팔레트(`GamePalSpec` + `fortPal`) → DIB 섹션 → 화면 모드 → `global.dd…` 설정, 타입 읽기, 메인 루프(우선순위 → 시각 고정 → 메시지 → 입력 자리 → 커널 → 프레임 제한·그리기 `00436450` → 쉼). 창 프로시저 `00436550`, 키·글자 사건 `00436020`·`00436190`, 폴링 `00452e00`. 옮기지 않은 원본 단계는 같은 자리에 `[원본]` 주석으로 남겼다.
- [x] **화면 장치(`client/Screen`)** — 표면 객체(종류 1 DIB 섹션·2 창 장치 문맥), 화면 모드 플래그와 허용 조건(`004a13d0`), 창 모드 경로(`004a4fe0`), 팔레트(`004a2640`: 0번 검정·255번 흰색 고정, `SetDIBColorTable` + 논리 팔레트), 잠금·자르기·채우기·지우기, 창으로 복사(`004a1800`, `BitBlt`), 커서 표시. **DirectDraw(전체화면·플리핑)는 옮기지 않았다.** 그래서 "DirectDraw 없음" 상태로 동작하고, 전체화면 요구는 원본 규칙대로 창 모드(플래그 10)가 된다.
- [x] **입력 사건 큐(새 파일 `client/InputEvent`)** — 40칸 고리(`00452f90`·`00452fe0`·`00452f00`), 사건 코드 비트.
- [x] **영역 배치(새 파일 `o/CanonDecoder`·`o/ChunkMap`·`o/Islandbuilder`)** — 패턴 반복자(`00425c20`·`00425860`), 청크 지도와 섬 목록, 영역 놓기(`0046dba0`·`0046dd70`), 영역 청크 순회(`004be2c0`). 표는 새 `tools/cpp_canon_tables.py`가 원본 실행 파일에서 뽑아 `CanonDecoderTables.inc`로 만든다(패턴 68개, CD판에도 같은 바이트). 이로써 `TerrNN` 청크의 월드 위치가 정해져 TEST01의 모든 오브젝트에 월드 칸 좌표가 붙는다.
- [x] **검사용 화면(새 파일 `app/InspectView`, 원본에 없는 임시 코드)과 명령** — `--run <게임 폴더> [--view types|<미션>] [--window] [--frames N] [--screenshot out.bmp] [--set "k=v;k=v"] [--cd]`, `--dump-territories <게임 폴더> [--cd]`. Renderer·UserInput 자리에는 임시 연결점(`Client::draw`·`input`·`ready`)을 두었다. **게임 화면의 재현이 아니다**(지형·그림자·프레임 선택·그리기 순서 없음).
- **검증(이 PC):** Windows Release 빌드 경고·오류 0. CTest **43개 테스트**(새 13개: 화면 모드·사각형 자르기·창 위치·입력 큐·패턴 표·반복자·청크 지도·섬 목록·영역 놓기), x86 기대값 4,955개는 그대로. 실제 자료 검사 5종 통과 — 새 `tools/cpp_window_smoke.py`: ① 두 판본의 모든 `.fort`에서 C++가 놓은 청크(좌표·방향 글자·순서)가 `TerritoryPatterns.json`으로 Python에서 따로 계산한 결과와 줄 단위로 일치(패치판 폴더 **465개·영역 2,060개**, CD판 **23개·영역 59개**), 영역마다 청크 수 = `TerrNN` 청크 레코드 수. ② 창을 5프레임 띄워 받은 타입 표 화면이 Python 해독기로 따로 그린 그림과 **모든 픽셀 일치**(1024 × 768, CD판 800 × 600). ③ 미션 화면(TEST01, CD판 `thewarbegins`)은 팔레트 색만 쓰였는지와 그려진 양. 원본 파일 해시 유지. 실제 창은 바탕 화면 캡처로 1회 눈으로 확인(제목·원본 아이콘·1024 × 768 클라이언트 영역에 DIB 내용).
  - **확인하지 못한 것:** 이번에 옮긴 함수는 x86 실행 대조가 없다(Win32 호출 함수는 지금의 제한 실행 환경으로 돌릴 수 없고, 입력 큐·사각형 자르기·패턴 반복자는 기대값을 아직 만들지 않았다). 회전된 영역은 실제 파일에 없어(2,119개 모두 방향 0) 회전 경로는 표와 코드를 읽은 해석뿐이다. 창으로의 `BitBlt` 결과는 자동 대조하지 않는다. 256색 화면에서의 팔레트 메시지, 창 메시지 처리(활성화 클릭·Alt·키 글자 판별)는 사람이 조작해 확인하지 않았다. CI의 Windows 작업은 돌려 보지 못했다.
- **알아둘 점**
  - `originalCD/`는 설치본이 아니라 CD 내용이라 `options.cfg`가 없다. `SCREENW`·`SCREENH`가 없으면 화면이 0 × 0이 되어 원본 assert `dibBuffer`와 같은 자리에서 멈춘다. `--set "SCREENW=800;SCREENH=600"`으로 준다. `--cd`는 인자 맨 끝에 둔다.
  - 아이콘과 로딩 그림은 게임 폴더의 원본 실행 파일을 **자료로 열어**(실행하지 않는다) 읽는다. 뮤텍스 `TitanicNetStormMutex`는 만들지 않는다. `options.cfg`는 **저장하지 않는다**(원본은 시작할 때 곧바로 다시 저장한다).
  - 작업 도중 사용자가 AGENTS.md를 직접 고쳤다(에이전트가 고친 것이 아니다). 그 뒤 문구가 한 번 더 다듬어졌다 — **지금의 정확한 문구와 그에 따른 작업 순서는 맨 위 인수인계 절에 있다.** dotnetpj 쪽 작업 순서에 영향을 주므로 다음 세션에서 참고한다.
- **(해결됨 — 2026-10-05 결정: `options.cfg`는 원본 게임과 동일하게 처리한다. 맨 위 인수인계 절 A.)** ~~사용자 결정이 필요한 것:~~ **설정 저장 위치.** 원본처럼 게임 폴더의 `options.cfg`에 쓸지(AGENTS.md가 그 파일의 수정을 허용한다. 다만 원본 게임과 설정을 함께 쓰게 된다), 따로 둘지. 그때까지 cpppj는 설정을 읽기만 한다.
- **남은 일 (다음 세션, [cpp-roadmap.md](docs/cpp-roadmap.md) "다음 작업 세션의 범위")**
  1. **Renderer**(`004994b0` 그리기, `00499fe0` 내보내기): 그리기 순서·바뀐 사각형·색 변환표·그림자 → 임시 연결점과 `InspectView`를 없앤다.
  2. 타입별 생성자 표와 Squid(오브젝트) 구조, 소유자 결정, 프레임 선택(기본·난수·방향). 섬 지형 만들기(`0046da70` 이후, Terrainbuilder).
  3. UserInput(`004d62b0`)의 화면 이동부터. 글꼴("init fonts" `004a3ce0`), 커서 모양, 소리.
  4. DirectDraw 전체화면과 설정 저장. (2026-10-05 정정: AGENTS.md의 목표 순서에 따라 1차 목표 단계에서는 원본대로 옮기고, 재실행 오류의 수정은 그 뒤 단계에서 한다. 설정 저장은 원본과 동일하게 — 맨 위 인수인계 절.)
  5. 입력 큐·사각형 자르기·패턴 반복자의 x86 기대값.

---

## 2026-10-05 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: cpppj 설정 계층·타입 표·요새 파일 읽기 복원

- [x] **요청:** "AGENTS.md, LEFT_JOBS.md 를 읽고, cpppj (기존 게임 디컴파일, C++ 복원) 더 진행." [후속 계획](docs/cpp-roadmap.md)의 1단계(설정·경로)와 2단계(맵 데이터·타입 연결)를 진행했다. 원본 게임 프로세스는 실행하지 않았다. AGENTS.md·원본 파일·C# 소스는 수정하지 않았다. **커밋하지 않았다.**
- [x] **설정 계층(1단계 완료)** — [근거·검증](docs/exe/cpp-config-reconstruction.md)
  - `o/Config.cpp`: 설정 객체 목록(`ConfigRegistry`), 이름 접두어, 조회(`00440760`), `{키|기본값}` 해석(`00440a00`), 치환(`00440b60`), 줄 삭제·값 쓰기(`00440240`·`004404e0`), 섹션(`0043fe60`·`00440570`·`00440640`), XOR 파일·명령줄 형식 읽기.
  - 새 `o/ConfigInterface.cpp`: 시작 순서(`[ARGS]` → 버전 → user·options·dev·guild·setup → `[END]`, InstallDir·CDDir), 경로 지정값(`local.1`~`3`), 정수·문자열 읽기, 언어 용어표, 저장 내용. 새 `client/Mission.cpp`: 미션 스크립트(`mission` 설정 객체)와 `loadFort`·`missionType`·`fortSpec` 경로(`00482fb0`).
  - **x86 대조:** 새 `tools/decomp_config_oracle.py` → `cpppj/tests/fixtures/config-x86.tsv`, `cpppj/recovery-config-evidence.json`. 치환 495·조회 77·줄 삭제 272·값 쓰기 143·섹션 검색 1,045·섹션 범위 408, 합계 **2,440개** 입력이 두 판본 기계어와 C++에서 일치. 실제 `options.cfg`·`setup.cfg` 버퍼의 모든 경로 지정값 포함. CRT(ASCII)·getenv·sprintf·assert·서식 이어 붙이기만 대체.
  - **원본 동작 정정:** 찾지 못한 키는 `{Not Found:키}`가 아니라 **`{키}`**로 남는다(원본이 `Not Found:` 위에 키를 덮어쓴다 — 두 판본 x86 실행). 인코딩된 설정 파일의 **첫 줄은 서명 `mQdsT`가 붙어 조회되지 않는다**(`options.cfg`의 `InstallDir`). 등록 순서는 용어표 → 전역 설정. CD판은 설정 객체 40개·버퍼 4KB(패치 1000개·8KB). [config.md](docs/formats/config.md)에 반영.
  - 실제 파일 검사: 새 `tools/cpp_config_smoke.py` — 두 판본의 설정 버퍼 전체를 Python이 원본 파일에서 직접 구성한 버퍼와 바이트 대조, 경로 지정값과 그 파일 읽기, 값 쓰기 → 저장 → `nscfg.py` 복호화 왕복, 원본 폴더 안 쓰기 거부, 원본 해시 유지.
- [x] **타입 표와 `.fort` 읽기(2단계의 파일 읽기 부분)** — [근거·검증](docs/exe/cpp-fort-reconstruction.md)
  - 새 `o/RiftTypeTable.cpp`: 전체 타입 번호 체계(패치 188개·CD 171개 — 내장 이름 5개, 프로세스 타입 53/52개, `.type` = 70 + 로딩 순서), 플래그 단어 48개 → 플래그 1·2, `group`·`level`·발자국·사용량 속성, 후처리(파생 플래그·목록 플래그·`foot_y` 6→8), 이름 해시.
  - 새 `o/Template.cpp`: 섹션 35개, `TypeNames` 변환표, `Chaff`·`TerrNN` 오브젝트 레코드(버전 0/1/2), 내용물, `Technology`, `Deck`, `Money`, `Subscriber`, `Territory` 원시 값. 형식 오류는 예외.
  - **새로 확인:** 기존 문서의 "내부 class 값"은 `.type`의 **`group` 속성**이고 battery·archer/cannon·blocker가 소유자 저장 비트를 켠다. 이름이 20글자 이상인 타입(`fakeThreeByThreeSurface`)은 이름이 설명 필드와 이어져 읽힌다(해시 `0534a54b`로 확인). 내장 타입 1·2·3·4·6의 이름. 버전 0은 모든 타입 뒤에 상태 1바이트(원본의 `|`/`&` 실수로 항상 참). 내용물의 목록 플래그 규칙. [fort.md](docs/formats/fort.md)에 반영.
  - 실제 파일 검사: 새 `tools/cpp_fort_smoke.py` — 패치판 폴더 **465개**(오브젝트 313,712개)·CD판 **23개**(6,922개)의 구조 전체가 기존 `tools/fort.py`의 결과와 줄 단위로 일치(326,169줄). C++ 타입 표의 해시가 실제 `TypeNames`와 완전히 같은 파일: 패치 표 29개, CD 표 424개. TEST01·캠페인 1-1(`thewarbegins`)·`savetheisland`의 미션 머리 값 일치.
- [x] **검사 명령:** `--config-dump`·`--config-get`·`--config-spec`·`--config-save`, `--dump-types`, `--inspect-fort`·`--dump-forts`, `--inspect-mission`. 자산 명령은 팔레트 경로를 설정(`GamePalSpec` + `battlePal`)에서 계산한다.
- **검증(이 PC):** Windows Release 빌드 경고·오류 0, CTest **30개 테스트**(x86 기대값 1,806 + 709 + 2,440 = **4,955개**). 실제 자료 검사 4종(`cpp_recovery_smoke`·`cpp_assets_smoke`·`cpp_config_smoke`·`cpp_fort_smoke`) 통과. WSL(GCC 15.2)에서 `-Wall -Wextra -Wpedantic` 경고 없이 직접 컴파일해 30개 테스트 통과. `cpppj/SOURCE_MAP.md` 재생성(파일 있음 13개).
  - **확인하지 못한 것:** 타입 플래그와 `.fort` 읽기는 x86 실행으로 대조하지 않았다(정적 대조 + 독립 판독기 전수 비교 + 실제 해시). 그룹·후처리가 켜는 플래그 비트는 독립 비교 대상이 없다. Linux의 CMake 빌드와 CI는 돌려 보지 못했다.
- **남은 일 (다음 세션, [cpp-roadmap.md](docs/cpp-roadmap.md) "다음 작업 세션의 범위")**
  1. ~~**영역 배치:** `Territory` 120바이트와 `TerrNN` 청크의 월드 위치(`004be2c0`·`0046f200`, [territory-layout.md](docs/exe/territory-layout.md))를 옮겨 TEST01의 모든 오브젝트에 월드 칸 좌표를 붙인다.~~ — 2026-10-05 완료(위 절).
  2. ~~**플랫폼 결정:** 창·그리기·입력·소리·글꼴 API와 의존성, 클론 설정의 저장 위치를 정해 문서로 남긴다(3단계 선행 조건).~~ — 2026-10-05 Win32 직접 호출로 결정(위 절). 설정 저장 위치와 소리·글꼴 방식은 남았다.
  3. 타입별 생성자 표·수치 필드, Squid(오브젝트) 구조, 프레임 선택 → 1024×768 TEST01 정적 창. — 창과 임시 검사용 화면까지 완료(위 절). Renderer·오브젝트 생성은 남았다.
- **dotnetpj 쪽에 남기는 메모(이번에 고치지 않음):** `ConfigStore`는 찾지 못한 키를 `{Not Found:키}`로 남기고 `{@absent.name}`도 `{Not Found:@ABSENT.NAME}`으로 만든다. 원본은 각각 `{키}`, `{@ABSENT}`('.'에서 잘린 키)다. 이 표시가 화면에 보이는 경로가 있는지 확인한 뒤 맞출지 정한다(관련 테스트 `TextResourceTests`·`GameResourcesTests`).

---

## 2026-10-05 ✅ 완료: cpppj 후속 복원 계획 문서화

- [x] **요청:** "cpppj 뒤의 계획은 문서로 남겨줘". [cpppj 후속 복원 계획](docs/cpp-roadmap.md)에 현재 구현 범위, 단계별 우선순위·의존 관계·완료 기준, CD판 대조·기계어 검증 방법과 다음 작업을 정리했다. 이번 작업은 문서만 변경했으며 향후 구현을 완료 처리하지 않았다.
- [x] **문서 연결:** cpppj README·C++ 빌드 규칙·타입/그래픽 복원 문서에서 후속 계획으로 연결했다. 기존 복원·검증 기록은 보존했다.
- [x] **다음 구현 1:** Config 객체 층·치환·자산 경로. 함수의 패치/CD 대응·호출 규약과 실제 설정 검사 사례부터 정리한다. — 2026-10-05 완료(위 절).
- [x] **다음 구현 2:** `.fort`·타입 해시/번호·게임 플래그 연결. TEST01·캠페인 1-1의 배치·소유자·자원·덱을 검사 보고서로 남긴다. — 2026-10-05 파일 읽기·타입 번호·플래그·검사 보고서 완료(위 절). 영역 배치에 따른 월드 좌표와 소유자 결정은 남았다.
- [ ] **다음 구현 3:** 플랫폼 API·의존성을 결정하고 1024×768 TEST01 정적 창을 만든다. 이후 SID·파생 프로세스·입력·메인 루프를 연결하여 캠페인 1-1 플레이로 이어 간다. — 2026-10-05 부분 완료: 플랫폼 결정(Win32), 원본 방식의 창·화면 장치(창 모드)·입력 큐·메인 루프의 뼈대, TEST01 오브젝트를 놓는 임시 검사용 화면(맨 위 절). Renderer·오브젝트 생성·UserInput이 남아 체크하지 않는다.
- **그 이후:** 원본 기능·튜토리얼·나머지 캠페인·메뉴·오디오·저장, 영어/한국어·전체화면·화면비·원본 속도 이후 60/120프레임, MCP·Windows 실행 패키지. 상세 작업과 완료 기준은 위 계획 문서를 따른다.

---

## 2026-10-05 ✅ 완료: cpppj 타입·그래픽 자산 복원 후속

- [x] **요청:** "cpppj (기존 게임 디컴파일 및 복원) 더 진행". AGENTS.md·LEFT_JOBS.md를 확인하고 `.type`·SHP·팔레트·기본 8비트 합성을 복원했다. 원본 게임 프로세스는 실행하지 않았다. AGENTS.md·원본 파일·C# 소스는 수정하지 않았다. [근거·재현·남은 범위](docs/exe/cpp-assets-reconstruction.md).
- [x] **타입 자산:** `o/TypeParser.cpp`·`TypeLoadOrder.cpp`·`RiftType.h`. 숫자/문자열 속성, 생성자 수식어·플래그 이름, 클러스터·GIF 참조, 원본 프레임 코드와 default/help/gump/base 인덱스, footprint 보정. 전체 게임 생성자·플래그 비트·ID 연결 완료를 뜻하지 않는다.
- [x] **그래픽 소스:** `client/VFXDraw`의 블록 상대·공유 오프셋, 프레임 헤더·RLE·투명 마스크·pane 원점·클리핑. `client/Screen`의 COL/RGBQUAD 팔레트, 신규 `GameAssets`·`platform/Bitmap` 연결과 BMP V4 출력. 번호 0·255도 불투명 색이므로 색과 투명을 분리한다. 특수 레코드는 이미지로 처리하지 않는다.
- [x] **CLI:** `--inspect-assets <dir> [--cd]`, 검사 전용 `--dump-assets`, `--export-frame <dir> <type> <cluster> <layer> <out.bmp> [--cd]`. 잘못된 판본의 블록 개수는 실패로 보고한다. `dude`의 typename `Man` 별명과 본체/그림자 레이어도 검사한다.
- [x] **판본 차이:** 자산 타입 패치 **116개**·CD **101개**. 패치 `manabolt` 정의 4개·SHP 8개, CD 4개·4개를 각각 보존한다. Sun Cannon `hpPerSec`는 패치 **16**·CD **14**다. 원본 RiftType은 **500/468바이트**, foot_x/y 오프셋도 다르며 `+0x114/+0x124` 프레임 필드는 공통이다. `ApplyRecoveredTypes.java`의 CD 자료형을 468로 정정하고 두 판본 읽기 전용 디컴파일 각 5개를 재생성했다.
- [x] **VFX 기계어:** `tools/decomp_graphics_oracle.py`, `cpppj/recovery-graphics-evidence.json`, Git 포함 `graphics-x86.tsv`. 원본 `00401d92/004021b5` ↔ CD `00465772/00465b95`, ret 20인 두 함수만 flat x86 세그먼트로 실행한다. CRT·assert 대체 없이 **709개** 입력의 반환값·화면 전체 바이트가 두 판본과 C++에서 일치. 기본 8비트 합성만 검증했다.
- [x] **검증:** Release 경고·오류 0, CTest **17개 테스트**(기존 x86 1,806개＋VFX 709개 = **2,515개** 기대값). `tools/cpp_assets_smoke.py`로 타입 **217개**, 일반 이미지 **6,777개**의 **15,940,472픽셀**과 특수 레코드 173개 헤더·전체 타입 속성/프레임 코드를 기존 Python 판독기와 대조했다. 검토용 BMP **8개**를 Pillow로 재판독하여 RGBA·알파·방향까지 확인한다. 원본 해시 유지. 결과는 `extracted/cpp-assets-smoke/report.json`·이미지에 있다.
- **남은 일:** Config 객체 스택·치환·저장과 동적 팔레트 선택, `.fort` 로더, 게임 타입 비트·생성자·전역 ID·float 변환·난수 프레임 선택·방향 폴백, BaseProcess SID·실제 파생 클래스, Screen 장치·Renderer·UserInput·전체 루프. 색 변환표·실제 그림자 합성·확대·반전·폰트·오디오·전투·MCP도 미구현이다. 한국어 글꼴·와이드 화면·전체화면·60/120프레임은 해당 계층 복원 후 적용한다. `.type` 자산 읽기와 SHP 기본 압축 해제는 완료했으며 전체 게임 복원은 진행 중이다.

---

## 2026-10-05 ✅ 완료: CD판 대조·기계어 검증과 cpppj 공용 계층 1차 복원

- [x] **요청:** AGENTS.md·LEFT_JOBS.md를 읽고 디컴파일 신뢰도를 더 높이는 방법을 찾은 뒤 CD판을 참고하여 cpppj C++ 소스 생성. 원본 게임 프로세스는 실행하지 않았으며 AGENTS.md·원본 파일은 수정하지 않았다. 전체 게임 복원 완료를 뜻하지 않는다. [상세 근거·재현·범위](docs/exe/cpp-reconstruction.md).
- [x] **오대응 수정:** 패치 `0049a9e0` ↔ CD `00444350`은 마스크 검색과 번호 검색의 다른 오버로드였다. 실제 대응은 각각 `00444410`, `0049a9a0`이다. 인자·`ret N`·본문 및 x86 결과로 확인한 5쌍을 `cpppj/recovery-manifest.json`에 기록하고 자동 매칭에 우선 적용한다. 실행 파일·검토 목록 SHA-256이 다르면 재검증하도록 한다.
- [x] **기계어 차등 검사:** `tools/decomp_oracle.py`, 선택적 의존성 `tools/requirements-decomp-oracle.txt`, Git 포함 기대값 `cpppj/tests/fixtures/original-x86.tsv`, 검증 기록 `cpppj/recovery-evidence.json`. 격리된 Unicorn x86 메모리에서 선택 함수만 에뮬레이션한다. 프레임 검색 3종 각 512개·설정 270개, 총 **1,806개**가 두 판본 및 C++에서 일치. CRT는 ASCII 비교·toupper, assert는 오류 보고만 대체하므로 원본 UI·전체 실행 검증은 아니다.
- [x] **선택 자료형 복원:** `tools/ghidra/ApplyRecoveredTypes.java`, `recover_types.ps1`. 확인된 필드·ECX/ESP 인자·반환형만 적용해 각 판본 `typed-core.c` 5개 함수를 생성한다. 정밀 프로젝트는 읽기 전용이고 수정 내용을 저장하지 않는다. `object->frameCodes`, `object->frameCount`로 읽힌다.
- [x] **컴파일되는 C++ 소스:** `cpppj/src/o/`의 `BaseFile`(TAFF/XOR/읽기 VFS)·`Config`(원시 파서)·`Xlat`·`RiftType`(검색)·`BaseProcess`(실행 인터페이스)·`Kernel`·`GameClock`·신규 `OriginalText`, `platform/Console`, `app/main.cpp` 검사 명령. 소스마다 출처·복원 범위를 적었다. UTF-8·한국어 주석을 사용하고 C# 빌드는 수정하지 않았다.
- [x] **판본 차이 반영:** 커널은 패치 **39,999칸**, CD **3,999칸**이다. 기존 메인 루프 문서의 동일 용량 가정을 정정하고 C++에 패치 값을 적용했다. 설정 원시 값 assert 경계도 8,191/4,095로 다르다. CD `NETSTORM.VER`의 `10.37` 표기만으로 실행 파일 판본을 재판정하지 않았다.
- [x] **검증:** Windows Release 빌드 오류·경고 0, CTest 내부 **10개 테스트** 통과. `tools/cpp_recovery_smoke.py`로 패치 **246개**·CD **258개**, 합계 **504개 엔트리** 전체 바이트 대조, 실제 번역 각 20개·고유 키 774/759개·임시 디스크 우선 조회 확인. 아카이브 SHA-256 유지. 자동 대응 **2,495쌍**; 수동 정답을 제외한 기존 홀드아웃 **195/198(98.5%)** 유지.
- **남은 일(후속 갱신):** `.type` 자산 읽기·SHP·팔레트·기본 합성은 위 후속 작업에서 완료했다. Config 객체 스택·치환·저장, `.fort` 로더, 게임 타입·BaseProcess SID 연결·파생 프로세스, Screen 장치/Renderer/UserInput·전체 루프·전투·오디오·MCP는 남아 있다. 한국어 글꼴·와이드 화면·전체화면과 정확한 60/120프레임은 해당 계층 복원 후 적용한다. 커널·시계는 정적 대조와 단위 검사이며 x86 기대값 대상 확장은 후속이다. Linux 제품 지원은 C++ 목표가 아니다.

---

## 2026-10-05 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: C# 프로젝트를 `dotnetpj/` 로 이동 · C++ 빌드 `cpppj/` 기초 구조

- **공통 구성:** analyzeManager는 C#으로 작성됐지만 원본 분석 도구다. 공용 assets/fonts/docs/tools는 루트에 남았으며 분석기의 Netstorm.Assets 참조와 루트/하위 SDK 선택에 주의한다. 당시 C++ Release configure/build·WSL g++ hello-world는 통과했고 WSL CMake/Ninja·CI는 미확인이었다. 최신 cpppj 결과는 문서 맨 위를 따른다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-031)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- [x] **요청:** "C#+Mono 빌드와 C++ 빌드를 병행 개발한다. 루트의 C#+Mono 프로젝트를 `dotnetpj` 안으로 옮긴다(닷넷 프로젝트 관련 파일만). `cpppj` 에는 기존 게임을 디컴파일한 소스를 토대로 만드는 C++ 빌드가 들어간다 — 문서에 반영하고 기초 프로젝트 구조만 만든다." 원본 게임은 실행하지 않았다. **커밋하지 않았다.**
- [x] **C++ 빌드 기초 구조(`cpppj/`):** CMake 3.21 이상·C++20, 외부 라이브러리 없음. 폴더는 원본 exe 의 assert 문자열에 남은 원본 소스 트리를 따른다 — `src/o/`(원본 `\Ns\O\`), `src/client/`(원본 클라이언트 폴더), `src/zacket/`(원본 `\Ns\Zacket\`), 새로 쓰는 `src/platform/`·`src/app/`, `tests/`. 정적 라이브러리 `netstorm` + 실행 파일 `NetstormCpp` + 테스트 `netstorm_tests`(ctest). 프리셋 `vs2022`·`ninja`.
  - 들어 있는 코드는 주석 규칙의 견본 하나뿐이다: `src/client/ClientMain.cpp` 의 `FrameIntervalSeconds`(원본 `FUN_00435220` 의 `maxFPS` 기본 75 → 간격 `1.0 / maxFPS`). 실행 파일은 빌드 정보만 출력한다. **창·게임 로직은 없다.**
  - `tools/cpp_source_map.py` → [cpppj/SOURCE_MAP.md](cpppj/SOURCE_MAP.md): 두 판본 exe 의 assert 문자열에서 원본 소스 파일 **136개**(공용 75·클라이언트 60·Zacket 1)를 모아 cpppj 경로와 짝지은 표. CD판에 원래 대소문자(`ClientMain.cpp`·`RiftType.cpp`)가 남아 있어 그 표기를 파일 이름으로 쓴다.
- [x] **문서:** 새 [docs/cpp-build.md](docs/cpp-build.md)(두 빌드의 관계, 폴더 구조, 빌드 방법, 디컴파일 결과에서 C++ 소스를 만드는 절차, 출처 주석·32비트→64비트 규칙, 정하지 않은 것), `cpppj/README.md`, `README.md`·`README.en.md`("프로젝트 구성" 절과 C++ 빌드 명령), 이 문서의 2·3절, `PREPARE.ps1`·`PREPARE.sh`(C++ 도구 설명을 "C# 확정으로 현재 불필요"에서 "C++ 빌드 cpppj/ 용"으로, 컴파일러·CMake 는 권장 항목으로 — 기본 선택 여부는 그대로), `.gitignore`(`/cpppj/build/`), CI(`dotnet-build-test`·`cpp-build-test` 두 작업).
- **남은 것 (C++ 빌드)** — [cpp-build.md](docs/cpp-build.md) 6절
  1. 플랫폼 계층의 라이브러리(창·화면·입력·소리)와 외부 라이브러리를 가져오는 방법을 정한다.
  2. 옮기는 순서를 정한다. 제안: `o/` 의 파일·설정 계층(BaseFile·Config·Xlat) → 데이터 형식(Template·RiftType) → 프로세스 커널과 시계(Kernel·BaseProcess) → 화면(Screen·Renderer) → 메인 루프와 입력(ClientMain·UserInput).
  3. 한국어 글꼴 그리기, 원본에 없는 기능(와이드 화면·60/120프레임·전체화면 재실행 오류 수정)을 다시 만든 코드에 넣는 방식.

---

## 2026-10-05 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: 클론 화면 루프를 원본 수준의 프레임으로 (디컴파일 결과 적용)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-032)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-05 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: 메인 루프와 시간 모델 분석 (패치판·CD판 대조)

- **공통 설계 근거:** 위치/애니메이션을 시간으로 계산하고 애니메이션 속도를 화면 주사율과 분리한다. UI는 벽시계, 게임 규칙은 게임 시계를 기준으로 한다. 화면 입력은 프레임마다 처리하되 규칙 명령은 시뮬레이션 시점에 적용하고 프레임당 값을 원본 기준의 초당 값으로 환산한다. 당시 24Hz 보간과 원본 수준 적용 완료는 C# 기록이다.

> **공통/이관 범위:** 원본 관찰·분석 도구는 두 프로젝트가 참고한다. [당시 C# 구현·검증·과제](LEFT_JOBS.dotnetpj.md#legacy-dotnet-033)는 dotnetpj 이력으로 이관했으며 cpppj 완료 기록이 아니다.

- [x] **요청:** 정밀 디컴파일의 "다음 단계"(메인 프레임 함수를 CD판과 대조해 입력·갱신·그리기 순서 정리) 진행. 원본 게임은 실행하지 않았다. 클론 코드는 고치지 않았다.
- [x] **결과 문서:** [docs/exe/main-loop.md](docs/exe/main-loop.md) — 루프 한 바퀴의 호출 순서 표(패치판·CD판 함수 대응 포함), 시각 고정, 프레임 제한, 입력·명령 처리, 프로세스 커널, 그리기, 클론에 주는 의미.
- **확정한 것**
  1. 루프(`WinMain` = `FUN_00438dc0`)는 고정 틱이 아니라 **프레임마다 한 번** 돈다: 시각 고정(`FUN_00460e90`) → 네트워크 → Windows 메시지 → 입력·명령(`FUN_004d62b0`, Userinput.cpp) → 갱신 목록 → 프레임 제한·그리기(`FUN_00436450`).
  2. **모든 오브젝트 로직은 프로세스 커널(`FUN_00471a30`, Kernel.cpp)에서 돈다.** 39,999칸 표의 프로세스마다 실행 메서드(가상 함수 표 `+0x18`)를 매 프레임 한 번 부른다.
  3. **이동은 절대 시각 보간이다.** 지상 이동(`FUN_0048a330`, Pathprocess)은 꼭짓점마다 도착 시각이 미리 계산되어 있고, 미사일(`FUN_00482340`)도 출발·도착 시각으로 위치를 구한다. 프레임 속도와 무관하다. ([animation-timing.md](docs/videos/animation-timing.md)의 "추가 확인 필요"를 해소)
  4. 애니메이션·주기 작업은 "다음 시각 = 지금 + 간격" 타이머라 **원본에서는 루프 속도에 따라 실제 주기가 달라진다**(0.04초 타이머: 약 71~75fps 에서 24Hz, 60fps 20Hz, 30fps 15Hz — 계산값).
  5. 프레임 제한은 `maxFPS`(기본 75)이고 **바쁜 대기**다. 게임 시계가 정지된 동안에는 제한하지 않는다. `timeBeginPeriod`를 쓰지 않아 시계 눈금은 시스템 타이머 해상도를 따른다.
  6. 1/30초 고정 간격 적분(`FUN_00418640`, Animator3d)은 멀티플레이 대기 화면의 섬 이동(Ch.cpp)에만 쓰인다. 프레임 간 시간차를 곱하는 적분은 `FUN_00467c80`(Gunanim) 하나뿐이다.
  7. 두 판본의 루프 구조·`maxFPS` 기본값·1/30초 상수가 모두 같다.
- **남은 것**
  2. 렌더러 `FUN_004994b0` 내부의 그리기 순서, `FUN_004c2070`의 정체, 타이머 방식 프로세스 각각의 간격 상수(유닛 걷기·공격·건설 등)는 분석하지 않았다. 실행 메서드는 `extracted/refined/originals/vtables.tsv`의 슬롯 6 에서 찾는다.
  3. 실제 루프 주기(64fps 또는 71fps 추정)는 원본을 실행해 재지 않았다.

---

## 2026-10-05 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: 정밀 디컴파일 — 신뢰도 높이기, CD판 대조

- [x] **요청:** "기존 게임 디컴파일 더 신뢰도 높이는 방법을 찾아 진행. CD쪽 디컴파일 소스를 같이 참고해 모호성을 해결할 수 있을 수도." 원본 게임은 실행하지 않았고 원본 파일은 읽기만 했다.
- [x] **결과물:** `extracted/refined/originals/Netstorm.c`(패치판 5,549개 함수), `extracted/refined/originalCD/NETSTORM.c`(CD판 5,823개 함수), `extracted/refined/match.tsv`(패치판↔CD판 2,493쌍). 함수마다 머리말에 신뢰도 등급(A~D)·호출자·포인터·상대 판본의 짝·호출 규약 근거·함수 포인터 표 위치가 붙는다. **방법·검증·읽는 법: [docs/exe/decompile-reliability.md](docs/exe/decompile-reliability.md).**
- **재현(다른 PC, 약 20분):** `powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1`. 결과는 `extracted/refined/`(Git 제외). 기본 디컴파일(`extracted/decomp/Netstorm.c`)은 기존 문서의 줄 번호 인용 때문에 그대로 둔다 — 함수 주소는 두 결과에서 같다.
- **두 판본 나란히 보기:** `python tools/decomp_refine.py --show 4d62b0` (패치판 주소), `--show 41ae80 --edition originalCD` (CD판 주소). 한쪽이 모호하면 다른 쪽을 대조한다. 메인 프레임 함수는 패치판 `FUN_004d62b0` ↔ CD판 `FUN_0041ae80`.
- **한 일**
  1. `switch` 분기 블록을 함수 몸체에 포함(`FixSwitches.java`) — 메인 프레임 함수 안의 가짜 함수 26개가 사라졌다.
  2. 누락 함수 복구를 프로젝트에 확정(`RecoverMissing.java`): 패치판 +1,031개, CD판 +2,065개. `mov edi, edi`(`8B FF`)를 프롤로그가 아니라 정렬 패딩으로 처리해, CD판 함수 451개가 진입점보다 앞에서 만들어지던 오류를 없앴다.
  3. 근거 기반 호출 규약(`ApplyConventions.java`): 패치판 `unknown` 4,211개 → 46개. 인자 없이 적히던 호출 지점이 34.7% → 18.3%(`this` 가 보인다).
  4. 패치판↔CD판 함수 대응(`tools/decomp_refine.py`): 이름 → 유일 문자열 → 호출 순서·호출자 전파 → 모듈 내 국소 구간, 문맥 불일치 취소.
  5. 상대 판본 근거로 호출 규약 보완(패치판 8개, CD판 9개), 함수 포인터 표 색인(`vtables.tsv`), 등급.
- **검증:** 대응 홀드아웃 정밀도 **98.5%**(숨긴 앵커 298개 중 198개 재발견, 195개 정답), 짝지어진 함수의 호출 규약 일치 **97.2%**, `ret N` 일치 **97.9%**. 등급은 패치판 A 2,322·B 3,156·C 40·D 31, CD판 A 2,311·B 2,943·C 484·D 85.
- **버린 방법(다시 시도하지 말 것):** (1) Ghidra "Decompiler Parameter ID" 로 원형 일괄 확정 — `__fastcall` 901개 오탐, 가변 인자 잘림, `extraout_` 있는 함수 38→311. (2) 점프 테이블 재정의(`JumpTable.writeOverride`) — `case` 값이 0부터 매겨진다. (3) 전체 주소 순서로 짝짓기 — 두 판본은 모듈 링크 순서가 다르다(모듈 안의 순서만 유지). 근거는 위 문서 6절.
- **정정:** 2026-10-04 절은 `FUN_004d62b0` 안의 가짜 함수를 "오탐"으로만 적었다. 원인은 Ghidra 리스팅의 함수 몸체에서 `switch` 분기 블록이 빠진 것이고, **디컴파일 결과에는 처음부터 `switch` 3개·`case` 48개가 다 들어 있었다**(디컴파일러가 점프 테이블을 스스로 복구한다).
- **삭제:** `tools/ghidra/recover_missing.ps1`, `tools/ghidra/merge_decomp.py` 와 그 산출물(`Netstorm.all.c`, `decomp-at/*-missing.*`, `*-gaps.tsv`). 정밀 파이프라인이 대체한다. 아래 2026-10-04 절의 사용법은 더 이상 유효하지 않다.
- **남은 것**
  1. 정밀 디컴파일을 써서 실제 분석을 이어 간다. 후보: ~~(a) 메인 프레임 함수 `FUN_004d62b0`↔`FUN_0041ae80` 대조로 입력·갱신·그리기 순서 정리~~ → ✅ 완료(위 절, [main-loop.md](docs/exe/main-loop.md)), (b) `vtables.tsv` 로 클래스별 가상 함수 표를 정리해 간접 호출 해석, (c) 기존 문서에서 "디컴파일에 없음"으로 남긴 지점 재확인.
  2. 대응률이 45% 안팎이다. 짝이 없는 함수가 필요하면 `--show` 로 주변 함수의 짝을 보고 CD판에서 같은 모듈 구간을 직접 찾는다.
  3. 자료형·구조체·함수 이름은 복구하지 않았다(RTTI·심볼 없음).
  4. Linux 용 실행 스크립트 없음(`refine_all.ps1`·`refine_decomp.ps1`·`run_script.ps1` 은 PowerShell 전용). Ghidra 스크립트와 `decomp_refine.py` 는 운영체제와 무관하다.

---

## 2026-10-04 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: 타임아웃 확대 재디컴파일 · 누락 함수 일괄 복구

- [x] **요청:** "디컴파일 더 가능한 부분 진행, 일단 타임아웃을 대폭 늘려 전체 디컴파일 다시". 원본 파일은 읽기만 했다.
- [x] **타임아웃 3600초:** `ExportDecomp.java`(60→3600초)·`DecompileAt.java`(120→3600초). 패치판 전체 디컴파일을 다시 돌려 **성공 4,506·실패 0**(5분 14초). `FUN_004d62b0`(메인 프레임 함수)이 `Netstorm.c` 141,241~143,296행(약 2,056줄)에 복구되었다. 앞부분 1~141,241행은 이전과 동일, 이후 행은 2,053줄 밀렸다(문서 인용 줄 번호는 모두 그 앞이라 영향 없음). 이전 `Netstorm.c`는 같은 주소·내용이며 해당 함수 한 개만 추가된 셈이다. CD판은 기존에도 실패 0이라 재실행하지 않았다.
- [x] **누락 함수 일괄 복구 도구:** 새 `tools/ghidra/RecoverMissing.java` + `recover_missing.ps1` + `merge_decomp.py`. 후보(패딩 뒤 프롤로그·함수 끝 직후·`ret` 직후·데이터 구간 코드 포인터)를 모아 함수로 만들고 디컴파일, 후보가 안 나올 때까지 반복한다. 결과: 패치판 **+1,093개**(합친 파일 5,573개), CD판 **+2,410개**(합친 파일 6,121개). 함수 밖 코드 구간은 패치판 46,319→24,219바이트, CD판 219,956→42,450바이트. 상세·출처별 신뢰도·알려진 오탐: [mission-header-flags.md §7.1](docs/exe/mission-header-flags.md).
- ~~**사용법(다른 PC):**~~ (2026-10-05: 아래 도구는 정밀 디컴파일로 대체되어 삭제됨 — 위 절 참고) `run_decomp.ps1`(두 판본) → `recover_missing.ps1`(두 판본) → `python tools/ghidra/merge_decomp.py [--edition originalCD]`. 결과는 `extracted/decomp-at/*-missing.{c,tsv}`, `*-gaps.tsv`, `extracted/decomp/Netstorm.all.c`, `extracted/originalCD/decomp/NETSTORM.all.c`(모두 Git 제외).
- **주의:** (1) `afterret` 출처 복구 함수는 다른 함수의 꼬리 조각이 섞여 신뢰도가 낮다. (2) 메인 프레임 함수 안 switch 블록 26개(`0x4d6cfe`~`0x4da53c`)가 가짜 함수로 복구되어 `merge_decomp.py`가 패치판에서 제외한다. (3) CD판 복구 함수 2,410개는 개별 검증 전이다. (4) 이전 인수인계의 "누락 함수 후보 528개"는 이 도구로 대체됐다.
- **다음 후보(2026-10-05: 위 절의 "남은 것"으로 이어짐, `Netstorm.all.c` 대신 `extracted/refined/originals/Netstorm.c` 를 쓴다):** (1) `pointer` 출처 복구 함수(가상 함수 표 슬롯)를 `Netstorm.all.c`에서 읽어 미해결 클래스 동작 확인(예: 슬롯 이름이 필요한 `Tutorial`/`Normal` 외 클래스), (2) 이전 분석 문서에서 "디컴파일에 없음"으로 남긴 지점 재확인, (3) `FUN_004d62b0` 전체(2,056줄)를 읽어 개발자 Pause 외 메인 루프 처리(입력·갱신 순서) 정리 — 30/60/120프레임 구현의 갱신 구조 근거가 된다, (4) CD판 `FUN_004735a0`(1,180줄) 등 큰 복구 함수와 패치판 비교.

---

## 2026-10-04 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: 두 판본 전체 재디컴파일

- [x] **요청:** "기존 게임 디컴파일 다시 진행". `tools/ghidra/run_decomp.ps1`(Ghidra 12.1.4, JDK 25)로 패치판·CD판을 순차 재실행했다. 원본 파일은 읽기만 했다.
- **패치판** `originals/Netstorm.exe`(SHA-256 `a305414c…`): 소요 5분 15초, 성공 4,505·실패 1. 결과 `extracted/decomp/Netstorm.c`(4,883,052바이트)의 SHA-256 `991E4184…`는 **재실행 전과 동일**하다. (실패 1건은 처음에 `0x40c6c0`으로 적었으나 **틀렸고**, 메인 프레임 함수 `FUN_004d62b0`의 60초 타임아웃이었다. `0x40c6c0`의 pcode 경고는 디컴파일이 끝난 함수의 경고다. 아래 절에서 해결.)
- **CD판** `originalCD/NETSTORM.EXE`: 소요 4분 36초, 성공 3,711·실패 0. 결과 `extracted/originalCD/decomp/NETSTORM.c`(4,164,124바이트)의 SHA-256 `F6FECC58…`도 **재실행 전과 동일**하다.
- 기존 Ghidra 프로젝트 파일(`*.gpr`)이 0바이트였던 문제는 `-overwrite` 재생성으로 해소됐다. `decompile_at.ps1`이 재생성한 프로젝트에서 정상 동작함을 확인했다(`4c2b20`·`4b1e80`).
- 로그의 `Invalid GIF data`·`WEVTResource`·`ExportDataDirectory` 경고는 리소스·DLL 자동 해석 경고로 디컴파일과 무관하다.
- 결과가 같으므로 기존 분석 문서의 줄 번호·주소는 그대로 유효하다. 남은 디컴파일 과제(누락 함수 후보 528개, `FUN_004d62b0` 전체 디컴파일 타임아웃)는 변하지 않았다.

---

## 2026-10-04 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: `extracted/` 중복 미디어 정리

- [x] **요청:** AGENTS.md·LEFT_JOBS.md를 읽고 `extracted/` 안에서 더 이상 불필요한 이미지·사운드·동영상 정리. `extracted/`는 Git 제외이므로 저장소에는 영향이 없다.
- [x] **삭제(8,352개, 약 4.2GB, I: 여유 9.8GB → 15.2GB):** ① `extracted/analyzeManager/<세션>/game/` 안 미디어(`.mus`·`.wav`·`.gif`) 29개 세션분 — 세션마다 `originals/`를 복사한 것이며 파일 이름·크기가 모두 같고(다른 것은 `setup.cfg`·`options.cfg`·`battle1.fort`뿐) 표본 33개는 SHA-1도 일치했다. ② 클론 검사 스크립트가 `assets/game-data`에서 매번 다시 만드는 `data/` 복사본 안 미디어 — `extracted/fangame-combat-20261003/smoke/data`, `extracted/crossbow-defense-20261003/smoke/data`, `extracted/screens/test02-implementation/combat-regression/data`. 삭제 목록은 이 세션 스크래치패드에만 있다.
- **지우지 않은 것(사용자 결정):** 세션별 원본 녹화 `recording/video-*.avi`·`audio-*.wav` 약 12.9GB(TEST02 S1 5.8GB·S4 2.8GB, 메뉴 버튼 5개 세션 2.2GB, 캠페인 1-1·실패 세션 등). 사용자가 "녹화는 하나도 안 지움"을 골랐다. 지우면 원본을 다시 실행해 녹화해야 하므로 나중에 지울 때도 사용자 확인을 받는다. 클론 검사 PNG(약 1GB)는 원본에서 뽑은 프레임(`orig\`·`frames\`·`cmp-*`)과 섞여 있고 문서가 인용하므로 남겼다.
- **영향:** 위 세션 폴더의 `game/`은 더 이상 실행 가능한 복사본이 아니다(이미 종료된 세션이며 새 세션은 매번 새로 복사한다). 도구는 `originals/sound`를 쓰고 세션 복사본을 쓰지 않아 영향이 없다. 검사 스크립트는 실행할 때 `data/`를 다시 만든다. AGENTS.md·originals·originalCD는 수정하지 않았고 커밋은 하지 않았다.

## 2026-10-04 (Windows, 원본 실행 없음 — 기존 분석·exe 정적 대조) ✅ 완료: TEST02 분석 클론 반영

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-038)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-04 (`HJOW-Athlon`, Windows 10, 원본 자동 분석 실행 있음 — 중간에 사용자 요청으로 추가 시험 중단) ✅ 완료: TEST02 사제 건설 분석

> **공통/이관 범위:** 원본 관찰·분석 도구는 두 프로젝트가 참고한다. [당시 C# 구현·검증·과제](LEFT_JOBS.dotnetpj.md#legacy-dotnet-039)는 dotnetpj 이력으로 이관했으며 cpppj 완료 기록이 아니다.

- [x] **요청:** Edit 로 TEST02 불러오기 → 사제로 템플·워크샵 건설 후 종료. 섬 안 건설 한도, 사제 클릭·이동·건설 때 마우스 커서, 건설 효과음, 건설 명령 직후 사제 이동, 건물이 드러난 뒤 사제 이동. 디스크 약 19GB 허용.
- [x] **결과(전부 [분석 노트](docs/videos/auto-test02-construct-20261004.md)):** ① TEST02는 Edit 목록(앞 50개)에 없어 **Create New Map 에 `TEST02` 입력**으로 연다 → Esc → Game → Test Battle → Go!(SP 500000). ② **템플 1기**(건설 중 공사장만 있어도 `Temple >` 줄이 어두워짐), 비용 5000. ③ **워크샵 개수 상한 없음** — 한 섬에 14채 거부 없이 지음, **7×8칸 발자국이 전부 땅 위·다른 건물과 안 겹칠 때만** 가능, 맞닿아도 됨, 설치 미리보기는 칸에 붙어 흰색=가능·붉은색=불가. ④ **커서 5종**(화살표·⊘·얇은 ×·안쪽 화살표 4개·굵은 ×)을 exe `RT_CURSOR` 리소스와 픽셀 대조로 확정. ⑤ **건설 명령 직후 사제 이동 = 취소+전액 환불**(그림자 0.2~0.9초 뒤 사라짐·환불 숫자 약 0.85초 뒤부터 굴러 오름). ⑥ **건물이 드러난 뒤 이동 = 건설 계속**(도착 후 정확히 10.0초, 템플·워크샵 모두). ⑦ 소리: 설치 `dropPiece-500`, 진행 `jimbuild` 1.14초 반복, 완공 `templeComplete`/`workshopComplete`, 메뉴 `openGump`/`openSubGump`, 선택 `select`, 이동 `priestmove1`/`priestMove3`, 취소·환불은 소리 없음.
- [x] **도구 추가(커밋 전):** `tools/ns_driver.py`(분석기 MCP 상시 호출 드라이버+커서 기록기), `tools/analyze_test02_construct.py`(전체 자동 시나리오), `tools/analyze_test02_pending.py`(추가 시험, 미완), `tools/preview_scan_from_video.py`(영상에서 설치 가능 칸 복원), `tools/cursor_catalog.py`(exe 커서 대조). 분석기 C# 코드는 수정하지 않았다. 사용법·함정은 [분석기 문서](docs/analyze-manager.md#파이썬-mcp-드라이버커서-기록기영상-판독-도구-2026-10-04).
- [x] **문서 갱신:** [priest-construction-flow.md](docs/gameplay/priest-construction-flow.md)(원본 확인 반영), [input-controls.md](docs/gameplay/input-controls.md)(커서 표), [docs/videos/README.md](docs/videos/README.md).
- **디스크:** 이번 작업 약 13GB 사용(시작 여유 21GB → 종료 9.8GB). 판독 끝난 세션 영상·소리는 삭제. 남은 증거(Git 제외): S1 `20261004T050106077Z-296fe46e5b50`(6.5GB, 영상·소리 보존), S4 `20261004T074554041Z-299bc952ee8c`(3.2GB, 영상·소리 보존), 나머지는 로그·PNG만. 필요 없으면 두 세션의 `recording/video-*.avi`·`audio-*.wav` 를 지워 약 9.5GB 확보 가능. 작업 폴더 `extracted/test02-construct-run*`, `extracted/test02-pending-run1`, `extracted/frames/test02`, `extracted/audio/test02-*`.
- **중단된 시험(미완, 다음에 이어서):** `tools/analyze_test02_pending.py` — B. 템플 공사장이 대기 중일 때 다른 자리에 워크샵을 설치하면 앞선 공사장이 취소되는지 — **사용자가 "이전 건설 즉시 취소·환불"이라고 확인해 줘 규칙은 [노트 7.1절](docs/videos/auto-test02-construct-20261004.md)에 반영 완료**(내 영상은 설치 한 프레임 안만 있어 취소 지연은 미측정), C. 들고 있는 건물을 취소하는 방법(허공 좌/우클릭 등). 또 정지 키·다른 유닛 선택이 도착 전 공사장을 취소하는지도 미시험.
- **주의(재현 시):** P 키를 이미 선택된 사제에게 또 누르면 카메라가 사제 위치로 이동한다. 선택된 사제를 우클릭하면 메뉴가 열리지 않는다(허공을 눌러 선택을 먼저 푼다). 시작 직후 약 10초 이상은 팁 창이 클릭을 받지 않는다. 자동 녹화 중 사람이 마우스를 움직이면 "다른 창이 가림"으로 녹화가 끊긴다(S5가 그랬다).
- AGENTS.md·originals·originalCD는 수정하지 않았고 커밋은 하지 않았다.

## 2026-10-04 (Windows, 원본 실행·추가 분석 없음 — 로컬 팬게임 참고) ✅ 완료: 유닛 생산 자원 운송·실체화

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-040)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-04 (Windows 10, 원본 실행 없음 — 기존 분석 문서·웹 팬게임 대조) ✅ 완료: 사제 건물 건설 흐름(Construct 메뉴·걸어가서 건설)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-041)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-04 (`HJOW-Athlon`, Windows 10, 원본 자동 분석 실행 있음) ✅ 완료: 공통 메뉴 버튼 동작

- **공통 도구 검증/후속:** 당시 분석기 테스트 108개 통과·기존 1개 건너뜀. Campaign 행 효과음·펼침 행 추가 소리·메뉴 바깥 클릭 전달·Esc·판정 경계는 원본 확인 대상이다. C# Assets/Core 734개와 클론 스모크·UI 배치 차이는 이관한 구현 이력이다.

> **공통/이관 범위:** 원본 관찰·분석 도구는 두 프로젝트가 참고한다. [당시 C# 구현·검증·과제](LEFT_JOBS.dotnetpj.md#legacy-dotnet-042)는 dotnetpj 이력으로 이관했으며 cpppj 완료 기록이 아니다.

- [x] **원본 분석(analyzeManager 자동 녹화, 사용자 요청):** 메인 메뉴 돌 버튼의 호버·누름·오래 누르기·누른 채 바깥/이웃 버튼에서 떼기·나갔다 돌아와 떼기·우클릭/가운데·판정 경계, 팁 창·Credits 대화상자 버튼, Options 펼침 메뉴 행, Campaign 목록 행을 측정했다(세션 5개). 키보드는 사용자 설명(원본에 키보드 버튼 조작·Tab 포커스 없음)을 따랐다. 결과 표: [원본 공통 메뉴 버튼 동작 분석](docs/videos/menu-buttons-20261003.md).
- [x] **도구:** analyzeManager `game_input drag`에 `viaX/viaY/holdViaMs/holdMs` 추가(`DragPlan`, 단위 테스트 포함 [analyze-manager.md](docs/analyze-manager.md#원본-버튼-누름-시험용-drag-옵션-2026-10-03)), `tools/audio_events.py`(입력 사건 앞뒤 소리 ↔ 원본 효과음), `tools/button_state.py`(버튼 평소/눌림 판정).
- **원본 실행 이력:** 5회(정찰 1, 본 시험 4). 분석 PC는 RDP라 창이 가려지면 분석기 호출이 57~305초 멈췄고 45초를 넘으면 Auto-Demo가 시작됐다 → 시험 첫머리에 Auto-Demo를 끄고 승인 프롬프트 없는 단일 스크립트로 실행해 해결했다. 원본·AGENTS.md는 수정하지 않았고 커밋은 하지 않았다. 증거(영상·WAV·입력 로그)는 Git 제외 `extracted/analyzeManager/20261003T14*~T15*`.
## 2026-10-03 (전투 후속, 원본 정적 분석·팬게임 대조, 원본 실행 없음) ✅ 완료: 석궁·방어 건물

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-043)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-03 (웹 팬게임 참고·원본 녹화/바이너리 대조, 원본 실행 없음) ✅ 완료: 전투 조준·착탄·그림 보강

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-044)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-03 (`HJOW-Athlon`, 기존 녹화 분석, 원본 실행 없음) ✅ 완료: 버튼 효과음·선택 사거리·Crossbow 방향

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-045)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-03 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: TEST01 후속 검증·소유자 색·all 시작 지식·캐논 규칙

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-046)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-03 (`HJOW-Athlon`, Windows 10, 원본 실행 없음) ✅ 완료: TEST01 녹화 화면 요소 구현 (실행 검증은 위 후속 기록에서 완료)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-047)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-03 (Windows, 원본 실행 없음) ✅ 완료: 옮겨 온 TEST01 녹화 추가 판독·클론 전투와 이동 수정

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-048)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-03 (원본 실행 없음, 코드 수정 없음) ✅ 완료: AGENTS.md 목표 변경을 문서에 반영

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-049)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **요청:** 사용자가 AGENTS.md의 프로젝트 목표 부분을 고쳤고(커밋 `ba6d9ba`·`923b3df`), 그 내용을 문서에 반영한다. AGENTS.md 자체는 AI가 수정하지 않았다. 코드와 원본 게임은 건드리지 않았다. 커밋은 하지 않았다.
- **AGENTS.md에서 바뀐 것과 반영 위치:**

  | 변경 | 반영한 문서 |
  |---|---|
  | 모방 범위: "기존 게임 그래픽을 최대한 모방" → **사운드·그래픽·애니메이션 등 거의 모든 요소를 가능한 한 동일하게** | 이 문서 머리말, [animation-timing.md](docs/videos/animation-timing.md) 5절 |
  | **30·60·120프레임 지원**(30 먼저, 60·120 후순위). 원본은 요소마다 애니메이션 프레임이 달랐던 것으로 추정 | 이 문서 머리말·1.7절, [map-viewer.md 화면 설정](docs/map-viewer.md#프레임-속도-agentsmd-2026-10-05-기존-게임-수준의-프레임-먼저), [animation-timing.md](docs/videos/animation-timing.md) 5절 |
  | 가장자리 이동: "원본도 지원" → **원본도 옵션에서 해당 기능을 켰을 때 지원** | 이 문서 머리말·1.7절, [edge-scroll.md](docs/exe/edge-scroll.md) |
  | 원본의 전체화면 전환 뒤 재실행 오류는 **클론에서 발생하지 않아야 한다** | 이 문서 머리말·1.4·1.7절, [config.md](docs/formats/config.md) |
  | 커스텀 맵 제작 방법 주소 2개 추가 | 이 문서 1.6절, [mission-script.md](docs/formats/mission-script.md#커스텀-맵-만들기-agentsmd-2026-10-03-test01-예) |
  | 커스텀 맵 **TEST01**(`originals/d/TEST01.fort` + `TEST01.english`) 설명: fort는 게임 Edit 메뉴로 생성·수정, english는 사용자가 텍스트 편집기로 작성, 다른 언어는 언어 이름을 확장자로 | 이 문서 1.6절, [mission-script.md](docs/formats/mission-script.md#커스텀-맵-만들기-agentsmd-2026-10-03-test01-예), [fort.md](docs/formats/fort.md) |


---

## 2026-10-03 (`HJOW-X3D`, Windows 11, 기존 게임 플레이 녹화 분석) ✅ 완료: 화면 이동 방법·TEST01 편집기·시험 전투 진입 녹화 판독·문서화

- **공통 후속 관찰:** 녹화 실패 원인, Shift·방향키 조건, TEST01 편집기/시험 전투·AI·배치·메뉴 진입은 원본 자료로 확인한다. FortMapViewer 입력 변경 과제는 이관한 C# 기록을 따른다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-050)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **요청:** 기존 게임 수동 컨트롤 방식으로 화면 이동 방법과 커스텀 맵 TEST01의 편집기·시험 전투 진입까지 분석(30FPS). 첫 시도(`guide`)는 사용자 녹화가 남지 않아 무효로 하고 **기존 게임 플레이 녹화 분석 방식(`record-play`)**으로 다시 진행했다. 사용자 직접 요청이라 실행 확인 면제 대상이다. AGENTS.md·`originals/`는 수정하지 않았다(작업 트리의 ` M AGENTS.md`는 시작 전부터 있던 사용자 변경이다). **커밋은 하지 않았다.**
- **결과 문서:** [녹화 노트](docs/videos/record-play-edit-test01-20261003.md) — 시각표·화면 이동 실측·편집기 진입·시험 전투 진입/나오기·사용자 보고 구분·한계. 요약은 [입력 조작 표](docs/gameplay/input-controls.md#화면-이동-실측-2026-10-03-30fps-사용자-녹화)·[화면 목록 Edit 절](docs/screens/README.md)·[main-menu.md](docs/screens/main-menu.md)·[edge-scroll.md](docs/exe/edge-scroll.md)·[분석기 문서](docs/analyze-manager.md)에 반영했다.
- **실행 이력:** ① `guide` 세션 `20261003T074032275Z-78b4662ce3e2` — 자동 녹화 14초만 남고 사용자 녹화 없음(원인 미확정, 파일 보존) ② **`record-play` 세션 `20261003T074745758Z-adc787856962` — 13,552프레임·452.27초·평균 29.96FPS, 오류 없음.** 두 세션 모두 `end_session`으로 종료했고 남은 프로세스는 없다.
- **핵심 확인:**
  - **Alt · 가운데 버튼 누른 채 이동**은 같은 동작: 카메라 속도 = k × (커서 − 지도 영역 중심), 가로 k≈9.2/s·세로 k≈8.3/s, 중심 ≈(692,485)(1280×960 캡처 기준), 상관 0.95~0.98, 놓으면 0.05~0.1초 안에 정지. 편집기·시험 전투 동일.
  - **미니맵**(왼쪽 아래)을 누른 채 끌면 현재 화면 사각형과 화면이 커서를 따라간다.
  - **TEST01:** Edit → Load Battle Map의 목록은 432개 중 앞 50개만 보여 TEST01이 없다. **Create New Map → `TEST01` 입력 → OK**로 기존 맵이 편집기로 열린다(7.95초). Esc → Game → Test Battle → 연결 창 0.47초 → 브리핑(EQUIPMENT / Go!) → 전투. Storm Power 50000·Knowledge 전체는 `TEST01.english`(`myStartMoney`·`myTech`)와 일치. 나오기는 Esc → Game → Leave Mission → Main Menu.
  - 사용자 보고(영상 증거 없음, 노트 §7): 가장자리 이동은 전체화면에서만 동작(정적 분석과 일치), **Shift+1 화면 저장은 동작하지 않았다**(원인 미확정).
- **추가한 것:** [`tools/scroll_measure.py`](tools/scroll_measure.py)(프레임 간 지도 이동량 + 입력 로그 회귀, 상수·함수·반복문 주석 포함). 판독 이미지는 Git 제외 `extracted/record-test01-20261003/`.
- **자료 정리 상태(사용자 결정 대기):** `playingVideos/20261003T074745758Z-adc787856962/`(AVI 40·WAV 4·입력 JSONL 1, 약 2.07GB, Git 제외)와 무효 `guide` 세션 `extracted/analyzeManager/20261003T074032275Z-78b4662ce3e2/`(약 196MiB, 화면 증거 포함)는 지우지 않았다. 필요한 시각표·접촉표는 `extracted/record-test01-20261003/`에 있으며, 원자료 삭제는 사용자 확인 후 한다.

---

## 2026-10-03 (Windows, 원본 실행 없음) ✅ 완료: 원본 AI 난이도·Edit 맵 저장/시험 플레이 경로 정적 조사

- **난이도:** 전체 미션에 적용하는 AI Easy/Normal/Hard 선택지는 찾지 못했다. 캠페인별 `.english` 머리 값에서 `aiStartMoney`, `aiTech`, `aiGeyserAttachments`, `aiCollectors`, `aiTimeBetweenMoves`, `aiAbility` 등을 따로 지정한다. `.fort`는 배치된 요새 상태를 저장하고 같은 이름의 미션 스크립트가 AI 설정을 제공한다. 온라인 BattleMaster의 `Player Handicap`은 선택 플레이어의 유닛을 강하게 하는 기능으로, 싱글플레이 AI 전략 난이도와는 다르다.
- **Edit 흐름:** 메인 메뉴 Edit → Load Battle Map → Create New Map(“New Save-Game Name?”) 또는 맵 선택 → 편집기 Edit → Save as... / Game → Main Menu → 저장 확인 Yes. Game → Test Battle은 현재 맵을 시험 전투로 열고, 전투 Game → Return to Editing으로 편집기로 돌아간다. 이 메뉴 경로는 기존 관찰 기록과 `extracted/decomp/Netstorm.c` 문자열·메뉴 구성으로 확인했다.
- **현재 작업 트리(읽기만):** `originals/d/options.cfg`의 변경된 설정 중 `lastEditFort = "TEST01"`이 있고, `originals/d/TEST01.fort`는 2,118바이트·35개 섹션으로 파싱된다. 같은 이름의 `TEST01.english`는 없다(이 조사 시점 기준 — 이후 사용자가 `TEST01.english`를 추가했고 `.fort`도 6,608바이트로 바뀌었다. 맨 위 두 절 참고). 두 파일은 작업 시작 전부터 있던 사용자 변경이며 수정하지 않았다. 따라서 TEST01 요새 파일이 존재한다는 점은 확인했지만, 그 맵의 편집기 목록 표시·Test Battle 실행이나 AI 동작까지 검증한 것은 아니다.
- **난이도 수정 가능성:** 표준 게임 UI에 AI 난이도 설정은 없다. 미션별 `.english` AI 수치/기술/능력 또는 `.fort`의 시작 배치를 조절하는 방식은 데이터 구조상 가능하다. 파일 조회의 loose-file 우선순위는 디컴파일과 일치하지만, 패치 이력의 tarc 우선 문구와 충돌하므로 원본에서 override가 실제 적용되는지는 동적 확인 전까지 미확정이다.
- **남은 동적 확인:** Create New Map → 저장 → 메뉴 복귀 → 목록에서 재선택 → Test Battle의 전체 왕복, Add Island·Set All Bridge, TEST01의 실제 테스트 플레이와 AI 스크립트 연결. 이 확인은 원본 게임 실행이 필요하므로 별도 실행 허용 조건을 따른다.

---

## 2026-10-03 (`HJOW-Athlon`, Windows, 원본 실행 없음) ✅ 완료: 클론의 이동·선택·프레임 개선 (원본 자동 분석 결과 적용)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-052)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-03 (`HJOW-Athlon`, Windows, 원본 자동 분석 실행) ✅ 완료: 캠페인 1-1 The War Begins! 자동 분석 녹화·문서화, 분석기 파일 충돌 재시도 수정

> **공통/이관 범위:** 원본 관찰·분석 도구는 두 프로젝트가 참고한다. [당시 C# 구현·검증·과제](LEFT_JOBS.dotnetpj.md#legacy-dotnet-053)는 dotnetpj 이력으로 이관했으며 cpppj 완료 기록이 아니다.

- **요청:** 30FPS 자동 분석으로 튜토리얼이 아닌 스테이지 1 The War Begins!를 진행한다 — 사제 이동(마우스 조작·이동 애니메이션 관찰), 골렘 생산·이동, 사제로 워크샵 건설, 워크샵으로 덱 등록, ESC 메뉴로 미션 이탈·종료 — 하고 녹화를 문서화한다. 지정 시스템 예외로 실행 확인 없이 원본 복사본을 구동했다. AGENTS.md·원본 게임은 수정하지 않았다.
- **결과 문서:** [원본 자동 분석 녹화 노트](docs/videos/auto-war-begins-20261003.md)(시각표·마우스 조작·이동 애니메이션 측정·건설·덱 등록·ESC 이탈). 요약은 [입력 조작 표](docs/gameplay/input-controls.md#자동-분석으로-확인한-유닛-조작-2026-10-03-캠페인-1-1)·[이동 관찰](docs/gameplay/movement-pathing.md#35-원본-이동-명령-관찰-2026-10-03)·[분석기 문서](docs/analyze-manager.md#2026-10-03-자동-녹화-일시-파일-충돌-재시도와-가림-중단-사례)에 반영했다.
- **실행 이력:** 네 번 시도했다. ① `20261003T043823078Z-b4728e0c84f1` 자동 녹화 8.5초에서 `Access to the path is denied.`로 실패 ② `20261003T044417167Z-406ee9c80055` 시작 직후 같은 오류 → **분석기 수정** ③ `20261003T044645609Z-cc06bc935dad`(1차) 전 과정 수행, 그러나 녹화가 6분 48초에서 가림 오류로 중단 ④ **`20261003T052032571Z-e652ee405b0d`(2차) 승인 필요 없는 스크립트 한 개로 완주, 4,318프레임·236.9초 오류 없이 녹화.** 게임은 모두 `end_session`으로 종료했고 남은 프로세스는 없다.
- **분석기 수정(코드):** Windows는 `FileShare.Delete`를 허용해도 열린 파일의 `File.Move` 덮어쓰기를 오류 5로 거부한다. `SessionStore.RetryTransient`(20번·25ms)를 `WriteSmallFile`·`Save`·`AppendPart`·`AutomaticRecording.SaveState`에 적용하고 녹화 루프의 진행 상태 저장 실패가 녹화를 끊지 않게 했다. 변경 파일: `analyzeManager/SessionStore.Files.cs`·`SessionStore.cs`·`AutomaticRecording.cs`·`tests/AnalysisFrameRateTests.cs`(테스트 1개 추가). 분석기 테스트 **103개 통과·기존 1개 건너뜀**, Release·portable 빌드 경고·오류 0. **커밋은 하지 않았다.**
- **주요 관찰:** 이동 명령은 선택 → 땅 좌클릭 → 선택 해제, 허공은 거부, 출발 지연 0.4~0.8초, 직선 속도 사제 약 1.8·골렘 약 2.0칸/초. 골렘 배치 약 1.4초(에너지 방울 → 반투명 → 실체화), Sun Workshop 건설 약 10~11초(사제가 걸어가서 건설), 워크샵 우클릭 등록으로 덱에 Rain Generator·Sun Cannon 추가. 자세한 값과 영상 시각은 결과 문서.
- **녹화 도구에서 알게 된 것:**
  - **자동 녹화 중 승인 프롬프트가 필요한 도구 호출을 하면 VS Code가 앞으로 나와 게임을 가리고 녹화가 영구 중단된다**(1차 사례, 입력 로그의 물리 클릭이 근거). 좌표가 확정된 절차는 스크립트 한 개로 실행하고 사용자는 그동안 조작하지 않는다.
  - 실제 FPS는 평균 **18.2**(요청 30). AVI 헤더는 30FPS라 그대로 재생하면 약 1.65배 빠르다. 시각은 `.frames.csv`·`sessionElapsedMs`로만 계산한다.
  - 입력 로그에서 분석기 입력은 `injected=true`, 사람의 입력은 `false`라 섞임을 판별할 수 있다.
  - PowerShell: 함수 이름 `Mv`가 별칭 `mv`에 가려졌고(호버 이동 누락), 한글이 든 `.ps1`은 UTF-8 BOM이 있어야 5.1이 읽는다.
- **사용자 결정을 기다리는 것:** 자동 녹화가 게임 창 가림 때 **영구 중단 대신 해당 프레임만 건너뛰고 이어서 녹화**하도록 바꿀지(다른 프로그램 화면을 저장하지 않는 원칙은 유지). 현재 문서·코드는 "가려지면 중단" 정책이다.
- **남은 것:**
  - 이동 중 재명령, 길 막힘·우회, 대각선 경로 규칙, 세로 속도 정밀 측정(18FPS 영상으로는 짧은 지연의 분산이 흐림)
  - 워크샵 설치 불가 원인(템플 남서쪽 평지가 세 번 모두 붉은색), `Production Slots Available` 숫자(화면 밖으로 잘림), Whirlibase 등록
  - 명령 응답음·건설음·등록음의 효과음 파일 대응(`tools/audiomatch.py sfx`)
  - 30FPS 미달 원인 분리(캡처·JPEG 압축·게임 CPU 사용)와 60FPS 실제 달성률, `guide`·`record-play` 전환의 실제 시험
- **자료 위치(Git 제외):** 2차 `extracted/analyzeManager/20261003T052032571Z-e652ee405b0d/`(`recording/`·`screens/`·`run-script.log`), 판독 접촉표 `extracted/record-auto-20261003/`. 1차 녹화 세션은 약 1.2GB·2차는 약 0.8GB이며 불필요하면 삭제해도 된다(삭제는 사용자 확인 후). 실행 스크립트 원본은 임시 폴더에만 있고 저장소에는 넣지 않았다.

---

## 2026-10-03 (Windows, 원본 실행 없음) ✅ 완료: 분석기 세 방식 30/60FPS·자동 분석 세션 전체 연속 녹화

- **요청:** AGENTS.md·이 문서를 확인하고 자동 분석·기존 게임 수동 컨트롤 방식(`guide`)·기존 게임 플레이 녹화 분석 방식(`record-play`)의 분석·녹화를 기본 30FPS로 높이고 선택 60FPS 매개변수를 추가했다. 사용자 추가 답변에 따라 자동 분석도 세션 전체를 연속 녹화한다. AGENTS.md·원본 게임·기존 녹화물은 수정하지 않았다.
- **설정:** `--fps 30|60`, CLI JSON/MCP `fps`를 지원한다. `start_session`에서 고른 FPS를 `session.json`에 보존하고 `wait_for_change`와 사용자 녹화 창이 이어받는다. 기존 세션에 필드가 없으면 30FPS다. `guide`·`record-play --fps 60`으로 해당 창의 녹화 속도를 따로 선택할 수 있다. `wait_for_change`는 기본 `pollMs=0`에서 30/60FPS로 비교하고 양수를 명시하면 그 밀리초 간격을 사용한다.
- **자동 녹화:** `start_session`에서 숨김 녹화 프로세스를 실행하고 첫 프레임 저장을 확인한다. CLI/MCP 연결이 끝나도 게임이 살아 있으면 계속 기록한다. 파일은 `extracted/analyzeManager/<ID>/recording/`, 진행·오류는 `automatic-recording.json`과 `game_status.automaticRecording`에 남긴다. `end_session`은 게임 종료 뒤 파일 마감을 기다린다. `guide`·`record-play`를 열면 자동 녹화를 마감한 뒤 사용자 녹화로 전환하며 세션별 잠금으로 중복 녹화를 막는다. 사용자 창을 닫은 뒤 자동 녹화는 재시작하지 않는다.
- **프레임·출력:** 소수 간격(30FPS 33.333ms·60FPS 16.667ms)을 누적 시각에서 계산하고 늦어진 목표 시각은 건너뛴다. 녹화는 PNG 변환 없이 JPEG로 바로 저장하며 변화 비교 프레임은 PNG 압축을 생략한다. 마지막 증거 PNG는 실제 판정 픽셀에서 복원한다. 게임 대화상자가 열려도 영상 크기는 주 창에 유지하고 입력 좌표는 전면 대화상자를 따른다. AVI 헤더·각 색인 `videos[].fps`·`frameRates`로 과거 10FPS와 새 30/60FPS 조각을 구분한다. 기존 AVI/WAV 48 MB 분할·CSV 시각·입력 로그를 유지한다.
- **검증:** 분석기 Windows Release 빌드 경고·오류 0, 전체 테스트 **102개 통과·기존 1개 건너뜀**, 실패 0(신규 FPS·시간표·AVI·색인·PNG·프로세스 실패 경계 검사 17건). MCP 기본 검사 통과(도구 15개, FPS 스키마·잘못된 FPS의 실행 전 거부·EOF 종료). CLI 옵션 검사 5건 통과. YouTube portable 빌드도 복원 후 경고·오류 0.
  - 원본 게임 없이 합성 영상 두 개를 실제 녹화기로 만들었다. FFprobe에서 **30/60FPS·각각 30/60프레임·각 1.0초**를 확인했고 FFmpeg 전체 디코딩도 통과했다. 자료는 Git 제외 `extracted/analyzeManager-fps-check/`다.
- **문서:** [세 방식의 FPS·자동 녹화 사용법](docs/analyze-manager.md#세-방식의-3060fps-설정-2026-10-03), [분석기 README](analyzeManager/README.md). `tools/recordplay_frames.py`의 `--step` 설명도 30/60FPS 기준으로 보완했다.
- **아직 실행 확인하지 않은 것:** 실제 게임에서의 30/60FPS 달성률·음성 동기, CLI 종료 뒤 연속 녹화·게임 종료 마감·사용자 모드 전환, Windows/Wine 실제 캡처 경로. 이번 요청은 코드 수정이며 원본 게임은 실행하지 않았다.

---

## 2026-10-02 (`HJOW-Athlon`, Windows, 원본 실행 없음) ✅ 완료: `playingVideos/` 녹화 세 세션 클론 반영 완료 후 원자료 삭제

- **공통 미확인:** unitLost 제외 종류, 생산 창 새 항목 깜빡임과 다리 금 간 그림의 원본 조건은 추가 분석 대상이다. 낙하·SP·소리의 클론 검증과 자동 데모 구현은 dotnetpj 기록이다. 원자료 삭제는 당시 완료 사실이며 새 삭제 지시가 아니다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-055)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **요청:** AGENTS.md·이 문서를 읽은 뒤 `playingVideos/`의 분석기 녹화를 모두 클론에 반영하고 녹화 자료를 지운다. 참고용 mp4 5개는 지우지 않는다. 원본 게임은 실행하지 않았고 클론만 실행했다. AGENTS.md는 수정하지 않았다.
- **삭제 전 최종 판독:** [녹화 최종 판독](docs/videos/record-play-final-20261002.md).
  - 룬 마크 = `jimbuild.wav` 반복 시작: 룬별 1.87~2.42초(음성 길이 + 약 0.1초). 마크 → 소멸 10.04초(14건 ±0.02), 소멸 → 다음 음성 2.77초. 제단 폭발은 완료 9.45초 뒤.
  - 희생 음악은 포로가 묶인 제단으로 사제를 보내는 명령에서 요청한다(1-2 18:28.2 → 18:28.8, 곡 끝 아님). 곡 끝 재선택은 룬이 하나 이상 탔을 때만이다.
  - SP 숫자는 exe `FUN_0043e530` 규칙(300 초과 100·30 초과 10·그 밖 1)으로 초당 약 60번 따라간다. 정지 중에는 즉시 맞추고, SP 부족 유닛 클릭 때는 0.15초 간격으로 10번 깜빡인다.
  - 낙하 궤적: 30px/초 + 109px/초², 유닛 약 1.8초·다리 조각 약 1초.
  - 효과음: `jimbuild` 1.14초 반복, `priestForceField` 반복, `priestFall`·`priestFree`·`collapse`·`upgradeComplete`·`priestStruggleFade02`. `unitLost`는 exe에서 내 오브젝트 제거 시 위치를 저장한다(U 키). 명령 응답음은 클릭 0.06~0.10초 뒤에 났다.
  - 화면: 시작 팁 창, View 8항목·Game·Help 목록, 우클릭 창 구성, 도움말 색·스크롤 화살표.
- **원자료 삭제:** `playingVideos/` 아래 세 세션 폴더(`20260930T154921831Z-8bdcc06b6539`, `20261001T114152818Z-778e248ca3aa`, `20261001T145330210Z-425636a2e82c`)를 지웠다. 1초 간격 축소 프레임·시각표·버튼/키 사건 요약은 Git 제외 `extracted/record-play-archive-20261002/`에 남겼다. mp4 5개는 그대로다.
## 2026-10-02 (`vm-debian-codex`, Linux, 원본 실행 없음) ✅ 완료: 생산 창(덱) 사이드바를 원본 구성·동작으로 재구현

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-056)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-02 (`vm-debian-codex`, Linux, 원본 실행 없음) ✅ 완료: 네 번째 녹화 프레임과 클론 화면 직접 대조·소규모 수정

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-057)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-02 (`vm-debian-codex`, Linux, 원본 실행 없음) ✅ 완료: 보류됐던 클론 최종 테스트 실행

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-058)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-02 (`HJOW-Athlon`, 원본 실행 없음) 구현·테스트 코드 완료 / 최종 테스트 실행 보류: 녹화 분석 클론 반영 (※ 자동 검증 부분은 위 절에서 실행 완료)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-059)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-02 (Windows, 원본 실행 없음) ✅ 완료: 옵션 메뉴 마크·동작 추가 분석과 도움말 전체 문서화

- **요청·준비:** AGENTS.md·이 문서를 읽었다. 사용자가 네 번째 녹화의 옵션 동작 분석, 활성화 항목에만 나타나는 파란 원형 마크 반영, 도움말 텍스트 전체 문서화를 요청했다. 녹화와 아카이브를 읽었으며 새 게임 실행은 필요하지 않았다.
- **옵션:** [옵션 메뉴 문서](docs/screens/options-menu.md). Sound On(켜짐→꺼짐→켜짐), Play Music(같은 전환), Auto-Demo(켜짐→꺼짐), 음악/효과음의 선택 마크 이동을 입력 시각·순번·전후 프레임으로 대조했다. 파란 마크는 기능이 켜졌거나 현재 값으로 선택됐을 때만 나타난다. 커서의 어두운 행 강조·하위 메뉴 진입·조작 가능 여부와 구분한다. 선택 뒤 메뉴 닫힘·재개방, 메뉴 밖 클릭 닫힘도 기록했다.
- **도움말 전체:** [11주제 한국어 정리](docs/gameplay/help-text-record-play-20261001.md), [전체 영문 본문](docs/sources/in-game-help.md), [태그·제어 코드·공백 포함 UTF-8 원문 JSON](docs/sources/in-game-help-source.json). `d/help.english` 120,888바이트·2,727행·138앵커 정의·113본문 묶음을 모두 보존했다. 원본과 녹화 세션 아카이브의 본문이 바이트 일치하며, 원문 JSON에서 CP1252로 원래 바이트·SHA-256을 복원했다. 영상에 표시된 High Priest 능력치 머리·Back/OK도 별도로 기록했다. 개별 유닛·주문·채팅·멀티플레이 등 녹화에서 열지 않은 연결 본문은 원본 자료 보완으로 구분했다.
- **자료 구분:** `unitHelp`의 첫 정의(937행)가 녹화 목록과 일치한다. 뒤 정의(2064행)는 레벨·목록이 달라 함께 보존하고 적용 우선순위를 일반화하지 않았다. 영문 원문 전체와 방문 주제 한국어 정리를 구분하며, 도움말 설명을 패치 실행 파일의 확정 규칙으로 사용하지 않는다.
- **산출물·연결:** 추가 프레임·옵션 관찰표·문서 생성 스크립트·검사 결과는 `extracted/record-play-20261001-2354/`에 둔다. 녹화 노트·메인 메뉴·화면 목록·영상 목록·원본 자료 목록·도움말 안내·조작키(C 설명)를 보완했다. 원자료와 AGENTS.md는 변경하지 않았고 게임·분석기 코드는 변경하지 않았다.
- **검증:** 원문 JSON → CP1252 복원과 원본/세션 아카이브 대조 통과. 앵커 정의 138/138 보존, 본문 113묶음·문서 내부 링크·상대 링크 대상·UTF-8을 검사했다. `git diff --check` 통과. 원문 텍스트 보존본도 유지한다.

---

## 2026-10-02 (`HJOW-Athlon`, Windows) ✅ 완료: 네 번째 기존 게임 녹화의 UI·조작키 분석·문서화

- **요청·준비:** AGENTS.md·이 문서를 확인했다. 사용자가 기존 게임 플레이 녹화 분석을 직접 요청해 해당 작업 단계의 게임 실행이 허용된다. 입력 지연 대응 소스보다 최신인 Release 실행 파일(23:34 빌드)을 사용했다.
- **실행·종료:** 세션 `20261001T145330210Z-425636a2e82c`의 복사본과 `record-play`로 10월 1일 23:54:36~23:58:53 KST 사용자 플레이를 기록했다. 사용자는 미션 완주 대신 UI·조작키 설명을 시연했다. 분석 중 게임·녹화 창이 이미 종료된 것을 확인했고 관찰 메모 후 `end_session force=false`가 `closed=true`를 반환했다. 분석을 위해 새 게임을 실행하지 않았다.
- **저장·무결성:** `playingVideos/20261001T145330210Z-425636a2e82c/`에 AVI 5·CSV 5·WAV 2·음성 시작 시각 2·입력 JSONL 1·색인을 보존했다(**2026-10-02 원자료 삭제**, 맨 위 절). 2,563프레임·실제 영상 경과 256.149초, 입력 3,864건, `error=null`. JPEG 전체 디코딩·CSV 수·색인 크기·WAV 데이터 길이/조각 연속성·입력 순번/경계가 정상이다. 분석 산출물·원자료/PNG 해시는 `extracted/record-play-20261001-2354/`, 관리 세션은 `extracted/analyzeManager/20261001T145330210Z-425636a2e82c/`에 둔다. 삭제된 세 번째 녹화 폴더와 구분한다.
- **분석·문서:** [녹화 관찰 노트](docs/videos/ui-controls-record-play-20261001.md), [원본 조작키 표](docs/gameplay/input-controls.md). Options·음량/해상도 목록, 사제/생산 창/작업장 우클릭, Rain Generator·Whirlibase 등록, Escape 상단 메뉴·Leave Mission→Main Menu, 도움말 링크·드래그 스크롤·Back·OK를 판독했다. 실제 키 로그는 Escape 누름/해제뿐이며 F1~F9·조합키·카메라 저장 등은 내장 도움말 기재로 구분했다. 영상 목록·워크샵·메인 메뉴·맵 뷰어·분석기 안내에 연결했다. 게임·분석기 코드와 AGENTS.md는 수정하지 않았다.
- **지연 대응 검증 범위:** 수정 후 실제 녹화가 오류 없이 종료됐다. 분별 상대 훅 도착 대기 중앙값 8.8~10.5 ms, 전체 최대 17.3 ms로 이전 녹화 후반의 큰 누적 대기는 없다. 최대 프레임 간격 약 296 ms, 수초 공백 없음. 훅 종료/게임 적용/화면 커서 지연을 직접 측정한 값은 아니다.
- **별도 남은 확인:** 이번 시연에는 포획·의식이 없어 해당 구간의 체감 지연 개선과 실제 커서 이동/늦은 화면 반영의 구분은 아직 미확정이다. 이 녹화의 요청된 분석·문서화는 완료했다.
- **문서 검증:** 수정·추가 문서 9개의 UTF-8과 상대 링크 대상, 영상 프레임 순번/시각 단조 증가, WAV 조각 시작 연속성을 확인했다. `git diff --check` 통과. 문서·로컬 분석 산출물만 변경해 게임 빌드나 새 실행은 필요하지 않았다.

---

## 2026-10-01 (`HJOW-Athlon`, Windows) ✅ 완료: 분석기 재컴파일·녹화 확인·입력 지연 대응·포획 이전 판독

- **요청:** 작업 전 AGENTS.md·이 문서를 확인하고 기존 게임 분석기를 재컴파일한 뒤 기존 게임 플레이 녹화 분석을 시작한다. 사용자 직접 요청으로 해당 작업 단계의 실제 게임 실행이 허용된다.
- **빌드·검증:** `dotnet build analyzeManager/AnalyzeManager.csproj -c Release -t:Rebuild` 성공, 경고·오류 0. 분석기 테스트 81개 통과·1개 기존 건너뜀(심볼릭 링크 생성 권한 없음), 실패 0.
- **실행·종료:** 세션 `20261001T141005334Z-dfc9992ac304`의 게임 복사본에 `record-play`를 연결해 사용자 플레이를 녹화했다. 사용자가 완료·중단을 알린 후 녹화 파일을 판독했다. 판독 중 게임·녹화 창이 이미 종료된 것을 확인했고, 관찰 메모를 남긴 뒤 `end_session force=false`가 `closed=true`를 반환했다. 새 게임을 실행하지 않았다.
- **저장·검증(삭제 전):** `playingVideos/20261001T141005334Z-dfc9992ac304/`에 영상 15·WAV 5·입력 JSONL 2가 있었다. 6,318프레임·실제 영상 경과 635.689초, 입력 9,428건, `error=null`. 모든 JPEG 디코딩·CSV 프레임 수·색인 크기·WAV 데이터 길이·입력 순번/구간 경계를 대조했다. 영상·음성 각각 48 MB 미만, 입력 각각 4 MB 이하. 분석 산출물은 `extracted/record-play-20261001-2311/`에 보존했다.
- **원자료 삭제 ✅ 완료(2026-10-01):** AI의 폴더·개별 파일 삭제 명령이 실행 정책에 거부된 뒤 사용자가 세 번째 녹화의 영상·사운드·입력 데이터를 직접 삭제했다. 위 `playingVideos/` 세션 폴더가 없는 것을 확인하고 관찰 노트·영상 목록·분석기 문서에 반영했다. 삭제 작업은 남아 있지 않다. 분석 문서·추출 이미지·통계 등 산출물은 유지한다. 기존 원자료를 이용한 추가 추출에는 보관본 복원 또는 새 녹화가 필요하다.
- **입력 증상:** 사용자는 적 사제 포획부터 클릭 지연, 의식 중 커서가 마음대로 이동한 듯한 현상을 보고했고, 과거 움직임이 늦게 반영됐을 가능성도 제시했다. 09:00 이후 훅의 상대 도착 지연 중앙값 155.5 ms·최대 1,051.5 ms를 확인했다. 훅 내부 처리·게임 반영 지연은 이 측정에 포함되지 않아 포획 직후 증상까지 단일 원인으로 확정하지 않는다. 기록 중 분석기의 자동 입력 호출은 없었다. 원본 메인 루프의 `GetCursorPos → SetCursorPos`와 지연의 상호작용은 가능성으로만 기록했다.
- **수정:** 공통 `GuidedRecorder`의 UI 스레드 저수준 훅을 전용 메시지 스레드로 옮겼다. 입력 훅에서 창 열거·제목 조회·동기 JSONL 저장을 제거했다. 새 `GuidedInputBuffer`는 4,096건 대기열·별도 저장 스레드로 순서·시각을 보존하며 포화·쓰기 실패는 녹화 오류로 전달한다. 정상 중단 시 수락한 입력을 비운 뒤 중단 경계를 기록한다. 좌표는 입력 당시, 제목은 최근 영상 샘플이다.
- **판독·문서:** [새 관찰 노트](docs/videos/master-of-whirligigs-record-play-20261001-2311.md). **07:42.4 포획 이전** 작업장 비용·생산 슬롯·회수액, 다리 확장·전투·적 신전 폭발·지면 전환·사제 기절을 판독했다. 포획 이후 의식·조작 시각은 정상 게임 규칙 근거로 쓰지 않는다. 영상 목록·캠페인 1-2·분석기 안내에 링크와 입력 지연 대응을 반영했다. 클론 게임 규칙과 원본 바이너리를 바꾸지 않았다. AGENTS.md는 수정하지 않았다.
- **최종 검증:** 분석기 Release 빌드 경고·오류 0. 전체 테스트 **85개 통과·1개 기존 건너뜀**, 실패 0. 신규 `GuidedInputBufferTests` 4개는 느린 저장·순서/시각·포화·저장 실패를 검사했다. 게임 없는 MCP 기본 검사(도구 15개·EOF 종료) 통과. `git diff --check` 통과.
- **남은 확인:** 수정 빌드에서 같은 포획·의식 구간을 사용자 직접 조작으로 재녹화해 입력 지연이 해소되는지 비교한다. 이 작업 당시에는 수정 후 실제 전역 훅·게임 녹화를 검증하지 않았다. 후속 네 번째 UI 녹화는 위 2026-10-02 완료 절처럼 정상 기록됐으나 포획·의식은 재현하지 않았다. 실제 좌표 이동과 지연 반영의 구분도 미확정이다.

---

## 2026-10-01 (Windows, 원본 실행 없음) ✅ 완료: 두 사용자 녹화 모드의 별도 조작 로그 확장

- **요청 완료:** AGENTS.md와 이 문서의 분석기·수동 녹화 인수인계를 확인했다. `guide`(기존 게임 수동 컨트롤 방식)와 `record-play`(기존 게임 플레이 녹화 분석 방식)의 기존 공용 입력 기록기를 확장했다. AGENTS.md는 직접 수정하지 않았다.
- **별도 로그:** 두 모드 모두 녹화 시작 시 `input-번호.jsonl`을 즉시 만들고 UTF-8로 저장한다. `guide`는 `extracted/analyzeManager/<세션 ID>/recording/`, `record-play`는 `playingVideos/<세션 ID>/`에 둔다.
- **조작 정보:** 키 이름·가상 키·스캔 코드·누름/해제(자동 반복 포함), 마우스 이동·좌/우/가운데/보조 버튼·가로/세로 휠, 대상 게임 창의 클라이언트/전체 화면 좌표·영역 안 여부·당시 창 위치/크기/제목을 남긴다. Windows 원시 메시지·훅 시각·플래그와 프로그램 생성 입력 표시도 보존한다. 게임 전면 입력만 기록하고 이동은 기존처럼 최대 50회/초다.
- **동기화·재시작:** 입력 훅 진입 시 읽은 UTC·`sessionElapsedMs`와 이번 녹화의 `recordingElapsedMs`를 저장한다. 프레임 CSV의 `sessionElapsedMs`와 직접 대조한다. 형식 번호 `schemaVersion=2`, 구간별 `recordingId`·`sequence`·`started/stopped` 경계가 있어 입력 없는 구간도 남는다. 4,000,000바이트 이하로 분할하고 재녹화 시 앞선 파일을 덮지 않는다. 세션 시작/중단 이벤트에도 구간 ID를 넣고 시작 이벤트에는 입력 파일 위치·형식 번호를 남긴다.
- **구현·문서:** `analyzeManager/GuidedInput.cs`·`GuidedInputJournal.cs`를 추가하고 `GuidedRecorder`에서 공용으로 사용한다. 자유 플레이 안내 문구·색인의 입력 형식 설명을 보완했다. [로그 필드와 예시](docs/analyze-manager.md#사용자-조작-로그-두-녹화-모드-공통-2026-10-01-확장), `analyzeManager/README.md`에 사용법을 반영했다.
- **검증:** 분석기 Release 빌드 경고·오류 0. 분석기 전체 테스트 **81개 통과·1개 기존 건너뜀**(심볼릭 링크 생성 권한 없음), 실패 0. 새 `GuidedInputTests` 25개로 키·버튼·휠 해석, 보조 모니터·창 이동·영역 밖 좌표, 밀리초 계산, UTF-8 분할·재녹화 보존·무입력 구간·크기 제한·자유 플레이 입력 색인을 검사했다. 게임 없는 MCP 기본 검사(도구 15개·EOF 종료)도 통과했다. 확장 형식으로 실제 원본 게임을 녹화한 검증은 하지 않았다.

---

## 2026-10-01 (`HJOW-Athlon`, Windows, 원본 실행 없음) ✅ 완료: 클론 캠페인 1-2 Master of Whirligigs 구현·잠금 해제

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-064)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-01 (`HJOW-Athlon`, Windows, 기존 게임 플레이 녹화 분석) ✅ 완료: 캠페인 1-2 녹화 판독 + 희생 의식 일시정지·완료 보상 구현

- **공통 정적 근거:** 파괴 폭발은 원본 FUN_0044b9e0과 [전투 분석](docs/exe/battle-options.md)을 참고한다. 싱글 처치 보상 25%·희생 보상/일시정지는 원본/사용자 근거다. BattleSession.Sacrifice.cs·426개 테스트는 C# 이력이다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-065)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **실행:** 사용자가 "기존 게임 플레이 녹화 분석"을 직접 요청했다. 이 PC는 지정 예외 시스템이기도 하다.
  - `analyzeManager` Release를 다시 빌드했다. 9/30 빌드에는 `record-play`가 없었다.
  - 세션 `20261001T114152818Z-778e248ca3aa`에 `record-play`를 붙였다. 사용자가 캠페인 1-2 `Master of Whirligigs`를 약 20분 플레이하며 재현 장면을 녹화했다.
  - 녹화 결과: 11,842프레임, AVI 29, WAV 9, 입력 1, `error=null`.
  - 분석 뒤 녹화 창을 닫고 관찰 메모를 남긴 다음 `end_session force=true`(`closed=true`)로 끝냈다. `Netstorm.exe`는 남아 있지 않다. `originals/`는 수정하지 않았다.
- **판독:** [녹화 노트](docs/videos/master-of-whirligigs-record-play-20261001.md).
  - 사용자 재현 항목: 실제 전투, 운반 골렘·내 사제가 서 있는 다리 붕괴, 의식 중 사제 이탈, 운반 골렘 판매·`Drop`, 의식 중 제단 판매.
  - 사용자 보충 설명 두 가지를 반영했다.
    - Thunder 재음성은 **룬 마크가 나타나기 전에** 사제를 옮긴 경우다.
    - 의식 완료 보상은 **+5,000 SP**다(멀티플레이 보상 선택은 후속).
  - 판독 확정 사항:
    - 이탈 시 의식은 깨지지 않고 멈춘다(포로 유지). 마크 후 이탈이면 룬 완료, 마크 전 이탈이면 룬 취소·복귀 시 재음성. 마크는 음성 뒤 1.6~2.6초에 나타났다.
    - 제단 판매 시 포로가 제단 자리에서 체력이 가득 찬 채 풀려나고, 다음 의식은 첫 룬부터 다시 한다.
    - 골렘 판매(+100)·`Drop` 시 사제가 그 자리에서 기절 없이 풀려난다.
    - 운반 골렘이 낙하하면(약 1.8초 연출) 사제는 허공에서 기절한다(체력 가득). 다리를 재건하면 0.4~0.6초 안에 복귀한다.
    - 허공 사제는 계속 떠 있다.
- **도구:** [`tools/recordplay_frames.py`](tools/recordplay_frames.py)를 추가했다. record-play AVI 조각을 FFmpeg 없이 읽어 `frames`·`sheet`(잘라내기)를 만들고, 시각은 `.frames.csv` 기준이다. 임시 스크립트와 같은 프레임 해시를 만드는 것을 확인했다.
- **문서:** [희생 의식 계약](docs/gameplay/sacrifice.md), [이동 경로 3.4 임시 정책 표](docs/gameplay/movement-pathing.md#34-낙하-규칙-구현과-임시-정책), [음악 5절](docs/exe/music.md), [분석기 record-play 실측](docs/analyze-manager.md#기존-게임-플레이-녹화-분석-모드), [맵 뷰어](docs/map-viewer.md), [영상 목록](docs/videos/README.md). **AGENTS.md는 수정하지 않았다.**
## 2026-10-01 (Linux GUI, 원본 실행 없음) ✅ 완료: 클론 빌드·테스트·배포의 originals/ 의존성 제거

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-066)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-01 (Linux, 원본 실행 없음) ✅ 완료: README에 실행 가능한 바이너리 빌드·배포 방법 추가

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-067)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-01 (Linux GUI, 원본 실행 없음) ✅ 완료: 클론 UI의 글씨·버튼·알림/선택창을 원본 외관에 맞춤

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-068)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-01 (Windows, 원본 실행 없음) ✅ 요청 범위 구현: 메인 메뉴·캠페인 목록·옵션·캠페인 1-1

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-069)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-01 (Windows, 원본 실행 없음) ✅ 구현: 지상 낙하·허공 사제 복귀·길 막힘 대기와 이동 재개

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-070)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-01 (원본 실행 없음, 코드 수정 없음) 📝 문서화: 이동 경로 재탐색 점검 + 유닛 생산 스톰 에너지 줄기 설계 요구

- **공통 낙하 요구(사용자 확인):** 지상 보행형 이동체는 허공 이동 또는 서 있는 다리 소멸 시 낙하·파괴된다. 사제는 파괴되지 않고 허공에서 기절해 비행 수송으로 포획할 수 있다. 다리를 다시 놓아 설 수 있고 체력이 절반 이상이면 사제만 회복한다. 경로 재탐색과 별도로 현재 발판 소멸도 검사한다. 당시 C# 허공 사제 점유 결함과 개선 제안은 이관했다([계약](docs/gameplay/movement-pathing.md)).

- **공통 요구:** 생산 스톰 에너지 줄기는 지상 경로가 끊기면 그 지점부터 공중 직선 이동으로 전환해 도착한다. 출발지 파괴·목적지 소멸의 도착/환불 정책은 원본 확인이 필요하다. 당시 C# 즉시 생성 상태와 후속 완료는 이관했다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-071)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **요청:** (1) 이동·채집·사제 포획 중 지나갈 다리가 붕괴하거나 건물형 유닛이 파괴돼 길이 없어졌는데도 옛 경로로 가다 추락·파괴되는 원본 문제를, 클론에는 넣지 말고 **경로 재탐색**으로 처리할 것. 현재 소스를 확인해 수정이 필요한지 판단하고 **코드는 건드리지 말고 문서에만** 반영. (2) 유닛 생산 때 워크샵·템플에서 스톰 에너지 줄기가 출발해 배치 위치에 닿은 뒤 유닛이 나타나는 효과에도 경로 탐색이 있는데, 이쪽은 **이동이 불가능해져도 어떻게든 도달**해야 한다(예: 불가 시점부터 공중 이동). 이것도 문서화.
## 2026-10-01 (원본 실행 없음) ✅ 코드 점검 인수인계의 남은 의심 사항 2건 수정 (DropPriest 경로 재탐색·희생 사제 RemoveEntity 일원화)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-072)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-01 (`vm-debian-codex`, 원본 실행 없음) ✅ 코드 점검·수정: 다른 AI 작업분(Linux 분석기·포획/제단/승패) 검토

- **공통 분석 도구:** 당시 Linux portable/Windows 분석기 빌드는 경고·오류 0, mcp_smoke.py --youtube-only는 통과했다. 게임 Core 결함 수정·382개 테스트와 구분한다.

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-073)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-10-01 (`vm-debian-codex`, Linux, 원본 실행 없음) ✅ 완료: Linux YouTube 분석기·캠페인 1-5 포획 관찰·Core 희생 의식 구현

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-074)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### A. 완료 — Linux에서 analyzeManager YouTube 분석 사용

- Windows 게임 제어기는 `net10.0-windows`와 Win32 GUI를 유지한다. 별도 `analyzeManager/portable/AnalyzeManager.Portable.csproj`는 공용 분석 코드를 링크해 Linux에서도 실행되는 `net10.0` CLI·stdio MCP를 제공한다. 등록 도구는 `youtube_*` 6개이며 원본 게임 제어 기능은 포함하지 않는다.
- `YouTubeAnalyzer`는 Linux에서 확장자 없는 `yt-dlp`·`ffmpeg`를 찾고, PNG 크기는 IHDR에서 읽으며, 관찰표는 ffmpeg RGB와 내장 글꼴로 PNG를 그린다. Windows 관찰표는 기존 GDI+ 경로다.
- `PREPARE.sh`에 Deno·`ytanalyzer`를 추가했다. 패키지 설치가 안 되거나 sudo 권한이 없으면 BtbN 정적 FFmpeg를 `~/.local/bin`에 설치한다. `tools/audiomatch.py`는 세션 폴더 외에 영상 파일과 `--offset`도 받는다.
- 사용법을 [분석기 문서](docs/analyze-manager.md#youtube-영상-분석-원본-게임-실행-없음), [README](analyzeManager/README.md), [영상 자료 안내](docs/videos/README.md)에 반영했다.
- 검증: Linux portable 빌드 성공(경고·오류 0), 프레임 추출과 `youtube_note` 기록 확인. 이전 단계의 `mcp_smoke.py --youtube-only`와 Windows/Wine 기본 검사도 통과했다. 전체 솔루션 Release 빌드와 Core 테스트 결과는 이 절 끝의 최신 검증 요약을 따른다.
- 남은 환경 회귀 확인: Windows 실기기에서 기존 GDI+ 관찰표 경로를 다시 확인한다. Linux 빌드만으로는 Windows 실제 화면 출력 회귀를 검증하지 않았다.

### B. 완료 — 캠페인 1-5 YouTube 포획·제단 의식 분석

- AGENTS.md에 적힌 13개 영상의 메타데이터·표본 관찰표를 만들었다. 방송 인트로·오버레이가 있는 영상은 색 비교에 주의한다. `8tcj3rI_YqE`·`b3VzVERd9CE`·`0p7VvzSxTAY`·`WDQSrGqZAH0`의 편집 요소 전수 확인은 후속 분석이다.
- 캠페인 1-5 `adR1Kap60hw`에서 골렘의 기절 사제 포획, 알타 500 SP 건설, 약 14.5초 클릭-완공, 다섯 룬, 희생, 알타 소멸·성공 창까지 관찰했다. 효과음 상관값과 두 번째 미션에서 재확인한 시각은 [관찰 노트](docs/videos/youtube-sacrifice.md)에 기록했다.
- Linux `youtube_note`로 관찰 시점 10개의 메모 11개(13:35.5/13:37.5 시각 정정 포함)를 `extracted/youtube/adR1Kap60hw/notes.jsonl`·`report.md`에 저장하고 `youtube_frames` SHA-256과 연결했다. 추출물은 Git 제외 경로다.
- 시간 보정에 쓴 측정은 룬 사이 14.8초, 음성에서 룬 소멸까지 약 12.1초, 의식 완료에서 알타 제거까지 평균 9.3초, 제거 후 성공 창까지 3초다. 희생 효과음은 완료 약 4초 뒤로 exe 상수와 일치한다.
- 3-4 `Enemy Territory` 구조 미션 확인 완료: 원본 미션 스크립트의 `allowAnyCapture = 1`, 프리스트 운반 화면, 내 섬에 내려놓은 뒤 뜨는 성공 화면을 대조했다. 관찰 범위와 SHA-256 근거는 [구조 미션 영상 노트](docs/videos/youtube-rescue.md)에 있다. 1-6·2-x에서 의식 시간 재확인, 2-x·3-x Man o'War/Dust Devil 동작은 후속이다.

### 최신 검증

- `dotnet build analyzeManager/portable/AnalyzeManager.Portable.csproj -c Release --no-restore`: 성공, 경고·오류 0개.
- `dotnet build analyzeManager/AnalyzeManager.csproj -c Release --no-restore`: Windows 대상도 교차 빌드 성공, 경고·오류 0개.
- `python3 analyzeManager/tests/mcp_smoke.py --exe analyzeManager/portable/bin/Release/net10.0/Netstorm.AnalyzeManager --repo . --youtube-only --youtube https://youtu.be/CI3dCrUt4tY`: 통과. 프로토콜 도구 6개, PNG 관찰표 응답, EOF 정상 종료.
- `youtube_frames`로 1-5 영상 프레임 10개 추출, `youtube_note` 메모 11개와 SHA-256 근거 저장 성공.
- 전체 검증은 원본 게임 실행 없이 수행했다. Windows 실기기에서의 GDI+ 화면 생성만 남았다.

---

## 2026-10-01 Whirlibase·Whirligig 구현 + 새 YouTube 도구 활용 ✅

- **공통 원본 근거:** .type HP 50·range 30·speed 2·hpPerSec 10, 도움말의 보급·수송 제외·3대 제한, 패치 기록의 출발점 사거리를 참고한다. 생성 5초·보급 3초·접근 1.5칸·직선 이동·1초 피해는 당시 C# 임시값이다([전체 근거/추정표](docs/gameplay/flyers.md)).

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-075)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **자료:** AGENTS.md·이 문서를 다시 읽고 새 `youtube_probe/frames/clip/note`로
  캠페인 1-1~4(`AMEzorbjQYQ`)의 4분 간격 12장·13:00~13:20 상세 표본을 확인했다.
  관찰 메모를 13:00 프레임 SHA-256에 연결했다. 원본 게임 실행·전체 영상 전수 시청은 하지 않았다.
## 2026-10-01 (`DESKTOP-HJOW`, 원본 실행 없음) ✅: analyzeManager YouTube 영상 분석 도구 (CLI + MCP)

- **목적(사용자 요청):** 원본 게임을 직접 플레이·녹화하는 대신, YouTube에 올라간 원본 플레이 영상의 주소를 받아 영상 내용을 읽어 분석한다. AI가 주로 쓰므로 **MCP 도구로도 공개**했다.
- **구현:** `analyzeManager/YouTubeVideo.cs`(주소·시각 해석, 광고 성격 구간·서버 삽입 광고 판정 — 순수 함수), `analyzeManager/YouTubeAnalyzer.cs`(yt-dlp·ffmpeg 실행, `extracted/youtube/<ID>/` 기록), `AnalysisEngine`(인자 추가, `youtube_*`는 데스크톱 잠금 없이 처리), `ExplorerTools`(MCP 도구 6개), `Program`(CLI 사용법).
  - `youtube_probe`(메타데이터·업로더 챕터·SponsorBlock 구간) / `youtube_list`(채널·재생목록 영상 목록) / `youtube_frames`(**원격 스트림에서 지정 시각만** 디코딩해 PNG, 여러 장이면 시각을 적은 관찰표 이미지 반환, 재사용 색인) / `youtube_clip`(최대 300초 구간을 원본 fps mp4로, 시작 프레임 정확, 50 MB 미만) / `youtube_note`(AI 메모 + 프레임 해시) / `youtube_videos`(조회해 둔 영상).
- **광고 처리:** (1) 재생기 광고는 별도 영상이라 원본 스트림에 없음 → 브라우저 재생·화면 녹화를 쓰지 않으므로 영상 시각 = 업로드 원본 시각. (2) 서버 삽입 광고 대비: 스트림 길이(ffprobe)가 메타데이터보다 `max(2초, 0.5%)` 넘게 길면 프레임·구간 도구가 거부(`allowDurationMismatch`로만 진행). (3) 업로더가 영상 안에 넣은 협찬·홍보·인트로 등은 SponsorBlock 구간으로 표시(프레임 `nonContentSegment`, 관찰표 주황 글씨, 구간 저장 경고). (4) 만료된 스트림 주소는 자동 갱신·1회 재시도.
- **검증:** analyzeManager Release 빌드 오류·경고 0. 단위 테스트 52개 중 51개 통과(1개 기존 건너뜀, 신규 `YouTubeVideoTests`). MCP 검사 `mcp_smoke.py --youtube https://youtu.be/CI3dCrUt4tY` 통과(도구 15개, PNG 이미지 콘텐츠). CLI 실측: `0p7VvzSxTAY` 조회(챕터 5개, 스트림 6093.5초/메타데이터 6094초 정상), 프레임 4장 약 25초, 재사용 0.5초, 10초 구간 약 16초, 채널 `@netstormcampaigns2591` 목록, 잘못된 주소·범위 거부. 원본 게임은 실행하지 않았다.
- **문서:** [analyze-manager.md "YouTube 영상 분석"](docs/analyze-manager.md#youtube-영상-분석-원본-게임-실행-없음), [analyzeManager/README.md](analyzeManager/README.md), [videos/README.md](docs/videos/README.md).
- **남은 것 / 다음 후보:** 로그인 쿠키(연령 제한·회원 전용 영상) 미지원, 장면 전환 자동 탐지(예: ffmpeg scene 필터로 메뉴·브리핑 창 찾기) 미구현, 영상 소리 판독(`tools/audiomatch.py`는 record-play 녹음 전용) 연결 미구현. 전용 채널의 미션별 스피드런 영상으로 캠페인 2·3 미션 관찰 노트를 만드는 것이 다음 활용 후보다.
- 변경 파일: `analyzeManager/{YouTubeVideo,YouTubeAnalyzer}.cs`(신규)·`{AnalysisEngine,ExplorerTools,Program}.cs`·`README.md`, `analyzeManager/tests/{YouTubeVideoTests.cs(신규),mcp_smoke.py}`, `docs/analyze-manager.md`, `docs/videos/README.md`, 이 문서. 산출물 `extracted/youtube/`는 Git 제외.

---

## 2026-10-01 포대 전투 1차 구현 + 추가 YouTube 영상 반영 ✅

- **공통 영상:** playingVideos/[Youtube] 3-1 to 3-5.mp4(출처 0p7VvzSxTAY), H.264 1280×720 30fps, 1:41:33.5, 오디오 없음. 2분 간격 51장과 08:00~08:05.5·01:20:00~01:20:01.1 표본을 판독했다. Vander Tower 번개 관찰은 [영상 노트](docs/videos/youtube-act3-combat.md), 가정은 [전투 계약](docs/gameplay/combat.md)에 있다. 공용 tools/video_contact.py는 시각별 관찰표를 만든다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-077)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **기존 녹화 재확인:** 캠페인 1-1 신전 파괴 전후 프레임에서 갈색/빨강 → 초록/주황 지면 전환을 직접 확인했다.
## 2026-10-01 (`DESKTOP-HJOW`, 원본 실행 없음) ✅: 녹화 소리·영상 추가 판독 + 음악·희생 의식 분석 + 오디오·지식 창 구현

- **공통 후속:** 원본 음악/효과음 이벤트 대응, 제단 애니메이션 속도/외관, 볼륨·색상·오디오 자료의 미확인 조건은 추가 분석 대상이다. AudioPlayer/MusicDirector·지식 창 구현과 청취/GUI 검증은 dotnetpj 기록이다. 공용 새 도구는 tools/audiomatch.py다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-078)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **진행상황 재파악:** AGENTS.md·이 문서·`docs/` 를 읽었다. 이 PC 의 `playingVideos/` 에는 `record-play` 세션 `20260930T154921831Z-8bdcc06b6539` 하나뿐이다(기존 방송 영상 4개는 원드라이브 보관, 이 PC 에 없음). 이 세션의 **녹음(WAV 7개)은 이전에 판독하지 않았다** → 이번에 판독했다.
- **새 도구 `tools/audiomatch.py`** (`level`·`music`·`sfx`): 녹음을 원본 `music/*.mus`·`sound/*.wav`와 FFT 정규화 상호상관으로 대조. 사용법 [videos/README.md](docs/videos/README.md#소리-판독-도구-toolsaudiomatchpy-2026-10-01).
- **소리 판독 결과** ([music.md](docs/exe/music.md) 4절, [녹화 노트](docs/videos/the-war-begins-record-play-20260930.md) 5절):
  - 배경음악: 메뉴 `ser22` → 미션 `rain22`(00:09.1) → `thu22`(03:42.7, `thunderCrack.wav` 동반) → `sun22`(07:27.9) → `wind22`(11:20.8) — 앞 곡이 끝나자마자 0.5초 안에 다음 곡. **12:06.4 `sacrifice.mus`가 wind22 를 끊고 시작**, 성공 창(13:34)이 뜬 뒤에도 이어진다. `fanfare.mus` 없음.
  - 희생 의식: 신전 파괴(10:58.5) → 적 사제 보호막(`priestForceField` 10:59.8~11:34.4) → 골렘이 사제를 집음(`golemPickUp` 11:35.9, 영상 확인) → 제단 도착(12:05.6) → **다섯 룬** 음성 `forWind2`·`forSun2`·`forRain2`·`forThunder2`·`forStorm2`(약 14.8초 간격, 룬마다 `altarBurnCollapse`) → `itIsDone2`(13:19.0) → 4.0초 뒤 `priestSacrifice2`(exe 상수와 일치) → 제단 소멸 → 성공 창.
- **사용자 설명(2026-10-01) 기록·대조:** "의식 중 배경음악이 바뀌고(모든 스테이지 공통), 다른 적 사제가 남아 있으면 의식 뒤 본래 루프로 돌아오며, 적 사제가 없으면 승리 창이 떠서 게임이 일시정지된다" → 녹음·exe 와 모두 일치. exe 기준으로 원소 루프 복귀 시점은 **희생 음악 곡이 끝날 때**(실시간, 139.9초)이고 중단된 곡이 아니라 다음 곡부터다 — 이번 녹화도 의식이 끝난 뒤 성공 창까지 희생 음악이 이어졌다. 승리 창이 열린 동안 SP 는 그대로였다(정지). [music.md](docs/exe/music.md) 0·5절.
- **exe 정적 분석(음악):** `FUN_00469fc0`(시작, 첫 곡 난수)·`FUN_00469f00`(다음 곡 wind→rain→thunder→sun, 희생 중이면 sacrifice)·`FUN_00469f60`(곡 끝 확인, 실시간 `timeGetTime`)·`FUN_00469db0`(요청, 30초 이하 곡은 180초 뒤 재확인, fanfare·defeat 잠금)·`FUN_00469c80`(천둥 곡 thunderCrack, `ascendancyPalette` 날씨 팔레트)·`FUN_00494eb0`(내 제단 의식 시작 → sacrifice). 제단 단계 `FUN_00449f40`·`FUN_00448080`. 효과음 호출 위치(다리 붕괴·금·건설 완료·조각 회전 등)는 [music.md](docs/exe/music.md) 7절.
- **영상 추가 판독:** (1) **F6 지식 창은 게임 시간을 멈추지 않는다**(열린 채 SP 3,450 → 3,650, 전투 소리 지속) — 이전 노트의 미확정 4번 해소. (2) **지식 격자의 어두운 카드 = 마우스 호버**(입력 로그와 대조). (3) 격자 25장 = `thewarbegins.fort` Technology 목록 플래그 4 타입 26개 − `sunWalker`, 행 안 순서 = `.type` group 순서. (4) 입력 로그: F6 4회 열고 닫음, 다리 칸 단축키 Q·W·A·S·Z·X 다수 사용. (5) `MissionAbort`: 인자 0 → `[ABORT]` 확인 창, 인자 ≠ 0 → 곧바로 LeaveBattle (exe `FUN_00463e40`).
## 2026-10-01 (사용자 플레이 녹화 분석) ✅: 캠페인 1-1 전체 플레이 관찰 노트

> **공통/이관 범위:** 원본 관찰·분석 도구는 두 프로젝트가 참고한다. [당시 C# 구현·검증·과제](LEFT_JOBS.dotnetpj.md#legacy-dotnet-079)는 dotnetpj 이력으로 이관했으며 cpppj 완료 기록이 아니다.

- 관리 세션 `20260930T154921831Z-8bdcc06b6539`의 `record-play` 녹화 8,335프레임(13분 53.5초, AVI 27개, WAV 7개, 입력 로그 1개)을 분석했다. 녹화 오류는 없었다. 게임은 `The War Begins!` 브리핑에서 시작해 승리 후 `Master of Whirligigs` 브리핑을 거쳐 메인 메뉴로 돌아왔다. 분석 뒤 안내창과 원본 게임을 종료했고 `end_session` 응답은 `closed=true`다.
- 원본 지식 창의 **SUN·WIND·RAIN·THUN. 4행 그림 카드 격자**, 개별 항목 상세창을 확인했다. 적 회오리 신전 파괴 직후 약 2초 안에 섬이 갈색 지면·빨간 테두리에서 초록 지면·주황 테두리로 바뀌었지만 성공 창은 약 2분 15초 뒤에 떴다. `Success!`의 `Leave Missions`·`Next Mission`과 다음 미션 브리핑도 확인했다.
- [타임스탬프·증거·미확정 사항](docs/videos/the-war-begins-record-play-20260930.md)을 기록하고 [영상 목록](docs/videos/README.md), [지식 창 정적 분석](docs/exe/show-technology.md)을 갱신했다. 승리 직전 제단 모양 구조물이 사라지는 모습은 보이나 사제 포획·희생·승리 조건의 정확한 연결은 추가 분석이 필요하다. → ✅ 2026-10-01 소리 판독·exe·사용자 설명으로 확정(맨 위 절).

---

## 2026-10-01 (지식 창, 원본 실행 없음) ✅: `Review Knowledge`(`ShowTechnology`) 정적 분석 + 클론 구현

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-080)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **분석(exe 바이트 역어셈블):** `ShowTechnology`는 디스패처에서 **인자 없이** `0x492b60`을 부르는 F6 "View Netstorm Knowledge" 지식 창이다(스크립트의 `55`는 쓰이지 않음). 내 플레이어가 아는 타입을 원소별로 모은다. 이름 표에서 `GetTechnology`(`0x494ea0`)는 지식 부여 + `[NewTech]` 안내로 별개다. 상세: [show-technology.md](docs/exe/show-technology.md). Ghidra 프로젝트(`extracted/ghidra/Netstorm.gpr`)가 이 PC에서 0바이트라 `decompile_at.ps1`은 쓰지 못했다(그대로 멈춤 — 프로세스 정리함). 필요하면 `run_decomp.ps1`로 프로젝트를 다시 만들 것.
## 2026-10-01 (`HJOW-X3D`, 원본 실행 없음) ✅: 기존 게임 플레이 녹화 분석 모드 구현

- `analyzeManager`에 `record-play --session ID`를 추가했다. 기존 관리 세션에 지침 없는 안내 창을 붙이며 게임을 새로 시작하지 않는다. 사용자가 녹화 시작·중단을 누르고 자유롭게 플레이한다. 창은 대기·녹화 중(경과 시간·프레임 수)·오류 중단·게임 종료를 색과 문장으로 표시한다. 화면 캡처·오디오·입력·창 위치 오류가 나면 녹화를 닫고 시작 버튼을 다시 켠다. 사용자가 다시 누르면 다음 번호의 파일로 이어 기록한다.
- 기존 10 FPS MJPEG AVI·루프백 WAV·입력 JSONL 기록기를 재사용한다. 새 모드 파일은 `playingVideos/<세션 ID>/`에 두며 AVI/WAV는 각각 48 MB 전에 자동 분할한다. 중단 시 `recording-index.json`에 영상·소리·입력 조각과 시각 보조 파일 이름·크기를 갱신한다. 기존 `guide` 모드의 출력 위치와 단계 안내는 유지한다. [사용법](docs/analyze-manager.md#기존-게임-플레이-녹화-분석-모드), [도구 README](analyzeManager/README.md).
- **확인:** Release 빌드 오류 0. NuGet 취약성 피드 연결 실패로 NU1900 경고 1개. 자동 승인 검토가 요청되지 않은 테스트 코드 추가를 거부해 테스트는 추가·실행하지 않았다. 원본 게임과 새 안내 GUI도 실행하지 않았다. 실제 녹화 화면·중단 후 재시작은 후속 동적 확인이 필요하다. 이 PC는 AGENTS.md의 무확인 원본 실행 예외 시스템이 아니므로 실제 게임 실행 전 사용자 명시 확인이 필요하다.
- 변경 파일: `analyzeManager/{Program,GuidedForm,GuidedRecorder,SessionStore,FreeplayRecordingIndex}.cs`, `analyzeManager/README.md`, `docs/analyze-manager.md`, 이 문서.

---

## 2026-09-30 (`HJOW-X3D`, Windows, 원본 실행 없음) ✅: 클론 캠페인 초기 브리핑 창 구현 — 브리핑을 닫기 전까지 세션 시간 정지

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-082)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-09-30 (`HJOW-Athlon`, Windows, 원본 실행 없음) ✅: 안내·브리핑 창 시계 정지 경로 디컴파일 — 정지 경로 4곳 확정, 캠페인 브리핑도 같은 경로

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-083)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- 아래 `HJOW-X3D` 절이 남긴 "디컴파일 필요 항목"을 사용자 요청으로 진행했다(`HJOW-X3D`의 중단 지시는 그 PC 한정이며 이 PC는 해당 없음). **원본 게임은 실행하지 않았다.** `tools/ghidra/decompile_at.ps1`(Ghidra 12.1.4)로 누락 함수를 복구하고, 디컴파일이 없는 곳은 capstone 바이트 역어셈블로 읽었다. 상세·표·근거: [dialog-pause.md](docs/gameplay/dialog-pause.md).
- **결론**
  1. **시계를 멈추는 코드는 정확히 4곳**(`FUN_00460df0` 직접 호출 4, 주소 값 참조 0, 카운터 직접 쓰기 없음): 미션 스크립트 객체 슬롯 10(`0x4c2a56`), Moviegump 열기(`0x4837d1`), **개발자용 `Pause - Shift-F9` 토글**(`0x4d82b5`), 어설션 처리(`0x4e090b`).
  2. 이전 인수인계 1번(`0x4d8200~0x4d8300`)은 캠페인 브리핑이 **아니라** 개발자용 Pause 토글이다. `0x4d7000~0x4d9fff`가 "함수가 없는 구간"으로 보였던 이유는 그 안이 **메인 프레임 함수 `FUN_004d62b0`(`0x4d62b0`~`0x4da64f`)이고 전체 디컴파일에서 타임아웃으로 실패했기 때문**이다.
  3. 인수인계 2번(`0x4837d1`)은 Moviegump 열기 `FUN_00483680`이다. 영상 재생 함수 `FUN_00484010`이 옵션 2(`+0xa8`)를 **항상** 켜므로 영상은 시계와 프레임 갱신을 모두 멈춘다. **출하 미션 파일에는 영상 재생 명령이 없다.**
  4. 인수인계 3번(스크립트 객체 가상 함수 표 누락 슬롯 6개)을 전부 읽었다(슬롯 3 `0x4c2b00`은 Ghidra가 함수로 만들지 못해 바이트로 읽음). 표 `0x5149ec`가 **`Tutorial` 미션 클래스**이고, 다른 클래스 `Normal`(표 `0x50d948`)의 정지 슬롯은 빈 함수다. **영어 튜토리얼·캠페인은 전부 `Tutorial`**, `Normal`은 옛 독일어 `mission1~5`뿐.
  5. 인수인계 4번: `DAT_00565dd4`는 **다이얼로그 gump 목록의 맨 위 창**(`FUN_004c8e90` = 열린 다이얼로그 없음).
- **캠페인 초기 브리핑 흐름(사용자 규칙 2와 일치):** 브리핑은 섹션 `[A.]`다. 미션 시작 뒤 **다이얼로그 없이 연속 10프레임**이 지나면 Tell되고 같은 순간 시계가 멈춘다. 재개는 열린 안내창·다이얼로그·모달이 모두 없는 상태가 **7프레임 연속**일 때다. 브리핑이 뜨기 전 10프레임은 시계가 흐른다.
- **게임 도중 창(사용자 규칙 3)의 원본 동작(exe 정적 확인, 동적 미검증):** 스크립트 객체가 여는 창은 **모두 시계를 멈춘다** — 단계 섹션, `Success`/`Failure`, AI 신전·사제 이벤트(`ai<N>TempleHalfDead`·`TempleDead`·`PriestDead`·`PriestCaptured`·`PriestSaved`), `NotVortex`·`VortexDestroyed`·`FactoryDestroyed`. 사용자가 허용한 "멈추는 것으로 통일"과 일치한다. **반대로 게임 코드가 `Tell`을 직접 부르는 경고·안내 창(`NewTech`, `NoBridgeYet`, `AltarUpgrade`, `WarnAscend`, `ZoneLocked` 등 약 40곳)은 시계를 멈추지 않는다.** 클론이 이런 창을 구현할 때의 시간 정책은 별도 결정이 필요하다(기본은 원본대로 흐름).
- **다리 붕괴 시각 측정에 미치는 영향:** 없음(기존 결론 유지). 튜토리얼 1 안내창은 단계 섹션이라 시계를 멈춘다는 사실이 코드로 확정됐다.
- **도구 변경:** [tools/exe_callscan.py](tools/exe_callscan.py)에 `--dis START END`(구간 역어셈블, `pip install capstone` 필요)와 `--str ADDR...`(주소의 C 문자열)을 추가했다. 기존 옵션 결과가 그대로 나오는 것을 확인했다(`0x460df0` 호출 4곳, 표 덤프).
- **남은 것**
  1. (선택) 원본에서 게임 중간 창이 떠 있는 동안 시간이 멈추는지, `NewTech` 같은 경고 창이 떠 있는 동안 시간이 흐르는지 동적 확인. 게임 실행이 필요하다(이 PC `HJOW-Athlon`은 AGENTS.md 실행 확인 예외 시스템). 클론은 확인 없이 진행해도 된다.
  3. (필요 시) 메인 프레임 함수 `FUN_004d62b0` 전체 디컴파일 — `tools/ghidra/DecompileAt.java`의 `DECOMPILE_TIMEOUT_SEC`(120초)를 늘려야 한다. 이번에는 필요한 분기만 역어셈블로 읽어 하지 않았다.
- 변경 파일: `docs/gameplay/dialog-pause.md`(전면 갱신), `docs/exe/mission-header-flags.md`(6절 미확인 3건 해소), `tools/exe_callscan.py`, 이 문서. 게임 코드 변경 없음. 산출물 `extracted/decomp-at/`은 Git 제외 경로.

## 2026-09-30 (`HJOW-X3D`, Windows, 원본 실행 없음): 안내·브리핑 창은 게임 시간을 멈춘다 — 사용자 규칙 기록 + exe 정적 확인

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-084)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **사용자 규칙(2026-09-30):** (1) 튜토리얼 안내 창이 떠 있는 동안 게임 시간은 멈춘다. (2) 캠페인 미션의 **초기 브리핑 창도 닫히기 전까지 시간이 흐르지 않는다.** (3) 일부 미션은 게임 **중간에** 창이 뜨는데 그때 원본에서 시간이 흐르는지는 **확인 필요**. 단 클론은 그 경우도 **시간을 멈추는 것으로 구현해도 된다.** → [안내·브리핑 창과 게임 시간](docs/gameplay/dialog-pause.md)(신규).
- **exe 정적 확인(부분):** 게임 시계 `FUN_00460cd0`은 정지 카운터 `DAT_0055b4b0`이 0이 아니면 멈춘 값을 돌려주고 재개 때 정지 구간을 누적 오프셋에 더한다(정지 `FUN_00460df0`, 재개 `FUN_00460e30`). 미션 스크립트 객체(`FUN_004c2a40` 정지, 프레임 함수 `FUN_004c34c0` 재개)는 **열린 안내창 수**(`DAT_005cab2c`, 안내창 생성자 `FUN_004cf120` +1 / 소멸자 `FUN_004cf1c0` −1)가 0이고 다른 조건이 **7프레임 연속** 참일 때 재개한다. 규칙 1·2를 뒷받침한다. **정지 함수 `FUN_004c2a40`의 호출자가 전체 디컴파일에 없어** 어떤 섹션(모든 Tell인지 초기 브리핑만인지)이 정지시키는지, 중간 창에서도 정지하는지는 확정하지 못했다. → ✅ 위 `HJOW-Athlon` 절에서 해소: 스크립트 객체가 Tell하는 창은 모두(단계·성공/실패·AI 이벤트 등) 정지, 게임 코드가 직접 부르는 경고 창은 정지 안 함.
- **이미 잰 다리 붕괴 시각에 미친 영향:** 영상 두 기록의 배치 직후 안내창 시간(최대 2.2~2.4초)은 게임 시간에서 빠진다. 보정해도 금 30~40초·제거 70~80초 모델 범위에 들어가고(한 칸짜리 금은 게임 시간 30.0초 이상) 결론은 그대로다. bridge-pieces.md 8.3·9, 화면 목록 1.11절 반영.
- **추가 exe 정적 확인(2026-09-30, `tools/exe_callscan.py` 바이트 스캔 + 기존 디컴파일 읽기):**
  - `FUN_004c2a40`(정지)은 스크립트 객체 가상 함수 표(`0x5149ec`)의 **10번 슬롯(+0x28)**이다. 섹션 알림 함수 `FUN_004c2a90`이 섹션(`<단계 글자>.` 또는 `Demo`)을 찾으면 이 슬롯을 불러 정지한다. `FUN_004c2a90`의 호출자는 프레임 함수 `FUN_004c34c0`(단계 잠금 카운터가 0이 될 때) 하나뿐 → **튜토리얼 단계 안내·데모 안내가 시계를 멈추는 경로는 확정.**
  - `FUN_00460df0`(정지) 직접 호출 지점 4곳: `0x4c2a56`·`0x4e090b`(어설션)는 디컴파일에 보이고 **`0x4837d1`·`0x4d82b5`는 안 보인다.** `FUN_00460e30`(재개) 5곳 중 `0x4d8293`(= `0x4d82b5` 바로 앞)이 안 보인다. `0x4d7000~0x4d9fff` 구간은 디컴파일에 함수가 하나도 없다.
  - 일반 Tell(`FUN_004cf960`)은 안 멈춘다고 보이고(이 함수에서 정지 호출 없음), **캠페인 초기 브리핑·중간 창의 정지 경로는 위 안 보이는 구간에 있을 것(추측)이라 확정하지 못했다.** → ❌ 추측이 틀렸다(`HJOW-Athlon` 절): 안 보이던 `0x4d82b5`는 개발자용 Pause 토글이고, 캠페인 브리핑도 위에서 확인한 슬롯 10 경로(`Tutorial` 클래스 `[A.]`)로 정지한다.
- **사용자 지시(2026-09-30): 디컴파일 작업은 이 PC(`HJOW-X3D`)에서 더 진행하지 않는다.** Ghidra 실행·함수 복구는 하지 않았다(`C:\Tools\ghidra_12.1.4_PUBLIC`과 `extracted/decomp/Netstorm.c`는 이 PC에 있음). 읽기 전용 바이트 스캔과 기존 디컴파일 텍스트 읽기까지만 했다.
- ~~**다음 PC 인수인계 — 디컴파일 필요 항목**~~ → ✅ **`HJOW-Athlon`에서 완료(위 절).** 네 항목 모두 풀렸다: (1) `0x4d8200~0x4d8300`은 개발자용 Pause 토글(메인 프레임 함수 `FUN_004d62b0` 안), (2) `0x4837d1`은 Moviegump 열기 `FUN_00483680`, (3) 누락 슬롯 6개는 `Tutorial` 미션 클래스의 함수(캠페인도 같은 클래스, 브리핑 섹션은 `A.`), (4) `DAT_00565dd4`는 다이얼로그 gump 목록 맨 위. 결과는 [dialog-pause.md](docs/gameplay/dialog-pause.md).
- **디컴파일 외 남은 일** (이 목록은 위 절의 "남은 것"으로 이어짐)
  1. 게임 중간에 창이 뜨는 캠페인 미션에서 창이 떠 있는 동안 시간이 흐르는지 원본 관찰(`playingVideos` 영상·스크린샷 대조 또는 수동 녹화). → exe 정적 분석으로 "스크립트 창은 정지, 게임 코드의 직접 경고 창은 안 멈춤"까지 확인됨(위 절). 동적 확인은 선택이다. 클론은 확인 없이 정지로 통일해도 된다.
- **새 도구:** [tools/exe_callscan.py](tools/exe_callscan.py) — 원본 exe 바이트만 읽는 스캔(직접 `call` 지점, 4바이트 값 참조 위치, 가상 함수 표 덤프). 디컴파일·원본 실행 없음. 재현: `python tools/exe_callscan.py 0x460df0 0x460e30`이 위 호출 지점과 같은 결과를 낸다.
- 변경 파일: `docs/gameplay/dialog-pause.md`(신규), `tools/exe_callscan.py`(신규), `docs/exe/bridge-pieces.md`, `docs/screens/README.md`, 이 문서. 게임 코드 변경 없음.

## 2026-09-30 (`HJOW-X3D`, Windows, 수동 녹화 완료) ✅: 한 칸짜리 다리 재측정 — "단일 칸 160초"의 정체는 두 칸 막대, 붕괴 모델은 맞음

- 사용자가 한 칸짜리 조각으로 재측정을 요청해(수동 분석 예외) 새 세션 `20260930T133835395Z-693470346a00`을 시작했고, 사용자가 배치 **전부터** 직접 녹화했다. 종료는 안내 창 닫기 → 관찰 메모 → `end_session force=true`(`closed=true`)로 했고 `Netstorm.exe`는 남아 있지 않다. `originals/`는 수정하지 않았다. 영상 분석이 성공해 `screenShots/`로 옮기지 않았다(AVI 2개·WAV·입력 로그는 세션 `recording/`, Git 제외).
- **결과**(10 FPS, 배치 클릭 13:39:57.91 UTC 기준): 한 칸짜리 조각이 **+32.4초에 금, +72.4초에 낙하**, 금→낙하 **40.0초**, 스캔 위상 2.4초(금·낙하 일치). 모델(금 4번째 스캔 +30~40초, 제거 8번째 스캔 +70~80초)과 맞는다. [화면 목록 1.11절](docs/screens/README.md#111-사용자-직접-조작-녹화-한-칸짜리-다리-붕괴-시간-2026-09-30-windows-hjow-x3d).
- **이전 "단일 칸 약 160초" 관찰 해소:** 이전 증거 세션(`20260929T133717181Z-7f413cde9896`, 이 PC에 있음)의 관심 영역 이미지를 다시 보니 그때 놓은 것은 **두 칸짜리 막대**였다. 해시 전환 시각이 배치 후 +38.9~40.1(바깥 칸 금) / +77.9~79.8(바깥 칸 낙하) / +118.0~119.9(안쪽 칸 금) / +158.6~159.7초(안쪽 칸 낙하)로 칸마다 80초다. 클론의 기존 테스트 `Decay_PlankChainCollapsesFromTheTip`(40·80·90·160초)가 이미 이것을 재현하므로 **모델·코드는 수정할 것이 없다.** 문서의 "단일 칸 120초 보통 + 40초 금" 서술은 [bridge-pieces.md 4절 끝·8.3](docs/exe/bridge-pieces.md)과 [화면 목록 1.5절](docs/screens/README.md) 표에서 정정했다. 세 관찰(두 칸 막대 · T자 · 한 칸) 비교표는 8.3절.
- **남은 것**
  1. **건물형 유닛 파괴·회수 뒤 주변 다리 약화**(금 간 다리는 바로 무너지는지, 5×5 건물의 범위) — 튜토리얼 2 같은 건물 미션에서 별도 실험(아래 절 2번 그대로).
  2. ~~(선택) 튜토리얼 안내 창을 오래 열어 둔 채 붕괴 스캔이 멈추는지 확인~~ → ✅ 사용자 확인으로 해결(멈춤, 위 절 [dialog-pause.md](docs/gameplay/dialog-pause.md)). 실험 불필요.
  3. (선택) 클론 `BridgeGrid` 테스트에 T자 조각을 추가(이전 절 3번 그대로).
- **참고(이 PC `HJOW-X3D`):** 이전 인수인계들이 "증거 세션이 이 PC에 없다"고 한 `20260929T133717181Z-…` 등은 이 PC에 있다. `extracted/analyzeManager/`의 세션 폴더(게임 복사본 포함)는 각각 약 310 MB이므로 필요 없어지면 사용자가 정리한다(자동 삭제 안 함).
- 변경 파일: `docs/exe/bridge-pieces.md`, `docs/screens/README.md`, 이 문서. 코드 변경 없음.

## 2026-09-30 (`HJOW-X3D`, Windows, 수동 녹화 완료): T자 조각 다리 붕괴 시간 재측정 — 모델과 일치

- 사용자가 수동 컨트롤 분석을 다시 요청해 새 세션 `20260930T132055915Z-667d1a82e7f4`를 시작했고(AGENTS.md 수동 분석 예외), 사용자가 `Tutorial → 1 Bridge the Gap!`에서 다리 조각 한 개를 놓는 과정을 직접 조작·녹화했다. 녹화 종료 후 안내 창을 닫고 관찰 메모를 남긴 뒤 `end_session force=true`(`closed=true`)로 종료했으며 `Netstorm.exe`는 남아 있지 않다. `originals/`는 수정하지 않았다. 영상 분석이 성공해 영상을 `screenShots/`에 옮기지 않았다(AVI 3개·WAV·입력 로그는 세션의 `recording/`, Git 제외).
- **결과**(10 FPS 영상 프레임 단위, 배치 클릭 13:24:55.75 UTC 기준): 놓은 **T자 조각**의 줄기 끝 칸에 **+36.9초 금**, **+77.1초 낙하**(금 간 뒤 40.2초). 가로 막대 동쪽 끝과 나머지 줄기는 녹화 끝(+97초)까지 그대로였다. 금은 4번째 스캔·제거는 8번째 스캔이라는 10초 스캔 수명 모델([bridge-pieces.md 8.1·8.3](docs/exe/bridge-pieces.md))과 맞고, 두 시각이 같은 스캔 위상(약 7.0초)을 준다. 어느 칸이 줄었는지도 8.1의 방문 목록 규칙을 손으로 따라간 결과와 같다(클론으로 이 모양을 돌려 보지는 않음). 상세: [화면 목록 1.10절](docs/screens/README.md#110-사용자-직접-조작-녹화-t자-다리-조각-붕괴-시간-2026-09-30-windows-hjow-x3d).
- **여전히 남은 것**
  1. ~~**단일 칸 약 160초 관찰**~~ → ✅ 위 절에서 해소(한 칸짜리 재측정 + 이전 증거 재검토: 160초는 두 칸 막대).
  2. **건물형 유닛 파괴·회수 뒤 주변 다리 약화**(금 간 다리는 바로 무너지는지, 5×5 건물의 범위) — 튜토리얼 2 같은 건물 미션에서 별도 실험.
  3. (선택) 클론의 `BridgeGrid`에 이 T자 조각을 같은 칸 배치로 넣어 줄기 끝 칸만 금 +30~40초·제거 +70~80초로 줄고 막대 동쪽 끝은 유지되는지 테스트로 고정(조각 모양은 `BridgePiece` 표에서 T자 계열을 골라 영상과 대조 필요).
- **이번에 배운 작업 요령**(자세히는 [analyze-manager.md](docs/analyze-manager.md#사용자-직접-조작-녹화-모드)): PowerShell 5.1의 `>`로 `start_session` 출력을 받으면 게임이 끝날 때까지 안 끝난다 → `cmd.exe /c` + 절대 경로; 안내 창의 단계 표시 시각은 직전 단계의 완료 시각이고 사용자가 사건마다 한 번만 누르도록 문장을 써야 하며, 시간은 영상 프레임 CSV로 잰다(이번 기록의 버튼 시각은 영상보다 최대 10초 늦음); ffmpeg 없이 MJPEG AVI를 Python+Pillow로 읽을 수 있다.
- 변경 파일: `docs/exe/bridge-pieces.md`, `docs/screens/README.md`, `docs/analyze-manager.md`, 이 문서. 코드 변경 없음. 빌드 산출물(`analyzeManager/bin`)은 이 PC에 없어서 Release 빌드를 새로 했다(경고·오류 0, Git 제외 경로).

## 2026-09-30 (`HJOW-X3D`, Windows): 수동 다리 붕괴 분석 중단·안내 창 크기 개선

- 사용자의 중단 요청으로 원본 복사본 세션 `20260930T125406012Z-d15793779d8d`를 끝냈다. 시작 화면만 캡처했고 미션 진입·게임 입력·녹화는 없었다. `end_session force=true`가 `closed=true`를 반환했으며 `Netstorm.exe`는 남아 있지 않다. 세션 보고서와 안내 단계는 `extracted/analyzeManager/20260930T125406012Z-d15793779d8d/`에 보존했다. `originals/`는 수정하지 않았다.
- **안내 창·DPI 수정**: [GuidedForm.cs](analyzeManager/GuidedForm.cs)는 96 DPI 기준 기본 클라이언트 크기 560×620, 최소 창 크기 500×540으로 바꾸고 테두리·크기 조절 손잡이를 추가했다. 이전에는 폰트가 포인트 크기라 125% 배율에서 물리 픽셀 기준 글자가 25% 커질 수 있었지만, 창·컨트롤의 자동 배율은 지정하지 않아 글자와 버튼 공간이 어긋날 수 있었다. `AutoScaleMode.Dpi`를 지정해 글꼴·창·컨트롤을 함께 조정하고, 배율 적용 후 창을 게임 옆에 다시 배치한다. 단계 이동·녹화 버튼·상태는 창 크기에 맞춰 배치하고 본문 영역이 늘어난다. 창이 화면 작업 영역을 벗어나거나 게임과 겹치면 크기를 유지하며 옆으로 다시 배치한다. [사용법](docs/analyze-manager.md)에 반영했다.
- **확인**: `analyzeManager` Release 빌드 성공(컴파일 오류 0). NuGet 취약성 피드에 연결할 수 없어 NU1900 경고 2개가 나왔다. `git diff --check` 통과. 실제 GUI 표시 확인과 테스트는 하지 않았고 원본 게임도 재실행하지 않았다.
- **미완료 분석** → ✅ 수동 녹화는 **위 절(2026-09-30 `HJOW-X3D`, 수동 녹화 완료)에서 수행**했다(사용자가 수동 분석을 다시 요청해 중단 요청은 해소됨). T자 조각 결과는 모델과 맞았고, 이어 한 칸짜리 재측정으로 단일 칸 불일치도 해소했다(맨 위 절). 건물형 유닛 파괴·회수 뒤 주변 다리 약화 실험만 남았다.

---

## 2026-09-30 (`HJOW-Athlon`, Windows): 미션 Game 메뉴·재시작·안내 창 Esc 동작 구현 ✅

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-088)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 2026-09-30 (`HJOW-Athlon`, Windows, 원본 자동 분석): Replay·Restart Mission 첫 화면 확인 ✅

- `analyzeManager` 세션 `20260930T115011876Z-a2dcd80d0ab9`에서 `Bridge the Gap!`에 들어가 `Leave Mission → Replay Mission`을 선택했다. `Starting Mission...` 로딩 창 뒤 같은 미션의 첫 `NetStorm!` 브리핑과 **0 SP** 화면이 다시 나타났다. 증거 해시: `326a0af414b6885b7832532bcc98fd71380418f116db105354d1dbf7a5982a0e`.
- 브리핑을 닫고 `Game → Restart Mission`을 선택해도 같은 로딩 창 뒤 첫 브리핑과 **0 SP**가 다시 나타났다. 증거 해시: `fd783fb1cdfaacec8f0a7cd2cbfe919ae7260bd7d4f6a6b29b935167217d63fb`. 배치물 초기화는 이 세션에서 시험하지 않았다. 관찰 메모를 남기고 게임을 `end_session force=true`로 종료했다.
- [화면 관찰 상세](docs/screens/README.md#19-replay-missionrestart-mission-전환-2026-09-30-windows-hjow-athlon). 아래 인수인계의 미확인 항목 중 두 전환을 완료로 표시했다. 변경 파일: `docs/screens/{README,main-menu}.md`, 이 문서.

## 2026-09-30 (`HJOW-Athlon`, Windows, 원본 자동 분석): Original Credits 마지막 전환 확인 ✅

- `analyzeManager` 세션 `20260930T113718545Z-fda9e5c1745b`에서 원본 복사본을 실행하고 Original Credits를 입력 없이 관찰했다. 선택 입력은 11:39:09.541 UTC, 마지막 쪽 캡처는 11:43:34.299 UTC, 메인 메뉴가 다시 보인 캡처는 11:43:49.931 UTC다. **선택부터 복귀 관찰까지 280.39초**로, 스크립트의 14쪽 × 각 20초와 일치한다. 복귀 증거 SHA-256: `198f9dda482552b67c70baf6430e013f7972f0bb845600df645f037671ea60a9`(중앙 ROI).
- 이후 입력 없이 `The Storm Rages!` Auto-Demo로 전환한 것도 확인했다. 메인 메뉴 복귀와 데모 화면 사이 캡처 간격 때문에 데모 시작의 정확한 시각은 이번 측정으로 확정하지 않는다. 세션에 관찰 메모를 저장하고 게임을 `end_session force=true`로 종료했다. 앞서 중단된 세션 `20260930T113311036Z-0b577dc139c4`도 실행 중인 프로세스가 없음을 확인해 종료 처리했다.
- [화면 관찰 상세](docs/screens/README.md#18-original-credits-마지막-전환-2026-09-30-windows-hjow-athlon). 이 항목의 미확인 상태를 아래 인수인계 목록에서 완료로 변경했다. 변경 파일: `docs/screens/{README,main-menu}.md`, 이 문서.

## 2026-09-30 (`vm-debian-codex`, Linux, 원본 게임 실행 없음): 수동 분석 안내 창의 게임 크기 변경 대응 ✅

- `guide` 안내 창을 처음 열 때뿐 아니라 1초마다 주 게임 창의 크기·위치를 확인해, 바뀌거나 서로 겹치면 게임 바깥의 오른쪽/왼쪽 빈 영역으로 옮긴다. 녹화 시작 전에도 적용된다. 게임 안의 작은 대화상자는 배치 기준에서 제외하고, 제목 표시줄과 테두리도 피한다. 창의 최초 위치가 유지되도록 수동 시작 위치를 지정했다.
- 옆 공간이 없어지면 상태 메시지를 표시하고 진행 중인 녹화를 중단한다. 공간이 다시 생기면 안내 창을 재배치한다. [수동 조작 녹화 사용법](docs/analyze-manager.md#사용자-직접-조작-녹화-모드)에 반영했다.
- **검증**: Linux에서 분석 도구와 테스트 프로젝트 Release 빌드 오류·경고 0. Wine 단위 테스트 27개 중 26개 통과·1개 기존 심볼릭 링크 검사 건너뜀. 새 배치 테스트 4개는 창 축소·왼쪽 배치·보조 모니터·공간 부족을 검사한다. 실제 게임과 수동 안내 GUI는 실행하지 않았으므로 Windows 화면에서 이동 시점과 배치가 맞는지는 후속 확인이 필요하다.
- 변경 파일: `analyzeManager/{GuidedForm,WindowsGame,GuidePlacement}.cs`, `analyzeManager/tests/GuidePlacementTests.cs`, `docs/analyze-manager.md`, 이 문서.

## 2026-09-30 심야 분석 + Windows 부분 확인: 방화벽 경고 원인·포트와 `PREPARE.ps1` TCP 6799 항목

- **공통 포트 범위:** TCP 6799 사전 등록은 원본 분석용 조치다. 클론 네트워크 구현/실행 여부는 프로젝트별로 확인하며 원본 호환 포트 참고값은 6799/6800/8998이다. 당시 C# 싱글플레이 소켓 미사용 상태는 이관했다.

> **공통/이관 범위:** 원본 관찰·분석 도구는 두 프로젝트가 참고한다. [당시 C# 구현·검증·과제](LEFT_JOBS.dotnetpj.md#legacy-dotnet-092)는 dotnetpj 이력으로 이관했으며 cpppj 완료 기록이 아니다.

- **질문**: Windows PC 수동 분석 중 방화벽 경고가 떴다 — 분석 프로그램이 쓰는 포트가 있는지, 고정할 수 있는지.
- **결론** ([network-ports.md](docs/exe/network-ports.md)): 분석 도구(analyzeManager)는 포트를 쓰지 않는다. 경고의 원인은 **게임 복사본 `Netstorm.exe` 가 전투 시작 때 TCP 6799(`gameServerPort`)를 0.0.0.0 으로 리슨**하는 것이다. 메인 메뉴에서는 열리지 않고 한번 열리면 종료까지 유지된다. 세션마다 게임을 새 경로로 복사하므로 경로 기준인 방화벽 규칙에 세션마다 걸릴 수 있다. 포트는 이미 6799 로 고정이고 `setup.cfg` 의 `gameServerPort` 로 바꿀 수 있지만(실측 6899 로 바뀜) **포트를 고정해도 경고는 없어지지 않는다.**
- **Wine 실측** (`ss -ltnup` 기준 대조): 시작·메뉴 = 새 리슨 없음, 자동 데모 시작 순간 `0.0.0.0:6799`, 메뉴 복귀 후에도 유지, 종료 시 닫힘, 8998/6800/6802/80 은 어느 시점에도 없음. 복사본 setup.cfg 에서 포트를 6899 로 바꾸면 6899 만 열림(원본·저장소는 그대로, 임시 저장소는 삭제).
- **정정**: `R.exe` 는 자동 업데이트 도구가 아니라 **NETSTORM Root Server**(LAN 서버, 6800/6802/8998 관련)다. 관련 문서 세 곳 수정.
- **사용자 결정(2026-09-30)**: (1) 클라이언트 유효성 검사("Not Validated")는 구현하지 않는다. (2) 멀티플레이는 후순위로 나중에 구현한다. (3) 6799 방화벽 경고는 **임시로 6799 방화벽 예외를 미리 등록**해 해결한다. → 문서 반영: [network-ports.md 1-1·7·8절](docs/exe/network-ports.md), [main-menu.md](docs/screens/main-menu.md), 이 문서의 표(13단계)·9단계·5절.
- **`PREPARE.ps1`(Windows용) 방화벽 항목 추가**: 항목 "방화벽 예외 (TCP 6799)", 관리자 권한 검사(`Test-IsAdministrator`), 비관리자면 선택 화면에 "[사용 불가]"로 표시하고 `-All`·`-CheckOnly` 에서도 건너뛰어 결과에 "사용 불가"로 남김, 규칙 점검(`Get-FirewallRuleStatus`)·등록·재점검. 규칙은 인바운드 TCP 6799 허용, 프로필 Any, 원격 범위 `LocalSubnet`(`$FirewallRemoteScope` 로 변경). 기본 선택은 해제(보안 설정 변경). `PREPARE.sh` 는 손대지 않았다(Linux 는 경고 창이 없음).
- **사용자 Windows 확인(2026-09-30)**: TCP 6799 방화벽 허용 기능이 정상 동작했고, 관리자 권한이 없는 경우 해당 항목을 선택할 수 없는 것도 확인했다.
  - **추가 Windows 확인 필요**: `-All`·`-CheckOnly` 결과 / 규칙의 포트·프로필·원격 범위 / 반복 등록 시 중복 방지 / 되돌리기 명령 / Windows PowerShell 5.1·7 호환 / **게임 실행 시 방화벽 경고가 실제로 사라지는지**. 자세한 목록: [network-ports.md 7.2](docs/exe/network-ports.md).
  - 검증한 것: Linux 의 PowerShell 7.6 파서로 문법 오류 없음, 파일 UTF-8 BOM·LF 유지. 방화벽 cmdlet 을 모의한 비관리자 경로 한 번은 "사용 불가" 행을 냈으나 다른 항목이 Linux 에 없는 명령으로 실패해 검증으로 치지 않는다(PowerShell 도구는 스크래치패드에만 설치, 저장소 변경 없음).
- **부수 발견**: 시작 후 창 크기가 1600×828 → 1024×768 로 바뀌는 시점에 안내 창 클릭 좌표가 달라져 첫 클릭이 빗나갔다(이전 인수인계의 "창 크기 변경 원인 미확인"과 같은 현상). 클릭 전 `capture_state` 로 확인할 것.
- 변경 파일: `PREPARE.ps1`, `docs/exe/network-ports.md`(신규), `docs/analyze-manager.md`, `docs/screens/main-menu.md`, `docs/sources/patch-history.md`, 이 문서.

## 2026-09-30 밤 (`vm-debian-codex`, Linux, 원본 게임 실행 없음): 다리 붕괴 알고리즘을 원본 칸 단위 처리로 교체 ✅

- **공통 알고리즘 근거:** FUN_004227e0·FUN_004218b0·FUN_004217f0·탐색기 필터와 Graph 검사로 구동자를 분석했다.
  - 구동자: 표면 그래프 5칸 미만이면 즉시 제거 → 단단하면 제외 → 열린 쪽 없으면 제외(B~E 2개 이상, F~K 1개 이상, L~O 항상, A·P 안 됨) → 접합 칸(A~I)으로만 재귀한 방문 목록의 0 아닌 최소 수명 − 1(없으면 7)로 맞춤.
  - 결과: 섬에 붙은 판자 사슬은 **바깥 끝부터** 한 칸씩(관찰 "두 칸짜리는 끝 칸이 먼저", "이어 붙이면 멈췄다가 끝 칸이 사라진 뒤 다시 줄어듦"을 설명), 접합 무리는 끝 판자와 함께(끝이 둘이면 스캔마다 두 번), 섬에서 떨어진 5칸 미만 조각은 즉시 붕괴.
- **당시 기록의 정정:** 아래 round(8001 × 프레임 시간 ÷ 10)·C# 10초 고정은 2026-09-30 기록이다. 최신 cpppj는 정수 절삭·주기 종료를 원본 x86과 대조했다. [최신 복원 근거](docs/exe/cpp-bridgedecay-reconstruction.md)와 맨 위 완료 절을 우선한다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-093)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **새로 알게 된 것**
  - 방문 목록 용량 100(`0x4fc640`의 `push 0x64`), 이웃 목록 10. 금은 "이전 수명 ≥ 5, 새 수명 < 5"일 때만 가서 수명 0 인 새 칸이 4 로 바로 맞춰지면 금이 가지 않는다.
  - 스캔 주기는 `round(8001 × 프레임 시간 ÷ 10)`개씩이라 프레임률이 아주 높으면 최대 1.5배까지 늘 수 있다(클론은 10초 고정).
  - **전체 디컴파일에 없던 함수 복구법 확립**: 탐색기 필터 `FUN_004b1e80`은 가상 함수 표로만 호출되어 빠져 있었다. 새 도구 `tools/ghidra/decompile_at.sh`(Linux, 약 5초)로 디컴파일해 연결 방향 검사 `FUN_00441e40`을 찾았다 → [사용법](docs/exe/mission-header-flags.md#7-ghidra-전체-디컴파일에서-빠진-함수-재현-방법-포함). 이전 인수인계의 "Linux 에서 `DecompileAt.java` 직접 실행은 미시도"가 해소됐다.
- **아직 설명 못 한 것**: 섬에 붙은 **단일 칸이 약 160초** 걸린 관찰(모델은 80초 안팎). 8.3절에 가능성 정리. 원본에서 캡처 간격을 좁힌 재측정(사용자 직접 조작 녹화 권장)이 필요하다. → ✅ **2026-09-30 해소**: 그 관찰은 두 칸 막대였고 한 칸은 금 +32초·낙하 +72초(맨 위 절).
- **다음 후보**: (1) 위 재측정, (2) 누락 함수 후보 528개 중 게임 로직 쪽 우선 디컴파일(`decompile_at.sh` 사용), (3) 붕괴 시 칸 위 이동체 낙하(`FUN_004202f0`~`FUN_00426120`)는 이동 구현 때, (4) 다리 연결·소유권 전파(`FUN_00421240`·`FUN_004213b0`)는 영역 소유 구현 때.
## 2026-09-30 저녁 (`vm-debian-codex`, Linux, 원본 게임 실행 없음): 다리 품질·주변 약화 정적 분석 + 구현

- **공통 도구 검증:** 당시 analyzeManager 빌드는 경고·오류 0이었다. Core 143·Assets 175와 BridgeGrid 구현은 C# 이력이다. 원본 주변 약화의 제거/회수 경로, 끝 칸 변환·폭발 피해의 미확인은 원본 분석으로 구분한다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-094)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **사용자 추가 규칙(2026-09-30)**: 다리 붕괴 조건은 시간만이 아니다. **건물형 유닛이 파괴될 때 바로 옆에 인접한 다리도 1단계 약화되고, 이로 인해 붕괴될 수 있다.** → [섬 소유권 규칙](docs/gameplay/island-ownership.md) 3번, [다리 분석 8.5절](docs/exe/bridge-pieces.md#85-건물형-유닛이-없어질-때-주변-다리-약화-2026-09-30-사용자-확인--exe-확인--구현).
- **exe 확인**
  - 주변 약화: 전투 오브젝트 공통 제거 처리 `FUN_0044b9e0`(vtable 슬롯 0x18). `maxHitPoints`가 있고(타입 플래그1 0x10) walker·balloon·flyer가 아닌 오브젝트가 없어지면 중심 칸 ±2 의 다리: 금 감 → 제거, 보통 → 금 감·수명 4(`FUN_00421db0`), 단단함 → 그대로. 제거 이유를 보지 않으므로 회수에도 적용된다고 추정.
  - 조각 품질: 생산 칸 조각은 **들어온 뒤 6초 동안 금 간 품질**(타이머 0x3c × 0.1초), 그 뒤 보통. 금 간 채로 놓으면 금 간 프레임·수명 4, 보통이면 수명 0(`FUN_00442c80` `param_10`, 로컬 배치 `FUN_004433b0`은 조각 `+0x1e`). 튜토리얼 C1 "Bridge Quality" 설명과 맞다.
  - 폭발(`Bomb.cpp`) 가장자리의 판자는 끝 칸(L~O)·수명 4 로 바뀐다(`FUN_004215d0`).
  - 단일 칸 "120초 뒤 금 감" 관찰은 exe 조건으로 설명되지 않았다(모델 30~40초, 같은 측정의 서쪽 칸 51초 관찰은 모델과 일치). 재측정 필요(→ ✅ 2026-09-30 해소: 두 칸 막대였음, 맨 위 절) — [8.3절](docs/exe/bridge-pieces.md#83-원본-관찰과의-차이-남은-확인).
- **정리**: `analyzeManager/ExplorerTools.cs` `start_session` 설명에 시스템 2(`192.168.0.94`·`HJOW-Athlon`)를 반영했다(아래 2026-09-29 절의 미반영 항목 해소, `Program.cs` 도움말은 이미 일반화되어 있었음).
## 2026-09-30 후속 분석 후보 — 공식 캠페인 1-1 The War Begins!

- **공중 공격 기지 검증**: 미션 시작 지식에 `sunAviary`가 있고 Sun Workshop 등록 목록에 Whirlibase가 나타난다. 기지를 건설해 적이 사정거리 안에 들어왔을 때 공격체(Whirligig)가 생성·발진·이동·공격하는 조건과 종료 과정을 원본 캡처·타입·exe와 대조한다. 비행형 수송 유닛의 이륙·착륙과는 별개 경로인지 확인한다.
- **적 사제 포획·Altar**: 적 High Priest의 체력 절반 기절·보호막 모습, 수송 유닛으로 **포획해 데려오는 절차**(Storm Power 환급용 `Salvage`와 다름), Altar 건설·사제 희생·지식 획득 및 중단 조건을 재현한다. 기존 `Playing 5`·`Playing 8` 캡처에 제단과 희생 중으로 보이는 장면이 있다. 상세 관찰 항목: [캠페인 1-1 분석 시나리오](docs/screens/the-war-begins-start.md#0-a-후속-분석-시나리오-공중-공격-기지와-사제-포획altar).
- **실험 조건**: `thewarbegins.english`의 `aiStartMoney = 0`은 적의 **시작 자금**이며 `aiCollectors = 1`, `aiGeyserAttachments = 1`도 설정되어 있다. 따라서 이후에도 적 자금이 0으로 유지된다고 가정하지 않는다. `myStartMoney = 3000`, `myTech = "sunWalker;rainBattery;sunAviary;sunCannon"`과 이미 배치된 적 방어 유닛도 기록한다. 이번 항목은 향후 작업 문서화이며 게임 실행·구현은 하지 않았다.

## 2026-09-30 사용자 추가 규칙 — 비행형 이동·공중 공격 (문서·주석 반영 완료, 기능 구현은 후속)

- **공통 요구:** 비행 이동은 출발 때 이륙 → 이동 → 목적지 착륙 순서다. 튜토리얼 1·2에는 등장하지 않는다. 수집·수송/알타 명령의 후속 C# 구현과 주석 반영 위치는 이관했다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-096)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- 일부 **건물형 유닛**은 적이 사정거리 안에 들어오면 비행형 공격 유닛을 생성해 보내 공격한다. `Air Attack Base` 타입(Whirlibase·Devil Maker·Man o'War Pool)과 `flyer` 공격체(Whirligig·Dust Devil·Man o'War)를 구분해, 사정거리 감지·생성·목표 선택·공격체 생명주기를 후속 분석한다. 이 공격체에 공중 수송 유닛의 이륙·착륙 규칙이 동일하게 적용되는지는 미확인이다.
## 2026-09-30 (`vm-debian-codex`, Linux, 원본 게임 실행 없음): 수집 경제·튜토리얼 1 및 사제 이동·건설 정적 분석

- **공통 정적 사실:** 튜토리얼 1 FUN_004c3a20 경계는 다리 8·19칸, 200·600 SP(0x510298·0x514c34)다. bridgethegap.fort에 없는 가이저는 FUN_00486440/00486360으로 동적 생성된다.
- **공통 이동·건설:** priest.type speed=1.8은 타입 +0xe0에 파싱되며 Golem 2.0·Balloon 1.9·Sail Skater 3.4·Crystal Crab 2.4 등 타입마다 다르다. 비용 차감은 00442c80 → 00442b50; constructionRate는 +0x50, 기본값 10.0이다. 실제 속도/시간 식·가이저 위치·누적 다리 제작 판정은 추가 분석 대상이다([상세](docs/exe/priest-construction.md)). 연습 가이저·MovementRate·고정 건설 시간·313개 테스트는 C# 이력이다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-097)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

## 0-A. 최신 인수인계 — AI용 원본 분석 도구 (2026-09-29)

### 2026-09-30 (`vm-debian-codex`, Linux, 게임 실행 없음): 튜토리얼 안내 창 ✅ (후보 2번 완료)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-098)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 2026-09-30 오후 (`vm-debian-codex`, Linux, 원본 실행 없음): 튜토리얼 2 단계 처리 구현 ✅ (후보 1번 완료)

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-098)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **exe 확인(정적, 2026-09-30)** → [mission-header-flags.md](docs/exe/mission-header-flags.md) 3.5절
  - 개수 표 세 개: `DAT_005c94d0` = 내 현재 개수, **`DAT_005c98d0` = 내가 지은 누적 수**(튜토리얼 단계 조건이 읽는 것, 파괴·회수는 줄이지 않고 전체 삭제 `FUN_004c27c0`만 줄임), `DAT_005c9cd0` = 전체 현재 개수. 이전 문서의 "누적인지 현재인지 미확인"을 해소했다.
  - 타이머 상수를 exe 바이트에서 읽었다: `0x506588` = **4.0초**(단계 F), `0x506590` = **2.0초**(단계 C의 `NotVortex`·단계 H). 단계 C·F가 읽는 `FUN_004d5430` = 선택한 오브젝트 번호(`DAT_005caea0`), `FUN_004d5c60` = 선택 해제.
  - 단계 넘김(`FUN_004c33f0`)은 글자 +1·타이머 지움·잠금 카운터 10, 잠금이 풀린 뒤에만 단계 함수가 호출된다(프레임 함수 `FUN_004c34c0`).
### 2026-09-30 오전 (`vm-debian-codex`, Linux, 원본 실행 없음): 디컴파일 준비만 하고 사용자 요청으로 중단

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-098)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **한 일 (코드·문서 변경 없음, 커밋할 것 없음)**
  - 이 PC에는 디컴파일 결과가 없어 Ghidra 12.1.4(`~/Tools/ghidra_12.1.4_PUBLIC`, JDK 25)로 `originals/Netstorm.exe`를 다시 디컴파일했다. **함수 4,506개**, `extracted/decomp/Netstorm.c` 183,825줄, 소요 약 3분. `extracted/`는 Git 제외라 커밋 대상이 아니다.
  - `run_decomp.ps1`은 PowerShell 전용이고 이 PC에는 `pwsh`가 없다. 같은 일을 하는 Linux 명령(저장소 루트에서):
    ```bash
    mkdir -p extracted/ghidra extracted/decomp extracted/ghidra-settings extracted/ghidra-cache
    export XDG_CONFIG_HOME=$PWD/extracted/ghidra-settings XDG_CACHE_HOME=$PWD/extracted/ghidra-cache
    ~/Tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless extracted/ghidra Netstorm -import originals/Netstorm.exe -overwrite \
      -scriptPath tools/ghidra -postScript ExportDecomp.java $PWD/extracted/decomp/Netstorm.c
    ```
  - `python3 tools/taff.py extract originals/netstorm.tarc extracted/tarc`로 아카이브를 풀었다(246개, 튜토리얼 스크립트는 `extracted/tarc/d/tutorial1~6.english`).
  - 기준 상태 확인: Release 빌드 오류 0(기존 CA2014 경고 1), 테스트 Core 128·Assets 172 통과.
- **참고**: 이 PC(`vm-debian-codex`)는 AGENTS.md 예외 시스템 1이라 게임 실행 확인은 필요 없지만, 이번에는 실행하지 않았다.

### 2026-09-30 (`DESKTOP-HJOW`, 원본 실행 없음): 재디컴파일 · 튜토리얼 2 헤더 플래그 원인 · 게임 세션

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-098)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **재디컴파일**(사용자 요청): 두 판본을 이 PC에서 다시 디컴파일했다 — 결과는 아래 3단계 절에 기록. 패치판은 기존 결과와 SHA-256 동일, CD판은 이 PC에 없어서 새로 생성.
- **튜토리얼 2 `denySalvage = 1`·`techAllowed` 모순 원인 규명·문서화**(사용자 요청) → **[docs/exe/mission-header-flags.md](docs/exe/mission-header-flags.md)**
  - 결론: 머리 값은 **시작 상태**이고 튜토리얼 단계 처리(`FUN_004c3bb0`)가 실행 중에 바꾼다. 단계 B에서 sunFactory 허용(`FUN_004c23e0(sunFactory, 1)`), 단계 H에서 회수 금지 해제(`DAT_00595078 = 0`). 사용자 관찰(템플 뒤 Sun Workshop 건설, 마지막에 유닛 회수 75 SP)과 일치한다.
  - 기술 허용 표는 **메뉴·덱에서만** 확인한다(Construct 메뉴 항목 `FUN_00461cf0`, 생산 목록 `FUN_004752b0`, 덱 갱신, 템플의 골렘). 회수 금지는 Salvage 메뉴 명령·실행(`FUN_0044ca00`·`FUN_0044c420`)이 확인하고 스크립트 섹션 `[DenySalvage]`를 Tell하는데, 원본 스크립트에 그 섹션이 하나도 없다.
  - 이전 세션의 "`techAllowed` = 만들 수 있는 기술" 해석을 "시작 상태 + 실행 중 변경"으로 정정했다(`TechPermissions` 변경 가능화).
  - **Ghidra 전체 디컴파일에서 빠진 함수를 발견**: 이 값을 읽는 `0x484ab0`·`0x4c2b20`·`0x4c3290`이 `extracted/decomp/Netstorm.c`에 없다. INT3 패딩 뒤 함수 프롤로그 휴리스틱으로 **약 528개 후보**(실제 누락 수는 미검증). 새 도구 `tools/ghidra/decompile_at.ps1`(`DecompileAt.java`, 읽기 전용 프로젝트에서 주소를 함수로 만들어 디컴파일)를 추가했다 → [재현 방법](docs/exe/mission-header-flags.md#7-ghidra-전체-디컴파일에서-빠진-함수-재현-방법-포함), [추출 순서](docs/formats/README.md#추출-순서-처음-받은-사람용). **설정 키 문자열이 디컴파일 검색에서 안 나오면 누락 함수를 의심할 것.**
  - 튜토리얼 단계 A~I의 조건 표도 문서화했다(단계 C·F는 UI 선택·타이머 상수 의존이라 미확정).
### 게임 구현: 다리 배치 판정·붕괴, 미션 시작 조건 (2026-09-30, `HJOW-Athlon`, 원본 실행 없음)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-098)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 튜토리얼 2 사용자 직접 조작 분석 완료 (2026-09-30)

- [x] `HJOW-Athlon` 세션 `20260929T160257202Z-2e9f1861506d`에서 `2 Secret Workshop`을 사용자 직접 조작으로 완료했다. `Mission Accomplished! … Tutorial Two` 결과 화면과 3,075 SP를 PNG·세션 메모에 남겼다. 다음 튜토리얼은 열지 않았고 녹화·안내 창·게임을 종료했다. [관찰 결과](docs/screens/README.md#17-사용자-직접-조작-녹화-튜토리얼-2-완료-2026-09-30-windows-hjow-athlon).
- [x] 영상 2,751프레임(AVI 5개), 소리 WAV 3개, 입력 JSONL 1개를 저장했다. 각 AVI/WAV는 48 MB 미만이고 AVI 전부 FFmpeg 디코딩 성공, 소리도 검출됐다. 영상·오디오 길이는 약 275.1초·275.3초다.
- [x] Wind Temple·Sun Workshop 건설, Sun Disc Thrower 생산 등록·네 개 배치, 에너지 범위 확인, 하나 Salvage를 관찰했다. SP는 10,000→5,000→4,200→3,000→3,075로 변화했고, 마지막 75는 Sun Disc Thrower 원가 300의 25%다. 배치 클릭부터 완공까지 Temple 약 16초, Workshop 약 10초(이동 포함), 첫 두 유닛 배치 뒤 생산 아이콘 복귀 약 1.1~1.2초. [재충전 시간](docs/exe/production-refresh.md#실제-화면-확인-2026-09-30-튜토리얼-2).

### 튜토리얼 1 사용자 직접 조작 분석 완료 (2026-09-30)

- [x] `HJOW-Athlon`에서 세션 `20260929T154230683Z-bdf8f92d93a1`로 `Bridge the Gap!`을 사용자 직접 조작 방식으로 끝냈다. 600 SP와 `Mission Accomplished!` 결과 화면을 PNG로 저장하고 세션 메모에 연결했다. 다음 튜토리얼은 열지 않았고 녹화·안내 창·게임을 모두 종료했다. [관찰 결과](docs/screens/README.md#16-사용자-직접-조작-녹화-튜토리얼-1-완료-2026-09-30-windows-hjow-athlon).
- [x] 실제 게임에서 10 FPS 영상 2,438프레임을 AVI 4개, 소리를 WAV 2개, 마우스·키 입력을 JSONL 1개에 저장했다. 모든 AVI/WAV는 48 MB 미만이고 FFmpeg로 읽혔다. 영상·오디오 길이는 각각 약 243.8초·243.9초다. 이전 RDP 화면 가림으로 0프레임에 그친 사례와 달리, RDP 화면이 보이는 상태에서 녹화가 정상 동작했다.
- [x] 가이저 연결 뒤 사제의 결정 반납으로 SP가 0→200→400→600이 되고 튜토리얼 1 완료 창이 뜨는 것을 확인했다. 사용자가 `Bridge Quality` 설명은 빠르게 넘겼다고 알렸지만, 이후 영상에서 연결되지 않은 오른쪽 다리 끝이 입력 없이 조각나 사라지는 장면은 확인했다. 개별 금 감 시작 시각과 일반적인 붕괴 시간은 이 영상만으로 확정하지 않는다.

### 사용자 직접 조작 녹화 기능과 PNG 한도 수정 (2026-09-29~30)

- 기존 CLI/MCP AI 조작을 유지하며, 실행 중인 세션에 `guide --session ID --steps-file UTF8파일`로 붙는 Windows 안내 창을 추가했다. 단계 표시·전후 이동·중단 후 재개, 게임 화면 10 FPS MJPEG AVI, 기본 출력 장치 루프백 WAV, 마우스/키보드 JSONL 기록을 지원한다. 영상·소리는 48 MB 전에 분할하고 각 조각의 시각 파일을 남긴다. 상세 사용법은 [docs/analyze-manager.md](docs/analyze-manager.md) "사용자 직접 조작 녹화 모드" 절.
- 기존 PNG의 세션당 500장/200 MB 제한은 제거했다. 개별 파일 50 MB 미만, 이벤트 10,000개 한도는 유지한다. 아래의 500장 도달 기록과 캡처 절약 지침은 **변경 전 당시 상황**이다.
- Release 빌드 통과(오류·경고 0), 단위 테스트 23개 중 22개 통과·1개 건너뜀(심볼릭 링크 생성 권한 없음), 게임 없는 MCP 기본 검사 9개 도구 통과. Windows 기본 출력 장치 2초 녹음은 약 0.7 MB WAV 생성 확인. AVI 조각·재개와 501장 PNG 저장은 단위 테스트로 확인했다.
- 짧은 실제 세션 `20260929T145451853Z-49cc0790f162`에서 안내 창은 열렸지만, RDP 화면에서 게임 창 전면 확보가 실패하고 화면 가림 검사에 걸려 영상이 0프레임에서 중단됐다. 이 과정에서 오디오 장치 위치의 절대값을 잘못 해석해 43 MB 무음을 만든 문제를 발견해 첫 패킷 기준으로 수정했으며, 위 2초 단독 녹음으로 크기를 재확인했다. 게임은 `end_session force=true`로 종료했고 안내 창도 닫았다. **실제 영상 프레임·사용자 입력·AVI 재생은 2026-09-30 튜토리얼 1 세션에서 확인했다(위 절).**
- **테스트 산출물 정리 완료(2026-09-30):** 사용자 요청에 따라 위 테스트 세션, 단독 오디오 검사 폴더, 테스트용 JSON·안내 파일을 삭제했다. 다른 분석 세션은 보존했다.

### 원본 동적 분석 시간 계획 (2026-09-29 22:45~, `HJOW-Athlon`) — 사용자 요청으로 23:23 중단

**결과 요약** — 자세한 측정은 [docs/screens/README.md](docs/screens/README.md) 1.5절. 세션은 `20260929T133717181Z-7f413cde9896`이며, 도구의 세션 이미지 한도(500장)에 걸려 튜토리얼 1 D 단계에서 종료(`end_session force=true`)했다. 새 세션은 시작하지 않았다.

- **완료한 항목:**
  - 로딩 창 Cancel: 반응 없음(두 번 시도, 데모 그대로 시작)
  - Campaign 창 `$Timeout=120`: 실측 120.1초에 닫힘 → 45.2초 뒤 Auto-Demo
  - Auto-Demo 안내 창: 뜬 뒤 14.7초에 저절로 닫힘(`$Timeout=15`). 이전에 "안 뜬다"고 기록한 것은 캡처 간격 때문으로 보인다.
  - Restart Demo: 로딩 창을 거쳐 처음부터 다시 시작, 안내 창 다시 뜸
  - Original Credits: 이 세션에서는 20초마다 넘어가는 7번의 전환만 측정했다. 마지막 쪽 뒤 흐름은 위 2026-09-30 Windows 세션에서 확인했다.
  - Create New Map: "New Save-Game Name?" 입력 창(Cancel 함)
  - Test Battle:
    - 창 제목 `NetStorm Test Battle "<미션 제목>"`, 브리핑 창
    - Game 메뉴 맨 위에 **Return to Editing** → 편집기로 돌아감
  - 튜토리얼 1:
    - 다리 칸 약 1초마다 채움
    - 칸 그림은 시간이 지나도 불변
    - 섬 윗 가장자리(초목)는 배치 불가, 오른쪽 물가 옆은 가능
    - 가능할 때 섬 쪽 짧은 연장이 보임
- **한쪽만 붙은 다리의 붕괴:**
  - ~~단일 칸~~ **두 칸 막대**(2026-09-30 정정): 놓은 뒤 +40초 바깥 칸 금 → +80초 낙하 → +120초 안쪽 칸 금 → +160초 낙하. 기록 당시엔 "단일 칸 약 120초 보통 → 금 간 그림(K11) 약 40초 → 사라짐(총 약 160초)"로 잘못 읽었다.
  - 두 칸짜리는 끝 칸이 먼저 사라짐
- **튜토리얼 진행:** B → 다리 몇 개 → C(q·w·a·s 칸 선택 안내) → C1 → 몇 개 더 → D. **가이저는 D 단계에서야 생김**(.fort 에 없음).
- **기타:** F2 = 큰 물체 투명. ALT 스크롤은 매우 빠름.
- **다음에 이어서 할 일:**
  - [x] 새 세션에서 튜토리얼 1을 처음부터 진행(저장 불가): 가이저 연결 → 사제 결정 왕복·결정당 SP → 600 SP 결과 화면 (2026-09-30 사용자 직접 조작 녹화로 완료, 위 절)
  - [x] 튜토리얼 2 건설 시간·배치 뒤 아이콘 복귀·완료 결과 (2026-09-30 사용자 직접 조작 녹화로 완료, 위 절)
  - [x] Leave → Replay Mission 흐름 — 2026-09-30 Windows 자동 분석에서 같은 미션 첫 브리핑 재진입 확인
  - [x] Credits 마지막 쪽 뒤 흐름 — 2026-09-30 Windows 자동 분석에서 메인 메뉴 복귀 확인
  - 캡처를 아끼도록 좁은 영역·긴 간격으로 측정한다(이미지 한도 500). 배치 가능 위치 탐색은 한 번에 캡처가 많이 쌓이므로 한 세션에서 오래 하지 않는다.
  - 배치 판정 분석([bridge-pieces.md](docs/exe/bridge-pieces.md) 8절)에 위 붕괴 시간을 기준값으로 쓴다.
- 이번 변경 파일(커밋 전): `docs/screens/README.md`(1.5절), `docs/exe/bridge-pieces.md`(4절 붕괴 관찰), `LEFT_JOBS.md`

**원래 계획 (참고)**

원본은 **진행 저장 기능이 없다**(Edit 모드 맵 저장만 가능, 사용자 확인). 미션 상태가 필요한 측정은 한 번의 진행 안에서 순서대로 한다.

| 시각(대략) | 단계 | 측정·확인 |
|---|---|---|
| 22:45~22:48 | 0. 시작, Demo → 로딩 창 Cancel | Cancel 결과 |
| 22:48~22:53 | 1. Campaign 창 방치 | `$Timeout=120`(120초) → Auto-Demo 45초 → 안내 창 여부 → Restart Demo → Exit Demo |
| 22:53~22:56 | 2. Original Credits | 20초 자동 넘김, 끝 흐름 |
| 22:56~23:00 | 3. Edit | Create New Map, Test Battle (저장 없이 나감) |
| 23:00~23:25 | 4. 튜토리얼 1 한 번 진행 | 다리 칸 채움 간격, 칸 조각 금 감→단단함 시간, 다리 배치(섬 가장자리·연결 표시), 연결 안 된 다리 붕괴 시간, 사제 결정 왕복·결정당 SP, 600 SP 결과 화면 |
| 23:25~23:45 | 5. 튜토리얼 2 (Unit Rate Fast) | 템플·워크샵 건설 시간, 생산 등록, 유닛 배치 뒤 아이콘 복귀(1초 예상), Leave → Replay Mission |
| 23:45~ | 6. 문서 반영 | |

### Windows 원본 동적 확인 (2026-09-29 저녁~밤, `HJOW-Athlon`) — ✅ 메인 메뉴 흐름 조사 완료

- **실행 상태:**
  - 지정 예외 시스템 2(Windows 10 Pro)라서 확인 없이 원본 복사본을 실행했다. 세션은 네 개다.
    - `…113420484Z-3358366e5f90`: 사용자 중단(작업 전 문서 확인 지시)
    - `…113907044Z-929836edfdde`: 사용자 중단(문서 정리 지시)
    - `…114913277Z-391e6a079ff0`: 원격 데스크톱 창이 최소화되어 포커스·캡처 실패
    - `…120526716Z-327849a23cac`: 본 조사
  - 마지막 세션은 `end_session force=true`로 종료했다. 실행 중인 `Netstorm.exe`는 없고 `originals/` 변경은 없다.
  - 증거는 `extracted/analyzeManager/<세션>/`에 있다(git 제외).
  - Technical Help를 누른 결과로 Windows 도움말 앱 `HelpPane.exe`와 Edge 창이 남아 있을 수 있다(사용자에게 알림).
- **원격 데스크톱 주의:** `HJOW-Athlon`은 RDP 세션으로 쓰고 있다. **RDP 창이 최소화되면 캡처·포커스가 실패한다** → 분석 중에는 RDP 창을 띄워 두어야 한다([docs/analyze-manager.md](docs/analyze-manager.md) "RDP" 절).
- **확인한 원본 동작** → [docs/screens/README.md](docs/screens/README.md) 1.2·1.3절, [docs/screens/main-menu.md](docs/screens/main-menu.md) "메뉴 흐름도":
  - **보유 exe = 10.78** (Help → Version 창 "Version v10.78 / 10.78 Patch by Ticonderoga Entertainment") → [docs/sources/README.md](docs/sources/README.md) 2절 갱신
  - Auto-Demo는 **마지막 입력(마우스 이동 포함)과 대화상자가 닫힌 시점 중 늦은 쪽부터 45초** 뒤에 시작한다. 대화상자가 열린 동안은 시작하지 않는다.
  - 로딩 창은 "Starting Mission..." 또는 "Connecting to Game Server - Countdown N"(Cancel)이다. Auto-Demo 안내 창은 뜰 때와 안 뜰 때가 있다(조건 미확인). 선택 데모의 안내 창은 `$Timeout=15`대로 15초 뒤 자동으로 닫힌다.
  - ESC는 데모·미션 중 상단 메뉴 막대를 켜고 끈다(브리핑 창이 열려 있으면 무반응). 메뉴 막대 구성:
    - 데모 Game: Restart/Exit Demo, Quit Game
    - 미션 Game: Review Objectives F8, Restart/Leave Mission, Quit Game
    - 미션 View(F2~F9)·Options·Players·About, 편집기 Game·Edit·View·Options·About
  - Exit Demo는 확인 없이 메인 메뉴로 간다. Leave Mission은 확인 창(Main Menu / Replay / Continue)을 거쳐 메인 메뉴로 간다. 편집기에서 나갈 때는 저장 확인(Yes / No / Cancel)이 뜬다.
  - **브리핑 창이 떠 있는 동안에도 게임이 진행된다**(다리 조각이 참).
  - 도움말 드래그는 1:1이다. 38px는 스크롤 끝이다.
  - Credits 각 쪽은 20초마다 자동으로 넘어간다. Campaign 버튼은 `[UCampaign]`을 연다. Edit에서 맵을 고르면 편집기 모드로 들어간다.
  - 메인 메뉴 버튼은 커서를 먼저 올린 뒤 클릭해야 반응한다. `>` 하위 메뉴는 클릭해야 열린다.
- **남은 원본 확인 (다음 작업 후보, 지정 시스템에서 확인 없이 실행 가능):**
  1. Auto-Demo 안내 창이 뜨는 조건과 45초 상수의 exe 위치(정적 분석)
  2. Campaign 나머지 묶음 확인, Replay/Restart Mission 뒤 배치물 초기화 확인 (두 명령의 첫 브리핑 재진입·Restart Demo·로딩 창 Cancel·Original Credits 마지막 전환·Campaign `$Timeout=120`은 후속 세션에서 확인 완료)
  3. 편집기 Create New Map·Test Battle·Add Island·Set All Bridge
  4. 미션 플레이 관찰: 사이드바 배치 조작, Unit Rate 간격 측정([production-refresh.md](docs/exe/production-refresh.md) 남은 일), 결과 화면 흐름
  5. 메인 메뉴 Options의 Pause 줄 표시 조건(`DAT_00594fa4`·`DAT_005c85a4`)
- 도구 쪽 미검증: 파이프 EOF(CLI 출력을 파일로 받아 확인하지 못함). `analyzeManager/ExplorerTools.cs`·`Program.cs` 도움말의 시스템 2 미반영은 아래 절 그대로다.
- 이번 변경 파일(커밋 전): `docs/screens/README.md`, `docs/screens/main-menu.md`, `docs/analyze-manager.md`, `docs/sources/{README,game-manual,patch-history}.md`, `LEFT_JOBS.md`

### AGENTS.md 게임 구동 허용 조건 추가 반영 (2026-09-30)

- AGENTS.md에 예외가 하나 더 생겼다: **개발자(사용자)가 기존 게임 수동 컨트롤 방식으로 분석 진행을 직접 요청한 경우, 해당 작업 단계에서는 시스템과 무관하게 실제 게임 구동 확인을 받지 않아도 된다.** (기존 예외: 시스템 1·2.) 요청이 없거나 다음 작업 단계로 넘어가면 일반 규칙(목적·필요성 설명 후 명시적 확인)으로 돌아간다.
- 반영: [docs/analyze-manager.md](docs/analyze-manager.md) "실제 게임 실행 전 개발자 확인", [analyzeManager/README.md](analyzeManager/README.md), `analyzeManager/{ExplorerTools,Program}.cs`의 `start_session` 설명·CLI 도움말 문구. 문자열만 바꿨으므로 재빌드는 하지 않았다(다음 빌드에서 확인 필요).

### AGENTS.md 규칙 갱신 반영 (2026-09-29)

- **`AGENTS.md`는 절대 수정하지 않는다.** 수정이 필요하면 개발자(사용자)에게 요청한다. (이전 인수인계에 "`AGENTS.md` 갱신"으로 남은 기록은 이 규칙이 생기기 전의 작업이다.)
- **게임 구동 확인 예외 시스템이 2개로 늘었다.** 아래 시스템에서는 `analyzeManager` 등으로 실제 게임을 구동할 때 개발자 확인을 받지 않아도 된다. 식별값이 확인되지 않거나 다르면 일반 시스템 규칙(실행 전 목적·필요성 설명 후 명시적 확인)을 따른다.

  | 구분 | IP | 호스트명 | 환경 |
  |---|---|---|---|
  | 시스템 1 | `10.0.0.15` | `vm-debian-codex` | Debian 13 + Wine 10.0 |
  | 시스템 2 | `192.168.0.94` | `HJOW-Athlon` | Windows 10 Pro (이 저장소 작업 PC, 2026-09-29 호스트명·IP 확인) |

- 시스템 2가 Windows이므로 아래 "다음 작업"의 **Windows 실제 GUI 재확인**(캡처, 파이프 상속 방지 효과, CLI 실행 뒤 게임 유지 등)을 별도 확인 없이 이 PC에서 진행할 수 있다.
- 반영 문서: [docs/analyze-manager.md](docs/analyze-manager.md) "실제 게임 실행 전 개발자 확인", [analyzeManager/README.md](analyzeManager/README.md). ~~**미반영:** `analyzeManager/ExplorerTools.cs`의 `start_session` 도구 설명과 `analyzeManager/Program.cs` CLI 도움말은 아직 시스템 1만 적고 있다(코드 수정·재빌드 필요).~~ → **2026-09-30 해소**: `start_session` 설명에 시스템 2 반영, `Program.cs` 도움말은 "AGENTS.md에 지정된 시스템"으로 이미 일반화되어 있었다.

### Linux/Wine 실제 실행 테스트 (2026-09-29 16시, `vm-debian-codex`) — 입력 전달 확인, 캡처는 X11 필요 → 아래 절에서 캡처 해결

- **실행 방식:** 사용자가 Claude Code 권한 방식을 Manual로 바꾸고 실행 테스트를 요청했다. 권한 창 조작 때문에 포커스가 VS Code로 가므로, 한 번 허용한 스크립트 안에서 게임 시작 → X11 활성 창 대기 → 입력 → 종료까지 진행했다. 사용자에게는 포커스용 클릭을 게임 그림 밖(오른쪽 검은 영역)에만 하도록 요청했다. 실행 2회(1회는 스크립트 실수로 X11 캡처 실패 — 제목 검색이 다른 X 창 id를 고름, 활성 창 id를 쓰도록 고쳐 재실행). 두 세션 모두 `end_session`으로 종료, 실행 중인 `Netstorm.exe` 없음, `git status` 깨끗(`originals/` 변경 없음).
  - 세션: `20260929T070136750Z-c11bc8b4f280`(캡처 비교), `20260929T070536517Z-6fa3657cf3de`(입력). X11 캡처 증거는 각 세션 폴더 `x11/*.png`. 사용한 스크립트는 세션 임시 폴더에 있었고 저장소에 넣지 않았다.
- **확인한 것(확실):**
  - `renderer=gdi`로도 **도구 캡처(Wine GDI 화면 복사)는 전부 (0,0,0)** → 이 대책은 효과 없음. 도구의 `changedRatio`·`wait_for_change`는 Wine에서 쓸 수 없다.
  - **X11 창 직접 캡처(`import -window <활성 창 id>`)는 정상.**
  - `linux-wine.sh call start_session`(임시 파일 방식)은 곧바로 반환하고 이후 CLI 호출로 세션을 이어 쓸 수 있다.
  - 게임 창은 뜨자마자 X11 활성 창이 되며 도구 포커스 검사 통과. 창은 1600×828 최대화 상태로 남고 게임은 왼쪽 위 1024×768에만 그린다.
  - **도구 입력이 게임에 전달된다:** 팁 창 OK (661,436) 클릭 → "Not Validated" 창 → OK (511,464) → 메인 메뉴. 메인 메뉴 버튼 위치는 Windows 측정값과 같다.
  - Windows 절의 재확인 대상 중: **메인 메뉴에서 F1 → 도움말 창 "NetStorm Instructions"가 열린다**(입력 1.5초 뒤 캡처). **도움말 창에서 ESC(80ms 누름)는 창을 닫지 않았다**(1초 뒤 화면 동일).
  - 일반 `end_session`(WM_CLOSE)은 도움말 창이 열린 상태에서도 확인 창 없이 종료했다.
- **다음 작업:** → 1·2는 아래 "Wine 캡처 경로 구현" 절에서 진행함.

### Wine 캡처 경로 구현 + 재확인 대상 조사 (2026-09-29 16시 10분, `vm-debian-codex`) — ✅ 캡처 해결

- **도구 수정(`analyzeManager/WindowsGame.cs`, `SessionStore.cs`, 테스트):** `ntdll!wine_get_version`으로 Wine을 감지해, Wine에서는 게임 **창 자체 DC를 BitBlt로 복사**(`wine-window-dc`)하고, 전부 검으면 `PrintWindow(PW_CLIENTONLY)`(`wine-printwindow`), 둘 다 검으면 `wine-black`으로 표시한다. Windows는 기존 화면 복사(`screen`)를 함수로 분리만 했다. 증거 항목에 `method` 필드를 추가했다(기본값 `screen`, 기존 JSON과 호환). 단위 테스트에 `method` 전달 확인을 추가했다.
  - Linux에서 Wine 쪽 X11 명령 실행(`cmd /c Z:\usr\bin\import`)은 Wine 10에서 되지 않아(ELF 실행 불가) 이 방식은 버렸다.
- **검증(확실):** `linux-wine.sh setup` 빌드 오류/경고 0, Wine 단위 테스트 20개 중 19개 통과·1개 건너뜀(기존과 같은 링크 테스트). 실제 실행 1회(세션 `20260929T071149864Z-507f034c3833`): 캡처 33건 전부 `wine-window-dc` 성공, 같은 순간 X11 캡처 9쌍과 **픽셀 단위로 동일**. `changedRatio`·`wait_for_change` 정상. 정상 종료, 실행 중인 `Netstorm.exe` 없음, `originals/` 변경 없음.
- **당시 미검증:** Wine 캡처 변경 직후에는 Windows 빌드·테스트·실제 캡처를 다시 하지 않았다. Wine 기본(OpenGL) 렌더러에서의 창 DC 캡처도 미확인(`renderer=gdi` 유지). Windows 빌드·테스트는 아래 후속 검증에서 처리했다.
- **2026-09-29 Windows 게임 없는 후속 검증:** `dotnet build analyzeManager/AnalyzeManager.csproj -c Release --no-restore` 오류·경고 0, `dotnet test analyzeManager/tests/AnalyzeManager.Tests.csproj -c Release --no-restore` 20개 중 19개 통과·1개 건너뜀(심볼릭 링크 생성 권한 없음), `mcp_smoke.py` 기본 모드 통과(프로토콜 2025-03-26, 도구 8개, EOF 종료). 이 결과로 위 **Windows 빌드·단위 테스트 미검증은 해소**했다. 샌드박스의 `obj` 및 `desktop.lock` 접근 거부 때문에 각 검사는 승인된 샌드박스 밖 권한으로 재실행했다. **Windows 실제 캡처는 이번에 검증하지 않았다.** 원본과 복사본 게임은 실행하지 않았다.
- **파이프 상속 정적 수정(게임 실행 없음):** `AnalysisEngine.StartAsync`의 게임 시작을 `UseShellExecute=false`와 표준 입출력 리디렉션으로 바꾸고, 입력을 닫으며 출력·오류를 비우도록 했다. Wine에서 관찰된 원본 게임의 호출자 stdout 파이프 상속을 끊으려는 변경이다. 변경 후 Windows Release 단위 테스트 20개 중 19개 통과·1개 건너뜀, MCP 기본 검사 통과. 이 검사는 실제 `start_session`을 호출하지 않으므로 **Windows·Wine에서 게임 실행 수명과 파이프 EOF 개선 여부는 아직 검증하지 않았다.**
- **원본 동작 확인(확실, [docs/screens/README.md](docs/screens/README.md) 1.1절):** 팁 창은 본문 클릭·ESC에 반응 없음. F1 → 도움말. 도움말 ESC로 안 닫힘. 도움말 100px 드래그 → 38px 스크롤, 화살표 1회 → 5px. Auto-Demo는 메뉴에서 마지막 입력 후 약 45초에 "The Storm Rages!" 자동 시작, 시작 직후 "NetStorm Demo" 안내 창(OK 약 (554,457))은 ESC로 안 닫힘.
- **관찰했지만 원인 미확인:** 데모 중 두 번째 ESC 호출 시점에 창이 1600×828(최대화)에서 1024×768로 바뀌었다. 첫 ESC 결과 기록(07:13:18.8)까지는 1600×828, 두 번째 입력 요청(07:13:20.3)에서 1024×768이었고 두 시점 모두 `foreground=true`라 도구 `FocusAsync`의 `ShowWindow(SW_RESTORE)`는 호출되지 않았을 것으로 본다. 게임 자체(데모 진입 후 창 크기 재설정)나 창 관리자가 원인일 수 있다.
- **다음 작업:**
  1. ~~Windows에서 빌드·단위 테스트·`mcp_smoke.py` 기본 모드로 회귀 확인~~ → 위 게임 없는 후속 검증에서 완료. Windows 실제 캡처 확인은 사용자 확인 후.
  2. 남은 재확인 대상: 데모 안내 창 OK 후 ESC 메뉴 막대·Game 드롭다운, Auto-Demo 대기 시간 기준(마지막 입력/메뉴 표시), 도움말 38px가 드래그 비율인지 스크롤 끝인지(작은 드래그로 확인), 창 크기 변경 원인.
  3. 메뉴 흐름 조사(Campaign·Demo·Help·Edit·Credits·Options 하위 화면; Multiplayer·전체화면 제외)를 Linux에서 도구로 진행.
  4. 파이프 상속 방지 코드를 적용했다. 실제 게임 실행이 허용되는 단계에서 Wine의 파이프 연결 CLI 호출과 MCP 서버 종료 후 EOF를 재확인한다.
  5. 게임 실행 없이 `screenShots/mainMenu*.png`와 추출 `tell.english`·`offical1~6.english`를 대조해 [메뉴 좌표·정적 이동 경로](docs/screens/main-menu.md)를 작성했다. 개별 항목의 클릭 결과는 위 2·3번 동적 확인에 남긴다. 크레딧 제목의 10.72를 보유 exe 버전으로 보던 기존 화면 문서의 오류도 정정했다.

### Linux/Wine 분석 도구 실행 (2026-09-29 저녁, `vm-debian-codex`) — 권한 거부로 중단 → 위 절에서 이어서 진행함

- **실행 상태:** 지정 예외 시스템이라 확인 없이 게임을 한 번 실행했다(세션 `20260929T061418633Z-0ab98c5681a7`, `end_session force=true`로 종료, 실행 중인 `Netstorm.exe` 없음, `originals/` 변경 없음). 이후 **두 번째 게임 실행과 X11 창 직접 캡처는 Claude Code 자동 모드 권한 분류기가 거부**했다. 우회하지 않고 멈췄다. 다음 실제 실행에는 사용자가 Claude Code 권한 규칙(예: `bash analyzeManager/linux-wine.sh call …`, `wine …` 허용)을 추가하거나 직접 허용해야 한다.
- **도구 수정:**
  - `SessionStore.RejectReparse`: 드라이브 루트는 검사에서 뺐다. Wine이 `Z:\`(리눅스 루트)를 링크로 보고해 실제 저장소 경로가 전부 거부되던 문제다. 중간 경로의 링크는 계속 거부한다. Wine은 마운트 지점(tmpfs `/tmp` 등)도 링크로 보고하므로 그 아래 저장소는 여전히 거부된다.
  - 단위 테스트 2개 추가(`AcceptsDriveRootAndOrdinaryPaths`, `RejectsLinkedParentDirectory` — 링크를 못 만드는 환경에서는 건너뜀).
  - `tests/mcp_smoke.py`에 `--wine` 선택 추가(Linux에서 Windows 전용 `CREATE_NO_WINDOW`를 쓰지 않음).
  - 신규 `analyzeManager/linux-wine.sh`: `setup`(win-x86 배포 → `extracted/wine/`, 32비트 접두 경로, 렌더러 gdi) · `test` · `smoke` · `call`.
- **게임 없는 검증(확실):** Linux Release 빌드 오류/경고 0. Wine에서 단위 테스트 20개 중 19개 통과·1개 건너뜀(Wine은 심볼릭 링크 생성을 오류 없이 무시). `mcp_smoke.py --wine` 기본 모드 통과. `linux-wine.sh`의 게임 없는 하위 명령 전부 동작. **Windows에서의 빌드·테스트는 이번 변경 후 다시 돌리지 않았다.**
- **실제 실행 1회에서 확인한 것(확실):** Wine에서 복사본 준비(약 2초), 게임 시작, 창 찾기·포커스·캡처 호출 성공, 다른 CLI 호출로 같은 세션 이어서 캡처, 강제 종료. 창 제목이 `Activision and Titanic Entertainment Present: NetStorm` → `NetStorm Main Menu`로 바뀌었다.
- **문제 1 — 캡처가 검은 화면:** 시작 직후(1600×828, 최대화 상태) 캡처와 7분 뒤(1024×768) 캡처가 모두 검은색이었다. 원인은 Wine의 기본 DirectDraw(wined3d/OpenGL) 출력이 GDI 화면 복사에 잡히지 않기 때문으로 **추정**한다. 대책으로 전용 접두 경로에 `HKCU\Software\Wine\Direct3D` `renderer=gdi`를 설정했으나 **그 뒤 실행이 거부되어 효과 미확인.**
- **문제 2 — 파이프 상속:** `start_session` 출력을 파이프로 받으면 셸이 게임 종료 때까지 기다렸다(게임이 도구의 표준 출력을 물려받음). `linux-wine.sh call`은 임시 파일로 받는다(이 방식의 `start_session`은 미검증). MCP 모드에서도 같은 문제로 호스트가 EOF를 늦게 받을 수 있다.
- **다음 작업 (실제 실행이 허용된 뒤):**
  1. `bash analyzeManager/linux-wine.sh call start_session '{"label":"…"}'` → 몇 초 뒤 `capture_state`로 gdi 렌더러에서 화면이 보이는지 확인. 여전히 검으면 Wine 가상 데스크톱(`explorer /desktop=…,1024x768`)이나 X11 창 캡처(XGetImage) 백엔드를 검토한다.
  2. 캡처가 되면 입력(클릭·ESC) 전달과 화면 변화를 확인하고, 아래 Windows 절의 "재확인 대상"을 Linux에서 진행한다.
  3. 파이프 상속: 게임 시작 시 표준 입출력을 넘기지 않는 방법(예: `UseShellExecute=false` + 표준 핸들 리디렉션 후 닫기)을 Windows·Wine 양쪽에서 확인 후 적용.
- 이번 변경 파일(커밋 전): `analyzeManager/SessionStore.cs`, `analyzeManager/tests/SessionStoreTests.cs`, `analyzeManager/tests/mcp_smoke.py`, `analyzeManager/linux-wine.sh`(신규), `analyzeManager/README.md`, `docs/analyze-manager.md`, `LEFT_JOBS.md`

### Windows 실제 실행 검증 (2026-09-29 오후, 사용자 요청으로 중단)

- **실행 상태:** 사용자가 이번 단계의 실제 게임 실행을 허용했다(시스템 `DESKTOP-HJOW`, 지정 예외 시스템 아님). 이후 사용자 요청으로 테스트를 중단했고, 곧바로 `end_session force=true`로 세션 게임을 종료했다. 실행 중인 `Netstorm.exe`가 없고 `originals/`가 변경되지 않았음을 확인했다.
- **실제 게임 테스트는 나중에 한다(사용자 결정).** 실행 중에 사용자가 동시에 마우스를 조작한 경우가 있었다. 그래서 실행 중 화면 변화 가운데 **도구 입력의 결과라고 단정할 수 없는 관찰은 결과에서 뺐다**(아래 "재확인 대상"). 다음에 실행할 때는 사용자 확인을 다시 받고, 실행 중 사용자 입력이 없는 상태에서 진행한다.
- **게임 없는 검증(확실):** Windows Release 빌드 오류/경고 0, 단위 테스트 **18개 통과**(Linux에서 추가한 손상 PNG 테스트 포함), `mcp_smoke.py` 기본 모드 통과. 코드 수정 후에도 다시 통과했다.
- **실제 실행으로 확인된 도구 동작(확실)** — 세션 `20260929T060009857Z-a78447df7657`·`…060326099Z-6e78f68fa740`·`…060406149Z-ac08dc951f43`, 증거는 `extracted/analyzeManager/<세션>/`:
  - Claude Code Bash에서 CLI `start_session`을 호출하고 CLI 프로세스가 끝난 뒤에도 게임 프로세스가 유지되었다. `game_status`·`capture_state`로 이어서 조작할 수 있었다.
  - `mcp_smoke.py --live` **통과**: CLI로 실행 → MCP 재접속·상태 확인, PNG 이미지 콘텐츠 응답, 입력 전달, `wait_for_change` 시간 초과 응답, 관찰 기록, 강제 종료.
  - 일반 `end_session`(WM_CLOSE)으로 메인 메뉴 상태의 게임이 확인 창 없이 종료되었다. `end_session force=true`도 해당 세션 게임만 종료했다.
  - 포커스를 잃었을 때 도구가 입력을 보내지 않고 오류를 반환했다(안전장치 동작).
- **도구 수정(`analyzeManager/WindowsGame.cs`):**
  - 첫 `start_session`이 "게임 창의 포커스를 얻지 못했습니다"로 실패했다. `FocusAsync`에 `AttachThreadInput` 방식과 Alt 키 신호 방식을 차례로 쓰는 전면 전환 재시도를 추가한 뒤 이후 캡처·입력의 포커스 획득은 성공했다. 실패 원인은 Windows의 백그라운드 전면 전환 제한으로 추정한다.
  - `CheckForeground`에 200ms 재확인을 추가했다. 끝내 실패하면 당시 전면 창의 handle·pid·제목을 오류에 남긴다. 이 정보로 마지막 실패 때 전면 창이 VS Code였음을 확인했다.
- **주의(사용자 확인):** 도움말 본문의 파란 글자는 외부 링크다. 도움말에서 (500,300)부터 드래그한 입력이 "Netstorm:HQ's Web Site" 링크 줄 위였고, 사용자가 NetstormHQ 홈페이지가 다른 브라우저로 열렸다고 확인했다. 도움말 창에서는 링크 글자가 없는 빈 영역에서만 클릭·드래그한다.
- **원본 화면에서 확인한 사실(정지 캡처 기준, 확실):**
  - 도구가 준비한 복사본의 시작 화면은 세 세션 모두 같았다. Activision 시작 화면 → 메인 메뉴 위의 "Did You Know?" 팁 창(Prior Tip / Next Tip / No More Tips / OK) → 그 뒤의 "Not Validated" 창(자동 업데이터 검증 실패 안내: 멀티플레이 불가, 싱글·LAN 가능, OK 버튼).
  - **메인 메뉴 버튼 좌표** (1024×768 클라이언트, 정지 캡처에서 버튼 테두리 선(밝기 52)으로 측정): 버튼 75×19px, 가로 간격 79, 세로 간격 23. 윗줄 y 311~329는 Campaign x 356~430, Multiplayer 435~509, Demo 514~588, Help 593~667. 아랫줄 y 334~352는 Edit·Credits·Options·Quit(같은 x). 묶음 가로 중심 ≈ 512. 증거: 세션 `…060406149Z-ac08dc951f43`의 `screens/0fba1e0f784c316746bc38c72b7a6823fb57b6fd1e54ee5cb2e1742fa6e26fc7.png`
  - 도움말 창 "NetStorm Instructions"의 모양: 창 약 (288,40)~(738,390), 본문의 파란 링크 목록(GAME HELP / GAME SUPPORT / GAME INFORMATION), 오른쪽 스크롤바, Back·OK 버튼. 증거: 세션 `…060009857Z-a78447df7657`의 `screens/235ec5d3….png`
- **재확인 대상 (사용자 동시 조작 가능성 때문에 결과로 쓰지 않음):**
  - 메인 메뉴에서 F1이 도움말 창을 여는지. 원본 도움말 메뉴 표기 "General Help - F1"과는 맞지만 이번 실행만으로는 단정하지 않는다.
  - 메인 메뉴에서 데모 전투 화면으로 바뀐 것이 Auto-Demo 자동 시작인지 다른 입력 때문인지.
  - 데모 중 ESC 뒤 화면 맨 위 메뉴 막대와 Game 드롭다운(Restart Demo / Exit Demo / Quit Game)이 보인 것이 ESC 때문인지. 드롭다운 항목 자체는 캡처에 보였다.
  - ~~팁 창이 본문 클릭·ESC에 반응하지 않는지~~ → **사용자 확인(2026-09-29): 안내 창(Demo 안내 창 포함)에서는 ESC가 동작하지 않는다.** 남은 것: 도움말 드래그·스크롤 화살표의 스크롤 양.
- **다음 작업 (사용자가 실제 게임 테스트 재개와 실행을 확인한 뒤):**
  1. 위 재확인 대상을 사용자 입력이 없는 상태에서 다시 확인한다.
  2. 메뉴 흐름 조사: Campaign·Demo·Help·Edit·Credits·Options 하위 화면의 좌표와 전환 기록 → `docs/screens/main-menu.md`, 5단계 "메뉴 흐름도", 9단계 UI 기준. Multiplayer(네트워크)·전체화면 항목은 누르지 않는다.
  3. `analyzeManager` 개선 후보(게임 실행 없이 가능): 복사본 설정에서 `autoDemo`·시작 팁을 끄는 선택 옵션, 실행 중 사용자 입력과 섞이지 않도록 안내 문구 추가.
- 이번 변경 파일(커밋 전): `analyzeManager/WindowsGame.cs`, `docs/analyze-manager.md`, `LEFT_JOBS.md`

### 이전 인수인계 (2026-09-29 오전)

**실행 확인 규칙(사용자 요청):** 일반 시스템에서 이 자동 탐험 프로그램으로 실제 게임을 구동하려면, 실행 전에 개발자에게 목적과 필요성을 알리고 명시적인 확인을 받아야 한다. CLI/MCP `start_session`, `mcp_smoke.py --live`, 검증용 직접 실행에 모두 적용한다. 일반 작업 지시나 과거 실행 이력을 새로운 실행의 확인으로 간주하지 않으며, 확인받은 범위 안에서만 실행한다. **예외:** `AGENTS.md`에 지정된 시스템(시스템 1: IP `10.0.0.15`·`vm-debian-codex`, 시스템 2: IP `192.168.0.94`·`HJOW-Athlon` — 2026-09-29 추가)에서는 개발자의 게임 구동 확인을 받지 않아도 된다. 사용자가 이번 지시에서 지정 시스템의 확인 없는 게임 실행을 명시적으로 허용했다.

**실행 상태:** 앞서 사용자가 실제 게임 실행 테스트 중단을 지시했고, 중단 요청 뒤 실행 중인 `Netstorm`이 없음을 확인했다. 이후 사용자가 지정 시스템의 확인 없는 게임 실행을 명시적으로 허용했으므로, **지정 시스템에서는 이 중단 지시를 해제한 것으로 적용한다.** 그 밖의 시스템에서는 기존 중단 지시가 유지되며, 사용자의 별도 재개 지시가 필요하다. 이번 문서 수정에서는 원본/복사본을 실행하지 않았다.

### Linux/Wine 실행 가능성 확인 (2026-09-29, 사용자 요청으로 이 단계에서 중단)

- 확인 시스템: `vm-debian-codex`(Debian 13, Linux x86_64), GUI 세션 Wayland/X11 `DISPLAY=:1`, X11 화면 1600×900. Wine 10.0과 32비트 Wine 구성 요소가 설치되어 있다. Wine 실행과 X11 조회는 명령 샌드박스 밖에서 성공했다.
- Windows용 .NET 10 런타임과 기존 분석 도구 빌드는 없었다. NuGet 연결을 확인한 뒤 `dotnet restore analyzeManager/AnalyzeManager.csproj -r win-x86 -p:NuGetAudit=false`와 `dotnet publish analyzeManager/AnalyzeManager.csproj -c Release -r win-x86 --self-contained true --no-restore -o /tmp/netstorm-analyze-winx86 -p:NuGetAudit=false`가 성공했다. 배포 출력은 `/tmp/netstorm-analyze-winx86/`(약 114 MB)에 있으며 Git에 포함되지 않는다.
- 사용자 소유의 임시 Wine 접두 경로 `/tmp/netstorm-wine-check/`(win32)에서 `wine cmd /c ver`와 분석 도구 `--help`가 성공했다. 저장소를 Wine의 `Z:\home\hjow\Workspaces\git\NetstormReborn` 경로로 전달한 `list_sessions`는 `RejectReparse`가 `Z:\` 드라이브 루트를 링크로 판정해 거부했다. 접두 경로의 실제 `C:\netstorm-probe` 디렉토리에 `Netstorm.exe`만 복사한 가짜 저장소에서는 `list_sessions`가 `sessions: []`로 성공했다.
- **검증 범위:** Linux에서 Wine을 통한 도구 프로세스 시작과 게임 없는 세션 목록 조회까지 가능하다. 원본 게임, `start_session`, 실제 화면 캡처·입력, MCP 통신은 실행/검증하지 않았다. 현재 원본 저장소 경로를 그대로 넘기면 `Z:\` 링크 검사에 막히며, `C:\netstorm-probe`는 설정·자산이 없는 가짜 저장소다. 따라서 Linux에서 원본 게임 분석 도구의 전체 사용 가능 여부는 아직 확정되지 않았다.
- **→ 2026-09-29 저녁 이어서 진행함** (위 "Linux/Wine 분석 도구 실행" 절: 링크 검사 수정, 실제 실행 1회, 캡처 검은 화면 문제). 당시 기록: 사용자가 재개를 요청하면 Wine 경로의 링크 검사 정책을 검토하거나 Wine C 드라이브의 독립 원본 복사본을 준비하고, 지속 실행되는 Wine 프로세스에서 세션 수명·창 캡처·입력·MCP 이미지를 순서대로 확인한다. `/tmp`의 접두 경로와 배포물은 임시 산출물이므로 다음 환경에서 다시 만들어야 할 수 있다. 이번 요청에 따라 여기서 멈추며 게임을 실행하지 않는다.

### 구현한 내용

- `TODO.md`의 AI용 원본 게임 탐험 도구를 `analyzeManager/`에 구현했다. 아직 작업 전체 완료로 보지 않는다. 사용자 작성 `TODO.md`는 그대로 유지했다.
- C#/.NET 10 Windows 독립 프로젝트. CLI와 공식 C# SDK 2.2.0 기반 stdio MCP가 같은 엔진을 호출한다. 게임 솔루션에는 추가하지 않았다.
- 도구 8개: `list_sessions`, `start_session`, `game_status`, `capture_state`, `game_input`, `wait_for_change`, `record_observation`, `end_session`.
- 원본 전체를 `extracted/analyzeManager/<sessionId>/game/`에 복사한 뒤 복사본의 options/setup 설정만 수정: 창 모드 1024×768, `InstallDir` 변경. 원본 파일을 수정하는 경로는 만들지 않았다.
- PID·시작 시각·exe 경로로 관리 대상 확인, 파일 잠금으로 동시 조작 제한, 클라이언트 물리 픽셀 입력, 관심 영역 캡처·변화 대기, 입력 전후 기록, 취소 시 키/버튼 해제 구현.
- PNG SHA-256 중복 재사용, JSONL/Markdown 약 4 MB 분할, 파일당 50,000,000바이트 미만, 세션당 PNG 500개/200 MB 및 이벤트 10,000개 한도. 원본 게임이 자체 생성하는 파일까지 크기를 강제하지는 않는다.
- [사용법·구조·제약](docs/analyze-manager.md), [도구 안내](analyzeManager/README.md), [MCP 설정 예시](analyzeManager/examples/mcp-settings.json)를 작성했다. 실제 사용자 MCP 설정은 변경하지 않았다.

### 검증된 것 / 검증하지 못한 것

- `dotnet build analyzeManager/AnalyzeManager.csproj --no-restore`: Debug 빌드 성공, 오류 0·경고 0.
- `dotnet test analyzeManager/tests/AnalyzeManager.Tests.csproj`: **17개 통과**. 가짜 원본을 사용하는 파일 보존·로그/이미지·입력 경계 검사이며 게임은 실행하지 않는 테스트다.
- **2026-09-29 게임 없는 후속 검증:** Release 빌드 오류/경고 0, Release 단위 테스트 17개 통과. `mcp_smoke.py` 기본 모드로 MCP `initialize`(프로토콜 2025-03-26), 도구 8개 스키마, `list_sessions`, 잘못된 세션의 오류 응답, EOF 종료를 확인했다. 서버와 테스트는 게임을 실행하지 않았다. `start_session` 도구 설명과 CLI 도움말에 일반 시스템의 개발자 확인 조건과 지정 시스템 예외를 반영했고, Release 재빌드 및 게임 없는 MCP 기본 검사를 다시 통과했다.
- **2026-09-29 Linux 정적 후속 작업:** `SessionStore.StoreFrame`이 같은 해시 이름의 손상된 PNG를 재사용하던 문제를 수정했다. 새 PNG는 임시 파일로 완성해 최종 이름으로 옮기며, 재사용 시 크기와 SHA-256을 검사한다. 손상 파일 회귀 테스트 1개를 추가했다. 이 Linux 시스템에서는 NuGet 패키지 다운로드가 완료되지 않아 Windows 대상 프로젝트의 변경 후 빌드·테스트를 실행하지 못했다. 대신 `SessionStore.cs`를 패키지 의존성 없는 .NET 10 임시 검증 프로젝트에 직접 연결해 정상 중복, 동일 길이 손상, 잘린 파일 검출을 통과했다. Windows 대상 전체 테스트와 MCP 검사는 이 변경 이후 재검증 필요하다.
- 실제 복사본을 한 번 실행하여 **1024×768 창과 Activision 시작 화면 PNG**를 저장·육안 확인했다. 세션 ID: `20260929T020751113Z-1ca6ff22ee2e`.
  - 증거: `extracted/analyzeManager/20260929T020751113Z-1ca6ff22ee2e/screens/3eb1594375aa2b0498cb594a3d3fdb4d3e955305bc1d634faa5b2031bf68294b.png`
  - 보고서: 같은 세션의 `report.md`. 실행 파일 SHA-256과 PID/시작 시각은 `session.json`에 있다. 결과는 Git 제외 경로에 보존했다.
- 후속 CLI `capture_state`는 “이 세션의 게임이 종료되었거나 프로세스 식별 정보가 다릅니다.” 오류를 반환했고 실제 프로세스도 없었다. **종료 원인은 미확정**이다. 명령 실행기의 자식 프로세스 정리 여부도 재개 후 확인할 후보이며 현재 확정해서 쓰면 안 된다. manifest의 `phase=running`은 마지막 기록 상태이고 실제 상태는 `game_status.running`으로 판단한다.
- 실제 마우스·키보드 반응, CLI 호출 간 게임 유지, **MCP 이미지 전송**은 미검증이다. `analyzeManager/tests/mcp_smoke.py --live`는 실행하지 않았다. 실제 MCP 호스트 등록도 미실시다.
- 중단 요청 이후 실제 게임은 재실행하지 않았다. 2026-09-29 후속 작업에서는 코드의 실행 확인 안내, Release 빌드·단위 테스트, **게임 없는** MCP 프로토콜 검사와 문서를 진행했다. 실제 입력과 MCP 이미지 전달 검증이 끝났다고 보고하면 안 된다.

### 다음 작업

1. 실제 GUI 테스트는 지정 시스템에서 별도 실행 확인 없이 진행할 수 있다. 그 밖의 시스템에서는 기존 중단 지시를 유지하고, 재개 지시를 받은 뒤 매 실행 전에 목적과 필요성을 설명해 명시적인 확인을 받는다. `analyzeManager`는 Windows 전용이므로 Linux 지정 시스템(시스템 1)에서는 Wine 실행 환경 준비가 별도로 필요하고, Windows 지정 시스템(시스템 2 `HJOW-Athlon`)에서는 바로 실행할 수 있다.
2. `mcp_smoke.py` 기본 모드의 프로토콜 검사는 완료했다. `--live`는 CLI 실행·MCP 캡처·입력·메모·강제 종료까지 수행한다. 지정 시스템에서는 실행 가능하며, 그 밖의 시스템에서는 위 규칙을 따른다.
3. Windows GUI 실행 환경이 준비되면 지속 실행되는 MCP 서버/상위 프로세스 안에서 CLI 실행 후 게임 생존을 확인하고, 이번 후속 캡처 실패 원인을 해결한다. 단일 `start_session` 성공만으로 장기 사용 가능하다고 결론 내리지 않는다.
4. 실제 입력 전후 캡처, ROI 변화 대기, MCP PNG 응답, 한국어 보고서 링크, 정상 종료 및 종료 확인 창 처리를 검증한다. 성공하면 [도구 문서](docs/analyze-manager.md)의 검증 범위를 갱신한다.
5. 원본 메뉴/도움말/캠페인 분석은 그 뒤 도구를 사용해 수행한다. 기존 6절의 게임 규칙 동적 확인 항목들은 이번에 완료하지 않았다.

이번 작업 파일: `analyzeManager/` 신규 소스·테스트·예시·README, `docs/analyze-manager.md` 신규, `LEFT_JOBS.md`·`AGENTS.md` 갱신. 시작 시 이미 사용자 수정이 있던 `TODO.md`는 보존했다. 커밋은 하지 않았다.

## 0. 진행 현황 요약

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-099)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 1. 원본 조사 결과 (1단계 조사 + 3단계 분석으로 갱신)

> **공통 조사·요구사항:** 원본 파일/포맷·실행 환경·글꼴·자료·화면 요구사항은 두 프로젝트의 근거다. 구현 라이브러리/완료 상태는 프로젝트별 기록을 따른다. 아래 파일 수·설정·미확인은 당시 조사이며 최신 판본별 분석/앞쪽 완료 절을 우선한다. 동적 분석 가능하다는 설명은 실행 허가가 아니다.

`originals/` 현재 파일 1,389개, 파일 크기 합계 164,486,189바이트(2026-09-29 사용자가 추가한 `help/manual.pdf` 포함).

`originalCD/`는 사용자가 추가한 더 이른 CD판의 내용이다. 사용자 제공 판본 정보는 **10.72**이고, 현재 실행·분석 기준인 `originals/Netstorm.exe`는 **10.78**이다. CD에는 설치 파일과 `MOVIE/englishintro2x.smk`·`germanintro2x.smk` 등 인트로 자료가 있다. 사용자 설명에 따르면 CD판은 Windows 98/ME에서 호환되고 XP에서는 동작하지 않았으며 Windows 10/11에서도 실행되지 않을 것으로 예상된다. 따라서 CD판은 자료·판본 비교용으로 두고 Windows 10/11 동적 분석은 `originals/` 패치판을 기준으로 한다([자료 설명](docs/sources/README.md)).

### 1.1 실행 파일 / 라이브러리

| 파일 | 내용 |
|---|---|
| `Netstorm.exe` (1.5MB) | x86 32bit PE, MSVC 빌드, **패킹되지 않음**(.text 약 1MB). 빌드 타임스탬프 2006-03 → 원본(1997) 이 아닌 **Ticonderoga Entertainment 비공식 패치 빌드(10.7x)**. 임포트: DDRAW, DSOUND, WINMM, WSOCK32, smackw32, GDI32, COMCTL32, ADVAPI32, SHELL32, VERSION, ole32, mscoree. 문자열에 `TAFF v%d.%d`, `_shapes.shp`, `*.tarc`, `IPX`, `SPX/IPX` 존재 |
| `NSENGLISHRES.DLL` | **설치·진단 프로그램용** 문자열 24개·다이얼로그 13개 (게임 UI 문자열 아님). 게임 UI 문자열은 exe 에 영어로 하드코딩되어 있고 `xlat.<언어>` 로 번역된다 → [xlat.md](docs/formats/xlat.md) |
| `Smackw32.dll` | Smacker 동영상 코덱. `originals/movie/`는 비어 있지만 별도 CD판의 `originalCD/MOVIE/`에는 영문·독문 인트로 `.smk` 파일이 있음 |
| `R.exe`, `unpack.exe`, `bzip2.exe`, `TMaker.exe` | `R.exe` = **NETSTORM Root Server**(로컬 LAN 서버, 2026-09-30 exe 문자열로 정정 — [network-ports.md](docs/exe/network-ports.md)), `unpack.exe`·`bzip2.exe` = 업데이트 압축 해제로 추정, `TMaker.exe` = 요새 생성기로 추정. 게임 본체 분석 대상은 아님 |
| `PatchFixs.txt` | 패치 변경 이력(892줄). 원본 대비 **변경된 규칙·버그 수정 목록** → 어느 버전 동작을 기준으로 할지 결정 시 참고 |

### 1.2 데이터 파일

| 파일/패턴 | 확인된 내용 | 비고 |
|---|---|---|
| `netstorm.tarc` (1.1MB) | TAFF 아카이브, 엔트리 246개. 데이터는 XOR 키 `mydoghasfleas` 로 인코딩 → **해독 완료** ([taff.md](docs/formats/taff.md)) | `.type`(오브젝트 정의), 공식 미션, 번역 테이블(`xlat.*`), 용어표(`config.*`) 포함 |
| `d/_shapes.shp` (5.6MB) | Miles VFX 셰이프 블록 116개(타입당 1개), 프레임 3,783개, 행 단위 RLE → **해독 완료** ([shp.md](docs/formats/shp.md)) | 모든 스프라이트. 블록 순서 = exe 의 타입 로딩 순서 |
| `d/*.COL` | Autodesk Animator Pro 팔레트 (8바이트 헤더 + 256×RGB) | **`GIFCLOUD.COL` = 게임 팔레트** (`setup.cfg` 지정). `TITLE*.COL` = 타이틀 화면. 나머지 유닛 이름 팔레트는 그래픽 제작용으로 추정 |
| `d/!color.dat` | 256바이트 색상 변환 테이블 9개 (항등·검정·단색·어둡게 등) | 개별 용도는 4단계에서 확인 ([config.md](docs/formats/config.md)) |
| `d/!*.chfnt` (18개) | Windows GDI 글꼴(Arial 등)로 게임이 **실행 중에 만든 캐시** (없으면 재생성) | 라틴 문자 전용 → 한국어는 `fonts/` 의 D2Coding 사용 (1.5절, [chfnt.md](docs/formats/chfnt.md)) |
| `d/*.english`, `*.german` 등 (약 580개) | 미션 스크립트. INI 유사 섹션(`[Header]`, `[A.]`, `[Succeeded]`, `[Failed]`, `[aiNPriestDead]`, `[aiNTempleHalfDead]`, `[@1]` …) + `key = value` + HTML 부분집합(`<h2>`, `<p>`, `<i>`) + 명령(`$Button=`, `$Timeout=`, `$Menu=`, `$Checked=`, `$OnExit=`, `<$Config,…>`) | 팬 제작 미션 다수 포함. 오타(`$Buton=`, `$button=`)가 섞여 있어 **관대한 파서** 필요 |
| `d/*.fort` (431개) | `'F'` + 플래그 + 길이 접두 섹션 35개(이름 고정). 월드 16×16 청크의 오브젝트 레코드 → **해독 완료** (일부 섹션 내부 제외) | 섬/요새·미션 맵 ([fort.md](docs/formats/fort.md)) |
| `d/*.gif` | 팬 제작 이미지로 추정 | 우선순위 낮음 |
| `d/options.cfg`, `setup.cfg`, `!rootservers.dat` | 설정 / 온라인 서버 목록. `.cfg` 는 TAFF 와 같은 키로 XOR 인코딩 → **해독 완료** | `tools/nscfg.py` 로 읽기·수정 가능. **화면 해상도 설정 `SCREENW/SCREENH` = 1024×768** (640×480 고정 아님) |
| `music/*.mus` (9개) | 확장자만 다른 **RIFF WAV** (PCM 16bit, 스테레오, 22,050Hz) | 그대로 재생 가능 |
| `sound/*.wav` (218개) | 표준 PCM WAV 효과음 (대부분 22,050Hz 8bit 모노) | 그대로 재생 가능 |
| `help/*.HLP` | WinHelp 도움말 (GAME, HELP, VOCAB, VENDOR, README) | 유닛·규칙·용어 설명 → 규칙 분석의 1차 자료 |
| `help/manual.pdf` | 사용자가 외부 사이트에서 확보해 추가한 구버전 공식 매뉴얼 PDF | [자료 목록](docs/sources/README.md), [조작·화면·비용 선별 대조](docs/sources/pdf-manual.md). `GAME.HLP` 전체와의 대조는 남음 |

### 1.3 주의 사항

* **원본 자산 재활용 가능**: 개발사 지원이 오래전에 끊겨 저작권 행사가 사실상 없으므로, 원본 그래픽·사운드·미션·도움말 등 **기존 파일을 필요 시 그대로 재활용**한다. 대체 자산을 새로 만들 필요는 없다.
  * `originals/`(약 310MB, 1,388개)는 **2026-09-28 사용자 커밋("오리지널 빌드")으로 git 저장소에 포함**되었다 (`.gitignore` 의 `/originals/` 제외 규칙을 주석 처리). 이전의 "커밋하지 않는다" 방침은 폐기.
    * 결과: 저장소를 받은 환경(CI 포함)에서도 원본 검증 테스트가 건너뛰지 않고 실행된다. `extracted/` 는 여전히 커밋하지 않는다.
  * 원본 플레이 영상 `playingVideos/` 는 용량이 커서 **git 에 커밋하지 않는다** (AGENTS.md, `.gitignore` 의 `/playingVideos/**`). 새 환경에는 따로 복사해 두어야 한다 (1.6절).
    * 주의: 원본을 실행하면 게임이 `d/options.cfg` 를 저장하고 `d/!*.chfnt` 캐시를 다시 만들 수 있으므로, 분석용 실행 뒤 의도하지 않은 원본 파일 변경이 커밋되지 않게 `git status` 로 확인한다.
  * 배포 패키지에는 필요한 자산을 동봉할 수 있다(14단계).
  * 게임 엔진은 원본 포맷을 직접 읽는 구조로 만든다(자산 변환 없이 원본 폴더 또는 동봉 폴더에서 로드).
* (Linux 대응 시) Linux 는 파일 시스템이 **대소문자를 구분**한다. 원본 데이터 파일명은 대소문자가 뒤섞여 있고(`Battle1.fort` / `battle1.english`), 아카이브 내부 경로는 `\` 구분자를 쓴다 → 파일 탐색은 반드시 대소문자 무시 + 경로 구분자 정규화 계층을 거친다.
* 원본 텍스트 파일 인코딩은 Windows-1252(영어/독일어)로 추정 → 로드 시 UTF-8 로 변환한다. 새로 만드는 문서·코드는 UTF-8.

### 1.4 원본 실행 환경 (AGENTS.md 기준)

* `originals/Netstorm.exe`(10.78)는 현재 **Windows 10/11 에서 제한적으로 구동 가능**하며, 실행 시 **창 모드**로 동작한다. 이 설명은 `originalCD/NETSTORM.EXE`(10.72)에 적용되지 않는다.
* 게임 옵션에서 전체화면으로 전환하면 전환 자체는 되지만, **게임 종료 후 재실행 시 오류가 발생**한다. 오류 후 한 번 더 실행하면 다시 창 모드로 실행된다.
  * 원본을 분석용으로 실행할 때는 **전체화면 전환을 하지 않는다**.
  * 전체화면 플레이 자체는 가능하다: 로컬 플레이 영상(`playingVideos/`)은 1024×768 전체화면으로 플레이하며 녹화한 것이다 (문제는 전체화면 설정이 저장된 뒤의 **재실행**).
  * **분석 결과(정적)**: 설정은 `d/options.cfg` 의 `startInFullScreen` 에 저장된다. 시작 시 이 값이 참이면 `workingFullScreenFlags` 모드로 화면을 초기화하는데, 실패하면 초기화 함수가 그대로 반환되어 게임이 뜨지 않는다 → DirectDraw 독점 전체화면 초기화 실패로 추정. 자세한 내용은 [config.md](docs/formats/config.md) "전체화면 재실행 오류".
  * **복구 방법(추정)**: `python tools/nscfg.py set originals/d/options.cfg startInFullScreen 0` (원본은 `.bak` 으로 백업됨). **현재 `originals/d/options.cfg` 는 `startInFullScreen = "1"` 상태**라 다음 실행 때 오류가 날 수 있다.
  * 오류 후 창 모드로 돌아오는 과정은 동적 분석으로 확인한다. 클론에서는 이 버그를 재현하지 않는다.
  * **2026-10-03 AGENTS.md 명시: "이 문제는 새 클론 게임에서는 발생하지 않아야 한다."** 클론은 디스플레이 모드를 바꾸지 않는 테두리 없는 전체 화면 창과 시작 안전장치로 대응한다([map-viewer.md](docs/map-viewer.md) 화면 설정).
* 원본을 직접 실행할 수 있으므로 다음이 가능하다.
  * **동적 분석**: x64dbg 등으로 실행 중인 원본의 메모리·함수 호출·파일 접근 관찰 (정적 분석 결과 검증)
  * **직접 대조**: 영상뿐 아니라 원본 화면 캡처·수치 측정과 클론 결과를 나란히 비교

### 1.5 한국어 글꼴 (AGENTS.md 기준)

* 한국어 표시가 필요한 경우 **`fonts/D2Coding-Ver1.3.2-20180524-all.ttc`** 를 사용한다.
  * **2026-10-08 AGENTS.md 추가:** 마이너 업그레이드판 **`fonts/D2Coding-Ver1.4.0-20261003-all.ttc`** 가 같은 폴더에 들어왔다. TTC이고 글꼴 4종인 것은 파일 머리로 확인했다(2026-10-10). **face 순서와 메트릭이 1.3.2와 같은지는 확인하지 않았다.** dotnetpj 코드는 아직 1.3.2 파일명을 직접 가리킨다(`Netstorm.Game.csproj`·`NetstormGame.cs`·`TrueTypeCollectionTests.cs`). **새 판으로의 교체는 중요도가 낮다(2026-10-10 사용자 결정).** 당분간 1.3.2를 그대로 쓰며, 교체할 때 확인할 것은 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md) 10절 7번에 있다. 아래 표는 1.3.2 기준이다.
* TrueType Collection(TTC)이며 글꼴 4종을 담고 있다. 로드할 때 **face 인덱스**를 지정해야 한다.

  | 인덱스 | 글꼴 |
  |---|---|
  | 0 | D2Coding (일반) |
  | 1 | D2Coding Bold |
  | 2 | D2Coding ligature |
  | 3 | D2Coding ligature Bold |

* 라이선스: SIL Open Font License 1.1 → 게임과 함께 배포 가능 (배포 시 OFL 고지 동봉).
* **monospace(고정폭)** 글꼴이다. 원본 UI 는 Arial(가변폭) 기준으로 배치되어 있으므로, 같은 문장이라도 폭이 달라진다 → 텍스트 영역은 글꼴 메트릭으로 측정해 줄바꿈·크기를 정해야 한다(좌표 하드코딩 금지).
* 영어 텍스트는 원본 느낌을 살리기 위해 원본 `.chfnt` 비트맵 폰트 사용을 우선 검토하고, 한국어(및 원본 폰트에 없는 문자)는 D2Coding 으로 대체하는 **폰트 폴백 체인**을 둔다. 영어도 D2Coding 으로 통일할지는 구현 시 화면을 보고 결정.

### 1.6 외부 참고 자료 (AGENTS.md 기준)

* **플레이 영상 — 로컬** (`playingVideos/`, 2026-09-28 AGENTS.md 추가, git 제외): 원본 게임을 최대 해상도 **1024×768 풀스크린**으로 두고 플레이하며 녹화한 영상 4개. 파일·미션 대응은 5단계 표 참고
  * 모니터가 16:9 여서 4:3 게임 화면 **좌우에 같은 크기의 검은 여백(레터박스)** 이 들어가 있다 → 좌표를 잴 때는 가운데 4:3 영역을 먼저 잘라 1024×768 기준으로 환산한다.
  * 형식 (2026-09-28, 4개 모두 `ffprobe` 확인): 영상 **AV1 1920×1080 60fps**, 소리 AAC. 길이·크기와 추출 방법은 [docs/videos/README.md](docs/videos/README.md). 이 PC 의 FFmpeg 에 AV1 디코더 `libdav1d` 가 있어 `ffprobe`·`ffmpeg` 로 그대로 처리 가능 (Python OpenCV 등은 AV1 미지원일 수 있으므로 프레임 추출은 `ffmpeg` 로 한다)
  * 계산상 게임 화면은 **가운데 1440×1080** (좌우 여백 각 240px), 배율 1080/768 = **1.40625배**(정수 아님 → 픽셀 경계가 흐려짐). 영상 좌표 → 게임 좌표: `x = (영상x − 240) / 1.40625`, `y = 영상y / 1.40625`. 실제 여백 경계는 첫 프레임에서 한 번 측정해 확인한다
  * 방송 중 녹화라 **사람 음성이 함께 녹음**되어 있다 → 효과음·음악 재생 시점을 소리로 판단할 때는 음성과 섞인 점에 주의한다 (원본 소리 자체는 `sound/`·`music/` 파일이 기준).
  * **평소보다 밝게 촬영**되었다 (AGENTS.md, HDR 설정 문제로 추정) → 영상의 색은 원본 팔레트와 다르므로 색 비교에는 쓰지 않는다 (스크린샷 사용).
* **플레이 영상 — YouTube**: 튜토리얼, 캠페인 1-1~4, 1-5, 1-6, 2-1, 2-2, 2-3, **3-2**(2026-09-29 AGENTS.md 추가) — 주소는 5단계 표 참고. 캠페인 2-x·3-2 는 로컬 영상이 없고 YouTube 에만 있다 (필요 시 `yt-dlp` 로 `extracted/videos/` 등에 받는다)
* **게임 플레이 방법 소개 홈페이지 (한국어)**: https://hjow.duckdns.org/netstorm/learnmain.htm (2026-09-28 AGENTS.md 에 추가)
  * 하위 페이지 (2026-09-28 목차 확인):

    | 페이지 | 주제 | 주로 참고할 단계 |
    |---|---|---|
    | [learninstall.htm](https://hjow.duckdns.org/netstorm/learninstall.htm) | 설치 및 시작, 메뉴, 옵션 | 9단계(메뉴·옵션 화면) |
    | [learninterface.htm](https://hjow.duckdns.org/netstorm/learninterface.htm) | 화면 구성·UI 설명 | 5단계(화면 관찰), 9단계(HUD) |
    | [learnmineral.htm](https://hjow.duckdns.org/netstorm/learnmineral.htm) | 광물 자원 | 4·8단계(경제) |
    | [learnresource.htm](https://hjow.duckdns.org/netstorm/learnresource.htm) | 자원 수집·관리 (Storm Power 등) | 4·8단계(경제) |
    | [learntemple.htm](https://hjow.duckdns.org/netstorm/learntemple.htm) | 신전 시스템 | 4·8단계(신전·기술) |
    | [learnpriest.htm](https://hjow.duckdns.org/netstorm/learnpriest.htm) | 사제(프리스트)·고위 사제 | 4·8단계(승패 조건·프리스트) |
    | [learnmovie.htm](https://hjow.duckdns.org/netstorm/learnmovie.htm) | 플레이 영상 | 5단계 |
    | [unitmain.htm](https://hjow.duckdns.org/netstorm/unitmain.htm) | 전체 유닛 목록·설명 | 4·8단계(유닛·건물 규칙), [types.md](docs/gameplay/types.md) 수치와 대조 |

  * 활용 방침: 게임 규칙의 **1차 근거는 원본 데이터·실행 파일 분석**이고, 이 홈페이지는 동작 이해와 교차 검증용으로 쓴다. 내용이 분석 결과와 다르면 원본 동작을 우선하고 차이를 문서에 남긴다.
  * **한국어 용어 참고 자료**로도 쓴다 (12단계 한국어 번역 시 유닛·건물·자원 이름의 기존 한국어 표기 확인).
* **커스텀 맵 제작 방법** (2026-10-03 AGENTS.md 추가): [방법 1(네이버 블로그)](https://blog.naver.com/hujinone22/221213700658), [방법 2(NetStorm 위키 Starting Editing)](https://netstorm.fandom.com/wiki/Starting_Editing). 두 페이지의 내용은 아직 읽어 정리하지 않았다.
* **커스텀 맵 TEST01** (2026-10-03 AGENTS.md 추가): `originals/d/TEST01.fort` + `originals/d/TEST01.english`.
  * `.fort`는 게임 안 **Edit 메뉴**로 생성·수정한다. `.english`는 사용자(개발자)가 **텍스트 편집기로 직접 작성**하는 텍스트 파일이다.
  * `.english`로 사용자·인공지능 구성, 시작 SP, 브리핑 창 내용을 적는다. 다른 언어는 그 **언어 이름을 확장자**로 같은 방식으로 작성한다.
  * 형식과 TEST01 예: [mission-script.md](docs/formats/mission-script.md#커스텀-맵-만들기-agentsmd-2026-10-03-test01-예). 원본에서 편집기·시험 전투로 여는 방법: [녹화 노트](docs/videos/record-play-edit-test01-20261003.md).
* **원본 스크린샷** (AGENTS.md, `screenShots/`, 2026-09-28 기준 42장): 외부 캡처 도구로 찍어 작업표시줄·창 테두리·바깥 영역이 일부 포함된다. 메인 메뉴 계열은 `mainMenu - …`, 미션 화면은 `미션 이름 - 상황` 형식의 파일 이름이다. 모두 원본 1024×768 창 모드.
  * **전체 목록과 캡처별 관찰: [docs/screens/README.md](docs/screens/README.md)** — 메인 메뉴·Campaign(하위 6개)·Multiplayer·Demo·Help·Edit·Credits·Options(하위 4개), The War Begins!(브리핑·진행 9장·정보 창 3장·게임 메뉴·승리·패배 2장), Dissolved Alliance!(시작 + 섬별 2장), 기존 튜토리얼·캠페인 시작 화면
  * 클라이언트 영역(1024×768)의 캡처 내 시작 위치는 파일마다 조금씩 다르므로 제목 표시줄·테두리 경계를 측정해 잘라 쓴다.
  * 원본 카메라는 **위치 이동만 되고 높이·확대 배율은 바꿀 수 없다** (사용자 확인). 그래서 섬 3개가 있는 Dissolved Alliance! 는 한 화면에 담을 수 없어 시점을 옮겨 섬마다 찍었다.

### 1.7 결과물 화면 요구사항 (2026-09-28 AGENTS.md 추가)

* **풀스크린 모드**를 지원해야 한다 (원본처럼 재실행 오류가 나서는 안 된다 — 1.4절, 2026-10-03 AGENTS.md에 명시).
* **프레임 속도(2026-10-05 AGENTS.md 변경): 기존 게임 수준의 프레임으로 먼저 만들고(원본 루프 14ms, 초당 약 71.4바퀴; 당시 C# 적용 기록은 별도 이관), 이후 60·120프레임을 지원한다.** (2026-10-03 문구는 "30·60·120 지원, 30 먼저"였다.) 기존 게임은 요소마다 애니메이션 프레임이 달랐던 것으로 추정된다(AGENTS.md).
  * 근거 자료: 원본 영상 측정에서 가이저 증기 약 24Hz, 신전 회오리·피해 연기 12Hz로 요소마다 달랐고, 화면 갱신 상한은 설정 `maxFPS = 75`였다([animation-timing.md](docs/videos/animation-timing.md)).
* **화면비 16:9, 16:10, 4:3** 을 지원해야 한다. 원본 기본 해상도 1024×768 과 메인 메뉴 타이틀 그림(640×480)은 4:3 이다.
  * **원본은 4:3 해상도만 지원했다** (AGENTS.md·사용자 확인): Options → Resolution 하위 메뉴가 `640 by 480`, `800 by 600`, `1024 by 768` 세 가지뿐 (`mainMenu - Options - Resolution.png`). 따라서 16:9·16:10 은 원본 참고 자료가 없는 **새 기능**이며 동작은 클론에서 설계한다.
  * 원본은 **1024×768 이 최대 해상도**이며, 16:9 모니터에서 풀스크린으로 실행하면 4:3 화면 좌우에 같은 크기의 검은 여백이 생긴다 (로컬 플레이 영상에서 확인). 즉 원본의 와이드 모니터 대응은 "4:3 가운데 + 좌우 여백" 이다.
  * 원본 카메라는 확대·축소가 없고 해상도가 곧 보이는 맵 범위다 (640×480 보다 1024×768 이 더 넓게 보임). 와이드 화면에서 맵을 옆으로 더 보여 주는 방식이 원본 해상도 선택의 동작과 가장 가깝다.
  * **결정됨(2026-09-29, 사용자): 와이드 화면(16:9·16:10)에서는 "시야 확장"으로 동작한다.** 원본과 같은 픽셀 배율을 유지한 채 맵을 옆으로 더 보여 준다 (논리 높이 = 고른 원본 해상도의 높이, 논리 폭이 화면비에 맞춰 늘어남). **4:3 레터박스(좌우 검은 여백) 화면은 게임 동작으로 쓰지 않는다.** 구현은 [map-viewer.md](docs/map-viewer.md) "화면 설정". 멀티플레이에서 해상도에 따른 시야 차이를 허용할지는 13단계에서 정한다.
  * 메뉴·대화상자는 원본처럼 가운데 배치하고 남는 영역은 배경(구름)으로 채우는 방식이 원본 캡처와 맞는다.
  * 참고: 코드에는 개발용 `WideScreenMode.Letterbox`(F10, `--wide letterbox`)가 남아 있다. 원본 캡처와 1024×768 로 겹쳐 볼 때만 쓰며, 게임 옵션으로 노출하지 않는다 (제거할지는 미정).
  * 사이드바·미니맵 등 HUD 는 화면 가장자리 기준으로 배치하고, 좌표를 4:3 기준으로 하드코딩하지 않는다.
* **풀스크린에서 마우스 커서를 화면 끝에 대면 화면(카메라)이 이동**해야 한다 (원본도 옵션에서 해당 기능을 켰을 때 지원 — 2026-10-03 AGENTS.md 문구 변경). 원본은 Options 메뉴의 **`Edge Scroll in Fullscreen`** 선택 항목으로 켜고 끈다 (캡처에서는 켜짐). 이동 속도·가장자리 폭은 원본 동작을 측정해 맞춘다 (4·5단계) → **2026-09-29 exe 분석·구현 완료** ([edge-scroll.md](docs/exe/edge-scroll.md)): 창 테두리가 없을 때(전체화면 또는 바탕화면과 같은 크기의 창)만 동작하므로 일반 창 모드에서는 동작하지 않는다.
* 원본 Options 메뉴 전체(설정 화면 구현 기준): Direct Draw / Full Screen, Resolution >, Sound On, Play Music, Wind Noise, Speaker Swap L/R, Sound Effect Volume >(1~5), Music Volume >(1~5), Edge Scroll in Fullscreen, Auto-Demo, Tell Tips at Startup, Pause - Shift-F9, Pass Server Diagnostic

---

## 2. 기술 스택 (**확정: C# + MonoGame**, 2026-09-27 사용자 결정)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-101)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

---

## 3. 디렉터리 구조 (2026-10-05: C# 프로젝트를 `dotnetpj/` 로 옮기고 C++ 빌드 `cpppj/` 추가)

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-102)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

```
Netstorm/
├─ originals/                 # 현재 분석 기준 패치판 10.78 (저장소 포함)
├─ originalCD/                # 이전 CD판 10.72 및 인트로·설치 자료 (자료 참고용)
├─ docs/
│  ├─ formats/                # 파일 포맷 명세 (taff.md, shp.md, type.md, fort.md, mission-script.md …)
│  ├─ gameplay/               # 게임 규칙·수치·공식 명세
│  ├─ exe/                    # 실행 파일 분석 노트 (모듈 맵, 구조체)
│  ├─ videos/                 # 영상 분석 노트
│  └─ screens/                # 원본 화면 캡처(screenShots/) 관찰 노트
├─ tools/                     # Python 추출·검증 도구, Ghidra 스크립트
├─ extracted/                 # 도구가 원본에서 추출한 결과물 (커밋하지 않음, 필요 시 생성)
├─ fonts/                     # 한국어 글꼴 D2Coding (TTC, SIL OFL)
├─ locale/                    # 번역 파일 (en, ko 1차 / de 등 이후)
├─ screenShots/               # 사용자가 찍은 원본 게임 캡처 (외부 캡처 도구 — 작업표시줄·창 테두리 일부 포함, 1.6절 표)
├─ assets/game-data/          # 두 빌드가 함께 쓰는 클론 게임 데이터
├─ analyzeManager/            # 원본 게임 자동 탐험 프로그램 (C#, 분석 도구 — dotnetpj/ 의 Netstorm.Assets 를 참조)
├─ dotnetpj/                  # C# + MonoGame 빌드
│  ├─ Netstorm.sln, global.json, Directory.Build.props, Directory.Packages.props
│  ├─ src/
│  │  ├─ Netstorm.Assets/     # 원본 포맷 로더 (TAFF, .type, _shapes.shp, 팔레트, .fort, 설정, 번역) — MonoGame 비의존
│  │  ├─ Netstorm.Core/       # 게임 규칙·엔티티·전투·경제·다리·AI·미션 스크립트 — MonoGame 비의존 (결정론·테스트 용이)
│  │  ├─ Netstorm.Net/        # 멀티플레이 (락스텝)
│  │  └─ Netstorm.Game/       # MonoGame DesktopGL 실행 프로젝트: 렌더링·입력·오디오·UI·화면 전환
│  └─ tests/
│     ├─ Netstorm.Assets.Tests/  # xUnit: 원본 전체 파일 파싱, Python 도구 결과와 비교
│     └─ Netstorm.Core.Tests/    # xUnit: 규칙·결정론 테스트
└─ cpppj/                     # C++ 빌드 (디컴파일한 소스를 토대로 다시 만든다, CMake) — docs/cpp-build.md
   ├─ CMakeLists.txt, CMakePresets.json, SOURCE_MAP.md(원본 소스 파일 → cpppj 경로)
   ├─ src/o/                  # 원본 \Ns\O\ 공용 모듈
   ├─ src/client/             # 원본 클라이언트 모듈
   ├─ src/zacket/             # 원본 \Ns\Zacket\ 네트워크 패킷
   ├─ src/platform/           # 새로 쓰는 플랫폼 계층 (원본의 Win32·DirectX 대체)
   ├─ src/app/                # 실행 파일 진입점
   └─ tests/                  # 단위 테스트 (ctest)
```


---

## 4. 단계별 작업 상세

각 작업은 `[ ]` 체크박스로 관리하고, 완료 시 `[x]` 로 바꾸고 산출물 경로를 적는다.

### 2단계. 개발 환경 구축 — ✅ 완료 (2026-09-27)

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

> 설치 스크립트·Git/인코딩 설정·Ghidra는 공통 도구다. 아래 “C# 기준 필수/선택”과 SDK/설치 확인은 당시 기록이며 cpppj 현재 준비물은 [C++ 빌드 안내](docs/cpp-build.md)를 따른다.

- [x] `git init`, `.gitignore` — 2026-09-27 완료. .NET 산출물(`bin/`, `obj/`, `publish/`, `TestResults/`)도 제외
- [x] 개발 도구 점검·설치 스크립트 `PREPARE.ps1` — 2026-09-27 완료 (C# 기준으로 갱신)
  - 사용법: `powershell -ExecutionPolicy Bypass -File .\PREPARE.ps1` (항목 선택 후 점검·설치), `-CheckOnly`(점검만), `-All`(선택 화면 생략), `-ToolsDir`(Ghidra·vcpkg 설치 폴더, 기본 `C:\Tools`)
  - 필수: Git, **.NET SDK 10**, Python 3 + 패키지(pillow·pefile·capstone), JDK 21+, Ghidra, x64dbg, Process Monitor, git safe.directory / 권장: MonoGame 템플릿, VS Code 확장(C# Dev Kit·C#·Python) / 선택: VS Build Tools(C++)·CMake·Ninja·vcpkg(C# 확정으로 불필요), yt-dlp, FFmpeg
  - **2026-09-30 추가: "방화벽 예외 (TCP 6799)" 항목**(권장, 기본 선택 해제) — 원본 게임이 전투 시작 때 여는 6799 인바운드를 허용하는 규칙을 점검·등록한다. **관리자 권한이 필요하며 관리자 권한이 아니면 "사용 불가"로 표시**하고 건너뛴다. 사용자가 Windows에서 포트 허용 기능과 비관리자 선택 차단이 동작함을 확인했다. 규칙 세부값·반복 등록·되돌리기·실제 경고 억제 여부 등 남은 검증은 [network-ports.md 7.2](docs/exe/network-ports.md)에 기록했다.
  - 2026-09-27 점검 결과: 필수 전부 설치됨 (.NET SDK 8.0.425 / 9.0.318 / 10.0.401 확인). MonoGame 템플릿은 미설치(프로젝트는 템플릿 없이 구성했으므로 필수 아님)
- [x] Linux 용 개발 도구 점검·설치 스크립트 `PREPARE.sh` — 2026-09-28 작성 (PREPARE.ps1 과 같은 항목 선택 → 점검 → 설치 → 재점검 흐름)
  - 사용법: `bash ./PREPARE.sh` (일반 사용자로 실행, 시스템 패키지는 스크립트가 sudo 호출), `--check-only`, `--all`, `--tools-dir DIR`(기본 `~/Tools`)
  - 패키지 관리자 apt / dnf / pacman / zypper 지원. .NET SDK 10 은 배포판 저장소 대신 `dotnet-install.sh` 로 `~/.dotnet` 에 설치하고 `~/.profile` 등에 PATH·DOTNET_ROOT 등록
  - 필수: 기본 도구(curl·unzip·tar), Git, 빌드·실행 라이브러리(ICU·OpenSSL·OpenGL), .NET SDK 10, Python 3 + 패키지(배포판 패키지 우선, 실패 시 pip --user), JDK 21+(javac 로 확인), Ghidra, git safe.directory / 권장: MonoGame 템플릿, **Wine**(x64dbg·Process Monitor 대체: 원본 32bit exe 실행, winedbg, `WINEDEBUG=+file`), VS Code 확장 / 선택: C++ 빌드 도구·CMake·Ninja·vcpkg, yt-dlp(`~/.local/bin` 에 공식 최신 실행 파일), FFmpeg
  - `.gitattributes` 에 `*.sh text eol=lf` 추가 (Windows 체크아웃에서도 LF 유지)
  - 검증: Windows Git Bash 에서 문법 검사·`--check-only --all`·선택 화면 입력 처리만 확인. **실제 Linux 배포판에서의 설치 동작은 미검증** (아래 "Linux 에서 빌드·실행 확인" 때 함께 확인)
  - Ghidra 헤드리스 디컴파일 스크립트 `tools/ghidra/run_decomp.ps1` 은 아직 Windows 전용 (Linux 용 필요 시 `analyzeHeadless` 로 옮길 것)
- [x] `.editorconfig` — 2026-09-27 완료. UTF-8·LF 기본, `.ps1` 은 UTF-8 BOM·CRLF, `.sln` CRLF, C# 스타일 규칙(파일 범위 네임스페이스, Allman 중괄호, `_camelCase` private 필드)
- [x] Ghidra 프로젝트 생성, `Netstorm.exe` 임포트·자동 분석·전체 디컴파일 — 2026-09-27 완료
  - `tools/ghidra/run_decomp.ps1` (헤드리스, 약 10~20분) → 프로젝트 `extracted/ghidra/`, 결과 `extracted/decomp/Netstorm.c` (2026-09-28 재추출: 함수 4,506개 성공, 실패 0개)
- [x] 이전 CD판 `originalCD/NETSTORM.EXE` 별도 디컴파일 — 2026-09-30 완료
  - `tools/ghidra/run_decomp.ps1 -Edition originalCD` → 프로젝트 `extracted/originalCD/ghidra/`, 결과 `extracted/originalCD/decomp/NETSTORM.c` (함수 3,711개 성공, 실패 0개). 기존 `originals/` 디컴파일 결과의 SHA-256은 작업 전후 동일하다.
  - 두 판본의 Ghidra 프로젝트와 C 결과는 `extracted/` 아래라 Git에 커밋되지 않는다. 다른 PC에서는 [재생성 명령](docs/formats/README.md#추출-순서-처음-받은-사람용)을 실행해야 한다.
  - **2026-09-30 `DESKTOP-HJOW` 재디컴파일 완료** (Ghidra 12.1.4, JDK 21, 두 판본 순차 실행 — 패치판 약 6분, CD판 약 3분):
    - 패치판 `extracted/decomp/Netstorm.c`: 함수 4,506개 성공·실패 0개. 이 PC에 있던 2026-09-28 결과와 **SHA-256이 같다**(`67e3e9eb…`, 183,914줄) — 내용은 이미 최신이었고 다시 만들어도 바뀌지 않는다. 그래서 `docs/exe/`의 `Netstorm.c` 줄 번호 인용(예: energy-requirements.md 약 104288행)은 그대로 유효하다.
    - CD판 `extracted/originalCD/decomp/NETSTORM.c`: 이 PC에는 없던 결과를 새로 만들었다. 함수 3,711개 성공·실패 0개, 155,935줄, SHA-256 `f6fecc58…`. 함수 수가 `HJOW-Athlon` 기록(3,711개)과 같다.
    - 입력 exe: `originals/Netstorm.exe` `a305414c…`, `originalCD/NETSTORM.EXE` `613500a3…`. Ghidra 로그의 `Invalid GIF data` 오류 4건은 exe 안 리소스 자동 해석 경고이며 디컴파일과 무관하다.
### 3단계. 원본 분석 — 데이터 포맷

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

- **공통 조사 이력:** 프레임 코드/검색·Territory 청크 위치·Deck/Technology 레코드·HTML 조건 규칙은 해석했다. 근거는 각 포맷 명세를 따른다. 대응 C# 클래스와 170/172개 테스트는 이관했다.

각 포맷마다 (1) `docs/formats/*.md` 명세, (2) `tools/` 의 Python 파서·추출기, (3) 원본 전체 파일에 대한 파싱 성공 검증을 산출물로 한다.
exe 내부의 파일 로딩 함수를 Ghidra 로 함께 추적하면 빠르다(`TAFF v%d.%d`, `_shapes.shp` 문자열 참조 지점부터 시작).

- [x] **TAFF 아카이브 (`netstorm.tarc`)** — 2026-09-27 완료: [taff.md](docs/formats/taff.md), `tools/taff.py`. XOR 키 `mydoghasfleas`, 246개 전부 추출
- [x] **`.type`** 문법·속성·플래그 — 2026-09-27 완료: [type.md](docs/formats/type.md), `tools/typefile.py`, 수치표 [docs/gameplay/types.md](docs/gameplay/types.md) (70종)
  - [ ] 남은 일: 속성·플래그의 정확한 게임 내 의미 확정 (4단계와 연계), 측면 글자의 방향 대응(예: sunCannon L~P), 동작별 재생 속도
- [x] **`_shapes.shp`** — 2026-09-27 완료: [shp.md](docs/formats/shp.md), `tools/shp.py`. 116블록·3,783프레임 전부 디코딩, PNG 추출 검증
  - [ ] 남은 일: 헤더 bounds/origin 의 정확한 의미, 특수 레코드 91개의 용도
- [x] **팔레트** — 2026-09-27 완료: 게임 팔레트 = `d/GIFCLOUD.COL` ([shp.md](docs/formats/shp.md) "색상")
  - [ ] 남은 일: `!color.dat` 테이블 9개의 개별 용도, 플레이어 색 구현 방식 (4단계)
- [x] **`.chfnt`** 용도 확인 — 2026-09-27 완료: GDI 글꼴의 런타임 캐시 ([chfnt.md](docs/formats/chfnt.md))
  - [ ] 남은 일(낮은 우선순위): 글리프 데이터 디코딩 (영어 원본 글꼴 재현이 필요할 때)
- [x] **`.fort`** 컨테이너·오브젝트 레코드 — 2026-09-27 완료: [fort.md](docs/formats/fort.md), `tools/fort.py` (`dump`, `verify`)
  - 원본 463개(느슨한 파일 431 + 아카이브 32) 전부 섹션 끝까지 정확히 해석
  - 섹션 35개(Subscriber … Deck, Reserved2~4, Terr00~19), 타입 번호 변환(TypeNames 해시), 청크 레코드, 타입 플래그(typeflags 단어 → 비트, 파생 규칙) 해독
    - 게임 의미(추정): 워크샵에서 등록하는 사이드바 "덱" — [workshop-deck.md](docs/gameplay/workshop-deck.md) 3절
  - [ ] 남은 일: `State`·`Badges`·`CoreData`·`Mission` 섹션 내부 구조, `Territory` 외관·나머지 플래그, 회전된 영역 동적 검증, 내부 class 값에 따른 파생 플래그
- [ ] **미션 스크립트** (`.english` 등): 문법 명세 — 섹션 종류와 발생 조건(이벤트), `[Header]` 키 전체 목록(`missionType`, `myTech`, `aiNName/Tech/StartMoney/Collectors/GeyserAttachments/color/BridgeDrawRate/Ability` …), `$` 명령과 인자, 인라인 HTML 태그, `{mission.filename}` 등 치환 변수. 전 파일 대상 키·명령 빈도 통계
  - [x] 문법 개요·빈도 통계·공식 캠페인 구성 — 2026-09-27 완료: [mission-script.md](docs/formats/mission-script.md)
  - [x] 머리 값·섹션 조회 규칙 — 2026-09-28 완료: 원본은 미션 파일을 설정 파일로 읽음(머리 값 = 파일 전체 첫 일치), 섹션 찾기·본문 범위 규칙 확인 ([mission-script.md](docs/formats/mission-script.md) "원본 해석 규칙")
  - [ ] 남은 일: 각 명령·버튼 동작·이벤트 섹션의 정확한 의미, 인라인 명령 실행·HTML 스타일 변환·동적 UI 대조 (4·9단계)
- [x] **설정 파일** — 2026-09-27 완료: [config.md](docs/formats/config.md), `tools/nscfg.py` (복호화·값 변경, 바이트 단위 라운드트립 검증)
  - [x] 2026-09-28: 조회 규칙(**첫 일치 우선** — 이전의 "마지막 값 우선" 기록 정정)·불러오는 순서(options.cfg 가 setup.cfg 보다 먼저)·설정 객체 층·`{키|기본값}` 치환·`` ` `` 이스케이프·`{Not Found:키}` 확인 ([config.md](docs/formats/config.md) "설정 조회 규칙")
- [x] **파일 조회 순서** (느슨한 파일 vs 아카이브) — 2026-09-28 완료: 데이터 폴더 디스크 → `*.tarc` → 보조(CD) 폴더 ([vfs.md](docs/formats/vfs.md))
- [x] **리소스** — 2026-09-27 완료: `tools/peres.py` (비트맵 3·커서 18·다이얼로그·문자열). DLL 문자열은 설치 프로그램용
- [x] **도움말** — 2026-09-29: 형제 저장소의 helpdeco 소스를 VS 2022 Build Tools Win32 Release로 빌드하고 `help/*.HLP` 5개에서 실제 본문 150개·BMP 77개를 추출. `tools/hlp.py`로 UTF-8 토픽별 텍스트 생성·목록 대조, [HLP 형식·재현](docs/formats/hlp.md), [게임 규칙 요약](docs/gameplay/help-manual.md). 게임 내 도움말 텍스트는 아카이브의 `help.english` 에도 있음
  - [x] **동봉 문서 전체 정독·정리** — 2026-09-29: `GAME.HLP`(규칙·화면·조작·튜토리얼·멀티·유닛/주문 핸드북), `readme.hlp`·`HELP.HLP`·`VENDOR`·`VOCAB`, `README.DOC`(Word, 1997-10 변경점), `*.CNT`·`HELP.EXE`, `PatchFixs.txt`(10.70~10.78), `Readme.txt`·`TMaker.txt`·`disclaimer.txt`·`steam_appid.txt` → **[docs/sources/](docs/sources/README.md)** (목록·불일치 12건, [게임 매뉴얼](docs/sources/game-manual.md), [패치 이력](docs/sources/patch-history.md), [설치·지원 문서](docs/sources/support-docs.md))
    - 주요 사실: 보유 exe 는 **10.78**(2026-09-29 게임 내 Version 창으로 확정. 이전 추정: 옵션 표 근거 10.75 이상). 섬 테마·거주지 테마는 패치 10.70 V5.3~6.0 기능. 1152×864·1280×960 해상도는 패치에서 제거. 멀티는 BattleMaster 가 서버·서버 인계·포트 6800. `bridgeDrawRate`·`stuffRefreshRate` 설정 키는 패치판 exe 에 없음(확인)
    - **확인 필요**: ~~level 1 원소 유닛의 에너지~~(→ 2026-09-29 확정: 유닛마다 다르며 Bulf = Thunder 1 — 사용자 확인·PDF 대조), 파일 조회 순서(패치 문서: tarc 우선 ↔ 정적 분석: 디스크 우선), Storm Power 노랑 기준(매뉴얼은 1000 미만 빨강만), 튜토리얼 3 의 범위
  - [ ] 남은 일: 원본 도움말의 링크·토픽 간 탐색 정보 복원과 패치 실행 파일에서 수치·규칙 검증. helpdeco는 모든 파일에서 browse 재구성 경고 출력
- [x] 오디오 전수 확인 — 2026-09-27 완료: 229개 모두 표준 PCM WAV
  - [ ] 남은 일: 사운드 파일명 ↔ 게임 이벤트 매핑 표 (`.type` 의 `*Sound` 속성 + exe 문자열)
- [x] **다국어 체계** (계획에 없던 발견) — 2026-09-27 완료: [xlat.md](docs/formats/xlat.md). 원본에 `xlat.<언어>` 번역 테이블(독일어 871블록, 고유 원문 774개)과 `config.<언어>` 용어표가 있어 12단계 설계 기반으로 사용
  - [x] 2026-09-28: `Xlat.cpp` 해석 규칙(줄 첫 글자 `*`/`-`/`=`, 대소문자 구분 완전 일치, 뒤 번역 우선)과 언어 표 확인

**완료 기준**: 게임에 필요한 모든 원본 자산을 Python 도구로 읽어 사람이 볼 수 있는 형태(PNG/JSON/텍스트)로 내보낼 수 있다.

### 4단계. 원본 분석 — 실행 파일 (게임 로직)

목표는 "코드를 그대로 옮기는 것"이 아니라 **동작 명세를 뽑아내는 것**이다. 결과는 `docs/exe/`(함수 맵, 구조체), `docs/gameplay/`(규칙·수치) 에 정리한다.
규칙 이해·교차 검증에는 플레이 방법 소개 홈페이지(1.6절: 자원·신전·사제·유닛 페이지)를 함께 참고한다.

- [ ] 라이브러리 함수(CRT, DirectX, WinSock 래퍼) 식별·명명 → 게임 고유 코드 범위 축소
- [ ] 동적 분석 환경: 원본을 창 모드로 실행 + x64dbg 연결, 파일 접근(Process Monitor)·메모리 관찰로 정적 분석 결과 검증
- [x] 전체화면 설정 저장 위치·시작 흐름 확인 (정적 분석, 1.4절) — 2026-09-27
  - [ ] 남은 일: 동적 분석으로 실패 지점·플래그 초기화 과정 확인
  - 클론 쪽 대응(2026-09-29): 디스플레이 모드를 바꾸지 않는 테두리 없는 전체화면 창 + 시작 실패 표식으로 같은 문제를 피한다 ([map-viewer.md](docs/map-viewer.md) "화면 설정")
- [x] 가장자리 스크롤 판정·속도 (정적 분석) — 2026-09-29: [edge-scroll.md](docs/exe/edge-scroll.md). 남은 일: 왼쪽 버튼·Shift 상태 극성, 카메라 목표 위치의 보간·맵 경계 제한을 원본 실행으로 확인
- [ ] 서브시스템 경계 식별: 메인 루프/틱, 렌더, 입력, 사운드, 파일 로딩, 네트워크, 스크립트, AI
  - 출발점: assert 메시지로 복원한 원본 소스 파일 114개 모듈 맵 [docs/exe/modules.md](docs/exe/modules.md) (`tools/ghidra/module_map.py`)
- [ ] 핵심 구조체 복원: 게임 오브젝트, 플레이어, 섬, 다리, 타일/좌표계
- [ ] **게임 틱**: 고정 프레임 여부, 틱 레이트, 난수 생성기(결정론 확보에 필수)
  - [x] 2026-09-28 부분 확인 ([animation-timing.md](docs/videos/animation-timing.md)): 화면 루프 상한 `maxFPS = 75`(`sleepPerLoop = 0`), 게임 시각 = `timeGetTime` 기반 초(`00460cd0`), 애니메이션은 고정 틱이 아니라 오브젝트별 "다음 시각 = 현재 시각 + 간격(0.04초·0.08초 등)" 타이머 → 루프 주기로 올림되어 실제 **24Hz·12Hz** (영상 측정과 일치)
  - [ ] 남은 일: 게임 로직(이동·전투·경제)의 갱신 방식(가변 시간 간격인지), 가이저·신전 등 타입별 간격 상수 위치, 난수 생성기
- [ ] **좌표계·맵**: 아이소메트릭 투영, 타일 크기, 섬 형태, 높이/레이어, 그리기 순서(정렬 규칙)
- [ ] **경제**: 가이저(geyser) → Storm Power(게임 내 재화) 수집 흐름, 수집 유닛(collector) 이동, 자원 운반, 제단(altar)·희생, 비용
  - [x] 2026-09-29 도움말 근거 확보: 수송 유닛의 가이저→신전·전초기지 운반, 가이저 초기 2000, 적 파괴·건강한 유닛 회수 보상 25%, 제단 업그레이드·희생 절차. 실행 파일 판정은 남음 — [도움말의 게임 규칙](docs/gameplay/help-manual.md)
- [ ] **섬 소유권** — 규칙은 사용자 확인으로 정리됨 (2026-09-28): [docs/gameplay/island-ownership.md](docs/gameplay/island-ownership.md)
  - 섬 테두리 색 = 소유 플레이어 색. **템플이 건설된 섬이 그 플레이어 소유**. 템플은 **플레이어당 동시에 1기(파괴되면 사제가 재건 가능), 빈 섬에만**, 원소 무관 **5000** Storm Power
  - **템플이 있어야 다리 건설** 가능. 섬 상태는 내 섬 / 빈 섬(무소유) / 남의 섬
  - **남의 섬**: 건물·유닛 일체 건설 불가, 이동형 유닛은 이동 가능
  - **빈 섬**: 워크샵·알타는 사제가 도달하면 건설 가능(다리 불필요, 내 섬도 가능), 건물형 유닛은 내 섬과 다리로 연결돼야 건설 가능. 연결은 건설 시점에만 필요하고, 다리가 끊겨도 지은 유닛은 계속 동작
  - 용어: 템플·워크샵·알타는 "유닛"이 아니다 (사제 `Construct` 로 건설). 그 밖의 작은 건물·이동체가 유닛
  - 캡처 근거: The War Begins! 에서 적 신전이 없어지자 적 섬 테두리가 빨강 → 주황(소유자 없음), 지면도 초록으로 바뀜
  - 남은 일: exe 판정 함수, 소유자 변경 시점, 템플이 파괴된 섬에 남은 건물·유닛·다리의 처리, Outpost(중립 섬 소유), 받침 섬·가이저 바위의 소유권
- [ ] **다리 건설**: 다리 조각 생성 규칙(모양 풀, 순서, `BridgeDrawRate`), 배치 판정, 연결·붕괴 조건 (다리는 소유한 섬에서만 시작 — 위 소유권 규칙)
  - [x] 2026-09-29 조각 생성: 모양 표 26개(VA 0x52f998, 가중치 합 287)·누적 가중치 추첨·회전 표(0x531590)·프레임 선택, 생산 칸 채우기(템플이 있을 때 1초마다, Bridge Slots 칸까지, 5번째 추첨마다 한 칸 조각, 템플을 잃으면 비움), AI 는 4번째마다 한 칸 조각 → [bridge-pieces.md](docs/exe/bridge-pieces.md). `Deck.cpp` 는 다리와 무관(지식·생산 덱, Gem.cpp 가중 추첨)
  - [x] 2026-09-29 원본 실행(Windows `HJOW-Athlon`, Bridge the Gap!)으로 확인. 세부는 [bridge-pieces.md](docs/exe/bridge-pieces.md) 4절·[화면 목록 1.4절](docs/screens/README.md). 클론 `RotateByPlayer`·`BridgeCursor` 에 반영, 테스트 2개 추가.
    - 오른쪽 클릭 = 시계 방향 회전
    - C = 반대 회전 켜기/끄기(켜면 반시계)
    - 조각 왼쪽 위 칸 = 커서로만 결정: (⌊(x+7)/16⌋, ⌊y/11⌋), 크기·회전 무관
    - 붙일 수 없으면 전체 빨강
    - 사이드바는 조각을 절반 크기로 그림
  - [ ] 남은 일: 배치 판정·연결·소유권 전파·붕괴(`Bridge.cpp` 00421240~004227e0 — 튜토리얼 규칙: 섬 가장자리 또는 다른 다리의 열린 끝에 붙임, 아래에 막는 것이 있으면 불가, 초목이 늘어진 가장자리는 불가), 금 간·단단해지는 시점, 미션 `aiNBridgeDrawRate` 의 쓰임
- [ ] **건물/유닛**: 배치 규칙(소유한 섬에만 배치 — 위 소유권 규칙), 건설 시간, 원소(Sun/Rain/Wind/Thunder) 별 기술 트리, 연구(기술 획득) 방식
  - **생산 규칙 (사용자 확인, 2026-09-28)**: 유닛·건물을 생산·건설하려면 **해당 타입의 워크샵을 우클릭**해 `Put Knowledge into Production >` 로 그 유닛을 **왼쪽 사이드바 "덱"에 등록**해야 한다. 등록된 것만 사이드바에서 골라 배치할 수 있다 — [workshop-deck.md](docs/gameplay/workshop-deck.md)
  - **덱의 출처 (사용자 확인, 2026-09-29)**: 화면 왼쪽 패널 = "덱" = 매뉴얼의 Production window. **워크샵이 파괴되면 그 워크샵으로 등록한 유닛이 덱에서 사라지고**, **템플은 다리 조각과 골렘을 덱에 넣으며 템플이 파괴되면 둘 다 사라진다**. **한 유닛은 한 워크샵에만 등록**(이미 등록된 유닛은 다른 워크샵 목록에 안 나옴), **파괴된 워크샵을 재건하면 우클릭으로 다시 등록해야 복구** — [workshop-deck.md](docs/gameplay/workshop-deck.md) "덱의 출처와 사라지는 조건"
  - 캡처 근거: Sun Workshop 목록(Rain Generator·Sun Cannon·Whirlibase) ↔ 이후 캡처 사이드바의 아이콘 3개. `.fort` `Deck` 섹션(타입·chance 가중 추첨)이 이 덱을 저장하는 것으로 추정
  - **원소·에너지 규칙 (사용자 확인, 2026-09-28)** — [elements-energy.md](docs/gameplay/elements-energy.md): 원소는 Rain·Wind·Thunder 3종 + 공용 Sun. 워크샵은 자기 원소 유닛만 등록(Sun Workshop 은 예외로 다른 원소 Generator 도 등록). 템플·워크샵 외 작은 건물도 "유닛". 건물형 유닛은 소유 섬·소유 섬과 다리로 연결된 무인도·본인 다리 끝에 건설. 건설·생산에는 Storm Power + 필요 원소 에너지(예: Ice Cannon = Rain 1 + Sun 1)가 그 위치에 공급돼야 함. 공급원은 Temple·Generator(넓은 범위에 자기 원소 1개분), Sun 은 어느 원소로든 대체
    - 데이터 대조: `.type` 의 `level` = 필요 에너지 개수 (사용자 확인), 발전기 `minUsage = maxUsage = -100`
    - 신전 공급(B) 해결: Temple 도 Generator 와 같이 자기 원소 1개분·같은 범위. 우클릭 시 노란 별이 돌며 범위 표시 (사용자 확인). **튜토리얼에서는 신전 범위가 대폭 축소된 경우가 있었음** (사용자 확인)
    - **공급 범위 측정 완료 (2026-09-28, 문서 6절)**: 공급 범위는 선택 시 **원소 모양 아이콘**(물방울·조개껍데기 등)으로 표시되고, 노란 별은 건물형 유닛의 **공격 범위** 표시 (사용자 확인). 아이콘이 칸 좌표 원 위에 놓임 → **일반 미션: 템플·Generator 모두 반지름 30칸(거리² ≤ 900)** (스크린샷 2장), **튜토리얼 2 Secret Workshop: 약 14칸**으로 축소 (튜토리얼 일부만). Dissolved Alliance 영상의 17.32칸 별 원은 공격 범위
    - **exe 확인 완료 (2026-09-29)** — [battle-options.md](docs/exe/battle-options.md): 반지름 = 전투 옵션 **Generator Range**. 표 `{14, 22, 30, 38}`(Short·Normal·Long·Very Long)에서 골라 **14~30칸으로 제한**(`004b4860`), 기본 Very Long → 30칸. **튜토리얼 2 처리 함수(`Totalmade.cpp` `004c3bb0`) 첫 단계만 Short = 14칸**으로 바꿈. 전투 옵션 21종 표(`0x52f5d0`)·저장 위치(`0x52f7e0 + 번호`)도 정리
    - 다음: 공격 범위(별)와 `.type` `range` 의 관계, 옵션 기본값이 적용되는 위치, `rangeDisplayProcessType`
    - **충족 판정 (사용자 확인)**: 필요 에너지 1개 = 서로 다른 공급원(Generator·Temple, 자기 또는 동맹 소유) 1개, 그 범위가 짓는 위치에서 모두 겹쳐야 함. Sun 은 아무 원소 공급원으로. 예: Vander Tower = Thunder 2 + Sun 1 → Thunder 공급원 2 + 아무 공급원 1. 구성은 **유닛마다 다르다**(2026-09-29 사용자 확인으로 정정 — 이전 일반식 `자기 원소 × (level − 1) + Sun × 1`은 폐기): 원소 유닛 L1 = 자기 원소 1(예: **Bulf = Thunder 1**, Thunder Generator 또는 Thunder Temple 필요), L2 = 자기 원소 1 + Sun 1, L3 = 자기 원소 2 + Sun 1, Sun 유닛 = Sun × 레벨, Generator·Outpost = 아무 1. 레벨은 패치판 `.type` 기준. 문서 4절에 전 유닛 필요 공급원 표, [PDF 유닛 핸드북 대조](docs/sources/pdf-manual.md#유닛별-건설-에너지-energy-to-build)
    - **소모 개념 아님**(같은 공급원 범위에 여러 유닛 가능), 조건은 **건설·생산 순간에만** 확인(공급원이 파괴돼도 유닛 유지), **이동 유닛도 생산 지점에 에너지 필요** — 질문 A~F 모두 확정 (문서 5절)
  - 남은 일: exe 의 등록·추첨(`Deck.cpp`)·도움말의 워크샵 생산 칸(Level I 2개·II 3개·III 4개) 검증·업그레이드 비용/효과, 에너지 공급 범위·판정 함수, 등록 목록과 `myTech`·`techBit` 의 관계
- [ ] **전투**: 사거리·명중·피해 공식, 발사체 궤적, 특수 효과(`bomb*` 계열: 마비, 중력, 치유, 반역 등), 방어(차단벽·실드)
- [ ] **승패 조건**: 프리스트(priest) 사망/포획, 신전(temple) 파괴, 미션 스크립트 이벤트 발생 지점
- [ ] 미션 스크립트 인터프리터 동작(3단계 문법 명세와 교차 검증)
- [ ] AI 의사결정 루틴 (10단계 입력)
- [ ] 네트워크: 프로토콜 방식(TCP/IP, IPX), 동기화 모델(락스텝 여부), 패킷 형식 (13단계 입력)
- [ ] `PatchFixs.txt` 와 대조하여 **기준 버전** 결정 (원본 1.x 동작 vs 패치 10.7x 동작)
  - 2026-09-29 정리: [patch-history.md](docs/sources/patch-history.md) (주제별 변경), 원판 매뉴얼 수치 ↔ 현재 `.type` 대조표 [game-manual.md](docs/sources/game-manual.md) 7절. 보유 exe = **10.78**(게임 내 Version 창). 사용자 캡처·설명(섬 테마, 거주지 원소 그림, Edit 메뉴 등)이 모두 패치판 기능이므로 **패치판 동작 기준**이 자연스러움 (결정은 5절 2번)

**완료 기준**: `docs/gameplay/` 만 보고도 게임 규칙을 재구현할 수 있다.

### 5단계. 원본 분석 — 플레이 영상

영상별로 `docs/videos/<이름>.md` 에 타임스탬프 기반 관찰 노트를 작성한다.
화면 구성은 홈페이지의 [게임 인터페이스](https://hjow.duckdns.org/netstorm/learninterface.htm) 설명과 대조해 UI 요소 이름을 정한다 (1.6절).
원본을 직접 실행할 수 있으므로(1.4절), 영상으로 확인하기 어려운 부분은 원본을 실행해 캡처·측정한다.
**로컬 영상(`playingVideos/`)을 우선 사용**한다 (원본 화질, 내려받기 불필요). 로컬에 없는 캠페인 2-x 만 YouTube 를 쓴다.

| 영상 | 로컬 파일 (`playingVideos/`) | YouTube 주소 | 주요 관찰 항목 |
|---|---|---|---|
| 튜토리얼 (Early Missions 1~6) | `Netstorm Islands at war - Early Missions.mp4` | https://www.youtube.com/watch?v=CI3dCrUt4tY | 기본 조작, UI 레이아웃, 튜토리얼 메시지 흐름 |
| 캠페인 1-1~4 (The War Begins! ~ Fragile Fortune) | `Netstorm Islands at war - The War Begins! ~ Fragile Fortune.mp4` | https://www.youtube.com/watch?v=AMEzorbjQYQ | 초반 미션 흐름, 메뉴·브리핑 화면 |
| 캠페인 1-5 (Thundering Power!) | `Netstorm Islands at war - Thundering Power!.mp4` | https://www.youtube.com/watch?v=adR1Kap60hw | |
| 캠페인 1-6 (Dissolved Alliance) | `Netstorm Islands at war - Dissolved Alliance.mp4` | https://www.youtube.com/watch?v=PUeIPg1BEzo | 섬 테마 3종·소유권 변화 (스크린샷과 대조) |
| 캠페인 2-1 (The Noose) | 없음 | https://www.youtube.com/watch?v=zpZsx4dRac8 | |
| 캠페인 2-2 (Rain Vs. Rain) | 없음 | https://www.youtube.com/watch?v=2NxTN314RnE | |
| 캠페인 2-3 (Run For It!) | 없음 | https://www.youtube.com/watch?v=LHkgSp0J73E | |
| 캠페인 3-2 (To The Rescue!) — 2026-09-29 추가 | 없음 | https://www.youtube.com/watch?v=WDQSrGqZAH0 | Complete Victory 캠페인 유일한 영상 (미션 이름은 캠페인 순서로 추정, 영상 확인 시 검증) |
| 캠페인 3-1~3-5 연속 (Complete Victory 스피드런) | `[Youtube] 3-1 to 3-5.mp4` (일부 PC) | https://www.youtube.com/watch?v=0p7VvzSxTAY | 업로더 챕터: Breaking Through 0:00 · To The Rescue! 13:30 · Vicious 27:05 · Enemy Territory 30:29 · Final Confrontation 34:12 (2026-10-01 `youtube_probe`) |
| 전용 채널 (미션별 스피드런 다수) | 없음 | https://www.youtube.com/@netstormcampaigns2591 | `youtube_list`로 목록 조회 (2026-10-01 추가) |

* 로컬 영상은 1024×768 풀스크린을 16:9 모니터에서 녹화해 **좌우 검은 여백**이 있다. 프레임을 뽑을 때(`ffmpeg`, 결과는 `extracted/videos/`) 가운데 4:3 영역을 잘라 1024×768 로 맞춘 뒤 스크린샷 분석과 같은 좌표계로 잰다.
  * 형식: **AV1 1920×1080 60fps** + AAC (4개 모두 확인). 게임 화면 = x 240~1679 의 1440×1080 (프레임 4장에서 측정, 계산값과 같음), 1.40625배 확대. 창 모드 스크린샷보다 밝고 채도가 높아(AGENTS.md: HDR 설정 문제로 밝게 촬영) 색 비교에는 쓰지 않는다
  * [x] 프레임 추출 도구 `tools/videoframes.py` (`probe`·`frame`·`range`, 게임 영역 잘라 1024×768 로 축소 또는 `--native`) — 2026-09-28, [사용법](docs/videos/README.md)
  * 추출 예: `ffmpeg -ss 00:01:00 -i "<영상>" -frames:v 1 -vf "crop=1440:1080:240:0,scale=1024:768:flags=area" out.png` — 확대가 정수배가 아니라 축소해도 원본 픽셀과 정확히 같지는 않으므로, 정밀 측정(스프라이트 템플릿 매칭 등)은 스크린샷을 우선하고 영상은 **시간 측정**(애니메이션·건설·다리·이동 속도, 60fps → 약 16.7ms 단위)에 주로 쓴다
  * AV1 소프트웨어 디코딩은 느리므로 긴 구간 전체를 풀지 말고 `-ss`/`-t` 로 필요한 구간만 뽑는다
* 방송 음성이 섞여 있으므로 소리 관련 관찰(효과음·음악 전환 시점)은 참고용으로만 쓴다.
* 미션 이름 대응은 영상 파일 이름과 공식 캠페인 구성([mission-script.md](docs/formats/mission-script.md)) 기준. YouTube 2-x 의 미션 이름은 A Nation Rises 순서로 추정한 것이므로 영상 확인 시 검증한다.

- [ ] 화면 구성(HUD, 사이드바, 미니맵, 버튼 배치, 커서 종류) 캡처 및 레이아웃 좌표 기록
  - [x] 원본 캡처 1: 튜토리얼 "Bridge the Gap!" 시작 직후 — 2026-09-28 완료: [docs/screens/bridge-the-gap-start.md](docs/screens/bridge-the-gap-start.md)
    (클라이언트 1024×768 위치, 사이드바·미니맵 배치, 맵 데이터 대조. 처음의 칸 크기 가설 32×22px 은 캡처 2 로 정정됨)
  - [x] 원본 캡처 2: 캠페인 1-1 "The War Begins!" 시작 직후 — 2026-09-28 완료: [docs/screens/the-war-begins-start.md](docs/screens/the-war-begins-start.md)
    (**좌표계 확정**: 위치 바이트 상위=x·하위=y, 청크 i → (i%16, i/16), 미니맵 1px = 2칸, 칸 = 16 × 약 11.25px(→ 캡처 3 에서 16 × 11 로 정정). noIsland = 가이저 받침 바위, 섬 지형 테마 = 신전 원소 추정)
  - [x] 원본 캡처 3: 캠페인 1-3 "Save the Island!" 시작 직후 (유닛·다리·바리케이드 배치) — 2026-09-28 완료: [docs/screens/save-the-island-start.md](docs/screens/save-the-island-start.md)
    (**템플릿 매칭으로 정밀 확정**: 칸 = 정확히 16 × 11px, 모든 타입이 칸 좌표→화면 점에 스프라이트 기준점(0,0)을 그림, TerrNN 청크 = 영역 소속 청크를 y·x 순으로 훑은 것. Sun Barricade = 두 기둥 사이 광선, 미니맵은 다리도 소유자 색으로 표시)
  - [x] 원본 캡처 4~42 목록·캡처별 관찰 정리 — 2026-09-28: [docs/screens/README.md](docs/screens/README.md)
    (메인 메뉴 계열 19장, The War Begins! 19장, Dissolved Alliance! 3장, 도움말 1장. 메인 메뉴 타이틀 그림 640×480 가운데 (192, 144), 도움말 = `help.english` `F1Help` 절(메뉴에서는 `<?{global.inMission}>` F8 줄 숨김 — 조건 평가 구현과 일치), Credits의 10.72는 제작진 명단 제목이며 exe 버전 증거가 아님, Storm Power 색 흰/노랑/빨강 실례, 정보 창·게임 메뉴·결과 창 구성)
  - [ ] 정밀 노트 작성(좌표·글꼴 측정): 메인 메뉴·대화상자(`docs/screens/main-menu.md`), 도움말 창, 미션 화면 정보 창·게임 메뉴 막대, 결과 창
    - 할 일: 타이틀·버튼·돌 테두리·스크롤바 그림의 원본 자산 찾기(exe 리소스 비트맵, `TITLE*.COL` 짝 그림), 줄 간격·글꼴(`!Arial.*.chfnt` 중 어느 것인지), 메뉴 정의가 exe 하드코딩인지 스크립트(`offical*.english`·`tell.english` 의 `$Button=`)인지 확인, 도움말 링크(`#앵커`, `cmd:Tell,…`, `http…`) 동작
  - [x] Dissolved Alliance! 시작 캡처와 `--map dissolvedalliance` 대조 — 2026-09-28: [dissolved-alliance-start.md](docs/screens/dissolved-alliance-start.md)
    - 섬 모양·다리·가이저·건물 위치 일치. **플레이어 1 = 가운데 바람 섬**, 왼쪽 눈 섬 = Duke of Rain(소유자 3), 오른쪽 돌 섬 = Prince of Thunder(소유자 2) (목록 문서의 "눈 섬 = 플레이어" 정정)
    - **원본 시작 카메라 규칙**: 플레이어 1 사제 칸 기준점이 클라이언트 약 (525, 393) (캡처 3장 공통 ±4px) → 맵 뷰어 시작 카메라에 반영, 원본 캡처와 ±6px 이내로 겹침
    - **거주지(Residence) 외관 = 섬 원소별 그림** 확인 → 뷰어가 영역 원소의 lit 그림을 고르도록 수정 (`MapSpriteFrames.BodyFrame(FortMapObject, theme)`, `FortTerrainPreview.TerritoryTheme`), 검사 9개 추가·전체 147개 통과
    - 남은 일: `Playing 1`·`Playing 2`(시점 이동 캡처)의 카메라 위치를 템플릿 매칭으로 구해 섬별 대조, 소유자 3 이상의 플레이어 색, 원본의 거주지 원소 재선택 경로(exe), 섬 가장자리 풀 장식
  - [ ] The War Begins! 진행 캡처로 **신전 원소 → 지면 테마**(비 → 눈·얼음, 바람 → 갈색 풀밭, 번개 → 어두운 돌, 신전 없음 → 초록 풀밭)와 소유권 변화 확인 (4단계 exe 분석으로 확정)
- [ ] 메뉴 흐름도(타이틀 → 캠페인/멀티 → 브리핑 → 게임 → 결과)
  - [x] 2026-09-29 원본 실행으로 메인 메뉴 전 항목(멀티 제외)·데모·미션 진입/이탈·편집기 진입/이탈 흐름 확인 → [main-menu.md](docs/screens/main-menu.md) "메뉴 흐름도". 남은 일: 결과 화면 이후, Replay/Restart, 멀티플레이 화면
- [ ] 애니메이션 속도·연출(건설, 다리 설치, 폭발, 승리/패배), 사운드·음악 재생 타이밍
  - [x] 반복 애니메이션 속도 — 2026-09-28: 가이저 증기 약 24Hz(41.7ms), 신전 회오리·피해 연기 12Hz(83.3ms). `tools/videoframes.py cadence` 로 영상 4개 중 3개에서 재현 측정 ([animation-timing.md](docs/videos/animation-timing.md))
  - [ ] 남은 일: 건설·다리 설치·폭발·유닛 동작별 속도, 승리/패배 연출
- [ ] 수치 검증용 관찰(건설 시간, 공격 간격, 자원 증가 속도) → 4단계 결과와 대조
- [ ] 각 미션의 시작 상태·목표·스크립트 이벤트 발생 순서

**완료 기준**: 구현 결과를 영상과 나란히 비교할 체크리스트가 준비된다.

### 6단계. 자산 로더 / 개발용 뷰어 (C#, `Netstorm.Assets` + `Netstorm.Game`) — 🔶 진행 중

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 7단계. 엔진 코어 (플랫폼 계층)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 8단계. 게임 월드 / 규칙 구현

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 9단계. UI · 미션 스크립트 · 튜토리얼

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 10단계. AI

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 11단계. 캠페인 · 저장

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 12단계. 다국어 지원 (설계는 2단계부터 반영)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 13단계. 멀티플레이

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 14단계. 패키징 · 배포

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

### 15단계. 검증 · QA (상시)

> **dotnetpj 이력 이관:** [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-103)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

## 5. 결정이 필요한 사항

> **기준 버전 갱신:** 아래 2번은 당시 비교 방침이다. 현재 cpppj 1차 복원 기준은 10.78이며 10.37·10.62·CD·10.82는 비교 자료다. dotnetpj도 10.78 기준 확정 자료를 우선한다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-104)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

2. **기준 버전**: 원본 1997 동작 vs Ticonderoga 패치(10.7x, 보유 exe) 동작 — 보유 exe 는 패치판이므로 기본적으로 패치판 동작을 따르되, 차이점은 `PatchFixs.txt` 로 문서화
3. ~~**자산 정책**~~ → **결정됨** (AGENTS.md): 원본 파일 재활용 가능. 저장소에는 커밋하지 않고, 배포 시 필요한 자산을 동봉한다
4. ~~**멀티플레이 범위**: LAN 만 / 인터넷 로비 포함 / 원본 호환~~ → **cpppj는 결정됨(AGENTS.md 2026-10-09 변경): TCP/IP 기반 로컬 네트워크만 구현한다.** 공식 서버는 도메인이 없어져 접속을 복원하지 않는다. cpppj의 3차 목표(Windows 10/11 오류 없는 실행 뒤)이며 멀티플레이 전용인 outpost 구현을 포함한다. dotnetpj의 멀티플레이 범위는 AGENTS.md에 따로 적혀 있지 않다(cpppj 완성 뒤 분석해 개발). 그 전 기록: 멀티플레이 자체가 후순위(2026-09-30 사용자 결정)
5. ~~**와이드 화면(16:9·16:10) 처리**~~ → **결정됨(2026-09-29, 사용자): 맵 시야 확장.** 4:3 + 좌우 여백(레터박스) 화면은 필요 없다. 근거: 원본은 4:3 만 지원하고(최대 1024×768) 카메라 확대가 없으며 해상도를 높이면 더 넓은 맵이 보였다 — 와이드에서 맵을 옆으로 더 보여 주는 것이 그 동작과 가깝다 (1.7절). 남은 문제: 멀티플레이에서 해상도(화면비)에 따른 시야 차이를 허용할지 (13단계)

## 5-0. 현재 진행 상황 (2026-09-29 세션 종료 시점)

- **공통/구현 경계:** 와이드 화면은 사용자가 맵 시야 확장으로 정했다. 당시 뷰어 4줄(128px) 변경·화면 확인·222개 테스트는 C# 이력에 보존했다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-105)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

**이 세션에서 완료 (모두 문서 작업·정적 분석, 코드 변경 없음, 커밋 전)**
* AGENTS.md 추가 사항 반영: 캠페인 3-2 YouTube 영상 (5단계 영상 표)
* 동봉 문서 전체 정리 → [docs/sources/](docs/sources/README.md) (게임 매뉴얼·패치 이력·설치/지원 문서, 불일치 12건)
* 에너지 공급 범위 exe 확인 → [docs/exe/battle-options.md](docs/exe/battle-options.md) (Generator Range 14/22/30/38, 30 상한, 튜토리얼 2 만 14칸, 전투 옵션 21종 표)
* 불일치 해소 3건: 파일 조회 순서 = **디스크 우선 유지**([vfs.md](docs/formats/vfs.md)), Storm Power 색 = **≤1000 빨강 / ≤2000 노랑 / 그 이상 흰색**(`0043da10`), `bridgeDrawRate`·`stuffRefreshRate` 설정 키는 패치판 exe 에 없음

**이후 이어서 진행 (2026-09-29 후속 세션) — 코드 변경 포함, 커밋 전**
* 가장자리 스크롤 exe 분석 → [docs/exe/edge-scroll.md](docs/exe/edge-scroll.md) (1픽셀 가장자리, 속도 2 → +30/초 → 상한 `edgeScrollSpeed` 35 프레임당 픽셀, 창 테두리 없을 때만, 위쪽은 메뉴 막대 예외, 왼쪽 버튼·Shift 조건)
**당시 중단된 작업 → 2026-09-29 후속 분석으로 처리**
* ~~level 1 원소 유닛의 필요 에너지 판정 함수 확인~~ → 타입 구조체 `+0xA0` 요구 문자열, `Rifttype.cpp` `0049b0d0` 생성 및 `Mana.cpp` `004734d0`·`00473330` 검사 확인. 이전 후보 `0044ac30`·`00473d00`은 다른 구조체의 필드였다. 결과는 [energy-requirements.md](docs/exe/energy-requirements.md), 게임 내 재현은 6절 후속 항목에 기록

**커밋되지 않은 변경 (이번 세션 마지막 작업분)**: `LEFT_JOBS.md`, `docs/formats/vfs.md`, `docs/gameplay/elements-energy.md`, `docs/screens/README.md`, `docs/sources/README.md` (그 이전 작업분은 0929 01·02 커밋에 포함됨)

## 5-1. 현재 진행 상황 (2026-09-28 세션 중단 시점 인수인계 → 같은 날 재개·처리)

- **공통 조사:** Dissolved Alliance! 원본 시작 카메라·거주지 원소별 외관은 [관찰 노트](docs/screens/dissolved-alliance-start.md)를 참고한다. 뷰어 반영·138/147개 테스트는 C# 이력이다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-106)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

작업 도중 셸 명령(Bash·PowerShell)이 자동 모드 안전 확인 단계의 무응답으로 계속 거부되어 세션을 멈췄다. 파일 읽기·수정만 가능했던 구간의 결과이므로 아래 "미검증" 항목을 다음 세션 처음에 확인한다.

**이 세션에서 완료한 것**
* 문서 (셸 없이 작성, 코드 변경 없음):
  * [docs/screens/README.md](docs/screens/README.md) — 스크린샷 42장 목록·캡처별 관찰, 원본 해상도 4:3 세 가지뿐·카메라 확대 없음(사용자 확인), 신전 원소 → 지면 테마 추정
  * [docs/gameplay/island-ownership.md](docs/gameplay/island-ownership.md) — 섬 소유권 규칙(사용자 확인): 테두리 색 = 소유자, 신전이 있어야 소유, 소유 섬에서만 배치·다리 시작, 비소유 섬은 통과만
  * 본 문서: AGENTS.md 변경(화면비·풀스크린·가장자리 스크롤·스크린샷·로컬 영상) 반영, `originals/` 커밋 방침 변경, 로컬 영상 형식(AV1 1920×1080 60fps, 사용자 확인) 반영
**위 중단 시점의 미확인 항목 → 2026-09-28 재개 후 처리 결과**
1. ~~커밋 여부 확인~~ → 사용자가 모두 커밋함 (재개 시점에 미반영 파일 없음)
2. ~~나머지 영상 형식·여백 측정~~ → 4개 모두 AV1 1920×1080 60fps, 게임 영역 x 240~1679 측정 ([docs/videos/README.md](docs/videos/README.md))
4. 영상 프레임 추출 도구 `tools/videoframes.py` 작성 완료. `docs/videos/` 미션별 관찰 노트는 **미착수** → 6절 5번

## 6. 바로 다음 작업

- **공통 후속 분석:** 지면·절벽·받침·소유자색·edgeFarm 프레임/좌표식은 [지면·다리](docs/exe/terrain-and-bridges.md)·[영역 배치](docs/exe/territory-layout.md)를 참고한다. 개별 난수·전역 시드/소비 순서·깊이/그림자·동적 받침/소유자 전파·회전 영역은 당시 미확인이었다. C# 뷰어 완료/검증/근사는 이관했다.
- **공통 포맷/영상:** 섹션은 [fort.md](docs/formats/fort.md), 캠페인/영상 대응은 [videos/README.md](docs/videos/README.md), 시간 간격은 [animation-timing.md](docs/videos/animation-timing.md), 에너지 범위는 [battle-options.md](docs/exe/battle-options.md), 덱 재충전은 [production-refresh.md](docs/exe/production-refresh.md)를 따른다. FortFile/GameResources/미리보기와 테스트 결과는 C# 이력이다.

> **공통 분석·자료·도구:** 아래 남은 원본 근거는 두 프로젝트가 참고한다. [C# 구현·검증·후속 기록](LEFT_JOBS.dotnetpj.md#legacy-dotnet-107)으로 이관했다. 당시 “클론”·Core/Assets/Game·MonoGame 검증은 dotnetpj에 해당하며 cpppj 완료 근거가 아니다.

0. **동봉 문서 참고 (2026-09-29 정리)**: 작업 전에 [docs/sources/README.md](docs/sources/README.md) 를 먼저 본다 — 규칙·조작·유닛 수치와 **현재 분석과의 불일치 12건**(3절)이 정리되어 있다. 사용자가 외부에서 추가한 [`originals/help/manual.pdf`](originals/help/manual.pdf)는 조작·생산 절차와 일부 비용을 `GAME.HLP`·보유 `.type`에 [선별 대조](docs/sources/pdf-manual.md)했다. 메인/컨텍스트 메뉴 그림은 실제 UI 형태와 부합한다(사용자 확인). 옛 메인 메뉴의 기본 항목은 7개, 보유 패치판은 `Edit` 포함 8개다. 비용·일부 메뉴 항목은 판본에 따라 다르며 전체 페이지 대조는 남아 있다. 우선 확인할 것:
   - ~~level 1 원소 유닛(Bulf·Arc Spire·Crystal Crab)의 필요 에너지 exe 분석~~ → **2026-09-29 완료**: `Rifttype.cpp` `0049b0d0`이 타입 `+0xA0`의 기본 요구값을 만들고, `Mana.cpp` `004734d0`·`00473330`이 배치 위치에서 검사한다. Bulf·Arc Spire는 Thunder 1, Crystal Crab은 Rain 1. Generator는 `.type`의 명시 `mana = "s"`로 아무 공급원 1개. 이전 후보 `0044ac30`·`00473d00`은 다른 구조체 필드로 인한 오인. [분석](docs/exe/energy-requirements.md), [유닛 표](docs/gameplay/elements-energy.md) 4절. ~~**후속**: 패치판 게임에서 교차 원소 공급원 아래 세 유닛의 배치 성공/실패를 동적으로 확인~~ → **2026-09-29 사용자 확인으로 확정**(Bulf는 Thunder 공급원 필수, 다른 원소는 소용없음, 같은 레벨이라도 유닛마다 다름). PDF 유닛 핸드북 전체와도 대조함 ([PDF 대조표](docs/sources/pdf-manual.md#유닛별-건설-에너지-energy-to-build)) — 게임 실행 검증 불필요
   - ~~파일 조회 순서~~ → 2026-09-29 재확인: 보유 exe 는 **디스크 우선** (열기 22곳·이름 해석 12곳 모두 디스크 먼저, 아카이브 우선 존재 검사 18곳은 결과를 있음/없음으로만 사용). 패치 문서 문장은 이 exe 의 열기 동작과 맞지 않음 — [vfs.md](docs/formats/vfs.md)
   - ~~Storm Power 숫자 색 기준값~~ → 2026-09-29 확인: ≤1000 빨강, 1001~2000 노랑, 그 이상 흰색 (`0043da10`)
4. 4단계: 메인 루프/틱, 다리 생성(`Deck.cpp`, `Bridge.cpp`), 경제 분석
6. (원격 저장소가 생기면) CI 실제 실행 확인 — `originals/` 포함 후 원본 검증 테스트까지 실행되는지
### 참고: 3단계에서 만든 도구 사용 순서

[docs/formats/README.md](docs/formats/README.md) "추출 순서" 참고. 추출 결과(`extracted/`)는 git 에 포함되지 않으므로 새 환경에서는 다시 실행해야 한다.

### 인수인계 문서 임시 분할

dotnetpj 와 병행 작업하기 위해, 
이 LEFT_JOBS.md 에는 당분간 cpppj 및 디컴파일 관련 진행 상황과 그 인수인계 내용을 적는다.
dotnetpj 진행 상황과 인수인계 내용은 LEFT_JOBS.dotnetpj.md 에 적는다
cpppj 1차 목표 달성 및 윈도우 10, 11 호환성 작업, MCP 추가 작업까지 완료되어 dotnetpj 구현에 cpppj 를 활용할 수 있게 되면, 인수인계 문서를 다시 LEFT_JOBS.md 로 통합한다.
