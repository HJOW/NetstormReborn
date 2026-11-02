# 760 — 일반 명령 네 개와 항해 명령 정적 분기

2026-11-03 HJOW-Athlon. 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23을 유지한다. 단위760 완료, 전체 목표 미완료다.

## 결과와 원문 부모

TakeScreenShot(542768/0200001c)은4d50e0의 기존SaveGifScreenshot→TellGifWritten 순서다. 연속 Capture active와 독립이며 저장 실패 때 Tell을 실행하지 않는다. URLAlias(542860/0200005a)는4a02b0의 첫NUL 별칭+" URL"→현재Config ReadString 성공 때만 기존URL 부모다. Run(542a5c/020000da)은cf970→cf730의mode1이며 일반Tell창으로 바꾸지 않는다. LookAtSid(54299c/020000aa)는 전체DWORD를 보존해 같은Pool/TYPE·CameraHome과 실제 즉시카메라를 호출한다. 이름/숫자/HTML 허용표를 공유하며 Guide/Gump/release 뒤, Console/공통정지 앞 원문 위치에 연결했다.

예약SID1은 fresh Pool에서도 state0/type0/좌표0으로 살아 있다. World없는 첫 메뉴의 기존 Runtime예외를 같은CameraHome→CenterViewPoint 부모로 고쳤다. free/dead는 같은 몸체의 자연 반환이다. World없는Carrier 후속preview만 카메라 적용 뒤 명시Runtime으로 남기며 실제Session의loadingDepth0을 합성하지 않는다. 전체 기본UI caller나World없는Carrier수명 완료를 주장하지 않는다.

## MoveIsland 분류와 최초 실패

처음 준비본은 MoveIsland(542714/02000006)가 param2를 강조모드로 전달한다고 해석했다. 실제C++ 최초5개 중 Native355검사에서15단언이 실패했고 나머지4개는 통과했다. 원문004d76f0의비교→004d76f8 **push6**→4d5bb0을 다시 확인해 고정6으로 고쳤다. mode5/2 기대의 합성 검사는 잘못된 명령계약이었다. 원본fixture는 바꾸지 않았다.

고정6은 ChalSailPos의 MP전용 항해모드다. **분류 전 일반명령 관측에 이 MP명령이 포함됐다.** 원본355행·이름48행, 준비/재관측 및 root 최초검사/15실패 원시는 보존한다. 세 전체Native묶음에는 해당72입력×3관측×3묶음=648회, 두이름묶음에는9행×2=18회가 포함됐다. C++ 최초검사에도72행·이름9행·잘못된합성3호출이 있었다. 실제게임/통신/항해검사는0이며 이 기록을 MP전체통과로 쓰지 않는다.

분류 뒤에는 사용자 MP검사 생략 지시에 맞춰 새C++에서72/9행을 명시 제외하고, 원문 도구도 기본네명령만 관측하도록 고쳤다. 고정6 수정은 정적기계어만으로 판정한다. 최초15실패 범위를 재실행/통과 처리한 것이 아니다. 원문기대값을 변경하지 않았고 제외한 행은 그대로 남긴다. 불필요해진 worldless mode5 역투영/이동장치도 제거했다.

## 최종 검증과 보호

분류 후 비MP 원문283입력/849관측, 이름39입력을 실행했다. 최초fixture에서 MP72/9행을 제외한 전체 바이트와 각각 일치했다(non-mp-subsets.json). 최종Release 제품/기존script집중대상 빌드 경고/오류0, **비MP 집중5개/실패0**이다. 별칭의실패/현재문자열·실제저장실패순서·캡처active독립·전체SID·예약SID1/free SID2를 확인했다. v1의기존Knowledge4/Script2통과는 중간제품 기록이며 최종5개 수에 합산하지 않는다.

Native는 원문Guide/Gump/공통suffix도 실행하지만 C++의283행검사는 선행소비의명시결과와 국소bodytrace만 대조한다. 실제Uber전체앞단/GUI통과가 아니다. URL키/설정값의원문256BYTE초과는호스트오류로거부한다.

보호 2670개 모두 불변, 새fixture2개를 더해 2672개/fixture852다. 다음 기준은 extracted/cpp-resume-760/protected-after.json이다. 제품SHA-256은 `c63c500ddf879c003eeedb1acb1404dfdf99ab39967fd14b1edd494e11c56097`다. 원시 build-v1/v2·tests-utility-v1/v2·분류전/후Native는같은extracted경로에있다. [보고서](../../cpppj/utility-command-input-20261103.json)에분류전상세와최종범위를구별했다.

새게임/CLI/외부프로그램/실제통신/물리/PCM·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP검사는0이다. 기본caller가없는관리명령을UI에임의노출하지않는다. 다음은761의outside/공통정지·모달과762~766이다.
