# 외부 프로젝트 `SAVACAZAN/NETSTORM`(netstorm-py) 검토와 cpppj 반영

> 검토일: 2026-10-10, PC `HJOW-X3D`(Windows 11). 문서 작업이며 cpppj 코드는 바꾸지 않았다.
> 대상: `https://github.com/SAVACAZAN/NETSTORM.git`의 `main`, 커밋 `d522d36`(커밋 3개). 다른 사람이 진행 중인 NetStorm 역분석·재구현 프로젝트다. PC별 클론 위치는 0절에 있다.
> **이 PC는 마지막 디컴파일 수행 PC(`HJOW-Athlon`)가 아니고, 디컴파일을 다시 하지도 않았다.** 그래서 `extracted/`의 디컴파일 산출물은 읽지 않고 **원본 exe의 바이트만** 읽어 확인했다(`tools/exe_callscan.py`, `tools/shp.py`). 원본 게임·분석 프로그램·클론 창은 실행하지 않았고, 상대 저장소의 스크립트·바이너리도 실행하지 않았다. 원본 폴더와 AGENTS.md는 바꾸지 않았다.

## 0. 상대 저장소의 위치 (PC별)

상대 저장소는 우리 저장소 **밖**에 따로 클론해 두고 읽기만 한다. 우리 저장소에 넣거나 커밋하지 않는다.

| PC | 경로 | 상태 |
|---|---|---|
| `HJOW-X3D` | `H:\Workspace\git\NETSTORM` | 클론돼 있다. 이 문서의 검토는 여기서 했다(커밋 `d522d36`) |
| `HJOW-Athlon` | `H:\T5Workspace\NETSTORM` | **사용자가 클론해 둘 예정**(2026-10-10 사용자 안내). 이 문서를 쓸 때는 아직 확인하지 못했다 |

다른 PC에서 이어 볼 때:

1. 폴더가 있는지와 커밋을 먼저 확인한다 — `git -C H:\T5Workspace\NETSTORM log -1 --oneline`. 없으면 `git clone https://github.com/SAVACAZAN/NETSTORM.git H:\T5Workspace\NETSTORM`.
2. 커밋이 `d522d36`이면 이 문서가 그대로 유효하다. **더 새 커밋이면** 바뀐 부분(`git -C <경로> log d522d36..HEAD --stat`)만 다시 읽고 이 문서를 갱신한다. 상대 프로젝트가 진행 중이라 5절의 오류가 고쳐졌거나 새 내용이 생겼을 수 있다.
3. 그쪽 스크립트·`harness/`의 DLL/주입 도구는 실행하지 않는다. 실행 중인 원본 프로세스에 붙는 도구라 AGENTS.md의 원본 실행 확인 규칙에 걸리고, 남이 만든 실행 파일이기도 하다. 문서와 소스를 읽어 주장만 가져온 뒤 **우리 exe 바이트로 확인**한다(7절).
4. 그쪽 주소는 모두 CD/10.37 계열 기준이다(3절). 10.78에 쓰려면 반드시 대응 주소를 다시 찾는다.

## 1. 무엇을 봤나

상대 프로젝트는 원본을 **Python + raylib**로 다시 만드는 것이 목표다(문서는 루마니아어·영어, 2026-05-02~03 작성). 내용은 다음과 같다.

| 구성 | 내용 | 이번에 한 일 |
|---|---|---|
| 문서 14개 (`README.md`, `CLAUDE.md`, `GEMINI.md`, `status_proiect.md`, `engine_flow.md`, `SCRIPTS.md`, `fort_entity_spec.md`, `sprite_name_mapping.md`, `shp_decoder_*.md`, `SHP_STATUS.md`, `PALETTE_DISCOVERY.md`, `RENDER_BLOCKERS.md`, `PROMPT_EXTERNAL_AI.md`, `planul1.md`, `raspuns.md`) | 주소 표, 포맷 해석, 메인 루프 해석, 막힌 점 | **전부 읽음** |
| `src/netstorm/` | TAFF·SHP·CHFNT·FORT·TYPE 읽기, 36바이트 슬롯 모형, 월드·그리기 뼈대 | `squid.py`만 읽음 |
| `harness/` | 실행 중인 원본에 DLL을 넣어 프레임마다 상태를 받는 기록기, 메모리 읽기 기록기, `.nsrec` 형식 | `record.py`·`netstorm_hook.c` 읽음 |
| `ghidra/NetStormRE.zip` (7.9MB) | Ghidra 프로젝트(이름 붙이기 없이 자동 분석만 했다고 적혀 있다) | 열지 않음 |
| `extracted/` (8,321개) | 내보낸 PNG·HTML | 쓰지 않음 |

## 2. 결론

