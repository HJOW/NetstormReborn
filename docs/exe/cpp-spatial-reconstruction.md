# Squid 공간 등록·해제 복원

2026-10-06 작업. [SquidSpatial.h](../../cpppj/src/o/SquidSpatial.h)와 [SquidSpatial.cpp](../../cpppj/src/o/SquidSpatial.cpp)는 원본 Squid.cpp의 Pop/Unpop 중 **좌표·spot·4단계 객체 번호 체인·비전투 활성화 상태**를 옮긴 C++ 계층이다. 앞서 복원한 [SquidHash/EffectiveGenus](cpp-hash-reconstruction.md)를 사용하고, 등록한 지도를 [SurfaceFinder](cpp-surface-reconstruction.md)에 전달한다.

현재 GameWorld의 순차 런타임 번호/GroundGrid를 교체하지 않았다. 원본 SID 할당/세대·파생 생성자·가상 효과·실제 섬/받침 생성·배치 UI는 다음 연결 범위다. 이 작업으로 미션 건설이나 완주가 가능해진 것은 아니다.

## 원본 진입점과 입력

| 경로 | 패치 10.78 | CD 10.72 | 호출 규약 |
|---|---|---|---|
| Pop | `004b02d0` | `004ad490` | thiscall, `ret 12` |
| Unpop | `004afe50` | `004ad0b0` | thiscall, `ret 4` |
| 비전투 Activate | `004ac560` | `004abfb0` | thiscall, `ret 4` |
| SetIsland | `004acd20` | `004aca80` | thiscall, `ret 4` |

원본 객체 배열은 패치 **50바이트**, CD **36바이트**다. 공통으로 next `+4`, island `+8`, 타입 `+a`, 상태 `+b`, 표면 word `+c`, float 좌표 `+e/+12`, 화면 캐시 short `+16/+18`을 사용한다. 해시 단계는 패치 `+21`/CD `+1f`, 부가 바이트는 패치 `+28`/CD `+23`이다. 원본 frame 인덱스 저장 형태도 다르므로 새 C++ 구조체를 그대로 raw 풀에 복사하지 않는다.

`SquidSpatial::Add`는 **이미 할당된 void 객체 번호**를 입력받는 어댑터다. 상태 바이트의 free=1/dead=2/void=4/contained=8과 부가 바이트의 buried=8/firstPop=128을 구별한다. 입력 번호 범위는 기존 표면 탐색과 같은 1..32767이며, 원본 SID 생성/재사용/세대 형식을 복원한 것이 아니다. 파생 가상 함수 반환 및 영역 지지 여부는 `SpatialDependencies` 계약으로 받는다. 화면/표면 알림은 순서 있는 사건으로 반환한다.

## 보존한 갱신 순서

Pop은 유효한 void·비 contained 객체에서 시작한다. 잘못된 유한 좌표는 원본처럼 `(10,10)`으로 복구한다. 좌표 캐시는 `trunc(x*16+0.5)`와 `trunc(y*11+0.5)`이며 월드 칸 좌표가 아니다.

발자국 등록 루프는 **float 절삭 → 정수에 0.9999 더하기 → 절삭**을 거친다. 비음수 정수에서는 첫 절삭 결과와 같다. `EffectiveGenus`가 자체 경계를 정할 때의 **float 좌표+0.9999 → 절삭**과 다르다. 소수 좌표에서 둘을 같은 ceil/floor 식으로 합치면 원본 spot 결과가 달라진다.

타입 genus가 `0x50444200|0x400ff`와 겹치거나 flags1의 surface `0x800`이면 발자국을 y/x 순서로 처리한다. buried/contained에서는 spot 처리를 건너뛴다. 각 칸에 `EffectiveGenus`의 **low byte**를 OR한다. 건물군의 좌우 변 중간 칸은 0단계 머리의 표면 word를 갱신하고 영역 조회/표면 알림 사건을 기록한다. 영역 지지 시 `(word & 0xfffb)|2`, 지지하지 않으면 `word & 0xfff9`다.

그 후 현재 SHP 크기로 해시 단계를 정하고 **기준점 버킷**에 `object.next=oldHead`, `head=id`를 저장한다. 전체 발자국에 객체 번호를 채우지 않는다. 0단계의 섬은 잘못된 섬 번호 **127**을 저장하고 다리는 기존 값을 유지한다. 화면 갱신 사건은 flags `0x2000`에 따라 가상 offset `0x88`/`0x8c`로 나뉜다. 최초 등록에서만 `firstPop` 반환 비트를 OR하고 extra `0x80`을 설정한다. 비전투 Activate는 void를 지우고, flags에 `8`이 없으면 `0x10`을 추가하여 postPop에 전달한다.

Unpop은 free/void 객체에서는 아무것도 바꾸지 않는다. dead=2는 해제를 막지 않는다. 활성 객체는 void 설정 → 같은 발자국에서 genus low byte를 AND 해제 → 좌우 변 중간 머리의 word에서 `6` 제거/알림 → **0~3단계**의 현재 좌표 버킷 체인 검색 → 머리 또는 이전 객체의 next 변경 → 화면 갱신 사건 순서다. 제거한 객체의 next·좌표 캐시·해시 단계는 그대로 남는다. 저장된 level만 검색하지 않는다.

새 보호 경로는 잘못된 ID/할당 상태·contained·NaN/무한대·지도 밖 발자국·손상된 순환 체인을 예외로 보고한다. 원본 assert 이후의 배열 밖 쓰기는 복원하지 않는다. 콜백 예외의 트랜잭션 복구는 제공하지 않으므로 호출자는 영역/firstPop 반환 계약을 준비한 뒤 등록해야 한다.

## 패치판과 CD판의 점유 충돌 차이

