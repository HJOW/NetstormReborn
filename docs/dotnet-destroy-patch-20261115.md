# dotnetpj 0.539 — 공통 삭제의 실제 PATCH 진입과 같은 풀 반납

2026-11-15, HJOW-Athlon. 직전0.538은2e0d86815다. 기준은cpppj `o/RawSquidDestroy.h/.cpp`의417f22d2967decbf2ad3a587c2190b9efb93f772다. 공통삭제/사제피해의제품조립을확인하다부모의실제PATCH경로가미이식임을확인해선행단위로분리했다. 제품호스트에는아직이경로를켜지않았다. 디컴파일산출물은변경하지않았다.

## 구현

SquidDestroyDiagnostics는선택위치의복귀단언·level0x27로그·최초로딩비용표를대여한다. 명시BindPatchDiagnostics가실제PATCH경로만선택하며CD진단연결은거부한다. 함수는값사본으로보존하고표는같은수명으로빌린다. 콜백이진단을재연결해도현재호출수명은보존하고다음진단은현재연결을읽는다. 최초비용표/현재TYPE보기는후속공통수명주기가사용할경계이며이번에는소비자를연결하지않았다.

RawSquidDestroy.Patch는dead선기록→free복귀단언/권한→pre깊이증가/현재TYPE이름NUL앞로그→가상Pre/깊이단언→현재종속체인→전파/현재전체선택DWORD→현재VT의가상Unpop→가상Post/깊이단언→실제반납순서를구현했다. next는두번째free단언보다앞에서저장하고Release뒤free는다시읽는다. upfront장치/체인선검사로앞효과를없애지않는다. 현재가상+48은raw TYPE가아닌실제네Form VT로분배하며다른미지VT는선택지점의명시실패다. 원문에없는손상순환은이전효과를보존한채호스트물리경계에서만중단한다.

RawSidPool.ReleasePatch는원문void/free단언복귀뒤현재TYPE기록/슬롯zero·종류보존·현재서버반납·같은client/server FIFO/tail단언·freeCount를처리한다. C++friend가접근하던private풀저장소를C#같은풀내부메서드로옮겼고독립Release의선검사/효과는바꾸지않았다. root도이실제반납기를호출해FormUnpop이void를쓰지않고복귀한경우Post뒤에반납단언이일어난다.

전체선택DWORD는원본같은파일의독립진입에도전달한다. SelectedRaw만제공해도동작하고고비트를SID로미리좁히지않는다. 기존Selected/SID만제공한계약·독립188TYPE구성·typed선검사는유지한다. 새필드를다른훅복제기/현재Form·ProcessHost/공통삭제수명으로전달하는상위cpp변경은다음단위에서함께반영한다.

## 대조와 현재 계약

기존destroy-x86의PATCH600·Bridge64, 합계664고유입력행을기존/명시PATCH두경로로재생해1328실행의외부가상효과·50바이트슬롯전체·풀카운터/머리/꼬리·풀/해시/spot/이력체크섬·선택·pre/post깊이를대조했다. 독립기대값은원본PE이며기대값을C#결과로재생성하지않았다. 기존fixture가포함하지않는진단/trace출력만명시장치로받는다. CD/1037행은이번대조로세지않는다. 새fixture는없다.

현재계약10검사는dead에서장치미요구·free단언뒤상태구제, 원격SID거부/dead보존과server-origin/현재반납권한, 함수사본/NUL로그·선택DWORD, 로그장치누락전dead/깊이부분효과, trace중진단교체, 두번째free단언앞next저장과Release뒤현재free, 복귀깊이단언, 현재FormVT/선택후장치누락, void없는Form복귀의Post뒤실제반납단언, 중복반납의기록/tail/FIFO를확인했다. 네추가경우는두진입의전체DWORD함수만제공/상위비트비교다. 원본재생4검사+현재14경우=집중18검사를통과했다. Source기반합성계약은별도PE관찰행으로세지않는다.

선택DWORD축소·두번째free단언뒤next조회·독립typed반납사용변이는각실행시현재10검사중1개실패로검출했다. 각제품Native파일을바이트/SHA로복원했다. 그뒤독립진입의선택함수추가계약4경우를더해18검사재통과했다. 새4경우를이전변이실행개수에합산하지않는다. 최종전체비증분Release빌드는경고0·오류0, 관련Core316/316·Client45/45는실패0·건너뜀0·각종료0으로통과했다. 최근전체Client회귀1232/1232와실제31단순미션진입은직전0.538의결과이며이번집중범위와구별한다. 자료는extracted/dotnet-destroy-0539-initial/·-mutations/·-final/에있고Git제외다.

## 다음 연결

SquidDestroyLifecycle의현재TYPE·장기장부/보상·선택/기억점·손실위치·목록/AI통지와선택DWORD의훅전달이후속선행이다. 그뒤제품damageable전처리/종속해방·사제배치/삭제상위를조립한다. 이번분리부품의완료를제품삭제/전체플레이완료로세지않는다. 전체Renderer/실제뷰·Kernel호스트프레임루프등도잔여다.

PORT_MAP860행(L1 418+port442). 기존root행base를417f22d2로재대조하고새header/분리PATCH몸체/같은풀반납3쌍을추가했다. fixture294개유지·전체floor/reflected불변이다. 이번새게임창/금지실제플레이/MP재현없음. 보호원본/CD·AGENTS.md·README개요·cpppj·LEFT_JOBS.md·다른프로젝트변경없음. 목표active·M1전체0/34다.
