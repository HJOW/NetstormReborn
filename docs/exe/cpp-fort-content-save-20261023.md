# 현재 기술 내용물의 원본 저장 순서

2026-10-23 단위363. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이다. 기존 `extracted/refined/originals/Netstorm.c`와 보호10.78 PE를 읽고 새 디컴파일/게임 프로세스 실행은 하지 않았다. 보호 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. AGENTS와 보호 원본을 수정하지 않는다.

원본004bcea0은 같은 stream의 현재 data+pos에 count BYTE를 예약한다. 용량 진단 없이 pos/end를 먼저 진행하고 그 주소에0을 쓴다. 이 count 주소는 이후 callback이 stream.data를 바꾸더라도 고정된다. 각 독립 finder 후보가 물리 SID 범위 안이면 가상+60 Robust를 호출한다. 통과하면 count를 BYTE로 증가시키고 현재 후보의 kind WORD를 읽는다. count는256번째에서 감긴다.

첫 GetType(kind)의 flags1을 보관하고 kind 하위 BYTE를 쓴다. flags1의1000이면 현재 후보+c BYTE를,2000이면 현재+c WORD를 각각 쓴다. 둘 다 켜지면 두 번 읽고 두 값을 모두 쓴다. 각 값은 용량 진단 전에 읽고 진단 복귀 뒤 현재 data/pos를 다시 읽는다. 두 번째 GetType은 최초 kind를 사용하되 현재 contentListFlags를 읽는다.0이면 진입 때 고정한 부모의 현재 containerListFlags를 읽는다. 여전히0이면 현재 후보+14의 BYTE를 쓴다. 그 외에는 현재 WORD와32BIT mask가 모두 일치하는지 진단278로 확인한다. 최초 flags1의20000이 켜져 있으면 현재 후보로 재귀하고, 자식 목록이 끝난 뒤 부모 Next를 호출한다.

`RawFortContentSave`는 이 순서와 부분 쓰기를 보존한다. 호출 스택 대신 명시 프레임을 사용하고 각 프레임의 count 위치/부모/독립 cursor를 보유한다. 순환 부모나 물리 버퍼 밖 접근은 호스트 메모리 손상으로 거부한다. 이 보호 처리를 원본의 잘못된 입력에서 스택 소진/메모리 손상과 동일하다고 주장하지 않는다. Pool 기준 주소는 고정 계약이다. 이 계약에서 부모 SID는 고정 raw 슬롯을 나타낸다. 추후 Pool 기준 자체를 callback 안에서 교체하는 경계를 지원하려면 별도 raw 부모 수명을 추가해야 한다.

`RawCampaignSession::SnapshotFortTechnology`는 설치/슬롯/manager를 확인하고 현재 Player의 기술 부모를 섹션 생성 전에 구한다. 같은 TemplateStorage의 새 Technology 쓰기 stream에 실제 RawContainedFinder·ContentFormHost::Robust·Pool·TYPE를 연결한다. 각 Begin은 새 검색기와 현재 SID를 소유한다. 실제 ParentType004ac550은 부모 슬롯의 BYTE+10을 통해 현재 TYPE를 읽는다. 저장 부모의 Notify, 전체 월드 snapshot, UI 입력 위치를 이 명시 API에서 앞당기지 않는다.

독립 관측 도구 `tools/decomp_fort_content_save.py`는 실제004bcea0~004bd123 명령을 실행한다. Begin/Next/Robust/GetType/ParentType/진단만 명시 외부 경계다. 실제 본체가 count/폭별 저장/현재 재조회/재귀를 수행한다. **353입력×두 x87 제어어 =706행,273,296본체명령, 정밀도 차이0**이다. 정상 stackdelta0·callee-save·x87 보존과 실제 본체 불변을 확인했다. 실패 경계의 음수 stackdelta는 명시 실행 중단이며 C++ 예외 unwind 검증이 아니다.

새 TSV는 앞10열 장치 입력/뒤12열 원본 관측이며22열이다. QA4조합, content/부모 fallback,32BIT mask 진단, kind65535 하위 BYTE,254~257개 count, 재귀/형제, 현재 후보·한계·타입·data 변경, 진단 복귀/중단과 부분 자료를 포함한다. 두 물리 버퍼를 모두 비교해 count 위치 고정을 확인한다. `extracted/cpp-fort-content-save-20261023-native.json`에 원본/도구/TSV 해시와 범위·명령·경계를 남겼다.10.72 대응 후보00426b60은 기존 정밀 원문의 주석에서만 확인했으며 이번 실행 검증은10.78이다.

