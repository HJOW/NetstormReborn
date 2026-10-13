# 초기 TerrainBuilder 프레임·stamp 주소 감사

2026-10-13, 현재 호스트와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 최신 AGENTS.md의 **CD10.72 복원에 필요한 주소/차이를 기록**하는 지침을 다시 읽었다. 이 감사는 현 호스트 정밀 C/함수 범위와 보호 두 PE의 읽기 전용 기계어 판독이다. **새 실행 관찰/단위/GUI 수는0**이며 TerrainStamp 구현·공통 파일 편집은 수행하지 않았다.

## 실제 몸체 대응

| 역할 | 패치10.78 | CD10.72 | 근거/차이 |
| --- | --- | --- | --- |
| 초기 지형 프레임 선택 | `004c04b0` | **`0049cf20`** | 현재 지면 문자/지역·직선/대각선 이웃→99개 CRTcache→AA/J블록 또는 실제 code검색 |
| 기본 지면 슬롯 생성/stamp 루프 | `004c0640` | **`0049d2c0`** | 반열린 bounds의 Y외부/X내부 순회→code alias 필터→프레임 선택→실제 factory/owner/frame/Pop→region·fringe |
| 선택 프레임으로 객체 하나 생성 | `004bfe50` | `0049d2c0` 내부 | 패치는 분리된 context+20808 함수, CD는 루프 안에 인라인 |
| 직선 이웃 mask | `004c00f0`→`004c0090` | `0049cf20` 안/`0049edb0` | 같은 code alias와 같은 지역BYTE·bounds 포함 검사 |
| 대각선 이웃 mask | `004c01e0`→`004c0090` | `0049cf20` 안/`0049ed70`·`0049edb0` | 두 판본의 실제 이웃 표/비트→문자 표를 사용한다 |
| 전체 saved 지형 생성 | `004c1510` | `0049e620` | 기본 경로 지면 성장→경계 표기→stamp→후속 처리 |
| 기본 경계/stamp wrapper | `004c09a0`→`004c0640`→`004c0ad0` | `0049e4c0` | CD는 `0049d890(x,'=')`→`0049d2c0(x)`→`0049d9a0`을 직접 묶는다 |
| 일반 code frame 검색 | `0049aa90` | `004444c0` | 패치는 theme 인자가 하나 더 있고 초기 지면은0, CD는 전체 후보 난수 선택 |
| 99 cache 최초 초기화 | `_rand=004e64ab` | `_rand=004f23b0` | **CRT 난수**99회를 연속 소비한다. 별도 GameRandom과 다르다 |

`extracted/refined/match.tsv`의 **`004c0640 → CD0049d690`은 잘못된 자동 대응**이다. 실제 CD0049d690은 PRNG/반경/3×3 성장과 재시도1000회로 패치004c0800에 대응하는 역할이다. CD0049e620의 기본 가지는 `0049e4c0`을 호출하고 그 wrapper는 실제 `0049d2c0`을 호출한다. 이후 `0049d2c0`이 프레임 helper0049cf20을 호출하는 PE주소`0049d381`을 확인했다. 자동 대응 점수와 decompiler 인자 개수를 그대로 구현 근거로 쓰면 안 된다.

정밀 내보내기 요청은 `tools/ghidra/terrainstamp-functions.json`의 패치14/CD13이다. 별도의 PE정적 범위6개는 `extracted/terrainstamp-audit/{patch,cd}-disassembly.txt`에 원본 SHA와 함께 기록했다. 두 판본 C에서 프레임/좌표 레지스터가 섞인 부분은 기계어가 기준이다.

## cache와 두 난수 저장소

초기 stamp cache는 패치 전역포인터`005c8be4`, CD`00537190`이다. 최초 포인터0에서396바이트(99DWORD)를 확보하고 **현재 CRT 난수에서99개**를 차례로 읽는다. 바로 앞 항목과 signedmod4가 같으면 현재 값을1 증가시킨다. cache는 전역 수명이므로 새 월드마다 재생성하거나 seed0를 매 항목에 주입하지 않는다.

cache 초기화 앞의`004437f0`/CD`0048ef00`은 실제 빈 함수이며 시드 설정 함수가 아니다. `00455970`은 별도 경로에서 고정`0x38d535` 또는 현재 초로 CRTseed를 설정하고100회+3회 CRT를 소비하여 세 값의 곱을 **별도 게임 난수 전역00532710**에 넣는다. 따라서 GameWorld의 고정seed→103회→99 cache는 제한된 재현 입력이며, 실제 시드 호출 전후의 다른 CRT 소비·전역cache 재사용까지 닫았다는 근거가 아니다.

