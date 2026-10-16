# 캠페인 1-1 실제 초기 자료와 실행 사슬 감사

2026-10-13, 현재 호스트와 마지막 정밀 디컴파일 PC는 **VM-W11-CODEX**다. `AGENTS.md`와 두 `LEFT_JOBS`를 다시 읽었다. 이번 작업은 원본 아카이브·기존 정밀 소스·현재 C++의 읽기 전용 분석이며 새 기계어 관찰/비교 수는 **0**이다. 게임/GUI·새 Ghidra 내보내기·보호 파일/공통 소스 편집·커밋은 하지 않았다. 앞선 미니맵 모듈/검사/근거도 변경하지 않았다. 전체화면 DirectDraw 수정은 범위 밖이다.

## 미션 식별과 번호 구분

실제 공식 캠페인 첫 미션은 보호 `originals/netstorm.tarc` 내부의 `\d\thewarbegins.english`와 `\d\thewarbegins.fort`다. 낱개 `originals/d/Beginning.English`나 기본 조작 튜토리얼 파일이 아니다. 현재 메뉴는 `offical3.Overview` → `MissionBegin,TheWarBegins` → `UberGump::BeginMission` → `MissionScript` → `FortTemplate::Parse` → `MissionPlayers::Load` → `GameWorld`로 들어간다. `.english`에 `loadFort`가 없으므로 미션 이름을 fort 이름으로 사용하는 현재 parser 규칙과 일치한다.

실제 header는 `missionType="Tutorial"`, `missionNumber=1`, `aid=1`, title `The War Begins!`, 내 시작 SP 3000과 시작 지식 `sunWalker;rainBattery;sunAviary;sunCannon`이다. 적은 구식 번호 없는 `aiName="The Jailer"` 설정으로 소유자2에 대응하며 시작 SP0, `sunWalker;windBattery;sunArcher`, 간헐천 부착1/콜렉터1/행동 간격6초와 두 금지 능력을 가진다. `Player.cpp`도 구식 `ai*`를 플레이어2 기본값으로 해석한다.

**`missionNumber=1`과 `tutorialNumber=1`은 다르다.** 이 파일에는 `tutorialNumber`가 없다. 원본 `004c3290`은 두 키를 각각 `005ca8e4`/`005ca8e0`에 읽으며 기본 문자열 `0050161c`는 `"0"`이다. `004c34c0`의 하드코딩 튜토리얼1~6(`004c3a20` 등) 분배 조건은 전자인 `tutorialNumber`다. 따라서 이번 캠페인에 기본 조작 튜토리얼1의 강제 단계 명령을 새로 연결하면 원본과 달라진다. 필요한 초기 스크립트 명령은 `[A.]`의 `ShowTechnology,55`와 `DoNothing,0`, 완료 후 `[Succeeded][BadTeamDead]` 및 `[Failed]`의 실제 사건이다.

## 초기 저장 객체: 172개와 별도 내용물/지식

Fort는 35섹션·파일2546바이트·마지막 미해석56바이트, `TypeNames`171개다. Chaff/Terr00/Terr01 모두 버전2이며 저장 청크는 각각256/6/3개다. Technology26개·Deck38개·Fort Money100000은 공간 객체와 별도 자료이고 미션 시작 SP에는 스크립트3000이 우선한다.

| 섹션 | 현재 타입 번호/이름 | 개수 | 저장 소유자와 프레임/특수 필드 |
| --- | --- | ---: | --- |
| Terr00 | 158 사제 | 1 | owner1, QA129, 빈 contents, **저장 frame 없음** |
| Terr00 | 122 간헐천 | 1 | owner0, 저장 frame 없음 |
| Terr00 | 124 레인 신전(`rainVortex`) | 1 | owner1, 저장 frame 없음, 내용물 타입133 항목1 |
| Terr00 | 89 거주 구역 | 1 | owner1, 저장 frame 없음, 로더의 실제 randframe 대상 |
| Terr01 | 158 사제 | 1 | owner2, QA2, 빈 contents, 저장 frame 없음 |
| Terr01 | 123 윈드 신전(`windVortex`) | 1 | owner2, 저장 frame 없음, 내용물 타입133 항목1 |
| Terr01 | 89 거주 구역 | 1 | owner2, 저장 frame 없음, 실제 randframe 대상 |
| Terr01 | 71 썬 디스크 발사기(`sunArcher`) | 9 | 모두 owner2, 저장 frame 없음 |
| Terr01 | 78 썬 바리케이드(`sunBlocker`) | 8 | 모두 owner2, 저장 frame 없음 |
| Terr01 | 75 윈드 제네레이터(`windBattery`) | 3 | 모두 owner2, 저장 frame 없음 |
| Chaff | 82 다리 | 15 | 약식 storedType253 → owner2, 저장 bridge frame: 35/3/41/35/35/3/41/23/41/41/41/29/41/13/35 |
| Chaff | 122 간헐천 | 13 | 모두 저장 owner0, 저장 frame 없음 |
| Chaff | 157 `noIsland` | 117 | 저장 owner0 또는2지만 실제 로더의 이 타입 강제 owner0 규칙 적용 |

