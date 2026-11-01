using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>미션 초기화가 빌리는 실제 Client 저장소와 파일·상태 소유자다.</summary>
public sealed class HostTutorialInitialization
{
    private readonly GuideMissionOwner _guide;
    private readonly MissionFortRequestGlobals _requestGlobals;
    private readonly byte[] _options;
    private readonly uint[] _restrictions;
    private readonly Action _clearTemplates;
    private readonly Func<string, string> _selectFort;
    private readonly Func<string, int> _findType;
    private readonly Func<bool> _pending;
    private readonly Action<string> _postFort;

    /// <summary>다음 State가 설치하기 전까지 소유하는 현재 요청이다.</summary>
    public MissionFortLoadRequest? Request { get; private set; }

    /// <summary>Guide·현재 저장소 및 실제 하위 효과를 같은 Client 수명으로 빌린다.</summary>
    public HostTutorialInitialization(GuideMissionOwner guide, MissionFortRequestGlobals requestGlobals, byte[] options,
        uint[] restrictions, Action clearTemplates, Func<string, string> selectFort, Func<string, int> findType,
        Func<bool> pending, Action<string> postFort)
    {
        _guide = guide; _requestGlobals = requestGlobals; _options = options; _restrictions = restrictions;
        _clearTemplates = clearTemplates; _selectFort = selectFort; _findType = findType; _pending = pending; _postFort = postFort;
        ArgumentNullException.ThrowIfNull(guide); ArgumentNullException.ThrowIfNull(requestGlobals); ArgumentNullException.ThrowIfNull(options);
        ArgumentNullException.ThrowIfNull(restrictions); ArgumentNullException.ThrowIfNull(clearTemplates); ArgumentNullException.ThrowIfNull(selectFort);
        ArgumentNullException.ThrowIfNull(findType); ArgumentNullException.ThrowIfNull(pending); ArgumentNullException.ThrowIfNull(postFort);
    }

    /// <summary>실제 Guide 설정·옵션·Fort 요청·기술 표를 원본 순서로 갱신한다.</summary>
    public void Initialize()
    {
        _guide.InitializeTutorial(new TutorialInitHooks
        {
            ClearTemplates = _clearTemplates,
            ResetOptions = () => RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, _options),
            RequestFort = name =>
            {
                if (_pending() || Request != null) throw new InvalidOperationException("Init 요새 로드 요청 전 다른 전환/이미지가 남아 있음");
                var request = new MissionFortLoadRequest();
                MissionFortRequest.Write(name, _requestGlobals, new MissionFortRequestHooks
                {
                    SelectFort = selected => request.Fort = _selectFort(selected),
                    Duplicate = packet => MissionFortRequest.CaptureZone(request, packet)
                });
                Request = request; _postFort(request.Fort);
            },
            Header = () => MissionTechnology.Apply(_guide.Mission ?? throw new InvalidOperationException("현재 미션 없음"), _findType, _restrictions)
        });
    }

    /// <summary>호스트가 자신의 지연 전환을 취소한 뒤 현재 요청을 반납한다.</summary>
    public void ReleaseRequest() => Request = null;
}
