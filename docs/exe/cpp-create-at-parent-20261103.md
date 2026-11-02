# 770 — CreateAt 실제 생성 부모

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23 유지. 단위770 완료·전체 목표 미완료다.

원문00542a74 이름/숫자와 같은 ScriptStringRing/QueueScriptCommand를 연결했다. Guide/현재 Gump 뒤 release를 거르고 Editor 또는 mission에서 param2 전체DWORD를 현재 Session.CreateAt에 보낸다. Console/정지/로컬owner 조건이나 기본 메뉴는 추가하지 않았다. 원문 입력408에는 생성 뒤 일반413d50/Paused/445330 순수조회 꼬리가 있으므로 실제 Uber 누름은 조기 반환하지 않고 현재 Help/모달/세계 입력 처리를 이어간다. 늦은 메뉴 dispatcher와 미연결 진단의 중복 처리만 막는다. release는 소비만 한다.

현재 parser의 signed x/y/type 양수 관문→실제World.Create(type,0)→owner0일 때 현재 지면BYTE→파생 가상 SetOwner→현재 Has/Max HP 재조회→공통 직접 HP→원래 parser TYPE의 특수4종 판정→현재 생성 객체 TYPE의 직접 frame→float32(x,y,0) Pop 순서다. 비용/FactoryConfirm·다른 임시 World·가짜 SID로 우회하지 않는다. 원문 decompile의 float owner/뒤집힌 Pop축/owner frame인덱스는 기계어로 정정했다.

프레임은 signed radius/2의0방향 나눗셈과DWORD LEA 감김으로 TYPE+15c를 읽는다. radius0은L.first이며A로 보정하지 않는다. 기존 RiftTypeFrames의 first16/count16/LetterKinds를 사용하며 음수/큰 값이 해당 메타데이터로 넘어가는 것도 보존한다. 그 밖의 raw TYPE 이웃필드/32BIT포인터는 앞선 생성·owner·HP 뒤 명시 out_of_range다. 누락frame0·임의포인터 성공이나 자동 rollback을 만들지 않는다. 원문256BYTE strcpy 초과/NULLnew/진단복귀의 host 경계도 구별한다.

원문 부모580입력/1160관측, 일반입력408/816, 이름/토큰/고리/FIFO15와 root 독립 반복1991관측의3fixture가 바이트 단위로 일치했다. 부모는 실제44c660/44c540/40da00/4adc60/4adcb0/4adca0/425830/4ac550/49fce0/49b060이며 TYPE lookup/new/가상 owner/Pop/TLS/assert는 독립 경계다. 원문 전체Factory/가상객체/OS/SEH 동등성을 주장하지 않는다.

독립 읽기 감사의 확정 결함0이며 실제 컴파일/실행 결과와 구별한다. 최종 Release 경고/오류0, 집중 **14개(새9개)/실패0**. 실제Runtime3개는 비전투TYPE76 RainGenerator와 실제 받침을 사용해 생성·소유자 상속·HP·공급장부·공간Find·SP불변과 미설치/권한 실패를 확인했다. 기존DestroyAt parser/입력/삭제 순서와 실제 현재 Session 회귀도 통과했다. Kernel/AI frame이나 실제 공격·공중3종 행동을 실행하지 않았다.

최초 집중검사의1실패는 새 ReachedParentFailures 검사의 재사용 callback이 세 번째 TYPE76 입력에도 이전 TYPE131을 요구한 검사 상태 오류였다. 각 입력의 expectedType131→76을 명시했다. 제품과 원문580/기존 fixture를 바꾸지 않았으며 첫 실패와 v1빌드를 보존했다. 기존DestroyAt 회귀의 첫 명령은 등록 대상 선택이 잘못되어0실행/no-match1이었다. 실제 기존 destroy_at 타깃의 네 검사를 실행했으며 미실행을 통과로 세지 않았다.

기존 보호2694개 모두 불변·새fixture3 포함2697개/fixture877. 다음 기준은 extracted/cpp-resume-770/protected-after.json이며 제품 SHA-256은 `ab430481d6c8d365bd3f186d4a15d87044c991a6e956076cce66833d9348ce6c`다. [보고서](../../cpppj/create-at-parent-20261103.json)와 extracted의 준비/반복Native·독립감사·최종build/tests를 따른다. 새 실행대상0/기존66개·전체71대상 유지다.

새 게임/GUI/CLI/MP·물리/PCM·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP 검사는0이다. 전체UI·특수TYPE 가상부모·전체 동일성으로 확대하지 않는다. 다음은 Config reader와 남은 AI/파일/호환성이다.
