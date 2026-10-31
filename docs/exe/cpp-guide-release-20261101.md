# Guide 해제와 Tutorial 소멸의 원문 수명 — 단위671

2026-11-01 / HJOW-Athlon. 현재 PC와 마지막 전체 정밀 디컴파일 PC **HJOW-Athlon / 2026-10-23**이 일치한다. 10.78 보호 PE와 해당 정밀 소스를 정적으로 대조했다. 원본·CD·AGENTS.md·정밀 디컴파일 산출물은 변경하지 않았다. 10.72는 이번 몸체 대조 대상이 아니다.

## 명시 MissionEnd와 Guide의 해제

원본 `00463e40`은 입력 명령의 상위 플래그를 `03ffffff`로 제거한 뒤 `00542930`의 MissionEnd를 처리한다. 해당 분기는 현재 미션의 가상 `+1c` 호출 → `00463ab0` 해제 → 반환1이다. 이 위치에서 메인 메뉴로 이동하거나 State3을 예약하지 않는다. Tutorial의 가상 `+1c`는 원문의 빈 `004437f0`이다.

`00463ab0`은 현재 미션과 Config의 진단 뒤 가상 소멸을 호출하고, Guide의 현재 포인터와 첫 BYTE를0으로 쓴다. 기존 Begin/Fork/Restart가 수행하는 서로 다른 선행 가상 종료와 이름 복사는 [단위251](cpp-guide-transitions-20261021.md)의 범위를 유지한다. 이번 해제 근거를 그 순서 전체의 새 검증으로 합산하지 않는다.

`RawMissionGuide`의 flags8 또는 sequence6 뒤 해제도 같은 `00463ab0`의 수명이다. 명시 MissionEnd와 자동 Guide 해제는 호출 원인은 다르지만, World를 곧바로 반납하는 별도 부모로 바꾸어 연결할 근거는 없다.

## Tutorial 소멸의 직접 쓰기와 큰 하위 효과

보호 PE의 `004c3210`은 Tutorial 가상표 설치 뒤 다음 순서로 실행된다. 현재 DWORD는 효과 사이에 복제하거나 최종 사본으로 되돌리지 않는다.

| 위치 | 실제 동작 | 대응 의미 |
| --- | --- | --- |
| `004c323c` | `004c23c0(1)` 호출 | 허용 표의255DWORD를 원문 부모로 갱신한다. |
| `004c3244` | `00595078=0` | 회수 금지 `denySalvage`를 지운다. |
| `004c324a` | `0059507c=0` | 승급 금지 `denyAscend`를 지운다. |
| `004c3250` | `005ca8e0=0` | 현재 미션 번호를 지운다. |
| `004c3256` | `005ca8e4=0` | Early Missions 분배 번호를 지운다. |
| `004c325c` | `005ca8e8=0` | 현재 단계의 전체 DWORD를 지운다. |
| `004c3262` | `005ca8ec=0` | 단계 변경 표식의 전체 DWORD를 지운다. |
| `004c3268` | `00413b20()` 호출 | 현재 owner1..8의 실제 AI를 반납한다. |
| `004c3277` | `00483210(this)` 호출 | 기본 Mission의 Config와 모드를 반납한다. |

`00413b20`은 시계 Resume가 아니다. 매 owner의 현재 Player+b0를 읽고, 비영이면 `00410100`의 실제 AI 소멸 → free → 같은 Player+b0=0을 실행한다. 중립0은 순회하지 않으며 flags20도 지우지 않는다. 이미 복원된 [RawPlayerParticipation의 RemoveAllBrains](cpp-player-participation-20261015.md)가 이 몸체에 해당한다. World를 보존하면서 이 호출을 빠뜨리면 기존 AI가 남는 별도 동작 차이가 된다.

기본 `00483210`은 미션 Config를 소멸/반납하고, this+5c를0으로 쓴 뒤 `00594fa0=0`, `global.inMission=0`을 공개한다. 별도 삭제 래퍼 `00483260`은 같은 기본 수명에 필요시 외부 객체 free를 더하는 함수이며, 이번 primitive가 호스트 객체의 메모리를 직접 free하는 것은 아니다.

