# 실제 커서 마나 프로세스와 전용 표시·Session 수명

2026-10-17 단위124. 시작 커밋은a9d3510이다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 전체 내보내기는2026-10-16이다. AGENTS.md와 양 LEFT를 읽었다. 새 정밀 내보내기·원본/클론 게임·GUI 실행은 없다.

## 실제 동작과 소유권

`RawCursorMana`는10.78의4741d0(new e0)/4737f0(생성자)/473d00(frame)/473c30(event)/473b90(소멸)을 복원한다. 현재188TYPE·SID 풀·Host의 double 시각을 빌린다. 실제 마나 객체103을 flags2로 만들고 WORD1·extra8·theme 프레임을 적용한 뒤8개를 실제 월드에 등록한다. Attach(type50,parent,form,10) 뒤 현재 시각을 저장한다. 원형의 반경 증가/종료·현재 부모 중심·시간의 _ftol EAX/회전·엄격한 맵 경계와 사각형 위상/이동/삭제를 보존한다. event1은 현재 WORD가 아닌 DWORD 숨김 값과 각 객체의 현재 void를 읽고, event3은 종료 표시를 켠다. 다른 event는 현재 시각+3을 저장한다.

원본 마나 생성자는mode98을0으로 초기화한다. 생성자 else 분기는 소스에서 옮겼으나 실제 진입 관측은 없다. 원형/사각형 frame은 독립 명시 필드 입력으로 모두 관찰했다. 사각 생성자의5변·개수20과 frame의4변을 임의로 통일하지 않았다.

`CursorManaProcess`는 같은`SquidProcessHost`/`Kernel`에 실제 ProcessForm을 붙인다. 소멸자는 현재 커서 토큰 해제→현재 목록의 살아 있는 표시 객체 가상 삭제→Baseprocess 필드 무효화를 실행한다. 원본 사각 frame은 Kill 뒤에도 마지막 시각을 쓴다. C++는 프레임 동안 필드를 shared_ptr로 보유해 객체가 즉시 삭제되어도 동일한 순서의 꼬리 쓰기를 안전하게 수행한다. UI에는 호스트 포인터 대신 현재 Kernel 번호를 전달하고, 제거/이벤트마다 같은 Host의 현재 객체인지 확인한다.

`RawCampaignSession`이 단일`CursorDropState`·마나 전역·몸체를 소유한다.`CursorDropSettings`/`CursorManaSettings`/`BindCursorMana`는 실제 Session의 Factory·Pop/Unpop/Repop·삭제·현재 RawGunTarget 중심·Host/Kernel에 연결한다. 설치 전 연결은 거부한다. 기존 Session 소멸 순서가 Kernel을 먼저 비워 참조 수명을 보존한다. 사정거리 createRange는 이 연결기에서 생략하거나 반환0으로 바꾸지 않았다.

## 원본과 전체 바이트 대조

`tools/decomp_cursor_mana.py`는 생성·프레임·통지·소멸 몸체와 원본 TYPE 조회·WORD setter·부모 조회·클램프·CRT 절삭·커서 토큰 해제·Baseprocess 소멸을 실제 PE 명령으로 실행한다. theme 표의 실제472e60도 실행한다. wrtsWRTS.?>에서 대문자로 조회한 프레임은4/5/6/7/4/5/6/7/8/9/10이다. 외부 할당/Attach/중심/객체 가상 효과만 명시 경계다.

최종 Native2는 **642고유 입력·두 정밀도1,344정상 반환·568,996명령·차이0**이다. 호출마다 ABI/스택/비휘발/x87와 실행/쓰기/읽기 허용 범위를 검사한다. 출력은 필드224바이트·raw31슬롯(1,550바이트)·커서/시각/할당 반환·전체 효과·각 연속 호출 뒤 전체 필드다. 실제 생성 뒤 여러 frame·종료·소멸과 자기 삭제 시 실제 소멸 본문/꼬리 쓰기도 포함한다. C++는 이를 그대로 대조한다. Native1은 음수 발자국을 DWORD로 인코딩하지 못해 중단했으며 근거 수에 넣지 않는다.

CD10.72 정밀 원문의 대응은 생성자48af10, frame48b440, event48b380, 소멸48b2d0이다. 일부는 이웃 대응 추정이므로 신뢰도가 다르며 이번 실행/제품 복원은10.78만 지원한다.

## 실제 월드에서 발견해 복원한 경로

첫 실제 Session 생성은 마나 VT50c3b0의 전용 update88이 미복원이어서 Pop 거부로 실패했다. 예외 정리에서 같은 객체의 미연결 삭제가 다시 선택되어 비정상 종료했다. 실패 로그는 보존했다.

