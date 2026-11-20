using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 월드의 실제 생성·공간·프레임·정기·삭제·소리에 플레어를 연결한다.
namespace Netstorm.Client.Host;

/// <summary>연구/업그레이드 효과가 공유하는 실제 플레어 세션 장치다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 생성과 정기가 같은 풀·표·난수를 빌리는 본체다.</summary>
    private RawFlare? _worldFlare;
    /// <summary>후행 사제/장식 등으로 넘길 원래 정기 분배기다.</summary>
    private RegularHandler? _beforeFlareRegular;
    /// <summary>예약 성공 뒤 실제 부착으로 소유권을 넘길 같은 객체다.</summary>
    private RegularProcess? _reservedFlareRegular;
    /// <summary>원본005c85f0의 현재 로그인 종료/효과 억제 시각이다. 로그인/상태 writer와 같은 장기Client 상자를 빌린다.</summary>
    public DoubleBox WorldLoginEnd => LocalState.LoginEnd;
    /// <summary>같은 실제 원문 플레어를 건설/업그레이드가 호출한다.</summary>
    public RawFlare WorldFlare => _worldFlare ?? throw new InvalidOperationException("호스트 플레어 미연결");

    /// <summary>실제 장치만 빌려 현재SHP 헤더·원본역할·로딩깊이·Host 정기 분배와 연결한다.</summary>
    public RawFlare BuildFlareWorldDevices(Action<float, float, string, uint> sound)
    {
        ArgumentNullException.ThrowIfNull(sound);
        if (_worldFlare is not null) throw new InvalidOperationException("호스트 플레어 중복 연결");
        PlayerWorldHooks world = BindLivePlayerWorld();
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes!;
        IReadOnlyList<SquidDisplayShape> shapes = _terrainPopShapes!;
        Action<int, uint> destroy = BuildSessionDestroy("플레어", PlayerProcesses);
        var flare = new RawFlare(Pool, types, WorldFrames, Game, new()
        {
            SuppressedTime = () => WorldLoginEnd.Value, LoadingDepth = () => _worldLoadingDepth,
            Type = () => GameAux.BombFlareTypeBox.Value,
        }, new()
        {
            ProcessCount = () => Kernel.Size(), Create = world.Create,
            SourceHotspot = parent =>
            {
                ReadOnlySpan<byte> raw = Pool.Slot(parent); int frame = BinaryPrimitives.ReadInt32LittleEndian(raw[36..]);
                if (raw[10] >= shapes.Count || frame < 0 || frame >= shapes[raw[10]].Frames.Count)
                    throw new ArgumentOutOfRangeException(nameof(parent), "플레어 원본 기준점 헤더 범위");
                SquidDisplayFrame header = shapes[raw[10]].Frames[frame];
                return (unchecked((short)header.HotspotX), unchecked((short)header.HotspotY));
            },
            Owner = (sid, owner) => _playerOwner!.Set(sid, owner), Pop = world.Pop,
            ReserveRegular = ReserveFlareRegular, AddRegular = AttachFlareRegular,
            Frame = (sid, frame, flags) => _liveFrame!.Set(sid, frame, flags),
            Update = sid => _terrainDisplay!.Update(Pool.Slot(sid)), Destroy = destroy, Sound = sound,
        });
        _beforeFlareRegular = PlayerProcesses.Regular();
        // 기존Host의 처리기null 분기는 현재payload를 그대로 반환한다. 미설치 미션에도 같은 기본 의미를 유지한다.
        RegularHandler previous = _beforeFlareRegular ?? ((_, _, _, payload) => payload);
        PlayerProcesses.SetRegularHandler(flare.Handler(previous)); _worldFlare = flare; return flare;
    }

    /// <summary>원본nothrow 예약과 같이 실제 객체 확보 실패만false로 돌린다.</summary>
    private bool ReserveFlareRegular()
    {
        if (_reservedFlareRegular is not null) throw new InvalidOperationException("플레어 Regular 예약 중첩");
        try { _reservedFlareRegular = new(0, 0); return true; }
        catch (OutOfMemoryException) { return false; }
    }

    /// <summary>같은 예약 객체에 원본 생성자 인자를 쓰고 실제Host에 부착한 뒤 현재시각을 둔다.</summary>
    private void AttachFlareRegular(int sid, uint ev, float payload)
    {
        RegularProcess process = _reservedFlareRegular ?? throw new InvalidOperationException("플레어 Regular 실제 예약 누락");
        _reservedFlareRegular = null; process.InitializeReserved(ev, payload);
        RegularProcess attached = PlayerProcesses.AttachRegular(process, sid, false)
            ?? throw new InvalidOperationException("플레어 Regular 실제 부착 실패");
        attached.SetNextTime(ProcessTime.Value);
    }

    /// <summary>미션 소유 프로세스를 비운 뒤 외부소리보다 먼저 정기 분배와 예약을 반납한다.</summary>
    private void ReleaseFlareWorld()
    {
        if (_worldFlare is null) return;
        PlayerProcesses.SetRegularHandler(_beforeFlareRegular);
        _beforeFlareRegular = null; _reservedFlareRegular = null; _worldFlare = null;
    }
}
