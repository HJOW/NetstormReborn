# 공통 종속 해제와 UpdateContents 예약의 실제 연결

단위835 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 2026-11-04 새 `/goal` 요청으로834 뒤의 일시 정지를 해제하고 재개했다. 직전 커밋은 `6ff267a1 / 20261104-16 CP`다.

PATCH004ac6b0/CD004ac160은 Form 전용 Unpop과 별개다. contained 진단→현재 가상 부모 getter→부모0/free 진단→현재 부모 dead 판단→contained 재진단→next 저장→현재 previous getter→이전 next 쓰기→다음 가상 previous setter→현재 부모 head의 조건부 쓰기→현재 contained 해제→현재 TYPE/ContentFormType에 따른 부모 제거2 통지 순서를 복원했다. 진단이 정상 복귀하면 본문을 계속하며 이미 실행한 쓰기를 취소하지 않는다. 부모 dead면 분리만 생략한다. 자기 next 진단의 파일 경로/줄은 두 판본을 구별한다.

가상 parent/previous/setPrevious는 TYPE 번호 추측 대신 실제 생성 VT를 선택한다. 보호 PE에서 PATCH83개/CD68개 자산의 세 진입점이 각각 공통임을 확인했고, 판본별 내장Form4종의 다른 WORD 위치를 별도로 확인했다. 알 수 없는 VT나 선택된 미연결 장치는 명시 실패한다. 기존 `SquidForms::Unpop`을 자산에 호출하지 않는다.

PATCH004ad230/CD004acf40은 분리가 정상 반환하면 부모 dead 여부와 관계없이 자신의 종속을 검색한다. Form 전체(<70)를 대상으로 하며 타입6 전용 finder와 다르다. 패치 checking/metamap/dead/boss 진단 뒤 현재 head를 읽고 현재 GumpFormType을 찾으면 현재 UpdateContents 명령을 한 번 예약한다. PATCH005427dc/CD00512964 초기값은 **0x02000039**이며 인자는 대상SID/0이다. Player 내용물 통지0x02000031과 구별한다. 원문에는 확인된 전용39 소비자가 없어 임의 UI Kill/즉시갱신은 추가하지 않았다.

RawSurfaceWorld의 같은 SID 풀/일반 SquidUnpop에 기본 엔진을 연결했다. 실제 Session은 기존 RawContentParentNotice→Player/Carrier/제단/Factory 통지와 `services.assets.queueCommand→Client::Input().PushCommand`를 연결한다. Session이 엔진을 소유하고 World는 weak 참조로 빌려, 생성 실패/반납 뒤 이전 Session callback을 남기지 않는다. 실제 프로세스 정리를 마친 뒤 대여를 해제한다. Storage에는 Session 포인터를 저장하지 않는다.

새 계약검사10개와 실제 Client/Session/World/GumpForm/InputQueue 통합검사1개가 통과했다. 기존 Unpop7개와 Session 회복 수명1개를 더해 **집중19개 실패0**이다. 기존 원문1300행·Unpop3618단계/VT151 기록을 유지했으며 새 독립CPU관측/fixture는0이다. 최초 새11개 실행에서는 제가 TYPE74를 빈 기본 타입으로 가정한 단언1개가 실패했다. 보호 TYPE 덤프의 banner/flags1=28000003/flags2=02000000을 확인해 검사 입력 설명/정확 값 단언만 교정했다. 실제 링크/상태/큐 결과는 첫 실행부터 통과했고 제품 코드는 바꾸지 않았다. 첫 소스/로그와 덤프를 보존했다.

**004afe50 전체 완료 집계는 보류한다.** 실제 Frame/Destroy/Factory nonvoid 및 World 소비자는 같은 부모에 연결돼 있으나 `SquidFactory::Take`의 contained 선행 거부와 void일 때 호출 생략이 남는다. 원문은 모든 !free 슬롯에서 가상 Unpop(0)을 호출한다. 다음836에서 해당 호출자와 현재 상태 쓰기/후단을 대조한 뒤 완료를 판정한다. checking15=부모9+부분3+MP분류3, 완료부모33/부분3을 유지한다.

Release 빌드2회 모두 종료0·경고/오류0이다. 제품835/11.01 EXE SHA-256은 `756f9972fe128efe612bc66df16c3d2ea39716f1bab0803301cff7a780563600`다. 원시는 `extracted/cpp-resume-835/`, 상세 해시는 [835 JSON](../../cpppj/squid-contained-unpop-20261104.json)에 있다. 보호2759개/fixture939개 변경0이다. 실제 게임/GUI·물리/PCM/CD삽입·포획/희생/특정 적 공격/승리·신규MP/통신·공중3종 전용 검사는0이며 전체71/CTest/34미션/ZIP을 반복 실행하지 않았다.
