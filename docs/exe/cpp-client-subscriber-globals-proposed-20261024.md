# Client 두 Subscriber 수명 접합 제안

호스트와 마지막 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 같다. 이 자료는 단위386 **d207940c**의 정확한 git blob 제품9개와 기존 검사1개를 기준으로 만든 **387 이후 적용용 미적용 사본**이다. 공유 제품·CMake·LEFT·git·공용 build/lib·GUI와 보호 원본은 변경하지 않았다.

## 원본 저장소와 실제 쓰기

보호 PE의 초기 DWORD는 **My 00540c74=ffffffff, Login 0054265c=1, Editor 005c85a4=0, missionPost 00594fa4=0**이다. 같은 네 값을 인접16BYTE 배열이라고 해석하지 않는다. 아래 주소는 실제 PE의 명시 메모리 쓰기이며, 전체 부모의 진입 관문·하위 성공을 독립 실행으로 증명했다는 의미가 아니다.

| 부모 | 확인한 순서·경계 |
|---|---|
| 00482bf0 | base Mission Init의 missionPost=1, 실제 store 00482c0c |
| 004b2df0 | cleanup에서 missionPost=현재 missionWorld(004b322e), 뒤 Editor=0(004b3246). My/Login의 직접 초기화 쓰기는 없다. |
| 004b32f0 | 현재 Template 존재 및 fort 관문 뒤 선택 handle/config/title, My=인자(004b3353), Template subscriber/Notify 순서 |
| 004b38d0 | My=−1일 때 등록값 ReadInt에 **같은 My 주소**를 주며 미등록이면 Generate 결과를 My에 쓴다(004b38f9). |
| 004b3920 | My=−1(004b392e) 먼저, Unnamed 번역/handle/config/title/등록 부모, 뒤 Fort/Player 작업. cleanup 부모와 구별한다. |
| 004b5300 | **현재 Login→My(004b5315)**, 현재 header+8=1, You/handle/config/title, Player와 저장 월드 순서 |
| 004b6dd0 | 선택 Editor에서 cleanup→0042e440→004e22c0→fort1 뒤 **Editor1(004b71cb)→missionPost1(004b71d1)**. 두 모드는 이 제안에서 Session별이다. |
| 004b8ac0 | 패킷+8→My(004b8ad9) |
| 004b8cb0 | 선택 savedSubscriber→My(004b8efd) |
| 004c3290 | **fileName 설정 callback 뒤 현재 Login→My(004c335c)**, loadFort 조회/예약/Header 순서 |

정밀 함수 범위의 명시 참조는298건, 직접 쓰기는12곳이다. Login0054265c의 명시 참조는 **004b5300·004c3355의 두 읽기**이며 직접 쓰기/즉시 주소 전달은 발견되지 않았다. 이 검색으로 모든 가능한 간접 alias 쓰기까지 없음을 주장하지 않는다. 정적 내역은 [v2 감사](../../extracted/cpp-client-subscriber-globals-20261024-static-v2.json)에 있다.

기존 월드 모듈의 CD 대응 저장소는 My0050f6cc/Login00511c7c, Editor00518904/missionPost0052e170이고 월드 부모는 PATCH004b5300/CD004322e0으로 기록되어 있다. 이번 추가 명령 관측은 **10.78만** 실행했다. CD 전체 수명/쓰기 동등성을 새로 검증한 자료가 아니다.

## 최초 관측과 보존한 실패

첫 [v1 도구](../../tools/decomp_subscriber_lifetime_edges.py)는 `.text` 선형 디코드가 중간 자료에서 끝나 **0 store/0 관측/0명령**을 기록했다. 결과와 tool/fixture/report를 보존하며 유효한 Native 증거로 세지 않는다. v2는 확정 함수 범위로 디코드하며 선행 source load가 확인되지 않은 두 preflight 실패 로그를 별도로 보존했다. 그 뒤 제안 제품 작성 전에 [v2 도구](../../tools/decomp_subscriber_lifetime_edges_v2.py)로 **12곳×5입력×2정밀도=120관측/190명령**을 고정했다. 원시 자료와 SHA는 [v2 보고서](../../cpppj/subscriber-lifetime-edges-v2-native.json)에 있다.

이 Native 범위는 **실제 store에 이르는 최소 명령 구간의 checkpoint**다. World는6명령, Tutorial은3명령, 나머지 선택 store는1명령이며 부모 전체 반환·분기 도달·원문 초기화 전체가 아니다. 동결된 v2 클래스 설명에 남은 “1 또는2명령” 표현은 이 문서에서 정정한다. 실제 각 행의 instructionCounter/ESP delta가 근거다. FPCW027f/037f의 같은 입력60쌍은 출력/trace/명령수/ESP 차이가 없다. 두 입력값/기존 Native 자료를 후속 제안에 맞춰 재생성하지 않았다.

## 사본의 수명·복사 계약

