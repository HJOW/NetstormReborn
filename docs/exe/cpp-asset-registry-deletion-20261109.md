# cpppj 1213 — 현재 Player·거주 구역·Dude 삭제 등록부

**구현·검증·인계 완료. 제품1213/11.01이며 전체 목표는 미완료다.**

실제 Player152 Pre4906c0, 거주 구역 Post49a4a0, Dude Post450920을 같은 장기 Storage의 현재 기록/목록/공통 부모에 연결했다. Native Session도 현재 VT로 같은 세 본체를 호출하며 독립 typed 경로는 기존 계약을 유지한다.

## 현재 주소와 목록의 순서

Player의 첫 현재 owner/base +9c가 자신의 SID면0으로 쓴 뒤 공통4b0950을 호출한다. 이후 현재 owner/base를 각각 다시 구해 +50의 bit0을 지우고 bit8을 켠다. 같은 필드의 읽기/쓰기는 하나의 실제 view/pin 안에서 수행하고 공통 Pre를 넘어 span이나 base를 보관하지 않는다. ClientPlayerRecords를 약하게 최초 결합하며 없던 Player 배열을 만들거나 초기화하지 않는다. 다른 SID·다른 행·다른 flags 비트는 보존한다.

일반 거주 구역은0059a9d8 목록, Dude는0055a4f0 population에서 원문40ea00의 signed count 반복으로 모든 일치 DWORD를 압축한다. stale 꼬리·capacity·등록 표는 그대로다. Native는 Items 전체 사전 검사를 넣지 않으며 음수 count는 원문처럼 읽기를 생략한다. 독립 typed Dude의 기존 사전 계약은 유지하고 같은 실제 제거 본체를 공유한다. 두 고유 Post는 그때의 extra9를 검사하고 같은 공통4b0840으로 깊이를 마친다.

원문 **8개 전체C/PE**를 구별해 보존했다. 이번 직접 구현/공유는 PATCH `4906c0/49a4a0/450920/40ea00` 4개다. 사제 후속 조사 `4919b0/4918e0`와 CD `40c330/40bf00`은 읽기 전용 근거이며 이번 구현 완료로 세지 않는다. 처음 가설과 달리4906c0은 포스 필드가 아닌 Player152,49a4a0은 거주 구역이다.

마지막 Client 정밀 디컴파일 **2026-11-03 / HJOW-Athlon(192.168.0.94)**, R **2026-11-07 / 같은 호스트**이며 현재도 같다. 새 디컴파일·원문 변경은 없다.

## 검증

Release/x64 제품2·집중5대상 종료0, 경고·오류0, 소스14개 고정. 기존 허용 비MP256개 + 새 등록부11개 + 기존 Dude410입력 사례1개 = **268/268** 통과했다. Player 미초기화/weak 반납/정확한 SID와 flags/공통 Pre의 실제 Player 재할당·owner 변경, 실제 두 목록의 중복·높은 DWORD·stale 꼬리·음수 count/현재 extra와 목록 재조회를 확인했다. 모든 새 사례는 MP 송신 장치를 금지한 비MP 공통 본체 검사다.

최초 집중부터 모두 통과했다.

같은 최종 EXE로 Tutorial4의 정상 UI 생산·건설 요청 → TYPE75/owner1 생성·요청 증가 → 600프레임 대기/정상 종료를 확인하고015 PNG를 확인했다. Client800×600 Edit TEST01↔Main2회/11관측도 종료0이다. 건물 완성/Test Battle/특정 적 공격/사제 포획·희생/시나리오 승리 재현이 아니다. MP 단위/패킷/통신/게임/기록과 공중3종 전용 실행도 하지 않았다. 모든 시작 핸들의 실제 종료를 확인했다.

- Netstorm.exe 13896192 bytes, SHA-256 `81453c33fa001f943738cb124e94196b0aa59d6d8ca78d13b42e04012d04f54b`.
- R.exe 677376 bytes, SHA-256 `d0ae04f9678a602ab321d5a0a68d38c28d563242cc8e669c2b756197d9b4c9c8`.
- 보호 원본/CD/fixture·AGENTS **2,800파일 불변**. 원본 게임 실행·새 MP 실행·원격 push0.

## 남은 경계

실제 Pre/Post 분배 **80/83**, 다이스/제단2쌍·사제Pre1쌍, 전용 통지6이 남는다. 사제의 현재 목록·보호막 조회·Native 계약, 큰 비영 AI·다른Form/Regular·다른 배치/Fort 상위 Finish·전수 Caller/owner 감사와 M6-l/m·M7/M8·K/S/B 상위16계획/등록34버그를 유지한다. 이번 연결을 알려진 버그 조사/수정 완료로 세지 않는다. 전체34진입 제품644/검증645·최신ZIP1092·최신 전체 솔루션 통과 없음도 유지한다.

[현행 인계](../../LEFT_JOBS.md)·[최종 보고서](../../cpppj/asset-registry-deletion-20261109.json), 원시 `extracted/cpp-resume-1213/`의 original-evidence/static-audit/source-frozen/build/tests/GUI JSON을 따른다.