검사는 독립 전체706행의 완료 여부·pos/end/data·두 버퍼·trace·현재 limit/Form/TYPE를 비교한다. 별도 검사는 빈 목록의 선택하지 않은 장치 생략, 용량0에서도 header 저장, 누락된 실제 Robust 및 순환 거부를 확인한다. 실제 Session 검사는 TheWarBegins를 적재하고 새 작업 Fort에 현재 TypeNames와 기술을 저장한다. 기술을 지우고 sunCannon을 추가한 실제 Form과 Robust가 거부하는 Form을 함께 둔다. 파일 저장 부모로 저장/재파싱해 robust 기술만 확인하고, 기술 삭제 뒤 같은 파일에 다시 저장해 목록이 비는지 확인한다. 입력은 TARC의 원본이며 출력은 `cpppj/build/test-data`의 새 작업본이다.

첫 관련 검사에서 실제 Session 저장/재읽기와 빈 목록/잘못된 장치 검사는 통과했다. 독립 관측 중 Robust 장치가 비영 DWORD를 bool로 축약하여 두 행의 외부 상태 출력만 달랐다. 장치의 raw DWORD를 보존하고 제품 호출 때만 비영 여부를 전달하도록 검사 장치를 수정했다. 제품 저장 본체는 수정하지 않았다.

최종 Release 빌드2는 종료0·경고/오류0이며 독립 전체706행과 관련3검사는 실패check0이다. 별도 코드 감사에서도 실제 Pool/Player/가상 Robust/부모+10 TYPE/명시 stream 수명에 추가 오류가 없었다. 최종 전체 CTest1은 **2,359검사·실패check0·종료0/257.92초**, LastTest.log 내부2359/0이다. 로그는 `extracted/cpp-fort-content-save-20261023-*`에 있다. UTF-8/추가 한국어 주석/diff/보호 경로 및 원본·도구·TSV 해시를 확인했고 인수인계를 마감해 **20261023-30 CP**로 로컬 커밋한다. 최종 검증 뒤 제품/검사 C++는 변경하지 않았다. 별도 dotnet 변경/원격 push는 없다.

전체 snapshot의 Objects/Terr·Chaff, Territory 현재 지역/120BYTE, Money 공유 Impl::random, Mission 실제 Runtime 이름/state, Badges live28×220장부/Notify는 남아 있다. 일반 Battle Mission을 guided Tutorial getter로 판정하거나 과거 Fort의 Badges bytes로 현재 장부를 대체하면 안 된다. generic Fort 설치/Editor Player·새 맵·입력·State1/6와 전체 사용자 완주는 계속 미완료다.

다음 Territory 단위의 읽기 전용 조사도 보존한다.004bf6d0은 먼저004bd660으로 기존120BYTE를 읽는다(`Netstorm.c:152674`, `151020`). 일반 모드는 Template 슬롯을 owner1..8로 clamp하고20개0046f380을 조회하며, 존재하는 현재 지역이면서 기존 record[1]&1일 때만 현재 지도 record6BYTE로 바꾼다. Editor special005c85a4 모드는20개0046f3f0을 조회하고 없는 지역은004b37c0으로 기존 record의 byte0 하위6비트·byte1 일부·byte3만 바꾼다. 빈 순번은 누락마다 증가하고 byte3은 같은 random.Next(254)+1이다(`152680`, `152699`, `142605`).

같은 live 지도는 Impl::chunks/islandBytes와 RawIslandRuntime에 있다. RawIslandRuntime::Bytes, RawPlayerGeyserBounds::RegionRecord의 지도 검색을 참고하되0046f200인 Matches를0046f380 대신 쓰면 안 된다.0046f380은 일반모드에서 originalOwner BYTE와 currentOwner DWORD가 모두 요청owner와 일치해야 하며 Fort모드에서는 currentOwner0 또는 요청owner를 허용한다. Territory 저장은 CreateSection 반환값을 무시하고 현재 Template manager/current를 다시 조회한 뒤 그 stream을 고정해 용량 진단→DWORD30개→pos/end를 진행한다(`152716`). Getter도 Select 반환값 대신 현재 manager/current를 다시 읽는다(`151033`). 따라서 단순 WriteSection으로 조회 순서를 합치면 안 된다.

Notify004bd360은 Territory 안에 없고 전체 snapshot의 Deck→Money→Mission→Badges 뒤 마지막이다(`152805`). 기존 RawPlayerTemplateState::Changed/BindFileSave를 재사용할 수 있으나 Session에서 BindFileSave는 아직 미연결이다. Changed의 빈 filename 예외는 원본004bd365가 filename 첫 BYTE의NUL을 정상 허용하는 것과 달라 후속 Notify 통합 검사에서 수정해야 한다.
