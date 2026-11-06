# Gump·Content 삭제의 현재 클래스 분배 — 단위970

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 우선순위4를 계속하며 원본·정밀 산출물은 수정하지 않는다.

## 원문과 발견

969의 커서 연결을 대조한 뒤 공통 Form 삭제 분배를 확인했다. 기존 `SquidFormDispatch::Route`는 Depend만 현재 VT로 선택하고 Gump·Content·Process는 TYPE 번호를 사용했다. Gump/Content 호스트 자체의 판정도 TYPE3/6을 사용했다. 실제 클래스와 TYPE가 달라지면 부모 삭제 통지와 preDestroy 효과가 뒤바뀌거나, 원래 지원하는 클래스가 미등록 타입으로 거부될 수 있다.

원문 PATCH 공통 삭제 `004af780`/CD `004ab7e0`은 현재 VT+14의 preDestroy, 종속의 현재 VT+1c 부모 삭제 통지, 현재 VT+10 삭제, 현재 VT+48 분리, 현재 VT+18 후처리를 호출한다. TYPE는 각 원문 소비 위치에서 별도로 읽으며 가상 클래스의 대체물이 아니다.

Gump VT PATCH00512188/CD00505a80의 +1c는004ae9d0/004afd40이다. owner DWORD(+18)가0이 아니면 parent WORD(+16)와 함께 UI에 통지하며 TYPE 선행 guard가 없다. PATCH004ae9d0은 정밀 C 함수 항목 검색에서 찾지 못해 두 판본 모두 실제 PE 진입부터 ret4까지 읽었다. Content VT PATCH00512208/CD00505b00의 +1c는0040b380/004afe60이며 실제 ret4 빈 함수다. Content preDestroy는 PATCH004aeeb0/CD004b0300이다. CD의0043d4c0도 같은 커서 SID 또는 자기 부모인 경우 취소한다. 주소/명령 정적 기록은 단위 보고서에 포함한다. 새 CPU 관측이나 원본 실행은 하지 않는다.

## 검사 준비

양판본×실제 Gump/Content 생성자×TYPE3/6/7의12입력을 준비한다. 실제 공통 삭제기·공유 Form 분배기·GumpFormHost·ContentFormLifetime을 같은 풀에 연결한다. Gump 부모 아래 실제 자식을 배치한 뒤 부모 삭제를 요청한다. Gump 자식은 전체 owner/parent 통지1회·Content 취소0회, Content 자식은 빈 부모 통지·Content 취소1회가 기대다. 마지막에는 두 슬롯 반납과 pre/post 공유 깊이0을 확인한다. 비서버 client SID만 사용하며 자산/전송 경계는 선택되면 실패한다. 너겟·경제·전투·사제 효과를 선택하지 않는다.

제품 수정 전12입력을 모두 실행했고 **18단언 실패**를 보존했다. TYPE3/6의 뒤바뀐 실제 알림/취소와 TYPE7의 미등록 거부를 확인했다.

현재 VT의 Depend/Gump/Content 클래스로 등록된 호스트를 선택하고, Process는 기존 실제 Process VT와 등록된 PID 타입 범위를 유지한다. 미지원 현재 VT를 다른 파생 호스트로 재귀 전달하지 않는다. 두 호스트는 사용중/dead 슬롯의 현재 VT로 판정하되 반납 후에는 VT가 지워지므로 기존 공개 TYPE 기록 조회 계약을 유지한다. 일반 ContentFormHost의 별도 필드 API, Process 별칭/기본 PID0/자산 TYPE범위의 전체 삭제 부모는 이번 완료 범위가 아니다.

최초 수정후 빌드에서 BYTE `type`과 DWORD `end`를 같은 auto 선언에 둔 C3538이 발생했다. 빌드 종료 후 선언을 분리해 교정했다. 수정전 빌드0/첫 수정후1/최종0의 원시를 보존한다. 최종 게임/집중 Release는 **종료0·경고0·오류0**이다.

최종 **고유6검사(새1/기존5), 실패0**이다. 새12입력과 반납 후 타입 기록 계약이 통과했고, 기존 Gump Take/삭제·미등록 실제 Depend 클래스·Gump/Content/Depend/Process 혼합 체인의 중첩 삭제와 Kernel/깊이 반납·실제 World Form Pop/진단 복귀와 정렬 Content의 공유 깊이를 대조했다. 미등록 Depend의 기존 dead/공유 깊이 부분효과도 보존된다.

제품970/11.01의 EXE는 **11,358,720바이트**, SHA256 `0017ecb1ab2d9ebbfde146e1252bde29f2434e6cb75e6e05f7442f42194bf169`이다. 보호2796개/fixture976개 불변·새fixture0이며 다음 기준은 `extracted/cpp-resume-970/protected-final.json`이다. [단위 보고서](../../cpppj/form-class-lifetime-20261106.json)를 따른다. checking잔여8/솔루션83은 불변이며 마지막 실제 GUI는968이다. 실제 GUI/MP/통신/공중3종 전용/포획·희생·특정 적 공격·승리/전체CTest·34미션·ZIP은 실행하지 않는다. 모든 Form의 TYPE 범위/임의 VT/Process PID·외부 heap 수명 완료와 구별한다.
