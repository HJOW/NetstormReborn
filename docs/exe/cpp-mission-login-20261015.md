# 실제 미션0x27 로그인 부모와 Ready 송신

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. 같은 PC의 전체 정밀 함수 범위를 사용했다. 원본 게임·복사본·GUI·OS 소켓을 실행하지 않았고 보호 자료와 AGENTS.md를 변경하지 않았다. 10.78 PATCH의 선택된 미션 분기다. CD10.72의 대응 `004c1a20`을 이 단위에서 관찰하지 않았다.

## 구현한 실제 순서

[`RawMissionLogin`](../../cpppj/src/o/RawMissionLogin.h)은 같은1620바이트 Player9, `RawPlayerParticipation`, `PlayerWorldState`, 원본 전역 참조 및 [타이머와 공유하는 실제 집계](../../cpppj/src/o/RawMissionLoginTimer.h)를 빌린다. caller가 참여 처리기와 로그인에 같은 Player9를 공급해야 한다. 별도 접속·Ready·시작 성공 사본은 없다.

| 원본 | 실제 연결/동작 |
| --- | --- |
| `004a7800` 시작 | 서버가 아니면 실제 disconnect 후 반환. 현재 subscriber의 첫 Player를 찾고 known0이면 `0048f3b0`의 양수owner 쓰기와 실제 CoreData10바이트 선택을 실행. owner 미발견은 pending 연결의 가상 송신 경계에 실제 거부 응답 전달 |
| `004a6dc0` | **기존 flags bit2를 먼저 보존**한 뒤 실제 `RawPlayerParticipation::CheckLogin` 실행. 반환0도 원본에 없는 조기 반환으로 바꾸지 않음. 참여·접속3 쓰기/해제100 제거 및 실제 연결 재배치·거부응답·해제는 기존 몸체 사용 |
| `004a6990`, 옵션 | flush→LoginBegin34바이트→flush→현재87바이트 옵션0x67→`00490a00<2`이면 필수 실제 옵션 소비→flush. fortCount0이면 필수 허용타입표 초기화→flush |
| `004a6a70`, `004a6b40` | subscriber가 있는 현재 Player1..8 데이터→flush→현재 중앙 Fort metadata 경계→flush |
| `004a6c50` | 새 로그인에서만 현재 Fort 순서/예측수/마지막owner 기록. server이면서 현재local이면 **현재 Fort 파일**을 그 Template에 읽는 필수 경계, 그 밖에는 요청 image의 실제 위치·크기를 전달하는 필수 경계 |
| `004a6ab0`, `00491270` | subscriber/IP→현재 flags3→가입한 Player를 현재 접속자에게 전송→사람 참여 Player를 새 대상에게 전송. 두 순회는 합치지 않고 각 callback 후 현재 기록을 다시 읽음 |
| `004a6920`, `004a6e70` | 실제18바이트 성공 응답→flush→현재1..8 SP 비트 조회/CRT 절삭으로44바이트0x81→flush. SP owner0 슬롯의ff도 보존 |
| clock/시간 | 현재 CRT clock 저장→이전 clock과32비트 감김 차이/1000→실제0x84 패킷을1..8 모두 요청→flush→가입Player+64=1 |
| `004a7240` 첫 순회 | 현재 Fort 순서별·현재 참여자별 실제 Fort 송신 경계→매번flush. 새 자기 Fort는 첫 순회에서 제외. 다른 현재local이면 실제 peer동기화 경계→flush |
| 새 로그인/`004b5300` | 현재 접속자 모두에게 새 Fort 재전송. 마지막Player의 예측수 진단은 각 방문에서 유지. **missionWorld 새 로그인 위치에서만 필수 loadWorld** |
| `0044dbc0`, Ready | 실제 설정패킷 경계→flush2회→그때의 **현재 battleOwner** Player+50에40→현재1..8 mask의0x6e8바이트→flush→현재 subscriber수/dropped수로 Begin1 선택 |

LoginBegin0x29, 옵션0x67, SP0x81, 시간0x84, Reply0x28, Ready0x6e의 전체 송신 바이트를 작성한다. 가변 패킷은 실제 송신 observer가 확정하는 선언 길이까지 적용한 바이트를 소비자에게 전달한다. 외부 `send`는 실제 연결 존재·접속 gate와 전송 수명을 담당한다. `SendReady(0)`은 별도 필수 `broadcast`에 전달한다. 이 값0을 일반 `send(0)`에 넘겨 버리지 않는다.

count는 구분한다. `00490ab0=Subscribers`는 subscriber!=0만 센다. `00490a00=Participating`은 subscriber!=0 && flags2다. `00490bc0=Disconnected`는 subscriber!=0 && flags2 && flags100이며 Ready 수가 아니다. 최종 Begin은 Subscribers==1 또는 Subscribers−Disconnected==1일 때만 쓴다.

## 실제 서비스와 남은 경계

큰 몸체 `applyOptions(004b4900)`, `clearTypes(0041c320)`, `sendPlayer(0048f9f0)`, `sendFortMetadata(004a6b40)`, Template 파일/이미지 읽기, `sendFort(004a7240)`, `synchronizePeer(004a7350)`, `loadWorld(004b5300)`, `sendSettings(0044dbc0)`는 선택 위치의 **필수 실제 callback**이다. 미등록을 빈 성공으로 처리하지 않는다. 앞선 부분 효과가 실행된 뒤 경계가 없으면 그 상태를 보존하고 오류를 반환하며 Ready/Begin을 합성하지 않는다. 단독 미션에서도 실제 선택되는 옵션 적용·자기 Fort 전송을 생략하지 않는다.

