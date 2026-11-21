using System.Globalization;
using System.Text;

// Chat manager의 sparse 생성/현재 설정/랭크 접두를 실제 바이트 저장소에 적용한다.
namespace Netstorm.Core.Original;

/// <summary>Client가 소유하는 현재 manager·subscriber·Core 활성 DWORD를 복사 없이 빌린다.</summary>
public sealed class ChatManagerPrefixGlobals
{
    /// <summary>00548714의 같은 현재 manager 토큰이다.</summary>
    public UIntBox Current { get; }
    /// <summary>00540c74의 같은 현재 구독자다. 키를 만들 때마다 다시 읽는다.</summary>
    public UIntBox Subscriber { get; }
    /// <summary>00595348의 같은 Core 활성 DWORD다.</summary>
    public UIntBox CoreDataActive { get; }
    /// <summary>세 실제 주소만 연결하며 값에 쓰거나 초기화하지 않는다.</summary>
    public ChatManagerPrefixGlobals(UIntBox current, UIntBox subscriber, UIntBox coreDataActive)
    {
        Current = current ?? throw new ArgumentNullException(nameof(current));
        Subscriber = subscriber ?? throw new ArgumentNullException(nameof(subscriber));
        CoreDataActive = coreDataActive ?? throw new ArgumentNullException(nameof(coreDataActive));
    }
}

/// <summary>같은 실제256BYTE 지역에 고정 icon 서식을 제자리로 붙이는 장치다.</summary>
public delegate void ChatRankIconAppender(Span<byte> destination, char kind, int value);

/// <summary>호출 위치에서만 선택하는 실제 할당/물리/설정/그림 장치다.</summary>
public sealed class ChatManagerPrefixHooks
{
    /// <summary>실제2712/128BYTE를 할당하고0 또는 비영 토큰을 반환한다.</summary>
    public Func<uint, uint>? Allocate { get; set; }
    /// <summary>토큰의 같은 가변 물리를 빌린다. 조회 자체는 전역을 바꾸지 않는다.</summary>
    public Func<uint, Memory<byte>>? Memory { get; set; }
    /// <summary>220/898/2474 진단 뒤 복귀하면 원문을 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>이름과 실제 callback 식별자/표시 flag를 같은 명령표에 등록한다.</summary>
    public Action<string, uint, uint>? RegisterCommand { get; set; }
    /// <summary>현재 설정의 독립 지역 문자열을 반환하며 누락은 null이다.</summary>
    public Func<string, string?>? ReadConfig { get; set; }
    /// <summary>CRT 진입을 보존하는 대상 토큰/offset/문자열/폭 장치다.</summary>
    public Action<uint, uint, string, uint>? CopyFormat { get; set; }
    /// <summary>현재 icon.C/B에 따른 최대 랭크를 호출마다 조회한다.</summary>
    public Func<int>? MaximumRank { get; set; }
    /// <summary>현재 icon.B 개수를 조회하며 진단 전 divisor로 캡처한다.</summary>
    public Func<int>? MinorCount { get; set; }
    /// <summary>기본값은 실제 고정 icon 서식이며 명시 callback으로 대체할 수 있다.</summary>
    public ChatRankIconAppender? AppendIcon { get; set; } = RawChatManagerPrefix.AppendChatRankIcon;
    /// <summary>실제 현재 CoreData 물리를 빌린다. unsigned+4만 읽는다.</summary>
    public Func<ReadOnlyMemory<byte>>? Core { get; set; }
    /// <summary>대리자 정체성을 유지한 채 장치 묶음만 복제한다.</summary>
    public ChatManagerPrefixHooks Copy() => (ChatManagerPrefixHooks)MemberwiseClone();
}

