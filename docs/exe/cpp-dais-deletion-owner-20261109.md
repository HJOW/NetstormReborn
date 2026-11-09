# cpppj 1215 — 다이스·제단의 장기 삭제

**구현·검증·인계 완료. 제품1215/11.01이며 전체 목표는 진행 중이다.**

실제 제단44a4d0/448b60·다이스44afd0/449580의 고유 삭제를 같은 장기 Storage의 현재 Pool/해시/TYPE/역할·약한 Player owner·참여 상태·전체 Damageable에 연결했다. Native Session도 PrepareRitual/Player snapshot 없이 같은 고유 본문을 호출한다. 기존 typed 수명/전처리는 같은 RawDaisDeletion 순서를 공유하며 CD/독립 배열의 명시 계약은 유지한다.

## 원문 순서와 실제 owner

제단 Pre는 flags400000이 없고 현재 typed 다이스가 유효·dead아님이면 같은 flags로 먼저 가상 삭제한다. 제단 자체extra9와 무관하게 그때의 실제 발자국에서 추상 룬을 찾고, 현재 boss 또는 client SID 권한을 만족하는 후보를 flags0으로 지운 뒤 전체 Damageable Pre를 호출한다. 다이스 Pre는 extra9의 최초 관문 뒤 실제 제단24 완료·강한 같은 owner 사제의604 중지·현재 제단48 삭제·owner1..8 참여 갱신·전체 Damageable 순서다. 주변 float 사각형의 우하단은41def0처럼 거의 올림한다.

현재 사제 조회4479c0과604 중지491b60은 기존 TempleRitual/DaisRegular 본문을 필요한 실제 자료만 받도록 공유했다. 장기 참여 조회도 기존 RawPriestParticipation의16칸 사제 목록·9DWORD ownerState·현재 세 역할·같은 Carrier를 사용하며 Session은 같은 객체를 빌린다. Player/SP/난수/현재 역할을 구성 시 초기화하지 않는다.

제단24 순회는 후보next를 먼저 저장하고 실제 Kernel 프로세스 Form의 현재 BYTE를 전체 Regular/Shared 역할 DWORD와 비교한 후 event24일 때만449b30을 선택한다. 큰449b30은 Client가 현재 전체 Runtime을 pin한 뒤 기존 전체 FinishSacrifice를 호출한다. 이 경계를 빈 성공이나 직접 상태/보상 쓰기로 대체하지 않는다. 전체 의식 Regular/획득/음향/UI의 장기 owner 완료를 주장하지 않는다.

다이스 Post는 현재 owner가0이면 Player를 읽지 않는다. PATCH449580의 ESI base 보유/조건부 재조회와 현재 owner 계산을 actual view/pin으로 유지하고, primary94/8c가 자신의 전체 SID와 같을 때만0과 mirror98/90을 쓴다. 다른 SID·높은 비트·다른 Player 행은 유지한다. 실제 Player pin/span은 외부 Damageable Post 전에 반납한다. 제단 Post448b60은44b9e0으로 직접 tail jump하는 같은 전체 부모다.

## 원문 근거

정밀C/PE **13함수**와 정밀 메타데이터가 없는 PATCH449580을 구별했다. 후자는 PE **00449580..0044965f,224BYTE**의 bounded disassembly만 보관했고 정밀 C를 합성/수정하지 않았다. 네 고유 진입·4479c0/491b60/495060/41bff0/4b1fa0/41def0 및 CD4836b0/483660/4821e0/481620이 근거다. 별도491a20 C의 signed HP와MaxHP/2 비교도 확인했다. CD는36BYTE 슬롯/extra35·별도 Player84..90/strideac와 단계별 대응 부모를 유지한다.

마지막 Client양판본 정밀 디컴파일 **2026-11-03 / HJOW-Athlon(192.168.0.94)**, R **2026-11-07 / 같은 호스트**이며 현재도 같다. 새 디컴파일/원문 변경0이다.

