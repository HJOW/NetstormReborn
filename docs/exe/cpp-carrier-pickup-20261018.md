# 일반 운반체의 포획 요청과 서버 내용물 변환

2026-10-18 / HJOW-Athlon / 기준 `524cafd`. 마지막 정밀 디컴파일 PC HJOW-Athlon / 2026-10-16과 현재 PC가 일치한다. AGENTS.md와 두 인수인계 문서를 다시 읽었고 직전 단위157은 제품 변경·원본 관측·회귀·커밋을 마친 진행 턴이었다. 보호 원본·AGENTS·dotnetpj 인계·DirectDraw는 변경하지 않았다. 새 디컴파일과 원본 게임 창 실행은 없다.

`RawCarrierPickup`은 PATCH10.78의 `428e00` 요청, `426520` 서버 수신, `4264d0` 기존 내용물 제거를 복원한다. 크리스탈 크랩의 `60h` 내용물 이동과 별도다. 같은 Session의 실제 CarrierArrival 후보, RawContainedCreate, ContentFormHost, World.Destroy, 현재 입력 큐, 현재 내용물 탐색 전역을 연결했다. 새 `carrierRequest`는 실제 전송 장치만 공급하며 요청을 보낸 자리에서 서버 수신을 직접 실행하지 않는다.

`428e00`은 부모의 현재 위치로 후보를 먼저 찾고 후보0/현재boss를 검사한다. 단독 미션 `594fa4`가 거짓일 때의 사용하지 않는 genus 조회와 두번째 boss 검사도 남겼다. 현재 `40a9a0` 헤더와 부모WORD/후보WORD/현재 후보TYPE BYTE를9바이트에 기록해 서버 번호9에 보낸다. 프로토콜 bit7만 제거하며 선언 길이를 임의 보정하지 않는다. 디컴파일의 `local_c`는 헤더와 별명처럼 보이지만 실제 어셈블리는 후보SID를 **EBX**에 보관해 `packet+6`에 쓴다. 헤더 WORD를 후보SID로 쓰는 오류를 만들지 않는다.

`426520`의 순서는 다음과 같다.

1. 최초 부모와 대상을 보관하고 boss 진단20c 뒤 현재 boss를 다시 읽는다. 대상과 부모의 free|dead(3), 대상TYPE와 packet+8만 검사한다. 요청 owner를 새 소유권 관문으로 사용하지 않는다.
2. genus2000이면 오벨리스크 변환4620c0을 요청한다. 그 뒤 현재 입력 명령5429b8, 현재 대상, 같은 packet에서 다시 읽은 부모WORD로452fe0을 호출한다. 실제 생성의 부모는 최초 보관한 부모다.
3. 현재 대상genus2100이면 부모의 첫mask1 내용물을 찾아 제거한다. kind WORD는 삭제 전에 보관하지만 특정타입5412f8(초기168)은 삭제 효과 뒤 다시 읽는다. 다르면485e40 보충 부모를 호출한다.
4. 현재 QA WORD와 TYPE BYTE를 보관하고 원객체의 dead만 켠다. 같은 실제4afa70에 `(parent,kind,QA,genus100이면1 아니면20h,0,0)`을 넘긴다. 복귀 뒤 **현재 상태 BYTE에서 dead만** 지운다. 생성 효과가 free/void/contained를 바꾸어도 다른 비트를 덮지 않는다. 이를 위해 SidPool의 제한된 friend만 추가했다.
5. 기존 실제 RawContainedFinder로 TYPE6 종속을 찾는다. **현재 종속의 다음을 먼저 선택한 뒤** 현재 종속을 Unpop하고 새 ContentForm에 Attach한다. 끝에 원객체의 현재 가상 삭제를 실행한다. 전체 종속을 미리 벡터로 복사하거나 현재 객체를 옮긴 뒤 next를 읽지 않는다.

포획된 사제의 기존 기술 내용물은 새 ContentForm 자체를 부모로 가진다. 보호 PE의 실제 **512208+24=41c100 / `c2 08 00`(빈ret8)**을 직접 확인하고 RawContentParentNotice에 그 정확한 PATCH 가상 표만 추가했다. 기존 생성자 표83/68행은 바꾸지 않았다. 생성기 `decomp_content_notice_table.py`는 verify 옵션이 없어 실행하지 않았고, 읽기 전용으로 기존 두 표 전체를 각 PE와 대조했다.

독립 관측기 `tools/decomp_carrier_pickup.py`는 요청/서버/제거/헤더/TYPE getter/종속 ctor·Begin·Next·기본 true 필터의 **11개 실제 몸체**를 전체 실행한다. 실제 TYPE stride는500이다. **2657입력·5314정상반환·1195390명령·53/64 차이0**을 기록했다. 후보426900/전송/권한 진단/오벨리스크 변환/입력 큐/기존 내용물 조회/보충/실제 Content 생성/가상 삭제·Unpop·Attach는 명시 경계다. 원본을 실행한 기대값과 C++는 독립이며, fixture의 첫 C++ 재생 전 SHA를 보관한다.

현재 권한/단독 모드/헤더/명령/특정타입/contained타입, 임시dead가 보이는 생성 인자, 효과 뒤 current 타입·QA, caller packet의 부모 별명, 다음 선행 선택, free/dead와 void/contained 구분을 교차했다. 반환 이후 아홉 슬롯 전체50바이트, 현재 전역, 호출 순서·인자, 패킷을 두 정밀도에서 보관한다. GP/retN/SEH/canary/x87/명령·읽기·쓰기 범위를 검사한다. 부가적으로 기존 contained 근거2415개와 기존 부모 통지 표 PATCH83/CD68행의 읽기 전용 PE 대조도 성공했다. CD10.72의 새 포획 몸체를 이식한 것은 아니다.

