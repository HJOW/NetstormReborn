# Template9의 명시 수명과 같은 State 섹션 — 단위261

시작 커밋은 `8c653e23`이다. 직전 단위260은 Init의 요새 요청/Zone 소유·실제 로그인 소비를 구현·검증·커밋한 진행 턴이다. AGENTS와 양 인수인계를 다시 읽었고 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**으로 같다. 새 디컴파일·원본/CD·AGENTS·DN/README 변경과 push는 없다. 사용자는 이번 작업 완료·문서화·로컬 커밋 뒤 **목표 일시 정지**를 요청했다. 다음 작업에는 착수하지 않는다.

이전에는 Template9·State 사본·지형 cursor를 Session 안에서 소유했다. Session 해제가 자료까지 반납하므로 Init의 템플릿 정리는 이전 자료가 이미 없다는 검사뿐이었다. 또한 State의 writable 사본이 파서의 원시 섹션과 달라 현재 State를 바꾸어도 `RawFortImage`가 읽는 원문에는 반영되지 않았다.

`RawTemplateLifetime`에 원본004bfb00/004bf690/004bd2e0의 전체 부모를 복원했다. 한 헤더는20BYTE이고 전역 표는9칸180BYTE다. 비영 manager이고 +4의 빌림DWORD가0일 때만 소멸→같은 저장 포인터의 free를 호출한다. 소멸이 헤더를 바꾸어도 최초 저장한 manager를 반납한다. 단일 반납은 포인터0이어도20BYTE를 비우지만 전역 정리는 **포인터0인 칸의 나머지 stale DWORD를 보존**한다. 초기 생성은 소멸 없이20BYTE만0으로 만든다. 앞 효과가 바꾼 뒤 칸은 현재 값으로 다시 읽는다.

`CampaignTemplateStorage`는 Client 수명의 실제 manager/파싱 자료를 소유하고 Session은 같은180BYTE 표를 빌린다. 소멸 효과는 안쪽 실제 버퍼를 비우고 free 효과는 manager 객체 할당을 반납한다. 호스트 포인터를DWORD로 자르지 않는 비영 핸들을 사용한다. 빌린 헤더는 원래 소유자와 같은 파싱 자료·State/지형 cursor를 반환하며,빌린 헤더 반납으로 소유자의 실제 버퍼를 해제하지 않는다. 원래 소유자가 사라진 뒤의 빌림 접근과 소유자를 파괴하는 역방향 빌림은 호스트 계약으로 거부한다. 원본의 정의되지 않은 메모리 접근을 실행하지 않는다.

실제 Init의 첫 템플릿 정리는 이제 같은 저장소의 `ClearAll()`을 호출한다. `ClearWorld`는 Session/프로세스 참조를 반납하고도 Client가 보유한 Template9는 명시 정리까지 유지한다. 실제 로그인의 파일 재선택·이미지 저장과 캠페인 로더는 현재 manager를 매번 조회한다. 기존 독립 로더 입력의9개 span과 State 배열 생성자도 유지하며,Client는 현재 manager getter로 연결한다. 따라서 빌린 헤더의 cursor를 새 사본으로 만들지 않는다.

State stream은 같은 `FortTemplate.sections[State]`의 바이트를 직접 사용한다. 짧은 기존 State를28BYTE로 늘릴 때는 이전 원문 할당을 임시 보유하고 새 실제 섹션 버퍼를 만든다. 기존 원본 Get 몸체가 복사/0채움·pos/end28을 끝낸 뒤에만 이전 할당을 반납한다. 이로써 로그인/Player가 쓴 현재 State와 실제 이미지 직렬화가 같은 원시 섹션을 읽는다. 별도의 `stateBytes/stateGrowth/stateStreams/stateNames` 저장소를 Session에서 제거했다.

유효 보정 관측은 **240고유 입력·240정상 반환·15483실제 명령**이다. 실제 세 부모의 진입 수는004bfb00=108/004bf690=108/004bd2e0=24이며 DataManager004bd330/CRT free만 명시 효과다. 전체196BYTE(표180+양쪽8)·각 소멸/free 직전 현재 헤더·소유/빌림/빈 stale·경계 뒤 재조회·cdecl/thiscall 스택/보존 레지스터/x87/쓰기 범위를 보존했다. 내부 MemStream 링크·힙/OS/게임을 실행한 관측이 아니다.

