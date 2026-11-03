# 제단 해방 부모의 현재 진단 복귀

단위828의 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며827 범위 정정은 `af2695ae / 20261104-09 CP`다. 최신 제품/집중 검사는828이다. 실제 포획/희생이나 해방 사제 생성은 실행하지 않는다. 부모00448bd0을 완료 집계하며 하위 사제 해방 본체를 실제 실행한 근거로 확대하지 않는다.

10.78 부모00448bd0은 현재 제단 좌표/다이스TYPE으로 typed finder를 호출하고 그 결과 포인터를 저장한다. 이후 현재 checking이 비영이면 방향 getter를 읽으며 첫 결과12가 아니면 두 번째 현재 결과11을 검사한다. 진단은 `!isInvisibleFrame() && !isSacrifice()`, `.\Dais.cpp`,0x578(1400)줄이다. 진단이 돌아오면 현재 풀 범위로 저장 결과를 검사하고 유효하면 기존00448500을 호출한다. 그 후 현재 제단의 가상 Unpop(flags0)→현재 TYPE의 P 첫번호를 rawDWORD에 표시 통지 없이 쓰기→현재 가상 Repop(flags0) 순서다. 진단이나 하위 해방이 finder 입력/위치/TYPE을 바꿔도 최초 검색을 반복하지 않는다.

새 RunAltarReleaseParent는 선택한 조회/진단/해방/공간/plain P 경계만 요구한다. RawAltarSacrifice.ReleaseFromAltar의 패치 경로는 같은 실제 DaisAt/Direction/Valid/ReleasePriest/Unpop·Repop/WriteFrame에 연결한다. 기존 CD 공개 경로는 유지하며 패치와 같은 별도 함수가 있다고 가정하지 않는다. 기존 ReleasePriest00448500의 altar.isValid/방향/boss 진단도 같은 장치가 정상 복귀하면 계속하도록 연결한다. 현재 권한은 복사본 대신 새 선택 getter로 실제 assetState.Boss()를 다시 읽는다. 독립 caller는 기존 상태 값을 유지한다. 이 사제 본체는 사용자 제외 검사를 대신 실행하지 않고 원문/기계어/정적 연결과 컴파일로 대조한다.

실제 RawCampaignRitual.inc는 이미 같은 Client checking DWORD에 DaisRegularState.debug를 Bind한다. 새로운 altar.assertion을 같은 services.ai.assertion에 연결했다. 유일한 원문 직접 caller는0044a6fa의 제단21 사건이며 현재 Handle의 같은 자동 동맹 해방 가지다. 이 사건을 실제 포획 상태로 실행하지 않는다.

독립 x86 관측기는 읽기 전용 원본 PE를 자신의 payload에 복사해 부모00448bd0만 호출한다. typed finder·방향·진단·00448500 해방 본체·가상 공간·TYPE getter는 명시 입력/관측 경계다. 실제 사제 생성/포획/희생·게임 초기화·창·통신은 실행하지 않는다. **1596행,진단468행**이며 재관측이 정확한 바이트로 일치했다. 첫/둘째 방향·checking0/상위비트/ffffffff, SID0/예약1/유효/끝/범위밖,현재 유효 범위,프레임DWORD, finder/방향/진단/해방/Unpop 뒤 checking·검색입력·범위·좌표·TYPE 변경과 전체50BYTE를 포함한다. fixture SHA-256은 `6d55205225413bd3cce63d6bcd36997244c2e07b9cef33334666a0cc432368ee`다.

최초 관측기 빌드는 같은 auto 선언에 uint8 TYPE과 uint32 record를 묶어C3538/종료2였다. 선언을 분리해 관측기만 교정했고 최초/최종 로그를 보존했다. 실패한 관측기에서 fixture를 만들지 않았으며 기존 원본/fixture를 바꾸지 않았다. 실제 해방/희생/사제 생성/강제 공격/승리·사용자 확인·MP/공중3종 전용·물리/PCM·전체71/34미션/CTest/ZIP 검사는 제외한다. 정상 유효 제단 메모리 밖/해제 슬롯의 직접 쓰기·전체 진단 UI/SEH는 별도 호스트 경계다. 원시는 `extracted/cpp-resume-828/`이며 보호 기준은2754개/fixture934개다. 기존2754개 불변/새 fixture 포함2755개·fixture935개다. Release 빌드3회와 최종 프로브는 종료0/경고·오류0이며 집중7개(새3) 실패0이다. EXE SHA-256은 `4c1855c351f59568bd4e9a71d64a2aedc7709e1816d8b214d35553df424c46b1`다. 확인 결과는 [828 JSON](../../cpppj/altar-release-parent-20261104.json)에 기록했다. checking19=부모11+부분5+MP분류3이며 완료부모31이다. 다음은 일반 숫자 확인004c9350의 남은 UI 진단 소비자다.
