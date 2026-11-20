using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 공장 목록·내용물·Player Deck·금지 표를 같은 월드에서 재계산한다.
namespace Netstorm.Client.Mission;

/// <summary>효과 뒤에도 같은 현재 생산 모드와 역할을 조회한다.</summary>
public sealed class CampaignProductionState
{
    /// <summary>현재 단독 미션 모드다.</summary>
    public Func<bool>? Mission { get; set; }
    /// <summary>비미션에서 재계산을 생략하는 현재 전체 Deck DWORD다.</summary>
    public Func<uint>? FullDeck { get; set; }
    /// <summary>현재 getTeleportTechFree가 추가하는 Mog153 역할이다.</summary>
    public Func<uint>? FreeTechnologyType { get; set; }
    /// <summary>현재 신전 기본 생산 Sun Walker133 역할이다.</summary>
    public Func<uint>? TempleProduct { get; set; }
}

/// <summary>재계산의 실제 설정과 원문 모드 진단만 외부로 연결한다.</summary>
public sealed class CampaignProductionEffects
{
    /// <summary>현재 getTeleportTechFree 설정을 조회한다.</summary>
    public Func<string, int>? ConfigInt { get; set; }
    /// <summary>inFortMode||inBattleMode 진단 뒤 현재 모드를 재조회할 수 있다.</summary>
    public Action? ModeAssertion { get; set; }
}

/// <summary>0044f7c0·00475530·004752b0(mode0)을 실제 월드의 같은 저장소에 연결한다.</summary>
/// <remarks>근거: cpppj/src/client/RawCampaignProductionBindings.cpp/.h다. 표시 Gump는 별도 실제 소비자다.</remarks>
public sealed class RawCampaignProductionBindings
{
    /// <summary>현재 공장·내용물의 실제 슬롯 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재188개 TYPE의 flags와 종류를 읽는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>공통 Pop/삭제가 직접 변경하는 현재 공장 목록이다.</summary>
    private readonly RawPostPopState _book;
    /// <summary>같은 현재 Fort/Battle 모드다.</summary>
    private readonly SquidOwnerMode _mode;
    /// <summary>현재 templateOwner와 실제 Player 기록이다.</summary>
    private readonly RawPlayerWorld _player;
    /// <summary>같은 Player+68와 실제 블록을 관리한다.</summary>
    private readonly RawProductionDeck _deck;
    /// <summary>메뉴/튜토리얼과 공유하는 실제 금지 표와 장부다.</summary>
    private readonly TotalMade _allowed;
    /// <summary>현재 내용물 역할·권한·진단을 사용하는 순회 상태다.</summary>
    private readonly ContainedFinderState _contained;
    /// <summary>현재 모드/역할을 조회하는 뷰다.</summary>
    private readonly CampaignProductionState _state;
    /// <summary>설정/진단의 실제 효과다.</summary>
    private readonly CampaignProductionEffects _effects;
    /// <summary>같은 Player·검색·생성 사슬의 기술 표시다.</summary>
    private readonly RawIndicateTechnology _indicate;

