using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;

// 객체 메뉴 제목과 정보는 현재 TYPE·Player·객체 상태를 원본 순서로 읽는다.
namespace Netstorm.Client.Gump;

/// <summary>객체 정보가 읽는 현재 역할과 모드다.</summary>
public sealed class ObjectMenuInfoState
{
    /// <summary>수량을 하나 줄여 표시하는 현재 제단 TYPE이다.</summary>
    public uint Altar { get; set; } = 164;
    /// <summary>소유자를 0으로 표시하는 현재 너겟 TYPE이다.</summary>
    public uint Nugget { get; set; } = 154;
    /// <summary>투명 상태의 현재 경고 그림 TYPE이다.</summary>
    public uint Invisible { get; set; } = 117;
    /// <summary>마비 상태의 현재 경고 그림 TYPE이다.</summary>
    public uint Paralyzed { get; set; } = 118;
    /// <summary>현재 로컬 플레이어 조회다.</summary>
    public Func<uint>? LocalOwner { get; set; }
    /// <summary>현재 편집 모드 조회다.</summary>
    public Func<uint>? Editor { get; set; }
    /// <summary>현재 미션 모드 조회다.</summary>
    public Func<uint>? MissionPost { get; set; }
}

/// <summary>객체 정보가 선택한 실제 문자·게임·메뉴 장치다.</summary>
public sealed class ObjectMenuInfoHooks
{
    /// <summary>현재 객체의 가상 수량이다.</summary>
    public Func<int, uint>? Count { get; set; }
    /// <summary>현재 TYPE 설명이다.</summary>
    public Func<uint, string>? Description { get; set; }
    /// <summary>현재 소유자 이름이다.</summary>
    public Func<uint, string>? OwnerName { get; set; }
    /// <summary>원문이 선택한 현재 번역이다.</summary>
    public Func<string, string>? Translate { get; set; }
    /// <summary>클래스 문구의 현재 설정 치환이다.</summary>
    public Func<string, string>? Substitute { get; set; }
    /// <summary>추가 번역 없는 원문 서식이다.</summary>
    public Func<string, IReadOnlyList<PriestMenuArgument>, string>? Print { get; set; }
    /// <summary>현재 메뉴 제목 작성이다.</summary>
    public Action<string>? Title { get; set; }
    /// <summary>현재 메뉴 정보 머리 작성이다.</summary>
    public Action<string>? Header { get; set; }
    /// <summary>현재 두 플레이어의 동맹 값이다.</summary>
    public Func<uint, uint, uint>? Alliance { get; set; }
    /// <summary>현재 객체의 포획 가능 지면 판정이다.</summary>
    public Func<int, bool>? GroundImmobile { get; set; }
    /// <summary>현재 소유자 Player 상태 바이트다.</summary>
    public Func<uint, byte>? OwnerStatus { get; set; }
}

/// <summary>10.78 객체 정보의 네 인자와 현재 상태 경고를 작성한다.</summary>
public sealed class RawObjectMenuInfo
{
    /// <summary>현재 50바이트 객체 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>효과 뒤에도 다시 읽는 현재 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>TYPE별 실제 클래스 문자열 표다.</summary>
    private readonly IReadOnlyList<string> _classes;
    /// <summary>같은 현재 역할과 모드다.</summary>
    private readonly ObjectMenuInfoState _state;
    /// <summary>선택된 외부 실제 장치다.</summary>
    private readonly ObjectMenuInfoHooks _hooks;

