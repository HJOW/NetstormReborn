# 같은 mutable 섬 목록의 소유자·지역 검색 복원

2026-10-13, 현재 호스트와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 새 작업 전 최신 AGENTS.md 전체와 두 인수인계를 확인했다. cpppj의10.78 복원과 CD10.72 대응 기록을 함께 유지한다. 보호 원본/AGENTS.md/dotnetpj 및 기존 IslandRuntime/Ascend/CMake/LEFT_JOBS의 완료 몸체는 수정하지 않았다.

## 원본 몸체와 주소

| 신규 몸체 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 소유자·지역의 첫 섬 검색 | `0046f380` | `004bdaa0` |
| 지역의 첫 섬 검색 | `0046f3f0` | `004bdb10` |
| 현재 소유자가 같은 섬의 개수 | `0046f430` | `004bdb60` |

별도 `islandqueries-functions.json`은 각 판본의 신규3몸체와 기존count/Exists/Owner3getter **6/6**을 정밀 내보냈다. root의 순차 Refined readOnly 내보내기75032가 종료0으로 완료됐다. 실제 CD Exists는 기존 `004bd480` validId를 호출하므로 이미 완료한 `island-runtime` CD14 export에서 **그 한 함수 범위만** 추가로 읽어 실행했다. 원본 전체파일의 SHA와 실제 선택range는 새 보고서에 기록했다. 해당getter/validId 실행횟수는 신규 독립 원본 입력에 더하지 않는다.

기존 **RawIslandRuntime**의 CoreData 저장소는 countDWORD+100개×32바이트=**3204BYTE**이며, 기록+4는 존재DWORD, +8은 현재ownerDWORD, +12는 territoryBYTE, +13은 원래ownerBYTE다. 새 검색은 같은runtime의 Count/Exists/Owner/Bytes를 직접 읽는다. 배치용 IslandList나 Ascend용8바이트기록으로 복사하거나 cast하지 않는다.

## 실제 검색 조건

**FindOwnerTerritory**는 실제번호0부터 존재DWORD가nonzero인 기록을 검사해 첫 번호를 반환한다. 일반모드에서는 **signed 원래ownerBYTE==요청owner**와 **현재ownerDWORD==요청owner의32비트**가 모두 필요하다. 현재owner가 달라진 섬은 원래owner가 같아도 선택되지 않는다. Fort모드에서는 현재owner가0이면 owner비교를 통과하고, 현재owner가nonzero이면 요청owner와 같아야 한다. 그 뒤 signedterritoryBYTE를 요청지역DWORD와 비교한다. 아무 기록도 통과하지 않으면 고정127을 반환한다.

위 분기는 두 PE의 실제 `movsx`/`cmp`/`je`를 읽어 확인했다. Ghidra C의 label흐름을 기존 `RawIslandRuntime::Matches` 조건으로 합치지 않았다. Matches가 사용하는 special/chaff는 이 세 검색 몸체의 분기조건이 아니다. 신규 입력에서 두 플래그를 각각 바꾸어 결과가 유지되는 것도 관찰했다.

**FindTerritory**는 현재 소유자/원래소유자/fort를 검사하지 않고 존재DWORD가nonzero이며 signedterritoryBYTE가 같은 첫 번호만 찾는다. BYTE255는 요청−1과 비교하고 BYTE128은 요청−128과 비교한다. **CountOwned**는 존재하는 기록의 현재ownerDWORD만 비교해 개수를 센다. 현재owner0, 256, `0x80000000`, `0xffffffff`도 그대로 비교한다. 원래ownerBYTE로 줄이거나0인 소유자를 건너뛰지 않는다.

count는 원본처럼 signedDWORD로 해석한다. count0 또는 부호비트가 있는 값에서 순회하지 않으며 검색은127/계수는0이다. 삭제 뒤count가 줄지 않는 기존runtime 정책을 유지하며 빈기록을 건너뛴다. 번호99까지 원본순서로 검사한다. 패치의 두 첫일치 검색은 시작count/fort를 보관하며 CD는 현재count/fort를 다시 읽는 형태도 보존했다. 외부callback은 이 세 몸체에 없다.

## 새 API와 실제 소비자 경계

`RawIslandQueries(const RawIslandRuntime&, const FortFinishOwnerState&)`는 월드가 장기소유하는 같은runtime과 현재모드를 참조한다. `FindOwnerTerritory(int32_t owner,int32_t territory)`, `FindTerritory(int32_t territory)`, `CountOwned(uint32_t owner)`는uint32_t 반환이다. 새 자료의 소유권은 만들지 않는다.

기존 **CountForeignOwnedIslands**와 Runtime의 Count/Exists/Owner 완료 몸체는 중복 구현하지 않았다. 새 CountOwned는 현재owner가 요청값과 **같은** 기록을 계수하는 별도의 원본몸체다. 일반 NativeCount/getter회귀나 이미 보존한 Ascend1496/Runtime2018 입력을 새 관찰로 합산하지 않았다.

