# 현재 지도의 Territory 저장

2026-10-23 단위364. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**으로 일치한다. 기존 `extracted/refined/originals/Netstorm.c`와 보호10.78 PE를 사용했다. 보호 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 새 디컴파일/게임 프로세스/GUI 실행과 AGENTS·보호 원본 변경은 없다. 직전 단위363은 `db1af853 / 20261023-30 CP`로 실제 기술 저장을 연결했다.

원본004bf6d0은 로컬120BYTE를0으로 만든 뒤004bd660으로 기존 Territory를 읽는다. 그 다음005c85a4를 한 번 읽어 분기한다. 일반 경로는 고정 Template 슬롯을 owner1..8로 clamp하고 그 밖은0으로 정한다. 지역0..19마다0046f380을 호출해 현재 noRegion과 비교한다. 지역이 존재하면서 기존 record[1]&1일 때만00432430의 현재 지도6BYTE를 복사한다. Editor 경로는0046f3f0으로 owner 조건 없이 검색하고, 존재하면 현재 지도6BYTE를 복사한다. 없으면 기존 기록을004b37c0으로 부분 변경한다. 누락 순번은 누락한 경우에만 증가한다. 근거는 정밀 원문 `Netstorm.c:152643~152737`이다.

그 후 현재 manager가0인지 진단521로 확인하고 새 Territory를 만든다. **생성 함수 반환값을 쓰지 않고 현재 Template manager와 manager+8 current를 다시 읽어 stream을 고정한다.** 이 stream의 signed capacity와 pos+120을 비교해 진단36을 호출하고, 복귀하면 같은 고정 stream의 현재 data/pos를 다시 읽는다.30DWORD에 해당하는120BYTE를 쓴 뒤 pos/end를 진행한다. 진단이 header/current를 바꿔도 이미 고정한 stream은 바꾸지 않는다. 이 단위는 상위 snapshot의 Notify/파일 저장 위치를 앞당기지 않는다.

두 지역 getter도 별도로 복원했다.0046f380은 진입 때 count를 보관하고 index 순서의 첫 존재 항목을 찾는다. 일반 모드는 signed originalOwner BYTE와 currentOwner DWORD가 요청 owner와 모두 일치해야 한다. Fort 모드는 currentOwner가0이거나 요청 owner와 일치해야 한다. 지역은 signed BYTE를 signed 인자와 비교한다.0046f3f0은 존재/지역만 비교한다. 둘 다 없으면127을 반환하며 기존 RawIslandRuntime::Matches0046f200으로 대신하지 않는다. 근거는 `Netstorm.c:91292~91357`과 실제 PE 관측이다.

004b37c0은 byte0의 상위2비트를 보존하고 하위6비트만 누락 순번의 하위6비트로 바꾼다. byte1은 bit3을 지우고 bit0/1을 켠다. 이 두 변경 후 공용 Next(254)를 호출하고, 복귀한 값+1의 하위 BYTE를 byte3에 쓴다. byte2/4/5는 보존한다. 난수 경계가 중단되면 첫 두 BYTE 변경은 남고 byte3은 아직 바뀌지 않는다. 근거는 `Netstorm.c:142601~142616`과 실제 PE 관측이다.10.72 대응 후보는 부모00427af0, getter004bdaa0/004bdb10, seed004300c0이며 이 주소만으로10.72 실행 동일성을 입증했다고 계산하지 않는다.

`RawFortTerritorySave`는 위 부모와 두 getter/seed, 현재 ChunkMap의 첫 섬 record 검색을 구현한다. 지도 검색00432430은 y 바깥/x 안쪽 순서의 첫 signed 섬 BYTE 일치를 복사하며 없으면0을 반환한다(`Netstorm.c:40935~40983`). 별도 축약 IslandList나 과거 Fort의 파싱 사본을 사용하지 않는다. 물리 버퍼/섬 기록 밖 접근과 누락된 실제 장치는 거부한다. 이 보호 처리를 잘못된 원본 입력의 메모리 손상과 동일하다고 주장하지 않는다.

