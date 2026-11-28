using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // 실제 게임 Root의 네 공장과 바깥 UI 장치를 같은 수명으로 소유한다.

// Root와 생산과 채팅과 메시지 공장을 실제 Root에 묶는 외부 장치 묶음이다. 0.987에서 추가됐다.
public sealed class HostRootFactorySetHooks
{
    /// <summary>같은 현재 배경과 TYPE과 색 등 바깥 Client 전역이다. 창 생성 때 읽는다.</summary>
    public required HostNsInfoDevices BaseDevices { get; init; }
    /// <summary>송신과 음향과 메뉴와 카메라와 입력과 배치 15종 묶음이다. 0.986 공장에서 만든다.</summary>
    public required HostNsInfoRootDevicesHooks Root { get; init; }
    /// <summary>메시지 공장 팔레트 프로브의 실제 외부 장치다.</summary>
    public required UberMessageProbeHooks Message { get; init; }
}

// 실제 게임 Root의 네 공장 가운데 Root와 메시지 공장을 만들고 소유한다. 생산과 채팅 공장은 같은 트리 주인을 빌린다. 0.987에서 추가됐다.
public sealed class HostRootFactorySet : IDisposable
{
    /// <summary>같은 실제 노드와 등록과 입력과 수명과 그림 소유자다. 바깥 UI 장치의 같은 주인이다.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>같은 동일 스크롤과 스타일과 한 줄 문자 본체다. 추가 창도 같은 주인을 빌린다.</summary>
    private readonly HostHtmlText _text;
    /// <summary>같은 현재 NSInfo와 zone과 서버 목록 owner다. 추가 창도 같은 전역을 공유한다.</summary>
    private readonly ClientNsInfoGlobals _globals;
    /// <summary>같은 실제 모드와 TYPE 물리다. 추가 창도 같은 주인을 빌린다.</summary>
    private readonly ClientGameplayGlobals _gameplay;
    /// <summary>같은 실제 풀과 자산 저장소다. 추가 창도 같은 저장소를 공유한다.</summary>
    private readonly RawSurfaceStorage _storage;
    /// <summary>추가 창에 넣는 같은 장치 묶음이다. 첫 창과 같은 실장치를 공유한다.</summary>
    private readonly HostNsInfoDevices _devices;
    /// <summary>같은 실제 NSInfo 창들이다. 세 번째 NSInfo까지 만든다.</summary>
    private readonly List<HostNsInfoWindows> _windows = [];
    /// <summary>살아 있는 각 창의 실제 NSInfo 루트 토큰이다. F6 지식 메뉴 토큰과 구별한다.</summary>
    private readonly List<uint> _tokens = [];
    /// <summary>수명 대여 반납 상태다.</summary>
    private bool _disposed;

    /// <summary>같은 실제 파일 이름 작성기를 빌린다. VFS와 소유권을 공유하며 닫을 때 반납하지 않는다.</summary>
    public RawFileName FileNames { get; }
    /// <summary>첫 NSInfo 창이다. 미션 진입 Root가 소유한다.</summary>
    public HostNsInfoWindows Primary => _windows.Count != 0 ? _windows[0] : throw new InvalidOperationException("Root 공장 집합의 첫 NSInfo창 없음");
    /// <summary>같은 공유 고리의 실제 메시지 공장이다. 미션 진입 Root가 소유한다.</summary>
    public RawUberMessageProbe MessageProbe { get; }
    /// <summary>살아 있는 NSInfo 창 수다. 첫 창과 추가 창을 합친다.</summary>
    public int Count => _tokens.Count;

    /// <summary>반납되지 않은 같은 장기 트리·문자·NSInfo·Gameplay·물리 소유자를 재사용하는지 확인한다. 창을 생성하거나 현재값을 초기화하지 않는다.</summary>
    public bool Borrows(GumpButtonTree tree, HostHtmlText text, ClientNsInfoGlobals globals, ClientGameplayGlobals gameplay, RawSurfaceStorage storage)
        => !_disposed && ReferenceEquals(_tree, tree) && ReferenceEquals(_text, text) && ReferenceEquals(_globals, globals)
            && ReferenceEquals(_gameplay, gameplay) && ReferenceEquals(_storage, storage);