**우리 쪽 분석이 훨씬 앞서 있다.** 상대가 "확인"이라고 적은 것 가운데 상당수는 우리 문서에 이미 더 정확히 있고, 일부는 틀렸다(5절). 그래도 그쪽 주장을 exe 바이트와 맞춰 보는 과정에서 **우리 문서에 없던 것 두 가지와 우리 문서의 오류를 얻었다.**

| 구분 | 항목 | 반영 |
|---|---|---|
| 새 정보 (기계어 확인) | **게임 상태 번호 43개의 이름과 상태 변수 주소** | [4.1절](#state-table) |
| 우리 문서 정정 (기계어 확인) | 메인 루프 갱신 목록: **첫 호출 누락**, **CD판 다리 스캔 대응**, 썽크, 패치판에만 있는 마지막 호출 | [4.2절](#update-list), [main-loop.md](main-loop.md) 2절 |
| 새 정보 (기계어 확인) | CRT `rand`는 **스레드마다 상태가 따로**다. 주소·시드 지점 | [4.3절](#crt-rand) |
| 방법 (채택 여부는 사용자 결정) | 실행 중인 원본에서 프레임마다 상태를 받아 비교하는 방식 | [4.4절](#runtime-observation) |
| 참고 (확인 불가) | 그쪽이 잰 실제 프레임 속도 71.3 / 64.9fps | [4.5절](#external-fps) |
| **채택하지 않음** | 스프라이트 이름 표(한 칸 밀림), `.fort` "타일 격자", "게임 난수 = MSVC rand", 갱신 함수 용도 추측 등 | [5절](#rejected) |

## 3. 그 프로젝트가 분석한 판본 — CD/10.37 계열

상대 문서의 주소는 모두 **우리 CD판(`originalCD/NETSTORM.EXE`)과 일치**한다. 10.78 주소가 아니다.

* 그쪽이 적은 문자열 주소 12개가 CD판 exe에서 그대로 나온다: `windy.col` `0051a1d0`, `rainy.col` `0051a1dc`, `thundery.col` `0051a1e8`, `sunny.col` `0051a1f8`, `_shapes.shp` `0051d73c`, `masterShapeDatabasePtr` `0051d77c`, `pendState.state == S_INVALID` `00518e04`, 모드 상호 배제 검사 문자열 `00519444`, `sendFort from %d to %d` `0053da94`, `fortDataImage.end` `0051963c`, `fortFileSize != (int)-1` `005104dc`, `ArchiveFortSpec` `0053f1d0`.
* 함수 주소도 우리 CD 열과 같다: WinMain `00485b20`, 시각 고정 `004012d0`, 입력·명령 처리 `0041ae80`, 그리기 `00486df0`([main-loop.md](main-loop.md)).
* 그쪽의 "함수 3,977개, 끝 주소 `006091ff`"는 CD판 이미지 끝(`.reloc` `005f5000` + `0x141ba`)과 맞는다. 10.78은 `.reloc`이 없고 끝이 `0061cfff`다.

우리 CD판과 추가 10.37 사본은 `InsertCD` 분기 한 바이트만 다르다([original1037-comparison.md](original1037-comparison.md)). 그쪽 exe가 둘 중 어느 쪽인지는 파일이 없어 확인하지 못했다("RIP" 배포본이라 CD 확인을 건너뛰는 쪽일 가능성이 높다 — 추정).

**읽을 때 주의:** 그쪽 주소·구조체 크기는 우리 문서의 **"CD판" 열에만** 대응한다. 슬롯 폭도 CD 36바이트 대 10.78 50바이트다([cpp-reference-versions.md](cpp-reference-versions.md)). cpppj의 복원 기준은 계속 10.78이다.

## 4. 새로 얻은 것

<a id="state-table"></a>
### 4.1 게임 상태 번호 표 (10.78·CD 공통)

상대 문서는 `S_FORT_*`·`S_BATT_*`·`S_META_*`·`S_GAME_*` 문자열이 "약 37개"라고만 적고 번호는 추측했다. exe에는 **이름 포인터 표**가 있고 상태 기계가 **상태 번호로 그 표를 찾는다.** 그래서 번호가 확정된다. 두 판본의 순서와 개수(43개, 0~42)가 같다.

| | 10.78 | CD판 |
|---|---|---|
| 이름 포인터 표 (43칸) | `00542548` | `00518968` |
| 상태 기계 본체 (State.cpp) | `004b6dd0` | `0042d630` |
| 표를 찾는 명령 | `004b6e28` `mov ecx, [eax*4 + 0x542548]` | `0042d691` `mov ecx, [eax*4 + 0x518968]` |
| 분기 | `상태 - 1`을 `0x29`와 비교한 뒤 42갈래 (점프 표 `004b874c`) | 같음 (점프 표 `0042f260`) |

상태 기계는 진입할 때 현재 상태의 이름을 형식 문자열 `"%s"`와 함께 출력 함수(10.78 `0045ad00`, CD `0047ca50` — 로그로 추정)에 넘긴 뒤 상태별로 분기한다.

| 번호 | 이름 | 번호 | 이름 | 번호 | 이름 |
|---|---|---|---|---|---|
| 0 | `S_INVALID` | 15 `0x0f` | `S_BATT_SERVER_TAKEOVER` | 30 `0x1e` | `S_META_CHAL_AWAITING_MOVE_REPLY` |
| 1 | `S_FORT_INIT` | 16 `0x10` | `S_BATT_AWAITING_RECONNECT_REPLY` | 31 `0x1f` | `S_META_CHAL_MOVE_REPLY` |
| 2 | `S_FORT_CREATE` | 17 `0x11` | `S_BATT_RECONNECT_REPLY` | 32 `0x20` | `S_META_CHAL_LOGIN_DENIED` |
| 3 | `S_FORT_BLANK` | 18 `0x12` | `S_BATT_RECONNECT_DENIED` | 33 `0x21` | `S_META_CHAL_CONNECTED_MONITOR` |
| 4 | `S_FORT_RUNNING` | 19 `0x13` | `S_META_INIT` | 34 `0x22` | `S_META_FIND_SERVER` |
| 5 | `S_FORT_CHECK_STATE` | 20 `0x14` | `S_META_UNCONNECTED` | 35 `0x23` | `S_META_AWAITING_FIND_SERVER` |
| 6 | `S_FORT_META_INIT` | 21 `0x15` | `S_META_ROOT_CONNECT` | 36 `0x24` | `S_META_FOUND_SERVER` |
| 7 | `S_BATT_INIT` | 22 `0x16` | `S_META_ROOT_AWAITING_CONNECT` | 37 `0x25` | `S_META_LAUNCH_SERVER` |
| 8 | `S_BATT_CONNECT` | 23 `0x17` | `S_META_ROOT_AWAITING_REPLY` | 38 `0x26` | `S_META_AWAITING_LAUNCH_SERVER` |
| 9 | `S_BATT_AWAITING_CONNECT` | 24 `0x18` | `S_META_ROOT_REPLY` | 39 `0x27` | `S_GAME_INIT` |
| 10 `0x0a` | `S_BATT_AWAITING_REPLY` | 25 `0x19` | `S_META_CHAL_CONNECT` | 40 `0x28` | `S_GAME_INTRO` |
| 11 `0x0b` | `S_BATT_LOGIN_DENIED` | 26 `0x1a` | `S_META_CHAL_AWAITING_CONNECT` | 41 `0x29` | `S_GAME_INTRO_DONE` |
| 12 `0x0c` | `S_VERSION_DIFF` | 27 `0x1b` | `S_META_CHAL_AWAITING_REPLY` | 42 `0x2a` | `S_GAME_STARTUP` |
| 13 `0x0d` | `S_BATT_REPLY` | 28 `0x1c` | `S_META_CHAL_REPLY` | | |
| 14 `0x0e` | `S_BATT_RUNNING` | 29 `0x1d` | `S_META_CHAL_CONNECTED` | | |

기존 문서의 숫자와 맞는다: [main-loop.md](main-loop.md)의 "전투 상태(`0xe`)"는 `S_BATT_RUNNING`, [dialog-pause.md](../gameplay/dialog-pause.md)의 "상태 `0x27`에서 시작 영상, 이어 상태 `0x28`이 끝나기를 기다린다"는 `S_GAME_INIT` → `S_GAME_INTRO`다.

**상태 구조체.** 한 묶음이 dword `0x24`개(144바이트)이고 첫 dword가 상태 번호다. 상태 기계는 진입할 때 "현재"를 나머지 둘에 통째로 복사한 뒤 둘의 상태 번호를 0(`S_INVALID`)으로 지운다.

| 역할 | 10.78 | CD판 | 근거 |
|---|---|---|---|
| 현재 상태 | `005c87e8` | `00564d20` | 상태 기계가 여기서 번호를 읽어 분기한다 |
| 다음 상태 (이름은 우리가 붙임) | `005c8878` | `00564c90` | 처리기와 작은 설정 함수들이 상수를 기록한다 — 아래 표. "다음 상태"라는 뜻은 추정 |
| `pendState` | `005c86c8` | `00564f68` | 설정 함수 10.78 `004b2bc0` / CD `0042d600`이 assert `pendState.state == S_INVALID`로 이 칸이 0인지 확인한 뒤 인자를 기록한다 |

`pendState`에 상수를 직접 쓰는 명령(`mov [칸], 상수` 꼴만 셌다)은 다섯 값뿐이고 두 판본이 같다: `0x10`·`0x0e`·`0x23`·`0x25`·`0x1a` — `S_BATT_AWAITING_RECONNECT_REPLY`·`S_BATT_RUNNING`·`S_META_AWAITING_FIND_SERVER`·`S_META_LAUNCH_SERVER`·`S_META_CHAL_AWAITING_CONNECT`. 누가 언제 쓰는지와 "다음 상태"와의 우선순위는 읽지 않았다.

"다음 상태"에 상수를 쓰는 짧은 설정 함수들(값과 순서가 같아 판본끼리 짝지었다):

| 기록 명령 (10.78) | 기록 명령 (CD판) | 값 | 상태 |
|---|---|---|---|
| `004b3500` | `0042fd10` | `0x2a` | `S_GAME_STARTUP` |
| `004b3520` | `0042fd30`, `0042fd50` | `0x27` | `S_GAME_INIT` |
| `004b3540` | `0042fd70` | `0x03` | `S_FORT_BLANK` |
| `004b3550` | `0042fd80` | `0x06` | `S_FORT_META_INIT` |
| `004b3567` | `0042fe00` | `0x01` | `S_FORT_INIT` |
| `004b3781` | `0042ffe9` | `0x13` | `S_META_INIT` |
| `004b37b0` | `004300b0` | `0x15` | `S_META_ROOT_CONNECT` |

**cpppj에서 쓰는 법.** State.cpp(미션 시작 절차·결과·메뉴 복귀)를 복원할 때 숫자 대신 이 이름을 쓴다. `S_META_*`는 메타서버(공식 서버 대기실) 상태라 공식 서버를 복원하지 않는 cpppj의 대상이 아니고, `S_BATT_CONNECT`~`S_BATT_RECONNECT_DENIED`의 접속 계열은 3차(LAN) 범위다. **상태 사이의 전이 순서는 이번에 읽지 않았다** — 싱글플레이가 어떤 순서로 지나가는지는 `004b6dd0`의 처리기를 읽어 확정해야 한다.

<a id="update-list"></a>
### 4.2 메인 루프 갱신 목록의 기계어 대조

상대 문서는 CD판 WinMain의 갱신 호출을 "19개"로 적었다. 우리 [main-loop.md](main-loop.md) 2절의 표와 맞지 않아 두 판본의 호출 명령을 직접 읽었다. **두 판본은 21자리가 1:1로 대응한다.** 프레임 갱신 정지 값(10.78 `DAT_0054dc58`, CD `DAT_0052ea34`)이 1 이상이면 이 목록을 건너뛰고 창 프레임 처리만 부른다.

| # | 10.78 (`00439aca`~) | CD판 (`0048675d`~) | 비고 |
|---|---|---|---|
| 1 | `00435060(0)` | `00433bf0(0)` | **우리 표에 없던 첫 호출.** CD판은 CD 확인(`InsertCD` 문자열을 쓴다), 10.78은 `mov eax, 1; ret`뿐이다 |
| 2 | `00441de0` | `004a9c30` | |
| 3 | `004b9010` | `004333d0` | |
| 4 | `004437f0` (빈 함수) | `00438e40` (빈 함수) | |
| 5 | `004b67c0(0)` | `004336e0(0)` | |
| 6 | `004180f0` | `004eca20` | |
| 7 | `004dd330` → `jmp 004dd2b0` | `00450490` → `jmp 004503f0` | 호출 대상은 **썽크**다. 몸체는 우리 표의 주소가 맞다 |
| 8 | `004182c0` | `004ecb30` | |
| 9 | `004437f0` (빈 함수) | `00462eb0` (빈 함수) | CD판은 4번과 **다른** 빈 함수다 |
| 10 | `00422bc0` | `00449250` | **다리 붕괴 스캔.** 우리 표는 CD 쪽을 "`00462eb0` 또는 `00449250`"이라고 적었는데 `00462eb0`은 빈 함수다. [cpp-bridgedecay-reconstruction.md](cpp-bridgedecay-reconstruction.md)의 대응과도 같다 |
| 11 | `00469f60` | `00436ca0` | |
| 12 | `00446c50` | `0043e280` | |
| 13 | `00481320` | `00442d10` | |
| 14 | `004662b0` | `004929b0` | |
| 15 | `00464170` | `00492990` | |
| 16 | `004ab9a0` — `DAT_0054db1c`가 0이 아닐 때만 | `004879c0` — `DAT_0052e8b0`이 0이 아닐 때만 | |
| 17 | `00417de0` | `004d7350` | |
| 18 | `00471a30` | `004ed5d0` | 프로세스 커널 |
| 19 | `00486bc0` | `00415b30` | |
| 20 | `004b6730` | `004326a0` | |
| 21 | `0044ac30` | `00484140` | |
| — | `004d3090(0x10, 1, 1, 0, "")` — `DAT_00594fbc`(`inBattleMode`)와 `DAT_00594fc0`이 모두 0이 아닐 때만 | 없음 | **10.78에만 있는 마지막 호출.** 내용은 읽지 않았다 |

상대 문서의 19개는 7번(썽크)과 16번(조건부)을 빠뜨린 것이다. 각 함수가 하는 일은 [main-loop.md](main-loop.md)의 설명을 그대로 둔다(상대 문서의 용도 추측은 5절).

<a id="crt-rand"></a>
### 4.3 CRT `rand`는 스레드마다 상태가 따로다

상대 프로젝트는 "난수는 MSVC `rand`이고 상태가 스레드 지역 저장소(TLS)에 있다"는 것을 찾아 기록기를 만들었다. **스레드별이라는 점은 맞고 우리 문서에 없던 내용이다.** 다만 "게임 난수 = `rand`"는 틀렸다(5절).

| | 10.78 | CD판 |
|---|---|---|
| `srand` | `004e649e` | `004f23a0` |
| `rand` | `004e64ab` | `004f23b0` |
| 스레드 자료 얻기 (`_getptd`) | `004ea61d` | `004f6010` |
| TLS 색인을 담은 전역 | `00543ca8` | `005472f4` |
| 상태의 위치 | 스레드 자료 `+0x14` | 스레드 자료 `+0x14` |

* 식은 표준이다: `상태 = 상태 × 0x343FD + 0x269EC3`, 결과 `(상태 >> 16) & 0x7FFF`. CD판은 곱셈을 `lea`·`shl`로 풀어 써서 상수 `0x343FD`가 바이트로 나오지 않는다.
* 10.78의 `_getptd`는 스레드 자료가 없으면 새로 만들고 `+0x14`에 **1**을 넣는다(`004ea665`). 즉 **새 스레드의 `rand`는 시드 1에서 시작**하고 주 스레드의 `srand`에 영향받지 않는다.
* `srand` 호출 지점: 10.78 `00455981`·`00455990`(게임 난수 시드 함수 `00455970`), `004b4071`, `004bc96e`. CD판 `0048ccd1`(시드 함수 `0048ccb0`), `00430d46`, `004bbb84`.
* `rand`를 직접 부르는 곳(`E8` 직접 호출을 센 것)은 10.78 **26곳**, CD판 35곳뿐이다. 10.78: `00439cf0`, `0044ed22`, `004559a0`·`004559a8`·`004559af`·`004559b7`(시드 함수), `004600ad`·`004600d6`·`00460145`·`004601b4`·`004601d0`·`00460244`·`004602f4`·`00460423`(Flyingshrapnel.cpp 구간), `004b4079`·`004b4082`·`004b408c`·`004b4099`, `004b7eba`·`004b7f2d`(상태 기계 본체 안), `004bc986`·`004bc9a1`, `004c0556`(Terrainbuilder.cpp 구간 — [terrain-and-bridges.md](terrain-and-bridges.md)의 변형 표), `004df083`·`004df17f`, `004e3bc0`.

**게임 난수와의 관계(기존 문서 내용, 이번에 바이트로 다시 확인).** 게임 로직이 쓰는 난수는 CRT가 아니라 **전역 한 칸짜리 별도 생성기**다: `상태 × 0x10003 + 3`, 상태가 0이면 `0x0BAD0BAD` — 10.78 `004558c0`·`004558f0`·`00455930`(실수), 상태 `00532710` / CD판 `0048cbd0`·`0048cc10`·`0048cc50`, 상태 `005308ec`([bridge-pieces.md](bridge-pieces.md) 3절). CRT `rand`는 그 **시드를 만들 때** 쓴다: `00455970`이 `srand(0x38D535)`(전역 `DAT_005c89a8`이 0이 아니면 `004e4f1f(0)`의 반환값 — 기존 문서의 "시간 시드 경로") → `rand` 100번 버림 → `rand × rand × rand`를 게임 난수 상태에 넣는다.

**cpppj에서 쓰는 법.**

1. 게임 난수 상태는 **프로세스에 하나**다. 지금 `ClientAudio`가 `timeGetTime()`으로 따로 시드해 둔 난수([cpp-client-audio.md](cpp-client-audio.md))를 월드에 연결할 때는 곡 색인(`004558f0(0, 4000)`)과 월드 로직이 같은 상태를 이어 써야 한다(이미 LEFT_JOBS.md의 인계 항목이다).
2. CRT `rand`를 직접 부르는 26곳은 **부르는 스레드의 상태**를 쓴다. 주 스레드에서 도는 것끼리는 한 수열을 나눠 쓰고, 음악 작업 스레드가 `rand`를 부른다면 시드 1의 별도 수열이다. 복원할 때 전역 변수 하나로 합치면 순서가 달라진다.

<a id="runtime-observation"></a>
### 4.4 실행 중인 원본을 관찰하는 방식 (아이디어만 기록)

상대 프로젝트의 검증 방식은 우리와 다르다. 실행 중인 원본 프로세스에 작은 DLL을 넣고, 매 프레임 불리는 시각 고정 함수의 앞머리를 점프 명령으로 바꿔(`harness/hook/netstorm_hook.c`) **프레임마다 한 줄**을 이름 있는 파이프로 내보낸다: 프레임 번호, CRT 난수 상태, 마우스 좌표·버튼, **슬롯 풀 전체의 FNV-1a 해시**. 재구현을 같은 입력으로 돌려 프레임 단위로 비교한다. 프로세스를 건드리지 않고 `ReadProcessMemory`로 주기적으로 읽기만 하는 방식도 있다(`harness/record.py`).

그쪽 결과는 초보적이다 — 받아 둔 기록의 상태 해시는 전부 0이고, 난수 일치율 35.59%(1,298프레임 중 462)에서 멈췄다. 그리고 기록하는 난수가 CRT 상태라 게임 난수를 보지 못한다.

**우리에게 의미가 있는 부분.** 우리 검증은 ① 원본 함수를 떼어 격리 실행한 결과와의 대조, ② 화면·입력 자동화(analyzeManager)다. **실행 중인 원본의 내부 상태를 프레임 단위로 받는 수단은 없다.** cpppj가 미션을 진행할 수 있게 되면 "같은 조작을 했을 때 N프레임 뒤 월드가 같은가"를 볼 방법이 필요해지고, 4차 목표의 분석용 MCP와도 닿는다.

10.78에서 같은 일을 하려면 필요한 주소(이번에 확인했거나 기존 문서에 있는 것):

| 대상 | 10.78 | CD판 (상대 문서의 주소) |
|---|---|---|
| 프레임마다 한 번 불리는 함수 | `00460e90` | `004012d0` |
| 프레임 번호 | `0055b4a8` | `0050f248` |
| 이 프레임의 게임 시각 (double) | `0055b4d0` | — |
| **게임 난수 상태** | `00532710` | `005308ec` |
| 현재 상태 번호 | `005c87e8` | `00564d20` |
| `inFortMode` / `inBattleMode` | `00594fb8` / `00594fbc` | `00540a20` / `00540a1c` (미확인) |
| 슬롯 풀 초기화 | `004abb10` | `004aaad0` |
| 슬롯 풀 포인터 / 개수 | 찾지 않음 | `005395dc` / `005395f4` (미확인) |
| 슬롯 폭 | 50바이트 | 36바이트 |

주의할 점:

* **원본을 실행해야 한다.** AGENTS.md의 실행 확인 규칙이 그대로 적용된다. DLL을 넣는 방식은 원본 **파일**은 바꾸지 않지만 실행 중인 프로세스의 코드를 바꾸므로, 쓰려면 사용자에게 따로 확인받아야 한다. 읽기만 하는 방식이 덜 침습적이다.
* 게임 난수 상태는 평범한 전역이라 밖에서 바로 읽힌다(TLS를 따라갈 필요가 없다).
* 풀 전체 해시는 **같은 판본끼리만** 비교할 수 있다. 슬롯에는 가상 함수 표 주소가 들어 있고 cpppj는 그 값을 저장하지 않으므로([cpp-sid-reconstruction.md](cpp-sid-reconstruction.md)), cpppj와 맞추려면 필드별로 골라 비교해야 한다.
* 그쪽 점프 덮어쓰기는 함수 앞 8바이트를 고정으로 옮긴다. CD판 `004012d0`은 `sub esp, 8`(3바이트) + `mov eax, [mem]`(5바이트)이라 명령 경계가 맞지만 10.78 `00460e90`은 다시 확인해야 한다.

**이번에 만들지 않았다.** 싱글플레이 복원이 비교할 만한 단계에 이른 뒤, 사용자가 원하면 검토한다.

<a id="external-fps"></a>
### 4.5 그쪽이 잰 프레임 속도 (확인 불가, 참고)

`status_proiect.md` 4절: 메뉴에서 **1,070프레임 / 15초 ≈ 71.3fps**, 플레이 중 **1,298프레임 / 20초 ≈ 64.9fps**. 프레임 번호 전역이 루프마다 1씩 오르는 것을 센 값이다.

우리 [main-loop.md](main-loop.md) 4절은 실제 루프 주기를 **계산으로만** 추정했다 — 시계 눈금이 1ms면 약 71fps, 15.6ms면 약 64fps. 그쪽 두 값이 이 두 후보와 가깝다. 다만 CD/10.37 계열 빌드를 다른 PC에서 잰 한 번씩의 값이고, 기록 시간이 정확히 15·20초였는지와 당시 타이머 해상도를 알 수 없다. **두 후보가 실제로 나타날 수 있다는 방증으로만** 본다. 10.78 실측은 여전히 하지 않았다.

<a id="rejected"></a>
## 5. 채택하지 않는 것

다음 세션에서 이 저장소를 다시 보더라도 아래 내용은 가져오지 않는다.

| 상대 문서의 주장 | 실제 | 근거 |
|---|---|---|
| `sprite_name_mapping.md`의 그룹 번호 ↔ 이름 표 ("0 = ui_font 130프레임, 1 = dude 35, 2 = sunArcher 64, … 100 = daisExtraFrames 2") | **이름이 전부 한 칸 밀렸다.** 블록 0이 `dude`(130프레임)이고 블록 1이 `sunArcher`(35), … 블록 100이 `fenceShield`(2)다. "글꼴용 바깥 그룹"은 없다 | CD판 타입 이름 표 `0051c6f8`의 0번이 `dude`, 100번이 `fenceShield`, 101번이 NULL. `tools/shp.py info --orig originalCD`. 배정 방식(다음 `"1.10"` 블록을 차례로 배정, GIF 이름은 쓰지 않음)은 [shp.md](../formats/shp.md)와 같다 |
| `.fort` = 머리 + `0x42`부터 16×16칸 × 3바이트 "타일 격자"(`0x63` = 물) + 뒤따르는 레코드 | 처음부터 끝까지 **길이 접두 섹션**이다. `63 00 00`은 `Chaff` 섹션의 **빈 청크 레코드**(`'c'` + 오브젝트 수 0) 256개이고, "머리"는 `Subscriber`·`State`·`Mission`·`CoreData` 섹션이다 | [fort.md](../formats/fort.md) — 원본 463개 전부 끝까지 해석. 길이 필드가 자신을 포함한다는 것만 그쪽과 같다 |
| 게임 난수는 MSVC `rand`이고 기록으로 일치를 확인했다 | 게임 로직의 난수는 별도 생성기다(4.3절). 그쪽 기록에서 CRT 상태가 거의 변하지 않은 것은 플레이가 CRT를 거의 쓰지 않기 때문이다 | `004558c0`/CD `0048cbd0`의 바이트 |
| `_getptd` = `004f5410` | `004f6010` (`004f5410`은 다른 함수의 중간이다) | CD판 `rand` `004f23b0`의 첫 명령이 `call 0x4f6010` |
| `DAT_00564c90` = `pendState.state` | assert가 검사하는 칸은 `00564f68`이다. `00564c90`은 4.1절의 "다음 상태" | CD판 `0042d600` |
| 상태 값 추측: `0x2a` = S_FORT_CHECK_STATE, `0x27` = S_BATT_LOGIN_DENIED, `0x06` = S_FORT_RUNNING. 상태 약 37개 | `0x2a` = `S_GAME_STARTUP`, `0x27` = `S_GAME_INIT`, `0x06` = `S_FORT_META_INIT`. 43개 | 4.1절 |
| `0041ae80` = 게임 상태 기계, `0042d630` = 네트워크(UDP) 상태 기계 | `0041ae80`은 입력·명령 처리(Userinput), 상태 기계 본체가 `0042d630`이다 | [main-loop.md](main-loop.md) 5.1절, 4.1절 |
| 갱신 함수의 용도: `00433bf0` 음악 대기 타이머, `004333d0` 곡 선택(곡 번호 `0x1d`/`0x21`/`0xe`), `004336e0` 음악 전환, `004ecb30` 불법 복제 방지·시한 장치, `00449250` AI, `0043e280` 채팅 생존 신호, `00442d10` 지형 높이 지도, `004d7350` HTTP 서버, `00415b30` 물리·애니메이션, `004a9c30` 커서 확인 | 차례로 CD 확인, 네트워크 생존 신호(그 숫자는 상태 번호 `S_META_CHAL_CONNECTED`·`S_META_CHAL_CONNECTED_MONITOR`·`S_BATT_RUNNING`), 게임 흐름 상태 기계, `monitor` 로그, 다리 붕괴 스캔, 커서, 미니맵, AI 객체 해제, 가이저 생성, 설정 저장 | [main-loop.md](main-loop.md) 2절(정밀 디컴파일 기준. 이번에 다시 검증하지는 않았다) |
| 그리기 함수 `004aa190` 등 = 슬롯 배열을 도는 스프라이트 층 | 디버그 덧그리기 | [main-loop.md](main-loop.md) 6절 |
| 15Hz 고정 틱 (`CLAUDE.md`·`GEMINI.md`·`planul1.md`) | 고정 틱이 없다. 그쪽도 뒤에 71Hz로 고쳤다 | [main-loop.md](main-loop.md) 1절 |
| 지형 팔레트가 없다 (`windy.col` 등 네 파일이 배포본에 없어 색을 맞추지 못함) | 기본 팔레트는 `d/GIFCLOUD.COL`이다. 네 이름은 `ascendancyPalette` 설정이 켜졌을 때만 쓴다 | [shp.md](../formats/shp.md) 색상 절 |
| 슬롯 `+0x1f` = 체력 대신 쓰는 0~3 단계, 별도 체력 없음. 소유자 0~3 | 체력 쪽은 확인된 바 없는 추측이라 채택하지 않는다. 소유자 번호는 0 또는 1~8이다 | [cpp-owner-reconstruction.md](cpp-owner-reconstruction.md)의 범위 검사. 슬롯 필드는 cpppj raw 배치 문서를 따른다 |
| 길찾기 = A\*, 128×128 격자, 경로를 네트워크로 맞추는 방식 | 메모리 확보 크기와 문자열에서 짐작한 것이다. 확인하지 않았고 채택하지 않는다 | [movement-pathing.md](../gameplay/movement-pathing.md) |

## 6. 우리 분석과 일치한 것

서로 모르고 따로 분석한 결과가 같다. 새 내용은 없다.

* TAFF v0.2 아카이브와 XOR 키 `mydoghasfleas` — [taff.md](../formats/taff.md)
* SHP 프레임 머리 24바이트와 행 단위 RLE 네 가지 토큰(행 끝 / 건너뛰기 / 그대로 복사 / 반복), 픽셀 영역이 상자(xmin~xmax)라는 점 — [shp.md](../formats/shp.md). 그쪽은 CD판 그리기 함수 `00465b95`에서 읽었다.
* 타입을 고정 순서로 읽으며 다음 `"1.10"` 블록을 차례로 배정하고 GIF 이름은 실행 중에 쓰지 않는다 — 같은 문서
* 프레임 제한은 수직 동기가 아니라 바쁜 대기이고 간격이 약 14ms다 — [main-loop.md](main-loop.md) 4절
* 날씨 팔레트 이름 네 개의 포인터 표(CD `0051a1a0`)와 그것을 쓰는 CD `00436960` — [cpp-scene-music-reconstruction.md](cpp-scene-music-reconstruction.md)의 날씨 효과
* 슬롯 풀 초기화 CD `004aaad0`, 슬롯의 `+4` next·`+10` 타입·`+11` 상태 비트(free 1, void 4) — [cpp-sid-reconstruction.md](cpp-sid-reconstruction.md)
* 프레임 번호 전역 CD `0050f248` — [cpp-sound-process-reconstruction.md](cpp-sound-process-reconstruction.md)

## 7. 재현

저장소 루트에서 실행한다. 모두 exe 바이트만 읽는다(`pip install capstone` 필요).

```powershell
# 4.1 상태 이름 표와 표를 찾는 명령, pendState 설정 함수
python tools/exe_callscan.py --table 0x542548 43
python tools/exe_callscan.py --str 0x513478 0x51346c 0x5130ec
python tools/exe_callscan.py --dis 0x4b6dd0 0x4b6e60
python tools/exe_callscan.py --dis 0x4b2bc0 0x4b2bf5
python tools/exe_callscan.py --exe originalCD/NETSTORM.EXE --table 0x518968 43
python tools/exe_callscan.py --exe originalCD/NETSTORM.EXE --dis 0x42d600 0x42d6c4

# 4.2 갱신 목록
python tools/exe_callscan.py --dis 0x439a95 0x439b73
python tools/exe_callscan.py --exe originalCD/NETSTORM.EXE --dis 0x48671d 0x4867e5
python tools/exe_callscan.py --dis 0x435060 0x435066
python tools/exe_callscan.py --dis 0x4dd330 0x4dd335

# 4.3 CRT rand 와 게임 난수
python tools/exe_callscan.py --dis 0x4e649e 0x4e64cd
python tools/exe_callscan.py --dis 0x4ea61d 0x4ea66c
python tools/exe_callscan.py 0x4e649e 0x4e64ab
python tools/exe_callscan.py --dis 0x4558c0 0x4559c7
python tools/exe_callscan.py --exe originalCD/NETSTORM.EXE --dis 0x4f23a0 0x4f23df
python tools/exe_callscan.py --exe originalCD/NETSTORM.EXE 0x4f23a0 0x4f23b0

# 5절 스프라이트 이름 표
python tools/shp.py info --orig originalCD
```

`--table`은 포인터 값만 보여 주므로 이름은 `--str`로 읽는다.

## 8. 남은 일

* cpppj State.cpp 복원 때 4.1절의 이름과 세 구조체를 쓰고, 상태 전이 순서를 `004b6dd0`에서 읽는다.
* [main-loop.md](main-loop.md) 2절의 나머지 "같은 자리" 대응(굵지 않은 CD 주소)은 이번 대조로 **자리**는 확정됐다. 각 함수의 용도 설명은 정밀 디컴파일이 있는 PC에서 필요할 때 다시 본다.
* 10.78 갱신 목록의 마지막 조건부 호출 `004d3090`과 전역 `DAT_00594fc0`의 뜻.
* 4.4절의 관찰 도구를 만들지는 사용자 결정 사항이다.
