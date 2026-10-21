# Template 생성자·파일명 조회·바깥 Load 복원

2026-10-21, 현재 및 마지막 전체 정밀 디컴파일 PC는 **VM-W11-CODEX**다. 기존 `extracted/refined/originals/Netstorm.c`·`functions.tsv`와 보호된 `originals/Netstorm.exe`의 명령을 읽었다. 새 디컴파일이나 원본/CD 수정은 없으며 사용자가 중단한 게임·집중 검사·희생 검증은 재개하지 않았다.

## 원본에서 확인한 동작

`004bcc60`은 276BYTE manager의 head/current를0, vptr를`00514690`, +10c를0, +110을`ffffffff`로 쓴다. 별도 DWORD `005c89c0`도0으로 쓴 다음 +12에 파일명을 첫 NUL까지 복사한다. 이름 뒤256BYTE 배열의 나머지는 쓰지 않는다. 정밀 C 원문은 반환형을 void로 표시하지만 실제 명령은 첫 `mov eax,ecx`로 받은 this를 EAX에 유지한 채 `ret 4`한다. `004bcca0`은 같은 +12 주소를 반환한다.

`004bf550`은 기존 ptr가 있으면 가상 파일명과 요청 이름을 CRT `stricmp`로 비교한다. 일치 시 그대로 반환하며 불일치도 `isCalled(_fileName)` 진단 후 반환한다. 기존 manager를 교체하거나 커서를 초기화하지 않는다.

빈 헤더에서는 +4의 빌림 값을0으로 쓴 뒤276BYTE 할당/생성자를 거쳐 ptr를 설치한다. 기존 +8이 비영이면 판정을 건너뛰고1로 정규화한다. 영이면 가상 이름으로 `0041af80`을 호출해 읽기 전용 여부를 얻는다. +12는 보존하고 +16만0으로 만든 뒤 현재 ptr에 파일 읽기 `0044d980`을 호출한다.

읽기가 성공하면 현재 ptr가 비영이고 this가 `Template9[currentIndex]`인 경우에만 `00460730`으로 창 제목을 갱신한다. 실패하면 그때의 현재 ptr를 저장해 base DataManager 소멸 `0044da60`·free를 순서대로 수행하고 ptr만0으로 만든다. 소멸 효과가 헤더 ptr를 바꾸어도 free는 저장한 포인터를 쓴다. 기존 단일 Template 반납의20BYTE 전체 초기화와 다르다.

`0041af80`은 **존재 여부가 아닌 읽기 전용 판정**이다. 해석한 파일 출처2/4는 true로 반환하고 그 외에는 CRT stat 결과의 모드 BYTE 부호를 확인한다. 이 판정과 +8의 용도는 `00460730`의 `readonly` 제목 문자열 분기로도 확인했다. 전체 파일 경로 해결/CRT stat 구현까지 복원한 것은 아니다.

## 적용 범위

- `RawTemplateLoad`에 생성자·같은 바이트의 파일명 getter·바깥 Load 제어 몸체를 복원했다. 하위 할당/생성/이름 비교/읽기 전용/파일 읽기/제목/소멸/free는 명시 효과 경계다. 선택하지 않은 경계의 연결을 강제하지 않는다.
- 실제 `CampaignTemplateStorage::Image`는 원본 생성자와 같은 manager의 파일명 getter를 사용한다. 별도 생성자 전역도 같은 Client 수명 저장소에0을 쓴다. 다른 소비자는 전체 State7 복원 때 연결해야 한다.
- 기존 `InstallFile`은 이전 파일을 교체하는 호스트 API다. 실제 Session/로그인의 교체 의미를 검토하지 않고 이 API를 원본 Load의 기존 파일 유지 동작으로 바꾸지 않았다. 새 바깥 Load 몸체의 실제 Session 연결은 후속이다.
- 원본에는 없는 물리 범위 보호로 manager 부족, 256BYTE를 넘는 이름, getter의 NUL 누락을 예외로 거부한다. 생성자 파일명 입력은 출력 manager/전역과 겹치지 않는 계약이다. 실제 호스트 manager 배열의 최초0 초기화를 원본의 미정 할당 바이트 관측으로 주장하지 않는다.
- 이름 metadata는 기존 API의 참조 반환을 위해 유지한다. 파일 읽기의 가상 이름은 manager+12에서 읽으며, 기본 fort 식별자와 원본 전체 파일 경로의 정체성이 같다고 주장하지 않는다.

## 확인과 남은 작업

새 `TemplateLoadTests.cpp`의5검사는 생성자의 지정 쓰기/미정 tail 보존, getter의 같은 메모리, 기존 파일 유지, readonly/현재 슬롯 분기, 하위 효과 이후 ptr 재조회, 저장 포인터 free, 실패의 부가 필드 보존 및 NULL 경계를 다룬다. **작성·컴파일만 하고 실행하지 않았다.**

Release 빌드는 `extracted/cpp-template-load-20261021-build1.log` 및 `-result.json`에 종료 코드를 별도로 보존한다. 정적 기록 `cpppj/template-load-static-20261021.json`과 `tools/cpp_template_load_audit.py`는 원본 함수 범위·명령·작업 트리 입력/바이너리 SHA·등록된 검사 이름을 읽기만 한다. **C++ 검사/CTest/게임/native 명령 실행0**이며 원본 명령 수나 컴파일 등록을 실행 통과 수로 세지 않는다. 기존 단위270 정적 기록과 과거 실행 보고서는 덮어쓰지 않는다.

후속은 새 바깥 Load와 실제 파일 출처/readonly·현재 슬롯/제목의 연결, 기본 전체 이름 표 생성0044d870, 저장, 전체 State7/전투 정리004b2df0다. 사용자 중단 상태의 스테이지 클리어/희생 GUI는 별도 재지시 전 보류한다. 이번 단위는11미션 자연 플레이·AI·표시/음향/동작 동일성의 완료가 아니다.
