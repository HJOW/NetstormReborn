# cpppj 현재 Client 커서 장치·Gump 트리·강조 연결 — 2026-10-17

작업 PC는 **HJOW-Athlon**, 마지막 정밀 디컴파일 PC/날짜는 **HJOW-Athlon / 2026-10-16**이다. 기준은 `f69dc67`이다. 단위137은 기존 정밀 C와 보호 PE를 읽었고 새 디컴파일이나 원본 게임 실행은 하지 않았다. 전체화면 DirectDraw 수정은 제외한다.

## 현재 장치와 UI

`CampaignCursorClientEffects`는 현재 Client의 커서 장치, 번역의 원본 Windows-1252 바이트, 같은 FontStore/GumpText의 측정 문맥, 현재 마우스 폴링, 현재 Renderer의 dirty 사각형을 실제 세션 커서에 공급한다. UberGump가 미션 월드를 설치한 뒤 `SetEffects`를 실제 호출하고 표시기의 부모 깊이도 같은 UI 트리에서 읽는다. 해상도 변경 뒤에도 기존 Renderer 주소를 캐시하지 않는다. 선택 변경, 비용 안내, 기록, 실제 로컬 송신 등 미복원 소비자는 비어 있는 성공 콜백으로 채우지 않는다.

커서 Query의 `465ed0`은 루트 직계 Gump만 고른다. 전체 자식 탐색 `466020`과 구별하며 signed 깊이 `<1000`, visible/dead, 반개구간 경계를 기존 원본 처리기가 판정한다. `4651b0`은 자식부터 재귀 수신한 뒤 성공한 **토큰**을 반환한다. Drop의 가상 `+28`은 대상 자신의 **DWORD 반환값**을 사용한다. 허용 `542874=0200005f`, 수신 `542870=0200005e`, Query 수신 `5427f4=0200003f`를 보호 PE에서 구별했다.

원본 `cursorLegalGump->parentSid`는 Gump `+24`의 `object`다. UI 부모 토큰 `+14`, 별도 Form `+28`과 합치지 않는다. 깊이와 부모 SID를 요청할 때마다 현재 노드에서 읽고 폐기된 토큰은 오류로 거부한다. `466710`은 sticky 팝업의 Kill만 요청하므로 `RequestClosePopups`는 즉시 Cleanup하지 않는다. 기존 메뉴 callback이 실행 중인 스택에서 파생 노드를 반납하지 않는다.

호스트 UI는 원본의 장기 포인터 대신 화면 교체 때 새 토큰을 만든다. 마우스가 정지하면 원본 `446b50`은 Query하지 않기 때문에 이전 토큰이 남을 수 있다. 새 호스트 연결 `GumpsChanged`는 트리 교체 후·다음 수집 전에 기존 dirty/Query/dirty/커서 선택을 다시 실행한다. 좌표와 미리보기 계산을 새로 구현하지 않는다. 잠금/빈 커서는 건너뛴다.

## 강조와 실제 프로세스

`RawUserHighlight`는 `4d5480`의 마우스 TYPE 초기화와 `4d5bb0`의 강조 모드 변경을 복원한다. 강조 모드 `5cae90`, 선택 SID `5caea0`, 마우스 TYPE `5caea4`는 별개다. 같은 모드면 효과가 없고, 변경하면 모드 쓰기→TYPE0→현재 모드의 커서 선택 순서다. 모드1/2/3은 커서3, 모드4는2, 기본은1이다. 모드5는 역투영 전 선택 SID를 보관해 반환 두 float 비트와 force0을 이동 강조 경계에 넘긴다. 현재 Client 연결은 실제 커서가 요청하는 강조0만 소비하며 모드5의 실제 이동 강조 하위는 미연결 오류를 유지한다.

GameWorld는 같은 현재 선택을 참조로 빌려 주며 폴링 좌표도 공유한다. 강조0을 `Select(0)`으로 대체하지 않는다. Drop 꼬리 `refresh(0,0,1)`는 원본 `4d5c90`의 실제 선택 해제이며 단순 화면 갱신이 아니므로 이 큰 몸체는 여전히 명시 경계다.

Drop의 `cursorGump`는 실제 `cursorManaProcess` 포인터다. `+1c(3,0,0)`을 UI 닫기 함수로 보내던 연결을 수정해 같은 게임의 `game.lifecycle.processEvent`를 내부 재사용한다. 기존 Host/Kernel 검증과 실제 Mana/Range Notify가 종료 필드를 쓴다. 외부 UI의 `drop.closeGump`는 세션 커서에서 사용하지 않는다.

10.72의 강조 대응은 기존 정밀 C의 `41ad10`이다. 이번 강조 CPU 실행과 Client 연결은 10.78 전용이며 CD 자료를 수정하지 않았다. 공통 Gump 공개 경계의 두 판본 검사는 각 기존 판본 처리기를 사용한다.

## 같은 세션의 객체 메뉴 체인

