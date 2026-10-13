# 실제 미션 저장 월드 부모의 독립 복원 초안

현재/마지막 정밀 PC는 **HJOW-Athlon, 2026-10-14**다. 단위15 신규 `RawCampaignWorldLoad`는 실제 War Begins 새로그인 부모 **PATCH004b5300/CD004322e0**이며, 기존 Fort 메뉴 부모004b5f70/CD004330c0의534 관찰과 분리한다. 기존13/14 동결 소스·원본·CMake·LEFT는 편집하지 않는다.

## 원본 전체 순서

1. mySubscriber←loginSubscriber, 현재Template header+8 DWORD=1.
2. 번역키`You`의 결과를 현재mySubHandle에 NUL까지 복사.
3. config `fort.mySubHandle` 갱신→Fort specification 재조회.
4. **현재battleOwner**의 실제 Player152 생성.
5. Builder ctor/같은ChunkMap attach(owner1,bounds1..15).
6. editor이면 현재battleOwner의 Player+78/CD+70=0.
7. zero120→**현재Template owner** Territory→실제PlaceAll(0,20).
8. 같은Builder/current noRegion으로 일반지형 생성.
9. **현재Template owner**의 ObjectLoad(0,20).
10. Graph(false)→923e0/CD40daf0→Camera reset.
11. **현재battleOwner** 이름+0xc에 현재mySubHandle NUL까지 복사.

Money/Badges/Technology 호출은 이 부모에 없다. 미션SP/myTech는 stage3 별도 몸체다. 각 효과 뒤 current owner/전역을 재조회하며 Template owner·battleOwner·Builder owner1을 합치지 않는다.

## 현재 API와 미복원 경계

`RawCampaignWorldLoad`는 같은 ChunkMap/Runtime3204BYTE/frames/puzzlePiece/noRegion을 참조한다. 기존 `RawFortWorldLoad::ReadTerritory`만 재사용하고 generic 부모의 Load를 호출하지 않는다. 기존 `RawPlayerWorld`의 실제 Create와 같은 Records/State를 사용한다.

필수 hooks는 번역결과, config setter, specification refresh, Builder 진단, 실제generate, 현재Template 선택, 실제ObjectLoad, Graph(false), Camera다. 선택된 경계를 빈 callback/기본성공으로 채우지 않는다. 번역은 좁은문자 NUL 포함 실제BYTE이며 물리범위 밖 복사 입력은 명시 거부한다.

후처리923e0/CD40daf0는 실제미션mode594fa4/CD52e170=1에서 두큰분기가 모두false여서 자연 반환한다. 현재 `FinishMissionPriests()`는 이 선택범위를 복원하고 비미션 Technology/Priest 초기화 요청은 그 시점에 명시 거부한다. 실제 parent 전에 mode를1로 설정하는 상위Mission ctor/Ready/login 자체는 이번 복원수에 넣지 않는다.

## 독립 PE 관찰

`campaign-world-load`의 최초 부모+login감사2/2 export와 별도 `campaign-world-post`1/1 export를 보존한다. `decomp_campaign_worldload_oracle.py`는 부모 prologue/tail·기존Builder/Territory/Runtime/ChunkMap과 **실제mission1 후처리 자연 반환**을 실행한다.

독립 입력은 **130=정상94/assert36**, 각판본65=정상47/assert18이다. 두owner/editor/활성120BYTE·stream현재pos/end/depth·하위효과 뒤 현재전역 변화·NUL 이름을 교차한다. 두x87 정상반환260은 반복이며 고유수에 더하지 않는다. 예비14×두판본 입력도 새로 합산하지 않는다.

번역/config/spec/Player 생성/지형/Objects/Graph/Camera는 명시 하위콜 경계다. Native에서 이 경계의 완료를 주장하지 않는다. Player 생성 경계는9기록에 가짜SID를 쓰지 않으며 실제 Player 몸체/Host는 별도14 자료다. Native는 현재120BYTE·전체Chunk3072/Island3204·Template180/현재Player9/handle64·원본순서를 관찰한다.

두 보호archive의 `\d\thewarbegins.fort`에는 각각 저장객체172개가 있다. 실제Territory120을 사용하는 별도profile3 입력을 포함한다. CD에 없는 patch전용 `.type`를 가져오는 대신 CD에 실제존재하는 타입 및 원래TypeNames 해시를 해석해 현재 저장 필드를 읽는다. 이 구조 파서는172 수/입력자료 확인이며 원본 ObjectLoad 전체 실행으로 세지 않는다.

**새 C++ 전수/실제173 설치 소비자는 아직 미수행**이다. `CampaignWorldLoadTests.cpp`에 양판본전수2검사 초안과 `NETSTORM_CAMPAIGN_WORLDLOAD_FIXTURE` 매크로를 준비했으며 미등록/미컴파일이다. 하위Player·지형·Objects·Graph·UI는 요청관찰 경계이고 실제Player Pop/최초Regular는 별도14 자료다. C++ 공개콜백으로 보이지 않는 pure ctor/attach/PlaceAll/mission finish의 내부진입은 Native 전체계수에 그대로 보존하고 CPP는 전체현재BYTE와 공개하위요청을 대조한다. 실제Builder bounds/지도/3204BYTE는 같은 몸체가 생성한다.

해당 부모 관찰은 전체게임/하위생성/Ready/GUI/네트워크/AI 완료 근거가 아니다. RawCampaignWorldRuntime의 같은 Pool에 Player152+저장172 및 부수 지형/Forms/Regular를 설치하는 소유 연결은 root가 단위15에 전담한다. 실제전체raw/장부/두난수/예약의 독립 대조를 별도로 확보해야 한다. 사용자중단 지시로 현재15 준비자료를 동결했으며 다음 재개에서 등록/전수/실제소비자를 이어간다.
