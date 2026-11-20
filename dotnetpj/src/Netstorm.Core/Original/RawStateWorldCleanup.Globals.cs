// 현재 전역과 Player/TYPE의 같은 물리를 원문 State 부모에 제공한다.
namespace Netstorm.Core.Original;

/// <summary>실제 하위 호출의 Native 주소 식별자다. 관리 코드에서 함수 포인터로 역참조하지 않는다.</summary>
public enum StateWorldCleanupEffect : uint
{
    CloseAllConnections=0x4e29c0, //reason0의첫전송정리다.
    EndConnections=0x4e2a10, //reason0의두번째전송정리다.
    ResumeGame=0x460e30, //현재pauseDepth가비영일때게임정지한단계를푼다.
    ResumeAudio=0x4aa970, //같은반복의음향정지한단계를푼다.
    Clear446e70=0x446e70, //정지반복뒤원문첫자식을실행한다.
    Clear4c1f20=0x4c1f20, //명령/현재표의기존몸체를재사용한다.
    ResetClock=0x460e70, //원문시계정리몸체다.
    Clear42b1b0=0x42b1b0, //22DWORD직접초기화앞의기존자식이다.
    RefreshSp=0x43dad0, //실제SP작성뒤현재표시소비자다.
    ClearTemplates=0x4bfb00, //Player8순회뒤Template정리다.
    LeaveMetamap=0x4795a0, //그때현재metamap비영에서만선택한다.
    Clear445140=0x445140, //현재Template정리뒤의기존자식이다.
    ClearCursor=0x4d5c60, //선택/커서해제몸체를재사용한다.
    ClearPlayerLinks=0x48fcd0, //Pool보다먼저Player동적필드/기술링크를정리한다.
    ClearKernel=0x471af0, //실제Kernel표의현재슬롯을가상소멸한다.
    KillGumps=0x4666d0, //기존제외타입필터로현재Gump들을Kill한다.
    CleanupGumps=0x4662b0, //현재kill큐의실제정리를수행한다.
    ReplaceUber=0x4c77c0, //기존루트자식이동/새Uber루트교체몸체다.
    Clear43b540=0x43b540, //Core선택해제직전의기존자식이다.
    Reset422490=0x422490, //Pool초기화뒤원문상태자식이다.
    Clear4c2450=0x4c2450, //기존세표정리몸체다.
    MapMode=0x4437f0, //이름은기존API호환용이다.실제본문은C3빈RET이며지도/모드계산이없다.두호출위치를보존한다.
    SceneMusic=0x469fc0, //실제음악부모를그시점에호출한다.
    Clear472830=0x472830, //등록20포인터의 현재 SID목록+0c 개수만0으로 만든다.
    Clear4b2b50=0x4b2b50, //원문의현재State하위몸체다.
    Clear4328a0=0x4328a0, //기존해당정리몸체를필수로호출한다.
    Clear4ab940=0x4ab940, //기존해시/목록정리몸체를재사용한다.
    Reset568b60=0x46edf0, //원문고정this00568b60에실행한다.
    Reset54c8a0=0x432360, //원문고정this0054c8a0에실행한뒤그첫DWORD를1로쓴다.
    ResetGraph=0x462b20, //UI/Kernel/Pool정리뒤실제Graph표를초기화한다.
    Clear4a9e50=0x4a9e50, //실제Player재할당직전의자식이다.
    InitializePlayers=0x4910a0, //Core0선택뒤이전Player해제/새1620BYTE구성을실행한다.
    ResetZone=0x4bfd60, //원문고정this005c86c0의실제Zone몸체다.
    InterpretOptions=0x435220 //후반모드정리/조건부Core선택뒤현재설정을재해석한다.
}

/// <summary>같은 실제 전역을 순수 조회/직접 쓰기하며 임의의 사본이나 상수값을 공급하지 않는 접근자다.</summary>
public sealed class StateWorldCleanupGlobals
{
    /// <summary>각 원문 사용 위치에서 지정 주소의 현재 DWORD를 읽는다.</summary>
    public Func<uint, uint>? Read32 { get; init; }
    /// <summary>같은 실제 owner 필드에 DWORD를 즉시 쓴다.</summary>
    public Action<uint, uint>? Write32 { get; init; }
    /// <summary>지정 전역의 하위 WORD만 쓰며 이웃 WORD를 보존한다.</summary>
    public Action<uint, ushort>? Write16 { get; init; }
    /// <summary>실제 double 필드의 지정64비트를 그대로 쓴다.</summary>
    public Action<uint, ulong>? Write64Bits { get; init; }
    /// <summary>각 Player DWORD 쓰기 직전에 현재 Player base의 같은 물리를 재조회한다.</summary>
    public Func<Memory<byte>>? Players { get; init; }
    /// <summary>선택 슬롯의 실제20바이트 Template 헤더 첫 DWORD를 읽는다.</summary>
    public Func<uint, uint>? TemplateManager { get; init; }
    /// <summary>선택된 현재 TYPE 레코드를 빌려 같은+c4 비용을 직접 쓴다. 레코드 복사본을 반환하면 안 된다.</summary>
    public Func<uint, OriginalTypeRecord>? TypeDefault { get; init; }
}

/// <summary>선택한 실제 자식만 필수로 요구하는 원문 정리 효과 경계다.</summary>
public sealed class StateWorldCleanupHooks
{
    /// <summary>각 주소 식별자의 실제 기존 자식에 분배한다. 미연결 선택은 명시 오류여야 한다.</summary>
    public Action<StateWorldCleanupEffect>? Effect { get; set; }
    /// <summary>현재 연결 번호1..8 또는9 및10/11을 해제한다.</summary>
    public Action<uint>? Disconnect { get; set; }
    /// <summary>중복 모드의 원문 진단 행을 보고하며 복귀하면 부모를 계속 실행한다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>정지 해제 복귀 뒤 매 반복의 현재 깊이를 읽는다.</summary>
    public Func<uint>? PauseDepth { get; set; }
    /// <summary>선택된 실제 Fort Template 슬롯을 저장한다.</summary>
    public Action<uint>? SaveFort { get; set; }
    /// <summary>선택된 실제 전투 Template 슬롯을 저장한다.</summary>
    public Action<uint>? SaveBattle { get; set; }
    /// <summary>지정 Player의 실제 AI를 초기화한다.</summary>
    public Action<uint>? ResetAi { get; set; }
    /// <summary>지정 Player의 같은 사제 owner 상태를 초기화한다.</summary>
    public Action<uint>? ResetPriest { get; set; }
    /// <summary>원문00447810의 owner/amount 인자로 같은 플레이어를 정리한다.</summary>
    public Action<uint, uint>? ResetOtherPlayer { get; set; }
    /// <summary>같은 실제 분산 SP/난수 소비자에 현재 owner와 float32를 전달한다.</summary>
    public Action<uint, float>? SetSp { get; set; }
    /// <summary>같은 실제 두 owner/Core 별칭을 선택하거나 정리한다.</summary>
    public Action<uint, uint>? SelectCore { get; set; }
    /// <summary>지정 크기0으로 실제 Pool 재초기화를 실행한다.</summary>
    public Action<uint>? ResetPool { get; set; }
    /// <summary>마지막 원문 위치에서 현재 벽시각을 읽는다. Frame의 캐시 시각으로 대체하지 않는다.</summary>
    public Func<double>? WallSeconds { get; set; }
}
