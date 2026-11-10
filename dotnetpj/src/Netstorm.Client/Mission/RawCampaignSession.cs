// 실제 캠페인 초기화/전투/이동의 상태와 모듈 수명을 클라이언트 세션 하나에 보관하는 껍데기를 공유한다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Mission;

// 캠페인 세션 모듈을 담는 네임스페이스다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.

// 초기 객체 생성부터 Kernel 해제까지 동일 주소에서 실제 모듈과 상태를 소유하는 세션 껍데기다. Fort 적재·AI·메뉴·연결 본체 조립은 후속 단위에서 잇는다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.
public sealed class RawCampaignSession
{
    /// <summary>월드가 따르는 원본 판본이다. 슬롯 오프셋이 다른 모듈이 이 값으로 갈라진다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private readonly OriginalEdition _edition;

    /// <summary>호출자가 소유하는 같은 전역 TYPE 표다. 월드·저장소와 같은 수명으로 둔다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>호출자가 소유하는 같은 장기 프레임 표다. 월드와 같은 수명으로 둔다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>호출자가 소유하는 같은 셰이프 표다. 표시 수집과 같은 수명으로 둔다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<SquidDisplayShape> _shapes;

    /// <summary>같은 물리 저장소다. 없으면 이 세션이 새로 만든다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private readonly RawSurfaceStorage _storage;

    /// <summary>같은 풀·공간 지도에서 표면 객체를 관리하는 실제 월드다. 세션보다 오래 살 수 없다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private readonly RawSurfaceWorld _world;

    /// <summary>세션이 보관하는 요새 이름이다. PATCH10.78을 쓴다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private readonly string _fortName;

    /// <summary>세션이 보관하는 미션 이름이다. 빈 값이면 편집기 진입이다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private readonly string _mission;

    /// <summary>Fort 적재가 끝났는지다. 껍데기에서는 놓지 않는다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private bool _loaded;

    /// <summary>로컬 연결의 현재 단계다. 사전 구현에서는 항상 0(미접속)이다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐다.</summary>
    private uint _localPhase = 0u;

    /// <summary>수신한 로컬 패킷 목록이다. 사전 구현에서는 항상 비어 있다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐다.</summary>
    private readonly List<byte[]> _localPackets = new();

    /// <summary>같은 자산·저장소·월드로 세션 껍데기를 만든다. Fort 적재 본체는 후속 단위에서 잇는다.</summary>
    /// <param name="edition">월드가 따르는 원본 판본이다.</param>
    /// <param name="types">같은 전역 TYPE 표다. 비어 있으면 안 된다.</param>
    /// <param name="frames">같은 장기 프레임 표다. TYPE와 개수가 같아야 한다.</param>
    /// <param name="shapes">같은 셰이프 표다. TYPE와 개수가 같아야 한다.</param>
    /// <param name="fortName">보관할 요새 이름이다. 비어 있으면 안 된다.</param>
    /// <param name="mission">보관할 미션 이름이다. null이면 안 되며 빈 값은 편집기 진입이다.</param>
    /// <param name="storage">빌릴 물리 저장소다. null이면 같은 자산으로 새로 만든다.</param>
    public RawCampaignSession(
        OriginalEdition edition,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<RiftTypeFrames> frames,
        IReadOnlyList<SquidDisplayShape> shapes,
        string fortName,
        string mission,
        RawSurfaceStorage? storage = null)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(shapes);
        ArgumentNullException.ThrowIfNull(fortName);
        ArgumentNullException.ThrowIfNull(mission);
        if (types.Count == 0)
        {
            throw new ArgumentException("캠페인 세션의 실제 TYPE 표가 비었습니다.", nameof(types));
        }

        if (frames.Count != types.Count)
        {
            throw new ArgumentException("캠페인 세션의 실제 프레임 표 크기 불일치", nameof(frames));
        }

        if (shapes.Count != types.Count)
        {
            throw new ArgumentException("캠페인 세션의 실제 셰이프 표 크기 불일치", nameof(shapes));
        }

        if (fortName.Length == 0)
        {
            throw new ArgumentException("캠페인 세션의 실제 요새 이름이 비었습니다.", nameof(fortName));
        }

