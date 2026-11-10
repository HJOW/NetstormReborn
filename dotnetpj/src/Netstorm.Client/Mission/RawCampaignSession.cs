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

    /// <summary>로그인·기술·AI가 공유하는 현재 원본 Player 상태다. 원본 battleOwner·templateOwner 기본값은 둘 다 1이다. 2026-11-10 F3 소유자 본체에서 추가됐다.</summary>
    private readonly PlayerWorldState _player = new();

    /// <summary>원본 자산 Battle 상태다. 설치 때만 참이 되며 현재는 거짓이다. 2026-11-10 F3 소유자 본체에서 추가됐다.</summary>
    private bool _inBattle;

    /// <summary>원본 600바이트 AI 설정의 현재 기본값이다. 설치 본체가 갱신하며 AI 생성·시작이 같은 참조를 빌린다. 2026-11-10 F3 AI 설정 소유자 본체에서 추가됐다.</summary>
    private readonly MissionAiSettingsState _aiSettings = new();

    /// <summary>원본 첫 목록(outposts)의 현재 항목이다. Fort 적재 본체가 채우기 전에는 비어 있다. 2026-11-10 F3 재장전 본체에서 추가됐다.</summary>
    private readonly RawPostPopList _outposts = new();

    /// <summary>원본 두 번째 목록(temples)의 현재 항목이다. Fort 적재 본체가 채우기 전에는 비어 있다. 2026-11-10 F3 재장전 본체에서 추가됐다.</summary>
    private readonly RawPostPopList _temples = new();

    /// <summary>로컬 연결의 현재 단계다. 사전 구현에서는 항상 0(미접속)이다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐다.</summary>
    private uint _localPhase = 0u;

    /// <summary>수신한 로컬 패킷 목록이다. 사전 구현에서는 항상 비어 있다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐다.</summary>
    private readonly List<byte[]> _localPackets = new();

    /// <summary>같은 실제 빌더의 건설 시작 효과다. 세션 조립 때 실제 건설 몸체를 묶으며 그 전까지는 비어 있다. 2026-11-10 F3 건설 시작 본체에서 추가됐다.</summary>
    private Action<int, float>? _constructionStart;

    /// <summary>같은 실제 AI 생성자의 생성 효과다. 세션 조립 때 실제 AI 몸체를 묶으며 그 전까지는 비어 있다. 2026-11-10 F3 AI 생성 본체에서 추가됐다.</summary>
    private Func<uint, byte[], uint>? _aiCreate;

    /// <summary>같은 실제 AI 준비 몸체의 준비 효과다. 세션 조립 때 실제 AI 준비 몸체를 묶으며 그 전까지는 비어 있다. 2026-11-10 F3 AI 준비 본체에서 추가됐다.</summary>
    private Func<uint, bool>? _aiPrepare;

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

    /// <summary>로그인·기술·AI가 공유하는 현재 원본 Player 상태다. 설치 본체가 battleOwner·templateOwner를 갱신하며 여러 몸체가 같은 참조를 빌린다. 2026-11-10 F3 소유자 본체에서 추가됐다.</summary>
    internal PlayerWorldState PlayerState => _player;

    /// <summary>원본 자산 Battle 상태다. 설치 본체가 켜고 끈다. 2026-11-10 F3 소유자 본체에서 추가됐다.</summary>
    internal bool InBattle
    {
        get => _inBattle;
        set => _inBattle = value;
    }

    /// <summary>원본 600바이트 AI 설정의 현재 기본값이다. 설치 본체가 갱신하며 AI 생성·시작이 같은 참조를 빌린다. 2026-11-10 F3 AI 설정 소유자 본체에서 추가됐다.</summary>
    internal MissionAiSettingsState AiSettings => _aiSettings;

    /// <summary>원본 첫 목록(outposts)의 현재 항목이다. Fort 적재 본체가 채우며 게임 메뉴가 같은 참조를 읽는다. 2026-11-10 F3 재장전 본체에서 추가됐다.</summary>
    internal RawPostPopList Outposts => _outposts;

    /// <summary>원본 두 번째 목록(temples)의 현재 항목이다. Fort 적재 본체가 채우며 게임 메뉴가 같은 참조를 읽는다. 2026-11-10 F3 재장전 본체에서 추가됐다.</summary>
    internal RawPostPopList Temples => _temples;

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

    /// <summary>같은 실제 빌더의 건설 시작 효과를 묶는다. 원본 세션 설치의 건설 몸체 연결만 둔다. 2026-11-10 F3 건설 시작 본체에서 추가됐다.</summary>
    /// <param name="start">실제 빌더 시작 효과다. 부모 번호와 기간을 그대로 받는다.</param>
    public void BindConstructionStart(Action<int, float> start)
    {
        ArgumentNullException.ThrowIfNull(start);
        _constructionStart = start;
    }

    /// <summary>같은 실제 빌더의 건설 시작 효과를 묶었는지 확인한다. 2026-11-10 F3 건설 시작 본체에서 추가됐다.</summary>
    /// <returns>묶었으면 참이다.</returns>
    public bool HasConstructionStart() => _constructionStart is not null;

    /// <summary>원본 건설 시작을 후속 배치와 공유한다. 묶은 실제 빌더 효과가 없으면 본체 미연결로 멈춘다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 건설 시작 Stub 이식에서 추가됐고 2026-11-10 F3 건설 시작 본체에서 본체를 이었다.</summary>
    /// <param name="parent">건설할 부모 번호다. 원본 SID 폭(0~65535)을 받는다.</param>
    /// <param name="duration">건설 기간이다. 0이면 원본 기본 10초다.</param>
    public void StartConstruction(int parent, float duration)
    {
        if (parent < 0 || parent > 65535)
        {
            throw new ArgumentOutOfRangeException(nameof(parent), parent, "건설 시작의 실제 부모 SID 범위 오류");
        }

        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 건설 시작입니다.");
        }

        // 원본 RawCampaignSession::StartConstruction의 설치·빌더 관문을 직역한다. 준비 시각 대입(PrepareBuilding)은 실제 빌더 몸체 쪽에서 잇는다(결정 6 — 성공을 합성하지 않는다).
        if (_constructionStart is null)
        {
            throw new InvalidOperationException("실제 건설 시작 미연결");
        }

        _constructionStart(parent, duration);
    }

    /// <summary>같은 실제 AI 생성자의 생성 효과를 묶는다. 원본 세션 설치의 AI 몸체 연결만 둔다. 2026-11-10 F3 AI 생성 본체에서 추가됐다.</summary>
    /// <param name="create">실제 AI 생성 효과다. 소유자 번호와 600바이트 설정을 그대로 받는다.</param>
    public void BindAiCreate(Func<uint, byte[], uint> create)
    {
        ArgumentNullException.ThrowIfNull(create);
        _aiCreate = create;
    }

    /// <summary>같은 실제 AI 생성자의 생성 효과를 묶었는지 확인한다. 2026-11-10 F3 AI 생성 본체에서 추가됐다.</summary>
    /// <returns>묶었으면 참이다.</returns>
    public bool HasAiCreate() => _aiCreate is not null;

    /// <summary>원본 Stage3 1..8 방문 순서의 AI 생성을 후속 AI 몸체와 공유한다. 묶은 실제 AI 효과가 없으면 본체 미연결로 멈춘다. 적재 전에는 적재 관문에서 먼저 멈춘다. 600바이트 검사는 실제 AI 몸체 쪽에서 잇는다. 2026-11-10 F3 AI 생성 본체에서 추가됐다.</summary>
    /// <param name="owner">AI를 만들 소유자 번호다. 원본 1..8 범위를 받는다.</param>
    /// <param name="settings">600바이트 AI 설정이다. 호출자가 소유하며 세션은 빌려서 넘긴다.</param>
    /// <returns>만든 brain 토큰이다.</returns>
    public uint CreateAi(uint owner, byte[] settings)
    {
        ArgumentNullException.ThrowIfNull(settings);
        if (owner < 1u || owner > 8u)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "AI 생성의 실제 소유자 1..8 범위 오류");
        }

        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 AI 생성입니다.");
        }

        // 원본 RawCampaignSession::CreateAi의 적재·owner 관문을 직역한다. 설치·600바이트 본체는 실제 AI 몸체 쪽에서 잇는다(결정 6 — 성공을 합성하지 않는다).
        if (_aiCreate is null)
        {
            throw new InvalidOperationException("실제 AI 생성 미연결");
        }

        return _aiCreate(owner, settings);
    }

    /// <summary>같은 실제 AI 준비 몸체의 준비 효과를 묶는다. 원본 세션 설치의 AI 준비 몸체 연결만 둔다. 2026-11-10 F3 AI 준비 본체에서 추가됐다.</summary>
    /// <param name="prepare">실제 AI 준비 효과다. 소유자 번호를 그대로 받고 준비된 brain이 있으면 참을 돌려준다.</param>
    public void BindAiPrepare(Func<uint, bool> prepare)
    {
        ArgumentNullException.ThrowIfNull(prepare);
        _aiPrepare = prepare;
    }

    /// <summary>같은 실제 AI 준비 몸체의 준비 효과를 묶었는지 확인한다. 2026-11-10 F3 AI 준비 본체에서 추가됐다.</summary>
    /// <returns>묶었으면 참이다.</returns>
    public bool HasAiPrepare() => _aiPrepare is not null;

    /// <summary>원본 00416eb0의 준비된 brain 준비를 후속 AI 몸체와 공유한다. 준비된 brain이 없으면 거짓을 돌려준다. 묶은 실제 AI 효과가 없으면 본체 미연결로 멈춘다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 AI 준비 본체에서 추가됐다.</summary>
    /// <param name="owner">AI를 준비할 소유자 번호다. 원본 1..8 범위를 받는다.</param>
    /// <returns>준비된 brain을 준비했으면 참, 없으면 거짓이다.</returns>
    public bool PrepareAi(uint owner)
    {
        if (owner < 1u || owner > 8u)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "AI 준비의 실제 소유자 1..8 범위 오류");
        }

        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 AI 준비입니다.");
        }

        // 원본 RawCampaignSession::PrepareAi의 적재·owner 관문과 brain 없음 거짓 경로를 직역한다. 실제 준비 몸체는 후속 단위에서 잇는다(결정 6 — 성공을 합성하지 않는다).
        if (_aiPrepare is null)
        {
            throw new InvalidOperationException("실제 AI 준비 미연결");
        }

        return _aiPrepare(owner);
    }

    /// <summary>같은 파서·생성 몸체의 위치 생성 효과다. 세션 조립 때 실제 생성 몸체를 묶으며 그 전까지는 비어 있다. 2026-11-10 F3 위치 명령 본체에서 추가됐다.</summary>
    private Action<string>? _createAt;

    /// <summary>같은 파서·삭제 몸체의 위치 삭제 효과다. 세션 조립 때 실제 삭제 몸체를 묶으며 그 전까지는 비어 있다. 2026-11-10 F3 위치 명령 본체에서 추가됐다.</summary>
    private Action<string>? _destroyAt;

    /// <summary>같은 파서로 위치 문자열을 해석해 실제 Runtime에 생성한다. 묶은 생성 효과가 없으면 본체 미연결로 멈춘다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 위치 명령 Stub 이식에서 추가됐고 2026-11-10 F3 위치 명령 본체에서 본체를 이었다.</summary>
    /// <param name="location">생성 위치 문자열이다.</param>
    public void CreateAt(string location)
    {
        ArgumentNullException.ThrowIfNull(location);
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 생성 명령입니다.");
        }

        // 원본 0044c660의 파서·지면·가상 소유자·HP·Pop 본체는 Fort 본체·규칙 단위에서 잇는다. 묶은 효과가 없으면 성공을 합성하지 않는다(결정 6).
        if (_createAt is null)
        {
            throw new InvalidOperationException("실제 위치 생성 미연결");
        }

        _createAt(location);
    }

    /// <summary>같은 파서로 위치 문자열을 해석해 같은 월드의 가상 삭제로 지운다. 묶은 삭제 효과가 없으면 본체 미연결로 멈춘다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 위치 명령 Stub 이식에서 추가됐고 2026-11-10 F3 위치 명령 본체에서 본체를 이었다.</summary>
    /// <param name="location">삭제 위치 문자열이다.</param>
    public void DestroyAt(string location)
    {
        ArgumentNullException.ThrowIfNull(location);
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 삭제 명령입니다.");
        }

        // 원본 0044c790의 점검·타입·소유자 필터·가상 삭제 본체는 Fort 본체·규칙 단위에서 잇는다. 묶은 효과가 없으면 성공을 합성하지 않는다(결정 6).
        if (_destroyAt is null)
        {
            throw new InvalidOperationException("실제 위치 삭제 미연결");
        }

        _destroyAt(location);
    }

    /// <summary>같은 파서·생성 몸체의 위치 생성 효과를 묶는다. 원본 세션 설치의 생성 몸체 연결만 둔다. 2026-11-10 F3 위치 명령 본체에서 추가됐다.</summary>
    /// <param name="create">실제 위치 생성 효과다. 위치 문자열을 그대로 받는다.</param>
    public void BindCreateAt(Action<string> create)
    {
        ArgumentNullException.ThrowIfNull(create);
        _createAt = create;
    }

    /// <summary>같은 파서·삭제 몸체의 위치 삭제 효과를 묶는다. 원본 세션 설치의 삭제 몸체 연결만 둔다. 2026-11-10 F3 위치 명령 본체에서 추가됐다.</summary>
    /// <param name="destroy">실제 위치 삭제 효과다. 위치 문자열을 그대로 받는다.</param>
    public void BindDestroyAt(Action<string> destroy)
    {
        ArgumentNullException.ThrowIfNull(destroy);
        _destroyAt = destroy;
    }

    /// <summary>같은 파서·생성 몸체의 위치 생성 효과를 묶었는지 확인한다. 2026-11-10 F3 위치 명령 본체에서 추가됐다.</summary>
    /// <returns>묶었으면 참이다.</returns>
    public bool HasCreateAt() => _createAt is not null;

    /// <summary>같은 파서·삭제 몸체의 위치 삭제 효과를 묶었는지 확인한다. 2026-11-10 F3 위치 명령 본체에서 추가됐다.</summary>
    /// <returns>묶었으면 참이다.</returns>
    public bool HasDestroyAt() => _destroyAt is not null;

    /// <summary>같은 ownership 몸체의 지역 투표·지형 갱신 효과다. 세션 조립 때 실제 지형 몸체를 묶으며 그 전까지는 비어 있다. 2026-11-10 F3 지형 갱신 본체에서 추가됐다.</summary>
    private Action<float, float>? _reloadTheam;

    /// <summary>원본 455380의 실제 mode0 지역 투표·지형 갱신을 같은 ownership 몸체로 실행한다. 묶은 ownership 효과가 없으면 본체 미연결로 멈춘다. 좌표는 원문 float 비트다. 2026-11-10 F3 세션 지형 갱신 Stub 이식에서 추가됐고 2026-11-10 F3 지형 갱신 본체에서 본체를 이었다.</summary>
    /// <param name="x">갱신할 X 좌표의 float 비트다.</param>
    /// <param name="y">갱신할 Y 좌표의 float 비트다.</param>
    /// <param name="mode">원본 mode0만 받는다. 0이 아니면 거부한다.</param>
    public void ReloadTheamAt(uint x, uint y, uint mode = 0u)
    {
        if (mode != 0u)
        {
            throw new ArgumentException("게임메뉴 ReloadTheams의 원본 mode0 필요", nameof(mode));
        }

        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 지형 갱신입니다.");
        }

        // 원본 ownership의 실제 투표·지형 갱신 본체는 Fort 본체 단위에서 잇는다. 묶은 효과가 없으면 성공을 합성하지 않는다(결정 6).
        if (_reloadTheam is null)
        {
            throw new InvalidOperationException("실제 지형 갱신 미연결");
        }

        // 원본처럼 uint 비트를 float으로 재해석해 넘긴다.
        _reloadTheam(BitConverter.UInt32BitsToSingle(x), BitConverter.UInt32BitsToSingle(y));
    }

    /// <summary>같은 ownership 몸체의 지역 투표·지형 갱신 효과를 묶는다. 원본 세션 설치의 지형 몸체 연결만 둔다. 2026-11-10 F3 지형 갱신 본체에서 추가됐다.</summary>
    /// <param name="reload">실제 지형 갱신 효과다. float 좌표를 그대로 받는다.</param>
    public void BindReloadTheam(Action<float, float> reload)
    {
        ArgumentNullException.ThrowIfNull(reload);
        _reloadTheam = reload;
    }

    /// <summary>같은 ownership 몸체의 지역 투표·지형 갱신 효과를 묶었는지 확인한다. 2026-11-10 F3 지형 갱신 본체에서 추가됐다.</summary>
    /// <returns>묶었으면 참이다.</returns>
    public bool HasReloadTheam() => _reloadTheam is not null;

    /// <summary>같은 완전 배치 몸체의 무작위 배치 효과다. 세션 조립 때 실제 배치 몸체를 묶으며 그 전까지는 비어 있다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    private Action<uint, uint>? _addRandomObjects;

    /// <summary>같은 실제 Deck의 생산 동기화 효과다. 세션 조립 때 실제 생산 몸체를 묶으며 그 전까지는 비어 있다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    private Action? _synchronizeProduction;

    /// <summary>같은 실제 세션·저장소의 메타메뉴 별칭 조회 효과다. 세션 조립 때 실제 조회 몸체를 묶으며 그 전까지는 비어 있다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    private Func<uint, uint>? _metaMenuType;

    /// <summary>원본 0040dfd0의 고정 범위·지역 난수를 같은 완전 배치·섬·Factory에 연결한다. 묶은 배치 효과가 없으면 본체 미연결로 멈춘다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 무작위 배치 Stub 이식에서 추가됐고 2026-11-10 F3 나머지 본체에서 본체를 이었다.</summary>
    /// <param name="type">배치할 객체 종류 번호다.</param>
    /// <param name="quantity">배치할 수량이다.</param>
    public void AddRandomObjects(uint type, uint quantity)
    {
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 무작위 배치입니다.");
        }

        // 원본 무작위 배치의 실제 범위·지역 난수·완전 배치 본체는 Fort 본체 단위에서 잇는다. 묶은 효과가 없으면 성공을 합성하지 않는다(결정 6).
        if (_addRandomObjects is null)
        {
            throw new InvalidOperationException("실제 무작위 배치 미연결");
        }

        _addRandomObjects(type, quantity);
    }

    /// <summary>원본 첫 목록(outposts) 또는 두 번째 목록(temples)의 현재 개수다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 게임메뉴 재장전 Stub 이식에서 추가됐고 2026-11-10 F3 재장전 본체에서 본체를 이었다.</summary>
    /// <param name="second">참이면 두 번째 목록(temples), 거짓이면 첫 목록(outposts)이다.</param>
    /// <returns>목록 개수다.</returns>
    public int GameMenuReloadCount(bool second)
    {
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 재장전 조회입니다.");
        }

        // 원본처럼 목록 count DWORD를 signed로 읽는다. Fort 적재 본체가 채우기 전에는 0이다.
        return unchecked((int)(second ? _temples : _outposts).Count);
    }

    /// <summary>같은 실제 Deck의 생산 동기화만 같은 실제 후반 HUD 설치에 허용한다. 묶은 동기화 효과가 없으면 본체 미연결로 멈춘다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 생산 동기화 Stub 이식에서 추가됐고 2026-11-10 F3 나머지 본체에서 본체를 이었다.</summary>
    public void SynchronizeProductionContents()
    {
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 생산 동기화입니다.");
        }

        // 원본 43ec00의 실제 Deck 동기화 본체는 Fort 본체 단위에서 잇는다. 묶은 효과가 없으면 성공을 합성하지 않는다(결정 6).
        if (_synchronizeProduction is null)
        {
            throw new InvalidOperationException("실제 생산 동기화 미연결");
        }

        _synchronizeProduction();
    }

    /// <summary>메타 제외 메뉴의 13개 현재 TYPE 별칭을 같은 실제 세션·저장소에서 조회한다. 묶은 조회 효과가 없으면 주소 검증 뒤 본체 미연결로 멈춘다. 2026-11-10 F3 세션 메타메뉴 별칭 Stub 이식에서 추가됐고 2026-11-10 F3 나머지 본체에서 본체를 이었다.</summary>
    /// <param name="address">조회할 원본 주소다.</param>
    /// <returns>해당 주소의 현재 TYPE 별칭이다.</returns>
    public uint MetaMenuType(uint address)
    {
        // 원본 별칭 주소 16개를 그대로 둔다. 임의 주소는 세션 상태와 무관하게 거부한다.
        switch (address)
        {
            case 0x5412d0u:
            case 0x54126cu:
            case 0x5411e8u:
            case 0x5411ecu:
            case 0x5411f0u:
            case 0x5411a8u:
            case 0x541240u:
            case 0x54123cu:
            case 0x541280u:
            case 0x541298u:
            case 0x5412b0u:
            case 0x54117cu:
            case 0x541214u:
            case 0x54128cu:
            case 0x5412a0u:
            case 0x5412f8u:
                break;
            default:
                throw new ArgumentOutOfRangeException(nameof(address), address, "메타 제외 메뉴의 알려지지 않은 현재 TYPE 별칭");
        }

        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 메타메뉴 조회입니다.");
        }

        // 원본 별칭의 실제 사제·장식·지형·간헐천·비행선 TYPE 조회 본체는 Fort 본체 단위에서 잇는다. 묶은 효과가 없으면 성공을 합성하지 않는다(결정 6).
        if (_metaMenuType is null)
        {
            throw new InvalidOperationException("실제 메타메뉴 별칭 미연결");
        }

        return _metaMenuType(address);
    }

    /// <summary>같은 완전 배치 몸체의 무작위 배치 효과를 묶는다. 원본 세션 설치의 배치 몸체 연결만 둔다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    /// <param name="place">실제 무작위 배치 효과다. 종류 번호와 수량을 그대로 받는다.</param>
    public void BindAddRandomObjects(Action<uint, uint> place)
    {
        ArgumentNullException.ThrowIfNull(place);
        _addRandomObjects = place;
    }

    /// <summary>같은 실제 Deck의 생산 동기화 효과를 묶는다. 원본 세션 설치의 생산 몸체 연결만 둔다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    /// <param name="synchronize">실제 생산 동기화 효과다.</param>
    public void BindSynchronizeProductionContents(Action synchronize)
    {
        ArgumentNullException.ThrowIfNull(synchronize);
        _synchronizeProduction = synchronize;
    }

    /// <summary>같은 실제 세션·저장소의 메타메뉴 별칭 조회 효과를 묶는다. 원본 세션 설치의 조회 몸체 연결만 둔다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    /// <param name="lookup">실제 별칭 조회 효과다. 원본 주소를 그대로 받는다.</param>
    public void BindMetaMenuType(Func<uint, uint> lookup)
    {
        ArgumentNullException.ThrowIfNull(lookup);
        _metaMenuType = lookup;
    }

    /// <summary>같은 완전 배치 몸체의 무작위 배치 효과를 묶었는지 확인한다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    /// <returns>묶었으면 참이다.</returns>
    public bool HasAddRandomObjects() => _addRandomObjects is not null;

    /// <summary>같은 실제 Deck의 생산 동기화 효과를 묶었는지 확인한다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    /// <returns>묶었으면 참이다.</returns>
    public bool HasSynchronizeProductionContents() => _synchronizeProduction is not null;

    /// <summary>같은 실제 세션·저장소의 메타메뉴 별칭 조회 효과를 묶었는지 확인한다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    /// <returns>묶었으면 참이다.</returns>
    public bool HasMetaMenuType() => _metaMenuType is not null;

    /// <summary>원본 첫 목록 또는 두 번째 목록의 현재 저장소 뷰에서 지정 번호를 읽는다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 게임메뉴 재장전 Stub 이식에서 추가됐고 2026-11-10 F3 재장전 본체에서 본체를 이었다.</summary>
    /// <param name="second">참이면 두 번째 목록(temples), 거짓이면 첫 목록(outposts)이다.</param>
    /// <param name="index">읽을 번호다.</param>
    /// <returns>해당 번호의 SID다.</returns>
    public uint GameMenuReloadSid(bool second, uint index)
    {
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 재장전 조회입니다.");
        }

        // 원본처럼 목록 뷰의 번호가 범위를 넘으면 거부한다.
        ReadOnlySpan<uint> items = (second ? _temples : _outposts).Items();
        if (index >= (uint)items.Length)
            throw new ArgumentOutOfRangeException(nameof(index), index, "재장전 목록 번호 범위 오류");
        return items[(int)index];
    }

    /// <summary>로그인·기술·AI가 공유하는 현재 원본 로컬 Player 번호다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 소유자 조회 Stub 이식에서 추가됐고 2026-11-10 F3 소유자 본체에서 본체를 이었다.</summary>
    /// <returns>로컬 소유자 번호다.</returns>
    public uint LocalOwner()
    {
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 소유자 조회입니다.");
        }

        // 원본 세션 Player 상태의 battleOwner를 그대로 읽는다. 설치 본체가 갱신하기 전에는 기본값 1이다.
        return _player.BattleOwner;
    }

    /// <summary>원본 정보 설명이 읽는 같은 PlayerWorld 템플릿 소유자다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 소유자 조회 Stub 이식에서 추가됐고 2026-11-10 F3 소유자 본체에서 본체를 이었다.</summary>
    /// <returns>템플릿 소유자 번호다.</returns>
    public uint TemplateOwner()
    {
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 소유자 조회입니다.");
        }

        // 원본 세션 Player 상태의 templateOwner를 그대로 읽는다. 설치 본체가 갱신하기 전에는 기본값 1이다.
        return _player.TemplateOwner;
    }

    /// <summary>원본 00594fbc의 현재 실제 자산 Battle 관문이다. 적재 전에는 적재 관문에서 먼저 멈춘다. 2026-11-10 F3 세션 소유자 조회 Stub 이식에서 추가됐고 2026-11-10 F3 소유자 본체에서 본체를 이었다.</summary>
    /// <returns>전투 중이면 1u, 아니면 0u이다.</returns>
    public uint BattleActive()
    {
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 전투 조회입니다.");
        }

        // 원본 세션 자산 상태의 inBattle을 그대로 읽으며 표시 상태는 만들지 않는다.
        return _inBattle ? 1u : 0u;
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

    /// <summary>같은 에디터 전송의 수신 폴링 효과다. 세션 조립 때 루프백 전송을 묶으며 그 전까지는 비어 있다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    private Func<int>? _editorPoll;

    /// <summary>같은 에디터 전송의 송신 효과다. 세션 조립 때 루프백 전송을 묶으며 그 전까지는 비어 있다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>

    /// <summary>같은 에디터 전송의 송신 효과다. 세션 조립 때 루프백 전송을 묶으며 그 전까지는 비어 있다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    private Func<int, byte[], bool>? _editorSend;

    /// <summary>로컬 패킷을 보낸다. 묶은 에디터 전송이 없으면 명시 오류로 멈춘다. 3차 로컬 구현 때 루프백 전송과 연결한다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐고 2026-11-10 멀티플레이 사전 구현에서 에디터 전송 분기를 이었다.</summary>
    /// <param name="target">받을 소유자 번호다.</param>
    /// <param name="packet">보낼 패킷이다.</param>
    public void SendLocalPacket(int target, byte[] packet)
    {
        ArgumentNullException.ThrowIfNull(packet);
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 로컬 송신입니다.");
        }

        // 원본 세션 송신의 Login 분기는 실제 Login 몸체 쪽에서 잇는다. 에디터 분기는 같은 물리 큐에 그대로 쓴다(결정 6 — 성공을 합성하지 않는다).
        if (_editorSend is not null)
        {
            _ = _editorSend(target, packet);
            return;
        }

        throw new InvalidOperationException("실제 로컬 연결 전송 미연결");
    }

    /// <summary>송수신 큐와 로그인 타이머를 진행한다. 묶은 에디터 전송이 없으면 명시 오류로 멈춘다. 3차 로컬 구현 때 루프백 펌프와 연결한다. 2026-11-10 F3 세션 연결 사전 구현에서 추가됐고 2026-11-10 멀티플레이 사전 구현에서 에디터 전송 분기를 이었다.</summary>
    public void PumpLocalConnection()
    {
        if (!_loaded)
        {
            throw new InvalidOperationException("캠페인 세션의 미적재 로컬 펌프입니다.");
        }

        // 원본 세션 펌프의 Login 분기는 실제 Login 몸체 쪽에서 잇는다. 에디터 분기는 같은 unbuffered 큐만 폴링한다(결정 6 — 성공을 합성하지 않는다).
        if (_editorPoll is not null)
        {
            _ = _editorPoll();
            return;
        }

        throw new InvalidOperationException("실제 로컬 연결 펌프 미연결");
    }

    /// <summary>같은 루프백 에디터 전송의 송수신 효과를 묶는다. 외부 주소 바인딩은 전송 쪽 정책이 거부한다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="poll">묶을 수신 폴링 효과다. 분배한 프레임 수를 돌려준다.</param>
    /// <param name="send">묶을 송신 효과다. 받아들였으면 참을 돌려준다.</param>
    public void BindLocalEditorTransport(Func<int> poll, Func<int, byte[], bool> send)
    {
        ArgumentNullException.ThrowIfNull(poll);
        ArgumentNullException.ThrowIfNull(send);
        _editorPoll = poll;
        _editorSend = send;
    }

    /// <summary>같은 루프백 에디터 전송을 묶었는지 확인한다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <returns>묶었으면 참이다.</returns>
    public bool HasLocalEditorTransport() => _editorPoll is not null && _editorSend is not null;
}
