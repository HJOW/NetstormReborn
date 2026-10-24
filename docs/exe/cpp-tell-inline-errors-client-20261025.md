# Tell % 행별 ERROR 링크 복구 — 2026-10-25

단위411은 단위410에 남긴 `%` 행별 예외 처리를 복원한다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 새 전체 디컴파일·원본 게임 직접 실행·이번 단위 클론 GUI 실행은 없다. 전체 `/goal`은 활성/미완료다.

## 원문 범위와 결과

원문 `4ce069..4ce0ed`는 제목 처리가 끝난 뒤 EH 상태6을 쓰고 첫 인자 치환부터 현재 링크 작성까지 처리한다. `4ce0ef` Catch는 이미 만든 String을 되돌리지 않고 `\n<a herf="cmd:명령,">~rERROR 제목</a>`를 덧붙인다. 이어 `4ce144`에서 현재 인자/행 변수를 다시 읽고 `4ce14d` 공통 꼬리로 넘어가 지역 cur를 반납한 뒤 다음 행을 진행한다.

따라서 첫 조회가 실패하면 ERROR 링크만 추가되지만, 두 번째 조회가 실패하면 앞서 쓴 정상 링크의 시작 조각도 남는다. 정리된 단일 오류 링크로 대체하거나 부분 출력을 롤백하지 않는다. 제목/파일 수집 중의 오류는 이 행별 try 범위에 포함되지 않는다.

[TellFileMenus::Inline](../../cpppj/src/client/TellFileMenus.cpp)은 이 범위에만 C++ Catch를 연결한다. 현재 실제 Config/VFS가 던진 예외를 복구하고 지역 cur/임시 probe Config를 같은 RAII 수명으로 반납한다. 원본 지역 오버런에 해당하는 기존 명시 거부는 private `length_error` 파생형으로 구별하여 ERROR 링크로 성공 대체하지 않는다. 따라서 제목 오류와 물리 범위 오류는 호출자에게 계속 전달한다. @/# 일반 목록과 % 정상 경로는 유지한다.

## 독립 관측과 검사

[관측 도구](../../tools/decomp_tell_inline_errors.py)는 정상 본체와 실제 Catch/현재 행 복구/공유 꼬리를 **336입력×두 정밀도=672행** 실행했다. 그중 실제 Catch 전달은384회다. 파일 순열/중복·기존 Config·빈 제목/구분선과 첫/두 번째 및 후속 행의 선택된 조회 실패를 입력으로 사용했다. 파일/VFS 조회/실패 순서, 남은 부분 문자열과 ERROR 링크 및 다음 행을 포함한 전체 HTML이 C++와 같다. 두 정밀도 차이0이다.

선택된 Config 예외 발생과 EH의 Catch 진입/저장ESP 복구/반환 continuation 전달은 **명시 장치**다. 실제 Windows EH 런타임 전체나 그 unwind를 실행했다고 주장하지 않는다. Catch 본체의 추가 할당 실패와 임의 비동기 예외도 관측 범위 밖이다. FileSpec/Config/String/할당/CRT 경계는 단위410과 같다. 보호 PE 코드는 실행 전후 동일하고 파일을 수정하지 않았다.

[목록 검사](../../cpppj/tests/TellFileMenusTests.cpp)는 실제 ConfigInterface→missionSpec→Reader의 n번째 조회에서 C++ 예외를 던져 원문672행과 대조한다. 제목 치환 오류가 전달되고 실제 임시 Config가 반납되는지, 지역 오버런 거부가 유지되는지도 검사한다. [실제 HTML 메타 결합](../../cpppj/tests/HtmlWindowMetadataClientTests.cpp)은 같은 전체 부모/고리/controlled 메뉴의 첫·두 번째 조회 실패를 복구한 뒤 정상 반납한다. 기존 @/#384행과 % 정상288행도 같은 자료를 유지하며 통과한다.

Release 빌드 종료0, 최종 집중 **Tell22+실제 메타10=32개/실패0**, 전체 CTest **2784개/실패0·300.41초**다. 보호 감사에서 **originals1394/CD425/AGENTS/정밀 소스=1821개**가 시작 시점과 같다. 제품 SHA256은 `20b7b21f4e177182565c9280f6ff4e1fce66e101fddab57b1883161c9f388485`, 검사 SHA256은 `a680866388561bf4aed7bd4300cfb62d3e70fbde9527c7b305421661e0f19cfa`이고 집중/전체에서 같다. [제품 보고서](../../cpppj/tell-inline-errors-client-runtime-20261025.json)에 소스/참조·원시 SHA와 한계를 기록했다. 원시는 `extracted/cpp-tell-errors-411-*`다.

## 다음 범위

기존 Tell의 전체 부모와 구형 % 표시 어댑터, 기술/Input/전송·배경/Checked 전역·Guide/실제 HTML 개수·full Uber 이행이 남는다. Production/Minimap·State/Core/Pool/Win32와 전체 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오·완전 동일성 및393 Wide SaveOptions는 미완료다. 원본10.72는 별도 비교 대상으로 유지한다.
