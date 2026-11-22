using Netstorm.Core.Original;

namespace Netstorm.Client.Display; // 모든 owner 그림이 같은 현재 색표를 빌리는 장기 저장소다.

/// <summary>보호10.78의 현재 TYPE 전역이며 원문 호출 순서로 유지한다.</summary>
public sealed class OwnerColorTableTypes
{
    /// <summary>배너 활성화 때 순서대로 반납하는 실제 다섯 TYPE다.</summary>
    public uint[] Banner { get; } = [85, 71, 128, 97, 94];
    /// <summary>최초 등록하는 실제 일반12TYPE다.</summary>
    public uint[] Standard { get; } = [82, 155, 158, 164, 135, 133, 156, 73, 170, 146, 144, 126];
    /// <summary>최초 전용 명도 표의 세 TYPE다.</summary>
    public uint[] Special { get; } = [111, 141, 137];
}

/// <summary>현재 DWORD와 포인터 표를 같은 실제 owner에서 빌리는 원문 인자다.</summary>
public sealed class OwnerColorTableGlobals
{
    /// <summary>00557938의 현재 준비 DWORD다.</summary>
    public required UIntBox Ready { get; init; }
    /// <summary>005319c0의 현재 배너 설정 DWORD다.</summary>
    public required UIntBox Enabled { get; init; }
    /// <summary>0059a8b0의 현재 전체 갱신 DWORD다.</summary>
    public required UIntBox Redraw { get; init; }
    /// <summary>TYPE별 같은 현재00557940 토큰 표다.</summary>
    public required uint[] Tables { get; init; }
}

/// <summary>계산을 대체하지 않는 실제 색표 생성과 반납 장치다.</summary>
public sealed class OwnerColorTableHooks
{
    /// <summary>현재 TYPE의 실제 일반9색표 생성이다.</summary>
    public Action<uint>? Standard { get; set; }
    /// <summary>현재 TYPE/명도/원문 소스의 실제 전용9색표 생성이다.</summary>
    public Action<uint, double, IReadOnlyList<uint>>? Special { get; set; }
    /// <summary>현재 비0토큰의 실제 표 반납이며 원문 본체가 뒤에0을 쓴다.</summary>
    public Action<uint>? Release { get; set; }
}

/// <summary>0043bc40/0043be30의 현재 관문·store·생성/반납 순서 전체다.</summary>
public static class OwnerColorTableOperations
{
    /// <summary>실제 선택한 효과 장치가 없으면 누락을 드러낸다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("공통 소유자 색표 장치 미연결");
    /// <summary>준비/같은값 관문 뒤 설정→dirty를 쓰고 현재 포인터를 순서대로 반납한다.</summary>
    public static void SetBannerOption(OwnerColorTableGlobals globals, uint requested, OwnerColorTableTypes types, OwnerColorTableHooks hooks)
    {
        if (globals.Ready.Value == 0 || globals.Enabled.Value == requested) return;
        uint dirty = unchecked(globals.Redraw.Value + 1); globals.Enabled.Value = requested; globals.Redraw.Value = dirty;
        if (requested == 0) { Need(hooks.Special)(types.Banner[3], 0, OwnerColorMapBuilder.SpecialOwnerColorSources(97)); Need(hooks.Special)(types.Banner[4], 0, OwnerColorMapBuilder.SpecialOwnerColorSources(94)); return; }
        // 각 현재 TYPE와 포인터를 해당 소비 시점에 읽고 free가 복귀한 뒤0을 쓴다.
        foreach (uint type in types.Banner) { if (type >= globals.Tables.Length) throw new ArgumentOutOfRangeException(nameof(type), "배너 현재 TYPE 색표 범위"); uint pointer = globals.Tables[type]; if (pointer != 0) { Need(hooks.Release)(pointer); globals.Tables[type] = 0; } }
    }
    /// <summary>최초 강제 설정과 일반12/전용3표를 실제 원문 순서로 등록한다.</summary>
    public static void InitializeOwnerColorTables(OwnerColorTableGlobals globals, uint skip, OwnerColorTableTypes types, OwnerColorTableHooks hooks)
    {
        uint previous = globals.Enabled.Value; globals.Ready.Value = 1; globals.Enabled.Value = uint.MaxValue; SetBannerOption(globals, previous, types, hooks);
        if (skip == 0) { Need(hooks.Standard)(types.Standard[0]); Need(hooks.Standard)(types.Standard[1]); }
        Need(hooks.Special)(types.Special[0], 0, OwnerColorMapBuilder.SpecialOwnerColorSources(111)); Need(hooks.Special)(types.Special[1], -0.15000000596046448, OwnerColorMapBuilder.SpecialOwnerColorSources(141));
        // 중간 네 일반 표 뒤에137 전용 표를 만든다.
        for (int index = 2; index < 6; index++) Need(hooks.Standard)(types.Standard[index]);
        Need(hooks.Special)(types.Special[2], 0, OwnerColorMapBuilder.SpecialOwnerColorSources(137));
        // 마지막 여섯 일반 표도 원문 호출 순서를 유지한다.
        for (int index = 6; index < types.Standard.Length; index++) Need(hooks.Standard)(types.Standard[index]);
    }
}

