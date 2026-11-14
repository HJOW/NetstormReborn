# dotnetpj 0.511 — 공통 Pop·Activate 현재 부모

2026-11-14, HJOW-Athlon. 기준은 `b2c41b42b`의 `o/RawSquidPopParent.h/.cpp`·`RawSquidActivate.h/.cpp`와 독립 부모 검사다. 기존 일반 Pop·Activate 어댑터는 아직 교체하지 않았으며 새 부모만 실제 물리 자료로 검증한다.

## 구현

`RawSquidPopParent`는 PATCH 입구의 범위/void/contained 조기 반환 뒤 현재 세계 크기를 사용한다. 잘못된 좌표는 현재 모드에 따라 원문 로그를 쓰고10으로 보정한다. 좌표·화면WORD→초기 TYPE 포인터/genus→미니맵→저장 범위의 y/x 점유 처리→현재 SHP/레벨→해시→현재 표시/최초Pop/Activate→마지막 contained 진단 순서다. 앞선 좌표·spot·해시는 늦은 실패에서도 남긴다.

미니맵이 바꾼 초기 TYPE 칸의 현재 발자국과 각 칸의 현재 TYPE·좌표를 구별한다. `SquidPopTypeReference`는 같은 배열 칸을 빌리므로 레코드 객체 교체도 물리 칸의 현재 필드로 보인다. grid 후보와 선택한 hash 버킷/next는 진단 뒤에도 저장된 대상을 유지한다. 입구 checking 보기는 원문 값 복사로 원래 owner를 빌리며 호출자 보기 자체의 재연결을 따라 바꾸지 않는다. `SquidPopDiagnostics`는 다음 일반 어댑터가 쓸 현재 조회/진단 묶음도 제공한다.

`RawSquidActivate`는 원문 진단 정상 복귀→현재 BYTE의 void 해제→저장한 전송중 비트 진단→현재 권한/선택된 송신→깊이 증가→현재 가상 후처리→현재 깊이 진단을 그대로 실행한다. 완료 효과를 되돌리거나 자동으로 깊이를 고치지 않는다. 선택한 대리자를 값으로 유지하고 다음 경계는 교체된 현재 장치를 읽는다. `RawPostPopState.ActivationDepth`는 기존 Depth와 같은 backing DWORD의 ref 보기이며 실제 공통 통계/후처리와 사본 없이 공유한다. 기존 Depth 프로퍼티 동작은 유지한다.

CD는 기존 별도 몸체가 대상이므로 새 PATCH 부모 진입에서 명시 거부한다. 네트워크나 게임 창은 열지 않았다. 검사에서 사용하는 권한은 비멀티이며 실제 서버 송신 검증으로 세지 않는다.

## 검증 범위

원본 `pop-world-bounds-x86.tsv`260입력의 Point 반환·세계 크기 읽기 횟수·Pop 앞단의 보정 좌표 비트를 전수 대조한다. signed INT_MIN/INT_MAX와 float 저장, NaN의 x 단락을 포함한다. 이것은 좌표 helper·앞단 관측이며 전체 Pop 후반의 원본 관측으로 확대하지 않는다.

독립 CPP 부모 계약을 이식해 조기 반환, 로그 전후 TYPE, 미니맵 뒤 현재 발자국/저장 범위, checking0 진단 복귀, grid 후보/이름/현재 genus, 옆면·SHP 실패 앞의 부분 OR, 겹침의 SHP 생략, 저장 next/버킷과 현재 섬 분기, 표시 뒤 현재 extra/flags, 최종 contained 진단의 정상 복귀/중단을 검사한다. Activate는 SID0/물리 범위, void 진단의 현재 BYTE 변경, 깊이 불일치 정상 복귀/중단·DWORD 감김·하위 실패, 진단 자기 재연결, 같은 실제 공통 장부의 통계·깊이를 확인한다.

원문 로그의 `%5.1f`는 현재 MSVC18 도구 집합으로 컴파일한 헤드리스 CRT 프로브9입력을 별도로 대조했다. NaN/-nan(ind)/다른 음수NaN, ±무한대·-0.0·2.25의 짝수 반올림이다. 새 게임이나 원본 PE를 실행한 결과가 아니다. 원문512바이트 버퍼 경계와 첫 NUL 이름을 유지한다. 넓은 화면 좌표는 정수 절삭 뒤 WORD 하위 비트를 보존해 .NET uint 포화로 바뀌지 않게 한다.

프로브는 이 PC의 `extracted/dotnet-pop-parent-0511/printf_probe.cpp`·`run_printf_probe.bat`·`printf_probe.exe`에 있다. 해당 디렉터리에서 `cmd /c run_printf_probe.bat`를 실행하면 설치된 VS18 개발 환경으로 콘솔 도구를 빌드하고 원본 비트/CRT 출력9행을 기록한다. 게임 실행·설정 변경 기능이 없고 배포 대상에도 넣지 않는다.

## 추적과 다음

PORT_MAP은4행 추가로721행(L1 346·port375)이다. fixture는1개 추가로249개이며 이전248개 해시는 불변이다. 새 자료 SHA256은 `f7266004f4a79668af5ad08bbe8fd62afe19dca5f793c1107d0f26e274ec7a50`이고 원본 파일을 수정하지 않고 Link로 공유한다. 결과·진단 프로브는 `extracted/dotnet-pop-parent-0511/`에 보존한다.

실제 일반 Pop·World/Client 진단/로그 어댑터의 결선, 현재 SHP/표시 순서와 기존 Pop 원본fixture 재생은 다음0.512에서 진행한다. 이 단위로 실제 Host 요새 Pop·AI 생성/초기 갱신이 완료된 것으로 세지 않는다. M1은0/34다. 이어 프레임 표시/공간 연결→낙하·포스필드·소리 소비자→요새 Pop·AI 결선을 계속한다.

포획/희생/특정 공격/승리·MP 실제 실행·새 NB·외부 접속·기존 경로 은퇴는 없다. 보호 파일·cpppj·LEFT_JOBS.md·병행 webpj를 보존했다.

## 최종 검증

초기 집중39건이 통과했다. 이어 checking 보기의 값 사본 재연결 검사1건을 더했다. 깊이 증가1→2와 SHP 선행 조회 삽입 변이는 부모34건 중12건의 실패를 검출했다. 두 소스를 바이트 비교로 원복한 뒤 전체 비증분 Release 빌드를 경고0·오류0으로 마쳤다.

최종 관련 Core438건과 Client 호스트116건이 실패0·건너뜀0·각종료0으로 끝났다. 최종 부모/manifest40건은 관련438건 안에 모두 포함되며 별도 전체 회귀 수로 더하지 않는다. `core-related.trx`·`client-host.trx`·`mutation.trx`에 결과를 보존한다. 전체 회귀 기준선은 직전0.509의 Assets366·Core1994·Client1172=3532건이다.
