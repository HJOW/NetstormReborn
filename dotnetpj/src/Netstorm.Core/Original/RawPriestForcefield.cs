using System.Buffers.Binary;

// 사제 한 점의 실제 공간 탐색과 현재 TYPE·소유자 비교로 보호막을 조회한다.
namespace Netstorm.Core.Original;

/// <summary>독립 typed 조회가 빌리는 현재 보호막TYPE DWORD다.</summary>
public sealed class PriestForcefieldState
{
    /// <summary>원본005412f4의 독립 기본값이며 BYTE로 줄이지 않는다.</summary>
    public uint Type { get; set; } = 167;
}

/// <summary>원본004918e0의 좌표 절삭·일반 finder·현재 소유자 조회를 조합한다.</summary>
public sealed class RawPriestForcefield
{
    /// <summary>매 호출 현재 raw를 읽는 동일 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>사제와 보호막이 사용하는 동일 공간 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>독립 생성 시 복사하거나 Native 연결에서 빌린 실제 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>독립 typed 조회의 현재 역할 상태다.</summary>
    private readonly PriestForcefieldState? _state;
    /// <summary>Native가 보호막 역할 DWORD를 다시 읽는 장치다.</summary>
    private readonly Func<uint>? _currentType;

    /// <summary>판본별 TYPE를 복사하고 독립 상태·풀·해시를 빌린다.</summary>
    public RawPriestForcefield(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, PriestForcefieldState state)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171))
            throw new ArgumentException("사제 보호막 조회 TYPE 판본 오류", nameof(types));
        _pool = pool; _hash = hash; _types = types.Select(type => type.Clone()).ToArray(); _state = state;
    }

    /// <summary>실제 PATCH의 현재 TYPE 표와 역할 DWORD를 빌리며 typed 사제 사전 관문을 추가하지 않는다.</summary>
    public RawPriestForcefield(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, Func<uint> currentType)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(currentType);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)
            throw new ArgumentException("Native 사제 보호막 조회 판본·TYPE 오류", nameof(types));
        _pool = pool; _hash = hash; _types = types; _currentType = currentType;
    }

    /// <summary>float→signed int의 원본 정상 범위를 검사하고 0방향으로 절삭한다.</summary>
    private static int Cell(uint bits)
    {
        double value = BitConverter.UInt32BitsToSingle(bits);
        if (!double.IsFinite(value) || value < int.MinValue || value > int.MaxValue)
            throw new ArgumentOutOfRangeException(nameof(bits), "사제 보호막 좌표 절삭 범위 오류");
        return (int)value;
    }

    /// <summary>매번 독립 커서로 level→y→x→next 순서를 읽고 첫 타입/현재owner 일치를 반환한다.</summary>
    public int Find(int priest)
    {
        if (_currentType is null && (priest < 5 || priest >= _pool.Capacity))
            throw new ArgumentOutOfRangeException(nameof(priest), "사제 보호막 조회 SID 오류");
        ReadOnlySpan<byte> raw = _pool.Slot(priest); bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (_currentType is null && (raw[10] != 158 || BinaryPrimitives.ReadUInt32LittleEndian(raw) != (patch ? 0x0050f210u : 0x005003e0u) || (raw[11] & 9) != 0))
            throw new ArgumentException("사제 보호막 조회 raw 상태 오류", nameof(priest));
        int x = Cell(BinaryPrimitives.ReadUInt32LittleEndian(raw[14..]));
        int y = Cell(BinaryPrimitives.ReadUInt32LittleEndian(raw[18..])); int ownerOffset = patch ? 34 : 32;
        var finder = new RawSquidFinder(_pool, _hash, _types);
        // 기본 true 필터이며 후보 free·dead·void를 추가로 제외하지 않는다.
        for (int sid = finder.Begin(new SquidSearchArea(x, y, x, y)); sid != 0; sid = finder.Next())
        {
            ReadOnlySpan<byte> candidate = _pool.Slot(sid);
            if (candidate[10] == Type() && candidate[ownerOffset] == _pool.Slot(priest)[ownerOffset]) return sid;
        }
        return 0;
    }

    /// <summary>조회 뒤 생성 직전에도 현재 역할 DWORD를 다시 읽는다.</summary>
    public uint Type() => _currentType is not null ? _currentType() : _state!.Type;
    /// <summary>삭제 준비·사제 후반 어댑터가 같은 실제 풀인지 확인하는 참조다.</summary>
    public RawSidPool Pool() => _pool;

    /// <summary>같은 풀의 실제 보호막 조회만 바꾸고 나머지 삭제/Carrier 효과는 원문 값 복사로 유지한다.</summary>
    public static PriestPreDestroyHooks MakePriestForcefieldHooks(RawSidPool pool, RawPriestForcefield lookup, PriestPreDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(lookup); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, lookup.Pool())) throw new ArgumentException("사제 보호막 조회·삭제 준비 풀 불일치");
        return new PriestPreDestroyHooks
        {
            FindForcefield = sid => unchecked((uint)lookup.Find(sid)), DestroyForcefield = hooks.DestroyForcefield,
            ValidateCarrier = hooks.ValidateCarrier, CarrierPre = hooks.CarrierPre,
        };
    }
}
