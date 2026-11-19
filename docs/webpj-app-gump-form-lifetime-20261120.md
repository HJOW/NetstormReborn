# 앱 GumpForm 객체 부착과 수명 (WB04-E1017)

실제 Session의 GumpForm 호스트를 Client Gump에 대여하고 등록 ID·객체 SID 부착, 기본 가상 객체 소멸 통지와 Form 삭제를 같은 풀에 연결했다. World 반납 전에 살아 있는 Form 부착 노드를 지연 삭제·정리한 뒤 대여를 해제한다. 생산 격자 전용 ParentGone 재부착과 실제 생산 UI는 후속이다.

실제 Tutorial1 기술 객체에 GumpForm을 부착해 부모·등록 ID, Gump 삭제, Form 객체 통지와 World 반납 전 정리를 확인했다. Gump 및 앱 Node 24건과 Chromium Tutorial1 취득·회전·배치 회귀 1건이 통과했다. 첫 Node 실행은 작업 디렉터리가 잘못돼 파일을 찾지 못했으며 올바른 디렉터리에서 다시 실행했다. E1006 고정 Node 26 전체 회귀도 6,559건 통과했다.