`RawCampaignSession::SnapshotFortTerritory`는 같은 Runtime의 기존 ReadTerritory, 실제 islandBytes/RawIslandRuntime::Bytes, live ChunkMap, 현재 editor/fort/noRegion, 공유 Impl::random을 연결한다. 새 manager/stream은 CampaignTemplateStorage의 같은 노드/생성 수명을 사용한다. Runtime의 새 ReadTerritory 접근은 기존 원본 읽기 몸체를 재사용하며 재파싱이나 새 로드를 하지 않는다.

새 CurrentWriteStream은 Create 반환값과 독립적으로 같은 현재 manager/current를 읽고 노드의 생성 수명을 고정한다. 로드된 과거 노드에는 첫 빌림 수명을 부여한다. 감사에서 typed 읽기 cursor의 진행 위치가 raw node에 아직 반영되지 않는 경로를 발견했다. 같은 참조로도 읽기 뒤의 위치에 쓸 수 있도록 CurrentWriteStream과 AccessSection 직전에 공개된 typed cursor의 position을 동기화한다. 이 작업은 current 선택·end·할당·노드 수명을 바꾸지 않는다. 같은 이름 재생성/manager 해제 뒤 옛 참조는 계속 거부한다. 인라인 빌림 중에는 raw 위치가 최신이므로 재동기화하지 않는다. 같은 노드의 중첩 쓰기 빌림은 호스트 자료 수명 계약으로 명시 거부하며, 현재 stream 조회는 허용한다. 다른 노드의 독립 빌림과 원본 게임 writer 호출 순서는 바꾸지 않는다.

독립 도구 `tools/decomp_fort_territory_save.py`는 실제 PE 부모와 두 getter/seed를 구분해 실행한다. 부모의 Get/region/record/seed/Create/assert만 명시 외부 경계다. 부모 분기/복사/현재 재조회/stream 쓰기는 실제 명령이다. 실제 getter는 하위 호출 경계가 없고 count/존재/owner/지역을 직접 읽는다. 실제 seed는 Next254만 명시 반환 장치다. **부모319입력·638관측·414,478명령 / getter10,081입력·20,162관측·530,278명령 / seed94입력·188관측·3,750명령**, 합계 **20,988관측·948,506본체명령·정밀도 차이0**이다. 정상 스택/callee-save/x87와 본체 불변을 확인했다. 실패 중단은 실제 CALL 도달 상태이며 SEH unwind를 관측한 것은 아니다.

새 세 TSV는 각각26/11/10열이다. 부모는 앞15열 입력/뒤11열 관측으로 현재 manager/current·전역·두 stream·두 출력 버퍼·최종 로컬120BYTE·trace를 전부 보존한다. 일반 bit0은 켜짐/꺼짐을 교차하며 전부 켜짐/꺼짐도 포함한다. 최초 입력 수식이 모든 byte1을 짝수로 만들던 관측 공백은 생성 전에 보완했다. getter는 signed owner/region과 count4294967295, 존재/현재·원래 owner/첫 일치를 교차한다. seed는 순번 BYTE 감김·보존 필드·난수 경계 변경/중단을 포함한다. `extracted/cpp-fort-territory-save-20261023-native.json`에 원본/도구/세 TSV 해시,638실행의 두 버퍼/지역배열 해시와 범위를 남겼다.

새 검사는 이20,988행을 대조하고 별도로 실제 ChunkMap의 현재 변경·첫 조회 순서·signed 섬 BYTE, current 참조와 노드 수명, typed 읽기 뒤 위치 동기화, 난수 장치 누락의 부분 쓰기를 확인한다. 실제 Session 검사는 TheWarBegins를 설치한 뒤 새 작업 Fort의 기존 Territory를 오염시킨다. 현재 지도에 존재하고 소유권 조건이 맞는 지역이 복원되는지 확인한 다음, 갱신된 지역의 bit0을 끄고 다시 저장하여 기존 비활성 record가 유지되는지 확인한다. 같은 파일 저장 부모로 작업 파일을 쓰고 재파싱해120BYTE를 확인하며 보호 원본 불변을 대조한다. 출력은 `cpppj/build/test-data`의 새 작업본이다.