/// <summary>화면/이미지/픽셀 선택이 공유하는 단일 현재 색표 소유자다.</summary>
public sealed class OwnerColorTableState
{
    /// <summary>호스트 주소 폭과 독립인 원문 표의 opaque 기준 토큰이다.</summary>
    public const uint TokenBase = 0x100000;
    /// <summary>한 TYPE의9×256 BYTE 크기다.</summary>
    public const uint TableBytes = 2304;
    /// <summary>실제 준비 DWORD의 고정 주소다.</summary>
    private readonly UIntBox _ready = new();
    /// <summary>PE 초기1인 현재 배너 DWORD의 고정 주소다.</summary>
    public UIntBox EnabledBox { get; } = new() { Value = 1 };
    /// <summary>원문 현재 TYPE 순서다.</summary>
    private readonly OwnerColorTableTypes _types = new();
    /// <summary>최초 계산에 쓰인 실제 다리 TYPE다.</summary>
    private uint _bridgeType = 82;
    /// <summary>저장 명도가 색표보다 오래 살도록 같은 primary를 보유한다.</summary>
    private RawColorSortPrimary? _primary;
    /// <summary>초기화 후 배열을 교체하지 않는 TYPE별 현재 토큰이다.</summary>
    private uint[] _tokens = [];
    /// <summary>각 현재 비0토큰의 실제9색표 소유자다.</summary>
    private OwnerColorMaps?[] _maps = [];
    /// <summary>실제 준비 DWORD만 조회한다.</summary>
    public bool Ready => _ready.Value != 0;
    /// <summary>실제 현재 배너 DWORD다.</summary>
    public uint Enabled => EnabledBox.Value;
    /// <summary>실제 초기 준비 후 같은 포인터 표를 복사 없이 빌린다.</summary>
    public ReadOnlyMemory<uint> Tokens => Ready ? _tokens : throw new InvalidOperationException("공통 소유자 색표 최초 등록 전 조회");
    /// <summary>같은 현재 dirty와 자신의 실제 표 주소를 원문 인자로 연결한다.</summary>
    private OwnerColorTableGlobals Globals(UIntBox redraw) => new() { Ready = _ready, Enabled = EnabledBox, Redraw = redraw, Tables = _tokens };
    /// <summary>초기 등록을 한 번만 실행하며 미완료 primary는 거부한다.</summary>
    public void Initialize(RawColorSortPrimary primary, int typeCount, uint bridgeType, UIntBox redraw, uint skip = 0)
    {
        ArgumentNullException.ThrowIfNull(primary); ArgumentNullException.ThrowIfNull(redraw); if (Ready || !primary.Constructed || typeCount <= 170) throw new ArgumentException("최초 공통 소유자 색표 준비 계약");
        _primary = primary; _bridgeType = bridgeType; _tokens = new uint[typeCount]; _maps = new OwnerColorMaps?[typeCount]; OwnerColorTableOperations.InitializeOwnerColorTables(Globals(redraw), skip, _types, Hooks());
    }
    /// <summary>초기 준비 전에도 원문처럼 무시하며 이후 같은 현재 표를 동기 변경한다.</summary>
    public void SetEnabled(uint value, UIntBox redraw) => OwnerColorTableOperations.SetBannerOption(Globals(redraw), value, _types, Hooks());
    /// <summary>현재 TYPE/색번호의 실제 비0표를 빌린다.</summary>
    public byte[] Color(uint type, uint palette) => type < _maps.Length && _tokens[type] != 0 && _maps[type] is { } maps && palette <= 8 ? maps.Tables[palette] : throw new ArgumentOutOfRangeException(nameof(type), "현재 공통 소유자 색표 조회 계약");
    /// <summary>ImageGump의 같은 base+색번호×256 토큰을 현재 표로 해석한다.</summary>
    public byte[] ColorToken(uint token)
    {
        if (token < TokenBase || (token - TokenBase) % 256 != 0) throw new ArgumentOutOfRangeException(nameof(token), "공통 색표 토큰 정렬"); uint offset = token - TokenBase; return Color(offset / TableBytes, offset % TableBytes / 256);
    }
    /// <summary>자신의 실제 표만 생성/반납하며 token0 쓰기는 원문 본체에 남긴다.</summary>
    private OwnerColorTableHooks Hooks() => new() { Standard = Standard, Special = Special, Release = token => { if (token < TokenBase || (token - TokenBase) % TableBytes != 0) throw new ArgumentOutOfRangeException(nameof(token), "공통 색표 반납 토큰"); _maps[(token - TokenBase) / TableBytes] = null; } };
    /// <summary>현재 TYPE/다리 특례의 실제 일반9색표를 설치한다.</summary>
    private void Standard(uint type) { _maps[type] = OwnerColorMapBuilder.BuildStandardOwnerColorMaps(type, _bridgeType); _tokens[type] = unchecked(TokenBase + type * TableBytes); }
    /// <summary>원문 소스/명도 계약을 확인하고 같은 primary로 전용 표를 만든다.</summary>
    private void Special(uint type, double offset, IReadOnlyList<uint> sources)
    {
        var expected = OwnerColorMapBuilder.SpecialOwnerColorSources(type); double wanted = type == 141 ? -0.15000000596046448 : 0; if (!sources.SequenceEqual(expected) || BitConverter.DoubleToUInt64Bits(offset) != BitConverter.DoubleToUInt64Bits(wanted)) throw new InvalidOperationException("공통 전용 색표의 원본 소스/명도 계약");
        _maps[type] = OwnerColorMapBuilder.BuildSpecialOwnerColorMaps(_primary ?? throw new InvalidOperationException("공통 색표 primary 미연결"), type); _tokens[type] = unchecked(TokenBase + type * TableBytes);
    }
}
