# 저장 Fort 부모의 원본 순서·Territory와 같은 월드 연결

현재/마지막 디컴파일 PC는 **HJOW-Athlon**, 날짜는 **2026-10-14**다. 최신 AGENTS 전체와 양 LEFT 최신 인수인계를 다시 읽고 단위11 **5bdb5be/전체1038**와 동결한 단위12 생산/원본 자료를 보존했다. 기준은10.78이며 CD10.72의 실제주소·차이도 기록한다. 이 작업은 새13파일만 사용하며 CMake/LEFT/공유 빌드/커밋/GameWorld 교체는 root 담당이다. 보호 원본/AGENTS/dotnet 변경·게임/GUI/OS 실행은0이다.

## 실제 부모의 순서

패치 **004b5f70**, CD **004330c0**를 `extracted/refined/{edition}/raw.c`와 새 현재PC `extracted/fortworldload`의 정밀원문으로 확인했다. 새 spec은 **4/7주소**이며 Graph/Populate 몸체도 읽기자료에 포함한다. 기존 완료Builder/Runtime/CanonDecoder를 새롭게 다시 구현하거나 완료수로 중복 합산하지 않는다.

1. 실제 builder ctor→같은 world ChunkMap attach. 최종 owner는 **1**, 청크 bounds는 **(1,1,15,15)**다. 현재localOwner와builder owner는 같은 값으로 합치지 않는다.
2. 임시 Territory **120BYTE를0**으로 만든다. patch는6BYTE×20 인라인쓰기이며 CD는 **0040f1e0**을20회 부른다.
3. 현재localOwner의 Template에서 **004bd660/CD00427c00** Territory 읽기.
4. 기존 **0046dd70/CD00490980** PlaceAll(records,0,20).
5. 기존 **0046da70/CD00490530** Generate(builder,현재noRegion).
6. 인자0이면 현재Template의 **ObjectLoad→Money→Badges→Technology**를 실행한다. 각child 직전 현재localOwner를 다시 읽어 Template 주소를 재계산한다. 첫Template snapshot을 뒤child에 고정하지 않는다.
7. 인자가nonzero이면 현재island0의 **Exists**를 읽고 nonzero일 때 **0046e630/CD004906c0** PopulateIsland0를 실행한다. count/Exists는 같은3204BYTE이며 별도vector를 만들지 않는다.
8. 마지막 **00463110/CD0045bd60(0)** Graph 재구성. patch의Populate 후 별도return 분기도 Graph는정확히한번이다.

원래 [초기월드순서 감사](cpp-fort-initial-world-order-audit-20261014.md)의 과거 Technology/Deck 포괄명칭은 실제 **Money→Badges→Technology** 직접호출로 정정됐으며 기존문서는 당시출처로 유지한다.

## Territory의 실제 현재pos 복사

Territory getter는 Template 첫pointer가0이면 patchline0x22e/CD0x226 진단 뒤 계속하고 `Territory` 섹션을 **resetpos=0**으로 조회한다. 반환된 stream의 **현재pos에서 DWORD30개=120BYTE**를복사한다. pos에120을더하고 **signed pos>end**일 때 line0x2e를보고한다. end는변경하지 않는다. Badges/Technology의resetpos1과구별한다.

CD getter는 진입 시 loadingDepth+1 후 **00427cd2→00429f30**이즉시−1하여 Template진단/섹션lookup 전에복구한다. 함수 끝에 또−1하지 않는다. patch에는이깊이쓰기가없다. 같은Depth DWORD는 Money/Meta/객체/Carrier/표면소비자와공유한다.

C++ `ReadTerritory`의 지원범위는 현재pos부터120BYTE가있는물리메모리이며 output과input이겹치지않는정상저장소다. 부모의local120BYTE는입력stream과분리돼있다. 부족한물리자료는부분복사나0채움으로성공처리하지않는다. nullTemplate의원래null역참조가완료됐다고주장하지않는다. 이번진단입력은 명시assert 경계가 Template 첫pointer를복구한뒤원본복사로정상반환한관찰이다.

## C++ 모듈과 재사용 경계

새 `RawFortWorldLoad`는 같은Pool/ChunkMap/RawIslandRuntime/frames와 현재puzzlePiece/localOwner/noRegion/Depth 참조를 받는다. 기존RawIslandBuilder를 동일world주소에서만사용하고 PlaceAll이쓴 전체청크/32BYTE Bounds를후속에그대로전달한다. 모든child/진단/Template선택경계는필수이며누락되면부모실행전에거부한다.

새 `MakeFortWorldChildrenHooks`는 같은풀의기존 `RawFortObjectLoad::Load`와 `RawFortMetaLoad`를연결한다. 각Metachild 전에현재Template칸/첫pointer/current localOwner를동기화하는selectMeta가필수다. 이미parsed FortTemplate의Contents/typeConversion 계약은기존Meta설명과동일하다.