`Session.MakeObjectMenus`는 기존 사제→Carrier→Damageable→공통 객체→Player의 **가상+a8 메뉴 작성 체인**을 한 고정 수명으로 소유한다. 같은 Pool/Factory/Player9/기술/SP/현재 HP/전체 MayPlace/운반 위치/실제 세션 커서를 사용한다. caller는 같은 원본 Gump 작성기와 실제 Chat 원문/번역/통신 경계를 공급해야 한다. 현재 owner/editor/특수 모드/미션은 값 사본 대신 getter로 읽고 기존 값형 fixture 경로도 보존한다. 반환 체인은 Session과 UI 트리보다 먼저 해제한다.

회수 계산과 생성자 VT 조회는 기존 `SquidReward.SalvageRefund`/`SquidFactory.ConstructorVtable`을 그대로 공개해 재사용했다. 보호 PE의 생성 VT83개 중81개의 가상+80은 기존 base `4ad0f0`이며 같은 `RawSquidLevel.Get`을 쓴다. 예외 `5076b8→44a480`, `507950→4478f0`은 명시 실제 파생 count 경계가 필요하다. 현재 raw VT와 해당 TYPE 생성자 VT가 맞지 않으면 성공한 기본 개수로 대체하지 않는다.

Player 메뉴의 기존 이름 `fort` 경계는 **미션 월드 `00594fa0`**를 읽으며, 요새 모드 `00594fb8`와 다르다. 같은 `core.bridgeState.mission`을 공급한다. 코드 검토에서 발견한 두 전역 혼용을 수정했고 실제 같은 fa0 상태만 바꿔 동맹 행의 차이를 확인하는 검사를 추가했다. `missionPost(00594fa4)`와도 별개로 둔다.

이 체인의 공개 `OpenPriest/Carrier/Damageable/Object`는 원본 종류가 이미 확인된 caller를 위한 **+a8** 입구다. 공통 **+ac `4adb30`**의 거부/다이스→제단 위임/제목·정보 `4ada70`/위치 `425f90`/GumpForm 객체부착 `475cf0`과 반환값은 아직 연결하지 않았다. 원본83VT 중29개는 다른 파생+a8이므로 일반 Damageable 메뉴로 대신 열지 않는다. 실제 우클릭 메뉴는 게임 시간을 정지하지 않으며 현재 UberGump의 비활성 임시 객체 메뉴는 후속 교체 대상이다.

현재 로컬 subscriber는 로그인 전 실제0이다. 이를 임의1로 바꾸지 않았다. 테스트의 peer22/Player2 기록은 Player 메뉴 경로를 선택하기 위한 명시 입력이며 자연 로그인이나 전송 성공의 증거가 아니다.

## Dais 미리보기의 삭제 경계

첫 메뉴 집중 검사에서 TYPE165 미리보기의 삭제 소비자가 없음을 확인했다. 보호 PE의 VT507950은 공통 Destroy `4af780`, Dais 선행 `44afd0`, 후행 `449580`을 가리킨다. 기존 정밀 C의 `44afd0`은 `(extra & 9)==0`일 때 큰 선행 효과를 실행하므로 extra bit8인 미리보기는 기존 DamageablePre로 진행한다. 이번 연결은 현재 runeState의 Dais TYPE·해당 VT·extra bit8이 모두 맞는 미리보기로 제한한다. 일반 살아 있는 Dais의 전체 수명을 복원했다고 주장하지 않는다.

후행은 같은 실제 Player9×180 기록의 +94/+98 쌍, +8c/+90 쌍 순서로 현재 SID와 일치하는 primary만 지우고 해당 mirror를 갱신한 뒤 기존 전체 DamageablePost를 호출한다. 다른 등록은 보존한다. 검사에는 한 쌍만 미리보기 SID로 두고 다른 쌍의 표식을 보존하는 입력을 추가했다. 이 변경은 빌드3에서 컴파일됐지만 메뉴 검사에는 별도의 선택 소비자 누락이 남았으므로 전체 메뉴 경로의 성공 근거로 삼지 않는다.

## 검증과 남은 일

`decomp_user_highlight.py`는 보호 PE 두 함수의 **178고유 입력/356부모 정상 반환/53·64비트 정밀도 차이0/6,296명령**을 관측했다. 커서 선택·역투영·이동 강조는 명시 경계이며 callback 뒤 현재값과 역투영 전 선택 스냅샷을 확인한다. 원본 반환 전역/효과 순서가 기대값이며 C++ 구현으로 기대 분기를 만들지 않았다.

새 검사는 실제 트리 선택·재귀 토큰/직접 반환 구별·현재 부모/깊이·지연 삭제, Client의 현재 폴링/바이트 인코딩/미공급 소비자 보존, 실제 Session의 선택 유지·Range 종료 통지·고정 마우스 UI 교체 후 수집을 다룬다. 명시 Form/GUI 입력은 생산 완료나 실제 제공 메뉴 행의 증거가 아니다.

