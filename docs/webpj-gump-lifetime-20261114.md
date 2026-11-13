# webpj Gump 지연 삭제·소멸자 수명 (WB04-E412, 2026-11-14)

RawMenuControls 선행 `client/GumpLifetime.cpp/.h`253줄을 이식했다. 기준선 `1f00a25a`의 전체 몸체와 GumpLifetimeTests/GumpCheckingParentsTests/GumpResetTests, 독립 관측 세 파일을 읽었다. 기준선 뒤 두 소스 변경은 없다. CD/추가판과80칸 idle 분기는 웹10.78 범위에서 제외한다.

## 현재 저장소와 해제 순서

`ui/gump-lifetime.js`는 같은 GumpCreateState/Node와 idle500·overlay100·삭제큐1000·dialog16DWORD를 사용한다. 실제 checking은 현재 getter를 빌리고 null일 때 기존 독립 diagnostics의 전체DWORD를 소비 때마다 읽는다.

- idle은 첫 빈/일치 하나, overlay는 전체100칸을 읽은 마지막 빈/일치를 고른다. overlay 진단은 선택한 위치를 고정하여 복귀 뒤 다시 검색하지 않는다. 순수 부모의−1 쓰기는 장치가 관찰할 수 있고 실제 배열 어댑터는 쓰기 직전에 범위 오류로 거부한다. 미선택 노드/진단은 요구하지 않는다.
- Kill은 자식 우선으로 현재 idle/overlay·Form·가상 Push·큐·focus/override/dialog·dead를 처리한다. 요청 중복을 제거하지 않는다. 자식 next는 삭제 전에 저장하며 해제된 자식을 다시 조회하지 않는다.
- Cleanup은 비0큐 항목의 Destroy 직후만 현재 개수를 다시 읽고 null칸에서는 로컬 한계를 유지한다. Destroy는 현재 등록 칸을 지운 뒤 개수를 줄이고 dead를 해제/destroying을 세운 다음 실제 가상 소멸자를 호출한다.
- 기본 소멸자는 VT 복원→선택 진단→현재 첫/두 번째 텍스트 해제→overlay/idle/큐의 모든 일치 제거다. 등록·dialog·플래그는 기본 소멸자에서 바꾸지 않는다. DeleteScalar는 bit0일 때 객체를 해제하며 뒤에서 객체를 읽지 않는다.
- KillExceptType은 루트 밖999칸을 현재 등록표/제외 signed BYTE getter로 읽는다. ReplaceRoot는 기존 루트 Kill과 현재 큐 Cleanup 뒤 새 루트/id0을 쓰며 count/serial이나 새 객체의 이전 등록 칸을 자동 초기화하지 않는다.

## 검사

새 검사 **7/7 첫 통과**, 주변 생성/트리/수집/Draw와 함께 **38/38 통과**했다. 이후 실제 동종 교체 지연/잘못된 Cleanup dead/overlay 중복 장치 검사를 보강한 최종 집중7/7도 첫 통과했다. 제품/기대값 교정은 없다.

1. `gumplifetime-x86.tsv` originals **173행**의 전체160BYTE 노드·다섯 고정 표·7전역·반환·가상/해제 콜백 당시 전체 상태가 같다.
2. `uireset-x86.tsv` originals **121행**의 제외BYTE/현재 등록 전체·루트 교체·9전역/다섯 표·콜백 당시 상태가 같다. CD 행은 읽지 않는다.
3. `gump-checking-parents-x86.tsv` **368행(overlay320·기본 소멸48)**의 진단 복귀/중단·정확한 조건/파일/줄 ASCII·현재 checking/플래그/포인터·전체 표와 명시 인접DWORD를 대조했다. 진단 콜백이 후보 칸/텍스트를 바꿔도 원문 순서다.
4. 실제 GumpCreate→GumpTree→Kill→Cleanup→기본 소멸자/Map 해제→등록 칸/serial 재사용→같은 GumpCollection을 연결했다. 중복 Kill도 실제 큐에서 처리한다. 동종0x4000 교체는 기존 노드를 즉시 해제하지 않고 실제 지연 큐에 넣는다.
5. 실제 제외 필터·자식 부모 Kill·생존 자식 이동·루트 교체·기존 임시 칸 해제와 새 칸 재사용을 CPP 조립 시험과 대조했다.
6. 현재 checking 대여/독립 진단 전환, 미선택 노드 조회 생략·중복 overlay 진단 뒤 마지막 빈 칸 유지·−1의 실제 배열 범위를 검사했다.
7. 기본 소멸의 모든 중복 제거/등록·dialog·flags 보존과 토큰·표/플래그·큐/dialog/id 한도·순환·dead 거부, 실제 가상 소멸/텍스트 해제 예외 앞에서 이미 쓴 상태 보존을 검사했다.

원본 자료 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3933/3933 통과**, 실패/취소/건너뜀0(320.9초/356.3초). 대응표 오류0, port663·partial58·todo175이다.

## 다음

같은 생성/트리/수명 저장소에 GumpEvents/GumpInput/GumpFrame과 현재 focus/overlay/가상 이벤트를 연결한다. 파생 메뉴·RawMenuControls와 실제 ClientRendererFrame/Session/앱 수명은 아직 미연결이며 미션은 플레이할 수 없다.
