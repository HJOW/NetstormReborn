using Netstorm.Core.Original;

// 실제Template헤더와Player State를같은Session/장기Client저장소에서빌린다.
namespace Netstorm.Client.Host;

/// <summary>현재manager/State커서/Notify가같은실제Template물리를사용한다.</summary>
public sealed partial class HostSession
{
    /// <summary>실제Root가넘긴장기Template이며독립호출만자체소유자를구성한다.</summary>
    public CampaignTemplateStorage Templates { get; }
    /// <summary>같은헤더/선택State/현재깊이의원문서비스다. 최초소비전에만조립한다.</summary>
    private RawPlayerTemplateState? _templateState;
    /// <summary>독립검사는알려진미정BYTE공급자를설치하고제품은실제현재할당입력을사용한다.</summary>
    private PlayerStateBytesProvider? _templateAllocationBytes;
    /// <summary>실제DataManager파일저장부모다. 미설치면선택된원문저장위치에서실패한다.</summary>
    private Action<uint>? _templateSave;

    /// <summary>같은현재manager에연결할실제저장효과를설치한다. 조회/저장을즉시실행하지않는다.</summary>
    public void BindTemplateSave(Action<uint> save)
    {
        _templateSave = save ?? throw new ArgumentNullException(nameof(save));
    }
    /// <summary>빈State의미정부분을독립입력으로고정한다. 이미생성한State를다시초기화하지않는다.</summary>
    public void BindTemplateAllocationBytes(PlayerStateBytesProvider bytes)
    {
        _templateAllocationBytes = bytes ?? throw new ArgumentNullException(nameof(bytes));
    }
    /// <summary>현재세션이반납된뒤이전Template소비자가재사용하지못하게한다.</summary>
    private RawPlayerTemplateState PlayerTemplateState()
    {
        if (_playerRecordsReleased) throw new InvalidOperationException("반납된Session의Template State소비");
        if (_templateState is not null) return _templateState;
        var state = new RawPlayerTemplateState(Pool.Edition, Templates.Headers, Templates.SelectState, Templates.Name,
            () => _worldLoadingDepth, value => _worldLoadingDepth = value, () => _fortState?.Mission ?? false,
            assertion => throw new InvalidOperationException("실제Player Template State진단:" + assertion),
            _ => throw new InvalidOperationException("10.78 Template State의예상하지않은CD깊이효과"));
        state.BindInitialization(new()
        {
            Bytes = (slot, bytes) =>
            {
                if (_templateAllocationBytes is { } supplied) supplied(slot, bytes); else ClientAllocationBytes.ReadCurrent(bytes);
            },
            Replace = Templates.InitializeState,
        });
        state.BindMinimumSize(Templates.EnsureStateSize);
        state.BindFileSave(slot => (_templateSave ?? throw new InvalidOperationException("실제Player Template파일저장부모미연결"))(slot));
        _templateState = state; return state;
    }
    /// <summary>원문getter가최소크기/현재cursor를준비한뒤동일State메모리를빌린다. 헤더를State로대체하지않는다.</summary>
    public Memory<byte> CurrentPlayerTemplateState(uint slot) => PlayerTemplateState().GetMemory(slot);
    /// <summary>실제Notify가정상복귀한뒤기존변경관측기록을남긴다. 원문실패prefix를성공으로기록하지않는다.</summary>
    public void NotifyPlayerTemplateState(uint slot)
    {
        PlayerTemplateState().Changed(slot); _playerTemplateMarks.Add(slot);
    }
}