[제안 사본](../../extracted/cpp-client-subscriber-globals-20261024-proposed/copy/cpppj/src/client/ClientSubscriberGlobals.h)은 **ClientSubscriberGlobals**의 실제 DWORD 두 개만 소유한다. 새 Client에서만 −1/1을 구성한다. `Client::SubscriberGlobals()`는 같은 shared_ptr 슬롯을, `Client::CurrentSubscriber()`는 같은 My의 **const uint32_t&**를 반환한다. 메뉴/월드가 없을 때0을 반환하던 별도 캐시 경로를 실제 저장소 조회로 교체한다. Chat prefix constref가 임시 반환값에 매달리지 않게 한다.

UberGump의 기존 `initLoginSubscriber_`/`initialSubscriber_` 이름은 호환성을 위해 유지하지만 각각 같은 My writable ref/Login constref를 빌린다. 실제 Tutorial Init의 `TutorialInitGlobals`도 그 두 주소를 빌린다. `MakeCampaignClientServices`가 같은 owner를 전달하고 Session의 `CampaignWorldGlobals::BorrowSubscribers`가 같은 두 주소를 직접 구성한다. services는 globals보다 먼저 선언되어 owner가 alias보다 오래 산다. Client owner도 메뉴/Session 소비자보다 먼저 생성되고 나중에 반납된다.

`CampaignWorldGlobals`의 기존 public 필드 이름은 유지한다. 기본/`{my,login,editor,post}` 값 생성은 자기 fallback을 소유하며 기존 기본 **0/0/0/true**를 유지한다. 명시 Borrow 생성은 외부 DWORD에 **어떤 초기화 쓰기도 하지 않는다**. 복사/이동 생성은 현재 네 값을 자기 fallback에 복사하여 외부 alias를 암묵 전파하지 않는다. 대입은 source 네 값을 먼저 캡처한 뒤 목적지의 기존 binding에 쓴다. 교차 alias에서도 두 값이 바뀌는 순서가 source 관측을 훼손하지 않는다. **Editor/post는 각 Session의 값**이며 새 Campaign에 Editor1이 남지 않는다.

현재 제품/검사의 `CampaignWorldGlobals` 사용은 Session/두 독립 장치의 기본 변수 및 runtime/load의 참조 인자·참조 멤버다. 기존 aggregate 부분 초기화/복사·이동·대입의 명시 소비, sizeof/원시 직렬화/비트 복사 소비는 발견되지 않았다. [사용 감사](../../extracted/cpp-client-subscriber-globals-20261024-proposed/usage-audit.json)에 검색한 모든 명시 사용을 남긴다. 새 구조는 aggregate/trivial/raw layout 계약이 달라지므로 **관련 전체 번역단위의 재빌드가 필요**하며 외부 바이너리 ABI 호환을 주장하지 않는다.

bound `Login::Begin`은 기존 `missionSubscriber` 값사본의 Login/My 재복사를 생략한다. 원문은 Init의 위 위치와 뒤 World의 현재 Login 복사이며, 새 Session 생성 또는 Begin 입구에서 임의 재초기화하는 쓰기는 확인되지 않았다. **owner 없는 독립 Session만** 기존 `missionSubscriber.value_or(1)` 동작을 유지한다. 실제 Zone/패킷/Fort 등록/World가 이후 같은 My에 쓰는 동작은 제거하지 않았다.

현재 비-guided `LoadMissionWorld`의 기존 `initLoginSubscriber_=1`은 보존하며 이제 같은 My를 쓴다. 원문 base Mission Init00482bf0의 post1과 이 제품 hard1의 시점 동등성은 확인되지 않았다. 이 제안은 해당 기존 어댑터 차이를 숨기거나 원본 Init 전체 복원으로 세지 않는다. common Client Chat 초기화/registry, Core 수명 정리, 전체 StateCleanup 접합도 이 사본이 완료한 범위가 아니다. 전체 `CampaignWorldGlobals`를 공유하거나 전환 때 임의 reset으로 미연결 cleanup을 가리지 않는다.

## 검증 및 적용 순서

순수 검사6개는 PE 초기값/같은 주소, 독립 기본·부분 중괄호 생성, Borrow 무쓰기와 모드 분리, 복사·이동·교차 alias 대입, 실제 Chat constref 타입의 owner 수명, **실제 TutorialInit.cpp의 설정 callback 뒤 현재 Login 읽기**를 검사한다. 마지막 검사는 최초120행 중 Tutorial10행을 원문 DWORD 기대와 대조하며 전체120행을 분류한다. 실제 코드의 순수 `ConfigParseLong` 한 함수만 같은 몸체로 추출하고 `OriginalText.cpp`/`TutorialInit.cpp`를 직접 컴파일한다. 전체 Config/게임 장치의 성공 stub과 공용 lib를 사용하지 않는다.