## 본문 예외의 기본 Mission 정리

정상 반환의 호출 순서만 옮기면 원문 소멸 수명이 완성되지 않는다. `004c3210`은 본문 전에 SEH 상태0을 설치하고, 전체 AI 반납이 정상 반환한 뒤 `004c326f`에서 상태를-1로 바꾼 다음 기본 Mission을 호출한다. 따라서 허용 표 또는 AI 하위에서 본문 예외가 발생해도 기본 Mission 정리는 필요하다.

정적 PE에서 확인한 연결은 handler `004fb948` → FuncInfo `00524b6c` → unwind map `00524b64` → funclet `004fb940`이다. FuncInfo의 앞부분은 `19930520,1,00524b64`이고, map의 한 행은 `ffffffff,004fb940`이다. funclet의 두 명령은 부모가 보관한 this를 ECX로 읽은 뒤 `00483210`으로 이동한다.

공용 primitive는 같은 기본 Mission 반납을 지역 guard에 두어 정상 꼬리와 본문 예외 정리 양쪽에서 한 번 수행하도록 작성한다. 본문 예외 전에 끝난 허용 표·직접 쓰기·AI의 부분 효과는 복구하지 않는다. 정상 꼬리의 기본 Mission 예외를 재호출로 바꾸지도 않는다. 본문 unwind 중 기본 반납이 다시 던지면 C++의 중첩 예외 종료 규칙을 따른다. 이를 원본 Win32 CxxFrameHandler 전체 실행이나32비트 SEH 객체 ABI의 동일성 검증으로 확대하지 않는다.

## Guide 소유와 World 소유의 경계

위 소멸 연쇄에는 World·Player 전체·Template·HUD·사용자 입력·연결·음향을 반납하는 호출이 없다. 음악을 메인 메뉴 곡으로 바꾸거나 현재 Native State를 초기화하지도 않는다. `00594fa4`의 missionPost와 `AllowAnyCapture`·`OrdinaryGeysers`에0을 쓰는 동작도 없다. `00594fa0`의 현재 미션 모드와 missionPost는 서로 다른 전역이다.

시계 Resume 호출 역시 이 소멸에 없다. Tutorial이 직접 보유한 정지와 HTML/일반 UI가 보유한 정지를 구별해야 하며, Guide가 사라졌다는 이유로 편의 재개를 추가하지 않는다. 자체 커스텀 명령이 정지 중에 MissionEnd를 실행했다면 이 원문 수명 자체가 남기는 정지와 제품의 임의 재개를 구별해야 한다.

호스트의 Tutorial/Start 바인딩은 Script와 미션 상태를 빌리므로 그 소유자가 사라지기 전에 닫아야 한다. 반면 이미 설치된 UserInput·World·Session·커서/HUD 장치는 해당 World가 살아 있는 동안 계속 사용할 수 있어야 한다. World 반납 때 UserInput을 먼저 닫는 기존 순서와 Guide만 반납할 때 입력을 보존하는 순서는 양립한다.

`worldMissionToken_`은 원본 게임 전역이 아니라 Guide와 비동기 World 완료의 호스트 대여 표식이다. 이전 Guide를 반납할 때 이 연결을 닫는 것과 이미 설치된 World 입력을 반납하는 것은 다른 수명이다. 이전 World의 늦은 완료가 새 Guide의 Script에 붙지 않게 하는 관문은 유지해야 한다. 접속 완료 전 해제의 모든 Native7 조합을 이번 정적 해제 근거로 완료 처리하지 않는다.

## 내장 시나리오와 커스텀 명령의 범위

이번 읽기 전용 검색에서 기본 `netstorm.tarc`의 `.english`50개와 `originals/` 아래 디스크 `.english`499개에 MissionEnd 문자열은0건이었다. 디스크 파일 수는 현재 작업 PC의 원본 자료 트리 검색 수이며, 전부 CD/기본 배포에 들어 있던 파일이라고 추정하지 않는다.