    /// <summary>같은 PATCH 풀·188 TYPE·클래스 표를 빌린다.</summary>
    public RawObjectMenuInfo(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<string> classes,
        ObjectMenuInfoState state, ObjectMenuInfoHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(classes);
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || classes.Count != 188) throw new ArgumentException("객체 정보 PATCH 풀·TYPE·클래스 표 오류");
        _pool = pool; _types = types; _classes = classes; _state = state; _hooks = hooks;
    }

    /// <summary>선택된 실제 함수가 없으면 그 위치에서 실패한다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("객체 메뉴 정보 실제 하위 장치 미연결");
    /// <summary>현재 물리 SID만 확인하며 별도의 상태 필터는 적용하지 않는다.</summary>
    private ReadOnlySpan<byte> Raw(int sid) => sid > 0 && sid < _pool.Capacity ? _pool.Slot(sid) : throw new ArgumentOutOfRangeException(nameof(sid));
    /// <summary>현재 TYPE의 실제 참조를 반환한다.</summary>
    private OriginalTypeRecord Type(uint type) => type < _types.Count ? _types[(int)type] : throw new ArgumentOutOfRangeException(nameof(type));
    /// <summary>첫 NUL 이전과 원본 클래스 23바이트 한도를 보존한다.</summary>
    private string Class(uint type) { _ = Type(type); string text = _classes[(int)type]; int zero = text.IndexOf('\0'); if (zero >= 0) text = text[..zero]; return text.Length <= 23 ? text : throw new ArgumentOutOfRangeException(nameof(type), "객체 정보 클래스 길이 초과"); }
    /// <summary>원본 종료 NUL을 포함한 고정 버퍼 한도를 검사한다.</summary>
    private static string Text(string value, int capacity) { int zero = value.IndexOf('\0'); if (zero >= 0) value = value[..zero]; return value.Length < capacity ? value : throw new ArgumentOutOfRangeException(nameof(value), "객체 정보 문자열 버퍼 초과"); }
    /// <summary>현재 서식 함수의 결과를 원본 256바이트 버퍼로 제한한다.</summary>
    private string Print(string pattern, params PriestMenuArgument[] arguments) => Text(Need(_hooks.Print)(pattern, arguments), 256);
    /// <summary>원본 signed64 절삭의 하위 DWORD다.</summary>
    private static int Truncate(float value) => !float.IsFinite(value) || (double)value < -9223372036854775808.0 || (double)value >= 9223372036854775808.0 ? 0 : unchecked((int)(long)value);
    /// <summary>원본 로마 숫자와 범위 밖 FF 바이트를 같은 문자로 돌린다.</summary>
    public static string LevelName(uint number) => Encoding.Latin1.GetString(RawSquidName.LevelName(number));
    /// <summary>원본 원소 이름 포인터 순서를 반환한다.</summary>
    public static string ThemeName(int number) => number switch { 0 => "Wind", 1 => "Rain", 2 => "Thunder", 3 => "Sun", 4 => "Energy", 5 => "Nature", 6 => "Holy", 7 => "Evil", _ => throw new ArgumentOutOfRangeException(nameof(number)) };

    /// <summary>TYPE·수량·소유자·SID 네 인자로 현재 정보 문구를 원본 순서대로 작성한다.</summary>
    public void Describe(uint type, uint count, uint owner, uint sid)
    {
        _ = Type(type); Need(_hooks.Title)(Text(Need(_hooks.Description)(type), 256));
        if (type == _state.Altar) count = unchecked(count - 1);
        if (Class(type).Length != 0 && (Type(type).Flags2 & 0x80074000u) != 0)
        {
            string pattern = Need(_hooks.Translate)(" Level %s");
            string level = LevelName(unchecked((int)count) < 0 ? 0u : count);
            Need(_hooks.Title)(Print(pattern, level));
        }
        if (type == _state.Nugget) owner = 0;
        string name = Text(unchecked((int)owner) > 0 ? Need(_hooks.OwnerName)(owner) : Need(_hooks.Translate)("Nobody"), 80);
        uint local = Need(_state.LocalOwner)();
        string relation = Text(owner == local || owner == 0 ? Need(_hooks.Translate)("Owner") : Need(_hooks.Translate)(Need(_hooks.Alliance)(owner, local) != 0 ? "Ally" : "Enemy"), 256);
        if (Need(_state.Editor)() != 0 && sid != uint.MaxValue)
        {
            if (sid > ushort.MaxValue) throw new ArgumentOutOfRangeException(nameof(sid));
            _ = Raw((int)sid);
            Need(_hooks.Header)(Print("\n%s: ~i%d~.", "SubID", unchecked((int)sid)));
            ReadOnlySpan<byte> raw = Raw((int)sid);
            int y = Truncate(BinaryPrimitives.ReadSingleLittleEndian(raw[18..])), x = Truncate(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]));
            Need(_hooks.Header)(Print("\nAt (~i%d~w,~i%d~w)~.", x, y));
        }
        Need(_hooks.Header)(Print("\n%s: ~i%s~.", relation, name));
        if (Class(type).Length != 0)
        {
            string pattern = Need(_hooks.Translate)("\nAlignment: ~i%s~."); int theme = Type(type).Theme;
            string alignment = Need(_hooks.Translate)(theme < 5 ? ThemeName(theme) : "None");
            Need(_hooks.Header)(Print(pattern, alignment));
            pattern = Need(_hooks.Translate)("\nClass: ~i%s~.");
            string translated = Need(_hooks.Translate)(Class(type));
            string substituted = Text(Need(_hooks.Substitute)(translated), 1024);
            Need(_hooks.Header)(Print(pattern, substituted));
        }
        Need(_hooks.Header)("\n");
    }

    /// <summary>현재 수량 호출 전 소유자·TYPE을 보존하고 이후 상태 경고를 다시 읽는다.</summary>
    public void Summary(int sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid); uint owner = raw[34], type = raw[10]; uint count = Need(_hooks.Count)(sid);
        Describe(type, count, owner, (uint)sid);
        if ((Raw(sid)[40] & 0x10) != 0) { string pattern = Need(_hooks.Translate)("~y~[I%d.*]Invisible!~.\n"); Need(_hooks.Header)(Print(pattern, unchecked((int)_state.Invisible))); }
        if ((Raw(sid)[40] & 0x20) != 0) { string pattern = Need(_hooks.Translate)("~y~[I%d.*]Paralyzed!~.\n"); Need(_hooks.Header)(Print(pattern, unchecked((int)_state.Paralyzed))); }
        CaptureStatus(sid);
    }

    /// <summary>실제 지면 관문과 현재 미션·소유자 상태로 포획 경고를 작성한다.</summary>
    public void CaptureStatus(int sid)
    {
        _ = Raw(sid); if (!Need(_hooks.GroundImmobile)(sid)) return;
        bool disconnected = Need(_state.MissionPost)() == 0 && (Need(_hooks.OwnerStatus)(Raw(sid)[34]) & 1) == 0;
        Need(_hooks.Header)(Need(_hooks.Translate)(disconnected ? "~yNot Connected!~.\n" : "~yCapturable!~.\n"));
    }
}
