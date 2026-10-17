# 실제 클라이언트 로컬 로그인과 짧은 State 확장 — 단위164

기준은 `f3e4525`다. AGENTS와 양 LEFT_JOBS를 재독했으며 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**으로 일치한다. 새 디컴파일, 원본 게임 실행, 보호 원본/AGENTS/dotnetpj 인계 수정, DirectDraw 수정은 없다. 직전 단위163은 코드·독립 원문·실제 전송 검사·커밋을 완료한 진행턴이다.

## 호출 순서 변경

기존 Client는 `BeginMission` → `GameWorld` 생성자 → `Session.Load`로 월드를 바로 읽었다. 이번에는 실제 Client가 현재 월드 포인터를 등록한 다음, 동일 Session의 로컬 Zone·접속·원본 로그인 처리를 거쳐 그 로그인 처리기 안에서 `Load`를 호출한다. 이 순서는 Chat의 현재 subscriber getter가 새 월드를 보도록 하는 데도 필요하다.

원본 `4c3290`은 `mySubscriber=54265c`를 준비하고 `4b5300`도 같은 값을 복사한다. 보호 PE의 `54265c` 초기값은1이다. 새 경로도 이 미션 부모 입력을 사용한다. owner/접속 성공/Ready를 미리 쓰는 것과 구별한다. 기존 독립 모듈 검사의 명시 `Load`는 유지하며 실제 Client만 `LoadLocalConnection`을 선택한다.

[연결부](../../cpppj/src/client/RawCampaignSessionLogin.inc)는 같은 Player9, 방향별 동맹, Template9/State stream, SP, TYPE, 에너지 범위와 다음 원본 몸체를 조립한다.

- `RawMissionLogin`: 실제27h 요청 → CheckLogin/연결 재배치 → Begin/옵션/Player/Fort/응답/SP/시각/설정/Ready 순서다.
- `RawMissionClientPackets`와 `RawMissionServerPackets`: 같은 상태에8종 응답을 적용하고 현재 Player/metadata를 직렬화한다.
- `RawMissionLoginPackets`/`RawFortImage`: 실제 VFS로 다시 읽은 현재 Template의 원문 섹션을 저장 이미지와2ah 패킷으로 쓴다.
- `RawMissionBattleOptions`: 실제87바이트 옵션, 같은 월드의 mutable188TYPE,256바이트 사용표, 에너지 반경 제곱을 연결한다. Chat 숨김9바이트도 옵션+3e의 동일 물리 저장소다.
- `RawMissionSettingsPacket`: 실제 초기7DWORD를 각각 보관하고 현재값을 송수신한다. 원본 전역을 임의 Config 키로 대체하지 않는다. 이후 패널/디버그 UI의 전역 공유 연결은 남은 범위다.
- `RawLocalMissionState`와 `RawMissionLoginTimer`: 실제8→9→10 확정 뒤 수신 응답의13→14, 같은 State28 갱신, Ready 집계/시작/종료, 현재 사제 참여 재조회를 수행한다.

`LocalMissionTransport.Open(port,false)`는 서버 끝만 연다. 이때 클라이언트 송신은 연결 부재로 거부되며 Player 접속 플래그도 그대로다. 상태8의 `Connect`에서 실제 클라이언트 끝을 열고 상태9가 로그인 패킷을 큐에 넣는다. 성공 플래그와 Ready는 기존 원본 처리기들이 쓴다. 기존 `Open(port)`의 양끝 즉시 열기 계약은 독립 검사 호환을 위해 보존한다.

실제 Client는 원본 설정 키 **gameServerPort**(기본6799), 현재 벽시각/CRT clock, Chat, Renderer 재수집을 공급한다. 등록한 건설/운반체 수신과 후속 프레임의 Flush/Poll도 같은 로그인 전송을 사용한다. 월드/Session 교체 때 이 전송 객체와 큐도 함께 반납한다. TCP/SPX/원격 네트워크 지원을 복원했다고 선언하지 않는다.

## Early Mission에서 발견한 실제 누락

