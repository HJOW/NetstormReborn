# webpj 기술 목록 TechnologyListMenu (WB04-E450, 2026-11-14)

기준선 `1f00a25a`의 `TechnologyListMenu.cpp/.h` 전체를 `www/ui/technology-list-menu.js`로 이식했다. 기준선 뒤 변경은 없다. 원문 주소는 004cb320/004cb3f0/004cb4b0/004cb690이다.

- `countKnown/countCandidates`: 현재 기술 부모의 선행 조회, 조건마다 현재 TYPE 수 조회, 사제/group 10 제외, theme 5의 레벨 무시, 잘못된 레벨 진단 후 계속, 실제 소유 종류 WORD 판정을 보존한다. 호출자가 빌려준 stored를 초기화하지 않으며 출력 용량과 전체 일치 수는 다르다. NULL 목록은 저장하지 않는다.
- `appendTheme`: 대상 DWORD를 유지한 가상 레벨 조회, 알려진/후보 각 40DWORD, 전체 후보 수만큼 저장 개수로 순환 방문, 레벨 순서·신전 상품 제외·소유 기술 비활성화·번역→현재 표시 레벨 캡처→설명→서식→현재 명령→메뉴 행 순서다.
- `appendAll`: 이 진입만 대상 WORD로 접고 네 원소의 선행 NULL 수집·제목/행/구분선, 현재 표면/전체 보유 수, 업그레이드 또는 최대 레벨/랭크와 SP 보상 행을 작성한다. 본섬은 getter를 먼저 보관하고 섬 조회 효과 뒤 현재값을 읽는다. 랭크 분기는 소스대로 정적 이식했으며 새 MP 검사는 없다.

원본 fixture `technology-list-menu-x86.tsv` 367입력의 전체 반환·stored·40DWORD·효과 순서와 `technology-all-menu-x86.tsv` 100입력의 전체 효과·문구·활성 상태가 첫 실행부터 일치했다. 각 해시는 fixture manifest의 동명 두 항목에 있다. 신규 검사 6건은 이 467입력과 NULL/빈 TYPE·저장 인덱스·현재 TYPE 수 변경·하위 장치 누락 뒤 부분 효과·대상 DWORD 유지·40칸 초과 순환도 확인한다.

전체 회귀는 Node v26.13.0·v22.23.3 각각 **4,658건 통과, 실패 0**이다. 대응표 984행 오류 0, fixture manifest 형식/해시/등록 검사 3건도 통과했다. Node v24는 설치되지 않아 미실행이다. cpppj의 HTML metadata 통합 검사 2건은 해당 모듈이 아직 웹에 없어 후속으로 넘긴다. 실제 RawMenuControls/HTML/current Session 조립과 화면 출력은 별도 단위이며, 이 검사는 실제 게임/브라우저 실행이나 제단/희생/생산 관측이 아니다.
