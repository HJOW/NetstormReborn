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
}
