// 미션 시작 흐름의 AI 생성 live 결선을 담은 세션 부분이다.
using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Host;

// 미션 시작의 실제 AI 생성 효과를 세션 소유 메모리·기록에 연결한다.
public sealed partial class HostSession
{
    /// <summary>미션 시작의 실제 AI 생성 효과를 세션 소유 메모리·기록에 연결한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 패치판 188개여야 한다.</param>
    /// <param name="definitions">같은 순서의 실제 자산 정의다. 에너지 조회에 116개가 필요하다.</param>
    /// <param name="initialUpdate">실제 초기 갱신 효과다. null이면 명시 거부한다. 실제 기술·목록 몸체 연결은 다음 단위다.</param>
    /// <param name="destroyTemple">실제 신전 제거 효과다. null이면 명시 거부한다. 월드 삭제 결선은 다음 단위다.</param>
    /// <param name="difficulty">11차 난이도 조회다. null이면 보통으로 둔다. 로컬·동맹 판정은 RawAiCreate가 한다.</param>
    /// <param name="changeNotify">실제 색 변경 뒤의 외부 통지다. null이면 표 쓰기만 한다.</param>
    /// <returns>미션 시작 순서와 같은 실제 생성 효과다. 호출자가 CreateAiBrains에 그대로 넘긴다.</returns>
    /// <remarks>2026-11-19 0.639에서 R4-5 미션 시작 CreateAiBrains live 연결용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 2807~2830행의 createHooks다. 실제 Construct는 세션 소유 AiBrainMemory·Player 기록·섬·동맹·TYPE를 공유하는 RawAiBrainConstructor이며, FirstRegion·IslandTemple·ChangePlayerColor·사제 조회도 세션 버퍼에서 읽는다. InitialUpdate·신전 삭제·난이도 서비스는 아직 실제 몸체가 없어 호출자가 주입한다.</remarks>
    public AiCreateHooks BuildLiveAiCreateHooks(
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<RiftTypeDefinition> definitions,
        Action<uint, ReadOnlyMemory<byte>>? initialUpdate = null,
        Action<uint, uint>? destroyTemple = null,
        Func<uint, ReadOnlyMemory<byte>, uint>? difficulty = null,
        Action<uint, uint>? changeNotify = null)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(definitions);
        // 같은 실제 자산과 현재 TYPE에서 생성자·초기 갱신이 읽는 에너지 문자열을 만든다.
        Func<uint, string> energy = AiTypeRequiredEnergy.BindAiTypeRequiredEnergy(types, definitions);
        // 생성자가 같은 PATCH 풀·Player 9기록을 읽는 최소 부모다. 화신 공장 장치는 아직 잇지 않아 빈 갈고리다.
        var player = new RawPlayerWorld(Pool, new RawPostPop(Pool, types, new RawPostPopState()), PlayerRecords, MissionPlayer, new PlayerWorldHooks());
        // 섬 조회가 같은 세션 기록·모드·목록·해시를 읽는 부모다. 거주 구역·진단은 아직 잇지 않아 명시 거부한다.
        RawIslandRuntime islands = BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => throw new InvalidOperationException("AI 생성의 실제 거주 구역 owner 미연결"),
            Assertion = _ => throw new InvalidOperationException("AI 생성의 실제 섬 진단 미연결"),
        });
        // 같은 세션 AiBrainMemory·Player·섬·동맹·TYPE를 공유하는 실제 생성자다.
        var constructor = new RawAiBrainConstructor(
            AiBrainMemory,
            player,
            new Netstorm.Core.Simulation.PlayerParticipation(PlayerRecords, Alliances, HiddenPlayers, new Netstorm.Core.Simulation.PlayerParticipationState(), new Netstorm.Core.Simulation.PlayerParticipationHooks()),
            islands,
            Alliances,
            types,
            new AiBrainConstructorGlobals
            {
                LocalOwner = () => MissionPlayer.BattleOwner,
                NoRegion = () => TerrainGlobals.NoRegion,
                InvalidGraph = () => TerrainBookkeeping.InvalidGraph,
            },
            new AiBrainConstructorHooks
            {
                Priest = owner => new RawPriestLookup(Pool, PriestState.Priests).Find(owner),
                RegionAt = (x, y) => RegionIslandAt(x, y),
                RequiredEnergy = energy,
            });
        return new AiCreateHooks
        {
            ChangeColor = (owner, color) =>
            {
                // 같은 실제 색 표·섬 받침 프레임을 갱신한다. 빈 풀에서는 섬 객체가 없어 프레임 갈고리에 닿지 않는다.
                RawPlayerColor.ChangePlayerColor(Pool, BridgeState, owner, color, (_, _, _) => throw new InvalidOperationException("AI 생성의 실제 색 프레임 미연결"));
                changeNotify?.Invoke(owner, color);
            },
            FirstRegion = owner =>
            {
                if (owner >= 9)
                {
                    throw new ArgumentOutOfRangeException(nameof(owner), "AI 생성의 실제 첫 지역 owner 범위 오류");
                }

                // 원문 2813행처럼 같은 현재 Player 기록의 +0x78을 매번 다시 읽는다.
                int offset = (int)owner * PlayerRecordStride + PlayerFirstRegionOffset;
                return BinaryPrimitives.ReadUInt32LittleEndian(PlayerRecords.AsSpan(offset, 4));
            },
            IslandTemple = region => islands.Temple(checked((int)region)),
            DestroyTemple = destroyTemple,
            Difficulty = difficulty,
            Construct = (owner, settings, size) => constructor.Construct(owner, settings.Span, size),
            InitialUpdate = initialUpdate is null ? null : (brain, settings) => initialUpdate(brain, settings),
        };
    }

    /// <summary>세션 소유 live 효과로 인공지능 brain을 만들어 같은 Player 기록에 부착한다.</summary>
    /// <param name="players">미션 플레이어 목록이다. 활성 인공지능 소유자만 만든다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="definitions">같은 순서의 실제 자산 정의다.</param>
    /// <param name="initialUpdate">실제 초기 갱신 효과다. null이면 명시 거부한다.</param>
    /// <param name="destroyTemple">실제 신전 제거 효과다. null이면 명시 거부한다.</param>
    /// <param name="difficulty">11차 난이도 조회다. null이면 보통으로 둔다.</param>
    /// <param name="changeNotify">실제 색 변경 뒤의 외부 통지다.</param>
    /// <remarks>2026-11-19 0.639에서 R4-5 미션 시작 live 연결용으로 추가했다. 기존 CreateAiBrains의 호출자 주입 순서를 바꾸지 않고 live 기본값을 묶는다.</remarks>
    public void CreateAiBrainsLive(
        MissionPlayers players,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<RiftTypeDefinition> definitions,
        Action<uint, ReadOnlyMemory<byte>>? initialUpdate = null,
        Action<uint, uint>? destroyTemple = null,
        Func<uint, ReadOnlyMemory<byte>, uint>? difficulty = null,
        Action<uint, uint>? changeNotify = null)
    {
        ArgumentNullException.ThrowIfNull(players);
        CreateAiBrains(players, BuildLiveAiCreateHooks(types, definitions, initialUpdate, destroyTemple, difficulty, changeNotify));
    }
}
