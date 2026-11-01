using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Client.Scripting;

namespace Netstorm.Client.Host;

/// <summary>현재 이름 창의 입력을 검사하고 같은 편집 선택 큐에 ReadFort를 예약한다.</summary>
public sealed class HostFortNameConfirmation
{
    /// <summary>현재 설정과 파일 작업본 경로다.</summary>
    private readonly HostStartupScene _startup;
    /// <summary>실제 공통 Gump와 문자 버퍼다.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>현재 입력 필드의 문자 본체다.</summary>
    private readonly HostTextEntry _entry;
    /// <summary>편집 작업본의 전체 경로 선택기다.</summary>
    private readonly EditorWorkspacePaths _paths;
    /// <summary>이름 문자열을 보관하는 현재 스크립트 고리다.</summary>
    private readonly ScriptStringRing _strings;
    /// <summary>실제 입력을 소비하는 현재 큐다.</summary>
    private readonly InputQueue _queue;
    /// <summary>현재 이름 필요 안내 창이다.</summary>
    private readonly Action<string> _tell;
    /// <summary>같은 호스트의 장치와 큐를 빌리며 파일을 생성하지 않는다.</summary>
    public HostFortNameConfirmation(HostStartupScene startup, HostMainMenu menu, HostTextEntry entry, InputQueue queue, ScriptStringRing strings, Action<string> tell)
    { _startup = startup; _tree = menu.Tree; _entry = entry; _queue = queue; _strings = strings; _tell = tell; _paths = new EditorWorkspacePaths(startup.Files.BaseDirectory); }
    /// <summary>현재 입력을 원문대로 정리하고 충돌이 없으면 두 인자가 같은 ReadFort를 큐에 쓴다.</summary>
    public uint Confirm(uint token)
    {
        var root = _tree.Node(token) as SaveFortNameDialogNode ?? throw new InvalidOperationException("Fort 이름 창 없음");
        return SaveFortName.Confirm(() => root.FortMode, new SaveFortNameHooks
        {
            CopyText = output => _entry.Entry.CopyText(root.Entry, output),
            Alnum = value => value is >= (sbyte)'0' and <= (sbyte)'9' or >= (sbyte)'A' and <= (sbyte)'Z' or >= (sbyte)'a' and <= (sbyte)'z',
            Path = name => _paths.Resolve(_startup.Configuration.PathSpec("fortSpec", name)),
            // 현재 목록은 아직 이관 장부가 없어 원본 loose 파일도 열거한다. 동일 목록 이름의 충돌을 보존한다.
            Exists = path => File.Exists(path) || Directory.Exists(path) || _paths.SourceExists(path) ? 1 : 0,
            ReadFort = name => { uint stored = _strings.Store(name); _queue.PushCommand(FortSelection.ReadFort, unchecked((int)stored), unchecked((int)stored)); },
            Tell = _tell,
            Header = () => throw new InvalidOperationException("Save as의 현재 Template 소유자 미연결"),
            SetName = _ => throw new InvalidOperationException("Save as의 현재 Fort 이름 소유자 미연결")
        });
    }
}
