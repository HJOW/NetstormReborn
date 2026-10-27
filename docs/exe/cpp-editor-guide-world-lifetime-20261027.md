# Guide 교체와 이전 World의 State 반납 수명

단위484는 Test Battle을 연결하기 전에, Guide 교체가 편집 World를 너무 일찍 닫던 경계를 수정한다. Guide는 Script/미션 실행기와 그 미션의 지연 로드 요청을 반납하고, 실제 World·Player·AI·Deck·Kernel·HUD는 뒤따르는 State의 원문 정리 위치에서 반납한다. c2 일반 입력과 전체 Edit 왕복의 완료를 뜻하지 않는다.

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. AGENTS.md와 공통/닷넷 인수인계를 다시 읽었으며, 원본/CD·AGENTS·기존 디컴파일을 수정하지 않는다. 사용자가 작업 도중 실제 실행 범위를 구체화하여 메인 메뉴·스테이지 진입 후 몇 초처럼 완주가 필요 없는 검증은 실제 실행할 수 있다. 전체 완주와 사용자 개입 검증은 계속 뒤로 미룬다.

## 원문과 실제 연결

- `004d81d0`의 c2 시작은 snapshot → Guide.Begin → 새 현재 +64=1이며, Guide.Close 분기는 Mission 해제 → pending 이름 → 선택2 → RequestFortEdit다. 기존 [481 원문174행](cpp-editor-test-battle-parent-20261027.md)을 유지한다.
- `00463b20`/`00463b70`은 Mission을 반납하며 World 정리를 호출하지 않는다. `MissionTransitionEffects.release`에서 `ClearWorld`를 제거하고 실제 Guide 해제만 남긴다. 새 Mission 공장은 기존 Guide의 부재를 검사하되 살아 있는 World를 금지하지 않는다.
- `004c3290`은 첫 호출에서 `004bfb00`으로 Template9개 manager를 정리한다. Session이 살아 있다는 이유로 이 정리를 거부하지 않는다. 현재 Session은 고정 헤더와 조회 callback을 보유하며 manager의 FortTemplate 참조를 멤버에 고정하지 않는다.
- 새 Init의 요청/Zone은 현재 Guide가 소유한다. `ReleaseMissionFortLoad`는 Guide 교체/전체 종료에서 이전 요청만 취소하며, State의 이전 World 반납에서는 호출하지 않는다. 다른 호스트 UI 명령은 보존한다.
- 실제 `LoadMissionWorld`는 살아 있는 이전 World를 허용하고 기존 `CleanupNativeStateWorld(1)`에서 실제 Runtime 증빙/반납을 실행한다. 그 정상 반납 뒤 빈 World를 확인하고 새 Session을 만든다. 10.72의 기존 동기 어댑터는 판본 전체 State 복원 전까지 유지한다.

`RetireWorldForState`는 기존 Session을 빌리는 참조·표시 callback을 닫되, 새 Guide의 `missionFortRequest_`와 호스트 로드 큐를 취소하지 않는다. 기존 전체 State 정리 부모와 큰 외부 경계는 재구현하지 않는다.

## 검증 기록

최종 Release build9는 **경고/오류0**, 신규3개 포함 집중 **82고유/82관측·실패0**이다. 기존3,271등록 전부를 유지해3,274등록이다. GuideTransitions·MissionGuide·TutorialInit·FortRequest·작업본 source/재로그인·전체 State cleanup·Runtime 조정기·기존 Editor checkpoint42개·c2 부모를 검사했다. 전체 CTest는 이번에 반복하지 않았으며 직전483의3,271개/실패0과 구별한다.

최초 build1은 신규 검사의 연결표 필드명 `connectionGlobals`가 실제 서비스의 `connectionRegistry`와 다른 컴파일 오류1건으로 실패했다. build2의 MissionLifecycle 헤더 누락, build4의 poll 인자 누락, build6의 SP 전방 선언만으로 Store를 호출한 오류도 원시 로그에 보존했다. 신규 검사만 실제 선언에 맞춰 교정했으며 제품 소스는 build1 이후 바꾸지 않았다. 신규 fixture의 앞선 실행4회는 필수 메뉴 장치·공통 SP 초기화·첫 Init의 현재 Subscriber 복사·실제 커서 Cancel의 장치 연결이 차례로 누락돼 중단됐다. 실제 서비스 계약/공통 초기화를 추가하고 기존 SessionCursorUi의 관측 장치를 재사용했다. 기존 검사나 제품 관문·기대값을 약화하지 않았다.

