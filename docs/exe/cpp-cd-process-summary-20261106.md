# CD 프로세스 요약과 실제 Client 상한 — 단위974

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 원본/정밀 자료는 읽기만 했으며 우선순위4를 계속한다.

## 원문과 변경

CD004ed400은 Kernel005b7bb0의 슬롯1~3999를 순회한다. 현재 프로세스의 Form TYPE를0048f8b0으로 읽고00443400의0051c868 이름 포인터 표를 조회한 뒤, 현재 슬롯을 다시 읽는다. 부모 SID는004ed439에서 EBX에 한 번 저장하고 조건·부모 TYPE 이름·출력에서 같은 값을 사용한다. PATCH00471890의 조건/출력 부모 재조회와 다르다. CD36바이트 슬롯/468바이트 TYPE/171개 표를 PATCH50/500/188과 구별한다.

CD의 앞10개 이름은 PATCH와 다르며9번 포인터는 NULL이다. 나머지52개는 기존 CD 전체 프로세스 이름표다. 원본 vsprintf004f4920과 fprintf004f28a0은 같은 CRT 출력 본문004f6c60을 호출한다. 실제 원본 vsprintf에서 NULL `%s`는 `(null)` 바이트를 출력하는 것을 확인했다. 임의의 빈 문자열이나 PATCH 이름으로 바꾸지 않았다.

기존 RawProcessSummary 본문에 판본별 슬롯 끝·TYPE 표 크기·전체 이름·부모 캐시를 연결했다. public PATCH 이름 조회의 기존 계약은 유지한다. CD62와 물리 범위 밖 SID는 원문의 잘못된 포인터 역참조를 실행하지 않고 해당 소비 위치에서 거부하며 앞선 open 효과는 남긴다. 실제 Client 생성의 장기 Kernel도 PATCH39999/CD3999를 사용하도록 고쳤다. 기존 사용자 Diagnostics·요약/상세/단언 접합을 공유하며 CD용 객체 덤프를 추가하지 않는다.

## 증거와 검사

새 도구는 현재 canonical 정밀 함수 범위를 직접 읽는다. **CD20개 독립 입력/두 x87 제어값의40관측**, 별도 실제 NULL CRT1관측이 성공했다. 원문 요약·Form getter·이름 조회는 실제 기계어이며 파일 open/write/close만 명시 경계다. 전체62이름/NULL9,0/4000 sentinel,3999 끝, 출력 중 현재 슬롯·Form·부모 변경, 부모 캐시 뒤 변경과 파일 토큰0을 포함한다. 기대값을 제품 계산에서 만들지 않았다. [새 CD fixture](../../cpppj/tests/fixtures/cd-process-summary-x86.tsv)는20행이며 기존 PATCH fixture는 불변이다.

수정 전 새3검사에서 **23단언 실패**를 확인했다(CD 원문20행/경계1·실제 Client 상한/소진2). 최종 **고유36검사(새3/기존33), 실패0**이다. PATCH20/CD20/기존 부착48의 Native fixture88행과 Kernel 진단8·Host 수명11·생성4·현재 클래스3·소멸 재진입1을 대조했다. 창 없는 Client 검사에서는 실제 CD Kernel 상한과 같은 장치 접합의 작은 Kernel을 사용해 사용자 CRT 파일/NULL 이름·하위 경로/상세/단언을 확인했다. CD 전체 시작/GUI/플레이를 검증한 결과가 아니다.

수정 전 집중 빌드와 최종 게임/집중 빌드의 **Release2명령 모두 종료0·경고0·오류0**이다. 초기 공통 Oracle의 옛 bridgedecay 산출물 부재와 번역 서식 wrapper의00582988 쓰기 거부는 준비 이력이다. 현재 canonical 범위를 사용하는 도구와 실제 fprintf와 동일한 CRT 본문을 호출하는 vsprintf로 원문 근거를 확보했다. 파일 CRT 바이트를 처리하는 호스트 모델의 범위를 실제 OS 스트림/전체 CRT 검증으로 확대하지 않는다.

제품974/표시·파일·제품11.01은 **11,373,056바이트**, SHA256 `90b893c8ff64f3ed6f11f56e055b1e496e98d6384f6310ad076e3e5a7dba3b06`이다. 기존 보호2796개/fixture976개 불변·새fixture1개다. 다음 기준은 **2797개/fixture977개**, `extracted/cpp-resume-974/protected-final.json`이다. [단위 보고서](../../cpppj/cd-process-summary-20261106.json)를 따른다. 솔루션83/checking잔여8은 불변이다.

## 남은 범위

973의 CD 전체 요약 잔여는 이번 단위에서 해소했다. 다음975는 Factory의 contained/Form·나머지 부모/쓰기/진단 경계를 함수별로 대조하고 외부 heap/할당 실패·파일/비멀티 미연결 소비자를 확인한다. 자산 TYPE 전체 삭제/Factory·파일·checking8/우선순위4는 미완료다. 새 실제 게임/GUI·진단UI·MP/패킷/통신·공중3종 전용·포획/희생/특정 적 공격/승리·물리 입력/PCM/CD 삽입·전체CTest/34미션/ZIP은 실행하지 않았다. 마지막 GUI는968이다.
