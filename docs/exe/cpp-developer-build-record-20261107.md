# 1040 — addLabel과 실제 건설 기록 파일

31h의 마지막 명령 분기 addLabel/al을 현재 기록기와 연결했다. 해석기의33개 알려진 명령 분기는 모두 접합했지만 실제 입력/송신·cr65h 소비 감사와 관련 기록 재생 부모는 남아 있다. 전체31h/LAN 기능 완료나 동작 검증 완료가 아니다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이며 원문 산출물/보호 PE를 읽었고 원본 실행·산출물 변경은 없다.

495860의 기록기 식별자는 같은 Client의 UI 준비 뒤/첫 프레임 전에 설치한다. 실제 FILE과 마지막 단정도 시각은 ClientTraceGlobals에 두어 Session보다 오래 유지한다. 기존 fresh/반납의 커서 대입이 별도 전역0059a7dc/e8을 지우지 않도록 두 기록 필드만 보존했다. 다른 기존 커서 초기화는 유지한다. Client 종료 때 남은 FILE은 fclose된다.

일반 입력의 Ctrl+F6은 기존 모달·정지·special 관문 뒤에만 기록을 시작/종료한다. 개발자 special 플래그는 새로 켜지 않는다. 시작은 현재 게임 시각 float 저장→0.rec부터 첫 빈 파일 선택→열기→수준29 로그→recording1, 종료는 수준29 로그→닫기→recording0 순서다. 자동 생성 파일을 설치 폴더에 쓰지 않도록 사용자 Captures/BuildRecords/d/번호.rec로 이관하며, UserData 장치가 없는 명시 호스트도 UserDataRoot.Resolve를 사용한다. 경로는 W API로 열고 로그는 UTF-8로 변환한다. 본문은 원문 fprintf 텍스트 모드와 CRLF를 유지한다.

기존 RawCursorDrop의473e0 기록 콜백을 실제 writer에 연결했다. 같은 snap/로컬 배치 직전의 타입명·좌표·canon/rotation, float(현재 game)-이전 float 시각으로 한 행을 쓰고 출력 뒤 현재 시각을 다시 float로 저장한다. 원문495330의20BYTE 이름 영역을 넘는 stack 손상은 복제하지 않고 명시 범위 오류로 구별한다. fprintf 반환은 원문처럼 소비하지 않는다.

addLabel은 현재 recording0이면 파싱과 장치 조회 전에 반환한다. 활성 때 두번째 토큰을 읽고 원문 RL 접두를 붙여 같은 FILE에 기록한다. 좌표는 같은 BoardTargetState의 현재 실패점(00545370/74), canon/rotation은-1/-1, delta는0이다. 마지막 배치 시각은 바꾸지 않는다. RL/서식/d 경로 성분과0.0 상수를 PE에서 확인했다. 기록 재생4955b0/495910과31h 직접 송신/입력 및cr65h 소비는 별도 후속 감사다.

NetstormCpp Release 제품 빌드 **종료0·경고/오류0**, EXE 12,116,992바이트/SHA-256 `ca3740b278c9a831a3a6d91b53c616eb36a93abc470e8cdc2a6597daaedd1bc1`다. 보호 2800파일 불변·신규fixture0이다. 신규 MP/단위/게임/통신/진단GUI·공중3종전용 및 지정된 사용자 검사는 실행하지 않았다. 실제 파일 생성/키/배치/라벨 재생 결과는 아직 검증하지 않았다. 다음은 남은 입력/송신·기록 재생/cr65h 감사→LAN 나머지→한국어 후속→시나리오 UI다.
