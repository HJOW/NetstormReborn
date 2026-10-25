# 카메라·보조 전역·프로세스 수명의 같은 실제 저장소

## 범위와 근거

439·440·441·442를 한 기능 묶음으로 병합한다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 수행일은 **2026-10-23**이다. 기준은 원본10.78이다. 이번 준비 과정에서는 원본 게임이나 새 디컴파일을 실행하지 않았다. 기존 정밀C, 보호PE 정적 바이트, 독립 fixture와 각 준비본의 읽기 검토를 사용했다.

이전 완료 기준은815fbd67의 Source2 제품a49f474f…이며 집중340/전체CTest2963·실패0, 메뉴GUI와 자연Early2→Tutorial3를 통과했다. 그 수치를 이번 변경 제품의 실행 결과로 옮겨 세지 않는다.

## 카메라와 상태 보조 전역

카메라의 실제64BYTE는 world float비트, 정수 픽셀·배율, float 배율·픽셀, zoom을 구별한다. Client 수명의 논리Renderer view, snapshot, 목표 픽셀/한계/zoom은 서로 다른 저장소다. 명시한 원문 복사 위치만 snapshot을 바꾼다. Renderer 장치 교체나 Session 생성/반납이 Native 초기화 전체를 반복하지 않는다.

Session의 커서 카메라와 화면 원점은 같은 snapshot의 현재 float비트와 픽셀을 직접 빌린다. 정수 픽셀에서 분수 world좌표를 다시 유도해 덮어쓰는 사본을 제거한다. 배치와 비용 표시의 dirty 효과 뒤에도 현재 화면 원점을 다시 읽는다. Native 초기화의 한계X→목표zoom→한계Y 순서도 보존한다.

독립 에뮬레이터는 실제 PE의16입력을 두 x87 정밀도에서 관측했고 정상 fixture는16개다. 별도 극단 정밀도2관측에서는 53bit/64bit 중간 정밀도 차이를 확인했다. 그 경계를 기록하며 전체 Native Frame/zoom 부모까지 복원했다고 주장하지 않는다. 원본exe나 GUI를 실행한 관측은 아니다.

State 보조 전역은 ScriptBreak와 별도DWORD들, 네 캐시DWORD, ReconnectSaveEnd double의 같은 Client 저장소를 제공한다. ScriptBreak의 최초값1과 캐시의ffffffff, 별개 기한들의BSS0을 구분한다. 005c85cc는 인접 battleByte가 아닌 정확한4BYTE다. 로그인과 타이머는 같은 ScriptBreak 참조를 사용하며 기존 실행 순서와 다른 기한을 유지한다. Native의 나머지 직접 부모 접합은 후속이다.

## 게임 보조17필드

RawGameAuxState를 Gameplay와 RawSurfaceStorage가 함께 소유한다. 서로 다른 owner를 명시한 입력은 대여나 fresh 초기화 전에 거부한다. Storage만 지정한 독립 호출자는 같은 Authority와 GameAux를 채택한다.

00595088은 signed ResourcePool 물리 하나다. Reward와 LocalGeyser는 직접 참조하고 LoginBegin/Bomb의 unsigned 소비는 bit_cast로 같은 비트를 읽고 쓴다. 다른 정수 타입의 참조를 강제로 alias하거나 별도값을 동기화하지 않는다. 환불률, 허용DWORD, 비용, Archipelago, 손실 좌표, 기본SP, GeyserTYPE도 실제 소비자가 같은 현재값을 읽는다.

일반 fresh는 기존 지역 Runtime의9필드 초기화 정책만 유지하고 나머지8필드는 보존한다. prepared 생성/반납/실패는17필드 모두의 현재값을 유지한다. 삭제의 손실 좌표 저장은X→Y이며 cleanup의-10000 저장은Y→X다. 네 double 기한과 미접합 Native writer도 별도 필드로 유지하며 상수0으로 대신하지 않는다.

## 프로세스 Host와 현재 Runtime 대여

같은 Storage가 ProcessState와 안정된 SquidProcessHost를 소유한다. 실제 Client의 Kernel과 FrameTime은 기존과 같은 shared 객체로 pin하고, 바인딩된 시각은 Now()에서 현재값을 읽는다. Prepare/Sync가 예전 시각 사본을 실제 Client에 역으로 쓰지 않는다. 같은 GameAux의 GeyserTYPE도 alias 공유 포인터로 유지한다.

Host의 실제 가상 처리기는 짧은 현재 Runtime 대여다. Notify, Kernel.Clear, 자기 대여 종료 정리의 호출이 진행되는 동안 shared 대여와 호출 수를 유지하여, 재진입한 반납이 진행 중 효과를 무효화하지 못하게 한다. Native Clear는 한 번의 실제 Kernel.Clear 정상 복귀에서만 이동전용 증빙을 발급한다. 이전 슬롯에 새로 등록되어 살아남은 프로세스를 반복Drain으로 없애지 않는다.

반납 증빙은 같은 Host/Kernel/Runtime 정체성과 진행 중 호출, 잔류 종류를 확인한 뒤 소비한다. 반납 뒤 holder 할당이 실패해 일반Drain으로 돌아가는 경계는 optional/noexcept 보관으로 제거했다. 새 Session의 생성 실패·후반Load 실패·일반 종료는 자기 binding이 부착한 프로세스만 반납한다. 이전 Native 잔류의 같은PID는 유지하고 다음 현재 handler/TYPE/Pool을 읽는다. Storage 최종 소멸은 실제 Host/Pool/TYPE/권한/시각이 살아 있는 동안 소유한 잔류를 반납한다.