## 검증·최초 실패와 교정

Release/x64 제품2·집중5대상 종료0, 경고·오류0, 소스19개 고정. 기존 허용287개+신규 다이스15개+기존 독립 원문4개 = **306/306** 통과했다. 기존 DaisLifecycle2사례는 각각1928행, Altar2866행 및 TempleRitual 전체 fixture를 원래 기대값으로 대조했다. 실제 포획/희생 플레이·MP 소비자 실행은 아니다.

신규는 동일 본체/무초기화·네 실제 Pre/Post/깊이·Player SID 두 쌍/미초기화/재할당/weak owner·제단400000 관문/extra와 소수 경계/비boss의 서버SID 생략/앞선 삭제 뒤 역할 변경·장기1..8 참여 갱신·실제604 제거/E프레임·전체 process역할 비교를 확인한다. 실제 네트워크 장치는 금지 callback이며 원문 client SID/억제 조건을 사용한다.

최초306사례는304통과/2실패였다. 604 사례는 원문 squid.h:291 n != getSid() 진단으로 수동 해시/void 입력의 저장 단계 불일치를 확인하고 실제 K프레임 StoreLevel을 먼저 적용해 self-next 재Pop 입력을 고쳤다. 벌룬은 이제 복원된 제단을 미복원으로 기대하던 사례를 실제 child 로그 뒤 현재 VT 재조회 실패로 옮겼다. 처음부터 미지 VT를 넣으면 가상10 진입 전 거부되어 깊이 기대를 재현하지 못한 진단 중간 결과도 보존했다. 두 깊이·dead·무반납·선택 유지·Session 우회 금지 기대와 원문 fixture는 약화하지 않았고 최종306/306을 확인했다.

최종 같은 EXE에서 Tutorial4 정상 UI 생산·건설 요청→TYPE75/owner1 생성·요청 증가→600프레임 대기/정상 종료와015 PNG를 확인했다. Client800×600 Edit TEST01↔Main2회/11관측도 종료0이다. 다이스 삭제/604/희생 완료·사제 포획/희생·특정 공격·승리·건물 완성/TestBattle를 실제 플레이로 검증한 결과는 아니다. MP 단위/패킷/통신/게임/기록·공중3종 전용 실행0이며 시작한 모든 빌드/집중/진단/GUI 핸들의 종료를 확인했다.

- Netstorm.exe 13930496 bytes, SHA-256 `f296bd1eb0d46f0e439597870cf64d9955ebb3a3756017a9485b0d7b497ef3b4`.
- R.exe 677376 bytes, SHA-256 `0cbf8e5bcdb29cd448d94ced60852dd308459f962fa052be8b54b69ebe82ac9e`.
- 보호 원본/CD/fixture·AGENTS **2,800파일 불변**. 원본 실행·원격 push0.

## 남은 범위

알려진83VT의 **Pre/Post분배83/83**, 전용 통지6이 남는다. 통지는VT509148/509210/50eb98의48f0a0과VT509930/5099e8/509aa0의45b4a0/45b5b0/45b680이다. 다른 Form/Regular·큰 비영 AI·다른 배치/Fort 상위 Finish·전수 Caller/owner 감사·M6-l/m·M7독립5건/M8·K/S/B의 상위16계획·등록34버그도 남는다. 이번 연결을 알려진 버그 조사/수정 완료로 세지 않는다. 전체34진입 제품644/검증645·최신ZIP1092·최신 전체 솔루션 통과 없음도 유지한다.

[현행 인계](../../LEFT_JOBS.md)·[최종 보고서](../../cpppj/dais-deletion-owner-20261109.json), 원시 `extracted/cpp-resume-1215/`의 original-evidence/unrefined-evidence/bounded ASM/static-audit/source-frozen/build/tests/GUI와 first-/diagnostic- 원시를 따른다.