실제 Client/Session 검사3개는 **별도 적용후용**이며 여기서는 /Zs만 수행한다. Run 없는 실제 Client의 같은 참조/새 Client 초기값, 실제 Editor·Campaign·독립 Session 주소와 모드 분리, Login Begin의 첫 `gameServerPort` 설정 조회에서 명시 중단하여 bound의 현재 값 보존/독립 fallback7을 확인하도록 준비했다. UberGump를 headless로 생성한 검사나 실제 로그인 전체 성공 검사는 아니다. 최초 검사 초안의 `missionFortImage`에 vector를 직접 대입한 문법 오류는 **shared_ptr<const vector> 구성**으로 교정했고 최초 입력/로그를 보존했다. 격리 첫 link 명령의 fixture macro 인용 오류도 build1 로그로 보존하고 제품 코드 변경 없이 명령 인용만 교정했다.

최종 **순수6검사/실패0, build2 종료0·/W4 /WX 경고0**, 사본 제품4개·새 검사2개 /Zs 종료0이다. ClientMain의 직접 /Zs에는 기존 `getenv`1곳/`fopen`3곳의 **C4996 경고4개**가 남는다. 공용 CMake의 `_CRT_SECURE_NO_WARNINGS`를 직접 명령에 공급하지 않은 범위이며 해당 기존 호출은 교정하지 않았다. 다른 최종 /Zs6개와 순수 link에는 경고가 없다. 기존 ChatChannels 검사도 실제 Client 초기0 기대 두 곳을 원본 −1로, 현재자기 입력을 같은 getter로 교정했다. 이 사본의 첫 /Zs와 첫 재시도는 기존 fixture macro 미공급으로 실패한 로그를 보존하고 최종 명령에 기존 macro를 공급하여 **/Zs 종료0**을 확인했다. 제품 본문을 바꿔 해당 오류를 숨기지 않았다. 실제 Client/Session3 및 기존 ChatChannels의 실행 결과는 아직 없다.

최종 격리 종료값/경고·원문 검증·patch SHA는 [검증 JSON](../../extracted/cpp-client-subscriber-globals-20261024-proposed/verification-final.json)과 [빌드 JSON](../../extracted/cpp-client-subscriber-globals-20261024-proposed/verification-build.json)에 기록한다. 동료 theme_technology는 owner/Borrow/copy·move/대입/참조 수명을 읽기 검토하여 명백한 문제 없음을 확인했으며 추가 실행은 하지 않았다.

적용은 **386 d207940c → root의387 적용분 → 이 제안의 문맥 수동 합성** 순서다. [patch](../../extracted/cpp-client-subscriber-globals-20261024-proposed.patch)의 SHA는 **22510ce0bc6d1884c10a425aab33de6abd16a519fb8669db560e0e57a676209d**이며 386 baseline 사본에서 check0/apply0이다. 기존10파일+신규 owner1/검사2이며 공용 CMake 변경은 포함하지 않는다. 저장소 core.autocrlf=true가 적용 결과를 CRLF로 바꾸므로 실제 물리 SHA 차이와 줄바꿈을 정규화한 전체본문13개 동일성을 따로 기록한다. 처음 신규 파일 mode 누락으로 `/dev/null` 경로 검사가 실패한 patch/log와, 그 다음 물리 줄바꿈 차이를 발견한 적용 사본도 보존했다. 387의 ClientMain SaveOptions 진단, Session.h/.cpp Cursor 색 서비스/ConnectCursor는 별도 문맥이지만 같은 파일이고, CampaignClientServices의 randomGlobals 근처 Cursor 서비스 주입은 수동 합성 지점이다. 기존 현재 공유 소스를386 SHA와 같다고 재검사하지 않는다.

공용 등록에는 새 두 검사 파일과 `NETSTORM_SUBSCRIBER_LIFETIME_EDGES_FIXTURE`를 최초 `subscriber-lifetime-edges-v2-x86.tsv` 절대 경로로 공급한다. Client/Session3은 실제 기존 제품 전체에 링크해야 한다. 순수6의 격리 추출 어댑터/Runner는 공용 target에 등록하지 않는다.

## 일시중지 전 인계

사용자의 현재 작업 후 goal 일시중지 요청에 따라 이 제안/자료를 현재 범위에서 동결한다. 실제 Client/Session3·기존 ChatChannels 실행, 387 적용분과 수동 합성, 전체 제품 재빌드/회귀와 자연 게임 검증은 **재개 뒤 root 작업**이다. 신규 원문 탐색/추가 범위는 시작하지 않았다.

별도 dialog_lifetime의 CoreRefresh Session 제안 새 함수/3미실행 검사는 읽기 검토하여 명백한 문제 없음을 root와 작성자에게 전달했다. 실제 Login current/cached→ReadPlayerCoreData, 현재 Pool 상대주소/VT 확인→165 다이스의 기존 Count, 실제 Finder/TYPE.techBitIndex를 사용하며 Editor에는 임의 Core를 만들지 않는다. 해당 사본의 /Zs 통과와 3검사 미실행은 별도 자료이며 이 Subscriber 순수6의 실행 수에 합산하지 않는다.