새 로그인으로 캠페인1-1/1-2는 실행됐으나 Early Mission1은 `shortState 실제DataManager힙확장미복원`에서 중단됐다. 기존 `RawPlayerTemplateState.Get`가 현재 State 길이28 이상만 지원하던 경계였다.

원본 `44d290`은 짧은 기존 State를 새 MemStream에 복사하고, 기존 끝부터 할당 끝28까지0으로 채운 뒤 현재 record로 교체한다. 새 stream의 position/end는 모두28이다. 다음 Get에서는 원본 lookup처럼 position을0으로 쓴다. [구현](../../cpppj/src/o/RawPlayerTemplateState.cpp)은 현재 실제 할당 callback을 요구하고, Session은 기존 원문이 복사 끝까지 살아 있는 별도 새 버퍼를 공급한다. 상태를 단순히28바이트0으로 교체하지 않는다.

[독립 관측기](../../tools/decomp_player_state_expand.py)는 보호 PE의 **44d290/44cce0 전체**를 실행했다. DataManager lookup, record/버퍼 할당·생성, 현재 record/이름 교체만 명시 경계다. **99개 입력,198개 정상 반환,20,700명령,53/64 정밀도 차이0**이다. GP/retN/canary/SEH/x87, 실행·쓰기 범위, 실제 복사/0 채움/position/end를 검사했다. 첫 C++ 재생 전 원문/도구/보고서를 동결했고 첫 재생부터99개가 일치했다. 기존 fixture는 수정하지 않았다.

없는/빈 State의 미정 stack 바이트를 포함한 초기 생성, 실제 DataManager 파일 저장은 여전히 명시 미복원이다. 이번에 사용한 것은 목표 미션 파일에 존재하는 짧은 State의 확장이다. 파일 이미지의 `Template.sections`와 별도 State stream을 저장 시점까지 완전히 통합하는 후속도 남아 있다.

## 검증과 실패 보존

로그 접두는 `extracted/goal-local-login-20261018-`다. 현재 제품·보호 SHA·빌드/검사/진입 결과는 [제품 보고서](../../cpppj/campaign-local-login.json)에 기록한다. 새 원문과 범위는 [player-state-expand.json](../../cpppj/player-state-expand.json)에 있다.

통합 검사는 Player 성공 플래그0에서 시작한다. 원본 처리기들이 접속/참여/Ready를 쓰고, 같은 실제 원본 상태8·9·10·13·14와27h/응답/설정 패킷을 소비했는지 확인한다. 이미 연결된 owner를 미리 만드는 종전 검사와 다르다. 실제 로그인 후의40h 이동도 같은 전송과 Session에서 검사한다.

첫 통합 실패는 Chat 추가가20채널의 자기 이름만 갱신한다고 가정한 검사였다. 실제 SetAlliance의40/80채널 갱신도 관측하도록 고쳤다. 첫 빌드의 지역 변수 그림자 경고는 이름을 고쳤고, 초기/수정 빌드 로그는 보존한다. 첫 Pump12개 다음에 자기 연결로 Begin/Ready(80000000h)가 추가 수신되는 원인도 추적했다. 원본4e2e30은 로컬 owner를 제외하므로 정상적인 추가 수신으로 합산하지 않았다. 기존 Session이 AssetPostPopState의 **BYTE localOwner를 const DWORD 참조 인자에 전달**해 임시 변환값의 사라진 주소를 Broadcast/CarrierSteal/CarrierTransfer/PlayerContentNotice에 보관하던 오류였다. 모두 실제 **PlayerWorldState.battleOwner DWORD**를 빌리도록 고쳤다. 세 생성자는 rvalue 참조 오버로드를 삭제했고, Player 내용물의 로컬 번호는 임시값을 거부하는 현재 getter로 바꿨다. BYTE 생성이 불가능함을 컴파일에서도 검사한다. 통합 검사는 첫12개 이후 불필요한 자기 수신이 없다는 검사를 다시 요구한다. Ready의 실제 sender/권한/local/mask와 Timer 송신 관측 로그도 보존한다.