합계는 **145+4+23=172**다. 두 신전의 타입133 `sunWalker` 항목은 `RawContainedCreate`가 **자산133을 공간에 Pop하는 대신 타입6 Form의 storedType WORD에133을 넣는 내용물/생산 항목**이다. 이를 살아 있는 골렘 두 마리로 초기 생성하면 잘못된 월드가 된다. 초기 사제·신전 외의 실제 골렘 생성은 생산/AI 흐름을 따라야 한다.

청크 안 상대 위치는 Terr00 사제 `chunk0 (14,12)`, 간헐천 `chunk1 (6,10)`, 신전 `chunk3 (9,4)`, 거주 구역 `chunk5 (5,14)`다. Terr01 사제 `chunk1 (0,11)`, 신전 `chunk1 (12,12)`, 거주 구역 `chunk2 (5,9)`다. Terr의 chunk 번호는 섹션 내 y/x 선택 순번이므로 그 자체를 월드 `(chunkX,chunkY)`로 사용하면 안 된다. 실제 위치는 저장 Territory의 `IslandBuilder`/현재 지역 지도에서 얻는다. Chaff의 chunk 번호는 전체16×16의 행 우선 번호다.

**훨리베이스72·썬 워크샾85·아웃포스트126·썬 캐논128은 초기 공간 저장에 각각0개다.** 훨리베이스는 시작 지식/지식 표시와 해당 건설 선택 이후 필요한 실제 타입이다. 초기 공간 이관을 위해 Factory85를 필수 새 자산으로 생성할 이유는 없다. 기존 자료 일부의 `sunAviary=85`, `sunWalker=137`은 번호 오류다. 현재 타입 표에서85는 `sunFactory`,72는 `sunAviary`,133은 `sunWalker`,137은 `windWalker`다.

## 원본 로딩 순서와 현재 차이

원본 저장 지도 경로 `004b5f70(0)`는 Territory 읽기 `004bd660` → 패턴/지역 배치 `0046dd70` → 지형 생성 `0046da70` → 객체 로드 `004be610` → 저장 Technology/Deck 등의 후처리 `004bf340/004be8e0/004beee0` 순이다. `0046da70`은 실제 지형 생성 `004c1510`과 지역별 간헐천 배치 `0040e560`을 호출한다. 간헐천 배치의 단일 시도 `0040db90`은 별도 몸체/연결 범위다. 이 정적 호출 사슬은 실제 전체 초기화 실행 대조를 대신하지 않는다.

이미 복원한 `RawFortObjectLoad::Load`는 Template 칸으로 기본 소유자를 구하고 같은 로딩 깊이를 공급하면서 **Terr00→Terr19→허용된 Chaff→finishOwner→깊이 감소**를 실행한다. 미션 정상 Template 칸1의 기본 소유자는1이고, 버전2/미션 분기에서는 저장 owner1/2를 유지한다. 간헐천·`noIsland` 등의 강제0 규칙과 다리 약식 소유자를 따로 적용한다.

각 객체는 factory `(type,0)` → 저장 QA/프레임 등 직접 필드 쓰기 → 가상 상태/소유자 → 실제 최대 HP/공통 HP 직접 저장 → 지형/랜덤 프레임 → **가상 Pop9 → contained 재귀** 순이다. 저장 frame이 없는 대부분의 초기 자산은 **생성자 frame0을 시작값으로 유지**하고 그 뒤 실제 후처리/프로세스가 수정한다. `.type defaultFrame`을 일괄 대입하지 않는다. 특히 두 사제는 저장 frame이 없으며 QA129/2와 실제 사제 소유자/로컬 표식 경로를 함께 보존한다.

