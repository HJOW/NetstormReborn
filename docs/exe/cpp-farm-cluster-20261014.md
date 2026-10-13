# edgeFarm121 군집과 실제 단일 배치 소비자 복원

작업은2026-10-13 시작해14일 후속 단위로 정리했다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이며 새helper 내보내기는 **2026-10-13 23:24**에 완료됐다. 작업 전 최신 AGENTS.md 전체와 두 LEFT_JOBS를 다시 읽었다. 10.78의 복원과 CD10.72 주소/차이를 함께 기록한다. 보호 원본/AGENTS.md/dotnetpj·기존 IslandRuntime/Queries/RawFortRandomPlacement 완료몸체와 공통 CMake/LEFT/CoreData/RawSurfaceWorld는 이 군집 구현에서 수정하지 않았다. 별도 승인한 Queries 등록 교정은 그 단위의 독립 lineage에 기록했다.

## 실제 원본 범위

| 역할 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 군집 entry | `0040e560` | `0042cd80` |
| 지역의 첫 Territory record | `00432430` | `004047c0` |
| 현재 청크 getter | `00432180` | `00404090` |
| 군집 local PRNG | `004bca20` | `004a59d0`→`004a59a0` |
| 실제 단일 배치 | `0040db90` | `0042c3d0` |
| 실제 edgeFarm121 생성자/최종vtable | `00462630`/`0050a8e8` | `00416d80`/`00500b10` |

군집 본체는 root의 현재 `terrainworld` **10/10** readOnly export에서 읽었다. 별도 `farmcluster-helper-functions.json`의 **패치3/CD11** 순차 Refined 내보내기97449가 종료0으로 완료됐다. CD 좌표helper `0042c010`과 벡터정리 `0042cf7b`→`00407c50`→`00407c91`→`00407c40`도 실제 명령을 실행한다. 실제 CRT allocator/free만 명시 외부경계다.

이전 VM의 `constructionarrival/fortobjectload/fortrandomplacement` 개별 내보내기는 현재 작업폴더에 없다. 기존 oracle의 명시 primitive/자료 구성 메서드를 재사용하면서 새 군집oracle은 보호PE와 현재메모리를 독립 초기화했다. 현재 HJOW-Athlon `refined/{edition}/Netstorm.c`/`functions.tsv`에서 기존fortrandomplacement **16/15 spec**, 현재finder **8/6**, 실제 도달하는 작은 point/genus/flags/프레임/난수/121Ctor 범위를 명시선택해 실행했다. 전체파일SHA와 선택된 기존range를 보고서에 보존했다. 임의 미허용 명령/쓰기/범위확장으로 성공을 만들지 않는다.

## 실제 시드와 군집 순서

시드는 현재 ChunkMap에서 요청지역과 같은 **첫 record의BYTE+3×77**이다. 조회는 x가 먼저 증가하고 한 줄 끝에서y가 증가하므로 **y바깥/x안쪽**이다. 현재 소유자DWORD나 시간시드를 쓰지 않는다. CD는 현재3204BYTE IslandList의 Exists/validId를 먼저 확인해 부재를 assertion으로 보고한다. record가 없으면 CD는 두 번째 진단을 보고하고0record/시드0으로 계속한다. 패치는 찾지 못했을 때 조용히0record를 반환한다.

군집은 별도의 **GameRandom local**을 만들며 최초목표는 quantity×20의 DWORD감김이다. 높이 차이를 CRT절삭한limit로 **dy먼저**, 폭limit로dx다음 난수를 뽑는다. limit<1이면0을 반환하며 local상태를 소비하지 않는 원본helper 분기를 보존한다. 두 판본에서 각 단일 배치에는 같은 local 객체, 현재edgeFarm타입, 단정도x/y, 요청region, 고정mode1을 전달한다.