    /// <summary>같은 실제 트리와 문자와 NSInfo와 월드 저장소로 첫 창과 메시지 공장을 만든다. 값을 복사하거나 초기화하지 않는다.</summary>
    /// <param name="tree">같은 실제 노드와 등록 소유자다.</param>
    /// <param name="text">동일 스크롤과 스타일과 한 줄 문자 본체다.</param>
    /// <param name="globals">같은 현재 NSInfo와 zone과 서버 목록 owner다.</param>
    /// <param name="gameplay">같은 실제 모드와 TYPE 물리다.</param>
    /// <param name="storage">같은 실제 풀과 자산 저장소다.</param>
    /// <param name="fileNames">같은 실제 원문 파일 이름 작성기다.</param>
    /// <param name="hooks">바깥 UI 장치와 Root 묶음과 메시지 장치다.</param>
    public HostRootFactorySet(GumpButtonTree tree, HostHtmlText text, ClientNsInfoGlobals globals, ClientGameplayGlobals gameplay, RawSurfaceStorage storage, RawFileName fileNames, HostRootFactorySetHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(tree);
        ArgumentNullException.ThrowIfNull(text);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(gameplay);
        ArgumentNullException.ThrowIfNull(storage);
        ArgumentNullException.ThrowIfNull(fileNames);
        ArgumentNullException.ThrowIfNull(hooks);
        ArgumentNullException.ThrowIfNull(hooks.BaseDevices);
        ArgumentNullException.ThrowIfNull(hooks.Root);
        ArgumentNullException.ThrowIfNull(hooks.Message);
        _tree = tree;
        _text = text;
        _globals = globals;
        _gameplay = gameplay;
        _storage = storage;
        FileNames = fileNames;
        _devices = HostNsInfoRootDevices.Build(hooks.Root, hooks.BaseDevices);
        var primary = new HostNsInfoWindows(tree, text, globals, gameplay, storage, _devices);
        try
        {
            _windows.Add(primary);
            _tokens.Add(primary.Create());
            MessageProbe = new RawUberMessageProbe(fileNames, hooks.Message);
        }
        catch
        {
            primary.Dispose();
            _windows.Clear();
            throw;
        }
    }

    /// <summary>같은 주인에 둘째와 세째 NSInfo 창을 만든다. F6 지식 메뉴는 만들지 않는다.</summary>
    /// <returns>추가 창의 순서다. 첫 창은 0이다.</returns>
    public int CreateExtra()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        var windows = new HostNsInfoWindows(_tree, _text, _globals, _gameplay, _storage, _devices);
        try
        {
            _windows.Add(windows);
            _tokens.Add(windows.Create());
            return _windows.Count - 1;
        }
        catch
        {
            windows.Dispose();
            _windows.Remove(windows);
            throw;
        }
    }

    /// <summary>State 정리로 첫 창이 실제 해제된 뒤 같은 첫 공장으로 새 창을 생성하고 장부를 갱신한다.</summary>
    public uint RecreatePrimary()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (_tokens.Count != 0 && _tree.Node(_tokens[0]) is not null)
            throw new InvalidOperationException("살아 있는 첫 NSInfo 창의 중복 생성 금지");
        uint token = Primary.Create();
        if (_tokens.Count == 0) _tokens.Add(token);
        else _tokens[0] = token;
        return token;
    }

    /// <summary>살아 있는 창이 소유한 실제 NSInfo 루트 토큰인지 판정한다. F6 등 바깥 토큰은 거짓이다.</summary>
    /// <param name="token">검사할 토큰이다.</param>
    public bool OwnsToken(uint token)
    {
        if (_disposed) return false;
        // 살아 있는 각 창의 루트 토큰과 순서대로 비교한다.
        foreach (uint owned in _tokens)
        {
            if (owned == token) return true;
        }

        return false;
    }

    /// <summary>순서로 살아 있는 창을 빌린다.</summary>
    /// <param name="index">첫 창은 0이다.</param>
    public HostNsInfoWindows At(int index)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (index < 0 || index >= _windows.Count) throw new ArgumentOutOfRangeException(nameof(index), "Root 공장 집합의 실제 NSInfo창 범위");
        return _windows[index];
    }

    /// <summary>공통 자식 우선 소멸과 미등록 부분 할당을 자기 owner 범위에서만 반납한다.</summary>
    public void CloseAll()
    {
        // 등록표 순서대로 각 창의 자기 노드만 Kill한다.
        foreach (HostNsInfoWindows windows in _windows)
        {
            windows.Close();
        }

        _tokens.Clear();
    }

    /// <summary>자기 전체 노드 뒤 자기 특수 그림 대여만 반납하고 다른 UI와 공유 문자는 유지한다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        // 뒤에 만든 창부터 반납한다.
        for (int index = _windows.Count - 1; index >= 0; index--)
        {
            _windows[index].Dispose();
        }

        _windows.Clear();
        _tokens.Clear();
    }
}
