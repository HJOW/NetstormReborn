# 실제 Pool 정리 부모와 장기 실패 좌표 (단위450)

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 디컴파일 날짜는 **2026-10-23**이다. 기준 HEAD는 `35041198`다. 기존 정밀 C·보호 PE를 읽기 전용으로 사용하며 원본·기존 fixture·디컴파일 산출물은 바꾸지 않는다.

## 원문과 실제 저장소

PATCH `004af170`(CD `004aaa70`)은 하위 Pool Reset → 두 삭제 기록 표 정리 → 실패 Y → 실패 X의 부모다. 기존 `SidPool::Reset`은 대응하는 하위 `004abb10/CD004aaad0`이며 원문대로 삭제 기록을 보존한다. 따라서 그 메서드만 호출하면 부모의 나머지 효과가 빠진다.

이전에 소유자가 없다고 분류했던 두40DWORD는 이미 **SidPool::deletions_의 두20쌍 SID/type**에 대응한다. PATCH는 `0059a738` other → `0059a698` client, CD는 `00589010` other → `005890b0` client 순서다. 이 단위는 별도0 배열을 만들지 않고 실제 기존 기록을 지우는 `ClearDeletionHistory`를 추가했다. 기존 하위 Reset 본체와 기록 보존 계약은 유지한다.

새 `ResetSquidWorldPool`은 실제 하위 Reset 정상 반환 뒤에만 같은 기록·실패점을 정리한다. 시작 생성자가 이미 하위 Reset을 수행한 경우에는 같은 꼬리 `FinishSquidPoolReset`을 이어서 호출해 중복 초기화를 피한다. 실패 좌표는 두 판본 모두 `0xc61c4000`(-10000.0f)을 Y 다음 X에 쓴다.

실패점 `00545370/374`(CD `00565498/49c`)의 기존 **BoardTargetState**를 RawSurfaceStorage 수명으로 옮겼다. Session의 BoardTarget과 AI 다이스 접근점은 같은 상태를 직접 빌리고, 진단 callback 뒤에도 현재값을 읽는다. 일반 fresh World의 기존 기본값은 같은 주소에서 호환하며 prepared 대여·반납은 실패점을 초기화하지 않는다. 다른 손실 좌표·미사일 위치와 합치지 않는다.

`ResetClientSquidPool`은 같은 실제 Client/표면을 검사하고 World 대여 중에는 첫 Native store 전에 거부한다. 호출자는 원문 위치대로 UI·Kernel·Runtime을 앞서 반납해야 한다. 이 작은 API를 전체 State 정리나 반납 coordinator로 설명하지 않는다.

## 먼저 고정한 Native 증거

제품 부모를 만들기 전에 [관측 도구](../../tools/decomp_squid_pool_reset.py)로 두 PE를 각각 실행했다. 판본당6, 총 **12관측**이다. 각 관측에서 실제 할당·반납으로 두20기록을 채우고, 상위 부모·하위 Pool Reset·기록 정리 본문을 모두 실제 CPU로 실행했다. 하위 Reset을 가짜 성공 callback으로 치환하지 않았다.

인자0·-1·INT32_MIN과 두 x87 정밀도를 포함한다. 부모→Pool→history의 실제 진입과 **기록80개 DWORD0→Y DWORD→X DWORD**, 호출당82개 store의 폭·순서를 확인했다. 전체 raw Pool·머리/꼬리·freeCount·예측 커서·양쪽 기록을 읽어 [최초 fixture](../../cpppj/tests/fixtures/squid-pool-reset-x86.tsv)와 [Native 보고서](../../cpppj/squid-pool-reset-native-20261026.json)에 고정했다. 양수 malloc 분기와 메모리 할당 실패는 이 CPU 관측 범위 밖이며 C++ 재확보/실패 검사는 구별한다. 원본 게임 창·OS API는 실행하지 않았다.

## 검증

신규5사례로 Native12관측 전체 재생, 실제 하위 Reset의 기록 보존과 상위 삭제 차이, 양수 재확보, 하위 실패 때 기록·좌표 보존, 기록만 정리할 때 Pool 불변, 같은 Client/대여/외부 Pool 관문, Board·AI의 현재 실패점 재조회와 prepared 보존을 검사한다.

Source1의 **2,038소스 동결**, build1 종료0·경고/오류0, 집중 **487고유/489관측·실패0**(신규5개 전부)을 확인했다. 기존46사례의 내용과 Native fixture3개를 체크아웃 개행 차이를 구별해 보존했고 실제로 다시 실행했다. 하위 Reset·Record·Release 본문은 바이트 그대로다.

같은 최종 제품 전체 CTest **3076개·실패0**, GUI2경로와 자연 Early2 A~I·회수75·SP3075·Tutorial3 진입도 통과했다. 이전447의3,052개와448/449 신규19개·이번5개의 실제 검사 이름 집합이 모두 일치했다. 보호1,821파일과 원본/CD 전체 파일 집합·허용 설정도 실행 전후 유지했다.

게임 SHA는 `b5966c23f73c6cfad7fb2f155d5f3283a2481e066307f46452056b616836b76e`다. [실행 보고서](../../cpppj/squid-pool-reset-runtime-20261026.json), 원시 `extracted/cpp-squid-pool-reset-450-*`에 빌드·집중·Native·통합·시각 확인을 보존했다. 화면 확인은 원본과 픽셀 동일성 검증이 아니다.

## 다음 연결

255DWORD `0054d4e8`의 PlayerNoMake 경고 표와 수신 부모 `00433c30`, 20 pool header 등록 `0044fb20→00472880`·정리 `00472830`은 별도로 실제 소비자를 확인해야 한다. 이들을 신규 빈 표로 대체하지 않았다. 기존 SID16/전체 Deck 경계, Native State1 전체 Effect/prefix·GameplayUse/Player 고정뷰 반납 coordinator와 실제 Edit 설치도 남아 있다.

전체 Early·캠페인·Priest Training·데모 자연 완주·진행 저장·표시/음향 동일성 목표는 계속 진행 중이다.
