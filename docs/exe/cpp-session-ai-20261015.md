# 같은 캠페인 Session의 AI 생성·초기 갱신·해제 연결

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. `AGENTS.md`와 양 LEFT를 읽고 기존 정밀 소스/실제 보호 자산을 읽기 전용으로 사용했다. 이번 단위는 원본 게임·복사본·GUI를 실행하지 않았고 보호 원본·AGENTS·dotnetpj를 변경하지 않았다. DirectDraw 전체화면 오류는 사용자 요청대로 제외한다.

## 실제로 연결한 범위

`RawCampaignSession::CreateAi(owner,settings)`는 설치를 마친 같은 Session에서 실제 `RawAiCreate`의 **Color → 기존 brain → NoTemple → 실제 생성자 → 현재 Player 부착 → 실제 초기 갱신** 순서를 실행한다. `Load`와 `RunFrame`에서 이를 자동 호출하지 않는다. 사용자 목표의 자연 완주 완료나 AI 전략 복원을 뜻하지 않는다.

| 대상 | 같은 현재 저장소/실제 몸체 |
| --- | --- |
| 실제 생성자 `00416860` | `RawAiBrainMemory`의0xb08본체/17개 목록, 기존 실제 Pool/Player9/섬/TYPE188 |
| 현재 요구 에너지 | `BindAiTypeRequiredEnergy`의 보호 GameAssets AST와 같은 현재 TYPE. 생성자와 초기 갱신이 같은 getter를 공유 |
| 현재 graph sentinel | 같은 `core.bookkeeping.invalidGraph` BYTE를 원본 두 위치에서 각각 읽어 DWORD에 기록 |
| 실제 부착 `004911a0` | 같은 Player+b0/flags20/subscriber/IP 원문. 참여 여부를 따로 복제하지 않음 |
| 초기 갱신 `00414710` | 같은 Player 기술 부모/실제 `RawPlayerTechnology`/ContentForm/현재 비용·theme·flags2/본체 분류 목록 |
| 방향별 동맹 | 같은81DWORD와 Player+54. 반대 방향 추가는 원본 조건에서만 실행 |
| 해제 `00413ae0` | 현재 Player 토큰의 실제17목록 소멸 → 본체 반납 → Player+b0=0. flags20 보존 |

본체/목록의32비트 값은 실제 host 소유 메모리 등록부의 opaque handle이다. 원본 allocator의 가상주소/미초기화 바이트/OOM과 동일하다고 주장하지 않는다. `allocationBytes`는 caller가 할당 직후 입력을 명시하는 필수 경계이며 누락을 임의0채움 성공으로 바꾸지 않는다. 검사에 사용한0xb6/0x91/0x83/0x62는 원본 실행 당시의 미초기화 값에 대한 관찰이 아니다.

## 현재 전역과 외부 경계

`RawPlayerParticipation::BindCurrentInputs`는 localOwner/playerKnown/battle/metamap/localTransport의 다섯 현재 getter를 모두 요구한다. 실제 Session에서는 `PlayerWorldState`와 같은 `SquidOwnerMode`를 직접 조회하며 callback 뒤에도 그 다음 원본 위치에서 다시 읽는다. 기존 값형 fixture backend는 바인딩하지 않을 때 유지한다. 채팅 숨김9BYTE도 Session의 한 저장소를 공유한다.

`RawAiBrainInitialUpdateGlobals::missionModeRead`는 같은 `CampaignWorldGlobals::missionPost`를 현재 이름 처리 위치에서 읽는다. Session의 bool backing을 임시 DWORD 참조나 프레임별 사본으로 바꾸지 않는다. 기존 단독 검사는 원래 DWORD 참조 경로를 유지한다. `004709d0`의 theme0/1/2/3은005411b0/b4/b8/ac의 실제 기본 번호86/87/88/85를 사용하고 분류 제외 타입은133이다.

실제 언어의 `defaultAiName`, Color 비제로의 전체 색 변경 `0043ca70`, Chat 채널 초기화/등록, 동맹 서버9 송신, 로그인 연결/로컬 전송 모드는 `CampaignAiServices`의 명시 외부 경계다. Session은 brain destructor/free를 이 외부 callback에 맡기지 않고 같은 실제 생성 소유자로 교체한다. 선택된 미연결 경계는 예외로 보고한다. 명시 이름은 기본 이름 조회를 선택하지 않는다. 기존 brain은 Color 이후 반환되며 새 설정/초기 갱신을 다시 적용하지 않는다.

