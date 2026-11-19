// 미션 시작 흐름의 AI 생성 live 결선을 담은 세션 부분이다.
using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Host;

// 미션 시작의 실제 AI 생성 효과를 세션 소유 메모리·기록에 연결한다.
public sealed partial class HostSession
{
    /// <summary>PATCH Player 슬롯 안 소유자 바이트 위치다. 최소 live 화신의 소유자 기록에 쓴다.</summary>
    /// <remarks>2026-11-19 0.640에서 R4-5 미션 시작 InitialUpdate live 연결용으로 추가했다. 근거: RawPlayerWorld 60행의 Patch OwnerOffset이다.</remarks>
    private const int LiveAiPlayerOwnerOffset = 34;

    /// <summary>AI 초기 갱신의 theme0/1/2/3 워크샾 4개다. 분류 후보 교체 때 같은 순서로 읽는다.</summary>
    /// <remarks>2026-11-19 0.640에서 R4-5 미션 시작 InitialUpdate live 연결용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 563행의 aiWorkshops다.</remarks>
    private static readonly uint[] LiveAiWorkshops = [86u, 87u, 88u, 85u];

    /// <summary>AI 초기 갱신이 분류에서 제외하는 sunWalker 타입이다.</summary>
    /// <remarks>2026-11-19 0.640에서 R4-5 미션 시작 InitialUpdate live 연결용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 564행의 aiExcludedType이다.</remarks>
    private const uint LiveAiExcludedType = 133u;