패치판은 각 칸에서 `(oldSpot & effectiveGenus)!=0`이면 **즉시 반환**한다. 앞서 쓴 spot/좌표/표면 변경을 되돌리지 않고, 체인 삽입/firstPop/Activate도 진행하지 않는다. `SpatialResult::Overlap`이 이 경로다. CD판에는 이 조기 반환이 없으며 OR와 등록을 계속한다.

두 판본의 차이를 각각 `SpatialEdition::Patch1078`/`CD1072`로 보존했다. 이것은 전체 배치 가능 여부 함수가 아니므로 건설 UI의 미리보기 검사에 사용하면 안 된다. 실제 배치는 `0049b510`↔CD `00445200`에서 복원한다. CD판에서 겹쳐 등록한 객체를 Unpop하면 다른 점유의 같은 bit도 지울 수 있으며 이 결과도 보존한다.

## 기계어 검증과 그 범위

[decomp_spatial_oracle.py](../../tools/decomp_spatial_oracle.py)는 원본 PE를 읽어 격리된 Unicorn 메모리에서 Pop/Unpop과 실제 타입·좌표·SHP·genus·해시·next·SetIsland·Activate 기계어를 실행한다. 입력 SID 풀/타입/SHP/지도는 합성 데이터다. 게임 진입점·OS API·원본/복사본/클론 GUI는 실행하지 않는다.

| 검사 | 결과 |
|---|---|
| 독립 입력 시퀀스 | **238개** |
| Pop / Unpop 전이 | **463 / 415, 합계 878회** |
| 두 판본의 결과가 다른 전이 | **32회**, 점유 충돌 시나리오 |
| x87 제어 워드 | **0x027f / 0x037f**, 전체 시퀀스를 각각 재실행 |
| 실제 assert 보고 호출 | **0 / 0** |
| 비교 출력 | 입력 객체의 공간 필드, 4단계 **86,272머리**, **65,536 spot** 전체, 사건 순서/인자 |
| 실행 제한 | 허용 함수 몸체·200,000명령·정상 EIP/ESP/ret N·x87 TOP/제어 워드·공간 필드 쓰기만 |

**의존성 대체를 포함하는 조건부 통합 검증**이다. 가상 `0x88/0x8c`는 사건 기록, `0x30`은 입력 firstPop flags 반환, `0x20`은 전달 flags 기록과 원본 curPostPop 감소 계약만 수행한다. 영역 조회 `0046f6e0`/CD `004bdd80`는 입력 bool을 반환하고, 표면 알림 `004214a0`/CD `00448c10`은 SID만 기록한다. 실제 영역/소유자·부착 프레임·화면/파생 효과는 실행하지 않았다. dirty 큐와 Battle 복제는 비활성이고 별도 grid 조회 지도는 비어 있다.

따라서 이 Pop/Unpop 쌍을 **전체 함수 검토 앵커에 추가하지 않았다**. 전체 대응은 기존 **2,496쌍·검토 24쌍**을 유지한다. 검증 기록 [recovery-spatial-evidence.json](../../cpppj/recovery-spatial-evidence.json)은 제한·대체 의존성과 바이너리/fixture/생성 도구 SHA-256을 포함한다. `--verify`는 기록과 입력 파일의 해시·행 수·판본 차이 수를 검사한다.

[SpatialTests.cpp](../../cpppj/tests/SpatialTests.cpp)의 6개 검사는 저장된 [spatial-x86.tsv](../../cpppj/tests/fixtures/spatial-x86.tsv)의 두 결과 열을 C++와 각각 비교하고, 실제 등록→표면 이웃 탐색/해제→스냅샷 수명·충돌의 부분 변경·범위 보호·firstPop 한 번 호출을 추가로 검사한다. Windows Release 빌드 경고·오류 0, **CTest 내부 93개 검사** 통과. 기존 31,500개와 합쳐 **32,378개 제한 x86 입력 사례**이며, 새 878개는 위 대체 조건을 포함한다. 수명 480개는 접두 구간, 해시 초기화 4개는 할당 없는 경로다. 게임 완성도나 전체 함수 의미 검증 수치로 세지 않는다.

```powershell
python tools/decomp_spatial_oracle.py
python tools/decomp_spatial_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

기대값 생성에는 기존 Python/Unicorn/PE 분석 환경이 필요하다. C++ 빌드/CTest는 저장된 fixture만 읽으므로 원본/분석 환경이 필요 없다. `SquidSpatial`은 새 보조 파일이며 SOURCE_MAP의 원본 assert 파일 존재 수 **29/136**은 변하지 않는다.

## 다음 순서와 완료 기준

1. 원본 SID 할당/반납·세대/free list·형식/포함 객체·파생 생성자를 복원한다. 새 런타임 번호를 원본 SID로 암묵 변환하지 않는다.
2. firstPop/postPop·영역 소속/소유자·dirty/grid·표면 부착 프레임/변경 알림을 복원한다. 사건 목록을 실제 효과로 소비하고 Battle 복제의 로컬 상태 분기를 확인한다.
3. 섬/지면/noIsland/받침의 실제 생성·등록을 GameWorld에 연결한다. SurfaceFinder에 실제 0단계/spot 스냅샷을 제공하고, 이동 중 소수 좌표·일반 공간 검색을 복원한다. 지금의 정수 스냅샷 어댑터만으로 전체 조회가 완성된 것은 아니다.
4. `0049b510` 다리 배치·소유자 전파·Construction → 전체 수명/붕괴/삭제 → 다리 생산 칸/커서·배치 UI → 사제 Construct·건설/경제/전투/AI/승패 순서로 진행한다. [실제 플레이 계획](../cpp-playable-plan.md)과 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 이어서 기록한다.
