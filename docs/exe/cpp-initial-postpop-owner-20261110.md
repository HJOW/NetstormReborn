# Dude·세 주문 효과 최초 후처리 장기 연결 — 작업1264

**제품1264/11.01에서 선택 네 가상20을 같은 실제 장기 본문에 연결했다. 전체 NoWorld·M6-n·M6·목표는 미완료이며 새 MP 실행은0이다.**

| 원본 VT | 실제 +20 | 연결 내용 |
| --- | --- | --- |
|00508cc8|00450cd0|최초 QA5101→같은 population30칸 추가→현재 I 프레임→공통 부모|
|00502c38|0041f870|중복 event1 조회→실제 new40 성공 시0.05 예약→공통 부모|
|00502cf0|0041f920|같은 조회/실제 확보 뒤 현재 게임 난수의 힐 지연→공통 부모|
|00502db0|0041fab0|같은 조회/실제 확보 뒤 현재 게임 난수의 미티어 지연→공통 부모|

주소는 보호 PE의 실제 +20 DWORD와 같은 HJOW-Athlon의 2026-11-03 디컴파일을 직접 대조했다. 새 디컴파일/원문 수정은 없다. DudePostPop은 Storage의 실제 Pool·frames·population·Bridge DWORD·Frame/Building/PostPop을 빌리고, PATCH에서 사용하지 않는 생성용 값형 필드를 게임 전역으로 주장하지 않는다. 세 효과의 기존 RawBombPostPop은 실제 같은 게임 난수·Host를 빌린다. 예약은 기존 장기 AllocateResourceRegular/ConstructResourceRegular가 확보한 같은 주소에서 생성/부착한다.

Native Session은 같은 두 인스턴스를 빌리며 종료 시 가상 분배만 반납한다. NoWorld 분배는 같은 인스턴스를 계속 사용한다. 독립 typed/CD는 기존 지역 소유를 유지한다. 공통 깊이는 각 전체 본문에서 한 번만 감소한다. 별도 World·Pool·Player·난수 사본이나 성공 타이머를 만들지 않는다. 큰 AI·다른 최초 후처리·Path/배치/Fort 상위 Finish/Snapshot과 전수 process/caller/owner는 별도 잔여다.

Release/x64 제품2개와 비MP 집중 대상의 최초 빌드는725.087초/종료0, 검사 행 수 교정 뒤 증분 빌드는47.588초/종료0이다. 경고·오류0과 고정 소스 일치를 확인했다. 기존 비MP300건과 새6건, 합계 **306/306**을40.875초에 통과했다. 새 검사는 World 반납 전후 같은 인스턴스·Dude 최초/비최초/가득 찬 population·세 효과 중복/비최초·독립 PE fixture의 실제 예약 가능54행을 현재 Host의 event/count/float 비트/난수와 대조한다. 최초 실행의 유일한 실패는 선택행 count를36으로 적은 검사 오류였으며54로 교정했다. 원본 기대값·제품 수치를 바꾸지 않았다. 최초 로그를 보존했다.

최종 EXE의 정상 Tutorial4 진입·생산/건설 요청→실제 TYPE75/owner1 생성→600프레임 대기→정상 종료0을 확인했고1920×1080 PNG를 직접 검토했다. TYPE75는 유닛 번호이며 패킷/MP 검증이 아니다. 포획/희생·특정 적 공격·승리/자연 완주·공중3종 전용·Join/Test Battle·MP 단위/패킷/통신/게임/기록 실행은0이다. 기존 일반Edit2회는1261의 증거를 유지한다. 새 ZIP은 없다.

보호 AGENTS·원본/CD/fixture2,808파일은 불변이다. [보고서](../../cpppj/initial-postpop-owner-20261110.json)와 원시 `extracted/cpp-resume-1264/`의 원문 발췌·최초 실패·빌드·검사·실행·해시를 따른다. 다음은 debugBurst 실제 본문과 큰 NoWorld/Fort Snapshot·전수 호출자 감사다.