    /// <summary>미션 시작의 실제 AI 생성 효과를 세션 소유 메모리·기록에 연결한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 패치판 188개여야 한다.</param>
    /// <param name="definitions">같은 순서의 실제 자산 정의다. 에너지 조회에 116개가 필요하다.</param>
    /// <param name="initialUpdate">실제 초기 갱신 효과다. null이면 실제 기술·목록 몸체로 잇는다. null인데 실제 표 조회도 없으면 명시 거부한다.</param>
    /// <param name="destroyTemple">실제 신전 제거 효과다. null이면 명시 거부한다. 월드 삭제 결선은 다음 단위다.</param>
    /// <param name="difficulty">11차 난이도 조회다. null이면 보통으로 둔다. 로컬·동맹 판정은 RawAiCreate가 한다.</param>
    /// <param name="changeNotify">실제 색 변경 뒤의 외부 통지다. null이면 표 쓰기만 한다.</param>
    /// <param name="typeTable">실제 기술 이름 조회용 표다. null이면 live 초기 갱신을 만들지 않고 명시 거부한다. 주입한 초기 갱신을 쓸 때는 필요 없다.</param>
    /// <param name="player">실제 화신 부모다. null이면 세션 풀에 최소 live 부모를 둔다. 흐름에서는 연결된 live 부모를 넘긴다.</param>
    /// <param name="contained">실제 내용물 생성이다. null이면 세션 풀에 최소 live 생성을 둔다. 흐름에서는 요새 내용물 생성을 넘긴다.</param>
    /// <param name="finderState">실제 내용물 조회 전역이다. null이면 새로 둔다. 호출자가 소유하며 세션보다 길게 둔다.</param>
    /// <param name="defaultName">현재 언어의 기본 AI 이름이다. null이면 미션 이름이 비었을 때 명시 거부한다. 이름 있는 미션에서는 호출하지 않는다.</param>
    /// <returns>미션 시작 순서와 같은 실제 생성 효과다. 호출자가 CreateAiBrains에 그대로 넘긴다.</returns>
    /// <remarks>2026-11-19 0.639에서 R4-5 미션 시작 CreateAiBrains live 연결용으로 추가했고 0.640에서 실제 초기 갱신 live를 이었다. 근거: cpppj src/client/RawCampaignSession.cpp 2807~2830행의 createHooks와 2805~2806행의 초기 갱신 조립이다. 실제 Construct는 세션 소유 AiBrainMemory·Player 기록·섬·동맹·TYPE를 공유하는 RawAiBrainConstructor이며, FirstRegion·IslandTemple·ChangePlayerColor·사제 조회도 세션 버퍼에서 읽는다. 초기 갱신은 같은 세션 풀·기록·TYPE의 실제 기술·목록 몸체를 공유한다.</remarks>
    public AiCreateHooks BuildLiveAiCreateHooks(
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<RiftTypeDefinition> definitions,
        Action<uint, ReadOnlyMemory<byte>>? initialUpdate = null,
        Action<uint, uint>? destroyTemple = null,
        Func<uint, ReadOnlyMemory<byte>, uint>? difficulty = null,
        Action<uint, uint>? changeNotify = null,
        RiftTypeTable? typeTable = null,
        RawPlayerWorld? player = null,
        RawContainedCreate? contained = null,
        ContainedFinderState? finderState = null,
        Func<string>? defaultName = null)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(definitions);
        // 같은 실제 자산과 현재 TYPE에서 생성자·초기 갱신이 읽는 에너지 문자열을 만든다.
        Func<uint, string> energy = AiTypeRequiredEnergy.BindAiTypeRequiredEnergy(types, definitions);
        // 생성자가 같은 PATCH 풀·Player 9기록을 읽는 최소 부모다. 화신 공장 장치는 아직 잇지 않아 빈 갈고리다.
        var createPlayer = new RawPlayerWorld(Pool, new RawPostPop(Pool, types, new RawPostPopState()), PlayerRecords, MissionPlayer, new PlayerWorldHooks());
        // 섬 조회가 같은 세션 기록·모드·목록·해시를 읽는 부모다. 거주 구역·진단은 아직 잇지 않아 명시 거부한다.
        RawIslandRuntime islands = BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => throw new InvalidOperationException("AI 생성의 실제 거주 구역 owner 미연결"),
            Assertion = _ => throw new InvalidOperationException("AI 생성의 실제 섬 진단 미연결"),
        });
        // 같은 세션 AiBrainMemory·Player·섬·동맹·TYPE를 공유하는 실제 생성자다.
        var constructor = new RawAiBrainConstructor(
            AiBrainMemory,
            createPlayer,
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
            InitialUpdate = ResolveLiveInitialUpdate(types, energy, typeTable, player, contained, finderState, defaultName, initialUpdate),
        };
    }

    /// <summary>주입한 초기 갱신이 있으면 그대로 쓰고, 없으면 실제 기술·목록 몸체로 잇는다.</summary>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="energy">같은 실제 자산의 요구 에너지 조회다.</param>
    /// <param name="typeTable">실제 기술 이름 조회용 표다. null이면 live를 만들지 않고 null을 돌려준다.</param>
    /// <param name="player">실제 화신 부모다. null이면 최소 live 부모를 둔다.</param>
    /// <param name="contained">실제 내용물 생성이다. null이면 최소 live 생성을 둔다.</param>
    /// <param name="finderState">실제 내용물 조회 전역이다. null이면 새로 둔다.</param>
    /// <param name="defaultName">현재 언어의 기본 AI 이름이다. null이면 이름이 비었을 때 거부한다.</param>
    /// <param name="initialUpdate">주입한 초기 갱신 효과다. null이면 live를 만든다.</param>
    /// <returns>쓸 초기 갱신 효과다. 둘 다 없으면 null이며 RawAiCreate의 Need가 생성 전에 명시 거부한다.</returns>
    /// <remarks>2026-11-19 0.640에서 R4-5 미션 시작 InitialUpdate live 연결용으로 추가했다. live 초기 갱신은 같은 세션 풀·기록·TYPE의 실제 기술·목록 몸체를 공유한다.</remarks>
    private Action<uint, ReadOnlyMemory<byte>>? ResolveLiveInitialUpdate(
        IReadOnlyList<OriginalTypeRecord> types,
        Func<uint, string> energy,
        RiftTypeTable? typeTable,
        RawPlayerWorld? player,
        RawContainedCreate? contained,
        ContainedFinderState? finderState,
        Func<string>? defaultName,
        Action<uint, ReadOnlyMemory<byte>>? initialUpdate)
    {
        if (initialUpdate is not null)
        {
            return (brain, settings) => initialUpdate(brain, settings);
        }

        if (typeTable is null)
        {
            return null;
        }

        // 같은 세션 풀에 두는 최소 live 공장으로 화신 생성과 내용물 생성을 함께 잇는다.
        var factory = new RawSquidFactory(Pool, types);
        RawPlayerWorld livePlayer = player ?? BuildMinimalAiPlayer(factory, types);
        RawContainedCreate liveContained = contained ?? BuildMinimalAiContained(factory);
        ContainedFinderState liveFinder = finderState ?? new ContainedFinderState();
        // 피해·종속 갈고리는 초기 갱신 경로(조회·생성)에서 도달하지 않으므로 명시 거부한다.
        var carrier = new RawCarrierPreDestroy(Pool, liveFinder, new CarrierPreDestroyHooks
        {
            ValidateDamageable = (_, _) => throw new InvalidOperationException("AI 초기의 실제 피해 검증 미연결(R6)"),
            DamageablePre = (_, _) => throw new InvalidOperationException("AI 초기의 실제 피해 후처리 미연결(R6)"),
            DependentFound = () => throw new InvalidOperationException("AI 초기의 실제 종속 통지 미연결(R6)"),
        });
        // 실제 표 조회로 기술 목록을 잇는다. 진단은 보고 뒤 계속하며 삭제는 초기 갱신에서 도달하지 않는다.
        var technology = new RawPlayerTechnology(livePlayer, carrier, liveContained, types,
            Netstorm.Core.Original.RawPlayerTechnology.BindPlayerTechnologyTypes(typeTable, new Netstorm.Core.Original.PlayerTechnologyHooks
            {
                Assertion = _ => throw new InvalidOperationException("AI 초기의 실제 기술 진단 미연결"),
                Destroy = (_, _) => throw new InvalidOperationException("AI 초기의 실제 기술 삭제 미연결(R6)"),
            }));
        var initial = new RawAiBrainInitialUpdate(
            AiBrainMemory,
            livePlayer,
            technology,
            liveFinder,
            types,
            new AiBrainInitialUpdateGlobals
            {
                MissionMode = () => WorldGlobals.MissionPost ? 1u : 0u,
                ExcludedType = () => LiveAiExcludedType,
                Workshops = LiveAiWorkshops,
            },
            new AiBrainInitialUpdateHooks
            {
                DefaultName = defaultName,
                RequiredEnergy = energy,
            });
        return (brain, settings) => initial.Update(brain, settings.Span);
    }

    /// <summary>같은 세션 풀에 화신을 만들고 기술 부모를 기록하는 최소 live 부모를 둔다.</summary>
    /// <param name="factory">같은 세션 풀의 실제 공장이다. 호출자가 소유하며 세션보다 길게 둔다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <returns>같은 Player 9기록을 읽고 쓰는 최소 부모다.</returns>
    /// <remarks>2026-11-19 0.640에서 R4-5 미션 시작 InitialUpdate live 연결용으로 추가했다. 빈 풀에서는 같은 좌표의 실제 화신이 없으므로 찾기가 0을 돌려주는 것이 실제 결과다. 흐름에서는 연결된 live 부모를 쓰므로 공간 Pop·예약 갈고리는 명시 거부한다.</remarks>
    private RawPlayerWorld BuildMinimalAiPlayer(RawSquidFactory factory, IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(factory);
        ArgumentNullException.ThrowIfNull(types);
        if (!ReferenceEquals(factory.Pool, Pool))
        {
            throw new ArgumentException("AI 최소 화신의 실제 공장 풀 불일치", nameof(factory));
        }

        return new RawPlayerWorld(Pool, new RawPostPop(Pool, types, new RawPostPopState()), PlayerRecords, MissionPlayer, new PlayerWorldHooks
        {
            Assertion = _ => throw new InvalidOperationException("AI 최소 화신의 실제 진단 미연결"),
            Find = (_, _, _) => 0,
            Create = (type, flags) => factory.Create(type, (SidAllocation)flags),
            SetOwner = (sid, owner) => Pool.AllocatedBytes(sid)[LiveAiPlayerOwnerOffset] = unchecked((byte)owner),
            Pop = (sid, _, _, _) =>
            {
                // 실제 기술 부모 SID를 같은 Player 기록 +0x9c에 둔다. 공간 등록·표시는 흐름의 live 부모에서 잇는다.
                uint owner = Pool.AllocatedBytes(sid)[LiveAiPlayerOwnerOffset];
                BinaryPrimitives.WriteUInt32LittleEndian(PlayerRecords.AsSpan((int)owner * PlayerRecordStride + 0x9c, 4), unchecked((uint)sid));
            },
            ReserveRegular = () => throw new InvalidOperationException("AI 최소 화신의 실제 Regular 예약 미연결(R6)"),
            AddRegular = (_, _, _) => throw new InvalidOperationException("AI 최소 화신의 실제 Regular 부착 미연결(R6)"),
        });
    }

    /// <summary>같은 세션 풀에 내용물을 만들고 부모 체인에 붙이는 최소 live 생성을 둔다.</summary>
    /// <param name="factory">같은 세션 풀의 실제 공장이다. 호출자가 소유하며 세션보다 길게 둔다.</param>
    /// <returns>같은 풀의 실제 내용물 생성이다.</returns>
    /// <remarks>2026-11-19 0.640에서 R4-5 미션 시작 InitialUpdate live 연결용으로 추가했다. 기술 목록은 내용물 6번만 만들므로 표면 통지에 닿지 않는다. 흐름에서는 요새 내용물 생성을 쓰므로 부착 갈고리는 흐름의 live 몸체를 따른다.</remarks>
    private RawContainedCreate BuildMinimalAiContained(RawSquidFactory factory)
    {
        ArgumentNullException.ThrowIfNull(factory);
        if (!ReferenceEquals(factory.Pool, Pool))
        {
            throw new ArgumentException("AI 최소 내용물의 실제 공장 풀 불일치", nameof(factory));
        }

        var hooks = new ContainedCreateHooks
        {
            NotifySurface = _ => throw new InvalidOperationException("AI 최소 내용물의 실제 표면 통지 미연결(R6)"),
            Attach = (form, parent, _) =>
            {
                // 다음 링크(+4)에 부모 head(+6 현재값)를 먼저 저장한 뒤 부모 head를 갱신한다. 원문 순서와 같다.
                Span<byte> formRaw = Pool.AllocatedBytes(form);
                ReadOnlySpan<byte> parentRaw = Pool.Slot(parent);
                BinaryPrimitives.WriteUInt16LittleEndian(formRaw.Slice(4, 2), BinaryPrimitives.ReadUInt16LittleEndian(parentRaw.Slice(6, 2)));
                BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(parent).Slice(6, 2), unchecked((ushort)form));
            },
        };
        Netstorm.Core.Original.RawContainedCreate.ConnectFactory(Pool, factory, hooks);
        return new RawContainedCreate(Pool, new ContainedCreateState(), hooks);
    }

    /// <summary>세션 소유 live 효과로 인공지능 brain을 만들어 같은 Player 기록에 부착한다.</summary>
    /// <param name="players">미션 플레이어 목록이다. 활성 인공지능 소유자만 만든다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="definitions">같은 순서의 실제 자산 정의다.</param>
    /// <param name="initialUpdate">실제 초기 갱신 효과다. null이면 실제 기술·목록 몸체로 잇는다.</param>
    /// <param name="destroyTemple">실제 신전 제거 효과다. null이면 명시 거부한다.</param>
    /// <param name="difficulty">11차 난이도 조회다. null이면 보통으로 둔다.</param>
    /// <param name="changeNotify">실제 색 변경 뒤의 외부 통지다.</param>
    /// <param name="typeTable">실제 기술 이름 조회용 표다. live 초기 갱신에 필요하며 주입 갱신을 쓸 때는 필요 없다.</param>
    /// <param name="player">실제 화신 부모다. null이면 최소 live 부모를 둔다.</param>
    /// <param name="contained">실제 내용물 생성이다. null이면 최소 live 생성을 둔다.</param>
    /// <param name="finderState">실제 내용물 조회 전역이다. null이면 새로 둔다.</param>
    /// <param name="defaultName">현재 언어의 기본 AI 이름이다.</param>
    /// <remarks>2026-11-19 0.639에서 R4-5 미션 시작 live 연결용으로 추가했고 0.640에서 live 초기 갱신 입력을 이었다. 기존 CreateAiBrains의 호출자 주입 순서를 바꾸지 않고 live 기본값을 묶는다.</remarks>
    public void CreateAiBrainsLive(
        MissionPlayers players,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<RiftTypeDefinition> definitions,
        Action<uint, ReadOnlyMemory<byte>>? initialUpdate = null,
        Action<uint, uint>? destroyTemple = null,
        Func<uint, ReadOnlyMemory<byte>, uint>? difficulty = null,
        Action<uint, uint>? changeNotify = null,
        RiftTypeTable? typeTable = null,
        RawPlayerWorld? player = null,
        RawContainedCreate? contained = null,
        ContainedFinderState? finderState = null,
        Func<string>? defaultName = null)
    {
        ArgumentNullException.ThrowIfNull(players);
        CreateAiBrains(players, BuildLiveAiCreateHooks(types, definitions, initialUpdate, destroyTemple, difficulty, changeNotify, typeTable, player, contained, finderState, defaultName));
    }

    /// <summary>미션 시작 순서와 같이 사제가 있는 인공지능 소유자만 실제 brain을 만든다. 원문 Start의 AI 생성 루프와 같다.</summary>
    /// <param name="players">미션 플레이어 목록이다. 활성 인공지능 소유자만 대상으로 한다.</param>
    /// <param name="table">실제 기술 이름 조회용 표다.</param>
    /// <param name="definitions">같은 순서의 실제 자산 정의다.</param>
    /// <param name="defaultName">현재 언어의 기본 AI 이름이다.</param>
    /// <param name="destroyTemple">실제 신전 제거 효과다. null이면 NoTemple 소유자에서 명시 거부한다. 출하 미션에는 NoTemple 인공지능이 없다.</param>
    /// <param name="difficulty">11차 난이도 조회다. null이면 보통으로 둔다.</param>
    /// <param name="changeNotify">실제 색 변경 뒤의 외부 통지다.</param>
    /// <remarks>2026-11-19 0.640에서 R4-5 미션 시작 흐름 결선용으로 추가했다. 근거: RawMissionStart 163~165행의 생성 루프다. 로컬 소유자는 데모가 아니면 제외하며, 사제가 없는 인공지능은 만들지 않는다. 요새 장치가 연결됐으면 live 화신·내용물 몸체를 쓰고, 아니면 최소 live 몸체로 잇는다.</remarks>
    public void CreateMissionAiBrains(
        MissionPlayers players,
        RiftTypeTable table,
        IReadOnlyList<RiftTypeDefinition> definitions,
        Func<string> defaultName,
        Action<uint, uint>? destroyTemple = null,
        Func<uint, ReadOnlyMemory<byte>, uint>? difficulty = null,
        Action<uint, uint>? changeNotify = null)
    {
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(table);
        ArgumentNullException.ThrowIfNull(definitions);
        ArgumentNullException.ThrowIfNull(defaultName);
        AiCreateHooks hooks = BuildLiveAiCreateHooks(
            table.Types, definitions,
            typeTable: table,
            player: _livePlayerWorld,
            contained: _fortContained,
            defaultName: defaultName,
            destroyTemple: destroyTemple,
            difficulty: difficulty,
            changeNotify: changeNotify);
        var participation = new Netstorm.Core.Simulation.PlayerParticipation(PlayerRecords, Alliances, HiddenPlayers, new Netstorm.Core.Simulation.PlayerParticipationState(), new Netstorm.Core.Simulation.PlayerParticipationHooks());
        var create = new RawAiCreate(OriginalEdition.Patch1078, participation, hooks);
        var priestLookup = new RawPriestLookup(Pool, PriestState.Priests);
        // 활성 인공지능 소유자를 번호 순서대로 방문한다. 원문 Start 루프와 같은 순서다.
        for (uint owner = 1; owner <= (uint)MissionPlayers.PlayerCount; owner++)
        {
            if (!players.Players[(int)owner].Active || !players.Players[(int)owner].Computer)
            {
                continue;
            }

            if (owner == MissionPlayer.BattleOwner)
            {
                continue;
            }

            if (priestLookup.Find(owner) == 0)
            {
                continue;
            }

            if (!AiSettings.TryGetValue((int)owner, out byte[]? settings))
            {
                throw new InvalidOperationException("인공지능 설정 없이 생성할 수 없다: " + owner);
            }

            AiBrains[owner] = create.Create(owner, settings);
        }
    }
}
