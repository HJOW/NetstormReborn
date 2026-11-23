using System.Buffers.Binary;

// Rune166 후처리의 예약/신전조회/건설시간과 항상실행하는공통호출을 원문 순서대로 유지한다.
namespace Netstorm.Core.Original;

/// <summary>RawRunePostPop.cpp의 Rune 후처리 전체몸체다.</summary>
public sealed class RawRunePostPop
{
    /// <summary>두PE의 최초Rune Regular사건번호다.</summary>
    public const uint RuneEvent = 0x304;
    /// <summary>304예약 payload와 좌표 almostceil의 원본float다.</summary>
    private const float RunePayload = 0.01F;
    /// <summary>좌표 almostceil의 원본float다.</summary>
    private const float FinderBias = 0.9999899864196777F;
    /// <summary>실제165 생성자의 최종 가상표다.</summary>
    private const uint PatchRuneVtable = 0x005075d8;
    /// <summary>두PE의 Rune166 생성자가 기록하는 실제가상표다.</summary>
    private const uint CdRuneVtable = 0x00504148;
    /// <summary>유일한 현재 raw 저장소다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>같은 해시의 현재 후보다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>같은 풀/물리 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>공유 원본 전역의 참조다.</summary>
    private readonly RunePostPopState _state;
    /// <summary>같은Kernel에 실제로컬304 Regular를부착하는공통후처리몸체다.</summary>
    private readonly RawPostPop _base;
    /// <summary>실제호스트/몸체에연결한효과다.</summary>
    private readonly RunePostPopHooks _hooks;
    /// <summary>도달한 장치만 요구하며 자기 연결 반납 중 callable 수명을 유지한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Rune 후처리의 선택된 실제 하위 미연결");
    /// <summary>정렬되지않은 littleendian필드를지정폭만읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // 낮은바이트부터조합해ownerWORD와vtableDWORD를구별한다.
        for (int index = 0; index < width; index++) value |= (uint)raw[offset + index] << (8 * index);
        return value;
    }
    /// <summary>유한정상지도좌표를원본CRT처럼0방향으로절삭한다.</summary>
    private static int Cell(double value)
    {
        if (!double.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value), "Rune 후처리 절삭 좌표 범위");
        return (int)value;
    }
    /// <summary>같은 풀/물리 타입 표를 확인하며 CD의 기존 전수 장치 계약만 유지한다.</summary>
    public RawRunePostPop(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, RunePostPopState state, RawPostPop @base, RunePostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(@base); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, @base.Pool) || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171)
            || (pool.Edition == OriginalEdition.Cd1072 && (hooks.ReserveRegular is null || hooks.AddRegular is null || hooks.StartConstruction is null)))
            throw new ArgumentException("Rune 후처리 실제몸체/판본 연결 누락", nameof(@base));
        _pool = pool; _hash = hash; _types = types; _state = state; _base = @base; _hooks = hooks.Copy();
    }
    /// <summary>PATCH 가상 진입은 현재 TYPE와 무관하며 CD의 기존 분류는 유지한다.</summary>
    public bool Handles(ushort sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return (_pool.Edition == OriginalEdition.Patch1078 || raw[10] == _state.CurrentRuneType())
            && Read(raw, 0) == (_pool.Edition == OriginalEdition.Patch1078 ? PatchRuneVtable : CdRuneVtable);
    }
    /// <summary>공통장부의원래예외/Graph조건도공간쓰기전에그대로검사한다.</summary>
    public void Validate(ushort sid, uint flags, PopProjection? projected = null)
    {
        if (!Handles(sid) || _pool.Slot(sid)[10] != _state.CurrentRuneType()) throw new InvalidOperationException("Rune 후처리 타입/가상표 오류");
        if (projected is null) _base.ValidateBase(sid, flags);
        else _base.ValidateBase(sid, flags, projected.Value);
    }
    /// <summary>현재raw좌표를효과뒤에도각조회시다시읽는다.</summary>
    public RawPathPoint Position(ushort sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return new(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), BinaryPrimitives.ReadSingleLittleEndian(raw[18..]));
    }
    /// <summary>owner는원래ownerWORD가아닌patch+34/CD+32다.</summary>
    public uint Owner(ushort sid) => _pool.Slot(sid)[_pool.Edition == OriginalEdition.Patch1078 ? 34 : 32];
    /// <summary>반경1의기존일반finder를같은해시의현재후보에적용한다.</summary>
    public ushort FindOwnedDais(ushort rune) => FindOwnedDaisIn(_pool, _hash, _types, _state.CurrentDaisType(), rune);
    /// <summary>실제 같은 풀/현재 역할을 조회하며 추가 생명/할당 필터를 넣지 않는다.</summary>
    public static ushort FindOwnedDaisIn(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, uint daisType, ushort rune)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types);
        ReadOnlySpan<byte> raw = pool.Slot(rune);
        int x = Cell(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]) + FinderBias), y = Cell(BinaryPrimitives.ReadSingleLittleEndian(raw[18..]) + FinderBias);
        var finder = new RawSquidFinder(pool, hash, types);
        uint ownerOffset = pool.Edition == OriginalEdition.Patch1078 ? 34u : 32u;
        // free/dead/abstract는추가필터가아니며genericfinder가buried를제외한다.
        for (int sid = finder.Begin(new(x - 1, y - 1, x + 1, y + 1)); sid != 0; sid = finder.Next())
            if (pool.Slot(sid)[10] == daisType && pool.Slot(sid)[(int)ownerOffset] == pool.Slot(rune)[(int)ownerOffset]) return (ushort)sid;
        return 0;
    }
    /// <summary>기존 시간 helper의 typed altar 조회 본문을 같은 풀의 의식 소비자에 공개한다.</summary>
    public ushort AltarAt(ushort rune)
    {
        uint type = _state.CurrentAltarType(); if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(rune), "Rune 현재altar 타입범위");
        RawPathPoint point = Position(rune); int x = Cell(point.X), y = Cell(point.Y);
        var finder = new RawSquidFinder(_pool, _hash, _types); ushort altar = 0;
        // 현재타입genus&6에따른시작단계와원본첫타입일치를재사용한다.
        for (int sid = finder.Begin(new(x, y, x, y), (_types[(int)type].Flags2 & 6) == 0 ? 1u : 0u); sid != 0; sid = finder.Next())
            if (_pool.Slot(sid)[10] == type) { altar = (ushort)sid; break; }
        return altar;
    }
    /// <summary>patch의char7/10/25와CD의int7/10/25를동일정수로반환한다.</summary>
    public int ConstructionSeconds(ushort rune)
    {
        ushort altar = AltarAt(rune);
        if (altar == 0) return 10;
        uint originalOwner = Read(_pool.Slot(altar), 12, 2);
        if (originalOwner == 0 || (_hooks.SuppressDuration is not null ? Need(_hooks.SuppressDuration)() : _state.CurrentSuppressDuration())) return 10;
        if (_hooks.PlayerFlags is not null) return (Need(_hooks.PlayerFlags)(originalOwner) & 1) != 0 ? 25 : 7;
        if (originalOwner >= (uint)_state.PlayerFlags.Length) throw new ArgumentOutOfRangeException(nameof(rune), "Rune altar 원래ownerWORD/player범위");
        return (_state.PlayerFlags[originalOwner] & 1) != 0 ? 25 : 7;
    }
    /// <summary>예약확보가실패해도신전/건설/공통후처리를계속한다. 발견한 Dais가 시간 helper의 this다. Rune의 위치로 대신 계산하지 않는다.</summary>
    public void PostPop(ushort rune, uint flags)
    {
        if (_pool.Edition == OriginalEdition.Cd1072) Validate(rune, flags);
        if ((flags & 1) != 0 && Need(_hooks.ReserveRegular)()) Need(_hooks.AddRegular)(rune, RuneEvent, RunePayload);
        if ((flags & 2) != 0)
        {
            ushort dais = FindOwnedDais(rune);
            if (dais != 0 && dais < _pool.Capacity) { float seconds = ConstructionSeconds(dais); Need(_hooks.StartConstruction)(rune, seconds); }
        }
        _base.PostPopBase(rune, flags);
    }
    /// <summary>등록/어댑터에동일한실제풀을제공한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>기존localRegular와실제건설프로세스를직접재사용한다.</summary>
    public static RunePostPopHooks MakeRunePostPopHooks(RawSidPool pool, SquidProcessHost host, RawConstructionProcess construction, RunePostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(host); ArgumentNullException.ThrowIfNull(construction); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, host.Pool()) || !ReferenceEquals(pool, construction.Pool)) throw new ArgumentException("Rune Host/Construction SID 풀 불일치", nameof(host));
        hooks.AddRegular = (sid, @event, payload) => { host.AddRegular(sid, @event, payload); };
        hooks.StartConstruction = (sid, seconds) => construction.Start(sid, seconds);
        return hooks;
    }
}
