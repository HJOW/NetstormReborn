using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// Native 주소를 호스트 포인터로 바꾸지 않고 실제 Client 필드에 연결한다.
namespace Netstorm.Client;

/// <summary>State 정리가 빌리는 같은 장기 소유자 묶음이며 생성 자체는 값을 초기화하지 않는다.</summary>
public sealed record ClientStateWorldContext
{
    /// <summary>같은 권한·Player·보조 전역·월드 모드의 실제 Client다.</summary>
    public required ClientGameplayGlobals Gameplay { get; init; }
    /// <summary>같은 다섯 상태 블록과 기한을 소유하는 장기 Client다.</summary>
    public required ClientLocalStateGlobals Local { get; init; }
    /// <summary>같은 로컬 상태와 활동22프레임 표를 빌린다.</summary>
    public required ClientConnectionGlobals Connections { get; init; }
    /// <summary>같은 Pool/TYPE/Chunk/모드의 실제 장기 저장소다.</summary>
    public required RawSurfaceStorage Surface { get; init; }
    /// <summary>현재9×20바이트 헤더를 소유하는 실제 Template 저장소다.</summary>
    public required CampaignTemplateStorage Templates { get; init; }
    /// <summary>현재 화면 다시 그리기 DWORD를 소유하는 실제 구름 모듈이다.</summary>
    public required ClientKencloudGlobals Cloud { get; init; }
    /// <summary>도움말과 공유하는 실제 팝업 명령 DWORD다.</summary>
    public required QuickHelpPopupState Popup { get; init; }
    /// <summary>005467b8의 실제 NSInfo zone 소유자다. 호출자는 제품과 같은 상자를 제공한다.</summary>
    public required UIntBox NsInfoZone { get; init; }
    /// <summary>005c89b4의 실제 전체 seed DWORD다. lowWORD 쓰기는 상위 비트를 유지한다.</summary>
    public required UIntBox FortCoreSeed { get; init; }
    /// <summary>00565e68의 실제 장면 음악 기한이다. 직접 쓰기는 재생 효과를 호출하지 않는다.</summary>
    public required DoubleBox SongEnd { get; init; }
}

/// <summary>004b2df0의 폭별 직접 읽기/쓰기만 연결하며 하위 정리 효과는 실행하지 않는다.</summary>
public static class ClientStateWorldGlobals
{
    /// <summary>같은 Client 권한/보조/로컬 상태를 검증하고 현재 조회 대리자를 돌려준다.</summary>
    public static StateWorldCleanupGlobals Bind(ClientStateWorldContext c)
    {
        ArgumentNullException.ThrowIfNull(c);
        ArgumentNullException.ThrowIfNull(c.Gameplay);
        ArgumentNullException.ThrowIfNull(c.Local);
        ArgumentNullException.ThrowIfNull(c.Connections);
        ArgumentNullException.ThrowIfNull(c.Surface);
        ArgumentNullException.ThrowIfNull(c.Templates);
        ArgumentNullException.ThrowIfNull(c.Cloud);
        ArgumentNullException.ThrowIfNull(c.Popup);
        ArgumentNullException.ThrowIfNull(c.NsInfoZone);
        ArgumentNullException.ThrowIfNull(c.FortCoreSeed);
        ArgumentNullException.ThrowIfNull(c.SongEnd);
        if (c.Surface.Pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(c.Surface.Authority(), c.Gameplay.Authority) || !ReferenceEquals(c.Surface.GameAux(), c.Gameplay.GameAux) || !ReferenceEquals(c.Local.Gameplay, c.Gameplay) || !ReferenceEquals(c.Connections.LocalState, c.Local))
            throw new ArgumentException("State 전역의 같은 실제 Client/표면/권한/로컬 owner 필요", nameof(c));
        return new()
        {
            Read32 = address => Read(c, address), Write32 = (address, value) => Write(c, address, value),
            Write16 = (address, value) => WriteWord(c, address, value), Write64Bits = (address, bits) => WriteWide(c, address, bits),
            Players = () => c.Gameplay.PlayerRecords.Records,
            TemplateManager = slot => Manager(c, slot), TypeDefault = type => c.Surface.Types[checked((int)type)]
        };
    }