    /// <summary>실제 월드/Player/Deck 동일성과 PATCH188개 TYPE를 확인하고 내용을 바꾸지 않는다.</summary>
    public RawCampaignProductionBindings(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawPostPopState book,
        SquidOwnerMode mode, RawPlayerWorld player, RawCarrierPreDestroy carrier, RawContainedCreate create,
        RawProductionDeck deck, TotalMade allowed, ContainedFinderState contained, CampaignProductionState state, CampaignProductionEffects effects)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(book);
        ArgumentNullException.ThrowIfNull(mode); ArgumentNullException.ThrowIfNull(player); ArgumentNullException.ThrowIfNull(deck);
        ArgumentNullException.ThrowIfNull(allowed); ArgumentNullException.ThrowIfNull(contained); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(effects);
        if (!ReferenceEquals(pool, player.Pool()) || !ReferenceEquals(pool, deck.Pool) || !ReferenceEquals(player, deck.Player)
            || pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)
            throw new ArgumentException("생산 재계산의 같은 실제 월드/Player/Deck 연결 누락");
        _pool = pool; _types = types; _book = book; _mode = mode; _player = player; _deck = deck; _allowed = allowed;
        _contained = contained; _state = state; _effects = effects; _indicate = new(player, carrier, create);
    }
    /// <summary>월드 장치 동일성 확인에 사용하는 실제 풀이다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>같은 현재 Player 포인터와 실제 할당 저장소다.</summary>
    public RawProductionDeck Deck => _deck;
    /// <summary>dirty나 현재 건물 수 조건을 추가하지 않고 같은 원문 Deck만 조회한다.</summary>
    public bool Contains(uint type) => _deck.Contains(type);
    /// <summary>현재 기술 부모와 저장 kind WORD를 사용하는 실제 기술 표시다.</summary>
    public void IndicateTechnology(uint type) => _indicate.Indicate(type);
    /// <summary>호출 시점의 현재 templateOwner를 다시 읽어 재계산한다.</summary>
    public uint RefreshCurrent() => Refresh(_player.State().TemplateOwner);
    /// <summary>같은 현재 금지 표를 표시/미션 소비자에게 제공한다.</summary>
    public bool Allowed(int type) => _allowed.Allowed(type);
    /// <summary>원문 두 Factory 가상표의 현재 level BYTE 상위3비트에2를 더한다.</summary>
    public int FactoryCapacity(int factory)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(factory);
        if (BinaryPrimitives.ReadUInt32LittleEndian(raw) is not (0x509148u or 0x509210u))
            throw new InvalidOperationException("생산 Factory 가상+b4의 현재 실제 본체 미복원");
        return (raw[11] >> 5) + 2;
    }
    /// <summary>현재 생산 내용을 순회해 Deck에 활성화하고 서로 다른 종류별 남은 슬롯을 돌려준다.</summary>
    public int AppendFactoryProducts(int factory, uint deckToken)
    {
        int remaining = FactoryCapacity(factory); var seen = new byte[255]; var finder = new RawContainedFinder(_pool, _contained);
        // 후보 반환 전에 저장된 next를 보존하고 현재 kind WORD와 flags BYTE를 읽는다.
        for (int content = finder.Begin(factory); content != 0; content = finder.Next())
        {
            ReadOnlySpan<byte> raw = _pool.Slot(content);
            if ((raw[20] & 0x10) == 0) continue;
            uint kind = BinaryPrimitives.ReadUInt16LittleEndian(raw[18..]);
            if (kind >= _types.Count || kind >= seen.Length) throw new ArgumentOutOfRangeException(nameof(factory), "생산 Factory의 현재 TYPE/중복 표 범위 오류");
            _ = _types[(int)kind]; _ = _allowed.Allowed((int)kind);
            if (deckToken != 0) _deck.Activate(deckToken, kind, 1);
            if (seen[kind] == 0) { remaining--; seen[kind] = 1; }
        }
        return remaining;
    }
    /// <summary>현재 공장/소유자/내용물로 같은 Deck의 power와 가중합을 원문 순서로 재계산한다.</summary>
    public uint Refresh(uint owner)
    {
        uint token = _deck.Get(owner);
        if (!_mode.Fort && !_mode.Battle) Need(_effects.ModeAssertion)();
        if (!Need(_state.Mission)() && Need(_state.FullDeck)() != 0) return token;
        Memory<byte> raw = _deck.Bytes(token);
        // 기존 chance/remaining/order는 유지하고 현재 항목의 power만-1로 바꾼다.
        for (int index = 0; index < Count(raw.Span); index++) raw.Span[index * ProductionDeckLayout.EntryBytes + 2] = 255;
        Put(raw.Span, ProductionDeckLayout.Chance, 0);
        if (Need(_effects.ConfigInt)("getTeleportTechFree") != 0) _deck.Activate(token, Need(_state.FreeTechnologyType)(), 1);
        // 공통 Pop/삭제가 바꿀 수 있는 현재 개수와 DWORD 목록을 매번 읽는다.
        for (uint index = 0; index < _book.Factories.Count; index++)
        {
            int current = Factory(_book.Factories.Items()[(int)index]); ReadOnlySpan<byte> item = _pool.Slot(current);
            if ((uint)item[34] != owner) continue;
            uint type = item[10];
            if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(owner), "생산 Factory 현재 TYPE 범위 오류");
            if ((_types[(int)type].Flags2 & 0x200u) != 0)
            {
                if (_allowed.Allowed(unchecked((int)Need(_state.TempleProduct)()))) _deck.Activate(token, Need(_state.TempleProduct)(), 1);
            }
            else if ((_types[(int)type].Flags1 & 0x04000000u) != 0) _ = AppendFactoryProducts(current, token);
        }
        _deck.Compact(token); raw = _deck.Bytes(token); uint total = 0;
        // 양수 signed power의 BYTE chance만 DWORD 감김으로 합산한다.
        for (int index = 0; index < Count(raw.Span); index++)
        {
            int offset = index * ProductionDeckLayout.EntryBytes;
            if (unchecked((sbyte)raw.Span[offset + 2]) > 0) total = unchecked(total + raw.Span[offset + 1]);
        }
        Put(raw.Span, ProductionDeckLayout.Chance, total); return token;
    }
    /// <summary>목록의 전체 DWORD가 실제 Pool/WORD 범위일 때만 SID로 변환한다.</summary>
    private int Factory(uint value) => value == 0 || value >= _pool.Capacity || value > ushort.MaxValue
        ? throw new ArgumentOutOfRangeException(nameof(value), "생산 Factory 실제 SID 범위 오류") : (int)value;
    /// <summary>signed 음수는 빈 순회이며 양수의66항목 물리 초과만 거부한다.</summary>
    private static int Count(ReadOnlySpan<byte> raw)
    {
        int count = BinaryPrimitives.ReadInt32LittleEndian(raw[ProductionDeckLayout.Count..]);
        return count > ProductionDeckLayout.Capacity ? throw new ArgumentOutOfRangeException(nameof(raw), "생산 재계산 Deck 물리 범위 오류") : count;
    }
    /// <summary>같은 Deck의 지정 DWORD만 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>선택된 실제 설정/진단/전역이 없으면 호출 지점에서 거부한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("생산 재계산 실제 효과/전역 미연결");
}
