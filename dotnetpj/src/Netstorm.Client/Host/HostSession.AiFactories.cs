using System.Buffers.Binary;
using Netstorm.Core.Original;

// AI 시설 생산이 실제 Player·내용물·같은 풀을 사용하도록 연결하는 네임스페이스다.
namespace Netstorm.Client.Host;

/// <summary>실제 시설 생산과 미처리·업그레이드 큐를 세션에 연결한다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 본체·풀·TYPE·내용물 상태를 빌리는 AI 시설 본체다.</summary>
    private RawAiFactoryProduction? _aiFactories;

    /// <summary>조립된 시설 본체를 빌려준다. 조립 전에는 명시 오류다.</summary>
    public RawAiFactoryProduction AiFactories => _aiFactories ?? throw new InvalidOperationException("AI 시설 생산 미연결");

    /// <summary>현재 Player 기술 부모·실제 생산 등록·시설 가상 레벨을 연결한다.</summary>
    /// <remarks>근거: cpppj/src/client/RawCampaignSession.cpp의 AiFactoryProductionHooks 조립이다.
    /// 2026-11-20 0.665에서 추가했다. 업그레이드 요청·완료는 후속 실제 본체에 연결한다.</remarks>
    private void BuildAiFactoryDevice(IReadOnlyList<OriginalTypeRecord> types, RawPlayerWorld player, RawAiStrategySetup strategy)
    {
        if (_aiFactories is not null)
        {
            throw new InvalidOperationException("AI 시설 생산 장치 중복 연결");
        }
        ContainedFinderState finder = _aiFinderState ?? throw new InvalidOperationException("AI 시설 내용물 조회 미연결");
        _aiFactories = new RawAiFactoryProduction(AiBrainMemory, Pool, types, finder,
            new AiFactoryProductionGlobals
            {
                Now = () => ProcessTime.Value,
                InvalidGraph = () => TerrainBookkeeping.InvalidGraph,
            },
            new AiFactoryProductionHooks
            {
                Technology = owner =>
                {
                    uint parent = player.Technology(unchecked((int)owner));
                    if (parent > ushort.MaxValue || parent >= (uint)Pool.Capacity)
                    {
                        throw new ArgumentOutOfRangeException(nameof(owner), "AI 시설 생산 현재 기술부모 SID 범위");
                    }
                    return (int)parent;
                },
                Product = strategy.FindProduct,
                AddProduct = strategy.AddProduct,
                Level = parent =>
                {
                    ReadOnlySpan<byte> raw = Pool.Slot(parent);
                    uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
                    if (table != 0x509148u && table != 0x509210u)
                    {
                        throw new InvalidOperationException("AI 시설 가상 level80 미지원 실제 표");
                    }
                    return (uint)(raw[RawSidPool.StateOffset] >> 5);
                },
                Upgrade = parent => WorldFactoryUpgradeCommand.Request(parent),
                Demo = () => _aiDemo() != 0,
                Log = (brain, action, type) => _aiRuntimeLogs.Add(brain + ":factory:" + action + ":" + type),
            });
    }

    /// <summary>현재 owner1..8의 실제 brain에 시설 관리를 실행한다. brain 부재는 효과 없이 거짓이다.</summary>
    /// <remarks>근거: cpppj RawCampaignSession.cpp의 ManageAiFactories다. 전체 매 프레임 순서는 후속에서 연결한다.</remarks>
    public bool ManageAiFactories(uint owner)
    {
        if (owner < 1 || owner > 8)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 시설 생산 소유자는 1..8이어야 합니다.");
        }
        if (!AiBrains.TryGetValue(owner, out uint brain))
        {
            return false;
        }
        AiFactories.Manage(brain);
        return true;
    }
}