    /// <summary>정상 bool 값만 쓰기 전에 허용하며 다른 DWORD 비트를 숨기지 않는다.</summary>
    private static bool Boolean(uint value) => value <= 1 ? value != 0 : throw new ArgumentOutOfRangeException(nameof(value), "State bool 전역의 정상0/1 밖 쓰기");
    /// <summary>기존 bool 모델의 현재0/1을 DWORD로 읽는다.</summary>
    private static uint Word(bool value) => value ? 1u : 0u;
    /// <summary>같은22DWORD의 정렬 주소만 활동 표로 해석한다.</summary>
    private static bool Activity(uint address) => address >= 0x5c84e8u && address < 0x5c8540u && (address - 0x5c84e8u) % 4 == 0;
    /// <summary>각 호출에서 같은 현재 실제 필드를 다시 읽는다.</summary>
    private static uint Read(ClientStateWorldContext c, uint address)
    {
        var game = c.Gameplay; var local = c.Local; var aux = game.GameAux;
        if (Activity(address)) return c.Connections.Activity.Frames[(address - 0x5c84e8u) / 4];
        return address switch
        {
            0x5424b0u => c.Surface.Pool.PredictableCursor,
            0x568afcu => BitConverter.SingleToUInt32Bits(aux.CentralPosition[0]),
            0x568b00u => BitConverter.SingleToUInt32Bits(aux.CentralPosition[1]),
            0x540bc0u => Word(game.Authority.Server),
            0x540bc4u => Word(game.Authority.Boss),
            0x540bdcu => unchecked((uint)aux.DefaultSp),
            0x540c70u => game.Player.BattleOwner,
            0x540cacu => game.Player.TemplateOwner,
            0x541240u => aux.GeyserType,
            0x5424bcu => aux.RefundPercent,
            0x5453e4u => unchecked((uint)aux.AllowAnyCapture),
            0x5467b8u => c.NsInfoZone.Value,
            0x5467bcu => aux.SubscriberMarker,
            0x54c8a0u => c.Surface.Chunks.Mode,
            0x54db14u => game.FullDeckBox.Value,
            0x5949d4u => game.MapRevision.Value,
            0x594fa0u => Word(c.Surface.Links.Mission),
            0x594fa4u => Word(game.World.MissionPost),
            0x594fb8u => Word(c.Surface.OwnerMode.Fort),
            0x594fbcu => Word(c.Surface.OwnerMode.Battle),
            0x594fc8u => Word(c.Surface.OwnerMode.Challenge),
            0x595088u => aux.ResourcePoolBits(),
            0x59508cu => unchecked((uint)aux.TotalCost),
            0x595090u => aux.SavedGeyserCount,
            0x5950acu => aux.OrdinaryGeysers,
            0x5950b0u => aux.GeyserFrameFlag,
            0x595124u => aux.IncomePrimed,
            0x595344u => game.Player.PlayerKnown,
            0x59a8acu => game.HideBuildings.Value,
            0x59a8b0u => c.Cloud.WorldRedraw,
            0x5c848cu => BitConverter.SingleToUInt32Bits(aux.LostX),
            0x5c8490u => BitConverter.SingleToUInt32Bits(aux.LostY),
            0x5c85a4u => game.World.Editor,
            0x5c85d4u => Word(local.SavedGame.Value),
            0x5c8758u => BinaryPrimitives.ReadUInt32LittleEndian(local.State.Phase.Span),
            0x5c8bb0u => aux.Archipelago,
            0x5ca8f0u => Word(aux.PriestQuarterHp),
            0x5ca938u => c.Popup.Command,
            0x542534u => local.CachedPeerIndex.Value,
            0x542538u => local.CachedPeerValue.Value,
            0x54253cu => local.CachedPeerFieldA4.Value,
            0x542540u => local.CachedPeerFieldA8.Value,
            0x542544u => local.ScriptBreak.Value,
            0x55a420u => local.SourceType.Value,
            0x5c7a74u => local.Begin.Value,
            0x5c85a0u => local.CentralCreated.Value,
            0x5c85b4u => local.ConnectionNotified.Value,
            0x5c85c0u => local.ZonePending.Value,
            0x5c85c4u => local.MetaConnected.Value,
            0x5c85c8u => local.MetadataMode.Value,
            0x5c85ccu => local.StateAuxCC.Value,
            0x5c85e0u => local.AbortPending.Value,
            _ => throw new ArgumentOutOfRangeException(nameof(address), "State 직접 읽기의 미등록 Native DWORD 주소")
        };
    }
    /// <summary>현재 실제 필드만 직접 쓰고 UI 갱신이나 재초기화 효과를 추가하지 않는다.</summary>
    private static void Write(ClientStateWorldContext c, uint address, uint value)
    {
        var game = c.Gameplay; var local = c.Local; var aux = game.GameAux;
        if (Activity(address)) { c.Connections.Activity.Frames[(address - 0x5c84e8u) / 4] = value; return; }
        switch (address)
        {
            case 0x5424b0u: c.Surface.Pool.PredictableCursorBox.Value = value; return;
            case 0x568afcu: aux.CentralPosition[0] = BitConverter.UInt32BitsToSingle(value); return;
            case 0x568b00u: aux.CentralPosition[1] = BitConverter.UInt32BitsToSingle(value); return;
            case 0x540bc0u: game.Authority.Server = Boolean(value); return;
            case 0x540bc4u: game.Authority.Boss = Boolean(value); return;
            case 0x5424bcu: aux.RefundPercent = value; return;
            case 0x5453e4u: aux.AllowAnyCapture = unchecked((int)value); return;
            case 0x5467b8u: c.NsInfoZone.Value = value; return;
            case 0x5467bcu: aux.SubscriberMarker = value; return;
            case 0x54c8a0u: c.Surface.Chunks.Mode = value; return;
            case 0x54db14u: game.FullDeckBox.Value = value; return;
            case 0x5949d4u: game.MapRevision.Value = value; return;
            case 0x594fa4u: game.World.MissionPost = Boolean(value); return;
            case 0x594fb8u: c.Surface.OwnerMode.Fort = Boolean(value); game.Player.FortMode = c.Surface.OwnerMode.Fort; return;
            case 0x594fbcu: c.Surface.OwnerMode.Battle = Boolean(value); c.Surface.Links.Battle = c.Surface.OwnerMode.Battle; return;
            case 0x594fc8u: c.Surface.OwnerMode.Challenge = Boolean(value); return;
            case 0x595088u: aux.StoreResourcePoolBits(value); return;
            case 0x59508cu: aux.TotalCost = unchecked((int)value); return;
            case 0x595090u: aux.SavedGeyserCount = value; return;
            case 0x5950acu: aux.OrdinaryGeysers = value; return;
            case 0x5950b0u: aux.GeyserFrameFlag = value; return;
            case 0x595124u: aux.IncomePrimed = value; return;
            case 0x595344u: game.Player.PlayerKnown = value; return;
            case 0x59a8acu: game.HideBuildings.Value = value; return;
            case 0x59a8b0u: c.Cloud.WorldRedraw = value; return;
            case 0x5c848cu: aux.LostX = BitConverter.UInt32BitsToSingle(value); return;
            case 0x5c8490u: aux.LostY = BitConverter.UInt32BitsToSingle(value); return;
            case 0x5c85a4u: game.World.Editor = value; return;
            case 0x5c85d4u: local.SavedGame.Value = Boolean(value); return;
            case 0x5c8bb0u: aux.Archipelago = value; return;
            case 0x5ca8f0u: aux.PriestQuarterHp = Boolean(value); return;
            case 0x5ca938u: c.Popup.Command = value; return;
            case 0x542534u: local.CachedPeerIndex.Value = value; return;
            case 0x542538u: local.CachedPeerValue.Value = value; return;
            case 0x54253cu: local.CachedPeerFieldA4.Value = value; return;
            case 0x542540u: local.CachedPeerFieldA8.Value = value; return;
            case 0x542544u: local.ScriptBreak.Value = value; return;
            case 0x55a420u: local.SourceType.Value = value; return;
            case 0x5c7a74u: local.Begin.Value = value; return;
            case 0x5c85a0u: local.CentralCreated.Value = value; return;
            case 0x5c85b4u: local.ConnectionNotified.Value = value; return;
            case 0x5c85c0u: local.ZonePending.Value = value; return;
            case 0x5c85c4u: local.MetaConnected.Value = value; return;
            case 0x5c85c8u: local.MetadataMode.Value = value; return;
            case 0x5c85ccu: local.StateAuxCC.Value = value; return;
            case 0x5c85e0u: local.AbortPending.Value = value; return;
            default: throw new ArgumentOutOfRangeException(nameof(address), "State 직접 쓰기의 미등록 Native DWORD 주소");
        }
    }
    /// <summary>seed의 현재 상위WORD를 유지하고 원문 lowWORD만 바꾼다.</summary>
    private static void WriteWord(ClientStateWorldContext c, uint address, ushort value)
    {
        if (address != 0x5c89b4u) throw new ArgumentOutOfRangeException(nameof(address), "State 직접 쓰기의 미등록 Native WORD 주소");
        c.FortCoreSeed.Value = (c.FortCoreSeed.Value & 0xffff0000u) | value;
    }
    /// <summary>원문의 아홉 double 위치에 같은64비트 패턴을 직접 쓴다.</summary>
    private static void WriteWide(ClientStateWorldContext c, uint address, ulong bits)
    {
        var local = c.Local; var aux = c.Gameplay.GameAux; double value = BitConverter.UInt64BitsToDouble(bits);
        switch (address)
        {
            case 0x565e68u: c.SongEnd.Value = value; return;
            case 0x5950b8u: aux.GeyserDeadline = value; return;
            case 0x5950c0u: aux.CleanupDeadline = value; return;
            case 0x5950c8u: aux.SecondaryGeyserDeadline = value; return;
            case 0x595358u: aux.ScheduledDrawDeadline = value; return;
            case 0x5c85f0u: local.LoginEnd.Value = value; return;
            case 0x5c85f8u: local.ConnectEnd.Value = value; return;
            case 0x5c8610u: local.ReconnectSaveEnd.Value = value; return;
            case 0x5c8618u: local.CaptureEnd.Value = value; return;
            default: throw new ArgumentOutOfRangeException(nameof(address), "State 직접 쓰기의 미등록 Native double 주소");
        }
    }
    /// <summary>현재 헤더의 첫DWORD만 읽고 슬롯9 이상은 물리 접근 전에 거부한다.</summary>
    private static uint Manager(ClientStateWorldContext c, uint slot)
    {
        if (slot >= 9) throw new ArgumentOutOfRangeException(nameof(slot), "State Template9의 슬롯 범위");
        return BinaryPrimitives.ReadUInt32LittleEndian(c.Templates.Headers.Span.Slice((int)slot * 20, 4));
    }
}