실제 Player/승패·초기배치 소비자는 이 조회API를 같은 CoreData에 연결해야 한다. 이 작업은 해당 호출자의 순서/수명과 GameWorld연결까지 완료했다고 주장하지 않는다. 물리100기록을 넘는 양수count는 원본 배열밖 접근이므로 정상입력에 포함하지 않았다. C++은 기존runtime의 물리기록 경계진단을 통해 거부한다.

## 독립 원본 실행과 검사

고유입력은 **8490개 정상/assert0**이다. 판본마다 **4245개**이며 FindOwnerTerritory3455/FindTerritory426/CountOwned364다. x87 control `0x027f`/`0x037f`에서 **16980회 정상복귀**했고, 보존레지스터/stack/x87와 반환값이 일치한다. 반복반환은 고유입력에 더하지 않는다. 실제게임/GUI/OS와 외부stub 호출은 모두0이다.

입력에는 원래owner/currentowner 불일치, signedBYTE255/128,0소유자/32비트상위값, 존재DWORD0/1/2, count0/1/3/100/부호비트, 삭제구멍/여러일치/최초번호/번호99, fort/special/chaff를 포함했다. 결과EAX를 실제 PE에서만 읽으며 Python/C++로 원본의 기대검색 결과를 계산하지 않는다.

native 메모리hook은 스택저장만 허용하므로 섬기록/원본전역의 예상밖 쓰기는 즉시 실패한다. 매 입력 뒤 실제 **전체3204BYTE가 입력과 같은지 직접 비교**하고 Adler32와 현재세모드를 fixture에 저장한다. C++도 전후전체array와 원본Adler/모드/revision 불변을 대조한다. 반복되는6.4KB hex를 행마다 보관하지 않고682534BYTE fixture로 고정했다.

CD의 실제 기존callee 호출은 x87반복 포함 Exists/validId각18912회와 Owner2054회다. 패치의 신규몸체는 해당getter를 인라인해 직접 읽었다. 이 숫자는 실행사슬 통계이며 신규 독립입력이 아니다. 허용 native 범위 밖 실행/미매핑접근/섬기록쓰기는 임의stub나 메모리 보충으로 처리하지 않는다.

현재 **native 전수 생성과 SHA/고유입력/신규검색·기존getter 분리/두정밀도 반환 감사가 통과**했다. 자료는 다음과 같다.

- `cpppj/src/o/RawIslandQueries.h/.cpp`, `cpppj/tests/IslandQueriesTests.cpp`.
- `tools/decomp_islandqueries_oracle.py`, `tools/ghidra/islandqueries-functions.json`.
- `cpppj/islandqueries.json`, `cpppj/tests/fixtures/islandqueries-x86.tsv`.
- `extracted/islandqueries-oracle-final.log`, `extracted/islandqueries/{originals,originalCD}/{creation.c,functions.tsv}`.

fixture SHA는 `9a95feaf0093fe87a35d095f4d9233a8a560aff5e225e649e6bd4e8e8e359ebb`, 실행기 SHA는 `39f222b4638e064684f50f18c13c9eb4a865bc5c252b6020fff6b531e7c6cfc0`다. PE/내보내기/현재신규source와 기존Runtime파일 SHA는 보고서에 함께 고정했다.

C++ **3개 집중검사**는 양판본 전수와 실제 Add/Delete/Reset·현재fort/liveDWORD 변경·물리overflow를 대상으로 준비했다. 2026-10-14 단위09의교정후공통Release는경고/오류0이며 `extracted/run9-IslandQueries_first.log`에서 **3tests/실패0·양판본8490개전수**가통과했다. 09 전체검사는다른단위교정뒤별도후속이며 아직전체09완료로세지않는다. 독립 검색복원은 초기섬 생성·실제 Player선택/승패·UI/캠페인1-1 완주 검증을 대체하지 않는다.

공통 등록 전에 정적검토로 검사장면의 `SidPool(edition,128,false)`가 원본 predictable SID영역보다 작아 생성 시 거부되는 결함을 발견했다. root 승인으로 해당 한 줄만 기존 검사상수 **kCapacity24000**으로 교정했다. production/원본실행기/fixture8490/PE/export와 이전 모든 나머지 소스는 그대로다. 교정전 전체검사/전체native보고서와 SHA는 `islandqueries-pool-correction.json`에 내장했고 `analyze_islandqueries_pool_correction.py`가 한줄 역변환과 이전 전체출처 불변을 감사한다. current native보고서는 이 lineage/current검사SHA를 연결한다. **새native0**이며 교정당시C++실행전상태와최종09집중통과를구분한다.

2026-10-14 root의 등록전 검토로 실제 TestSupport 등록매크로가 `TEST_CASE`임을 확인했고 새3개 등록의 `TEST` 식별자만 교정했다. `islandqueries-macro-correction.json`은 직전 전체검사/native보고서/pool근거/pool실행기 원문과 SHA를 내장한다. 새매크로실행기가세identifier 역변환·이전전체나머지소스/fixture8490·새native0을 감사하며, pool근거는 `macro_successor`로 현재검사와 옛pool 교정전/후를 연결한다. native/macro/pool 세 감사가 모두 통과했다. 정적등록교정뒤실제3검사/8490 CPP대조를완료했고현재source/fixture/report는동결한다.
