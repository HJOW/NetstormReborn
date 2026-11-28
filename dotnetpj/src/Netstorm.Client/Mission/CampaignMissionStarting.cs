using System.Buffers.Binary;
using Netstorm.Core.Original;

// 실제 Client 요청과 State7의 정리 전 캡처·공통 확정을 같은 저장소로 연결한다.
namespace Netstorm.Client.Mission;

/// <summary>CampaignClientServices의 미션 시작 부모를 현재 Client 소유자에 결합한다.</summary>
public static class CampaignMissionStarting
{
    /// <summary>현재 실제720BYTE·Player·모드·프레임을 빌린다. 생성은 값·곡·난수를 바꾸지 않는다.</summary>
    public static LocalMissionStateGlobals BindGlobals(ClientSurfaceGlobals surface, Memory<byte> title)
    {
        ArgumentNullException.ThrowIfNull(surface);
        if (title.Length != 80) throw new ArgumentException("실제 HTML 배경80BYTE 필요", nameof(title));
        ClientLocalStateGlobals local = surface.Clock.LocalState;
        ClientGameplayGlobals game = surface.Gameplay;
        if (!ReferenceEquals(local.Gameplay, game) || !ReferenceEquals(surface.Storage.Authority(), game.Authority))
            throw new ArgumentException("미션 상태의 같은 Client 전역 필요", nameof(surface));
        return new()
        {
            Server = () => game.Authority.Server, Boss = () => game.Authority.Boss,
            SetInBattle = value => surface.Storage.OwnerMode.Battle = value,
            MissionPost = () => game.World.MissionPost, SavedGame = () => local.SavedGame.Value,
            SetSavedGame = value => local.SavedGame.Value = value, MissionWorld = () => surface.Storage.Links.Mission,
            Simulconnect = () => local.Simulconnect.Value, Metamap = () => surface.Storage.OwnerMode.Challenge,
            Player = game.Player, ZonePending = local.ZonePending, GameTicks = local.GameTicks,
            SourceType = local.SourceType, CdPresent = local.CdPresent, MenuWindow = local.MenuWindow,
            MetaRefresh = local.MetaRefresh, MetaRebuild = local.MetaRebuild, TitleInitial = title,
            BattleByte = () => local.BattleByte, ConnectEnd = local.ConnectEnd, ActivityEnd = local.ActivityEnd,
            GameSeconds = () => surface.Clock.Frame.Value.Game, WallCached = () => surface.Clock.Frame.Value.Wall,
        };
    }

    /// <summary>유효한 실제 next7만 current로 옮기며 다음 첫DWORD를 지운 뒤 정리 전 지역값을 캡처한다.</summary>
    public static void Prepare(ClientSurfaceGlobals surface, MissionFortLoadRequest request, Memory<byte> title, Action selectDenominator)
    {
        ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(request);
        ArgumentNullException.ThrowIfNull(selectDenominator);
        LocalMissionStateStorage state = surface.Clock.LocalState.State;
        if (surface.Storage.Pool.Edition != OriginalEdition.Patch1078 || request.Consumed || request.Starting is not null
            || request.Zone.Length != 86 || Word(state.Next, 0) != 7 || Word(state.Next, 32) != 1 || Word(request.NextSnapshot, 32) != 1)
            throw new InvalidOperationException("실제 State7 현재 요청·Zone·새 앞단 계약 오류");
        state.Next.Span.CopyTo(state.Current.Span); BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span, 0);
        Capture(surface, request, title, selectDenominator);
    }

    /// <summary>최초 전체 복사 뒤 현재7과 요청 토큰을 재확인하고 분모→예약/제목/시각/출처 순서로 캡처한다.</summary>
    public static void Capture(ClientSurfaceGlobals surface, MissionFortLoadRequest request, Memory<byte> title, Action selectDenominator)
    {
        ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(request);
        ArgumentNullException.ThrowIfNull(selectDenominator);
        ClientLocalStateGlobals local = surface.Clock.LocalState;
        if (surface.Storage.Pool.Edition != OriginalEdition.Patch1078 || request.Consumed || request.Starting is not null
            || request.Zone.Length != 86 || Word(local.State.Current, 0) != 7 || Word(local.State.Current, 32) != 1 || Word(request.NextSnapshot, 32) != 1)
            throw new InvalidOperationException("실제 현재7·Zone·미캡처 앞단 계약 오류");
        selectDenominator();
        request.Starting = RawLocalMissionState.CaptureStarting(local.State.Current, local.ZonePending, title, local.GameTicks, local.SourceType);
    }

    /// <summary>현재 실제 next7의 부모 몸체와 즉시8 접속을 원문144BYTE 공통 확정 아래 실행한다.</summary>
    public static void Run(ClientSurfaceGlobals surface, Memory<byte> title, Action body,
        Action<ReadOnlyMemory<byte>, uint> connect, Action<LocalMissionProgress, int> progress, Action<uint> log)
    {
        ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(body);
        ArgumentNullException.ThrowIfNull(connect); ArgumentNullException.ThrowIfNull(progress); ArgumentNullException.ThrowIfNull(log);
        var parent = new RawLocalMissionState(surface.Storage.Pool.Edition, surface.Clock.LocalState.State, BindGlobals(surface, title), new()
        {
            LogState = log, WallSeconds = surface.WallSeconds, ConnectServer = connect, Progress = progress,
            InvalidState = value => throw new InvalidOperationException($"미션 부모의 선택된 다른 Native 상태 미연결:{value}"),
        });
        parent.PollStarting(body);
    }

    /// <summary>같은 요청 블록의 지정 원문 DWORD를 복사 없이 읽는다.</summary>
    private static uint Word(ReadOnlyMemory<byte> block, int index) => BinaryPrimitives.ReadUInt32LittleEndian(block.Span.Slice(checked(index * 4), 4));
}
