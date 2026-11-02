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
    /// <summary>목록과 공유하는 편집 작업본과 이관 장부다.</summary>
    private readonly EditorCatalogWorkspace _workspace;
    /// <summary>이름 문자열을 보관하는 현재 스크립트 고리다.</summary>
    private readonly ScriptStringRing _strings;
    /// <summary>실제 입력을 소비하는 현재 큐다.</summary>
    private readonly InputQueue _queue;
    /// <summary>현재 이름 필요 안내 창이다.</summary>
    private readonly Action<string> _tell;
    /// <summary>
    /// 현재 Template 소유자의 20바이트 헤더 장치다. Save as의 현재 Fort 모드에서만 쓴다. 소유자는 세션(F3)이 정하므로 세션 연결 뒤 호스트가 주입하며,
    /// 주입하지 않으면 그 지점에서 명시 오류로 멈춘다. (2026-11-02 TS6)
    /// </summary>
    private readonly Func<Memory<byte>>? _header;
    /// <summary>
    /// 현재 Fort 이름 전체 부모(<see cref="FortCurrentNameDevices.Set"/>)를 실행하는 장치다. 전체 snapshot과 월드 소유자가 필요해 세션 연결 뒤 주입한다.
    /// 주입하지 않으면 그 지점에서 명시 오류로 멈춘다. (2026-11-02 TS6)
    /// </summary>
    private readonly Action<string>? _setName;
    /// <summary>
    /// 같은 호스트의 장치와 큐, 목록이 쓰는 편집 작업본을 빌리며 파일을 생성하지 않는다.
    /// <paramref name="header"/>와 <paramref name="setName"/>은 현재 Fort 모드의 Save as가 쓰는 세션 소유 경계이며, 생략하면 해당 지점에서 명시 오류로 멈춘다.
    /// </summary>
    public HostFortNameConfirmation(HostStartupScene startup, HostMainMenu menu, HostTextEntry entry, InputQueue queue, ScriptStringRing strings, EditorCatalogWorkspace workspace, Action<string> tell,
        Func<Memory<byte>>? header = null, Action<string>? setName = null)
    { _startup = startup; _tree = menu.Tree; _entry = entry; _queue = queue; _strings = strings; _tell = tell; _workspace = workspace; _header = header; _setName = setName; }
    /// <summary>현재 입력을 원문대로 정리하고 충돌이 없으면 두 인자가 같은 ReadFort를 큐에 쓴다.</summary>
    public uint Confirm(uint token)
    {
        var root = _tree.Node(token) as SaveFortNameDialogNode ?? throw new InvalidOperationException("Fort 이름 창 없음");
        return SaveFortName.Confirm(() => root.FortMode, new SaveFortNameHooks
        {
            CopyText = output => _entry.Entry.CopyText(root.Entry, output),
            Alnum = value => value is >= (sbyte)'0' and <= (sbyte)'9' or >= (sbyte)'A' and <= (sbyte)'Z' or >= (sbyte)'a' and <= (sbyte)'z',
            Path = name => _workspace.Resolve(_startup.Configuration.PathSpec("fortSpec", name)),
            // cpppj와 같이 작업본에서만 파일이나 디렉터리 존재를 본다. 장부에서 삭제 완료된 원본 이름은 충돌이 아니다.
            Exists = path => _workspace.Files.PathExists(path) ? 1 : 0,
            ReadFort = name => { uint stored = _strings.Store(name); _queue.PushCommand(FortSelection.ReadFort, unchecked((int)stored), unchecked((int)stored)); },
            Tell = _tell,
            Header = _header ?? (() => throw new InvalidOperationException("Save as의 현재 Template 소유자 미연결")),
            SetName = _setName ?? (_ => throw new InvalidOperationException("Save as의 현재 Fort 이름 소유자 미연결"))
        });
    }
}
