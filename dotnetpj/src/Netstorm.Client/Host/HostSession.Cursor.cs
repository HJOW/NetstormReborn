using Netstorm.Client.Input;
using Netstorm.Core.Original;

using Netstorm.Client.Display;

namespace Netstorm.Client.Host;

// 실제 입력 경로가 같은 세션 풀·TYPE·게임 커서에서 취득·취소·조회·위치를 실행한다.
public sealed partial class HostSession
{
    /// <summary>같은 실제 풀·TYPE·게임 커서로 조립한 취득·취소 본체다. 미연결 hook은 R6에서 잇는다.</summary>
    private RawCursorLifecycle? _cursorLifecycle;

    /// <summary>같은 실제 풀·TYPE·게임 커서로 조립한 내려놓기 조회·준비 본체다. 미연결 hook은 R6에서 잇는다.</summary>
    private RawCursorPlacement? _cursorPlacement;

    /// <summary>실제 입력 경로가 취득·취소를 요청하는 같은 세션 커서 수명이다.</summary>
    public RawCursorLifecycle CursorLifecycle => _cursorLifecycle ?? throw new InvalidOperationException("세션 커서 수명 미연결");

    /// <summary>실제 입력 경로가 조회·준비를 요청하는 같은 세션 커서 배치다.</summary>
    public RawCursorPlacement CursorPlacement => _cursorPlacement ?? throw new InvalidOperationException("세션 커서 배치 미연결");

    /// <summary>도달한 누락 장치를 R6 작업으로 미룬다는 명시 실패다.</summary>
    private static T Missing<T>(string name) => throw new InvalidOperationException(name);

