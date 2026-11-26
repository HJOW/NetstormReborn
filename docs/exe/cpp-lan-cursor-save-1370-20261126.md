# LAN 내려놓기 뒤 복구 저장 연결 — 단위1370

2026-11-26, HJOW-Athlon. 마지막 디컴파일 PC도 HJOW-Athlon이다. 기존 C004473e0/004b54e0을 정적으로 읽었으며 새 디컴파일·원본 실행/변경은 없다. 사용자 요청에 따라 이 단위 검증·문서·커밋 뒤 목표를 일시 중지한다.

첫 실제 LAN 검사에서 사제→Construct→Wind 신전5000→허용 위치 배치까지 진행했으나 `캠페인 커서 실제 경계 미연결: 전투 표시 갱신`으로 호스트가 종료1이었다. 원문의 해당 호출004b54e0은 비용 차감/표시 뒤 재접속 상태를 저장한다. 미션은 missionPost 관문으로 이 호출을 생략해 앞선 미션 검사에서 드러나지 않았다.

커서의 역사적 `battleRefresh`를 원문 의미로 주석 교정하고, 실제 작성자를 `CampaignCursorGame.saveReconnect`로 공급했다. Session의 같은 SP/Player·복구 슬롯과 기존 사용자 파일 작성자를 사용하며 UI 효과 교체로 저장 연결이 사라지지 않는다. 원문의 battle/!missionPost 관문과 호출 순서·파일 예외/부분 쓰기는 유지한다. 게임 상태/ACK/비용·저장 성공을 주입하지 않는다.

Release 성공. 커서 내려놓기의 기존 양판본/추가1037·원시 DWORD/호출 순서와 실제 저장 경로 회귀를 집중 실행해12/12 통과했다. 새 검사 실행 파일은 `netstorm_cursor_drop_tests.exe`이며 [사용법](../testing/cpp-lan-opponent-guide.md)에 기록했다. 기존 fixture는 수정하지 않았다.

재검증은 새 두 프로필/실시간 TCP에서 동일 메뉴와 실제 유효 배치 위치를 사용했다. 양쪽 owner1 SP가6500→1500으로 바뀌었다. 호스트 `Saves/!state.dat`976BYTE의 owner1·SP1500을 확인했고 당시 파일을 `after-drop-state.dat`로 보존했다. 20초 자연 대기 뒤 양쪽에 SID15265/자산windVortex/owner1/좌표129,212/HP5000이 일치했다. 별도 시점의 애니메이션 프레임2/6을 제외한 객체 전체 필드가 일치했다. 두 게임 모두 종료0이다. [호스트](../screens/cpp-lan-cursor-save-1370-20261126/host.png)·[상대](../screens/cpp-lan-cursor-save-1370-20261126/bot.png).

첫 집계 필터는 이름에temple/builder/construct가 있는 행만 골라 신전의 실제 자산명windVortex를 누락했다. 원시 보고/화면·메뉴 TYPE123을 대조해 이를 교정했으며 최초 result.json의 빈buildings배열도 그대로 보존했다. 제품 결함이나 신전 미생성으로 판정하지 않는다. 전체 유닛 생산·추가 건설·재접속의 완료로 넓히지 않는다.

EXE SHA-256 `c3daa5bfbebfbcb91ab1b95965f9cecb745e17ec1110e9b574080229227249b7`. 보호3212파일·기존 CP fixture 불변. [검증 JSON](../../cpppj/lan-cursor-save-1370-20261126.json). 원시는 `extracted/cpp-lan-1364/production-1370-1`/`2`, `build-cursor-save-1370.log`, `tests-cursor-save-1370.log`, `retest-production-1370.py`다. 재검증 입력은1024×768/영어에서 우클릭520,384→Construct→Temple→Wind항목900,407→일반 pointer로 유효600,420 확인→같은 위치 클릭이었다.

다음 재개1371은 유닛 생산/추가 건설·이탈 뒤 새 참가자 재접속과 최종 ZIP 갱신이다. 최신 ZIP1362는1363~1370 수정 전이다. 포획/희생·특정 공격/승리·실물 LAN·명시 사용자 확인은 실행하지 않았다. [DN-MP-07](../dotnet-cpp-lan-followup-20261126.md)에 원문 저장 의미·호출 순서·완료 기준을 기록했다. DN 코드/실행/진행률은 변경하지 않았다.
