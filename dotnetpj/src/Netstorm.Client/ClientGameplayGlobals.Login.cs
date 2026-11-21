using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 같은 로그인/옵션 소비자가 세션을 넘어 빌리는 실제 Client 저장소다.
namespace Netstorm.Client;

/// <summary>현재87바이트옵션·Core·Player단일할당소유자를장기게임전역에연결한다.</summary>
public sealed partial class ClientGameplayGlobals
{
    /// <summary>기존 장기 소유자와 명시 Subscriber를 연결하며 Player할당·옵션Reset·Core선택을 실행하지 않는다.</summary>
    public ClientGameplayGlobals(ClientSubscriberGlobals? subscribers = null)
    {
        Subscribers = subscribers;
        World = new(subscribers?.MySubscriber, subscribers?.LoginSubscriber);
        PlayerRecords = new(productionDeckStorage: ProductionDeckStorage);
        PlayerRecords.BindCoreData(CoreData);
        // Session 이전부터 같은 현재 checking DWORD를 커서가 직접 빌린다.
        CursorGlobals.Lifecycle.Checking = new(() => AiDebug.Checking.Value);
        CursorGlobals.Placement.SharedScreenOrigin = Camera.Snapshot.Pixels;
    }

    /// <summary>제품 Client가 제공한 장기 구독자 소유자다. 독립 조합은 null이며 기존0/0 월드를 유지한다.</summary>
    public ClientSubscriberGlobals? Subscribers { get; }

    /// <summary>0052f7e0의같은87바이트이며명시Reset/수신writer만값을쓴다.</summary>
    public byte[] BattleOptions { get; } = new byte[87];
    /// <summary>같은옵션+3e의현재Chat숨김9바이트를복사없이빌린다.</summary>
    public Memory<byte> HiddenPlayers => BattleOptions.AsMemory(0x3e, 9);
    /// <summary>현재Core/active/캐시/메타데이터와Player빌린슬롯의실제장기소유자다.</summary>
    public ClientCoreDataGlobals CoreData { get; } = new();
    /// <summary>현재known·전투/Template소유자·Player역할을모든세션이같은주소로빌린다.</summary>
    public PlayerWorldState Player { get; } = new();
    /// <summary>현재Player/factory와같은두로그인카운터·기존Deck저장소를소유한다.</summary>
    public ClientPlayerRecords PlayerRecords { get; }
}
