# 원본 월드 코어 W02 첫 조각: 슬롯 풀과 오브젝트 생성

2026-10-16, 작업 PC **DESKTOP-HJOW**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 첫 조각이다. `dotnetpj/src/Netstorm.Core/Original/`(이름공간 `Netstorm.Core.Original`)를 처음 만들고 오브젝트 슬롯 풀과 기본·파생 생성·Take를 옮겼다. 이 PC는 마지막 전체 정밀 디컴파일 PC(VM-W11-Codex, 2026-10-16)와 달라 **디컴파일 산출물을 읽지 않았다.** 근거는 커밋된 cpppj 소스·보고서와 원본 기계어 fixture다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없고 `cpppj/`·공통 `LEFT_JOBS.md`는 수정하지 않았다. 이 조각은 **아직 세션에 연결하지 않았다**(L1 모듈 단계). 게임 플레이는 달라지지 않는다.

## 1. 저장 방식 결정: 슬롯 바이트를 그대로 둔다

계획 3절 3번은 "raw 50바이트 슬롯을 옮기지 않고 의미만 옮긴다"였다. 이 조각에서 **원본 월드 코어의 저장소에 한해 이를 바꿨다.** 이유는 세 가지다.

1. **검증 단위가 슬롯 전체 바이트다.** 생성·Pop·Unpop·소유자·프레임 등 W02~W03의 fixture는 호출 뒤 슬롯 50바이트를 통째로 대조한다. 의미 필드로만 모델링하면 "지정하지 않은 필드는 보존한다" 같은 원본 동작을 시험용 어댑터가 따로 흉내 내야 한다. 기존 `X86SidTests`가 `payloads` 사전으로 그렇게 버티고 있다.
2. **필드 이름을 모르는 쓰기가 많다.** 파생 생성자 82개의 쓰기는 가상 표(+0)와 추가 비트(+40 OR 2), 섬 번호(+8), anim HP(+26)다. 클래스마다 나머지 배치가 다르고 cpppj도 오프셋으로 다룬다. 모든 모듈을 의미 이름으로 먼저 옮기려면 클래스별 배치를 전부 풀어야 해서 이식 순서가 막힌다.
3. **원본 동일성 기준은 결과다.** 인수인계 9절은 내부 표현을 "플랫폼과 내부 표현의 문제이며 화면·소리·규칙의 결과가 원본과 같아야 한다는 기준만 적용한다"고 했다. 슬롯 바이트 저장소는 그 기준을 더 쉽게 만족시킨다.

가상 표 주소(vtable)는 호스트 함수 포인터가 아니라 **클래스를 알아보는 4바이트 식별 값**으로만 저장하고 비교한다. 게임이 쓰는 모양(`GameEntity` 등)은 나중에 이 코어 위의 보기(view)로 만든다. 슬롯을 바이트로 둔다는 결정은 코어 안쪽에 한정되며 기존 세션 코드와 `SidPool`은 그대로다. 되돌리려면 이 폴더를 지우면 된다(세션 미연결). 사용자가 다른 방식을 원하면 인수인계 10절 11번을 바꾸면 된다.

## 2. 구성

| 파일 | 내용 | 원본 / cpppj 근거 |
| --- | --- | --- |
| `RawSidPool.cs` | 슬롯 풀. 슬롯 50바이트(번호 N은 N×50), 일반 클라이언트 5~14999·서버 15000~23000·예측 23001, 할당·반납·서버 재구성, 삭제 기록 20개. 0번 슬롯에 두 목록 머리(+0·+14)가 겹침. 최대 120,000슬롯 | `004abb10`/`004af1d0`/`004abf60`/`004abe00`, `SidPool.cpp`, [cpp-sid-reconstruction.md](exe/cpp-sid-reconstruction.md) |
| `RawSquidFactory.cs` | `Create`(할당→생성자→타입→postCreate), `Construct`, `Take`, `PostCreate`, `PostTake`, `FirstPopFlags`. `IRawUnpop` 연결 지점 | `004af530`/`004af610`/`004b0c60`/`004ad4a0`/`004ad470`, `SquidFactory.cpp`, [cpp-creation-reconstruction.md](exe/cpp-creation-reconstruction.md) |
| `OriginalConstructors.cs` + `.Data.cs` | 타입 번호 0~187의 생성자 주소 188개, 파생 생성자 82개(주소·최종 vtable·순서 있는 쓰기). `Data.cs`는 도구가 만든다 | `TypeConstructors.inc`, `DerivedConstructors.inc`, [cpp-derived-reconstruction.md](exe/cpp-derived-reconstruction.md) |
| `OriginalTypeRecord.cs` | 코어가 쓰는 타입 필드: 플래그1·2, 최대 HP, zOrder, 생성자 주소 | `RiftType.h`의 `RiftTypeRecord` |
| `tools/gen_original_constructors.py` | cpppj 목록에서 `Data.cs` 생성(`--check`는 일치 확인, 출처 SHA-256을 머리에 기록) | — |