신규 창 없는 검사는 실제 Client·표면·Player·Kernel·원본 Fort·작업본 reader·로컬 로그인/패킷을 사용한다. 최초 세대의 State7 후 미션 모드와 Init의 Subscriber 복사는 명시 입력이며, 다음 전환에서는 실제 전체 정리/Init이 같은 전역을 갱신한다. 살아 있는 세션의 Template 반납→Guide 요청/Zone 보존→전체 cleanup/실제 Runtime 반납→동일 요청으로 새 작업본 로그인과, finish 없는 Guide.Close/요청 owner 반납을 확인했다. 전체 State cleanup의 게임 자식/Runtime 반납은 실제 본체를 호출하며, HUD 교체/표시 장치는 명시 외부 관측 경계다. 이 검사를 실제 Editor 설치 출발/Test Battle 왕복 또는 자연 완주의 증거로 합산하지 않는다.

같은 최종 게임의 실제 실행도 추가했다. 읽기 전용 원본을 새 격리 폴더에 복사하고 `--no-audio`로 실행했다.

- **메인 메뉴:** 최초 안내 닫기→원본8버튼 표시→종료0, 시간 상한 미도달. 출력 PNG를 직접 확인했다.
- **Early1 실제 메뉴/Restart:** Campaign→Early Missions→Bridge the Gap→MORE/OK→수초간 플레이→ESC/Game/Restart Mission→새 브리핑→MORE/OK→다시 수초간 플레이를530프레임/약13.97초에 종료0으로 통과했다. 재시작 전후 실제 Guide/Session·정지 해제와 새 Fort 이름 revision2→3을 관측했다. 다리/이동/채취/미션 클리어 입력은 하지 않았다.
- **별도 실패 보존:** `--mission TheWarBegins`로 직접 진입하면 로그인14/카메라 이동 뒤 첫 표시에서 `원시 bitmap의 보이는 할당 밖 BYTE 공급 필요`로 종료1이다. 실제 소비자는 RawMinimapControls의 crop 블릿이다. 이 경로를 다른 미션 성공으로 대체하지 않으며 다음 수정 대상으로 남긴다. 아직 같은 스테이지의 메뉴 경로와 대조하지 않았으므로 원인을 CLI 시점으로 확정하지 않는다.

세 실행 모두 원본 전체 해시는 같았고, 두 성공 실행은 같은 게임 SHA를 사용했다. 소스/검사2,846파일은 최종 빌드/집중 검사 뒤 동일하다. 원본/CD/기존 검사3,194파일에서 기존 검사 CMake와 CampaignSessionTests에 끝 등록만 추가한 것을 제외한 바이트를 보존했고, AGENTS도 동일하다. 두 기존 파일의 원래 전체 접두부는 각각162,466/759,379BYTE다.

최종 게임 SHA256은 `d72c785dbdafa1aa9458310de0be785415502e112459e702e209bfdb37f7396d`, 검사 실행 파일은 `d6af750ac1f7a68ff0fb67e4dc79e4a2df198f89925951b60cbca00927370ba1`이다. [실행 보고서](../../cpppj/editor-guide-world-lifetime-20261027.json), 원시 `extracted/cpp-editor-guide-world-484/`을 본다. 신규 Native CPU 관측은 없으며 기존 fixture를 집중 검사에서 재생했다.

사용자 요청으로 커밋 전에 원격 문서 변경을 반영했다. 로컬9파일을 복구용 stash `f3b5f156…`에 보존한 뒤 `git pull --no-rebase`로 `93c054f1`에서 `bbbb69f3`으로 fast-forward했다. LEFT_JOBS.md는 자동 병합됐으며 양쪽 기록과 국·영 README 변경을 보존했다. pull 전후 소스/검사2,846파일의 바이트가 같음을 확인한 뒤 신규 검사의 직접 헤더 연결을 이어갔다. 원격 push는 하지 않는다.

## 남은 연결

이 수명을 기반으로 c2 일반 입력에 작업본 namespace·실제 snapshot/Guide/편집 예약을 연결해야 한다. 실제 Editor 출발→Init→State7→Test Battle→Close→State1 복귀, Add Island GUI/저장 재진입, 전체 미션 자연 완주와 화면/음향 동일성은 계속 남는다. 짧은 실행은 허용된 범위에서 검증하고, 전체 완주와 사용자 개입 검증은 뒤로 미룬다.
