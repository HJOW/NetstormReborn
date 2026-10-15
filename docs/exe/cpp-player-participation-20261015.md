# 실제 Player 참여·동맹·AI 포인터 수명 경계

2026-10-15, 현재 PC와 마지막 정밀 디컴파일 PC는 **VM-W11-Codex**다. AGENTS.md와 양 인수인계 문서를 확인했다. 기존 정밀 Ghidra 프로젝트에서 PATCH10.78 함수 **7개**를 읽기 전용으로 내보내고 보호 PE의 기계어를 격리 실행했다. 원본 게임·복사본·GUI는 실행하지 않았고 보호 원본을 변경하지 않았다.

## 실제 초기화 위치

기존 [미션 설치 경계 감사](cpp-raw-fort-install-boundaries-audit-2-72d4d.md)의 원본 순서를 다시 확인했다.

1. Tutorial Load `004c3290`은 Template 정리 `004bfb00` 후 `loadFort`를 `004b51e0`에 전달한다. `004bfb00`은 Template9 이미지의 정리이며 Player 활성화 함수가 아니다.
2. `004b51e0`은 가상 Zone packet과 pending State7을 만든다. State7 `004b6dd0`은 `004b2df0` reset으로 이전 AI/Pool/Player를 정리하고 `004b4be0`으로 Zone/Subscriber 정보를 설치한다. Player 초기화 `004910a0`의 자기 동맹 bit와 첫 지역127은 로그인 완료 상태를 뜻하지 않는다.
3. 실제 packet0x27 처리기 `004a7800`이 subscriber의 Player 번호를 찾고 **`004a6dc0`으로 사람 연결 flag를 갱신한 뒤** 새 로그인 분기의 Template/월드 부모 `004b5300`을 호출한다. 이 함수는 `004a6ab0`에서도 같은 flags 갱신을 수행한다. 부모 설치 뒤의 Ready `0x40`은 연결 `0x01/0x02`와 별개다.
4. 이후 pending State가 없고 진행 GUI도 비었을 때 Guide가 Tutorial Start `004c2b20`을 실행한다. 내 시작 SP/기술을 적용하고, owner1..8에서 **`owner!=local || demo`이며 실제 사제가 존재하면** `004c2d40`→`00417e50`로 AI를 생성한다. `aiName`의 유무가 AI 참여 여부를 결정하지 않는다. 빈 이름은 `00414710`이 `defaultAiName`으로 채운다.
5. 실제 AI 생성자 `00416860`은 기술/전략 자료를 구성하고 선언된 AllyList로 `00490ea0(AI,mask)`를 호출한다. `allies[AI][local]`이 참인 경우에만 `0048f6a0(local,AI)`로 로컬의 반대 방향을 별도로 더한다. 모든 AI 사이의 관계를 자동 대칭화하지 않는다. `004911a0`이 실제 brain 포인터를 Player에 기록하고 `0x20`을 켠 뒤 초기 업데이트 `00414710`으로 기술·SP/행동 자료를 구성한다.

원본 승패 `004c36c0`과 사제 생존 getter의 참여 조건은 **Player `+0x50 & 0x22`**다. 사람의 연결 bit `0x02` 또는 AI 부착 bit `0x20`을 읽는다. bit1(값 `0x01`)만 검사하거나 기존 값형 `MissionPlayers::active`/대칭 동맹을 복사하여 실제 초기화라고 하면 원본과 다르다.

## 구현한 최소 공용 모듈

`o/RawPlayerParticipation`은 PATCH의 실제 **Player9×0xb4 바이트**, 방향별 **81 DWORD**, 현재 숨김표9바이트와 mutable 모드/로컬 번호를 참조한다. 생성자는 다른 판본/크기를 거부하고 게임 상태 사본을 만들지 않는다.

| 원본 주소 | API | 실제 쓰기·외부 효과 |
| --- | --- | --- |
| `004a6dc0` | `CheckLogin(pending,owner)` | owner1..8이며 현재 flags bit1이 없을 때 `(flags & ~0x100) | 3`. localTransport이며 pending==owner이면 재배치를 생략하고, 그 외는 실제 연결 번호를 재배치한다. 실패는 거부 응답→연결 해제다. |
| `00490ea0` | `SetAlliance(owner,mask)` | 자기 bit를 OR한 mask를 Player+54와 해당 행9칸에만 쓴다. playerKnown이면 Chat40/80 채널을 지우고, 현재 양방향 동맹 또는 숨김표 조건으로 원본 순서대로 재등록한다. |
| `0048f6a0` | `AddAlly(owner,target)` | 해당 방향의 mask 한 bit와 행렬 한 cell만 켠다. battle이면 현재 ownerBYTE/maskDWORD를 서버9에 요청한다. |
| `004911a0` | `AttachBrain(owner,actualToken)` | 이전 brain은 assert만 보고 임의 해제하지 않는다. 전달된 실제 opaque32를+b0에 쓰고 flags20을 켠다. signed subscriber<1이면 owner+10000과 IP30바이트 초기화를 수행한다. 0 입력의 필드쓰기 역시 원본과 같다. |
| `00413ae0` | `RemoveBrain(owner)` | 현재 nonzero brain의 실제 destructor→실제 메모리 반납→+b0=0 순서다. 두 콜백이 끝나기 전에는 Player에서 이전 토큰이 조회된다. flags20은 원본대로 보존한다. |
| `00413b20` | `RemoveAllBrains()` | 현재 owner1..8의 포인터만 순회한다. 중립0의 brain은 남긴다. |
| `0048f550` | 안정된 Player span 인덱스·진단으로 재사용 | 원본의 owner getter/부착 시 subscriber 파생 값 및 metamap 진단 근거다. |

