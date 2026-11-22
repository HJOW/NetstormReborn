using System.Buffers.Binary;

namespace Netstorm.Core.Original; // 현재 Deck과 실제 기술 부모 내용물을 원문 순서로 동기화한다.

/// <summary>현재 실제 owner/fullDeck을 값 사본 없이 읽는다.</summary>
public sealed class ProductionSyncState
{
    /// <summary>00540c70의 현재 로컬 owner다.</summary>
    public required Func<uint> LocalOwner { get; init; }
    /// <summary>0054db14의 현재 전체 Deck DWORD다.</summary>
    public required Func<uint> FullDeck { get; init; }
}

/// <summary>같은 현재 Deck/기술과 실제 삭제·커서·생성 경계다.</summary>
public sealed class ProductionSyncHooks
{
    /// <summary>44f760의 실제 현재 templateOwner Deck 원문이다.</summary>
    public Func<ReadOnlyMemory<byte>>? Deck { get; set; }
    /// <summary>48fc60의 현재 기술 부모이며 순회 전후 각각 읽는다.</summary>
    public Func<uint>? Technology { get; set; }
    /// <summary>현재 Player+7c의 비0 여부다.</summary>
    public Func<uint, bool>? Temple { get; set; }
    /// <summary>선택한 내용물 삭제 전 실제 커서 취소다.</summary>
    public Action? CancelCursor { get; set; }
    /// <summary>내용물 가상+10의 실제 삭제다.</summary>
    public Action<uint, uint>? Destroy { get; set; }
    /// <summary>실제 검색의 저장 kind WORD 반환이다. SID가 아니다.</summary>
    public Func<uint, uint, uint, uint, uint>? FindKind { get; set; }
    /// <summary>부모/kind/quantity/flags/attach/quality의 실제 내용물 생성/부착이다.</summary>
    public Func<uint, uint, uint, uint, uint, uint, uint>? Create { get; set; }
    /// <summary>정상 복귀하면 중복 생성도 계속하는 원문 진단이다.</summary>
    public Action<string>? Assertion { get; set; }
}

/// <summary>Source RawProductionSync43df80 전체다. 같은 물리와255BYTE 중복 카운터를 사용한다.</summary>
public sealed class RawProductionSync
{
    /// <summary>실제 내용물 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>현재188TYPE의 flags2다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>같은 현재 내용물 순회 정책이다.</summary>
    private readonly ContainedFinderState _finder;
    /// <summary>현재 실제 원문 전역 getter다.</summary>
    private readonly ProductionSyncState _state;
    /// <summary>도달한 실제 장치다.</summary>
    private readonly ProductionSyncHooks _hooks;
    /// <summary>선택된 실제 하위만 도달 위치에서 요구한다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("생산 내용물 동기화 실제 하위 미연결");
    /// <summary>PATCH/188TYPE와 같은 현재 풀/정책을 빌리고 상태를 바꾸지 않는다.</summary>
    public RawProductionSync(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ContainedFinderState finder, ProductionSyncState state, ProductionSyncHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(finder); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188) throw new ArgumentException("생산 내용물 동기화 PATCH 자료 오류"); Pool = pool; _types = types; _finder = finder; _state = state; _hooks = hooks;
    }
    /// <summary>SID0도 실제 슬롯을 읽으며 전체DWORD 범위 확인 뒤에만 좁힌다.</summary>
    private int Parent(uint value) => value < Pool.Capacity && value <= ushort.MaxValue ? (int)value : throw new ArgumentOutOfRangeException(nameof(value), "생산 동기화 현재 기술 부모 범위 오류");
    /// <summary>원본 signed 개수다. 음수는 빈 순회이며 물리66개를 넘지 못한다.</summary>
    private static int Count(ReadOnlyMemory<byte> deck)
    {
        if (deck.Length < ProductionDeckLayout.Bytes) throw new ArgumentException("생산 동기화 실제 Deck 크기 부족", nameof(deck)); int count = BinaryPrimitives.ReadInt32LittleEndian(deck.Span[0x14c..]); if (count > 66) throw new ArgumentOutOfRangeException(nameof(deck), "생산 동기화 Deck 물리 개수 초과"); return count;
    }
    /// <summary>중복 중 첫 type BYTE 일치의 signed power만 반환한다.</summary>
    private static sbyte? Power(ReadOnlyMemory<byte> deck, uint kind)
    {
        // 같은 포인터의 현재 개수를 매 반복마다 읽는다.
        for (int i = 0; i < Count(deck); i++) { int offset = i * 5; if (deck.Span[offset] == kind) return unchecked((sbyte)deck.Span[offset + 2]); }
        return null;
    }
    /// <summary>Deck getter→다음 후보 선조회/현재 필드→삭제→현재 부모/개수 재조회/생성의 실제 순서다.</summary>
    public void Synchronize()
    {
        ReadOnlyMemory<byte> deck = Need(_hooks.Deck)(); var seen = new byte[255]; var finder = new RawContainedFinder(Pool, _finder); int current = finder.Begin(Parent(Need(_hooks.Technology)()));
        // 삭제 대상도 먼저 seen을 올리며 다음 후보를 찾은 뒤 현재 kind/flags를 읽는다.
        while (current != 0)
        {
            int form = current; current = finder.Next(); var raw = Pool.Slot(form); uint flags = BinaryPrimitives.ReadUInt16LittleEndian(raw[20..]), kind = BinaryPrimitives.ReadUInt16LittleEndian(raw[18..]); if (kind >= _types.Count || kind >= seen.Length) throw new ArgumentOutOfRangeException(nameof(kind), "생산 동기화 내용물 현재 TYPE 범위 오류"); if ((flags & 8) != 0) seen[kind] = unchecked((byte)(seen[kind] + 1)); bool remove = false;
            if ((_types[(int)kind].Flags2 & 4) != 0) remove = !Need(_hooks.Temple)(_state.LocalOwner()); else if ((flags & 8) != 0) { sbyte? power = Power(deck, kind); if (power is null || power < 1) remove = _state.FullDeck() == 0; }
            if (remove) { if ((flags & 0x100) != 0) Need(_hooks.CancelCursor)(); Need(_hooks.Destroy)((uint)form, 0); }
        }
        int parent = Parent(Need(_hooks.Technology)());
        // 매번 같은 Deck의 현재 개수를 읽고 새 생성 뒤에는 seen을 올리지 않는다.
        for (int i = 0; i < Count(deck); i++) { uint kind = deck.Span[i * 5]; if (kind >= seen.Length) throw new ArgumentOutOfRangeException(nameof(kind), "생산 동기화 Deck type BYTE 범위 오류"); if (seen[kind] == 0) { if (Need(_hooks.FindKind)((uint)parent, 8, kind, 0) != 0) Need(_hooks.Assertion)("!me.findContainedType( CFF_TELEPORT, type )"); Need(_hooks.Create)((uint)parent, kind, 0, 8, 16, 3); } }
    }
}
