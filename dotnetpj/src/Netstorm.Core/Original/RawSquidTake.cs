// 원본 Take 부모의 진단·가상 해제·생성자 반환 객체와 판본별 저장 순서를 연결한다.
namespace Netstorm.Core.Original;

/// <summary>실제 객체/가상 자식은 외부 장치가 해석하는 전체32비트 토큰으로 주고받는다.</summary>
public sealed class RawSquidTakeHooks
{
    /// <summary>선택된 진입 범위 검사에서 현재 numSquids DWORD를 읽는다.</summary>
    public Func<uint>? Capacity { get; set; }
    /// <summary>현재 풀 기준에서 입력 SID의 실제 객체 토큰을 얻는다.</summary>
    public Func<uint, uint>? Slot { get; set; }
    /// <summary>해당 객체의 현재 상태 BYTE를 읽는다.</summary>
    public Func<uint, byte>? State { get; set; }
    /// <summary>해당 객체의 현재 TYPE BYTE를 읽는다.</summary>
    public Func<uint, byte>? Type { get; set; }
    /// <summary>가상 진입 직전의 현재 VT DWORD다.</summary>
    public Func<uint, uint>? Vtable { get; set; }
    /// <summary>서버/클라이언트 TYPE 이름의 상세를 복사한다.</summary>
    public Action<uint, uint>? Detail { get; set; }
    /// <summary>원문 식·파일·줄의 진단이 정상 복귀하면 같은 현재 상태로 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>선택된 객체/VT의 실제 가상+48을 실행한다.</summary>
    public Action<uint, uint>? Unpop { get; set; }
    /// <summary>상태 BYTE 하나만 쓴다.</summary>
    public Action<uint, byte>? StoreState { get; set; }
    /// <summary>TYPE BYTE 하나만 쓴다.</summary>
    public Action<uint, byte>? StoreType { get; set; }
    /// <summary>해제와 상태 변경 뒤 현재 TYPE의 생성자 주소를 읽는다.</summary>
    public Func<uint, uint>? Constructor { get; set; }
    /// <summary>cdecl 생성자의 반환 객체 토큰을 보존한다. 주소0은 실제 기본 생성자다.</summary>
    public Func<uint, uint, uint>? Construct { get; set; }
    /// <summary>저장한 반환 객체의 실제 VT+2c를 실행한다.</summary>
    public Action<uint, uint>? PostTake { get; set; }
}

/// <summary>004af610/CD004ab440의 원문 부모만 처리하고 선택하지 않은 자식을 요구하지 않는다.</summary>
public static class RawSquidTake
{
    /// <summary>현재 선택된 함수 객체를 호출 끝까지 보유하고 미연결이면 해당 위치에서 실패한다.</summary>
    private static T Need<T>(T? body) where T : class => body ?? throw new InvalidOperationException("Take의 선택된 실제 장치 미연결");

    /// <summary>할당/자유 목록을 바꾸지 않고 반환 객체와 전체 TYPE 인자·판본별 저장 순서를 보존한다.</summary>
    public static uint Take(OriginalEdition edition, uint type, uint sid, RawSquidTakeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        bool patch = edition == OriginalEdition.Patch1078;
        // 선택한 진단만 요구하며 각 진단 뒤 현재 장치를 다시 선택한다.
        void Diagnose(string expression, uint line) => Need(hooks.Assertion)(expression, patch ? @"\Ns\O\Squid.cpp" : @"\ns\o\Squid.cpp", line);
        int number = unchecked((int)sid);
        if (number < (patch ? 15000 : 6000) || number >= unchecked((int)Need(hooks.Capacity)()))
            Diagnose("(FIRST_SERVER_SID <= sid && sid < numSquids)", patch ? 0x1a7u : 0x19eu);
        uint original = Need(hooks.Slot)(sid);
        if ((Need(hooks.State)(original) & 1) == 0)
        {
            byte previous = Need(hooks.Type)(original);
            if (previous != type) { Need(hooks.Detail)(type, previous); Diagnose("0", patch ? 0x1bau : 0x1b1u); }
            uint table = Need(hooks.Vtable)(original); Need(hooks.Unpop)(original, table);
        }
        byte state = Need(hooks.State)(original);
        if (!patch) Need(hooks.StoreState)(original, (byte)(state & 0xfe));
        Need(hooks.StoreState)(original, (byte)(state & 0xfc));
        uint address = Need(hooks.Constructor)(type); uint returned = Need(hooks.Construct)(address, sid);
        if (patch)
        {
            byte current = Need(hooks.State)(returned); uint table = Need(hooks.Vtable)(returned);
            Need(hooks.StoreType)(returned, unchecked((byte)type)); Need(hooks.StoreState)(returned, (byte)(current | 4)); Need(hooks.PostTake)(returned, table);
        }
        else
        {
            uint table = Need(hooks.Vtable)(returned); byte current = Need(hooks.State)(returned);
            Need(hooks.StoreState)(returned, (byte)(current | 4)); Need(hooks.StoreType)(returned, unchecked((byte)type)); Need(hooks.PostTake)(returned, table);
        }
        return returned;
    }
}
