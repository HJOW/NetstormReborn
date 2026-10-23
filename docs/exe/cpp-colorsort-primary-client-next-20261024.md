# 같은 raw primary를 Client와 명도 소비자에 연결하는 후속 설계

단위388 후속 설계이며 **제품/사본 패치는 아직 적용하거나 작성하지 않았다.** [독립439de0 모듈과 최초80관측](cpp-colorsort-primary-native-20261024.md)의 동결7파일을 바꾸지 않는다. root387의 Cursor/SaveOptions 접합과 별개다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon /2026-10-23이다.

현재 실제 연결 위치는 `Client::Run`의 첫 LoadPalette 뒤 `colorSortInput_->Capture(screen_->LogicalPalette())`다. ClientColorSortInput은 uint32 최초 logical과9표만 소유하며 아직 raw primary를 갖지 않는다. Scene LoadPalette의 `RebuildColorSortMaps`, Screen 재생성의 `BindColorSortInput`, Session 교체는 이 최초 입력 소유자를 재생성하지 않는다. 이 수명 구분을 그대로 유지한다.

다음 최소 API는 ClientColorSortInput 안의 **shared RawColorSortPrimary 한 개**다. 제안 이름은 `CaptureWithPrimary(logical, allocationBytes)`와 `Primary()`이며 기존 `Capture(logical)`은 독립 검사 adapter로 남긴다. 전자는 필수 실제0x3800 초기 물리로 raw owner를 만들고 같은 입력의 Construct가 정상 완료된 뒤 소유 슬롯에 연결한다. Primary getter는 누락/미완성을 명시 거부한다. Logical에서 즉석으로 primary를 만들거나 COL을 대신 읽는 fallback은 넣지 않는다. 실제 생성 시의 재진입·실패/기존 슬롯 반납 순서는 원래43b430 전체 부모와 구별해 후속 patch에서 명시해야 한다.

**필수 장치 의존성은 초기 allocationBytes 공급**이다. `CampaignClientServices.cpp`의 기존 `CurrentAllocationBytes`는 현재 malloc의 실제 바이트를 복사하고 free하지만 익명 namespace 안에 있어 Client 시작에서 직접 재사용할 수 없다. 새 startup 사본은 이 실제 장치의 공개/공유 범위를 결정하거나, 새 RawColorSortPrimary payload의 초기 물리를 읽는 명시 Windows 장치를 공급해야 한다. 임의0/고정 패턴이나 현재 COL에서 만들어 준 값으로 대체하면 안 된다. padding은 기능상 미작성이어도 최초 물리의 일부다. 이는 생성자 입력 장치이며43b430의 두 슬롯 free/0·검색객체·qsort·gradient가 완료됐다는 뜻은 아니다.

소비자 patch는 다음처럼 나누면 기존 근거를 유지할 수 있다.

- `OwnerColorMaps.h/.cpp`: `BuildSpecialOwnerColorMaps(const RawColorSortPrimary&, type, candidates)`를 추가한다. 소스와8후보의 명도는 같은 owner의 `Lightness(index)`에서 읽고 RGB→명도 재계산을 하지 않는다. 기존 logical overload는 HSL15관측/독립 adapter로 유지한다. 후보 순서·엄격 비교·보정·항등 행·소스 목록은 기존 구현을 공유한다.
- `RawCampaignSession.cpp`: 실제 services.colorSortInput이 있으면 그 같은 `Primary()`를 전용5종 lazy 표에 사용한다. 일부 장치 누락 때 파일 COL로 성공하지 않는다. owner가 없는 기존 headless adapter만 기존 logical/COL 경로로 구별한다. 일반 정적 색표와 표0/ThemeMode/등록 수명까지 임의 변경하지 않는다.
- `ClientColorSortInput.h/.cpp`와 첫 Client 시작 사본: 같은 raw 소유/필수 할당 입력만 연결한다. 팔레트 변경·9표 Rebuild·해상도/Session 교체 때 Construct/Capture를 다시 호출하지 않는다.

이미 복원한9표 함수가 logical에서 RGB를 계산하는 부분까지 한 번에 없애면15개 소비자 전체가 다시 바뀐다. 우선 raw owner와 Building의 **저장 double 소비**를 독립 연결하고 기존6/15/130 fixture를 그대로 검증한다. 이후 RGB/명도/색검색의 현재 raw view API로 기존 표 계산을 바꾸는 작업은 별도 대조가 필요하다. hue/saturation의037f 차이는 보존하며53비트 지원만 먼저 주장한다. 원문037f 명도가 같았어도 이후9번째 표의 차이를 삭제하지 않는다.

검증은 실제 Client가 생성한 같은 raw 주소/전체0x3800·Logical 주소/값을 두 palette 변경과 Screen/Session 교체 뒤 비교해야 한다. Owner HSL15 최초 관측을 새 raw overload에 재생하고, 실제 Session Capture의 전용5종 픽셀/출력도 같은 저장 명도로 확인해야 한다. 미캡처/미연결 Primary는 실패해야 한다. 기존 독립 adapter와 일반 색표 결과도 보존한다. 이 문서는 그 검증을 수행했다는 기록이 아니며 공용 빌드/GUI/원본 파일 쓰기는0이다.
