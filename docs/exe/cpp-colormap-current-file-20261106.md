# 현재 색표 캐시 FILE과 실제 표시 연결 — 단위967

**후속968:** 두8행/마지막 표는 직접 참조 범위에서 별도 소비자가 확인되지 않았고, 첫 표의45bcd0 추가 Draw를 원문 코드로 연결했다(공중 전용 검사 제외). 최초 팔레트도 현재 Native FILE/clear1로 연결했다. [968기록](cpp-colormap-consumers-startup-20261106.md).

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 산출물을 수정하지 않았다. 우선순위4의 실제 파일 소비자를 대조하다 찾은 `!color.dat` 공백을 연결했다. 제품967/11.01이다.

## 공백과 구현

기존 `RawColorMapCache`에는 PATCH0043c4e0의 부모·catch·cleanup이 있었지만 실제 시작의 호출은 없었다. `ClientMain`은 TYPE별 owner 표만 초기화했고, `ClientColorSortInput`의9표는 표시기가 사용하지 않았다. 건물·커서·생산·메뉴·Screen은 같은 초기RGB에서 표를 따로 계산했다. 단순 파일 읽기를 더해도 표시에는 반영되지 않는 상태였다.

새 `ClientColorMapFile.cpp`는 기존 원문 부모에 실제 현재 CRT/VFS·키 수명을 연결한다. 최초 primary에서 두8행 표를 쓴 뒤 기존 owner 등록을 호출하고, 캐시 hit이면 같은 장기9표에 순차 읽기한다. 정확한 `ColorCacheUnableToOpen`만 재계산→writer→9write로 이어진다. 새 캐시의 부모 폴더는 Client 시작이 사용자 `Cache`에 만들며, 파일 장치는 임의 부모를 만들지 않는다. 원래 설치DataDir 경로 대신 사용자 홈 `Cache/!color.dat`를 쓰는 것은5차 요구의 명시 호환 차이다.

원본 PE의8DWORD는84/249/184/140/150/112/156/222이고 파일 목적지 순서는556038/557738/556438/556238/556138/556338/555f38/555e38/557838이다. 실제 주소 순서9표의 인덱스로는 **2/7/6/4/3/5/1/0/8**이다. 정밀 부모와 PE를 읽어 재확인했다. 새 Native 실행이나 fixture 변경은 없다.

실제 시작은 TYPE 크기 확정 뒤·첫 UI/World 앞에서 초기화한다. 건물의 bright/dead/shadow, 커서 destroy/illegal/labelShadow, 생산 disabled/underlay와 명암, 메뉴 그림/도움말·meta disabled, Screen 명암을 같은 현재9표에 연결했다. 지연 커서뿐 아니라 월드 생성 직후 첫 건물 표시에도 전달한다. Screen과 메뉴는 Draw 조회 시 현재 표를 읽으며 팔레트 재생성은 같은9표 주소를 갱신하고 기존 `PaletteChanged` 경로를 거친다. 기존 독립 호출의 선택적 인수 생략과 headless fallback은 유지했다.

CD10.72 실제 캐시 부모는 이번에 새로 접합하지 않았다. CD/호스트는 기존 값형 계산을 유지하며 PATCH 파일 부모를 판본 확인 없이 호출하지 않는다. 두8행 출력은 이제 같은 Client 수명이지만, 그 표의 모든 별도 표시 소비자까지 연결됐다는 주장은 하지 않는다.

## 검증

첫 Release 빌드·집중12검사부터 실패0이었다. 이후 첫 건물 표시 전달과 암호TAFF 검사를 보완한 최종 게임/집중 Release도 종료0·경고0·오류0이다. 최종 **고유13검사=새5/기존8, 실패0**이다. 전체 CTest 결과로 확대하지 않는다.