실제 연결 검사는 같은1-1 지도에서 골렘의 실제 경로 도착이 `4f`를 로컬 buffered 큐에만 기록하고, 실제 LocalMissionTransport의 Flush/집합/Poll이 같은 서버 등록 수신기를 실행하도록 구성했다. 이미 연결된 단계의 pending-1→owner1과 접속 비트는 명시 입력이며 로그인 성공 전체로 계산하지 않는다. 스톰 크리스탈의 원래 QA가 실제 새 TYPE6에 보존되고 원객체는 실제 삭제된다. 입력 큐에는030000b1/대상SID/골렘SID가 들어가며 SP는 변하지 않는다.

사제 검사는 실제 생성자와 전용 SetOwner로 원래 소유자3을 초기 입력한다. 이 준비에서만 현재 전투 모드를 보관해 잠시 전투 전 상태로 지정하고, 포획 실행 전에 원래 전투 모드를 복구한다. 실제HP setter로 절반 미만의 이동 불가 상태를 만들고 기술 ContentForm을 붙인다. 실제 골렘 도착/전송/서버 수신 후 같은 기술 Form은 새 포획 ContentForm 안으로 옮겨지고, 원래 사제의 가상 삭제/등록 목록 제거가 끝난다. 현재 참여 조회도 포획 골렘을 찾으며 새 포획Form의 원래 소유자QA와 장부·삭제·전송 depth0을 확인했다. 초기 소유자·연결 상태 준비는 명시 입력이며 원본의 전체 생성/로그인 흐름을 검증한 것으로 계산하지 않는다.

처음 집중검사에서 사제 포획이 선택한 두 실제 하위 연결 누락을 발견해 해결했다. 첫째 TYPE파서49db09가 `pickupSound`의4a8ee0 결과를TYPE+7c에 쓰고42970b~429720이 현재 소리를 `(sound,0,0,0,1,0)`으로 전역 재생함을 확인했다. 같은GameAssets 속성/단일SoundList/실제장치에 연결했고 실제golemPickup.wav와6인자가 관측됐다. 폭탄TYPE+6c와 큰폭탄 효과는 기존 별도 경계로 보존한다. 정적근거는 `extracted/goal-carrier-pickup-20261018-sound-binding.json`이다.

둘째 사제의 실제 VT **50f210+10/+14/+18 = 4af780/4919b0/44b9e0**을 보호 PE에서 확인하고, 기존 RawPriestPreDestroy 래퍼와 RawDamageablePostDestroy를 같은 공통 가상 삭제 호출에 연결했다. TYPE158과 정확한PATCH VT가 함께 맞을 때만 추가 분배한다. 일반Carrier 전처리로 대체하지 않으며 실제 고유 전처리가 같은 사제 목록과 보호막을 처리한다. 근거는 `extracted/goal-carrier-pickup-20261018-priest-delete-binding.json`이다. 세번째 집중검사의 참여 조회 실패는 검사가 공통 SetOwner만 써 원래 소유자QA를0으로 남긴 준비 오류여서 위 전용 SetOwner 초기 입력으로 바로잡았다. 공개 const 바인딩에서 비const 로딩 깊이를 바꾸려던 검사 build5의 컴파일 실패도 보존했으며 제품 API를 넓히지 않았다. 네번째 집중검사는4개 모두 통과했고 독립 원문 fixture는 첫 재생 이후 바꾸지 않았다.

최종 검증은 Release build6 경고0/오류0, 집중4개, 전체 **1632개 실패0 / 213.80초**(CTest 213.83초)다. 분석 도구79개 중68개 통과·기존11개 실패·새 실패0이며 기존 실패 집합은 단위157과 같다. contained2415개와 기존 부모 통지 표 PATCH83/CD68행도 별도 통과했다. 실제 클론 창의 캠페인1-1/1-2 브리핑→월드와 tutorial1~3 브리핑의5개 진입을 통과했고 보호 originals 전체 스냅샷도 동일했다. 이 진입 확인을 새 포획의 UI 조작·자연 왕복·완주 검증으로 확대하지 않는다.

기존64보고서의191현재핀만 갱신해 unit158 이력을 추가했다. 현재 핀과 추가 이력을 역치환하면 HEAD의 과거 전체 JSON과 동일하다. 보호 PE2개/tarc/AGENTS/dotnetpj 인계의 SHA도 기준과 같다. 새 자료는 UTF8/LF이며 기존 -text 소스의 개행을 보존했다. 마지막 인덱스 대조에서 RawContentParentNotice.cpp의 작업 CRLF가 Git에서 LF로 정규화됨을 발견해 원래 HEAD와 같은 LF로 맞췄다. 컴파일 당시 바이트를 CRLF→LF만 변환한 SHA가 최종 소스 SHA와 같음을 별도 보관했고 실행 코드·검사 기대값은 동일하다. 최초 실패·수정 후 통과·독립 원본 관측·정적 연결 근거의 로그 SHA는 cpppj/campaign-carrier-pickup.json에 보관한다.

남은 경계는 **4620c0/461f00 오벨리스크 보상 변환**과 **485e40 전역 보충 부모**다. 이 두 게임 계산을 caller가 만든 결과나 빈 성공으로 바꾸지 않았다. 실제42/38/41 이동과5040c0의42ae80, AI의간헐천등록/제거·전략, 실제 Client 명령 소비자·생산·경제·AI 및11스테이지 자연 왕복/승패/저장/다음미션/완주·전체동일성도 미완료다. 이번 단위 결과를 사용자 UI 전체와 자연 완주 검증으로 확대하지 않는다.