    /// <summary>같은 실제 풀·TYPE·게임 커서 상태로 커서 취득·취소·조회·위치 본체를 연결한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <param name="effects">호출자가 미리 묶은 수명 장치다. 비어 있으면 세션 실제 장치와 R6 명시 거부로 채운다.</param>
    /// <param name="placement">호출자가 미리 묶은 배치 장치다. 비어 있으면 세션 실제 장치와 R6 명시 거부로 채운다.</param>
    /// <remarks>2026-11-26 1.046에서 DI-3 커서 호출자용으로 추가했다. 근거: cpppj RawCampaignCursor.cpp의 HandleInput 분기(왼쪽 Drop·0x1b Cancel)와 Take·GumpsChanged·Update 결선이다. UI·음향·프로세스 하위와 카메라는 R6·DI-4에서 잇는다.</remarks>
    public void BuildCursorDevices(IReadOnlyList<OriginalTypeRecord> types, CursorLifecycleHooks? effects = null, CursorPlacementHooks? placement = null)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (_cursorLifecycle is not null || _cursorPlacement is not null) throw new InvalidOperationException("세션 커서 장치 중복 연결");
        if (_terrainPopTypes is null || !ReferenceEquals(types, _terrainPopTypes)) throw new ArgumentException("세션 커서 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("세션 커서 실제 프로세스 미연결");

        CursorDropState drop = Gameplay.Cursor;
        CursorLifecycleState life = Gameplay.CursorGlobals.Lifecycle;
        CursorPlacementState placing = Gameplay.CursorGlobals.Placement;
        CursorPlacementHooks place = placement?.Copy() ?? new();
        place.Describe ??= (ref string _, float _, float _, uint _, uint _) => Missing<uint>("커서 설명 실제 장치 미연결(R6)");
        place.Walk ??= (_, _) => throw new InvalidOperationException("커서 칸 순회 실제 장치 미연결(R6)");
        place.Image ??= (uint _, int _) => Missing<CursorCostImage>("커서 그림 실제 장치 미연결(R6)");
        place.GumpAt ??= (int _, int _) => Missing<uint>("커서 Gump 조회 실제 장치 미연결(R6)");
        place.Accept ??= (uint _, uint _, uint _, uint _, uint _) => Missing<uint>("커서 Gump 수신 실제 장치 미연결(R6)");
        place.Parent ??= (uint _) => Missing<uint>("커서 Gump 부모 실제 장치 미연결(R6)");
        place.MayPlace ??= (_) => Missing<uint>("커서 배치 판정 실제 장치 미연결(R6)");
        place.Source ??= preview => RawContainedLink.CommonContainedParent(Pool, preview);
        place.Region ??= (int _) => Missing<int>("커서 연결 지역 실제 장치 미연결(R6)");
        place.ProcessEvent ??= (_, _, _, _) => throw new InvalidOperationException("커서 프로세스 통지 실제 장치 미연결(R6)");
        place.Project ??= (float _, float _) => Missing<ScreenPoint>("커서 화면 투영 실제 장치 미연결(DI-4)");
        place.Invalidate ??= (_, _) => throw new InvalidOperationException("커서 화면 dirty 실제 장치 미연결(R6)");
        var placementBody = new RawCursorPlacement(Pool, types, ReadOnlyMemory<ushort>.Empty, drop, life, placing, place);

        CursorLifecycleHooks live = effects?.Copy() ?? new();
        live.Highlight ??= _ => throw new InvalidOperationException("커서 강조 실제 장치 미연결(R6)");
        live.SelectCursor ??= _ => throw new InvalidOperationException("커서 종류 선택 실제 장치 미연결(R6)");
        live.PrepareCursor ??= () => placementBody.Prepare();
        live.ClosePopups ??= () => throw new InvalidOperationException("커서 팝업 정리 실제 장치 미연결(R6)");
        live.CreatePreviews ??= () => throw new InvalidOperationException("커서 미리보기 생성 실제 장치 미연결(R6)");
        live.QueryCursor ??= () => placementBody.Query();
        live.Source ??= preview => RawContainedLink.CommonContainedParent(Pool, preview);
        live.SourceNotice ??= (_, _, _) => throw new InvalidOperationException("커서 연관 통지 실제 장치 미연결(R6)");
        live.Detach ??= (_, _) => throw new InvalidOperationException("커서 연관 해제 실제 장치 미연결(R6)");
        live.Unpop ??= (sid, flags) => UnpopWorldObject(sid, flags);
        live.Destroy ??= (sid, flags) => DestroyWorldObject(sid, flags);
        live.RemoveProcess ??= (form, flags) =>
        {
            SquidProcess? process = processes.FromForm(checked((int)form));
            if (process is null) throw new InvalidOperationException("커서 실제 프로세스 누락");
            processes.Kill(process, flags);
        };
        live.CreateMana ??= (int _, uint _) => Missing<uint>("커서 마나 프로세스 실제 장치 미연결(R6)");
        live.CreateRange ??= (int _, uint _) => Missing<uint>("커서 사정거리 프로세스 실제 장치 미연결(R6)");
        live.ProcessEvent ??= (_, _, _, _) => throw new InvalidOperationException("커서 프로세스 사건 실제 장치 미연결(R6)");
        live.DefaultArgument ??= type => Missing<uint>("커서 기본 인자 실제 장치 미연결(R6)");
        live.TypeNumber ??= type => Missing<uint>("커서 TYPE 번호 실제 장치 미연결(R6)");
        live.HasRange ??= type => RawCursorLifecycle.HasCursorRange(types, type);
        live.Translate ??= value => value;
        live.Measure ??= (string _, ReadOnlyMemory<uint> _) => Missing<(int Width, int Height)>("커서 글자 측정 실제 장치 미연결(R6)");
        live.CostImage ??= (uint _, uint _) => Missing<CursorCostImage>("커서 비용 그림 실제 장치 미연결(R6)");
        live.Mouse ??= () => (Gameplay.Cursor.ScreenX, Gameplay.Cursor.ScreenY);
        live.UpdatePosition ??= (x, y) => { Gameplay.Cursor.ScreenX = x; Gameplay.Cursor.ScreenY = y; };
        live.Sound ??= _ => throw new InvalidOperationException("커서 효과음 실제 장치 미연결(R6)");
        live.AddLocalSp ??= value =>
        {
            uint owner = MissionPlayer.BattleOwner;
            float current = BitConverter.UInt32BitsToSingle(Sp.Get(owner));
            Sp.Set(owner, BitConverter.SingleToUInt32Bits(current - value));
        };
        live.Assertion ??= (expression, file, line) => throw new InvalidOperationException($"커서 원본 진단: {expression} {file}:{line}");
        _cursorPlacement = placementBody;
        _cursorLifecycle = new RawCursorLifecycle(Pool, types, drop, life, live);
    }

    /// <summary>실제 미리보기 SID를 현재 커서에 취득한다. 입력 경로의 Take 호출자다.</summary>
    /// <param name="preview">취득할 미리보기 SID.</param>
    /// <param name="flags">취득 플래그.</param>
    public void TakeCursor(int preview, uint flags) => CursorLifecycle.Take(preview, flags);

    /// <summary>잡은 커서를 실제 놓는다. 입력 경로의 Cancel 호출자다.</summary>
    public void CancelCursor() => CursorLifecycle.Cancel();

    /// <summary>현재 커서 위치의 배치 허용을 다시 조회한다. 입력 경로의 Query 호출자다.</summary>
    public void QueryCursor() => CursorPlacement.Query();

    /// <summary>현재 커서 위치의 dirty 준비를 실행한다. 입력 경로의 Prepare 호출자다.</summary>
    public void PrepareCursor() => CursorPlacement.Prepare();

    /// <summary>잡은 커서가 있으면 놓고 참을 돌려준다. 없으면 거짓이다.</summary>
    public bool TryCancelCursor()
    {
        if (Gameplay.Cursor.LockDepth != 0 || Gameplay.Cursor.Preview == 0) return false;
        CancelCursor();
        return true;
    }
}