`missionWorld/missionPost`가 모두 참인 선택 경로만 복원했다. 일반 Battle 섬, simulconnect, LoginBegin의63000바이트 디버그3회 분기는 명시 거부한다. Config0054db50을 로컬이라는 이유로 임의true로 바꾸지 않는다. 등록 함수 `RegisterMissionLogin`은 버전1/정책0/실제0x27 callback을 등록한다. 아직 Session/GameWorld/Client에 자동 연결하지 않았다.

현재 후속 연결 위치는 다음과 같다.

1. GameWorld 생성자의 현재 `campaign_->Load()`를 제거하고 이 처리기의 `loadWorld`가 같은 Session.Load를 호출하게 한다. 앞선 Template 파일/이미지 준비와 실제 전달 패킷 소비자도 같은 수명 안에 연결한다.
2. [로컬 전송](cpp-local-mission-transport-20261015.md)의 실제 pending−1→owner 재배치를 CheckLogin hook에, send/pendingReply/flush를 실제 두 끝에 연결한다. 서버가 보내는0x29/0x67/Player/Fort/0x28/0x81/0x84/0x6e를 소비하는 실제 client handlers가 필요하다. 미등록 packet을 조용히 버리는 분배 결과를 로그인 성공으로 세지 않는다.
3. 현재 벽시각의004b67c0 타이머→실제 사제 상태8회→진행창 종료를 연결한다. Ready40 자체가 타이머 종료나 Guide 시작은 아니다.
4. Guide00463bb0의 실제 pending005c8878와 dialogs00565dd4 gate 뒤 sequence2(004c2b00)의 blank Tell/advance→sequence3 [RawMissionStart004c2b20](cpp-mission-start-20261015.md)를 실행한다. 미션 Config의 같은 SectionNames/Set, 직접SP 지정·현재기술·사제·AI600바이트·명령Gump의 실제 소비자가 필요하다.
5. 현재 Uber의 즉시A. 표시/blank 즉시재개/모든Tell 일괄정지는 [RawMissionFrameGate](../../cpppj/src/o/RawMissionFrameGate.h)의10연속프레임·PauseForDialog·7연속프레임 수명에 연결해야 한다. Guide는 원본의 메시지후·입력전, 로그인 타이머는 입력후·Kernel전 호출 위치를 보존한다.

## 원본 대조와 현재 검증 상태

[`decomp_mission_login.py`](../../tools/decomp_mission_login.py)는 전체 정밀 함수표에서 검토한 실제 범위만 Unicorn으로 실행한다. [`fixture`](../../cpppj/tests/fixtures/mission-login-x86.tsv)의 **48고유입력**은 서버부재·subscriber거부·known/현재owner·새/기존bit2·CheckLogin 거부 후 계속·복수참여/해제·callback의 현재Player 및 worldLoad후owner변경·clock 감김을 포함한다. `--verify`의 고유성/SHA 검사와 UTF-8/후행공백 검사를 통과했다.

실제004a7800/양수owner지정/CheckLogin/Player두순회/Template선택·순서쓰기/집계/Begin·SP·Reply·Ready helper와 가변 observer를 실행했다. **전체 Player1620바이트, 현재 전역8개와 CoreData 선택identity, 모든 송신 packet바이트, 외부효과 전체순서, cdecl 반환스택과 EBX/ESI/EDI/EBP 보존**을 기록한다. Template/metadata/Fort직렬화/worldLoad/옵션/SP저장소/CRT clock/벽시계/실제 전송은 명시 외부 입력·효과다. 따라서48은 로그인 부모의 선택된 경계 대조이며 실제 게임 전체나 TCP/GUI까지 실행한 결과가 아니다. 처음 strict guard가 멈춘004e78d0을 실제 CRT clock 주소로 확인하고 명시 시계 경계로 분리했다. 넓은 코드 허용으로 우회하지 않았다.

후속 client 등록표 감사에서 최초 도구가0x67 observer를 `0040aed0`으로 선택한 오류를 확인했다. 원본 등록 주소는 **`0040ae80`**이다. 두 몸체의 정상길이 계산은 같지만 진단 줄번호가 다르므로 root의 동결 해제 확인 후 검토 범위/호출 주소를 모두ae80으로 교정했다. 같은48입력을 다시 실행하고 fixture SHA가 전후 **`d57f9122eb04350eb4eceb141b64ea9d3e4d63ac4b06066e7206902126e24790`**로 완전히 같음을 확인했다. 제품 C++/검사는 변경하지 않았다. root가 추가한 `product_validation` 기록을 재생 보고서에 보존했다.

[`MissionLoginTests.cpp`](../../cpppj/tests/MissionLoginTests.cpp)는48입력의 전체출력을 재생하고 필수 metadata 누락의 부분효과/Ready미실행, 실제 등록, 미복원 모드 거부, Ready의 별도 broadcast를 검사한다. 소스 고정 뒤 **Release 경고0·오류0, 이3개를 포함한 집중60개 실패0·전체 CTest1/1·내부1,283개 실패0·216.33초**를 확인했다. `extracted/goal-mission-entry-final-{build,focused}.log`, `goal-mission-entry-{ctest,tests}.log` 및 [`report`](../../cpppj/mission-login.json)에 결과/소스 SHA를 보존한다. 게임 자연 완주·AI전략·모든 입력/픽셀/음향 동일성은 미완료다.
