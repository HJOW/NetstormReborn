# cpppj 의식 뒤 로컬 Core·저장 연결 — 단위599

2026-10-30, HJOW-Athlon(192.168.0.94, Windows 10 19045). AGENTS.md와 인계 문서를 다시 확인했다. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23이다. 원본/정밀 C·함수 표는 읽기만 했고 AGENTS.md는 바꾸지 않았다.

## 구현과 범위

기존 `DaisRegularHooks::refreshLocal`의 미연결 오류를 현재 Session의 실제 Core 갱신, CoreData 기록, 현재 맵 snapshot에 연결했다. 이 콜백은 단순 UI 갱신이 아니므로 기존 설명도 고쳤다. 상위 `RawDaisRegular::FinishSacrifice`의 missionPost/현재 owner 조건은 그대로 유지했다.

원문00449b30 내부의00449d98~00449dc5는45BYTE/9명령으로0044fc00→현재 Template owner 읽기→004beb30→현재 owner 재읽기→004bfac0을 실행한다. 마지막 cdecl snapshot에 전달되지 않는 ECX를 준비할 때도 현재 owner DWORD를 다시 읽는다. 새 `RefreshLocalRitualData`는 이 읽기를 생략하지 않는다. 실제 Session은 기존 현재 Core, 같은 Template9와 각 저장 body를 사용하고 성공 기본값·이중 World·새 저장소를 만들지 않는다.

0044fc00은 현재 기술 비트와 소유 다이스 count 하한을 갱신하는 기존 실제 `RefreshCurrentPlayerCore`다.004beb30은598에서 공통화한 CoreData body이며 manager/Metamap 진단과 missionPost/editor 자연 반환을 유지한다.004bfac0도 선행 manager 존재 관문을 추가하지 않고 당시 Battle/Fort 두 독립 if와 각 현재 owner를 읽는다. 원래 쓰기 순서를 바꾸거나 앞선 효과를 롤백하지 않는다.

이 단위는 의식 전체를 실행하거나 포획/희생을 재현한 결과가 아니다. 상위 의식의 로컬 완료 콜백을 연결한 것이며 랭크 상승/비로컬 보상/MP 상태·안내 등은 여전히 잔여다.598의 로컬 기술 획득과 별개 완료 항목으로 기록한다.

## 원문과 C++ 검증

[Native 집계](../../cpppj/ritual-refresh-native-20261030.json), [관측 도구](../../tools/decomp_ritual_refresh.py)는 보호 PE에서 이 분기 내부만 실제 CPU로 실행한다. 큰 Core/기록/snapshot은 명시 외부 경계다. owner 입력은 정상 슬롯0/1/8과 다음0/2/8이며 각 효과 진입 중단0~5를 조합한54사례다. 두 독립 emulation의108관측/재관측 차이0을 확인했다. 상위 owner/mission 조건·원시 손상 주소 감김·실제 자식 전체의 동일성 검증으로 확대하지 않는다.

최초 도구는 정밀 표의 앞자리0 없는16진 entry를 문자열 `00449b30`과 직접 비교해 StopIteration으로 종료했다. 처음의 빈 native 디렉터리/로그를 보존하고 숫자 비교로 고친 새 native-v2에서 관측했다. 최초 Native TSV는 새 파일로 한 번만 만들었고 -text로 보존한다.

새 C++2검사는54개 전체 trace, 두 번의 현재 owner 읽기, 각 진입 중단의 부분효과, 도달한 기록 장치 누락과 CD 명시 거부를 대조한다. 인접 현재 Core/실제 Session Core·CoreData·전체 Battle/Fort 저장을 합한 집중20개/실패0, 등록3492개다. 최종 게임/검사 Release 빌드는 경고·오류0이다. 전체3492개를 실행한 결과는 아니다. 기존 Dais 전체 희생/MP 경로를 새로 실행한 검사도 아니다.

이번 새 함수와 Session 콜백 연결의 실제 게임 실행은 없다. 내부 자식은 기존 독립/Session 검사와 소스 연결로 확인했다. 이번 경로의 실제 GUI 완료는 사용자의 플레이 확인에 남긴다.597~598의 실행 결과를599의 실행 성공으로 합산하지 않는다.

## CD 10.72와 다음 경계

CD 대응 의식 완료00482620은 로컬 조건에서 별도 wrapper004825f0을 호출한다. 이 wrapper는0045f0d0→00429090(현재 Template)→004299d0의 세 호출이다.10.78에서는 해당 wrapper가 내부로 합쳐져 있다. 이번 새 부모는10.78만 허용하며 CD 복원을 완료한 것으로 세지 않는다.

정적 분석으로 다음 사항을 확인했으며 아직 연결하지 않았다.

- 로컬 `noIsland` 획득의 `localSpecial(0)`은0044fa00 랭크 상승이다. Core rank 쓰기,0042f540, 기존 내용물 삭제,004923a0 기술 선택,00470a90, NewRank/Tell/snapshot/004b54e0이 이어진다. 전체 흐름을 단순 rank 증가로 대체하면 안 된다.
- 비로컬 일반 기술0048f380은 현재 Player 기술 contained를 추가하지 않는다. 실제 별도 Core 장부9×16BYTE의 owner 기록+4에 TYPE+1ec 기술 비트를 쓴다. `RawPlayerTechnology::Ensure`로 대신하지 않는다.
- 비로컬 SP 정규화는0040ef30 getter→원문 정수 변환→00447810 정책 setter다. 현재 Session이 사용하지 않는 기존 `MakeDaisRegularHooks`의 직접 분산 SP 쓰기를 그대로 연결하면 현재 Editor/known/표시 정책이 빠진다. 실제 Session은 여전히 이 경계를 명시 미연결로 남긴다.
- MP 상태/사망 안내와 Help 외부 OS 프로토콜은 별도 잔여다. 신규MP 검사는 하지 않는다.

## 산출물과 인계

[집계](../../cpppj/ritual-refresh-runtime-20261030.json)와 원시 `extracted/cpp-ritual-rewards-599-20261030/`에 입력/최초 실패·최종 Native/C++/빌드/보호 해시를 남겼다. 보호2566개는 이전 fixture743개와 원본1394/CD425/AGENTS/정밀 표·양 판본 C를 포함하며 모두 불변이다. 현재 fixture는744개다.

새 게임 빌드는599이지만 새 ZIP/GUI 실행은 없다. 마지막 실제 검증 배포본은598(게임13f35ef0…/ZIP3a68ff02…)이며 Tutorial1·Edit 두 왕복의 근거는[598](cpp-new-technology-20261030.md)이다. 마지막 전체34 진입은595 제품/596이다. 사용자 지정 실행 검사 범위를 지키고 기능 묶음 뒤 배포/인접 실행을 갱신한다.

현재 전체C++/전체34·실제 획득/의식 UI·Win11·PCM/활성 사용자 전체화면·자연 완주/전체 동일성과 후순위MP 무시험/4~7차는 미완료다. 다음은 랭크 상승과 남은 실제 UI/Edit·Native 선택·시나리오 조건이다. 포획/희생·특정 적 공격/승리 재현, 공중3종 전용, 신규MP 검사는 하지 않았다.
