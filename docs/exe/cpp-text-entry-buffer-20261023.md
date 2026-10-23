# TextEntry의 실제 문자 편집 버퍼

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이다. 보호 PE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 기존 정밀 C와 PE만 재사용했으며 이 작업에서 게임/GUI·새 디컴파일·CMake 변경·빌드·커밋은 하지 않았다.

TextEntry `00452430`의 `+108`은 **0x28BYTE 원시 문자 편집 버퍼**다. 기존 StyleTextBuffer의 표시 조각·개행 체인과 다른 소유 자료이므로 서로 대체하지 않는다. 새 [TextEntryBuffer.h](../../cpppj/src/client/TextEntryBuffer.h)와 [구현](../../cpppj/src/client/TextEntryBuffer.cpp)은 아래 필드와 13개 진입을 복원한다.

| offset | 현재 필드 |
|---|---|
| `+0/+4` | allocator VT / DWORD 줄offset 배열 주소 |
| `+8/+c/+10` | offset 배열 칸 수 / 현재 줄 수 / dirty DWORD |
| `+14/+18` | 정렬된 signed 선택 시작/끝 |
| `+1c/+20/+24` | 본문 길이 / capacity / 현재 NUL 문자열 주소 |

`00451a80`은 요청0이면 본문2048BYTE를 할당하고, 성공하면 요청값과 무관하게 capacity=2048을 기록한다. 이어 줄offset512BYTE를 별도로 할당하고 offsetCapacity=128/dirty=1을 쓴다. 생성자는 본문 첫 NUL을 쓰지 않는다. 실제 TextEntry 생성자가 곧 호출하는 `00451930` Reset이 첫 NUL을 쓴 뒤 초기 문자열을 삽입한다. 근거: [원문 65847행](../../extracted/refined/originals/Netstorm.c:65847), [66350행](../../extracted/refined/originals/Netstorm.c:66350).

`00451710` Insert는 NUL 앞 길이와 원래 선택 폭을 먼저 고정하고, 필요한 경우1024단위의 새 capacity로 가상 realloc을 호출한다. 실패 시 뒤의 이동/본문/선택/dirty 쓰기를 생략한다. 성공 뒤 data/capacity를 설치하고 현재 선택 끝/현재 length를 읽어 꼬리를 memmove한다. 꼬리 복사는 NUL을 포함하지 않으며, 마지막에 현재 length 위치에 NUL을 쓴다. 원래 선택 폭이 삽입 길이와 같으면 꼬리 이동과 length 갱신만 생략한다. 근거: [65578행](../../extracted/refined/originals/Netstorm.c:65578), PE `00451710~004517f1`.

원본은 `length == capacity`에도 `data+capacity`에 마지막 NUL을 쓸 수 있다. 제품의 실제 소유 장치는 **본문 nominal 요청 뒤 최소1BYTE를 안전하게 소유**하고, 원문 capacity 및 realloc 요청값은 그대로 유지해야 한다. 독립 검사는 nominal+16BYTE 가드를 소유해 그 실제 쓰기와 NUL 바깥 바이트까지 관측한다. bounds 예외로 정상 exact-capacity 입력을 실패시키거나 capacity 자체를+1로 고치는 방식으로 분기 의미를 바꾸지 않는다.

`00451550`은 dirty0이면 아무 줄표도 작성하지 않고1을 반환한다. 그 밖에는 첫offset0/줄 수1을 먼저 쓰고, NUL 앞의 각 LF 다음 위치를 기록한다. 줄 수가 현재 칸 수에 도달하면 새 `currentCapacity*4+512`BYTE를 할당하고, **현재 표와 현재 capacity**를 읽어 복사한다. 현재 표 free 뒤 고정된 새 포인터를 설치하고, free 콜백 뒤 현재 capacity에128을 더한다. 마지막offset은 현재 length다. 할당실패 시 앞서 쓴 offset/lineCount/dirty를 되돌리지 않는다. 근거: [65431행](../../extracted/refined/originals/Netstorm.c:65431), PE `00451590~00451633`. 정밀 C의 `offsets=this` 표시는 잘못된 추정이며 실제 명령은 스택에 보관한 새 할당주소를 쓴다.

