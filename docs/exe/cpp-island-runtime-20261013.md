# 실제 mutable IslandList 기록과 소유자 변경 복원

2026-10-13, 현재 호스트와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 새 작업 전에 최신 AGENTS.md를 다시 읽고 두 인수인계의 현재 기록을 확인했다. CD10.72 대응 주소와 판본 차이도 기록한다. 보호 원본/AGENTS.md/dotnetpj와 공통 CMake/LEFT_JOBS/CoreData는 이 독립 작업에서 수정하지 않았다.

## 실제 주소와 자료

| 몸체 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 전체 초기화/기록 초기화 | `0046edf0` 인라인 | `004bd450`→`004bd3f0` |
| 존재/범위 | `0046ee30` 인라인 | `004bd4c0`→`004bd480` |
| 현재 개수 | `0046ee50` | `004bd4f0` |
| 끝에 추가/기존번호 삭제 | `0046ee60`/`0046ee90` | `004bd500`/`004bd640` |
| 현재 소유자 변경 | `0046ef10` | `004bd670` |
| 경계 getter/setter | `0046f130`/`0046f190` | `004bd8a0`/`004bd900` |
| 원래owner/지역 선택 | `0046f200` | `004bd940` |
| 신전DWORD getter/setter | `0046f260`/`0046f2c0` | `004bd9c0`/`004bda40` |
| 현재owner getter | `0046f320` | `004bda70` |

CoreData가 장기 소유해야 하는 원본 저장소는 **countDWORD+100개×32바이트=3204BYTE**다. 각 기록은 `+0` 신전DWORD, `+4` 존재DWORD, `+8` 현재ownerDWORD, `+12` 지역BYTE, `+13` **원래ownerBYTE**, `+14/+15` padding, `+16..28` 네float 경계다. 원래owner와 현재owner를 하나로 합치지 않는다.

Reset/Delete는 앞의 세DWORD와 두BYTE, 네float의−10000비트만 쓰고 padding2BYTE는 보존한다. Delete는 count를 줄이지 않으며 기존번호를 다음Add에서 재사용하지 않는다. Add는 이전count를 EAX로 반환하고 count+1, 존재1/현재owner/원래owner/지역만 쓰며 신전과 경계/padding은 그대로 둔다. 두 판본 실제 기계어에서 Add 반환을 확인했다.

Exists는 true로 정규화하지 않고 현재 존재DWORD 자체를 반환한다. 논리범위는 signedcount 비교다. getter/setter 대부분의 존재 실패는 패치에서 assertion 뒤 조건부 작업/0반환이며 CD는 조용히 반환한다. 경계getter는 두 판본 모두 assertion 뒤에도 **물리기록의 네float를 그대로 읽는다.** 정상물리 범위 안의 논리부재도 독립 대조했다.

일반 Matches는 **signed 원래ownerBYTE**와 signed지역BYTE를 DWORD인자와 비교한다. special/editor 또는 chaff모드에서는 요청owner를 읽지 않고 nonzero현재owner와 signed지역만 비교한다. owner255/territory255와 요청−1의 부호확장까지 보존한다.

## 현재owner 변경의 실제 효과

존재하는 섬이고 `!fort || special/editor`인 경우에만 현재owner를 바꾼다. 공유revision DWORD를1 증가시키고 패치만 별도ownerChanged DWORD에1을 쓴다. 원래ownerBYTE는 유지한다. 그 뒤 **현재 경계의 snapshot**을 얻어 유효성을 진단하며, 진단 후에도 원본처럼 현재 레지던스목록을 순회한다.

목록은 매 반복마다 현재count/SID/type/좌표를 다시 읽으며 free/dead/void필터를 추가하지 않는다. 현재 레지던스타입89이고 경계 안이면 실제 원래좌표의 표면을 조회해 그 슬롯의ownerBYTE를 가상`+0x74`로 전달한다. 표면0도 임의 소유자로 대체하지 않고 실제SID0의BYTE를 읽는다. 이 조회는 기존 **RawPathSurfaceAt**의 실제0.9999f 보정/WORD해시0 조회를 재사용하며 완료된 ChunkMap/SetSupportOwner를 다시 구현하지 않는다.

마지막에는 newOwner가nonzero이고 그 Player의 현재첫지역이 원본의 **고정127**일 때만 islandId를 쓴다. callback이 첫지역을 이미5 등으로 바꾸면 덮어쓰지 않는다. 초기 owner기록/두DWORD와 레지던스전파가 끝난 후의 현재 Player값을 읽는다.

