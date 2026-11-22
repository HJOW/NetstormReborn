using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client; // 메뉴 HUD와 현재 Session이 같은 실제 Deck/내용물 동기화를 사용한다.

/// <summary>Source ClientProductionSync의 현재 대여 분기와 장기 물리 조립 전체다.</summary>
public static class ClientProductionSync
{
    /// <summary>현재 Player의 실제 DWORD만 읽고 owner/필드/물리 범위를 검증한다.</summary>
    private static uint PlayerField(ProductionControlContext context, uint owner, uint offset)
    {
        ReadOnlyMemory<byte> records = context.Players.Records; ulong at = (ulong)owner * 180 + offset;
        if (owner >= 9 || offset > 176 || at > (ulong)records.Length || (ulong)records.Length - at < 4)
            throw new ArgumentOutOfRangeException(nameof(owner), "생산의 실제 현재 Player DWORD 없음");
        return BinaryPrimitives.ReadUInt32LittleEndian(records.Span.Slice((int)at, 4));
    }

    /// <summary>대여 중에는 실제 Session 관문을 통과하고 그 외에는 같은 장기 Player/Deck/Finder로 본체를 실행한다.</summary>
    public static void SynchronizeProductionControlContents(ProductionControlContext context)
    {
        ArgumentNullException.ThrowIfNull(context);
        if (context.Storage is null || context.Gameplay is null || context.Players is null)
            throw new ArgumentException("생산 동기화의 실제 Client 문맥 미연결", nameof(context));
        if (context.Gameplay.Borrowed() || context.Storage.Borrowed()) { context.Gameplay.RequireSession().SynchronizeProductionContents(); return; }
        ClientGameplayGlobals game = context.Gameplay; RawSurfaceStorage storage = context.Storage; RawProductionDeckStorage memory = context.Players.ProductionDeckStorage;
        var carrier = new RawCarrierPreDestroy(storage.Pool, game.ProductionFinder, new()
        {
            ValidateDamageable = (_, _) => { _ = game.RequireSession(); },
            DamageablePre = (_, _) => { _ = game.RequireSession(); },
            DependentFound = () => { _ = game.RequireSession(); },
        });
        var hooks = new ProductionSyncHooks
        {
            Deck = () => memory.Bytes(memory.Get(context.Players.Records.Span, game.Player.TemplateOwner, ClientAllocationBytes.ReadCurrent)),
            Technology = () => game.Player.PlayerKnown != 0 ? PlayerField(context, game.Player.BattleOwner, 0x9c) : 0u,
            Temple = owner => PlayerField(context, owner, 0x7c) != 0,
            CancelCursor = () => game.RequireSession().CancelProductionCursor(),
            Destroy = (sid, flags) => game.RequireSession().DestroyWorldObject(unchecked((int)sid), flags),
            FindKind = (parent, mask, kind, index) =>
            {
                int found = carrier.FindContained(unchecked((int)parent), mask, kind, index);
                return found == 0 ? 0u : BinaryPrimitives.ReadUInt16LittleEndian(storage.Pool.Slot(found)[18..]);
            },
            Create = (parent, kind, quantity, flags, attach, quality) => unchecked((uint)game.RequireSession().WorldContentCreate.Create(unchecked((int)parent), kind, quantity, flags, attach, quality)),
            Assertion = text => Console.Error.WriteLine("생산 동기화 원본 진단: " + text),
        };
        new RawProductionSync(storage.Pool, storage.Types, game.ProductionFinder,
            new() { LocalOwner = () => game.Player.BattleOwner, FullDeck = () => game.FullDeckBox.Value }, hooks).Synchronize();
    }
}