제품 수정 전에 생성한 초기240행/13257명령은 **경계 감사 실패로 거부했다**. 공용 Oracle의 `TemplateFree`/`Template` 대체 호출이 단일 반납/생성 진입을 가로채 두 본문이 실행되지 않았다. 초기 도구/fixture/JSON과 `extracted/template-lifetime-20261021-first-capture/`는 수정하지 않았다. `template-lifetime-initial-rejection.json`이 거부 이유와 전체 SHA를 기록한다. 별도 `decomp_template_lifetime_v2.py`는 공용 대체 호출을 사용하지 않고 세 본문의 실제 진입 수를 확인한다. **보정 관측은 제품 수정 뒤** 원본 PE만 독립 실행했으며,C++ 알고리즘/결과로 기대값을 만들거나 구현을 보정 관측에 맞춰 바꾸지 않았다. 유효 원문은 `template-lifetime-v2-native.json`/`template-lifetime-v2-x86.tsv`이며 `extracted/template-lifetime-20261021-corrected-capture/`에 따로 동결했다. C++검사는 유효 v2만 읽는다. 초기 행을 성공 수에 합산하지 않는다.

새 C++검사4건은 유효240행·실제 파일의 소유/빌림과 같은 State 직렬화·짧은 State 확장/동일 cursor·Session 해제 후 Client 자료 생존과 명시 Init 정리를 확인하도록 작성했다. 기존 State 배열 입력과 과거 원문/기대값은 보존한다. **새 C++검사와 게임 실행은 이월하며 컴파일만 한다.** 최종 전체 Release는 `extracted/cpp-template-lifetime-20261021-build2.log`이며 종료0·경고0·오류0이다. 두 번째 빌드는 보정 fixture의 컴파일 입력을 반영했다.

직전 요새 요청 감사가 과거 바이너리를 같은 경로의 새 빌드와 비교하던 오류도 고쳤다. 과거 바이너리 해시를 새 값으로 바꾸지 않고,과거 빌드 로그/전체 JSON 보존과 현재 소스·원문을 확인한다. 이번 최신 세 바이너리는 이번 보고서에서 직접 대조한다.

기존 `decomp_fort_image_packets.py --verify`는 `tools/decomp_priestmenu.py`의 과거 SHA 차이로 종료1이다. 현재 파일은 시작 커밋 `8c653e23`과 같은 SHA `8d1474792e7511fa777e7bd7bd369f2a2a0f9f5c35643e420d65a3fa4744bac9`이며,과거 보고서의 기대 SHA는 `02fcb4d010e0558e969eaf3e9c55bc21d14779d8eb147d3fc2ecbcaeca7a9c2c`다. 줄바꿈 차이도 아니다. 기대값/기존 도구를 바꿔 통과시키지 않고 기존 문제로 기록한다. 이 감사를 새 통과로 세지 않는다.

최종 정적 감사 **10개가 종료0**이다. 새 Template 연결/v2 원문 보존,기존 요새 요청·Init·미션 수명·기술 허용·공통 프레임·기본 옵션·Client Help와 State 확장99입력/198반환의 원문 감사를 포함한다. 위 기존 Fort 이미지/패킷 감사1건은 종료1로 별도 기록했다. 현재 소스 핀 **140보고서·434항목**만 갱신하고 이번 이력/현재 핀을 되돌린 전체 JSON이 시작 커밋과 같은 것을 확인했다. `o/CMakeLists.txt`는 LF로 고정한 뒤 빌드했다. 과거 원문/기대값/실행·바이너리와 기존 역사 핀 차이는 유지한다. 최초 거부 자료3개/보정 유효 자료3개 및 최신 세 바이너리의 전체 바이트 해시도 보존 자료·이번 보고서와 대조한다.

인수인계 뒤 **20261021-22 CP**로 로컬 커밋하고 사용자 요청에 따라 목표를 일시 정지한다. 원격 push는 하지 않는다. 이번 단위 뒤에 다음 구현을 시작하지 않는다.

이 연결은 원본 전역 표의 소유·빌림과 실제 호스트 버퍼/manager 수명이다. 원본 DataManager의 전체 MemStream 링크/파일 생성·저장/오류 처리·가상 소멸을 모두 복원했다는 뜻은 아니다. 전체 비동기 State7/전투 정리4b2df0·Fort 선택/Guide 공장·가상 소멸과 전체11미션 자연 완주·외관/음향/동작 동일성은 미완료다. 로그인은 여전히 상태8부터 시작한다. 재개 시 이 항목들과 이월 C++검사/실제 통합 플레이에서 이어간다.