패치의 bounds검사는 네float의 `(0,worldExtent)`와 양 끝 포함순서를 검사한다. CD의 최소값 검사는 float절삭이 아니라 **raw floatDWORD의 signed양수 검사**이며 일부 top/bottom unordered조건을 수용한다. 현재owner/주어진경계를 바꾼 뒤 원본assert에서 계속 진행하는 NaN입력을 별도로 보존했다. CD의 후보 포함검사도 left/top unordered를 허용하고 right/bottom은 ordered 상한을 요구한다. decompiled C의 `(int)float` 표기와 일반 C++rectangle검사로 치환하면 판본별 진단/owner전파가 달라진다.

## 공유 API와 실제 소비자 경계

`RawIslandRuntime`는 CoreData의3204BYTE span, 같은 pool/hash와 실제AssetPostPop residences목록, 기존 **FortFinishOwnerState&**의 fort/special/chaff/Player첫지역/레지던스타입, 공유 **revision/ownerChanged DWORD참조**를 받는다. 필수 `setResidenceOwner`/assertion이 빠지면 생성 시 거부한다.

조회 API는 **uint32_t Count() const**, **uint32_t Exists(int32_t island) const**, **uint32_t Owner(int32_t island) const**이며 Temple/Matches/Bounds/Bytes/Pool도 제공한다. 배치용 IslandList의축약 vector나 Ascend의8바이트exists/owner span을 이 raw32바이트기록으로 cast하면 안 된다. 기존 CountForeignOwnedIslands의 완료순회를 중복 구현하지 않았으며 후속root가현재Count/Exists/Owner accessor로 데이터연결 lineage를 작성한다. FortFinishOwner.regionBounds/loader.Matches/현재owner·Temple getter의 실제 연결도 root의 소비자작업이다.

현재owner의 레지던스가상효과는 필수외부경계다. 기존 **SquidOwner::Set**의 같은 pool/장부/모드를 연결할 수 있으며 레지던스의 실제분배와 월드수명/표시결합은 별도 소비자검사에서 확인해야 한다. 독립 fixture검사의 경계는 실제BYTE효과/원본인자/호출순서와 callback뒤 현재자료변화를 보존하지만 전체파생owner/UI효과 완료로 세지 않는다.

## 독립 원본 실행과 현재 검사 상태

최종 고유입력은 **2018개**다. 패치는 **정상508/assert501**, CD는 **정상730/assert279**이며 합계 **정상1238/assert780**을 분리했다. 두 x87정밀도 정상복귀4036회/원본assert호출1560회는 반복횟수이며 고유입력에 더하지 않는다. 각 판본 입력은 Reset3/Count5/Exists18/OwnerGet18/TempleGet18/Delete18/GetBounds15/Add36/TempleSet24/SetBounds72/Match128/**OwnerSet654**=1009다.

100개 전체필드/padding, count0/1/3/100/−1, 논리부재/존재DWORD2, 원래owner/currentowner, 끝추가99→100, 경계float/NaN, fort/editor/chaff, revision−1→0, 현재목록SID/count·표면·모드·Player첫지역을 callback뒤 바꾼 입력을 포함한다. 전체3204 raw BYTE, 반환/출력경계, 두DWORD/Player첫지역, 가상효과순서, 현재residences/owner/mode를 실제폭으로 저장했다.

원본실행기는 현재`island-runtime` 정밀범위 **패치12/CD14**와 보호PE를 실행한다. patch의작은Point유효/표면helper 범위는 현호스트 전체정밀 metadata에서 읽고 SHA를 고정했다. 실제기록/owner/범위/표면WORD/Player쓰기는 native 명령이고 **가상레지던스owner와 assertion만 관찰경계**다. 양판본smoke17개씩과 전수2018개 생성, 현재SHA/고유입력/정상·진단분리/두정밀도복귀 감사가 통과했다. OS/게임/GUI진입은0이다.

자료는 `tools/decomp_islandruntime_oracle.py`, `tools/ghidra/island-runtime-functions.json`, `cpppj/islandruntime.json`, `cpppj/tests/fixtures/islandruntime-x86.tsv`, `extracted/islandruntime-oracle-final.log`다. 공통08의 `extracted/run8-IslandRuntime_-first.log`에서 **C++3 tests와 양판본2018개 전수대조(정상1238/assert780)** 및 missing/물리100기록경계 거부가 모두 통과했다. 08 전체검사/최종단위 완료는 아직 별도 진행중이다.

원본의 물리100기록밖Add/GetBounds는 메모리경계오류 입력으로 구분하며 정상반환을 꾸미지 않는다. C++은 해당범위를 진단한다. 전체실제섬생성/맵배치/WorldData이관·가상파생owner·GUI/캠페인플레이/완주를 검증한 보고서는 아니다.