raw 외부Kernel만 주는 기존 독립 호출은 legacy Host, fallback 시각 쓰기와 기존 최종 종료 정리를 유지한다. 명시 pin 경로와 혼동해 기존 Native/legacy fixture 숫자를 바꾸지 않는다. free/dead Regular의 무동작은 현재 Runtime을 요구하는 관문보다 앞에 있다. Native의 선행 head진단은 별도 미복원 경계다.

추가 PE 정적 확인에서 Regular/SharedRegular의 scalar소멸자는468230→4ab2c0 thunk→41bd00이며 기본vtable/두poison필드 쓰기와 조건부free만 수행한다. 이 소멸자에는 Pool/Form/Runtime callback이 없다. 원시 바이트와 보호PE SHA는 `extracted/cpp-regular-destructor-static-20261025.json`에 보존했다. 최초 opcode 기대의 즉시store 가정은 실제EAX store형으로 교정했고 이전 정적assert도 보존했다. 다른 파생 소멸자에는 이 결론을 확대하지 않는다.

## 병합과 검증

초기93파일(기존82+신규11)의 원시6충돌/6파일을 보존하고 같은Camera/GameAux/Authority/Kernel 연결과 공통helper 각1회, CMake 등록을 감사했다. 적용 전 신규카메라검사의 Load 누락은 제품관문과 기존9조건·좌표기대를 유지한 채 실제Load/NativeHome Reset과 setup3조건으로 교정했다.

Source1의 첫 빌드는285.553초 뒤 종료1·경고0/오류1이었다. 제거한cursorView를 기존CarrierDrop inc가 읽는 한곳이 최초93파일 검토에서 빠졌다. 같은현재snapshot의 `ProjectCameraView`로497220을 연결해 Source2는94파일이 됐다. peer는 수명·반환형과 SP차감→현재부모좌표→투영→팝업 순서를 확인했다. 전체KernelDrain이나 사본동기화를 추가하지 않았다.

build2/3에서는 신규검사의 완전한커서선언, 명시SID, 가변TYPE API, 정확한BYTE입력을 교정했다. 제품이나 Nativefixture를 검사에 맞춰 바꾸지 않았다. build4는종료0·컴파일경고/오류0이었다. 집중1은368개에서18실패check를 남겼으며 모두 원시로그에 보존했다.

실패 검토에서 신규HostSession의 같은Subscriber와 StateAux의 같은GameAux 공급 누락을 고쳤다. 기존커서/미션Start 검사는 지역fallback 대신 같은현재getter로 기존숫자를 읽도록 이행했다. 소멸관측기는 실제현재Host binding에 부착하여 Player/9Deck/9factory/Capacity30/다음재할당 조건을 모두 유지했다. prepared TotalCost는 실제0059508c의장기DWORD이므로 기존입력91을 보존하도록 이전지역0기대를 명시 이행하고 주소동일성도 확인했다. 이는 원문 장기물리 복원에 따른 경계 변경이며 다른지역 제어상태는 계속0이다.

Source5의95소스는 build5 종료0·컴파일경고/오류0, 집중2의368고유·실패0이었다. 첫통합batch1의전체CTest3007개에서기존실제두Session검사1개가같은GameAux를공급하지않아소유자검증에서실패했다. 해당구성에같은GameAux를추가했고제품/기대값을바꾸지않았다. 최초전체실패로그·보고서원시사본을보존하고해당2사례를집중범위에추가했다.

최종 **Source6의95소스**는 `extracted/cpp-camera-process-439442-source6/hashes.json`에동결했다. SHA는 `4c50721108ec16b6eb7281aa886c4c9cede72e19325037c28059a76b99820b01`다. **build6 종료0·컴파일경고/오류0(55.293초), 집중3의370고유·실패0**이며신규44개이름을모두확인했다. 보호1821/95소스/4실행파일SHA가같다. 게임SHA는 `712fd7aead4838bccfaf22bfa6c334e8d316d57bd3156c4f27e0372e01f47616`으로제품코드가같음을확인했다.

같은제품의 메뉴GUI2실행과 자연Early2→Tutorial3가 통과했다. 실제타이머1/상태주입false/초기진행주입false·기존done1→1로 환불75·종료SP3075·다음초기SP2000·같은프레임안내를 확인했고 실제3캡처도 육안확인했다. 캡처SP1700은 설치뒤실제생산이 진행된 시점이며 초기SP2000과 구분한다. 첫플레이캡처에는Bridges안내가 열려 있다. 평상시Host Game버튼은 없다.

최종Source6의 **통합batch2 종료0·전체CTest3007개/실패0**을 확인했다. 같은95소스/4실행파일·보호1821·9보고서원시사본/driver/helper/PLAN·원본전체집합/두설정복원 감사를 모두통과했다. 전체로그의확인된진단출력만메모리분리하고원시SHA를보존했으며, 실제검사명집합이현재3007/이전HEAD2963의등록이름과각각완전히같았다. 기존제거0·추가44개가이번신규44개와정확히일치한다. [최종실행보고서](../../cpppj/camera-process-runtime-20261025.json)에실패batch1과최종통과batch2·교정이력·세캡처를구분해기록했다.

사용자 요청에 따라 본작업과 이미 진행 중이던443/444 준비본의 문서·로컬커밋을 마치면 목표를 일시정지한다. [준비본Git보관안내](prepared/README.md)의443/444는 제품미적용·C++미빌드/검사이며 현재제품 통과수에 포함하지 않는다. 각각 보관자료만으로 결과를 재현하는 정적검사는 완료했다.

NativeState1 전체cleanup/prefix·AI/Links/UI/Player고정뷰·GameplayUse 반납coordinator, 다른파생프로세스 수명, 카메라전체Frame/zoom부모는 미완료다. 실제Edit 편집/저장·전체Early/캠페인/데모 자연완주·진행저장/재실행·원본완전동일성도 계속 미완료이며 목표를완료로표시하지 않는다.