MissionEnd 자체는 원문 명령표와 현행 `MissionControlScriptCode`에 등록되어 있다. 커스텀 시나리오의 `$Button=Stop,MissionEnd,0`은 기존 HTML parser·문자열 고리·일반 입력 큐가 내보낼 수 있는 유효한 명령이다. 따라서 잘못된 메인 메뉴 이동은 커스텀 시나리오 명령의 동작 차이로 다루며, 내장34경로의 Retry·Next Tutorial·Next Mission·Abort가 새로 실패했다는 근거로 확대하지 않는다. 자연 승리·포획·희생·특정 적 공격을 재현할 필요도 없다.

## 정적 자료와 실행 범위

원시 정적 목록은 작업 PC의 `extracted/cpp-mission-end-671-work/static-pe.txt`에 있다. `004c3210`32명령, `00413b20`24명령, `00483210`21명령, unwind2명령, handler2명령으로 합계81명령을 읽었다. 추가로 확인한 metadata DWORD와 명령 수는 구별한다. 사전 감사는 같은 폴더의 `audit.md`에 있다.

이 수치는 PE의 정적 디스어셈블리이며 **새 Native 실행0·새 fixture0**이다. 원본 게임이나 GUI, 실제 통신을 실행한 기록이 아니다. 전체 정밀 디컴파일 산출물 변경도 없다.

## 제품 접합

기존 UberGump의 내부 MissionRuntime을 같은 필드와 생성 입력의 `CampaignGuideRuntime`으로 옮겼다. `ReleaseCampaignGuideMission`은 실제 Script/Runtime/요청 unique/shared 소유자와 Client를 직접 받는다. 실제 Uber와 창 없는 검사가 이 함수를 공유하며, 검사별 release lambda가 제품 소멸 본문을 대신 작성하지 않는다.

이 함수는 이전 Guide의 호스트 지연 요청을 취소하고, Tutorial이면 공용 원문 소멸 primitive에 현재 Client 전역과 실제 AI 반납을 연결한다. 기본 Mission 해제 자리에서는 Tutorial→Start→Runtime을 반납한 다음 기존 `ReleaseGuideMissionScript`로 Config 해제→Native mission0→global.inMission0을 실행한다. `worldMissionToken`만0으로 만들며 UserInput·World·HUD·Template·음악·시계는 소유 인자에 포함하지 않는다. 기존 `DetachMissionWorldBindings`의 입력 반납은 실제 ClearWorld/State Runtime 반납에서 계속 수행한다.

자동 Guide release와 명시 MissionEnd의 두 Uber 연결에서 MainMenu/메뉴 음악 전환을 제거했다. Guide 해제의 `PopGamePause`도 원문에 없으므로 제거했다. 실제 로딩 안내의 창 장치 반납은 현재 Connecting 토큰이 같은 경우 기존 Uber 위치에 남는다.

회수/승급 금지값 `00595078/7c`는 Session마다 사라지는 사본 대신 `ClientGameplayGlobals`의 두 signed DWORD를 공유한다. 실제 Session Start·회수·Early2·게임/객체 메뉴·진단은 같은 참조 accessor를 사용한다. World가 없는 Guide 소멸도 그 주소에0을 쓴다. 일반/준비 대여나 반납에 임의 초기화는 추가하지 않았고, 독립 원문 primitive 검사의 값형 fallback은 유지한다.

AI 반납은 살아 있는 Client Gameplay의 실제 현재 Session과 같은 Player9 정체성을 확인한 뒤 `Participation().RemoveAllBrains()`를 호출한다. 일반 Guide 소멸에서는 State reset 증빙을 발급하지 않는다. World가 이미 반납됐으면 같은 Client의 실제 초기화된 Player9에서 owner1..8의 현재 brain이 모두 NULL일 때만 자연 반환한다. 비NULL인데 소유 Session이 없는 경우는 잘못된 호스트 수명으로 거부하며 가짜 brain/0쓰기를 만들지 않는다. 이 진단을 원본의 손상 포인터 예외 ABI와 동일하다고 주장하지 않는다.

