namespace Netstorm.Client.Host;

// 장기자산표의실제설치를첫메뉴앞한번으로제한한다.
/// <summary>메뉴/State/Session이같은TYPE/Frame/SHP소유자를공유한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>장면/Session교체에서다시읽거나다시구성하지않는실제자산소유자다.</summary>
    private HostAssetTables? _assetTables;
    /// <summary>이미준비된실제자산소유자를조회하며미설치시명시실패한다.</summary>
    public HostAssetTables AssetTables => _assetTables ?? throw new InvalidOperationException("실제장기자산표미설치");
    /// <summary>원문init types위치에서같은장기표를한번준비한다.</summary>
    private void PrepareNativeAssetTables()
    {
        if (_assetTables is not null) return;
        _assetTables = new HostAssetTables(_startup ?? throw new InvalidOperationException("자산초기화의실제시작자료미연결"), _surfaceGlobals.Storage);
    }
}
