# 실제 Client 미션 Start와 데모 시작 연결

기준 `30cc125`, 현재/마지막 정밀 디컴파일 PC **HJOW-Athlon / 2026-10-16**다. AGENTS와 양 LEFT를 다시 읽었다. 새 디컴파일·원본 게임 창·보호 원본 수정·DirectDraw 변경은 없다. 전체 플레이 복원 goal은 active다.

기존 실제 Client는 로그인/월드 설치 뒤 `RawCampaignMissionStartBindings`를 호출하지 않았다. 따라서1-1은 실제 SP0/기술없음이었다. 원본 스크립트의 **3000 SP/기술4종(`sunWalker;rainBattery;sunAviary;sunCannon`)**은 실제 Start에서 기록해야 한다. 객체 메뉴 연결보다 먼저 이 선행 누락을 해결했다. 메뉴 초안은 `extracted/goal-object-menu-client-20261018-drafts`에 보존하고 이 커밋에 포함하지 않는다.

원본 `004b6dd0`은 전투 음악 시작 뒤 `00463a40`으로 현재 Mission의 가상+28을 호출한다. Client도 같은 순서로 로컬 로그인 완료/같은 Session·Script에 기존 `004c2b20` 몸체를 실행한다. `MissionRuntime`은 같은 started/demo/start/tellAt 상태를 소유하며 World/Script보다 먼저 해제한다. SP와 기술은 현재 실제 저장소, AI는 현재 살아 있는 사제 기준, 미정600BYTE 입력은 실제 CRT 할당이다. 화면 갱신/카메라는 현재 Client/기존 원본 카메라다. 전용 명령Gump의 원본 초기0 분기를 유지하고 미연결 nonzero/세계randGeysers를 빈 성공으로 바꾸지 않았다.11개 보호 스크립트는 randGeysers를 요청하지 않는다.

미션 생성 `00482bf0`은 demo0, `StartDemo`의 `00463a00`은 현재 Mission+68에1을 직접 기록한다. `DemoStormRages` 스크립트의 demo키를 모드로 읽으면 일반 CLI MissionBegin까지 로컬 AI가 되어 잘못이다. 원본 메인 메뉴의 Demo/세 목록을 실제 활성화하고 같은 demo상태의 제목/안내/로컬 AI를 연결했다.15초 안내 뒤 실제 timeout으로 플레이 화면이 열린다. 기존 전체 FrameGate/Guide/Outcome 자동 실행과 전략AI는 별도 후속이다.

첫11진입은 GuardMyBack에서 AI 동맹 요청의 실제 전송 누락을 드러냈다. 원본 `0048f6a0`/`0040a780`은 버전1/종류3Dh/길이9의 ownerBYTE+maskDWORD를 서버9에 보낸다. 원본 `00491220` 전체와 실제 `00490ea0`을 **288입력·576정상반환·191484명령·53/64차이0**으로 C++ 전에 동결했다. 같은 실제 수신 버퍼의 자기bit→Player+54→방향표/현재Chat→broadcast 순서를 연결했다. 원문9Player1620BYTE/방향표324BYTE/packet9BYTE/효과 전체, GP/cdecl/제어어·빈x87/packet앞뒤canary/PE머리 불변을 검사한다. Chat·진단·broadcast는 명시 관측 경계다. 이전608개 원문을 새 관측 수에 합산하지 않는다.

초기 관측의 과거 분리 export 경로 없음은 현재 전체 정밀 산출물 범위를 읽도록 고쳐 native2를 동결했다. 첫 C++ 검사의 fixture헤더 파싱/뒤 범위 접근, 실제 통합 검사 할당입력 누락과 BYTE변환 경고/람다 인자 오류를 수정했다. 데모 첫 검사는1100프레임만으로 실제15초가 지났다고 가정해 실패했으며 원본 sleepPerLoop16으로 실제 walltime을 기다리도록 고쳤다. 모든 실패 로그와 동결 원문은 보존했다. 기대 출력을 구현에 맞춰 바꾸지 않았다.

Release build4 경고0·오류0, 집중 Start3/Player3(원문608+288 포함)/실제로그인Start1이 통과했다. **전체1653개 실패0/247.58초**(CTest247.61초). 감사90도구79통과/기존11실패동일/새0이다. 실제 일반11미션은 원본 시작SP/기술/demo0/로컬 AI없음을 검사했고1-1/1-2는 적AI도 확인했다. 실제 메인 Demo3은 demo1/로컬 AI/원본SP·기술과 자연15초 timeout 뒤 phase4/paused0을 통과했다. 기존5진입+실제1-1선택/이동도 통과했다. 각 도구의 originals_unchanged=true이며 GuardMyBack 실제화면 SP1200을 눈으로 확인했다. 진입/단일이동을 자연완주로 확대하지 않는다.

현재 소스·원문·보호SHA·실행/화면 증거·남은 경계는 [제품 보고서](../../cpppj/campaign-mission-start-client.json)에 기록했다. 다음은 실제 원본 객체 메뉴/Construct와 건설 커서·배치·생산/경제다. 전체11스테이지 자연완주/승패/저장/다음미션/동일성은 미완료다. 로컬 커밋만 수행하고 push하지 않는다.