새 client `MakeFortWorldLoadBinding`은 기존 `RawNormalTerrainWorldBinding::Generate`에builder owner/bounds/현재지역을그대로전달하고, 실제RawFortMoneyLoad와섹션선택경계, 기존Objects/Meta, 같은Core.Graph의 **Rebuild(false)** 를참조한다. 같은난수·독립CRT99cache·지역/장부·Pool/Hash/Factory를재사용하며기존완료몸체를재작성하지않는다. Normal binding과부모가같은ChunkMap/Runtime/전역을갖도록호출자가연결해야하며풀참조를검사한다.

군도와PopulateIsland0의전체하위생성·가상삭제, TypeNames runtime 전신·현재Technology 부모getter/UI는미복원범위를명시하고요청되면거부하거나해당원본효과를공급해야한다. 전체게임완료를기본noop으로대체하지않는다.

## 마지막 Graph와 현재 전체풀 제한

기존 Graph의 **00463110/CD0045bd60** 원본 재구성은 이미 [Graph재구성](cpp-graphrebuild-reconstruction.md)에서관찰됐다. 부모인자0은 **Rebuild(false)** 이며 Rebuild(true) 전체초기화로바꾸지않는다.

현재 `RawGraph::Calculate`의global/fullPool 순회는 free 아닌 **type<70**을거부한다. 원본은그Form들의Type flags1&0x800을읽어비표면이면건너뛴다. 따라서현재Technology Contents6·ProcessForm46이생긴전체저장월드는아직이Graph로닫을수없다. 후속허용에는기존전체Graph body/fixture를보존한최소확장lineage가필요하다. 이번선택consumer는 **Form이없는** 범위로검사하며기존Graph소스를수정하지않았다.

## 독립 원본 관찰과 현재 출처

`decomp_fortworldload_oracle.py`는두PE에서 새부모/Territory/zero120과기존완료Builder ctor/attach/PlaceAll·Runtime.Add/Bounds/Exists·CanonDecoder를실제실행한다. 전체청크256×12BYTE 및원본3204BYTE 기록/padding, 현재owner/noRegion/Depth/streampos/end,120BYTE 출력과진단/후처리순서를저장했다.

**Generate/Object/Money/Badges/Technology/Populate/Graph 하위는부모oracle의명시관찰경계**다. 이순서관찰을하위전체생성/GUI/게임완료로세지않는다. 각child의현재owner 변경과 Generate뒤island0 Exists변경도관찰했다. fixture의1/8변경은native PlaceAll진입에서현재owner/noRegion만바꾸며C++ replay는lookup경계에서같은다음조회앞에그변경을공급한다. 실제Builder는owner1을유지하고그사이에해당두전역을읽지않는다.

| 판본 | 고유입력 | 정상 | 진단후반환 | 두x87 ABI복귀 |
|---|---:|---:|---:|---:|
| 10.78 | 267 | 126 | 141 | 534 |
| CD10.72 | 267 | 126 | 141 | 534 |
| 합계 | **534** | **252** | **282** | **1068** |

fixture SHA256은 **18d9946e08d189c92388e11e5163802b54299d2cc7b9c08f68579c11e4a9d59f**다. 정상/assert는행마지막진단횟수로분리한다. smoke/실패·x87 반복·기존1156 Builder/2018 Runtime/Graph 입력은534에더하지않는다.

첫smoke의CD missingTemplate 복구가실행중ECX를this로오인해중단됐다. 진입시Template this를보관해해당pointer만복구하도록교정했고처음whole도구를보존했다. 최종30×두판본smoke와전체534가성공했다. 생성후C++의namespace닫힘누락도정적확인해등록전에두CPP EOF에 **각`}`와LF 2BYTE만**추가했다. 당시새CPP/test/native/tool/spec/첫smoke도구 **whole11파일**을 `fortworldload-before-static-namespace`에보존했다. 최초원본보고서/도구/534행의source SHA를덮어쓰지않는다.

현재완료출처감사는 **`python tools/analyze_fortworldload_binding.py`**다. 초기native의producer CPP 한파일은보존된whole snapshot으로명시연결하고현재source6파일·기존생산의존성52파일·이전원본/교정보고서8개·전체역사의현재SHA를엄격히연결한다. 정확한EOF역변환이며 **추가binding native0**이다.

## 새 C++ 검사와 남은 범위

새 TEST_CASE는3개다. 전수534의현재raw/후처리/stream과직접getter의120BYTE를대조하고,미연결child/다른풀/물리부족을거부한다. 별도선택consumer는같은실제World에서단일Territory→일반성장/Stamp/edgeFarm121→기존빈ObjectLoad/FinishOwner 자연반환→Money→Badges→빈Technology→실제Graph(false)를조립한다. 현재Depth1/owner1과그대로인섹션cursor·Factory/지역/Hash/HP/Kernel·서로다른GameRandom/CRT99cache를검사한다. Technology 부모getter/UI는명시관찰경계이고군도/Populate/TypeNames/저장비표면자산및전체피해효과는완료로세지않는다.

**현재 새13 C++ 공유등록/실행대기**다. 단위12교정/최종커밋과원본자료는분리했다. 실제전체Fort/GameWorld설치·Form이있는전체Graph·경제/AI/생산/운반·시각/음향·승패/UI·캠페인1-1 완주동일성은계속미완료다.