생성자에서 Chat 누락이 발생하면 이미 쓴 동맹 효과는 원본처럼 남고 구성한 목록/본체는 실제로 해제한다. 초기 갱신의 언어 누락은 원본 부착 이후에 발생하므로 현재 Player에 실제 brain이 남는다. 이를 성공으로 표시하거나 합성 rollback하지 않는다. 명시 `RemoveAi`는 오류를 caller에 전파한다. Session destructor는 모든 owner1..8의 실제 brain 정리를 먼저 시도하고, 예외를 stderr에 남기며 다른 owner/Kernel 정리를 계속한다. 이후 각 모듈/Pool 참조를 해제한다. 손상 상태에서의 정리 오류를 정상 원본 수명 검증으로 세지 않는다.

같은81DWORD/Player+54를 총에 직접 읽게 하는 제품 변경은 **단위64**의 별도 범위다. Session은 그 현재 getter들을 연결한다. 이 변경의 작성/검증은 [총 동맹 현재 조회 기록](cpp-gun-current-alliance-20261015.md)을 따른다.

## 연결 검사와 독립 원본 근거

새 console 통합 검사4개는 실제 보호1-1 요새/188TYPE/같은 Session을 사용한다.

1. `CampaignSession_AiUsesCurrentWorldAndLifetime`: 실제 생성/부착/현재graph252/기본 이름/기술ContentForm/방향별 동맹/17목록과 본체18할당, 기존brain 미갱신, 실제18해제와 flags20 보존.
2. `CampaignSession_AiMissingServicesKeepRealFailure`: 설치 전 요청/할당/Chat 누락을 실제 실패로 유지하고, 부착 뒤 언어 누락의 실제 본체를 명시 소멸/반납.
3. `CampaignSession_AiColorPrecedesExistingBrain`: 기존brain의 재요청에서도 Color가 먼저 실행되고 재할당/초기 갱신은 실행되지 않음. 명시 해제하지 않은 본체를 Session destructor가 처리하는 경로도 실행.
4. `CampaignSession_ParticipationUsesCurrentReads`: 값형 사본과 다른 현재 전역, 채팅 callback이 바꾼 현재owner의 다음 조회, localTransport의 실제 연결 생략, 현재battle의 단방향 서버 송신, 부분 getter 바인딩 거부.

신규 native 고유입력은0개다. 기존 생성자160/초기갱신133/참여608/에너지2352 fixture의 C++ 재생은 같은 입력 검증이며 새로운 관찰로 합산하지 않는다. 기존 보고서의 historical SHA를 덮어쓰지 않는다. 현재 소스/기존fixture SHA와 실제 빌드/집중 결과는 [Session 통합 보고서](../../cpppj/session-ai.json)에 별도 기록한다.

root의 소스 고정 공용 **Release 빌드 종료0·경고0·오류0**, 집중 **39개·실패0**가 통과했다. 구성은 CampaignSession12(신규4포함)·AiBrain4·AiInitialUpdate2·PlayerParticipation2·TypeRequiredEnergy3·rawGunTarget4·GunProcess7·FenceTarget5다. 로그는 `extracted/goal-session-ai-combat-build.log`, `extracted/goal-session-ai-combat-focused.log`다. 실제 기존 Session 교전 검사도 같은 실행에서 최초피해frame173/HP1frame178/감시·효과 종료frame898로 통과했다. 이는 별도의 자연 미션 완주나 새 원본 게임 관찰이 아니다. **이 소스의 전체 CTest는 아직 실행하지 않았다.** 이전 단위62의 전체1,241개를 이 변경 뒤 전체 검사로 세지 않는다.

## 후속 실제 플레이 연결

`MakeCampaignClientServices`는 아직 위 Chat/Color/AI 전략 소비자를 구현하지 않는다. 실제 로그인 CheckLogin → 저장맵 부모 → Ready40 → 로그인 완료/진행GUI 종료 → Guide pending0/GUIempty → Stage3의 시작SP/기술/owner별사제/AI 순서는 계속 별도 구현 대상이다. 이번 공개 API는 그 후속 부모가 같은 실제 AI 생성·초기갱신·수명을 호출할 수 있게 연결한다.

전략 `00417d30/00417de0/00414af0/00416eb0/004139f0/00417c70`, 비영brain 피해 통지00415460·간헐천 등록·경제/명령 소비자는 미복원이다. 빈 전략 성공을 넣거나 flags20만으로 자동 Outcome을 켜지 않는다. 자연 미션 단계/정지 소유권/승패 저장과 Early Missions·캠페인1-1~1-2 자연 완주/원본 동일성은 미완료다.