        _edition = edition;
        _types = types;
        _frames = frames;
        _shapes = shapes;
        _fortName = fortName;
        _mission = mission;
        _storage = storage ?? new RawSurfaceStorage(edition, types, frames, shapes);
        // 원본 세션이 같은 지면 종류로 월드를 여는 모양을 직역한다. 실제 지형 종류는 저장소 링크의 섬 받침 종류를 쓴다.
        _world = new RawSurfaceWorld(edition, types, _storage.Links.IslandType, _storage, frames);
        _loaded = false;
    }

    /// <summary>세션이 보관하는 요새 이름이다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    /// <returns>요새 이름이다.</returns>
    public string FortName() => _fortName;

    /// <summary>세션이 보관하는 미션 이름이다. 빈 값이면 편집기 진입이다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    /// <returns>미션 이름이다.</returns>
    public string MissionName() => _mission;

    /// <summary>빌리거나 새로 만든 같은 물리 저장소다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    /// <returns>물리 저장소다.</returns>
    public RawSurfaceStorage Storage() => _storage;

    /// <summary>같은 풀·공간 지도에서 표면 객체를 관리하는 실제 월드다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    /// <returns>세션 소유 월드다.</returns>
    public RawSurfaceWorld World() => _world;

    /// <summary>같은 풀·공간 지도에서 실제 미션 모듈이 쓰는 현재 참조 묶음을 빌린다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    /// <returns>월드와 같은 수명의 실제 월드 자료 보기다.</returns>
    public RawWorldCore Core() => _world.Core();

    /// <summary>Fort 적재가 끝났는지다. 씨앗 적재 뒤에는 참이다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐고 2026-11-10 F3 Fort 씨앗 적재 이식에서 갱신했다.</summary>
    /// <returns>적재됐으면 참이다.</returns>
    public bool IsLoaded() => _loaded;

    /// <summary>실제 부모 설치를 한 번 실행한다. 껍데기에서는 Fort 적재 미연결로 멈춘다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    public void Load()
    {
        // 원본 RawCampaignSession::Load의 Fort 적재 본체는 후속 단위에서 잇는다(결정 6 — 성공을 합성하지 않는다).
        throw new InvalidOperationException("실제 미션 Fort 적재 미연결");
    }

    /// <summary>Fort 씨앗 목록을 같은 월드에 적재하고 세션을 적재됨으로 표시한다. 원본 Fort 순서의 지면·무인도·다리 씨앗 경로만 둔다. 2026-11-10 F3 Fort 씨앗 적재 이식에서 추가됐다.</summary>
    /// <param name="seeds">초기 위치 목록이다.</param>
    public void LoadSeeds(IReadOnlyList<SurfaceSeed> seeds)
    {
        ArgumentNullException.ThrowIfNull(seeds);
        if (_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 Fort 중복 적재입니다.");
        }

        // 원본 표면 월드 적재 순서·검증을 그대로 빌리며 성공 뒤에만 세션을 적재됨으로 표시한다.
        _world.Load(seeds);
        _loaded = true;
    }

    /// <summary>현재 공급된 프레임 시각을 돌려준다. 원본에 없는 C# 조회이며 검사·소비자 연결용이다. 2026-11-10 F3 세션 프레임 위임 이식에서 추가됐다.</summary>
    /// <returns>현재 프레임 시각이다.</returns>
    public FrameTime CurrentTime() => _world.CurrentTime();

    /// <summary>같은 시각을 월드에 공급한다. 적재 전에는 거부한다. 원본 세션 준비의 월드 시각 대입만 둔다. 2026-11-10 F3 세션 프레임 위임 이식에서 추가됐다.</summary>
    /// <param name="time">공급할 프레임 시각이다.</param>
    /// <param name="paused">정지 중이면 참이다. 월드 시각 공급에는 쓰지 않으며 AI·입자 연결 때 쓴다.</param>
    public void PrepareFrame(FrameTime time, bool paused)
    {
        _ = paused;
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 프레임 준비입니다.");
        }

        // 원본 세션 준비의 AI·입자·소리·정지 분기는 후속 단위에서 잇는다(결정 6 — 월드 시각만 둔다).
        _world.PrepareFrame(time);
    }

    /// <summary>같은 시각으로 다리 스캔·월드 준비·커널을 한 번 실행한다. 적재 전에는 거부한다. 원본 세션 실행의 월드·커널 부분만 둔다. 2026-11-10 F3 세션 프레임 위임 이식에서 추가됐다.</summary>
    /// <param name="time">공급할 프레임 시각이다.</param>
    /// <param name="paused">정지 중이면 참이다. 정지 중에는 커널을 실행하지 않는다.</param>
    public void RunFrame(FrameTime time, bool paused)
    {
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 프레임 실행입니다.");
        }

        // 원본 세션 실행의 AI·건설 효과 분기는 후속 단위에서 잇는다(결정 6 — 월드 스캔·준비·커널만 둔다).
        _world.RunFrame(time, paused);
    }

    /// <summary>원본 건설 시작을 후속 배치와 공유한다. 껍데기에서는 건설 본체 미연결로 멈춘다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 건설 시작 Stub 이식에서 추가됐다.</summary>
    /// <param name="parent">건설할 부모 번호다.</param>
    /// <param name="duration">건설 기간이다. 0이면 원본 기본 10초다.</param>
    public void StartConstruction(int parent, float duration)
    {
        _ = parent;
        _ = duration;
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 건설 시작입니다.");
        }

        // 원본 세션 건설의 실제 빌더 본체는 Fort 본체 단위에서 잇는다(결정 6 — 성공을 합성하지 않는다).
        throw new InvalidOperationException("실제 건설 시작 미연결");
    }

    /// <summary>현재 연결 단계를 읽는다. 사전 구현에서는 항상 0(미접속)이다. 3차 로컬 구현 때 실제 전송과 연결한다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐다.</summary>
    /// <returns>연결 단계다.</returns>
    public uint LocalConnectionPhase() => _localPhase;

    /// <summary>수신한 로컬 패킷 목록을 빌린다. 사전 구현에서는 항상 비어 있다. 3차 로컬 구현 때 실제 분배기와 연결한다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐다.</summary>
    /// <returns>수신 패킷 목록이다.</returns>
    public IReadOnlyList<byte[]> LocalConnectionPackets() => _localPackets.AsReadOnly();

    /// <summary>다음 연결 상태를 한 번 소비했는지 조회한다. 사전 구현에서는 항상 거짓이다. 3차 로컬 구현 때 실제 큐와 연결한다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐다.</summary>
    /// <returns>소비했으면 참이다.</returns>
    public bool PollLocalConnectionState() => false;

    /// <summary>깊이·Graph·프로세스·공간의 실제 상태를 읽기 전용으로 노출한다. 월드 보고서를 그대로 빌린다. 2026-11-10 F3 세션 보고서 위임 이식에서 추가됐다.</summary>
    /// <returns>원본 CLI 관측과 같은 여러 줄 보고서다.</returns>
    public string Report() => _world.Report();

    /// <summary>현재 섬 목록의 개수를 읽는다. 설치 중 Add Island 관문도 같은 3204바이트 목록의 count DWORD를 직접 읽는다. 2026-11-10 F3 세션 섬 개수 조회 이식에서 추가됐다.</summary>
    /// <returns>섬 개수다.</returns>
    public uint IslandCount()
    {
        // 원본 세션 섬 개수 조회가 현재 IslandList의 count DWORD를 직접 읽는 모양을 직역한다.
        return BitConverter.ToUInt32(_storage.Islands.Records, 0);
    }

    /// <summary>지정 소유자의 실제 섬 개수를 읽는다. 소유자 0도 원문대로 비교한다. 2026-11-10 F3 세션 소유자별 섬 개수 조회 이식에서 추가됐다.</summary>
    /// <param name="owner">셀 소유자 번호다.</param>
    /// <returns>해당 소유자의 섬 개수다.</returns>
    public uint OwnedIslandCount(uint owner)
    {
        // 원본 0046f430의 같은 현재 섬 목록 순회를 직역한다. 존재하지 않는 기록은 건너뛴다.
        byte[] records = _storage.Islands.Records;
        uint total = BitConverter.ToUInt32(records, 0);
        int count = (int)Math.Min(total, 100u);
        uint owned = 0u;
        // 기록마다 존재 DWORD와 현재 소유자 DWORD를 읽는다.
        for (int island = 0; island < count; island++)
        {
            int record = 4 + island * 32;
            uint exists = BitConverter.ToUInt32(records, record + 4);
            if (exists == 0u)
            {
                continue;
            }

            if (BitConverter.ToUInt32(records, record + 8) == owner)
            {
                owned++;
            }
        }

        return owned;
    }

    /// <summary>로컬 패킷을 보낸다. 사전 구현에서는 전송기가 없어 명시 오류로 멈춘다. 3차 로컬 구현 때 루프백 전송과 연결한다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐다.</summary>
    /// <param name="target">받을 소유자 번호다.</param>
    /// <param name="packet">보낼 패킷이다.</param>
    public void SendLocalPacket(int target, byte[] packet)
    {
        _ = target;
        ArgumentNullException.ThrowIfNull(packet);

        // 원본 세션 송신의 실제 로컬 전송 본체는 3차 로컬 구현 때 잇는다(결정 6 — 성공을 합성하지 않는다).
        throw new InvalidOperationException("실제 로컬 연결 전송 미연결");
    }

    /// <summary>송수신 큐와 로그인 타이머를 진행한다. 사전 구현에서는 전송기가 없어 명시 오류로 멈춘다. 3차 로컬 구현 때 루프백 펌프와 연결한다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐다.</summary>
    public void PumpLocalConnection()
    {
        // 원본 세션 펌프의 실제 큐·타이머 본체는 3차 로컬 구현 때 잇는다(결정 6 — 성공을 합성하지 않는다).
        throw new InvalidOperationException("실제 로컬 연결 펌프 미연결");
    }
}