10.78 슬롯 필드(바이트): 가상 표 +0(4) · 다음 빈 번호 +4(2) · 섬 번호 +8(2) · 타입 +10 · 상태 +11 · HP +26(4) · 소유자 +34 · zOrder +35 · 추가 비트 +40.

**CD 10.72는 옮기지 않았다.** 기준 판본이 10.78이다. CD는 슬롯 36바이트·HP 2바이트·영역 경계 6000/14000으로 달라, 필요해지면 풀 배치를 매개변수로 만들어 따로 낸다.

## 3. 검증 (원본 fixture의 10.78 행)

| 대상 | fixture | 일치한 것 |
| --- | --- | --- |
| `RawSidPool` | `sid-x86.tsv` 10.78 세션 8개·전이 **1,584** | 반환 번호, 빈 번호 카운터, 예측 커서, 두 목록 머리·꼬리, **풀 전체 바이트의 Adler-32**, 삭제 기록 체크섬 |
| 생성자 주소 표 | `constructors-x86.tsv` **188**행 | 모든 타입 번호의 주소(0이 아닌 것 122) |
| `RawSquidFactory` 기본 | `creation-x86.tsv` 세션 2·호출 **482** | 반환 번호, 풀 카운터·머리/꼬리·전체 체크섬, **대상 슬롯 50바이트 전부** |
| `RawSquidFactory` 파생 | `derived-x86.tsv` 세션 4·호출 **3,300**(Create 392·Construct 784·PostCreate 392·PostTake 392·Take 1,340) | 위와 같음. 지원하는 98개 타입 연결을 서버/클라이언트·mana 약화 옵션 조합으로 모두 실행 |

보호 경로(`RawSquidFactoryTests`)는 미복원·null·assert 생성자(153·159·169), 패치판 금지 타입 162, 자산 밖 번호, 서버 권한 없는 할당, non-void·타입 불일치·다른 클래스·contained 슬롯 Take가 **풀을 한 바이트도 바꾸기 전에** 거부되고, 정상 Take가 풀 카운터와 지정하지 않은 필드를 보존함을 확인한다. 생성자 표는 자산 타입 중 표에 없는 것이 정확히 153·159·169뿐이라는 점도 검사한다.

**변이 확인:** 구현의 핵심 11곳(HP 폭 4→2, OR 쓰기 무시, Take의 dead 비트 보존, postCreate의 소유자 0, mana 1/4→1/2, 최초 Pop 기본값, 162 금지 해제, 반납 시 슬롯 지움, 할당 시 슬롯 지움, 삭제 기록 이동, 생성자 표 값 변조)를 하나씩 망가뜨리자 모두 하나 이상의 검사가 실패했다. 원복한 뒤 전체 통과를 다시 확인했다.

## 4. 이번 조각에서 하지 않은 것 (경계)

- ~~**non-void Take의 공간 해제.**~~ 둘째 조각에서 `RawUnpop`을 구현해 연결했고 Take 384회를 원본 행으로 대조했다. [기록](dotnet-original-core-unpop-20261016.md)
- ~~**`FirstPopFlags`의 원본 기대값 대조.**~~ 같은 조각에서 `unpop-x86.tsv`의 `FirstPop` 448회로 대조했다.
- **생산 칸 contained Form(타입 6)과 GumpForm(타입 3)의 생성.** Form 모듈(W03)에서 옮긴다.
- **타입 로더 연결.** `OriginalTypeRecord`는 코어 검사용 얇은 모양이다. Assets의 `TypeCatalog`에서 이 레코드를 채우는 어댑터(최대 HP 속성, 플래그, 생성자 주소 표 조회)는 세션 연결 때 만든다.
- **세션 연결.** `BattleMap`·`BridgeGrid`는 여전히 기존 `SidPool`(압축 표현)을 쓴다. 코어가 세션에 연결될 때 기존 풀을 이 풀로 대체한다(두 풀의 할당 규칙은 같은 fixture로 각각 검증됨).

## 5. 다음 조각 (W02 안의 의존 순서)

~~`Unpop`(공간 해제, `unpop-x86.tsv`)~~ (둘째 조각에서 완료, [기록](dotnet-original-core-unpop-20261016.md)) → `Pop`/공간 해시·spot(`pop`·`spatial`, 기존 `SpotMap`·`SpotRules` 재사용 검토) → `PostPop`·`assetpostpop` → 소유자(`owner`)·프레임(`setframe`) → 건물 공간·이웃·탐색 → Graph 계열. 각 조각은 그 fixture의 10.78 행을 SHA 목록(`cpp_sync.py --update --add`)에 등록하며 시작한다.
