# dotnetpj 0.535 — 연소 생성·입자·연소점과 현재 대상/SID 진단

2026-11-15, HJOW-Athlon. 직전0.534는4183b7f94다. cpppj RawSidRobust.h/.cpp는8b467ca03249dbc963ceaf8efdd35fa6e26986b7, RawBurningVictim.h/.cpp는f4d1312fa823513787f7e3e63576636fd6560531, RawBurningVisual.h는db9b626c0a9c6b9440c6b181dfc401506a07d3e4/.cpp는961f6be53a319c5c9cf78c7cc8eb2fdd007938a7, RawBurningSourceCount와RawBurningSources의h/.cpp는247855746f40366e6c45c9c2b97dc378f0cb8bf8이 기준이다. 디컴파일 산출물은 변경하지 않았다.

## 구현

RawSidRobust의 signed1..65534 번호 부모와 선택된 상세raw 진단을 함께 옮겼다. live0/checking0은 Pool에 접근하지 않고, 상세 진입 때만 최근진단0→선행TYPE저장→같은base하한/상한→현재필드 실패bit 순서를 실행한다. 상세 진단이 복귀하면 번호 부모는1을 반환한다. 정렬/세대/void/TYPE표상한 관문을 합성하지 않는다. 32비트 주소 토큰의 감김과64비트 호스트 주소 산술을 명시 구별하고 선택된 실제 메모리 범위는 주입 장치가 검증한다. C#은 unsafe 포인터 대신 ulong주소/ReadOnlyMemory 경계를 사용한다. 제품 Host의 상세 물리메모리 어댑터는 아직 미연결이다.

RawBurningVictim은 checking 뒤 첫robust/복귀진단·항상 두번째robust·그후 선택SID고정·캡처VT·HP표식 순서를 유지한다. const Sid&의 현재주소는 Func<int> 조회로 표현한다. 각 부모의 원문 식/파일/줄은 그대로 보존하며 기본checking0에서 없는 상세 장치를 선행 요구하지 않는다. MakeHooks는 같은 raw의 현재VT/TYPE/extra와 교체 가능한 실제 진단을 읽는다.

RawBurningVisual은 공통 등록→의미필드 초기화→150*24입자zero→필요한 첫 공유팔레트의 순서를 이식했다. 진입 Victim과 부착 Parent를 구별하고 모드는 임의DWORD까지 보존한다. 함수 묶음은 복사하며 공유 팔레트 owner는 빌린다. Gradient 뒤 BuildPalette 인자도 현재 토큰을 다시 읽는다. Append는 여유칸/첫만료칸의 여섯필드를 쓰며 PATCH ordered float<=0과 CD signedDWORD비교/NaN차이를 구별한다.

RawBurningSourceCount/RawBurningSources는 저장한 이전HP→실제현재TYPE 최대HP/복귀진단→같은 선택raw의 현재TYPE 강제1→행/픽셀/난수/Append를 실행한다. 원문 DWORD곱/합/차 감김과 signed IDIV의0/몫overflow를 보존한다. HP getter 진입 때 선택한 bool/TYPE주소는 진단이 어댑터 포인터를 교체해도 유지하며 그 주소의 현재값은 복귀 뒤 읽는다. 현재checking·장기 GameAux의 PriestQuarterHpBox/PriestTypeBox/DaisTypeBox를 연결한 검사도 포함했다. 300번째 시도의 원문 후위감소,7점 폴백, PATCH/CD 픽셀/row단락 순서, Append실패와 행/selected 개수의 구별·제거 뒤total/selected 보존을 옮겼다.

## 검사

SID상세/번호1012, 연소대상504, warm생성256, 입자564, 연소점652, 현재TYPE HP/정수5652의 총8640고유원본입력을 대조했다. 연소점652입력은 공유약화owner에 반대 fallback을 둔 채 한 번 더 재생해 총9292실행이 일치했다. HP정수5652에는 정상4764·0나눗셈864·signed몫범위24입력이 포함되며 결과뿐 아니라 진단/현재HP·TYPE·피해SID·장기설정·난수/풀불변을 확인했다. 원본/golden/fixture를 변경하지 않았다.

추가 현재 검사는64비트 주소 보존/선행물리읽기·미선택장치/DWORD 불변, cold팔레트 한 번 생성/현재토큰/함수사본과 미연결 부분효과, 양판본의 현재VT/HP/진단교체, 진단중pointer재연결 뒤 선택한 장기주소 보존을 확인했다. 처음부터 원본14검사가 통과했고 현재/cold 추가 뒤19/19통과했다. 실장치/게임창/실제포획·희생·특정적공격·승리·MP는 실행하지 않았다.

SID 선행고정과 진단 뒤 약화주소 재연결의 대표변이는 각각19검사 중1실패로 검출했다. 각 소스 바이트/SHA를 복원했다. 원복 뒤 전체 비증분Release빌드 경고0·오류0, 관련 Core183/183·Client23/23은 실패0·건너뜀0·각 종료0으로 통과했다. 이번 집중 결과는 최근 전체회귀0.529와 구별한다. 로컬자료는 extracted/dotnet-burning-0535-mutations/와 extracted/dotnet-burning-0535-final/이다(Git제외).

## 남은 조립

실제 연소 프레임/캡처·그리기/수명·RenderFrameGate·BurningVisualSystem과 제품HostSession의 현재권한/그래픽/소리 결선은 후속이다. 이번 실제 생성/선택 모듈 대조를 제품의 미션 표시/플레이 완료로 합산하지 않는다. 다음0.536에서 같은 공통표시/Host/Kernel에 프레임·삭제를 연결한다.

PORT_MAP836행(L1 408+port428), fixture287개(6개추가). 전체floor/reflected커밋은 유지했다. 새 외부접속/NB수정은 없다. AGENTS.md·보호 원본/CD·README개요·cpppj·LEFT_JOBS.md·다른 프로젝트 변경 없음. 목표active·M1전체0/34·후속 단계 잔여다.
