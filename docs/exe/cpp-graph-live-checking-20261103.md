# Graph의 현재 실행과 진단·로그 복귀 — 단위818

실제 `RawGraph`의 Add/Flood/Allocate/Rebuild/PostPop/PostDestroy를 사본 Calculate/Commit에서 현재 풀·255개 기록·4096 DWORD stack·지도 순회로 옮긴다. 원문의 현재값 읽기와 WORD 감김·처리 순서·장부 차이를 유지한다. Validate는 사본에만 적용하는 사전 검사로 남긴다. 실제 Free/sentinel은816, 현재 flags8 이웃 탐색기는817의 부품을 재사용한다.

원문10.78은 `00462f10` Flood·`004633c0` Add·`00463110` Rebuild·`00463330` Allocate이며 CD10.72는 `0045bb90/0045b7d0/0045bd60/0045c390`이다. sentinel의 CD 실제 대응은 빈 `0045b560`이다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 기존 산출물을 읽었다. 이번에 원본 실행 파일이나 디컴파일 산출물은 변경하지 않았다.

Add는 sentinel→보관한 위치 유효성→현재 상태→현재 flags8 이웃 수집→가장 큰 첫 연결 선택→현재 행/graph BYTE 쓰기→순차 Flood/패배 행 해제→사용 수 로그 순서다. 이웃 SID를 보관하고 뒤 비교 때 현재 graph BYTE를 다시 읽는다. winner 크기 초기값 `0x80000001`은 실제 어셈블리로 확인했다. 진단 `g->inUse`, 패배 크기, 병합 수의 현재 checking을 각각 다시 읽으며 10.72에서 제거된 진단은 적용하지 않는다.

Flood는 실제 LIFO stack의 중복 push·사용 후 흔적을 유지한다. 이전/목적 행의 count는 low16비트로 증감하고 각 현재 슬롯의 graph BYTE를 쓴 뒤 이웃을 확장한다. 사용 수를 세는 원문 level0x20 로그 뒤 sentinel을 다시 읽는다. Rebuild는 최초 sentinel을 Reset 전에 검사하고, 전체 SID 두 순회 사이에도 앞 Flood/로그가 바꾼 현재 번호를 읽는다. 임시 행의 reserved는 유지한다. 실제 World는 전체 풀 복구 정책으로 연결하며 부분 입력의 명시 Reject 정책은 독립 RawGraph에 남긴다.

Client는 장기 `RawSurfaceStorage`에 현재 trace writer를 연결하고 World의 실제 Graph가 이를 빌린다. `WriteDiagnosticLog`의 기존 수준 필터·들여쓰기·FILE 수명을 사용한다. 독립 raw 입력의 trace는 생략할 수 있다. postDestroy는 현재 일반 finder의 다음 체인 저장과 순차 Add를 사용하며 미리 수집한 대상 목록으로 바꾸지 않는다.

독립 PE144행은 Add/void/좌표0/Flood/Rebuild(false/true) × checking0/고위 비트 × 진단/로그 변경6종 × 양판본이다. 전체 기록6BYTE×255·전체 stack DWORD×4096·입력6슬롯 전체와 이벤트를 저장한다. checking 끄기, 다음 후보 번호 변경, 현재 count/reserved 변경, stack 마지막 DWORD 변경, 뒤 지도 셀 제거, 로그 후 sentinel 손상/진단을 포함한다. 별도24행은 첫 로그에서 뒤 후보의 free 비트를 켜는 원문을 관측했다. 콜백은 명시 입력 장치이며 결과를 Python 모델로 계산하지 않는다. 게임·GUI·OS·소켓은 실행하지 않았다.

최초 관측 준비는 Native SID1~6을 C++ 할당5~10으로 옮겨 전역 iterator의 예약 번호 의미까지 바꿨다. `0040eb40`은 초기1에서 선증가해2부터 읽으므로 이 입력은 재구성 비교에 같은 입력이 아니었다. 최초144행 중 재구성48행 불일치와 원문 파일은 `extracted/cpp-resume-818/reserved-sid-*.tsv/.json`·`test-live-first.log`·`test-live-diagnostic.log`에 보존했다. 양쪽 모두 실제 할당5~10과 TYPE75~80을 사용하도록 입력을 교정해 다시 관측했다. stack은 이제 SID 정규화 없이 전체 바이트를 직접 대조한다. 제품의 전역 순회 시작도 원문 Next에 맞는2로 명시했다.

또한 내부 Flood에 외부 입력 검사를 반복하면 재구성의 contained/void와 로그가 free로 만든 뒤 후보를 새 수명 관문으로 거부했다. 최초 기존 raw 검사2건과 추가 free 검사1건의 예외를 보존하고, 외부 직접 Flood만 최초 물리 검사를 수행하도록 구분했다. 이후 남은 복구 회귀1건은 Inspect의 부분 풀 판정이 FullPool의 지도 밖 contained/void 계약을 거부한 것이어서 기존 전체 풀 정책에 맞춰 제품을 교정했다. 실제 내부 호출은 기존 검사 이후 바뀐 현재 필드를 소비한다. 기존 영역 검사는 실제 Add 연결 전의 참조 감소 두 진단만 허용해 최초3단언이 실패했다(추가 상태 진단1개 뒤 행 수 검사2개). 이 listener는 별도 PE144행에서 관측된 식/파일/행 목록만 추가로 허용하며, 기존 슬롯/표/지도/stack·통계 기대값은 바꾸지 않았다.

Detach의 sourceType 치환과 기존 사본 분할은 다음 단위에 남긴다. 따라서 Flood와 sentinel의 **모든** 실제 소비자 완료로 세지 않는다. Add의 두 실제 소비자인 postPop/postDestroy와 Rebuild/Flood의 현행 연결은 집중 검사 결과와 함께 완료 판정한다. 기존 물리/범위/미지원 입력 보호와 원문 메모리 밖/주변 스택의 복제 한계는 구별한다. 실제 공격·사제 포획/희생·승리·공중3종 전용·MP·물리/PCM 검사는 제외한다.

Release 명령4회(최초/상세/수정/전체 풀 정책 교정)가 모두 종료0·경고/오류0이다. 최종 C++ 집중34개(새3)는 실패0이며 새168행과 기존 raw5094연산·checking 영역1536연산(참조18회/추가 원문 부모64회)을 확인했다. 실제 자산 `--dump-world originals Tutorial1` 초기 로드/Resize/Report도 종료0이다. 기존 보호2,742개/fixture922개 불변, 새2개를 포함한2,744개/924개도 콘솔 실행 후 변경/추가0이다. EXE818/11.01 SHA-256은 `04b63384d5f75d1a81a41a08c768108e0a6648c0f4ca10a3ff4bf0659080f648`다. Add의 원문 두 호출자004b0840/postDestroy·004b0d30/postPop을 연결해 checking 잔여는26(부모18·부분5·MP3)이다. Flood/sentinel의 Detach 소비자는 다음이며 전체 원본 동일성 완료로 세지 않는다. [보고서](../../cpppj/graph-live-checking-20261103.json)를 따른다.