첫 Release 빌드1은 종료0이며 독립20,988행과 실제 Session 저장/재읽기가 통과했다. 저장소 수명 검사에서 빈 Territory 뒤 새 State 생성이 옛 tail을 할당0으로 압축한다는 기존 원본 동작을 빠뜨려, 빈 옛 참조로2BYTE를 쓰려다 용량 진단으로 실패했다. 유효한 참조의 필드 빌림과 실제 용량 실패를 별도로 확인하도록 검사를 수정했다. 인라인 중첩 쓰기 거부/진행 중 current 조회 보존 검사를 추가했다.

최종 Release 빌드2 종료0·경고/오류0, 새 관련5검사와 기존 TypeNames·Deck4/Technology3검사 모두 실패check0이다. 독립20,988행 전체와 실제 월드 저장/재읽기, 커서/참조 수명 및 부분 쓰기를 확인했다. 최종 코드 감사에서 추가 오류가 없었다. 이 검증 뒤 제품/검사 C++는 동결했다. 최종 전체 CTest1은 **2,364검사·실패check0·종료0/280.93초**, LastTest.log 내부2364/0이다. 로그는 `extracted/cpp-fort-territory-save-20261023-*`다. UTF-8/추가 한국어 주석/diff/보호 경로/원본·도구·세TSV SHA를 확인하고 인수인계를 마감해 **20261023-31 CP**로 로컬 커밋한다. 별도 dotnet 변경과 원격 push는 없다.

실제 Editor 설치/입력/새 맵 단계는 아직 이 API를 UI에 연결하지 않는다. 전체 snapshot에는 Objects/Terr·Chaff, Money 공용 난수, 실제 Mission Runtime/Badges live 장부와 마지막 Notify가 남는다. Notify의 BindFileSave 미연결/빈 filename 차이도 후속이다. 전체 Early Missions·캠페인·데모3 사용자 완주와 Multiplayer 외 메인 메뉴/GUI·음향·최종 배포 목표를 계속 유지한다.

다음 Objects 저장 조사도 보존한다.004be430은 단일 Objects 섹션을 만들지 않고 Terr00..19와 Fort일 때 Chaff를 쓴다(`Netstorm.c:151730~151761`). 공유005c89c0을2로 쓰고, 각 TerrNN 검색/없으면 생성→정확한 임시 이름`Temp` 생성→version BYTE→지역 청크→Temp 재선택(writable1)→data/end가 있으면 기존 TerrNN 제거/Temp rename 순서다. 이 이름은 State 확장의`T$e$m$p`와 다르다. version BYTE만 있어도 교체 조건을 통과한다. 현재 Storage의 표준35개 이름/handle 고정은 Temp/rename을 지원하지 않아 기존 생성·목록 제거·압축·RenameDataManagerStream 몸체를 연결해야 한다.

004be230/004be350은 같은 live ChunkMap을 y 바깥/x 안쪽으로 돌며, Terr는0046f200(기존 Matches) 정책으로 고르고 Chaff는 전체 청크를 처리한다(`151568`, `151642`).004be020은 기존 RawSquidFinder로 청크 사각형을 검색하고 표지99와 고정 count WORD를 예약해 단일 객체 저장 성공 수를 쓴다(`151476`). RawSurfaceWorld::Objects는 표시용 필터/SID 순서이므로 저장 입력으로 대체하면 안 된다.

단일 객체004bd940은 좌표 절삭/상태·전투·Terr/Chaff 필터/가상60/섬 위 판정 후 위치 nibble·타입/다리 약식·frame·QA/QB·가상80·owner·내용물을 저장한다(`151206`). SaveFortContents와 RawBuildingSpace::HasIsland는 재사용할 수 있다. 가상60은 ContentForm Robust와 다르다. 기존 PATCH Pop VT26개만 정적으로 보면22개는004bcb50의`!(flags1&0x20000000)`,4개는004429e0의 항상0이고 다른 파생 VT는 별도 분배 근거가 필요하다. 공유 version/scope는 기존 FortObjectLoadState와 Storage.managerGlobal_에 나뉘어 있으므로 writer를 추가할 때 수명/단일 저장소를 정리해야 한다. 다음은 단일 객체→청크/Finder→Temp 지원/부모 연결 순서이며, 세 계층이 연결되기 전에 전체 snapshot에서 호출해 기존 객체 섹션을 불완전하게 교체하지 않는다.
