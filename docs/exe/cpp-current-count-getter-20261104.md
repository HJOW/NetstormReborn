# 공통 레벨과 다이스 Count의 현재 읽기 정의역

단위839 기록이다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 현재 호스트와 같다. 직전838은 `7bbe8af1 / 20261104-20 CP`다.

원문 Has004ad0a0/CD004acdc0은 현재 TYPE를 조회하되 group10의 flags1|04000000 조건은0이 될 수 없어 항상true다. free/dead/contained/예약SID를 별도로 거르지 않는다. RawSquidLevel.Has의 AllocatedBytes를 Slot으로 바꾸고 현재 TYPE 배열 범위만 유지했다. 기존 Get은 현재 상태 BYTE 상위3비트를 읽는다. Set 바디와 실제 AllocatedBytes 쓰기 경계는 유지했으며 모든 setter/free쓰기까지 완료했다고 확대하지 않는다.

Dais Count004478f0/CD00481310도 TYPE165를 강제하지 않는다. 현재 raw TYPE의 코드 표와 현재 frame을 사용한다. TYPE165 guard를 types_/frames_의 물리 범위로 바꾸고 미사용 상수를 정리했다. 원문의 Direction004ad710/CD004ae4b0은 직접 CALL이며 현재 코드 side의 signed BYTE에서'A'를 뺀다. 기존 RawPathAnimation의bit_cast<int8_t>가 같은 해석이다. N/M/L이면1/2/3을 먼저 반환하고, 나머지는 frame<64/64..119/>=120으로1/2/3을 반환한다. 프레임/코드 배열 밖은 기존 호스트 오류로 거부한다.

새 순수 읽기 검사3개는 두 판본에서7 SID(0/1/4/5/서버첫/예측머리/마지막)×8레벨×32상태의 **3584입력**, 두 TYPE(74/165)×5방향×6경계프레임×16상태의 **1920입력**을 실행했다. 수동 Count 기대표와 풀 전체 BYTE·지도·해시·난수 불변, 효과0, TYPE/빈 코드/프레임 경계 오류를 확인했다. 의식·프레임 setter·생성·음향·예약 hook은 호출하면 실패하도록 연결했다. 이5504입력은 새 독립CPU 관측이 아니다. 0x80방향 Count 결과만으로 signed/unsigned를 구별하지는 못하므로 부호 근거는 원문movsx와 기존bit_cast 정적 대조로 구별한다.

새3개와 실제 메뉴 가상80 회귀8개, 기존 Level setter/진단 복귀 및 양판본 원문96행 재생2개를 합해 **집중13개 첫 실행 실패0**이다. Release 빌드2회 종료0·경고/오류0이다. 새 독립CPU/fixture0·기존96행 및 보호2759개/fixture939개 변경0이다. 현재 제품839/11.01 EXE SHA-256은 `26ac381f65db988d75e6ac6c412f332a3c0f7852a36ae721b7a65a93160a537c`다. 원시는 `extracted/cpp-resume-839/`, 상세 해시는 [839 JSON](../../cpppj/current-count-getter-20261104.json)에 있다.

checking14=부모8+부분3+MP3·완료부모34/부분3은 유지한다. Stage 등 내부 Count의 실제 가상 연결과 나머지 메뉴·Dais의CanBridge00449710/BridgeCommand00449900·공통Pop 등은 별도 미완료다. 다음840은 두 Dais checking 부모와 부속 현재 조회/진단 순서를 이어 대조한다. 실제 사제 희생 진입·완료 재현은 제외하고 자격 판정·진단/경로 실패 경계만 확인한다.

실제 게임/GUI·물리/PCM/CD삽입·포획/희생/특정 적 공격/승리·신규MP/통신·공중3종 전용 검사는0이며 전체71/CTest/34미션/ZIP을 반복하지 않았다. push/DN/WB 변경0이다.
