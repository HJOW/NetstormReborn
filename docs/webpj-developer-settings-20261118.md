# WB04-E740 개발자 설정 명령 앞단

`31h` 개발자 명령의 설정 분기 `setHackMove`·`setNoDamage`·`shootMyFriends`·`fillMyTank`·`generatorTimer`·`quickVortex`·`quickBuild`·`roundToTile`과 각 원본 별칭을 이식했다. 같은 Session의 장기 GameAux 설정을 변경하고 원본 추적 수준과 선택된 로그를 사용한다. 다른 명령은 아직 성공으로 처리하지 않고 명시적인 미연결 오류를 낸다.

근거는 기준선 `225553df`의 `RawCampaignSession.cpp` `ExecuteDeveloperCommand`이며 기준선 이후 해당 파일 변경은 없다. 새 원본 fixture나 직접 cpppj 검사는 없다. 정수 변환에서 signed DWORD 범위를 벗어나는 입력은 CRT의 미정 변환을 추측하지 않고 명시 오류로 둔다.

독립 검사에서 대소문자·별칭·기본값·입력 누락·범위·로그를 확인했다. Tutorial 1 Session의 실제 `31h` 분배기에서 설정 변경과 미이식 명령의 오류를 확인했다. 집중 검사 30건이 통과했다. 실제 게임 조작이나 개발자 명령의 나머지 분기는 검증하지 않았다.

Node v26.13.0 전체 회귀 6,219건을 통과했다. 대응표 오류는 0건이며 Node v22 전체 재검사 주기는 2/8회다.

다음은 `31h`의 나머지 개발자 명령과 내부 로그인 서버 후반·응답 왕복이다. Native 분기, 행 base 뒤 변경 판정 및 앱 미션 연결도 남아 있다.
