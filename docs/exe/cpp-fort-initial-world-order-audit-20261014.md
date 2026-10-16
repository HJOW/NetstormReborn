# Fort 초기 로드와 같은 raw 월드 조립 순서 감사

2026-10-14, 최신 AGENTS/두 LEFT 재독 후 **현재/마지막 디컴파일 PC HJOW-Athlon**의 정밀 C와 현재 C++를 읽었다. 기준 단위09 **5b10ce8/전체1010검사**와 준비된 단위10 일반 지형 소비자 소스는 동결했다. 이 조사는 새 원본 실행/검사/게임·GUI/기존 파일 변경이 각각0이다.

## 저장 월드의 실제 부모

패치 **004b5f70**와 CD **004330c0**의 현재 실제 호출을 직접 확인했다. `param1==0` 정상 저장 지도 경로는 다음 순서다.

| 순서 | 원본 역할/문자열 | 10.78 | CD 10.72 |
|---:|---|---:|---:|
| 1 | builder 초기화 | `0046d9b0/0046da20` | `00490430/004904d0` |
| 2 | 현재 Template의 Territory120BYTE | `004bd660` | `00427c00` |
| 3 | 활성 Territory 레코드 PlaceAll | `0046dd70` | `00490980` |
| 4 | 지형 성장·도장·edgeFarm postpass | `0046da70` | `00490530` |
| 5 | Terr[0,20)→허용된 Chaff→finishOwner | `004be610` | `00428d50` |
| 6 | 실제 문자열 **Money** | `004bf340` | `004297f0` |
| 7 | 실제 문자열 **Badges** | `004be8e0` | `00428fc0` |
| 8 | 실제 문자열 **Technology** | `004beee0` | `004293d0` |
| 9 | 현재 표면 Graph 재등록/전체 순회 | `00463110(0)` | `0045bd60(0)` |

과거13일 캠페인 문서의 Technology/Deck 등의 포괄적 서술과 다르게, 위 세 후처리는 현재 몸체에 있는 **Money→Badges→Technology** 문자열로 확인됐다. Deck가 이 세 직접 호출에 있다는 근거는 없다. 과거 문서를 당시에 확보한 출처로 보존하며 이번 감사에서 명칭/순서를 정정한다.

Money/Technology는 각각 실제 loadingDepth를 증가/감소하며 Technology는 플레이어 기술 내용물을 flags`0x11`로 생성한다. 마지막 호출은 표면 flags`0x800`/현재 graphBYTE/spot0x8을 읽고 실제 Graph 레코드와 객체를 등록한다. 단순 UI 새로고침이나 Player 초기화 호출로 해석하지 않는다. 기존 `RawSurfaceWorld::FinishLoad`의 `RawGraph::Rebuild(true)`가 부모의 인자0 몸체 전체와 같다는 독립 관찰은 아직 없다.

## 실제 IslandBuilder가 먼저 필요하다

새 일반 지형 소비자는 실제 `ChunkMap`과3204BYTE `RawIslandRuntime`이 이미 채워져 있을 때 성장/도장/군집을 실행한다. 현재 `IslandBuilder`는 `vector<IslandInfo>`의 일반 owner/Terr만 추가하며 `RawIslandRuntime`의 Add/Bounds와 inWorld 조건을 실행하지 않는다. 이 vector를 나중에 복사해 실제 목록으로 만들면 부모가 기록한 currentOwnerDWORD/originalOwnerBYTE/Exists/Bounds 순서를 대신하게 된다.

실제 **0046dba0/CD00490750**은 저장 레코드에서 위치 nibble을 읽고, `inWorld`일 때만 실제 `RawIslandRuntime::Add(owner,territory)`의 반환번호로 지역을 선택한다. `CanonDecoder`의 유효 칸마다 현재 `ChunkMap::At`에 signed 지역BYTE/방향/record6BYTE/ordinalDWORD를 쓴다. 마지막 inWorld 분기에서 실제 Bounds→16배 float→Runtime.Bounds를 기록한다. map가 world가 아닐 때 Add/Bounds를 생략하고 전달한 region을 유지한다.

PlaceAll은 **0046dd70/CD00490980**의 `[start,end)` 순회이며 recordBYTE+1의 bit0이 있고 bit1이 없을 때만 Place를 호출한다. 실제Place의 방향은 `record[0] >> 6` 그대로다. 패턴Count `004bfde0/CD0040b9d0`의 방향은 이 값×2이므로 두 helper를 같은 변환으로 통일하면 원본과 달라진다. CD constructor의 홀수 방향/assert와 실제 청크 끝 clamp는 후속 독립 PE 관찰에서 별도로 분류해야 한다.