다른 code 검색 및 fringe 선택은 실제`GameRandom`을 소비한다. 기존 `RawRegionPaint::Choose`의 원본검색/정규화/방향 fallback을 재사용할 수 있으며 **초기 stamp의99 CRTcache는 RegionPaint의99 cache와 공유하면 안 된다.** 두 함수의 cache 생성 몸체와 전역주소가 다르다.

## 프레임과 루프의 순서

지면/지역BYTE는 원본 메모리에서 **`x*256+y`** 순서다. 패치 offset는 code`+8`, 지역`+0x10008`, 타입 mapping`+0x20008`, code alias`+0x20408`이다. CD는 앞의 네 offset가 각각4바이트 작다. 패치 bounds는`+0x20820/+24/+28/+2c`, CD는`+0x2081c/+20/+24/+28`이다. 현재 `TerrainBuilder::Mask()`/`GameWorld`의 Y행 우선 자료를 그대로 rawmemory 인덱스로 대입하지 말고 실제 호출 X/Y와 어댑터의 전치를 명시해야 한다.

두 판본 frame helper의 AA special은 code가`x`이고 직선/대각선 문자가A/A일 때다. 타입의 **J구간 첫 물리 프레임(`+0x154`)**을 사용하며 문자열`JJ00` 검색으로 대체하지 않는다. cache항은 `(x/3)%99`와`(y/3)%99`, 합계mod99의 cache와`y/3`의 합을 signedmod4로 고르고, 최종프레임은`J.first+1+(y%3+variant*3)*3+x%3`이다. **초기 helper 자체는 owner별theme*36을 더하지 않는다.** 소유자 변경/테마 도장은 이후 별도 RawRegionPaint 사슬이다.

AA가 아닌 지면은 패치 `0049aa90(type,side,variant,-1,0)`, CD `004444c0(type,side,variant,-1)`로 실제 검색한다. 패치는 초기 theme0의 quartile을 선택하지만 CD는 해당 판본의 전체 후보 선택이다. 실패−1은 helper가0으로 바꾼다. `GameWorld::TerrainFrame`의`x*31+y*17` 나머지로 후보를 고르는 방식과 난수 소비/프레임 결과가 다르다.

stamp 순회는 Y외부/X내부이며 대상code의 alias가 요청char`x`에 해당하고 mapping타입이 nonzero인 칸만 처리한다. 호출 때마다 `x/=/기타`의 일부 mapping을 현재 전역isle97/edge98로 다시 지정하는 동작도 보존해야 한다. 원본 객체 생성 순서는 **factory(type,makePredictable)→가상owner0→frameDWORD/BYTE 직접 쓰기→가상Pop(x,y,0x02000061)→islandgenus의region 지정→현재 frame codeflag1의fringe 처리**다.

패치 `004bfe98`의 DWORD프레임 저장은 frame인자이며 CD의`0049d3d8/0049d3dc`도 frame helper의 반환BYTE를 저장한다. CD C에서 좌표float의 하위BYTE를 frame으로 저장하는 것처럼 보인 문장은 잘못된 레지스터 추론이다.

islandgenus이면 저장 지역127의 현재 chunk조회 분기/원본 assert와 실제 regionsetter를 사용한다. frame codeflag1에서 fringe102를 실제 검색하며 context excludedFlags와 겹치면 **난수를 다시 소비하여 검색을 반복**한다. −1 또는 성공의 분기, makePredictable assert, 직접frame→Pop(x,y+4,같은flags) 순서다. 현재 GameWorld의 fringe 그림레이어/flags0x10 필터는 이 raw생성/반복 선택 사슬을 구현한 것이 아니다.

## 권장 복원 경계와 검사 계획

독립 `RawTerrainStamp`는 원본 code/지역/타입mapping/alias/bounds/context와 별도 CRTstate/전역99cache를 참조하고 `RawRegionPaint::Choose`의 실제 frame검색만 공유하는 형태가 맞는다. 같은 raw pool의 factory·owner·Pop·region setter를 필수효과로 연결하고 마지막에 동일 SID에 fringe도 생성한다. 기존 렌더 tiles나 GroundGrid를 또 다른 게임 상태로 만들면 현재 목표의 월드 이관이 완료되지 않는다.

독립 검사는 AA/다른code·첫/재사용cache·CRT0/임의state·두 이웃mask·지역 같음/다름·bounds접촉·빈mapping·실패−1·최대 후보·초기theme0/CD전체후보·fringe제외/retry·실제 생성순서와 게임/CRT난수의 각 소비를 두 보호PE에서 대조해야 한다. `004bfe50`/CDstamp의 인자와 frameBYTE는 실제 PE명령에서 확인하고, 현재 복원된 Shape/Pop/PostPop을 같은 SID에 연결한 C++검사를 별도로 수행한다. 이번 감사는 이 구현/검사가 완료됐다는 보고가 아니다.