기존 초기 화면 검사는 stderr가 비어야 한다고 가정해 정상 로그인/타이머 로그가 생긴 뒤 실패했다. `cpp_campaign_entry_smoke.py`는 원본 처리기의 확인한 상태/Player/타이머 로그만 정확한 목록으로 허용하며, 모르는 stderr와 실패 종료 코드는 계속 실패한다. 종료0과 실제 phase/paused/원본 객체/깊이 검사도 유지한다.

첫 전체 회귀는1,649개 중 기존 PlayerRegular의 두 판본 검사2개가 실패했다. 이 기존 관측은 MemStream 할당 진입에서 멈추며 할당 callback을 공급하지 않는다. 새 구현의 미연결 오류 문구로 바뀐 데 맞춰 테스트의 경계 문구만 갱신했다. 기존 원본 fixture/출력 기대값은 그대로이며 새 확장 본문의99개 대조와 구별한다.

두 화면 도구를 동시에 실행한 시도에서는 공유 options 파일의 보존/복구 검사가 충돌했다. 실패 로그를 남겼고 최종 화면 검사는 순서대로 실행한다. 보호5종 SHA는 변하지 않았다. 게임 실행 자체의 실패와 검사 도구의 공유 파일 충돌을 구별한다.

## 완료로 합산하지 않는 범위

- 상태7의 큰 전투 정리/전체 진행 UI 부모는 아직 기존 부분 `BeginMission`에 남아 있다. 이번 경로는 원본 로컬 Zone 이후의 접속8부터 기존 몸체를 연결했다. 일반 전투/재접속/simulconnect/정책 오류의 연결 해제·반납도 완전 복원이 아니다.
- Client Player UI 사건은 현재 Renderer 재수집으로 전달한다. 원본 `4d3090`의 모든 UI/안내 효과, 패널 설정 전역, 상태13의 모드와 입력 UI 전체는 추가 연결이 필요하다. 최초 이름/등록 핸들의 Client 전역 수명도 원본 부모와 더 대조해야 한다.
- zlocal client 주소의 family20/네트워크 바이트 순서 포트/플래그는4e0cd0/4e09d0/4e0950과 고정 `cnb` 문자열을 정적으로 확인했다. 서버 peer 주소 문자열의 모든 선택과 실제 원격 주소 수명까지 검증한 것은 아니다.
- 사용자 선택·강조·4277f0·42a360은 아직 직접 사제 이동을 대체하지 않았다. 건설/운반체 수신 등록이 실제 전송에 연결된 것과 실제 메뉴 명령의 전체 입력 연결을 구별한다.
- 전체11단계 자연 건설·경제·AI·전투·승패·저장·다음 미션·완주와 원본 동일성은 미완료다. 실제 로그인/CLI 진입을 사용자 자연 완주로 합산하지 않으며 goal은 active다.

## 최종 결과

Release build7 경고0·오류0, 전송4개/확장1개/기존PlayerRegular3개/실제로그인 통합1개가 통과했다. 전체 CTest는 **1,649개·실패0·종료0·216.82초**(CTest 216.85초)다. 독립 검증 감사88종은77통과/기존11실패/새실패0이다. 실제 화면은 두 도구를 순서대로 실행해 기존5진입+1사제이동 및 목표11진입을 통과했고 원본 불변 검사를 통과했다. 현재68보고서258핀만 갱신하고 과거 전체JSON을 역치환 대조한다. 보호SHA·최초동결3종·기존fixture·CMake접두·UTF8/LF·정확한인덱스바이트를 확인한 뒤 인계와 로컬 커밋을 수행한다.

기존 추적 fixture는 모두 git 내용과 일치한다. 추가 바이트 검사에서10개 파일의 작업트리는 CRLF, HEAD는 LF여서 줄바꿈 차이만 별도로 기록했다. 파일은 수정하지 않았으며 새99행 fixture/도구/보고서의 최초동결 SHA는 바이트 그대로 일치한다.