/// <summary>RawChatManagerPrefix.cpp의0042e000/0042e050/0042d620/0042f480/0042f540 본체다.</summary>
public static class RawChatManagerPrefix
{
    /// <summary>실제 manager 할당 폭0xa98이다.</summary>
    public const uint ManagerBytes = 0xa98;
    /// <summary>초기화하지 않는 별도 내부 할당 폭0x80이다.</summary>
    public const uint BufferBytes = 0x80;
    /// <summary>manager 랭크 문자열 위치이며 ChatWindow 입력 접두와 다르다.</summary>
    public const uint RankPrefixOffset = 0x244;
    /// <summary>문자열 NUL 뒤까지 복사하는 실제 지역 출력 폭이다.</summary>
    public const int RankOutputBytes = 256;
    /// <summary>선택된 실제 효과만 요구하며 앞선 부분 쓰기는 복구하지 않는다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Chatmanager 실제 장치 누락");
    /// <summary>첫NUL 뒤를 서식이나 복사로 해석하지 않는다.</summary>
    private static string CString(string source) { int end = source.IndexOf('\0'); return end < 0 ? source : source[..end]; }
    /// <summary>현재 전체 DWORD의 signed%d 키를 invariant 십진수로 만든다.</summary>
    private static string Key(uint subscriber, string suffix) => unchecked((int)subscriber).ToString(CultureInfo.InvariantCulture) + suffix;
    /// <summary>현재 토큰의 실제 할당을 매 접근 위치에서 다시 빌린다.</summary>
    private static Span<byte> Memory(ChatManagerPrefixHooks hooks, uint token) => Need(hooks.Memory)(token).Span;
    /// <summary>한 store의 전체 폭을 먼저 확인하고 인접 미작성 바이트를 보존한다.</summary>
    private static void Write(Span<byte> memory, uint offset, uint value, int width = 4)
    {
        if ((ulong)offset + (uint)width > (ulong)memory.Length) throw new ArgumentOutOfRangeException(nameof(offset), "Chatmanager store 물리 범위");
        // x86 little endian의 지정 폭만 낮은 바이트부터 쓴다.
        for (int index = 0; index < width; index++) memory[(int)offset + index] = unchecked((byte)(value >> (index * 8)));
    }
    /// <summary>strncpy60/500의 NUL padding과 명시 BYTE 문자열을 그대로 복사한다.</summary>
    public static void CopyChatManagerFormat(Span<byte> destination, string source, uint count)
    {
        if (count > destination.Length) throw new ArgumentOutOfRangeException(nameof(count), "Chatmanager strncpy 물리 범위");
        source = CString(source);
        // 원문 count 전체를 쓰며 source 첫NUL 뒤는0으로 채운다.
        for (int index = 0; index < (int)count; index++) destination[index] = index < source.Length ? unchecked((byte)source[index]) : (byte)0;
    }
    /// <summary>미공급 copyFormat만 같은 memory와 실제 CRT 복사에 연결하며 기존 장치는 유지한다.</summary>
    public static ChatManagerPrefixHooks BindChatManagerMemoryCopy(ChatManagerPrefixHooks hooks)
    {
        var result = hooks.Copy(); var memory = hooks.Memory;
        result.CopyFormat ??= (token, offset, source, count) =>
        {
            Memory<byte> bytes = Need(memory)(token);
            if (offset > bytes.Length) throw new ArgumentOutOfRangeException(nameof(offset), "Chatmanager CRT target 물리 범위");
            CopyChatManagerFormat(bytes.Span[(int)offset..], source, count);
        };
        return result;
    }
    /// <summary>현재 C문자열 끝에 signed icon 서식을 NUL까지 쓰며 나머지 지역 바이트를 유지한다.</summary>
    public static void AppendChatRankIcon(Span<byte> destination, char kind, int value)
    {
        int offset = destination.IndexOf((byte)0);
        if (offset < 0) throw new ArgumentOutOfRangeException(nameof(destination), "Chat 랭크 append의 NUL 없음");
        byte[] text = Encoding.Latin1.GetBytes("~[Iicon." + kind + value.ToString(CultureInfo.InvariantCulture) + ']');
        if (text.Length + 1 > destination.Length - offset) throw new ArgumentOutOfRangeException(nameof(destination), "Chat 랭크 append 물리 범위");
        // 기존 문자열 뒤에만 실제 서식 바이트를 순서대로 붙인다.
        for (int index = 0; index < text.Length; index++) destination[offset + index] = text[index];
        destination[offset + text.Length] = 0;
    }
    /// <summary>내부 할당→this+8→조건부 진단→지정 필드만 쓰는 실제 생성자다.</summary>
    public static uint ConstructChatManager(uint token, ChatManagerPrefixHooks hooks)
    {
        uint buffer = Need(hooks.Allocate)(BufferBytes); Write(Memory(hooks, token), 8, buffer);
        if (buffer == 0) Need(hooks.Assertion)(0xdc);
        Write(Memory(hooks, token), 0, 0); Write(Memory(hooks, token), 0xc, 0);
        Write(Memory(hooks, token), 0x14, 0, 1); Write(Memory(hooks, token), 0x50, 0, 1); Write(Memory(hooks, token), RankPrefixOffset, 0, 1); Write(Memory(hooks, token), 4, 0x20);
        return token;
    }
    /// <summary>할당/현재대입/명령3등록/두 현재 키/각 현재 manager의 CRT 복사를 원문 순서로 수행한다.</summary>
    public static void InitializeChatManager(ChatManagerPrefixGlobals globals, ChatManagerPrefixHooks hooks)
    {
        uint token = Need(hooks.Allocate)(ManagerBytes);
        globals.Current.Value = token != 0 ? ConstructChatManager(token, hooks) : 0;
        Need(hooks.RegisterCommand)("format", 0x42ddf0, 1); Need(hooks.RegisterCommand)("back", 0x42ddf0, 1); Need(hooks.RegisterCommand)("front", 0x42def0, 1);
        string? front = Need(hooks.ReadConfig)(Key(globals.Subscriber.Value, "chatFormat"));
        string? back = Need(hooks.ReadConfig)(Key(globals.Subscriber.Value, "chatFormat2"));
        if (globals.Current.Value == 0) Need(hooks.Assertion)(0x382);
        uint first = globals.Current.Value;
        Need(hooks.CopyFormat)(first, 0x14, front is null ? "" : CString(front), 60); Write(Memory(hooks, first), 0x4f, 0, 1);
        uint second = globals.Current.Value;
        Need(hooks.CopyFormat)(second, 0x50, back is null ? "" : CString(back), 500); Write(Memory(hooks, second), 0x243, 0, 1);
    }
    /// <summary>NUL 포함 순방향 복사로 겹침 부작용과 물리 실패 전의 부분 쓰기를 보존한다.</summary>
    public static void SetChatManagerRankPrefix(Span<byte> manager, ReadOnlySpan<byte> source)
    {
        // 읽은 한BYTE를 즉시 쓰며 같은 배열의 다음 source가 바뀌는 경우도 유지한다.
        for (int index = 0; ; index++)
        {
            if (index >= source.Length) throw new ArgumentOutOfRangeException(nameof(source), "Chat 랭크 prefix source 물리 범위");
            byte value = source[index]; Write(manager, RankPrefixOffset + (uint)index, value, 1); if (value == 0) return;
        }
    }
    /// <summary>현재 최대값 재조회와 캡처 divisor·미작성 임시 바이트·순방향 DWORD 복사를 보존한다.</summary>
    public static ReadOnlySpan<byte> ChatPrefixIcons(Span<byte> output, Span<byte> temporary, int rank, ChatManagerPrefixHooks hooks)
    {
        if (temporary.Length < RankOutputBytes) throw new ArgumentOutOfRangeException(nameof(temporary), "Chat 랭크 temporary256 물리 범위");
        temporary[0] = 0;
        if (rank >= 0)
        {
            int first = Need(hooks.MaximumRank)(); int compared = first < rank ? Need(hooks.MaximumRank)() : rank;
            if (compared < 0) rank = 0; else if (Need(hooks.MaximumRank)() < rank) rank = Need(hooks.MaximumRank)();
            int minor = Need(hooks.MinorCount)(); if (minor <= 0) Need(hooks.Assertion)(0x9aa);
            if (minor == 0 || (rank == int.MinValue && minor == -1)) throw new DivideByZeroException("Chat 랭크 원문 idiv fault");
            int quotient = rank / minor, remainder = rank % minor;
            if (quotient > 0) Need(hooks.AppendIcon)(temporary[..RankOutputBytes], 'C', quotient - 1);
            Need(hooks.AppendIcon)(temporary[..RankOutputBytes], 'B', remainder);
        }
        // rep movsd처럼 겹침이 있어도 매 DWORD의 네BYTE를 먼저 캡처하고 출력한다.
        for (int offset = 0; offset < RankOutputBytes; offset += 4)
        {
            if (offset + 4 > output.Length) throw new ArgumentOutOfRangeException(nameof(output), "Chat 랭크 output256 물리 범위");
            byte a = temporary[offset], b = temporary[offset + 1], c = temporary[offset + 2], d = temporary[offset + 3];
            output[offset] = a; output[offset + 1] = b; output[offset + 2] = c; output[offset + 3] = d;
        }
        return output[..RankOutputBytes];
    }
    /// <summary>manager/active 자연 관문 뒤 현재 Core+4와 마지막 현재 manager를 읽는다.</summary>
    public static void RefreshChatManagerRankPrefix(ChatManagerPrefixGlobals globals, ChatManagerPrefixHooks hooks, Span<byte> output, Span<byte> temporary)
    {
        if (globals.Current.Value == 0 || globals.CoreDataActive.Value == 0) return;
        ReadOnlyMemory<byte> core = Need(hooks.Core)();
        if (core.Length < 5) throw new ArgumentOutOfRangeException(nameof(core), "Chat 랭크 현재 CoreData+4 물리 범위");
        ReadOnlySpan<byte> text = ChatPrefixIcons(output, temporary, core.Span[4], hooks);
        SetChatManagerRankPrefix(Memory(hooks, globals.Current.Value), text);
    }
}
