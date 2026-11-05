# 현재 CoreData 복원과 두 진단의 정상 복귀

단위895는 `0048fb30/CD00406a90`의 Core 복원을 `RestorePlayerCoreData`와 `ClientCoreDataGlobals::Restore`로 구현하고 실제 두 caller를 같은 장기 Client에 연결한다. **코드/정적 연결은 완료했으며 새 복원·MP·패킷·통신 실행 검사는 사용자 지시로 제외했다. 전체 목표는 미완료**다. 현재/마지막 전체 정밀 디컴파일 호스트는 **HJOW-Athlon / 2026-11-03**이다. 보호 PE와 전체 정밀 산출물은 읽기만 했다.

missionPost1은 다른 load/장치 없이 반환한다. 패치는 known→현재 Player base/보존BYTE→local/remote 선택, 현재 checking 캡처→선택 active1→subscriber/9기록 ID 비교를 원문 순서로 수행한다. 두 진단은 `Player.cpp`0x224/0x21f와 정확한 표현식을 전달하고 정상 복귀하면 Player base만 다시 읽는다. 이미 선택한 local/remote는 재판정하지 않는다. 기록 Core10을 Player에 DWORD/DWORD/WORD로 순서대로 복원하며 겹치는 물리/부분 실패에서 이전 store를 보존한다. local만 현재 myPlayerId와 현재 base로 current를 다시 지정하고 remote의active/current는 유지한다. 후행 savedGame이 비영이면 처음 보존한BYTE를 현재 Player base에 돌려쓰고 실제0042e050으로 Chat을 재초기화한다.

CD는172BYTE stride/Core+100/보존BYTE+105이며 패치는180BYTE/+108/+113이다. CD는 처음 읽은 Player base/localOwner를 유지하며 checking 진단/Chat 재초기화가 없다. 논리 owner 상한을 새로 강제하지 않고 원문 DWORD 주소 감김과 선택된 load/store의 물리 경계를 구별한다. 원문 범위밖 주소와 C++에서 표현할 수 없는 current10BYTE는 기존 명시적 호스트 오류다. lastKnown10BYTE와9×16BYTE 기록을 합치거나 초기화하지 않는다.

`RawCampaignSessionLogin.inc`는 두 원본 caller(004912d0/004a7800)가 같은 `ClientCoreDataGlobals`의 기록/Player/current/active와 같은 checking owner를 사용하도록 연결한다. 저장 게임의 선택된 Chat은 실제 `InitializeClientChatManager`에 연결했으며 화면 invalidate로 대신하지 않았다. 일반 로그인 부모의 기존 미복원 MP 관문은 유지했다. 이 하위 연결을 주변 MP 부모 전체 구현/안정성 검증으로 합산하지 않는다.

Release 제품/집중 검사 빌드는1회 종료0·경고/오류0이다. 새 집중 target은 기존 getter/선택3·9기록3·장기 owner2, 합계 **기존8사례/1명령/8실행·첫 실패0**이다. 기존 PE804/186행도 그대로 재생했다. 새 복원 본문/Chat/MP 호출·Native/fixture 실행은0이다. 기존 보호2774개/fixture954개 변경0·기존 기대값 변경0이다. 최종 코드는 최초 빌드 전 snapshot과 같으며 교정/재빌드0이다.

checking 잔여는 **8=부모3+부분2+MP3**, 완료부모39/완료부분4다. 이번0048fb30은 부분 항목1개를 코드/정적 완료했다. 추가로004c0e20은892의원문378행/부품과894의실제두caller/같은checking연결로이미구현됐으므로이번에잔여집계에서제외했다. 이 집계 정정은 새 실행/MP 검증 결과가 아니다. 남은 부모는004a7510/004b02d0/004dc220, 부분은004b54e0/004b8cb0, MP분류는0042bc10/004b4df0/004b8b20이며779 당시51개=완료43+잔여8을 유지한다.

최신 활성 솔루션은74프로젝트(Application69=검사이름64/게임1/probe·dump4, 라이브러리2, Utility3)다. 기존 소스의집중실행파일1개를추가했고새검사사례는없다. 남아 있는 오래된vcxproj파일 수를 활성 target 수로 쓰지 않았다. 779 당시 전체 컴파일/549 당시 전체 실행의 이력은 현재 집중 통과와 구별한다.

제품은 **895 / 파일·제품 버전11.01 / 10696704BYTE**, SHA-256은 `24a2373d030cadee64e39fdbc5cf20cea15ab2bc6ce81766a1f8127f8766c5f0`다. [JSON](../../cpppj/player-core-restore-current-20261105.json), Git 제외 `extracted/cpp-resume-895/`에 전체 소스/원문/빌드/검사/보호 근거를 보존했다. 다음896은 공통 Pop004b02d0의비멀티잔여와MP메타맵2표분리를정적으로재감사한다. Factory/State/파일·객체UI/Edit·Windows·무검사MP/4~8차는 계속 남는다. 게임/GUI·포획/희생/특정 적 공격/승리·공중3종·물리입력/PCM/CD·전체C++/CTest/34미션/ZIP 반복과push는하지않았다.
