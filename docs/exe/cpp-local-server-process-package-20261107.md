# 1092 — 클라이언트의 새 R.exe 실행 장치와 배포

클라이언트37의 실제 새R.exe Windows 실행 콜백과 배포 동봉을 연결했다. 제품1092/11.01, Netstorm.exe 12,370,944바이트/SHA-256 `228b158634e984f179abfb349fffb07900ece5f15fc08c4081df66d3804d4aec`, 새R.exe 669,696바이트/SHA-256 `25d7bfba53a5dc935864eb3f4fae4fe9c17026a811b6efac903750118c70a061`다. 직전 CP는 `dc59e44d / 20261107-164 CP`다. **CreateProcess/게임·서버·MP 실행 검사는0이다.** 코드의 실제 OS호출 연결과 파일 배포 검증을 실제 서버 기동/접속 성공으로 확대하지 않는다.

## 원문37과 실제 새 실행 파일 선택

004b8026..4b80ee/4dd6b0의 원문은 안내→현재12/13닫기→rootServerExe ReadString→여섯후보/각실패34→첫채택 후보 lan/wait0 실행→다음38/바깥공통확정이다. NativeLocalServerLaunch의 이 부모와 여섯 순서는 변경하지 않았다. 보호PE00542624의 여섯 문자열도 대조했다. 원문Winutil은program/본문을추가인용없이이어CreateProcessA하며 시작결과를 void로 버린다. 비영wait일 때만 WaitForInputIdle5000/exitcode 순회가 있고37은 실제0을 넘긴다.

현대클론은자료폴더와실행파일폴더가다르다. 따라서37의 서버후보 조합만 **클론모듈폴더** 기준으로 같은 여섯prefix/name을 같은RawFileName 한 칸에서 만든다. 게임자료 root/VFS·나머지정적문자열을 바꾸지 않는다. 원문435cd0의 모듈폴더/명시root 개념과 현대 분리배치의 차이를 구별한다. 원래자료폴더의 R.exe를 새서버로 오인해 실행하지 않는다.

각 후보는 먼저 기존 Files.Probe의 실제 출처조회 효과를 수행한다. 이후 Windows 버전리소스를 읽어 현재11.01 파일/제품버전·VFT_APP·OriginalFilename=R.exe를 만족한 실제 파일만 채택한다. 현재리소스040904b0을 사용하며 코드/서버를 로드·실행하는 조회가 아니다. 원문R/게임Netstorm.exe/아카이브의 실행을 대신하지 않는다. 원문클라이언트검증7f/80을 다시 활성화하는 절차가 아니라 **현재 빌드 산출물을 고르는 파일식별 경계**다.

## Unicode 비대기 실행과 호스트 경계

LocalServerProcess는 정확한 lan/wait0만 받는다. 같은 실제파일 식별을 실행 경계에서도 확인하고 CreateProcessW에 명시한 실행파일/인용된argv0+' lan'을 전달한다. 한글·공백경로와 현재Client.GameDirectory의 명시자식CWD를 보존하며 부모CWD를 바꾸지 않는다. 환경은 원문처럼 상속한다. 작업 폴더는 원본Data일 수도 있으나 새R의 자동쓰기는 기존 사용자홈 정책을 따른다.

원문GUI와달리 현재호스트는GUI/콘솔·MCP를겸한다. 자식전용콘솔/최초SW_HIDE를 사용해 부모콘솔·표준출력이Root에의해바뀌지않게 했다. 새R의기존 lan본문은 자기제목/40ms/자기창최소화/사전500통지를 그대로 수행한다. 성공한실제CreateProcess의hThread/hProcess는 즉시반납하며 자식을종료하지않는다. 원문의 추가인용없는A API/flags0·상속CWD/핸들누수와 다른 현대Windows분리배치/소유권 경계다.

