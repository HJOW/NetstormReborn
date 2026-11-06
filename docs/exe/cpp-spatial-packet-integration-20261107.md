# 1033 — 위치·카메라·섬 정리 수신 연결

10.78의33h/004a71c0,34h/004333f0,35h/0046e940을 실제 Session 등록표에 연결했다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다. 정밀 C와 보호 PE를 정적으로 대조했으며 원본 실행/수정과 신규 MP 검사는 하지 않았다.

세 등록은 원문의0040b380 빈 observer/버전1/정책0과 고정18/12/20BYTE를 따른다. Editor의 동일 등록/반납 목록도39개로 확장했다. 33h는 WORD SID가5..14999이면 즉시 반환하고, 나머지 SID의 현재 flags&7이0일 때만 실제 Unpop(0)→패킷+14의frame DWORD를 raw+36에 직접 저장→Pop(x,y,0)을 수행한다. 프레임 setter/경로 제거/소유자 변경을 추가하지 않는다. Unpop이 돌아온 뒤 현재 frame/좌표를 읽는다. 호스트 풀의 물리 범위 밖 접근은 기존 오류 계약이다.

좌표+6/+10과35h의+4/+8은 signed DWORD의16.16이다. 보호 PE005092cc는 정확한2^-16이며 실제 명령은 FILD32→FMUL→FSTP32이다. Ghidra C의 겉모양처럼 정수를 먼저 float로 축소하면 큰 값의 저위 비트를 잃을 수 있으므로 double(int32)×2^-16 뒤 float 저장으로 복원했다. 이 곱은 정수 입력 범위에서 double로 정확하다.

34h는+8 Y/+4 X의float 비트를 읽고 현재 논리 Screen 크기를 조회해 기존497f20/RequestCameraCenter를 호출한다. 같은 Client 카메라의 목표X/Y와005c85dc만 바뀌며 즉시commit/제한/현재 Renderer/snapshot 갱신을 추가하지 않는다. FSR 출력 크기나 지도 도킹 사각형을 화면 크기로 대체하지 않는다. 실제 서비스는 Screen 교체 뒤에도 현재 Width/Height를 조회하며 선택된 독립 장치가 없으면 명시 오류다.

35h는 현재flags/owner와 복원 좌표를 기존46e6c0/RawConstructionEffects.DropIsland에 전달한다. 이 몸체의0054db84 억제 DWORD(BSS0)와005411d8 그루터기 TYPE(96)을 장기 GameAux에 두고 기존 지면 소유자의 섬/표면/종유석 TYPE 주소를 공유했다. 객체 로딩 깊이005c89b8와 합치지 않는다. 원문처럼 억제→서버 전파→현재 TYPE 발자국 조회 순서이며 전파 전 별도 검증을 없앴다. 삭제/표시 콜백 뒤 TYPE 비교는 현재 값을 다시 읽는다. 기존 공간 탐색·낙하 표시·위치 음향·파생 삭제·FallWalker를 사용하고 Islanddropper.cpp의9/1/1 진단을 실제 공통 표시 장치에 연결했다.

CD판은33h/004c1fd0의 예약 범위가5..5999이며 stride36과raw+34의frame BYTE를 사용한다. 34h/00488480은00458990,35h/0048fc80은0048f960을 호출하며 세 observer는 각각0040a370/0040a3c0/0040a410의빈 몸체다. 이번 수신은10.78 전용이며 CD 배치에 PATCH DWORD 저장이나 예약 범위를 적용하지 않았다. CD 전체 복원 완료로 세지 않는다.

첫 Release는 종료0·길이 상수 폭 변환 C4244 경고1건이었다. 등록표 상수를 명시 uint16_t 값으로 고친 최종 Release 제품 빌드는 **종료0·경고/오류0**이다. EXE **12,070,400바이트**, SHA-256 **`ee82b53adba738c03864a119525dadaf1bad8560d6b87a8004dfd20b03921e77`**, 표시 버전11.01이다. AGENTS/원본/CD/기존fixture **2800파일 불변**, 신규fixture0이다. 신규 MP 단위/통신/게임 및 지정된 사용자 확인·공중3종 전용 검사는 실행하지 않았다. 이 기록은 실제 LAN 동작 검증 결과가 아니다.

다음은31h 해석기/32h 설정 소비자 등 나머지 수신·전송 감사와 재동기화/Activate·메타/Outpost/checking5다. 32h는 이미 실제 Login에 등록되어 있고 GeneratorTimer는HUD 읽기, ShootMyFriends는현재 gunTargetState에 연결되어 있으므로 중복 등록보다 나머지 소비자·수명 대조를 먼저 한다. 이후 한국어 후속→시나리오UI 순서를 유지한다.