전용473770은 현재 정밀표에 누락되어 실제 PE의 **473770~4737b2,67바이트·ret**를 직접 확인했다. WORD12가0이면 즉시 반환한다. 비영점이면499240으로 bounds/clip/dirty를 실행하고 오른쪽·아래만 **10픽셀** 넓힌다. 확대율로10을 곱하지 않으며 선택 표식/그림자 영역을 더하지 않는다.8c의4ad670은 현재 가상88로 꼬리 호출하므로 같은 동작이다.`SquidDisplay`에 이 경로를 복원했다. 기존 공통 지원 metadata/fixture는 보존하고 이 실제 VT의 복원 경로만 Pop/Unpop에 추가했다.

`tools/decomp_cursor_mana_display.py`는 누락 몸체/8c thunk와499240·공통 실제 bounds/clip/dirty를 실행한다. 기존 표시 관찰기의 실제 Reset/Create와 읽은 보조 범위 자료를 재사용하며 사용한 파일15개를 SHA로 고정했다. Native2는 **432고유 입력·두 정밀도864정상 반환·차이0**이다. WORD0/1/8000·표식/그림자·Q16 확대율·클립·표시 억제·88/8c를 교차했고 raw50바이트 불변·억제 전역·전체 변경표를 대조했다. 이 fixture의 full 열은0059a8b0 **표시 억제 전역**이다. 첫 C++ 검사에서 이를 DirtyRegions의 full과 잘못 비교해 실패한 것을 고쳤다. 사각형 결과는 처음부터 일치했다.

이후 실제 생성·frame·숨김/복원은 통과했으나 삭제는 Session의 TYPE103 분배에서 거부됐다. 실제 PE에서 마나+10/+14/+18/+24/+48 및 firstPop/postPop/8c가 base와 같음을 확인했다.`RawCampaignLifetimeBindings`는 **TYPE103+VT50c3b0** 쌍만 기존 공통 삭제/Pre/Post로 연결한다.84/baseVT 및 다른 파생 분배도 유지한다. 생성/해제 결과를 관측 카운터나 빈 효과로 대체하지 않았다.

## 제품 검증과 남은 작업

실제 thewarbegins GameWorld/Session 검사에서 같은 SID의 TYPE/WORD/extra/theme 프레임과 실제 공간 등록, Host/Kernel/ProcessForm, 이동·숨김·복원·명시 제거 및 현재 토큰/삭제 깊이를 확인했다. 두 번째 실제 부모 삭제는 그 종속 ProcessForm→Kernel 소멸→마나8개 반납까지 확인했다. 같은 Kernel 소유 객체의 원형 종료와 사각형 SID0 frame에서도 실제 자기 삭제를 실행해 현재 토큰/객체 반납/삭제 깊이를 확인했다. 사각형의 삭제 뒤 꼬리 쓰기도 프레임을 끝까지 실행했다. 임시 진단 출력은 제거했다. 초기 검사 빌드의 Read/Get 이름 오류 및 잘못된 일괄 치환은 수정했고 실패 기록을 남긴다.

최종 **Release 빌드11 종료0·경고0·오류0**, 새 마나3/실제Session1 및 기존Pop3/Unpop4 집중 검사가 통과했다. 전체 CTest 내부 **1,500개 모두 통과·실패0**, wrapper1/1·종료0·233.58초다. 자기 삭제 추가 전 첫 전체1500/233.98초는ctest1 로그로 보존했다. 최종 로그는`extracted/goal-cursor-mana-20261017-ctest.log`와`-ctest-output.log`다. 관련 현대 --verify15개가 통과했다. 새 제품 의존15개와 실제 읽은 관측 의존/보호 SHA·UTF-8/LF/문자를 감사했다. 실제 HEAD와 일치했던 기존30보고서68공유 핀을 전후 이력과 갱신하고, 최종 자기 삭제 검사 소스 핀도 다시 이력과 갱신했다. 기존부터 다른 버전이었던 과거124핀은 보존했다. 기존 커서/Pop/Unpop fixture·생성 metadata와AGENTS/DN/보호 디렉토리는 변경하지 않았다. CMake는 기존 전체 접두 바이트를 보존해 추가했다.

실제 Client의 사제 메뉴 전체 DWORD/GumpForm→취득/위치/수집/그리기/내려놓기 조립과 사정거리 프로세스474900/474520/474b80/474790/474c30/474ed0은 남았다. 일반 건설·생산·경제·AI전략·GUI 미션 시작/진행/승패·저장/다음 미션·사용자 자연 완주/원본 동일성도 미완료다. 전체화면 DirectDraw 오류는 요청대로 제외한다. 전체 목표는 active다.