실제시작BOOL은 원문void부모처럼 버리고 다음38을확정한다. 시작실패를 접속성공으로 바꾸거나 Wait/알림을 추가하지 않는다. **500통지는 실제R이 초기화전에 보내는사전통지이고 준비성공ACK가 아니다.** 원문실행헬퍼의 비영wait 범용분기는 이37전용장치에서 구현했다고 주장하지 않는다. 실제창·콘솔/CWD/환경·수명/동시파일교체·시작실패/후속TCP와알림은 실행미검증이다.

## 배포 스크립트와 실제 ZIP

package-cpppj.ps1은 기존 NetstormCpp 빌드의 NetstormRoot 의존성을 사용하며 같은 산출물 폴더의 새R.exe를 필수로 동봉한다. ExePath를 직접 지정해도 같은 폴더의 새11.01 R이필요하고 GameSource/원본R로fallback하지 않는다. 파일/제품버전과OriginalFilename 식별을읽어잘못된R을거부한다. 누락/오래된두실행파일·NoBuild/DryRun의기존의미를 유지하며, 기본배포전CMake의존성확인도유지했다.

두실행파일의VisualC++ DLL 목록을합쳐중복을제거한다. BUILD_INFO에는두파일의크기/시각·SHA-256을기록한다. ZIP 검증은R의필수단일항목과실제압축본문SHA-256까지현재빌드산출물과대조한다. 안내문에는재빌드R동봉과멀티플레이개발/미검증상태를기록했다. UTF-8 BOM을유지해실제Windows PowerShell5.1에서도한국어문자열을읽었다.

실제 PowerShell5.1 DryRun 종료0, 한 번의 NoBuild ZIP생성/검증 종료0이다. ZIP은 **1,384항목/게임자료1,358개·202,440,265바이트**, SHA-256 `33b2459cca9dcbde8b10c7e2cca2e12cc071715244868e3a60908fb085089ae7`다. 전체CRC와Netstorm.exe/R.exe의빌드바이트완전일치·새R한개/원본R없음·사용자options/fullscreen/php제외·런타임3DLL합집합을확인했다. 동봉DLL은msvcp140.dll/vcruntime140.dll/vcruntime140_1.dll이다. BUILD_INFO는현재단위커밋전의개발스냅샷임도기록한다. **이ZIP의추출후게임/서버실행 검증은하지않았다.**

원시는Git제외 `extracted/cpp-resume-1092/NetstormReborn-cpppj-1092.zip`과같은폴더의계획/패키지로그·정적/바이트감사다. 빌드/문서만바뀔때매번ZIP을반복하지않는규칙을유지하며이번은새R동봉/검증수정때문에한번만생성했다.

## 검증과 잔여

NetstormCpp/NetstormRoot Release/x64 빌드 종료0·경고/오류0이다. 새C++단위/MP/게임/통신/소켓열기/실제CreateProcess/새R·원본R 실행0,전체검사/34미션반복0이다. 마지막C++실행검사는1052 기존7개며이번에반복하지않았다. 보호2800파일불변·새fixture0이다. 원문/보호PE·현재소스와파일패키지만대조했으며서버/통신실행결과가아니다.

다음은제공R의3BYTE존과클라이언트0F의47BYTE차이·0F/1d수신/메타의실제연결을우선대조한다. 세.server콘텐츠/배포·다른LAN hostname/Config registryValue경계·LAN/Outpost/checking5·한국어후속/Edit시나리오UI도남아전체목표는미완료다. 원문에없는원격포트Open/handler등록은추가하지않고현재IPv4 TCP/UDP/IPX·SPX명시미지원·폐기검증/PIRATE quota비활성을유지한다.

추적보고서는 `cpppj/local-server-process-package-20261107.json`이다. 마지막전체정밀디컴파일 **2026-11-03/HJOW-Athlon**,R전용 **2026-11-07/HJOW-Athlon**이며같은호스트기존산출물만읽었다. 재디컴파일/생성소스변경0이다.