현재 `GameWorld`는 `IslandBuilder`와 `TerrainBuilder`를 사용하지만 **Chaff를 먼저 값형 `objects_`에 넣고 Terr를 나중에 넣는다**. 저장 frame이 없으면 `.type defaultFrame`을 선택하고, 고정 CRT 시드/좌표 해시로 지형 그림 변형을 고른다. `BuildTerrain` 후 표면 seed만 `RawSurfaceWorld::Load`에 넘긴다. 이는 이미 복원한 실제 loader와 같은 실행 순서/전역 난수 소비가 아니다. 로더·지형·프레임·동적 간헐천 생성 효과를 같은 raw pool에 조립해야 하며 기존 `GroundGrid`/그림 tiles를 별도 진실 상태로 유지할 수 없다.

`RawFortFinishOwner`의 전체 거주 구역 목록 검사/후처리는 이미 복원돼 있다. 이 파일에는 owner1/2 거주 구역과 두 신전이 있으므로 실제 같은 목록을 등록했다면 기본 owner1의 **거주 구역 존재 자연 반환 분기**가 가능하다. 이 단위를 불필요한 새 fallback 거주 구역 생성으로 바꾸지 않는다. 단일 랜덤 배치가 모든 미션에서 제거됐다는 뜻은 아니며, 위 지형 간헐천 생성과 추가 배치에는 별도 도달성이 있다.

## 최초 예약: 바로 필요한 실제 몸체

원본 Regular 생성자 `00496e80`은 float 인자를 **payload +0x1c**에 넣고 double 예약 시각 `+0x10`은 **현재 게임 시각**으로 설정한다. 첫 호출은 다음 Kernel 프레임이며 부모 `vtable+0x5c Handle(event,count,payload)`의 양수 반환이 그때부터 다음 지연/payload가 된다. 따라서 아래0.08/0.1/8을 최초 대기 시간으로 그대로 사용하는 것은 오류다. `SquidProcessHost::AddRegular/RunRegular`는 이 생성/예약 몸체를 이미 복원했다.

| 실제 초기 부모 | 실제 postPop/최초 요청 | 필요 Handle/현재 복원 상태 |
| --- | --- | --- |
| 사제158 ×2 | `004950f0`, 목록/중복 검사 뒤 event `0x25a`; 실제 HP에서 회복 payload 계산 | `00494580`: `RawPriestRegen`·HP/상태·낙하/보호막과 `RawPriestWorldBindings`에 이미 복원. 동일 Host/목록/HP/게임 난수로 연결할 일이며 새 미복원 본체로 세지 않음 |
| 거주 구역89 ×2 | `0049a2b0`, surface owner→목록→event0/1/3 payload0→알림/잔여 frame | **`0049a4e0` Handle은 별도 필수**. event0의 주민/frame/random 주기, event1의 현재 주변 장식 finder, event3의 지면 owner/신전 테마 frame/0.1 반환. 현재 postPop만으로 첫 프레임 동작을 대신할 수 없음 |
| 간헐천122 ×14 및 지형이 추가 생성하는 간헐천 | `004852e0`, cost2000→raw 결정량 WORD→event0 payload0.1→받침 frame8 | **`00485640` Handle 필수**. display 전/후 갱신, 현재 결정량/type.cost 비율로 실제 frame/주기 결정, 마지막 Damageable Handle 연결 |
| 레인/윈드 신전124/123 ×1씩 | `004dce40/004dd020` → 공통 `004dc700` → event1 payload0.08; 로딩 중 생산 slot 자동 생성 분기는 억제, 이후 저장 내용물 Form 생성 | **`004dc220` Handle 필수**. 최초 event1은 실제 frame advance `004afc90`; 이후 생산/신전 사건의 공통 분배를 사용. postPop의 현재 player 신전/섬 등록/소유자/소리·slot/생산 경계도 같은 상태에 연결 |
| 썬 디스크 발사기71 ×9 | `0046a810`, 기존 애니메이션 제거→`0046a1a0` GunProcess 생성/flags0 부착 | 최초 Gun은 **일반 Regular가 아닌 전용 Kernel Process**다. `RawGunProcess` 생성/상태 기계/송신과 현재 `RawGunTarget` Archer finder는 복원. 실제 Gun animation/투사체/피해·sound/삭제 연결은 여전히 필요 |
| 윈드 제네레이터75 ×3 | `0041c150`→Damageable/공통; 별도 최초 Regular 없음 | 발전/에너지 공급 효과는 Mana/생산/건설 흐름에서 필요. 임의 Regular를 추가하지 않음 |
| 썬 바리케이드78 ×8 | `0048eda0`는 최초·전투 **및 loadingDepth0**에서만 startBlocker | 저장 로딩은 depth>0이므로 이 시작 분기를 억제한다. 이후 새 건설/비로딩 경로와 현재 Damageable 연결까지 완성된 것은 아님 |

