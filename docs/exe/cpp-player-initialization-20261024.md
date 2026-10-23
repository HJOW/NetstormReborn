# Player 전체 재할당과 Core 해제 전후 수명

2026-10-24 단위390 기록이다. 기준은 **65bc3e0c / 20261024-12 CP**이며 현재 호스트와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. AGENTS.md와 공통/닷넷 인수인계를 다시 확인했다. 기존 정밀 산출물을 읽었으며 새 디컴파일이나 보호 원문 변경은 없다.

## 원본 전체 부모

현재 함수표의 PATCH004910a0은 **236BYTE**(`4910a0-491117;491120-491193`)다. 첫 Player base를 두 카운터 초기화보다 먼저 캡처한다. 현재 base가 있었으면 각행 +0의 factory 토큰을 **NULL도 포함해9회** CRT004e4354에 전달한다. 각 free 뒤 다음 토큰은 현재 전역base에서 다시 읽는다. 마지막에는 다시 읽은 현재 base를0043fa90에 반납한 뒤 전역에NULL을 쓴다.

그다음0043fa80으로1620BYTE를 할당하고 전역에 반환 주소를 기록한다.405DWORD의0쓰기는 현재 전역을 반복 읽지 않고 **malloc 반환 때 캡처한 주소**에 수행한다. NULL malloc의 첫 쓰기는 원문 물리 오류이며 별도 성공값이나 진단 후 계속을 만들지 않는다.

각 Player0..8에서 현재base+64와+58에0을 별도로 쓴다. 그 뒤 캡처한 행 주소를 유지한 채 operator new004e4391(120)를 호출하고, **캡처행**의 +0포인터/+4용량30/+8개수0/+2c Subscriber0/+78첫지역127을 기록한다. 할당 복귀 뒤 **현재 전역base**의+54에 자기bit를 쓰며 동맹 인자도 그때 다시 읽는다.00490ea0은 실제 기존 방향별 동맹/조건부 Chat 몸체다.

초기화 부모 자체는 현재 Core 포인터·활성 DWORD·known을 정리하지 않는다. 그 일을 임의로 부모 앞에 추가하면 원문 State 정리의 Select0/cache 위치가 사라진다. 현재 Session의 수동 `ownerFactories.resize(30)`와 일부 필드 쓰기를 이 전체 부모 완료로 취급하지 않는다.

CD10.72 SourceMap 후보는 **00406200**이다. 같은 본문/동작으로 확정하지 않았고 새 CD 실행도 없다.

## 제품 전 독립 관측

[새 Native 도구](../../tools/decomp_player_initialization.py)와 [최초 보고서](../../cpppj/player-initialization-native-20261024.json), [fixture](../../cpppj/tests/fixtures/player-initialization-x86.tsv)에 **101입력×두 x87 정밀도=202관측**을 제품 작성 전에 고정했다. 정상174, NULL 물리 오류8, 선택 callee 진입 중단20이다. 그때의 `productWritten=false`는 최초 시점의 기록으로 유지한다.

부모236BYTE와 실제 동맹491BYTE를 실행하며, Known 비0에서는 기존 실제 Chat Add/Clear·이름정제/빈 Refresh 몸체까지 수행한다. CRT malloc/free/new와 기존 문자열 CRT만 명시 장치다. Player 초기화/동맹 계산을 상수 성공이나 C++ 결과로 대체하지 않았다. 기존 할당의 VM 물리는 사후 비교를 위해 보존하므로 실제 Windows heap 해제 성공을 입증한 실험은 아니다.

입력은 기존 base NULL/A/B, 같은/다른 주소 재할당, malloc/new NULL, free(NULL), Known0/1·숨김 입력, free/new/동맹의 현재base 변경, new 뒤 필드 보존, 실제 부모 재진입과 각 선택 중단이다. 세1620BYTE Player 물리·81DWORD 동맹·4800BYTE Chat·실제 자식 진입과 부모 base read/write 순서, 현재 Core/known/카운터 전역을 모두 보존했다. 정상 반환의 GP·ESP·제어어도 확인했다.

## 실제 C++ 부모와 검증