다음 단위11은 새 `RawIslandBuilder` 몸체/검사/oracle/fixture로 이 경계를 복원한다. 기존 mutable Runtime의 Add/Bounds와 ChunkMap getter/Bounds/CanonDecoder를 재사용하며 기존 완료 관찰을 다시 신규로 더하지 않는다.

## 저장 객체의 같은 풀 연결

기존 `RawFortObjectLoad`의 실제 body는 Factory(type,0)→저장 QA/frame 등→가상 state/owner→HasHP/MaxHP/공통 직접HP→지형/게임random frame→**Pop9→contained 재귀** 순서다. Terr를 같은 current Runtime.Matches/ChunkMap으로 선택한 뒤 Chaff를 읽는다. Template칸1의 미션 저장owner1/2와 간헐천/noIsland의 강제0을 구분한다.

필수 연결은 같은 `RawSurfaceWorld.Core`의 Factory/HP/표면 알림, 사제 포함 실제 state/owner 분배, 실제 Pop/Destroy, `RawContainedCreate`의 타입6 Form 생성/실제 Attach, 현재3204BYTE Matches, 단일 게임GameRandom, `RawAssetPostPopState.residences` 자체를 쓰는 실제 FinishOwner다. 신전의 저장sunWalker133을 공간 자산으로 Pop하지 않고 contained 타입6 Form의 storedTypeWORD로 등록해야 한다.

로딩 깊이는 하나의 현재 원본 카운터에서 사제·Carrier·Asset·Factory·표면에 전달해야 한다. 현재 `RawSurfaceWorld.BeginLoad`는1을 지정하고 `RawFortObjectLoad.Load`는 자기 전역을 증가/감소하므로 호출자가 서로 다른 복제 카운터를 만들거나 원본보다 이중 증가시키지 않도록 부모의 실제 기본 깊이를 독립 관찰해야 한다. 후처리 Money/Technology까지 loadingDepth0이라고 가정해서는 안 된다.

## GameWorld 교체 순서와 남은 경계

현재 GameWorld constructor는 기존 vector IslandBuilder→TerrainBuilder→**Chaff 값형 객체→Terr 값형 객체**→고정CRT seed/103회 선소비/99cache→좌표 hash 지형 프레임→BuildSurfaces 순이다. BuildSurfaces는 저장 지면/noIsland/다리 seed만 정렬한 raw `Load`에 넣고, 비표면 객체와 이동을 `objects_`/`GroundGrid`에 유지한다. 이 경로는 위 실제 원본 부모의 실행 순서/난수/현재 SID와 다르다.

후속 교체는 다음 경계를 순서대로 닫아야 한다.

1. 실제 자산/SHP/타입으로 raw World를 만들고 **첫 지형 객체 전에** 같은 Pool의 Runtime/HP/사제/Asset/Factory/의식/Gun/Path/피해/승패 바인딩과 Host/Kernel을 설치한다.
2. 실제 초기 builder가 같은 ChunkMap/3204BYTE에 Territory를 놓는다. 정상/군도 분기 판정은 해당 원본 전역을 읽으며 군도가 미복원일 때는 명시 거부한다.
3. 완료된 일반 성장·초기 Stamp·edgeFarm 소비자를 실행한다. 이전 tiles/좌표hash 프레임으로 raw 상태를 다시 덮지 않는다.
4. 기존 실제 Fort 로더에 같은 Runtime.Matches와 모든 효과를 연결해 Terr→Chaff→FinishOwner를 실행한다. Pop9·현재 게임 난수·저장owner·contained/Regular 최초현재시각을 보존한다.
5. 실제 Money→Badges→Technology와 Graph 후처리를 연결한 뒤 정상 로드를 완료한다. 전체 부모가 미복원인데 빈 효과로 성공을 반환하지 않는다.
6. 렌더/선택/우클릭/명령/현재 이동은 같은 raw SID를 읽도록 GameWorld를 이관한다. 표시용 snapshot은 읽기 전용으로만 사용하고 다음 프레임의 진실 상태로 되돌려 쓰지 않는다.

현재1-1의 저장 공간 객체172개(Chaff145/Terr00 4/Terr01 23)와 기술/내용물은 과거 보호 자료 감사의 입력 계수다. 성장 지면/edgeFarm/생산 Form/프로세스 생성과 합산해 원본 초기 상태가 동일하다고 선언하지 않는다. 이 문서는 이관에 필요한 실제 순서와 요구 경계를 확인한 것이며 미션 완주나 GUI/실제 경제·AI·운반·승패를 완료한 결과가 아니다.