CD의 decompiled prototype은 localRandom의this를 생략한다. 실제 PE는 `0042c010`의 전달포인터를ESI에 보관하고 `mov ecx,esi`→`004a59d0`→`004a59a0`으로 소비한다. 군집위치는 전역GameRandom에서 뽑지 않는다. 단일 randframe프로필의 프레임선택만 기존 globalGameRandom을 소비하며 두 상태를 fixture에서 분리한다.

중심의 단일배치 반환이nonzero이면 목표를1 줄이고 budget을30000으로 되돌린다. 주변은 **x offset바깥/y offset안쪽**의−radius..+radius 포함순서이며 중심좌표도 다시 시도한다. 주변의 성공에서만 목표를 감소시키고0이 되는 즉시 남은 주변시도를 멈춘다. 실제 모서리 거부반환 **1도 원본군집은nonzero 성공으로 센다.** 실제새SID나 생성수로 재해석하지 않는다.

초기 budget은30000이며 목표가남은 각 중심시도 끝에서1 감소한다. 성공중심의 끝에서도1 감소하므로 다음시도는29999에서 시작한다. 수량0도최초중심을 실행하고 최초실패에서남은0 검사로 끝난다. 수량0에서중심성공이면−1로감겨 바로 끝나지 않는 원본동작을 유지한다. 정상입력은 양수수량의 완료 또는 allfail의유한반환과 수량0의최초실패이며, 무한에 가까운0수량연속성공을 정상완료로 꾸미지 않았다.

`archipelago && region==0`이면 현재span×16으로 left/top을−(extension−16), right/bottom을+extension 확장한다. 목표는 **signed(quantity×20)/9**의0방향절삭 후 `(2×span+2)²`을곱한DWORD다. force1/radius2로5×5 주변을 시도한다. 나머지는 force0/radius1의3×3이다. 이 분기는 한 번 정한area/region/force/radius snapshot이며 각 실제호출의edgeFarm타입은 현재전역을 다시 읽는다.

## 같은 월드 API와 재사용

`RawFarmCluster`는 같은 pool, 현재 **ChunkMap**, 실제 **RawIslandRuntime**3204BYTE, 기존 **FortRandomPlacementState**, 현재archipelago/span, 필수place/diagnostic을 참조한다. `Place(FortRandomRectangle,int32_t region,uint32_t quantity)`가 원본void 군집몸체다. 사본 섬벡터를 만들지 않으며 현재지역record/Exists/Owner를 동일자료에서 읽는다.

`MakeFarmClusterPlacementHook`은 pool동일성을 검사한 뒤 기존 **RawFortRandomPlacement::Place**를 직접 연결한다. 실제단일모듈의spot→finder→legalDrop→현재chunkRegion→지면/모서리→Retype/HP/프레임→owner/Pop 순서를 다시 구현하지 않았다. 현재CMake에 단일모듈이 빠진 사실은 root에 전달했고, 후속등록에서 기존 `.cpp`도 함께 포함한다. 이전 VM의독립placement 관찰을 신규로 더하지 않는다.

실제121Ctor는 이미완료된 **SquidFactory::Construct/DerivedConstructors**의 vtableDWORD쓰기다. 신규생성자recipe를 만들지 않았다. 실제Ctor 군집입력에서는 native도`00462630`/`00416d80` 명령을 실행하고 C++도기존Factory에직접위임한다. constructor0 fallback입력은 원본이제공하는 별도의basevtable분기로 보존했다. constructor미복원/noop callback으로 실제Ctor완료를 주장하지 않는다.

## 원본 입력·C++ 대조와 한계

최종 고유입력은 **208개**다. patch는 **정상104/assert0**, CD는 **정상101/assert3**이며 합계 **정상205/assert3**을행마지막의assert호출횟수로분류했다. 각판본104개이며 quantity1/2, seedBYTE0/1/255, region0/2, archipelago0/1, span1/2, 실제matchframe/모드1 지면교체와 **명시합성randframe 프로필**, allfail의수량0/1/−1, 지역/record부재, 실제121Ctor추가4개를분리한다. 두x87반복 정상복귀/군집entry는각416회이며 신규입력에 더하지 않는다. 실제121Ctor의실행은양판본208회씩이지만기존생성자관찰을별도의신규몸체완료로합산하지않는다. 기존Runtime2018/Queries8490/08과 단일placement/121Ctor의이전관찰은 신규추가0이다.