`RawPlayerInitialization`은 현재 두 카운터와 base getter/setter, 주소를 실제 물리로 해석하는 장치, 네 할당/반납 경계 및 기존 동맹 자식을 받는다.32비트 주소는 같은 소유자가 발급한 물리 식별자이며64비트 host 포인터를 잘라 쓰지 않는다. 메모리/getter/setter는 순수 실제 저장소 장치이고 누락/짧은 물리는 도달한 위치에서만 실패한다. 앞선 부분효과는 되돌리지 않는다.

첫 새 검사는202관측의 전체 Player·동맹·Chat·전역·효과 trace를 각각 Native 기대에 대조했다. 실제 C++ 동맹/Chat 자식도 기존 `RawPlayerParticipation`·`RawChatChannels`를 같은 입력에 사용한다. 둘째는 선택하지 않은 free 장치 부재와 선택된 malloc 누락/짧은 DWORD store의 부분효과를 구별한다. 셋째는 실제 `ClientCoreDataGlobals`의 다음 수명을 검증한다.

1. 같은 이전 Player+6c를 선택하고 원문 호출자가 known0→SelectCore(0,1)을 수행한다.
2. 4·4·2 cache 복사/현재NULL/active0을 완료한 뒤에만 이전 Player 반납 장치가 실행된다.
3. 새 base 기록을 같은 Core의 Player뷰 슬롯에 제공하되 초기화 부모는 current를 임의 선택하지 않는다.
4. 부모 반환 뒤 호출자가 선택한 SelectCore(1,1)은 새 Player+6c만 빌리고 이전 cache는 보존한다.

configure1/build1은 종료0·관측 경고오류0이다. 신규3검사는202관측/물리계약/실제 Core 수명까지 첫 실행에서 모두 통과했다. 관련 기존 CoreData3·ClientCoreRecords2·ClientChatCoreOwners3도 **8검사/실패0**이다. 전체 CTest1도 **2658검사/실패0·종료0**이다. 검사305.37초/전체305.40초, wrapper의 CTest 실행 측정305.48초이며 원본 SHA는 측정 전후 별도로 읽었다. 내부 전체 출력도 사본으로 보존했다. 원시 경로는 `extracted/cpp-player-initialize-390-*`다.

최종 제품SHA는 `908424cd619d0b0e9baddde0d222fcb1bbd28e088276ce45d6b41e376f050fe4`, 검사SHA는 `c70b3585c567fa418c8030848b405ba98ecad5439b486242f76cce9dba428763`이며 검사 전후 동일하다. 원본1394/1394동일, AGENTS.md·원본/CD·닷넷/웹의 추적 변경0을 확인했다. [새 제품 보고서](../../cpppj/player-initialization-runtime-20261024.json)에 소스14개/증거9개 SHA와 두 Native 정밀도101쌍의 결과 동일성을 기록했다. 최초 Native 보고서·도구·fixture3파일은 `.gitattributes`의 `-text`로 보존한다.

엄격 UTF-8·AST·로컬 링크·Native SHA 및 작업본/인덱스의 원시 바이트·stage 집합/공백을 확인해 `20261024-13 CP`로 로컬 커밋한다. 원격 push는 하지 않는다.

## 실제 Client/Session에 남은 연결

이 단위는 전체 초기화 부모와 기존 Core 소유자 사이의 수명 근거를 확보한 것이며 **실제 State 정리/Client Player 소유자/Session 연결은 아직 없다**. 새 GUI 실행도0이다. 이전 제품의 자연 플레이·34진입을 새 제품 관측으로 합산하지 않는다.

다음에는 실제 Client 수명의 단일 Player/factory 할당 소유자를 마련하고,9개 factory 목록을 실제 Player +0/+4/+8 및 기존 bookkeeping 소비자와 연결해야 한다. 현재 여러 모듈은 생성 때 고정 span을 빌리므로 재할당 후 옛 span이 남지 않게 wrapper 재구성 또는 정확한 현재base 조회 경계를 정해야 한다. 기존 Session destructor/ClearWorld 앞에서 임의 새vector·Core reset·양쪽 구조체0을 넣어 원문 순서를 가리지 않는다. 앞선 PlayerLinks/Kernel/Gump/Core0/Pool 순서와 SP/모드·선택적Core1도 같은 실제 State 부모에 연결해야 한다.

전체 Edit/NewFort·편집 도구·저장/언어별 시나리오·재로드, Early/전체 캠페인/Demo 사용자 완주와 픽셀·음향·규칙 완전 동일성은 계속 남으며 목표는 활성 상태다.
