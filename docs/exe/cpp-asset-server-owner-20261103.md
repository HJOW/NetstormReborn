# 자산 PostPop의 현재 서버 권한과 전투 상태 분리

단위807은 RawAssetPostPop의 Gun/바리케이드 서버 관문4곳을 같은 Client 권한 owner로 연결한다. 원문은 패치00540bc0/CD00540a28의 서버 상태를 읽는다. 실제 assetState.inBattle은00594fbc의 전투 표시/콘텐츠 상태로 사용되므로 서버 권한으로 읽으면 안 된다. 실제 연결에서 이미 공유하던 RawAuthorityState.server를 Server()로 소비한다. 기존 Boss()의 같은 owner는 유지한다.

기존 독립 원문 관측기의 battle 열은 이름과 달리 서버 주소540bc0/540a28을 입력했다. 따라서 독립 기존 fixture에 한해 Server()는 역사적 inBattle 입력을 fallback으로 유지한다. 실제 Session은 이미 sharedAuthority를 첫 모듈보다 먼저 연결하므로 이 fallback을 쓰지 않는다. 실제 inBattle의 IceTower/ContentNotice/BattleActive 등 별도 소비자는 이 단위에서 바꾸지 않았다.

수정은 두 Validate 관문과 Gun/바리케이드 본문의 서버 조회4곳이다. 나머지 PostPop 본문은 문자열 치환 외에 동일함을 대조했다. 기존 shared 권한 검사도 서버 입력을 원문 열에서 공급하도록 명확히 했고, 새 검사는 전투 표시 fallback을 반대로 설정해도 현재 서버/권한 owner가 원문 출력을 결정하는지 확인한다. 원본 양판본1636행은 덮어쓰지 않고 재사용한다. 실제 GunProcess 생성/표시/음향 등 큰 자식은 기존 독립 관측 장치이며 실제 발사를 실행하지 않는다.

이 단위는 아직 GunProcess 통지46a340과 네 PostPop46a520/46a600/46a810/46aca0의 진단 복귀를 완료한 것이 아니다. 해당 잔여와 checking36은 유지한다. 새 Native 관측·게임/원본 GUI·특정 적 공격·포획/희생·시나리오 승리·공중 전용·MP 기능/통신·물리/PCM·전체34/전체 C++/전체71대상·ZIP은 없다. 공통 권한 조회의 독립 검사와 MP 기능 검증을 구별한다.

마지막 전체 정밀 디컴파일은 HJOW-Athlon/2026-10-23이며 현재 호스트가 같고 추출물 변경은 없다. 원시 빌드/검사·4곳 동일 치환 감사는 `extracted/cpp-resume-807/`에 있으며 최종 근거는 [JSON 보고서](../../cpppj/asset-server-owner-20261103.json)에 기록한다.

추가 C++ 순서성 검사는 각도 조회 callback이 서버 owner를 true→false로 바꾼 뒤, 원문 위치의 다음 서버 관문이 현재값을 읽는지 확인한다. 결과는 같은 관문의 기존 server0 원문 행과 비교하며 변경된 callback을 새 Native에서 관측했다고 주장하지 않는다.

최종 집중12개(새2개)가 실패0이고 게임v1/최종 집중v3 빌드의 경고·오류는0이다. 각도 callback 순서성 검사 추가 후 집중 대상을 재컴파일한 v2는 LNK1104로 실행파일을 열지 못했다. 관련 검사/빌드 프로세스가 없음을 확인했고 같은 빌드 v3가 성공했다. 파일 잠금의 원인은 미확정이다. 검사 실패는 없고 새 Native 관측도 없다. checking 잔여는36 그대로다. 최종 EXE SHA256은 `148b4585b32efdfe2cdef2d0e1f6679bff578a46e9843ab8bf222b96c578564f`이며 보호2736개/fixture916개는 모두 불변이다. 다음 기준은 `extracted/cpp-resume-807/protected-after.json`이다.