가상 표는 실제 PE에서 직접 읽었다: 사제 `0050f210/+5c=00494580`, 거주 구역 `0050f540/+5c=0049a4e0`, 간헐천 `0050da88/+5c=00485640`, 레인/윈드 신전 `005196f0/00519860`의 `+5c=004dc220`이다. 클래스 이름을 기준으로 공통 payload 반환을 넣는 방식은 이 첫 프레임 효과를 생략한다.

썬 워크샾85는 초기에는 없지만 실제 `00509148/+20=004538f0`의 예약은 event2 payload8 및 패치 event5 payload0.5다. **`+5c=004549c0`의 상태 frame/업그레이드 Handle**과 연결해야 동작한다. Factory postPop 자체는 `RawFactoryPostPop`에 이미 복원됐다. 이것을 초기 사제/신전 Regular의 대용으로 쓰지 않는다.

훨리베이스72는 Factory85와 다른 `00509930/+20=0045ec50`이다. 원본은 server/최초 조건에서 **전용 sunAviaryProcess** 중복 검사를 하고 new0x30/`0045d690` → 내장 Process+0x20 → `0041bd20(..., flags8)`로 붙인 뒤 Damageable로 이어진다. 일반 Regular/Factory 후처리로 대신할 수 없다. 이 몸체는 초기 공간 저장에서는 도달하지 않지만 **시작 지식에 있는 훨리베이스 건설을 선택하는 실제 1-1 플레이**에서 필수 후속이다.

## 실제 이관 우선순위와 남은 최소 전체 사슬

1. **초기 동일 월드:** `RawSurfaceWorld::Core/BeginLoad/FinishLoad/Create/Pop/BindRuntime`의 실제 pool/hash/spot/types/frames/Graph/장부/Owner/Kernel을 사용해 현재 `GameWorld::objects_`의 비표면 객체를 이관한다. `RawFortObjectLoad`, 실제 지형/간헐천 생성 순서, `RawFortFinishOwner`, `RawContainedCreate`/Form 분배기를 조립한다. 선택·그림·체력·내용물도 같은 SID를 읽어야 한다. 사제/일반 자산 postPop/삭제를 미리 연결하고 depth와 inBattle/boss/미션 전역을 실제 단계에 공급한다.
2. **첫 Kernel 프레임:** 사제는 기존 `RawPriestWorldBindings`와 회복/보호막/낙하·삭제를 재사용하고, 거주 구역 `0049a4e0`·간헐천 `00485640`·신전 `004dc220`의 현재 필요한 Handle를 복원/등록한다. 적 썬 디스크 발사기의 실제 GunProcess와 Archer target를 연결한다. payload fallback으로 미연결 자산을 정상 처리하지 않는다.
3. **브리핑과 실제 생산/명령:** `[A.]`의 지식55는 저장 Technology26에서 골렘을 제외한25개 카드와 같은 자료를 사용한다. 시작 지식4와 혼합하지 않는다. 사제 Construct/신전 골렘 생산/Deck38의 실제 시간·내용물/기술/SP·커서/배치 입력을 연결한다. `GameWorld::MoveSelected`의 임시 GroundGrid 이동을 실제 Path 명령으로 교체한다. `RawPathSearch/RawPathRuntime/RawPathProcess`와 최신 `RawPathData`는 기존 몸체지만 **00489080 시작 분배와 타입54 사제/40 골렘/23 운반 프로세스의 가상 도착·명령 행동**이 모두 연결됐다는 뜻은 아니다.
4. **첫 썬 캐논의 전체 건설:** 기존 CursorDrop→Local→Request/Confirm/Notice→Place→Builder/Arrival→ConstructionProcess/Visual/Sound/삭제를 조립한다. 비용400, 실제 needsBuilder 플래그와 teleportEffect134/manaBolt105·공급원/에너지 소비→Path23 도착→서버 SID에 duration0.6초의 사슬을 유지한다. 기존 [건설 감사](cpp-construction-runtime-audit.md)와 [도착 몸체](cpp-construction-arrival.md)를 따른다. 시작 지식의 레인 제네레이터·훨리베이스도 선택 가능한 실제 기능이므로 1-1 전체 동작 동일성 범위에서 제외할 수 없다.
5. **전투/경제/AI/승리까지:** 새/기존 Gun의 animation→발사/투사체→피해/파괴·sound, 간헐천 자원/골렘 수확/운반, 적 AI의 실제6초 행동/간헐천 부착/콜렉터와 제한 능력을 연결한다. 신전 파괴→적 사제 보호막/포획→골렘 운반→제단의 다섯 룬 의식→사제 사망을 거쳐 기존 `RawMissionOutcome::Check`와 `[BadTeamDead]` 성공 사건을 실제 목록/상태에 연결한다. **신전 파괴 즉시 승리는 원본 1-1이 아니다.** 성공 inline Config/정지/`Done{fileName}` 저장/다음 미션 버튼도 종단 사슬이다.