전송·Chat·brain destructor/메모리 반납은 실제 상위 소유자가 공급할 필수 효과 경계다. 실제 도달한 미연결 경계는 예외로 거부하며 가짜 연결·가짜 brain·성공 기본값을 제공하지 않는다. 이 클래스는 **brain 메모리나 전략 프로세스를 생성/소유하지 않는다**. Session이 실제 생성 소유자를 갖추기 전에 임의 토큰을 넣어 AI 참여를 위장하면 안 된다.

CD10.72의 정적 참고 대응은 로그인 `004c17f0`, 전체 동맹 `00406130`, 한 방향 추가 `00406550`, brain 부착 `004062f0`, owner별 해제 `004012b0`이다. 이 단위의 신규 구현/독립 실행 대조는 PATCH 전용이며 CD의 필드·분기 동일성을 주장하지 않는다.

## 독립 검증

`tools/decomp_player_participation.py`는 보호 PE를 가상 메모리에 복사해 위 7개 export의 실제 불연속 주소 범위만 실행한다. 원본 로그/assert·연결·Chat·메시지·AI 파괴/반납은 명시 관찰 경계다. 실제 AI 메모리를 역참조하거나 원본 게임/OS 진입을 수행하지 않는다. 매 호출은 정상 반환·cdecl/thiscall 스택·보존 레지스터·x87 제어어를 확인한다.

최종 **608개 고유 입력**은 로그인210, 전체 동맹122, 단방향 추가72, brain 부착192, owner별 해제6, 전체 해제6이다. 모든 입력에서 **전체 Player9 1,620바이트·전체 방향별 표324바이트·반환값·외부 효과 인자/순서**를 C++에 전수 대조한다. 다른 행의 비정규 nonzero 값7·높은 mask bit·기존 flags·signed subscriber·IP의 바로 뒤 두 바이트도 보존 여부를 확인한다. 이름 필드 `+0xc..+0x2b`의 정상30/31글자 경계를 추가해 초기30바이트 호스트 제한을 실제32바이트로 교정했다. 초기606입력은 최종608개에 포함되며 합산하지 않는다. 별도 C++ 검사는 파괴/반납 중의 현재 포인터 수명, flags20 보존, 반대 방향을 자동으로 바꾸지 않는 순서를 확인한다.

첫 oracle는 다른 CRT 주소를 free로 추정해 미등록 명령 `004e4354`에서 실행을 차단했다. 실제 호출 주소로 교정한 뒤 같은 606입력을 관찰했다. 허용 범위를 넓혀 알 수 없는 명령을 실행하지 않았다. 첫 공용 빌드는 다른 agent 빌드와 겹쳐 `CL.write.1.tlog` 잠금의 **MSB6003**으로 종료했고 이후 빌드 독점을 조율했다. 첫 집중 검사는 fixture 머리행을 입력으로 읽는 setup 오류2체크였으며, 머리행 skip1줄만 교정했다. 이 교정에서 production 몸체와 native fixture는 바꾸지 않았다.

606입력 당시 Release 빌드 **경고/오류0**, 집중 **2개·실패 체크0**다. Outcome/Early2까지 포함한 통합 CTest는 **1/1·내부1,183개·실패 체크0**, 검사114.74초/전체114.85초다(`extracted/run41-42-{build,ctest}.log`). 통합 실행 중 이름32바이트/608입력 보강이 들어왔으므로, 통합 종료 뒤 최신 소스로 다시 Release게임/검사를 빌드해 **경고/오류0·최종집중2개 실패0**을 확인했다(`extracted/run42-final-{build,focused}.log`). 전체1,183개를608입력 수정 뒤 다시 실행한 것으로 세지 않는다. 최초 빌드와 집중 실패 로그 및606입력 빌드·집중 결과는 `extracted/player-participation-{initial-build,focused,final-build,final-focused}.log`에 있다. Fixture는 `cpppj/tests/fixtures/player-participation-x86.tsv`, 입력/함수/현재 소스/검사/도구 SHA와 제한은 `cpppj/player-participation.json`이며 `python tools/decomp_player_participation.py --verify` 최종608행 감사를 통과했다.

## 아직 실제 미션을 활성화하지 않는 이유

이 단위는 원본 필드 경계를 복원했으며 `RawCampaignSession`/`GameWorld`의 자동 초기화나 승패 Check를 켜지 않았다. 실제 로그인 부모 순서와 Chat 소비자를 연결하고, `00417e50`→`00416860`→`00414710`의 실제 brain 생성·등록·기술/SP 초기화 및 `00417d30`/`00417de0` 전략 프레임을 구현해야 한다. 비영 brain이 되면 기존 피해·가이저·명령 경계도 실제 AI를 호출하므로 flags20만 세팅하는 방식으로 건너뛸 수 없다. 이 전제가 완성되기 전에는 실제 UI의 자동 승패/팀전멸 검사를 보류한다.

Early Missions/캠페인1-1~1-2의 실제 완주·원본 동일성은 미완료다. 원본 DirectDraw 전체화면 오류 수정은 범위에서 제외한다.
