using Netstorm.Core.Original;

// 같은 Host의 실제 수량/난수/장기 역할과 음향/가상삭제/생성/Pop을 결합한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession의5bh 채집 본문 연결이다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 실제 풀/표면/게임난수를 빌리는 채집 본체다.</summary>
    private RawGeyserMined? _geyserMined;
    /// <summary>현재 권한/너겟/진단의 실제 owner를 빌리는 상태다.</summary>
    private GeyserMinedState? _geyserMinedState;
    /// <summary>미설치/반납 뒤에는 실패하는 같은 채집 본체다.</summary>
    public RawGeyserMined GeyserMined => FixedPacketNeed(_geyserMined);
    /// <summary>같은 채집 본체가 빌리는 현재 전역 상태이며 반납 뒤에는 조회할 수 없다.</summary>
    public GeyserMinedState GeyserMinedState => FixedPacketNeed(_geyserMinedState);
    /// <summary>현재 장기 owner만 결합하며 설치 중 난수/수량/음향을 실행하지 않는다.</summary>
    private void BindGeyserMinedDevices()
    {
        if (_geyserMined is not null) return;
        var state = new GeyserMinedState { SharedBoss = () => Gameplay.Authority.Boss, SharedNuggetType = GameAux.NuggetTypeBox }; state.Checking.Bind(() => AiChecking.Read());
        var body = new RawGeyserMined(Pool, FixedPacketNeed(_terrainHash), FixedPacketNeed(_terrainPopTypes), Game, state, new()
        {
            Sound = (x, y, name, flags) => FixedPacketNeed(_squidPacketEffects?.PlayAtFile)(x, y, name, flags),
            Destroy = (sid, flags) => FixedPacketNeed(_squidPacketEffects?.VirtualDestroy)(sid, flags),
            Create = (type, flags) => FixedPacketNeed(_livePlayerHooks?.Create)(type, flags), Pop = PopWorldObject,
        });
        _geyserMinedState = state; _geyserMined = body;
    }
    /// <summary>실제 Kernel 정리가 끝난 뒤 빌린 수신 전용 참조를 반납한다.</summary>
    private void ReleaseGeyserMinedDevices() { _geyserMined = null; _geyserMinedState = null; }
}