보고서 생성 때 Python 키워드 인수 구문 오류가 한 번 있었다. 제품/검사는 이미 종료0인 상태였고 해당 스크립트는 실행 전 컴파일 단계에서 멈췄다. 오류 메모를 보존하고 보고서 생성식을 교정했다. 기존 파일의 혼합 줄바꿈은 동일 행의 HEAD 바이트를 보존해 무관한 전체파일 변경을 피했다.

- 새 cache miss는 기존 Native53비트 자료의9표 전체와 같고 파일 순서2304BYTE도 같다. 두8행 출력4096BYTE·owner 등록1회, 새 소유자의hit와 다른 빨강에서의 비재계산을 확인했다.
- 명시 sentinel9표와 후행BYTE를 실제 파일에서 읽었다. Screen의 실제 조회가 그 밝음/어두움 행을 사용했고, 후속 재생성 뒤 같은 주소의 새 표를 사용했다. hit/재생성은 디스크 캐시를 다시 쓰지 않았다. Screen::Init/창/표면을 만들지 않은 조회 검사다.
- 실제3BYTE 파일의 fread는 첫 목적표 접두만 바꾸고 다른 BSS0/tail을 유지했다. 원문 부모처럼 재생성·자동 수정하지 않았다.
- 부모 부재의 reader miss→표 생성→writer 실패를 정확 marker로 확인했다. 선행 owner 등록과 생성 표는 유지됐고 부모/성공 파일을 합성하지 않았다.
- 등록된 암호TAFF의 payload를 바꾼 뒤 같은 metadata/current FILE에서 새9표를 순차 복호화했다. 이전 snapshot은 원래 값이고 반납 뒤 실제 archive rename도 성공했다.
- 기존 부모/catch/cleanup Native130관측, 전체9표6관측의53/64비트 구분과 초기 primary/특수 owner 회귀도 같은 집중 대상에서 통과했다. 이번의 새로운 CPU 관측으로 세지 않는다.

## 실제 실행

`mcp_cpp_mission_abort_smoke.py`에 선택적 `--user-data`를 더했다. 기본 새 격리 루트는 유지하며 명시 재시작 검사만 같은 검사 프로필을 재사용한다.

현재967 제품으로 **실제2프로세스**를 순서대로 실행했다. 첫 miss는 입력37/관측26, 둘째 hit는 입력35/관측24이며 각각 새 진입2회/Replay2회·Continue/Abort·메인 복귀·종료0·설치자료 불변이다. 실제 관측 미션은 **tutorial1 / Tutorial**이다. 도구의 과거 고정 unit945 표기를 현재 단위 번호나 다른 미션의 증거로 해석하지 않는다. 포획/공격/희생/승리·MP는 실행하지 않았다.

첫 실행의 캐시2304BYTE SHA256은 `ac1205f05c99c526235676c93f06ad6c6a60c49e161e3fef3cd5a695fa1eaad1`이다. 같은 프로필 재시작 뒤 내용·길이·mtimeNs가 모두 유지됐다. 미션/메인 PNG4장(800×600)을 직접 확인했다. 캐시 적재와 정상 왕복이며 전체 픽셀 동일성·모든 색표의 모든 게임 조합 검증은 아니다.

보호 **2796개/fixture976개 불변**, 새fixture0이다. 다음 기준은 `extracted/cpp-resume-967/protected-final.json`이다. 보고서·지문은 [JSON](../../cpppj/colormap-current-file-20261106.json), 원시는 `extracted/cpp-resume-967/`에 있다. 솔루션은 새 집중 대상 추가로83프로젝트(Application78/라이브러리2/Utility3)가 됐다.

32비트 FILE24BYTE/전체SEH ABI·물리 디스크/할당 실패·미정 heap 자체 일치는 현대 호스트 한계로 남는다. 다른 Factory/파일/비멀티 수명·두8행의 남은 표시 소비자와 원문 색표 소비 전체 대조는 계속한다. 다음968도 우선순위4이며 전체 목표는 미완료다.