메뉴 검사는 실제+a8 작성에서 제공된 건설 행의 원본 DWORD를 읽고 현재 SP 부족/성공 관문에서 같은 세션의 ContentForm 취득으로 진행한다. 지식 행과 명시 peer의 Player→Find Priest!도 실제 원본 Gump 사건과 InputQueue 정수 명령을 사용한다. 전체 공통Open/+ac나 실제 Client 명령 소비자를 검증한 것은 아니다.

Release 빌드2·3·4는 모두 종료0/경고0/오류0이다. 빌드1은 편집 중 소스 재컴파일이어서 루트의 해당 프로세스를 중지했다. 로그 접두부는 `extracted/goal-cursor-client-20261017-build`다. 빌드4는 마지막 메뉴 fixture 수정 직후 시작됐으며 사용자 최종 지시 뒤에는 완료 결과만 회수했다.

첫 집중 실행은27개 중2개가 실패했다. 메뉴의 TYPE165 미리보기 삭제는 위 제한된 실제 수명 연결로 수정했고, Range 검사의 잘못된 기대도 고쳤다. Range는 종료 통지 뒤 부모 미리보기 삭제가 종속 프로세스를 반납하므로, 마지막 Prepare에서 종료 필드를 관측하고 Drop 완료 뒤 Kernel에서 사라짐을 확인한다. 실제 Notify나 프로세스를 모의 객체로 바꾸지 않았다.

빌드3의 마지막 집중 재실행(`extracted/goal-cursor-client-20261017-focus3-{1..5}.log`)은 **27개 중26개 성공/1개 실패**다. RawButtonCursorBoundary4·UserHighlight3·CampaignCursorClientEffects1·CampaignSession_Cursor16은 모두 통과했고, CampaignObjectMenus3개 중 `CampaignObjectMenus_ActualChainAndBuildSelection`만 현재 선택 소비자 미연결 오류로 실패했다. 이 검사는 GameWorld 없이 Session을 사용하는 headless UI이므로 `MenuServices`에 같은 현재 선택 getter와 ClearSelection 소비자를 추가했다. **이 마지막 fixture 수정은 빌드4만 성공했으며 재검증하지 않았다.** 실제 GameWorld의 선택 getter를 기본0으로 바꿔 오류를 감추지 않았다.

이번 단위의 전체 CTest는 실행하지 않았다. 신규14개를 포함한 예상1,565개는 등록 예상치이며 통과 수가 아니다. 원본 근거48종의 마지막 성공 로그는 `extracted/goal-cursor-client-20261017-verify-final.log`이고 마지막 Dais 수정 전 결과다. 빌드2 실행 파일의 클론 진입 회귀5회도 종료0이며 1-1/1-2의 브리핑→Play Mission→월드, tutorial1/2/3의 브리핑 범위다. `extracted/goal-cursor-client-20261017-entry/results.json`은 `originals_unchanged=true`다. 마지막 수정 뒤 원본 감사/클론 회귀를 다시 실행하지 않았고 사용자 자연 완주를 검증한 것도 아니다. 현재 제품 SHA 갱신은 원본 대조를 다시 수행했다는 의미가 아니다.

사용자는 마지막으로 **수정까지만 진행하고 테스트는 다음으로 넘긴 뒤 인수인계·git 커밋 후 종료**하도록 지시했다. 이에 추가 검사나 구현을 시작하지 않고 이 문서와 `LEFT_JOBS.md`, `cpppj/campaign-cursor-client.json`에 미검증 상태를 인계한다. 완료한 서브 AI3명은 추가 작업 없이 대기한다. 로컬 커밋 메시지는 `20261017-37 CP 현재 커서 장치와 실제 Gump 및 객체 메뉴 체인 연결`이며 push하지 않는다. 커밋 후 전체 목표를 완료 대신 일시중지한다.

인계 시 정적 스테이징 감사는89파일·제품/검사31파일·기존53보고서의현재181핀·보호3SHA·UTF-8/LF·기존fixture/CMake/역사 보존을 확인했다. AGENTS/원본/DN은 변경하지 않았다. 이 결과는 Git과 자료 일관성 점검이며 보류한 제품 검사의 성공을 뜻하지 않는다.

재개 시 첫 순서는 AGENTS/두 LEFT·디컴파일 호스트 확인 후 `CampaignObjectMenus_` 집중 재검증이다. 실패 해결 뒤 필요한 전체 CTest·최종 원본 근거 감사·클론 진입 회귀를 수행한다. 재개 지시 전 이 검사를 실행하지 않는다.

실제 객체 메뉴의 공통 가상 Open과 입력 명령 부모, 생산 메뉴와 완료·경제·AI, 로그인된 로컬 전송 양 끝·큐잉·등록 수명, 전체 Kernel, Early Missions1~6/캠페인1-1~1-2/데모3의 자연 완주·승패·저장·다음 미션과 전체 동일성은 아직 미완료다.