기존 `RawRegionOwnership`의 `00455380` 투표/테마 선택/도장 요청은 **이미 복원**됐다. 실제 flood/지형 frame 도장 `00470900`과 현재 섬/표면 owner 변경 효과, Damageable 로딩 claim `0044bd20` 등 외부 효과의 조립은 별도다. 도장을 연결할 때 existing outposts/temples 목록을 그대로 공유하고 투표 본체를 다시 구현하거나 신규 관찰로 합산하지 않는다. 본섬의 초기 생성·소유자 도장과 신전 파괴 후 지형 전환의 같은 raw 표면/Frame/Display 연결은 실제 원본 동일성에 필요한 작업이다.

## 재현 자료와 증거의 한계

읽기 전용 C++ 명령 `NetstormCpp.exe --read-game originals d/thewarbegins.english`와 `--inspect-fort originals thewarbegins`, 기존 `tools/fort.py`/`shp.py`/`taff.py`를 사용했다. parser 출력은 실제 게임 플레이 관찰이 아니다. 새 출력은 `extracted/campaign11-fort-audit.txt`, `campaign11-fort-objects-audit.txt`이며 기존 정밀 원본은 `extracted/refined/originals/Netstorm.c`, `extracted/fortobjectload/`, `assetpostpop/`, `factorypostpop/`에 있다. Regular/State 함수 추출 파일은 콘솔 인코딩 영향이 있어 근거 주소의 원본 UTF-8 정밀 파일을 최종 출처로 사용한다.

- `netstorm.tarc` SHA256: `f2cafc1a0242ffe59b9b2fbfd6555907a03585cd6053d4bb4b0599a2445ea2e6`.
- `\d\thewarbegins.fort` SHA256: `e7c97f70ae206cafa302882606617e5bdb21722080c4b7f79adee376f0f5df75`.
- `\d\thewarbegins.english` SHA256: `db4fb183bc12c3b3f2f20e7a6c1ffb9d9ed0aac5401e6001372464c897ae9b96`.

현재 본체 재사용 판단의 주된 소스는 `RawFortObjectLoad.cpp`, `RawFortFinishOwner.cpp`, `RawAssetPostPop.cpp`, `RawFactoryPostPop.cpp`, `RawPriestWorldBindings.cpp`, `RawPriestRegen.cpp`, `RawGunProcess.cpp`, `RawGunTarget.cpp`, `RawPathData.cpp`, `RawRegionOwnership.cpp`, `RawMissionOutcome.cpp`다. 실제 원본 플레이의 포획/의식/성공과 지형 변화는 기존 [1-1 녹화 판독](../videos/the-war-begins-record-play-20260930.md)을 참고한다. 과거 감사에서 '모듈 없음'으로 적은 GunProcess/Path/승패 등의 목록은 현재 복원 상태로 갱신해 읽어야 한다. 이번 문서는 새 연결·C++ 동작/GUI/종단 완주를 검증했다는 보고가 아니다.
