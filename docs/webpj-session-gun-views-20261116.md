# Session 총 검색·에너지·커서 표시 (WB04-E580)

`225553df` RawCampaignSession.cpp1318..1339를 E574~E579의 같은 기반 owner에 연결했다.
GunMode→GunTarget→EnergySupply→CursorMana→CursorRange의 실제 생성 순서다.
전체 Session은 계속 **partial**이며 피해/나머지 생성자·외곽 공개·Load/로그인/프레임은
잔여다. 이 구간은 전체 커서 또는 Session Runtime을 공개하지 않는다.

## 연결 범위

`connectGunAndCursorViews()`는 TYPE 파생 표 뒤 한 번 실행한다. 실제 GunMode는 같은
풀/FrameCodeTable과 현재 GunState.WindArcherType을 읽는다. 독립 typed 분기의 기존
타입 칸을 사용하며 원문1051..1062의 Native 장기 상태 대여를 앞당기지 않는다.
GunTarget은 같은 해시/TYPE/Gun 파생 행/SHP·앞서 연결한 현재 동맹/Player 상태를 빌린다.

에너지는 실제 `bookkeeping.currentProviders()`와 같은 Client 공급 장부를 사용한다.
빌린 장부 대신 비어 있는 지역 providers 필드를 읽지 않는다. 선택 목록은 원문492의
독립30DWORD/개수이며 Builder 등 후속 소비자가 같은 목록을 빌릴 수 있다.
동맹은 같은81DWORD다. 원소 요구는 보호116정의와 현재188TYPE을 재조회하며 공급 가상
주소는 기존 실제 EnergyProviderEntry 표로 분배한다. editor/special/mission은 현재 게임/
Player 전역 getter로 읽고 나머지 값은 같은 Client EnergySupplyState를 빌린다.

기반 자료에 원문489/653의 CursorManaGlobals/CursorRangeGlobals를 구성했다. mapLimit과
radiusLimit은 같은 장기 WorldCells/ManaRadius 상자의 읽기/쓰기 접근자다. 원문 C++ 참조처럼
양쪽 변경을 즉시 보며 별도 좌표/반경 사본을 유지하지 않는다. Range는 같은 Mana 좌표와
GunTarget 상태를 빌린다. 해당 상자는 기존 정수/float32 저장 폭을 적용한다.

마나·사정거리의 World 효과는 기존 BindCursorManaWorld로 실제 Create/WORD/Pop/
Unpop/Repop/Destroy에 연결한다. 중심/발자국·방향/모드·첫 내용물은 실제 GunTarget/
GunMode/사제 Carrier에서 조회한다. 현재 사정거리는 같은 Gun 행, 발사구는 보호 SHP의
signed WORD 두 개, Run은 같은 A..P FrameCodeTable을 사용한다. 제공 진단은 값형으로
보존하며 누락을 빈 성공 장치로 바꾸지 않는다.

반납은 현재 World 프로세스 정리→이 연결 단계 무효화→앞선 몸체/Player/사제 정리 순서다.
표시 논리는 실제 프로세스 소멸까지 살아 있다. 부분 구성 실패는 앞선 실제 Gun을 남기고
뒤의 에너지/표시 생성을 완료로 표시하지 않으며 반복을 거부한다.

## 근거와 검사

Session과 RawGunMode/RawGunTarget/RawEnergySupply/RawCursorMana/RawCursorRange의
소스/헤더는 기준선 이후 HEAD 변경0이다. cpppj 수정은 없다. CampaignSessionTests의
실제 커서 표시/공급 장부·30칸 선택/동맹 방향·공유 좌표 계약을 읽었다. 기존 원본 Energy/
Gun/마나/사정거리 fixture 검사는 전체 회귀에서 유지한다. 새 원본 관측/fixture는 없고
manifest803항목을 유지한다.

신규12검사는 실제 보호 자산·World/Core·Client 전역·Host에서 다음을 확인한다.

- 같은 좌표/반경 상자의 양방향 변경과30칸0초기화, 앞 단계/반복/반납/미공개 관문.
- 현재 모드 타입·프레임·같은 Gun 방향 표/중심과 썬 캐논의 고정 프레임 규칙.
- 실제 장기 공급 장부/선택 배열·TakeSource의 개수/꼬리와 방향별 동맹.
- 현재 editor/special/mission·같은 제곱 반경과 현재 TYPE 테마/레벨의 요구 문자열.
- 실제 마나8표시·Host 등록/현재 시각/반경·프레임 이동/축소 종료.
- 실제 원형 사정거리8표시·같은 현재 범위/Run/Host와 축소 종료.
- 실제 썬 캐논 직선 표시의 모드/방향·SHP 포구·프레임 이동/종료.
- SHP signed 좌표·같은 Run·자산/번호 범위의 명시 오류.
- 제공 진단 함수의 값 복사와 실제 World WORD/위치/공간/삭제 효과.
- 에너지 생성 실패 앞의 실제 Gun 접두·반복 거부·전역/Player 대여 반납.
- 살아 있는 실제 마나/사정거리 프로세스를 부모 종료 시 논리 반납 전에 정리함.

공급 검사는 실제 TYPE76을 생성하고 같은 공급 장부에 시험 입력으로 등록한다. 아직 연결하지
않은 자산 PostPop/건설/Builder 부모가 동작했다고 세지 않는다. 커서 검사는 실제 논리/Host
생성 함수를 직접 선택하며 후속 Session.BindCursorMana/Range나 앱 수명 연결 증거가 아니다.
내용물 기반 사정거리의 첫 내용물 조회는 연결했으나 이번 새 검사에서 해당 파생을 실행하지
않았다. 기존 Carrier/Range 단위 검사는 유지한다.

최초10검사는7통과/3실패였다. 시험이 썬 캐논의 고정 방향 프레임을 일반 Run과 혼동했고
종료 통지 직후 원형 표시가 즉시 삭제된다고 가정했다. 실제 고정 프레임 규칙과 시간 진행/
축소/다음 프레임 삭제 계약으로 시험을 교정했다. 제품 계산은 변경하지 않았다.
직선 표시/부모 종료 검사를 더한 최종 집중85검사는 Node22·Node26 모두 통과했다.
신규 모듈 줄100%·분기100%·함수96%다. 로그 `extracted/wb-e580-focus3-node22.log`·
`extracted/wb-e580-focus3-node26.log`는 Git 제외다.

전체 Node22(v22.23.3)·Node26(v26.13.0)은 각각5,516건 통과, 실패·취소·건너뜀0이다.
로그 `extracted/wb-e580-node22.log`·`extracted/wb-e580-node26.log`는 Git 제외다.
대응표 검사1,160행/오류0(`extracted/wb-e580-port-map-pre.log`)이다. port841·partial34·todo131·제외154,
session/oracle0·단계28/43은 유지한다. 원본/클론 게임·브라우저·오라클·디컴파일·공중3종
전용 생성/행동·새 원격 멀티플레이 실행은 하지 않았다.

## 다음 시작점

1340의 피해 상태 getter·AI 피격 통지와 Hit 효과부터 원문 생성 순서로 이어간다.
해당 구간에 필요한 원문666의 GunImpactState는 같은 loginEnd double 상자를 빌려
구성해야 한다. 현재 Hit 효과/이 상태의 연결은 이번 완료 범위에 포함하지 않았다.
나머지 실제 모듈/등록·외곽 Session 공개·같은 Template TypeNames→Objects/Contents·Load·
로그인·프레임/종료가 잔여다. L2/session-dump·실제 미션 화면/자연 플레이는 미검증이다.