줄 길이는 다음offset-현재offset이므로 LF가 포함된다. LF로 끝나는 문자열에는 길이0인 마지막 줄도 있다. `00451990` CopyLine은 유효성/출력 용량을 통과해야 본문과 NUL/선택쌍을 쓰며, 실패는 ffffffff를 반환하고 출력을 보존한다. 선택이 해당 줄과 교차하지 않으면 {-1,-1}이다. `00451950`/`00451960` 부모는 EnsureLines의 반환값과 별개로 현재 count/길이를 조회하는 원문의 순서를 유지한다.

Copy `004518c0`은 빈 선택이면 전체본문을 읽되 저장한 선택을 바꾸지 않는다. Cut `00451b10`은 빈 선택을 실제 전체선택으로 바꾼 뒤 clipboard복사→null삽입→dirty1을 실행한다. Paste `00451800`은 실제 clipboard256BYTE의 제외문자를 지운 뒤 비어 있지 않으면 Insert하고, 빈 clipboard도 dirty1을 남긴다. Destroy `00451680`은 data가0이면 줄표도 반납하지 않는다. data가 있으면 본문 free→data/capacity0→현재 줄표 free→offsets0 순서다. 이 조건을 일반적인 전체 정리로 바꾸지 않았다.

원시 메모리 장치의 `bytes(address)`는 순수 조회이며 base와 base+offset에 같은 실제 소유 저장소를 반환한다. allocate/free/resize/clipboard는 현재 상태를 바꿀 수 있다. 입력 문자열 span은 NUL을 포함해야 하며, 빈 span은 원문의 null 입력이다. realloc으로 무효가 되는 자기 본문 입력은 원본에서도 위험하므로 입력 수명 계약에서 구별한다. `State()`는 같은 상태 참조, `Data()`는 현재 메모리 범위, `Text()`는 원문 length 앞 읽기 전용 뷰다. 재할당 뒤 이전 뷰를 재사용하지 않는다.

[독립 도구](../../tools/decomp_text_entry_buffer.py)는 생성/소멸/선택/삽입/Reset/EnsureLines/LineCount/LineLength/CopyLine/CopyAll/Copy/Cut/Paste의 **실제 원문 기계어**를 실행한다. malloc/free/realloc·표준 strchr/memmove·clipboard만 명시 외부 경계다. 표준 메모리 함수는 순수 C 계약이며 관측 가능한 장치 콜백/trace를 만들지 않는다.

[fixture](../../cpppj/tests/fixtures/text-entry-buffer-x86.tsv)는 **285입력×두x87제어어=570행**, 정상560/명시중단10, 정밀도간 현재 관측 차이0, 실제178,982명령이다. signed선택 경계, NUL 앞/0x80·0xff, 같은/짧은/긴 선택 교체,1024성장과 실패,128칸 줄표 성장과 실패, 끝 빈 줄, dirty0, data0 소멸, 출력 용량과 보존, clipboard필터, callback 현재필드 재조회와 부분쓰기를 포함한다. 앞11열만 입력이며 뒤11열은 관측이다. 모든10DWORD·반납 메모리/가드·출력/선택·clipboard·trace를 대조한다. void/미완료 EAX는 의미 반환값으로 대조하지 않는다.

[새 검사](../../cpppj/tests/TextEntryBufferTests.cpp)는 `NETSTORM_TEXT_ENTRY_BUFFER_FIXTURE` 매크로를 사용한다. 이 준비 작업에서는 CMake 등록/빌드/검사 실행을 하지 않았다. 원문/도구/fixture SHA와 호출/쓰기 계수는 `extracted/cpp-text-entry-buffer-20261023-native.json`에 기록했다. 실제 Windows clipboard, TextEntry/Console 전체 생성·Draw·키보드 분배 및 ChatWindow GUI 완료를 이 관측으로 주장하지 않는다.

후속 TextEntry/Console은 공통 `GumpBackgroundCreate`, Slider, 글꼴 측정·클립·Draw, 기존 `ClientMain::CharacterEvent` 큐와 같은 `GumpInput`에 이 버퍼를 연결해야 한다. 현재 `platform/Console`은 표준출력의 바이너리 모드 함수여서 게임 Console 구현이 아니다. 일반 월드 분배에서 키보드 사건이 `UserInput`으로 먼저 넘어가는 부분도 실제 입력창 설치와 함께 원본 Gump 키보드 경계로 연결해야 한다.