State가 이미 증빙을 소비한 NULL owner도 전체 RemoveAllBrains에서는 자연 반환하도록 했다. 각 owner의 현재 정체성을 검사하고 다음 owner를 계속 방문하며, 개별 RemoveBrain/State reset의 중복 증빙 관문은 유지한다. 원문에 없는 전역 초기화나 전체 순회 생략을 추가하지 않는다.

## 검증 결과

최종 관련 **35개/실패0**이다. 새10개와 기존25개를 구별하며, 집중 대상의287등록 전체나 주 검사 전체를 실행한 결과는 아니다. 실행 로그별 목록·해시·최초 실패는 [보고서](../../cpppj/guide-release-20261101.json)에 있다.

| 범위 | 결과 |
| --- | --- |
| 실제 Guide/World 접합 새3개 | 실제 End 분배→공용 해제 뒤 같은 World/UserInput/Player/Pool/Kernel을 보존했다. 실제 지상 AI18할당 반납, 일반 해제와 State 증빙의 구분, 로컬 사제 잉크의 좌선택을 확인했다. World 없는 정상/손상 입력의 기본 Config unwind와 자기 정지도 확인했다. |
| 소멸 primitive 새4개 | 직접 쓰기 순서·allow 실패·AI 실패의 부분 효과·기본 Mission 예외1회를 확인했다. |
| 장기 제한 전역 새2개·전체 AI 새1개 | Client 대여/반납의 주소·DWORD 보존, 실제 Start의 공유 출력, State 반납 NULL 뒤 다음 owner 순회와 손상 정체성 거부를 확인했다. |
| 기존 회귀25개 | Guide 교체2·MissionLifecycle2·MissionStart4·GameAux1·PlayerParticipation8·GuideWorldLifetime3·게임 메뉴3·Runtime 반납1·전역 반납1을 확인했다. |

기존 fixture의 C++ 재생은 PlayerParticipation608·동맹288·Guide286·MissionLifecycle831·MissionStart204의 고유2217행이다. CurrentInputs48·GameAux48은204 내 부분집합의 추가 재생이므로 고유 합계에 다시 더하지 않는다. 새 Native 실행이나 fixture 생성은0이다.

기존 GuideWorldLifetime의 같은1개 검사는 최초 세 번 실패했다. 첫 실패는655~656에서 필수가 된 Renderer/후반 UI 장치가 옛 검사 장면에 없었고, 두 번째는 Run의 setup 읽기를 생략한 Client가 State InterpretOptions 뒤 화면 크기0이 된 입력 누락이었다. 실제 Client Renderer 초기화와 기본 화면 설정을 연결했다. 세 번째는 새 관측 단언이 CloseProgress를1회로 잘못 기대한 것이다. 원문 타이머 종료와 State 완료가 각각 호출하므로2회로 정정했다. 최종 관측은 Renderer1·시작 안내1·팔레트1·닫기2·UI2/3이며 기존 장면3개를 모두 재검사했다. 제품의 필수 장치 관문과 기존 World/Template 수명 단언은 유지했다. HUD/팔레트/안내의 화면 출력은 이 장면의 명시 외부 장치 경계이며 픽셀 검증으로 합산하지 않는다.

총15회 필터 호출·44개 검사 함수 실행에 위 초기 실패3회를 포함한다. 제품 PE는 이 검사 중 변경되지 않았고, 검사 장면만 보완한 뒤 관련3개를 재실행했다. 여섯 순차 Release 빌드는 모두 종료0·경고/오류0이었다. 최종 제품은 `4ea8e712282bb224a65d41906cc75c421d7d3a7348759ddc982d58f0a33e7d93`이다.

관련 보호4개(AGENTS.md·양판본 PE·기본 TAFF)의 전후 크기/해시가 같다. 마지막 전체2632개/기존fixture773개 감사645와 구별한다. 사제 포획/희생·특정 적 공격·시나리오 승리·공중3종 새 생성/행동·MP 검사는 하지 않았다. 게임 창·물리 입력·PCM 실행도0이다. 현재 제품의 전체34·자연 완주·전체 동일성 검증이 아니며 최신 전체34/ZIP은645의644제품이다. 전체 목표는 계속 진행 중이다.