모든 단일entry의 local상태/type/x/y/region/mode/force와 실제반환을 DWORD열로관찰해 traceAdler로 고정한다. L/U/K/R/C/O/P/원본assert의 실제primitive 순서/인자는 별도effectAdler로 저장한다. 청크getter의read요청G는 게임효과가아니므로 실제callee통계로 별도보존한다. 실제변경 ground와 생성슬롯의전체raw, 공유global과 마지막local 상태, 진단개수도 비교한다. read분류전/후 최초204 preflight의tool/fixture/report/log는 `extracted/farmcluster-before-{read-classification,real-constructor}-*`에 보존했고, 동일입력의예비재실행을 최종고유입력에 중복합산하지 않는다.

native는 군집/지역시드/local난수/단일Place/finder/모서리/Retype/HP/프레임/121Ctor 명령을 실행한다. **완전한legalDrop, factory전체, 가상공간/owner, allocator와원본assert**는 명시primitive경계다. Unpop/Pop/Repop경계는 실제같은raw해시의제거/재등록과void/좌표쓰기를수행한다. 지면121로교체한뒤에도hash0에잔류시켜후속finder가못찾는 모형을 사용하지 않는다. 입력constructor0/합성randframe과factory잔여payload는 실제타입전체메타데이터/Factory전체초기화를 검증한것으로세지 않는다.

C++ **3검사**는 양판본전수, 필수효과/다른월드 거부다. 실제단일Place와 같은pool/hash/타입/청크/IslandRuntime/두난수를연결하고 실제Factory의121Ctor를소비한다. root의등록전검토에서새등록매크로3개를실제`TEST_CASE`로교정했고 전수최종report생성전에현재소스에반영했다. 최종native34897은종료0으로208개전수/현재SHA/정상·assert분리감사가통과했다. fixture SHA는 `c5e53132543b400050630a81ca9df677d1595eed595ef757fb1398b7ac19db24`다. 교정후단위09공통Release는경고/오류0이며 `extracted/campaign-runtime9-FarmCluster_initial-tests.log`에서 **3tests/실패0·양판본208전수(정상205/assert3)**가통과했다. 09전체검사는다른단위교정뒤별도후속이며현재단위의C++집중통과와구분한다.

09두번째공유빌드는 `RiftTypeFrames{}` 기본생성자가없어실패했다. 벡터최초생성과행별재초기화모두원본의명시빈생성 `RiftTypeFrames({})`를요구하므로두구문을교정했다. `recovery-farmcluster-cpp-correction-evidence.json`은교정전전체test/oracle/native보고서원문과SHA를내장하며 `analyze_farmcluster_cpp_correction.py`가두구문역변환·208fixture/전체나머지소스/PE/export불변을감사한다. 기존실행에사용한constructionarrival/damageablepredestroy Python의존성의SHA추가도오직보고서파일목록한구문역변환으로연결했다. 이빌드/메타데이터교정은 **새native0**이며현재실행기 SHA는 `77183fb47158e24bb7133853dbf93adb8c9dcc05171557c3467e46dc1a82e1ad`다. native/cpp교정 두감사와최종C++3집중이모두통과했고현재source/tool/fixture/report는동결한다.

자료는 `RawFarmCluster.h/.cpp`, `FarmClusterTests.cpp`, `decomp_farmcluster_oracle.py`, `farmcluster-helper-functions.json`, `farmcluster-x86.tsv`, `recovery-farmcluster-evidence.json` 및 `extracted/farmcluster-oracle-final.log`다. 전체GameWorld의지형성장/최초도장/군집호출순서·actuallegalDrop/factory/가상공간·전투/생산/GUI/캠페인1-1 완주는 별도필수후속이다.
