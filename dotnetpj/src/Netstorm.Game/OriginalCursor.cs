using System.Runtime.InteropServices;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Game;

/// <summary>원본 Windows 단색 커서를 SDL로 재생한다. 게임 프레임과 별개로 움직이며 AND/XOR의 배경 반전을 보존한다.</summary>
internal sealed class OriginalCursor : IDisposable
{
    /// <summary>확인된 상태와 exe의 RT_CURSOR 이미지 번호. 실행 중에는 동봉된 리소스만 읽는다.</summary>
    private static readonly (GameCursor Kind, int Resource)[] Resources =
        [(GameCursor.Arrow, 8), (GameCursor.Forbidden, 5), (GameCursor.Move, 20), (GameCursor.Temple, 7), (GameCursor.Place, 6)];

    /// <summary>SDL 단색 커서 생성 함수의 ABI.</summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate nint CreateCursor([In] byte[] data, [In] byte[] mask, int width, int height, int hotX, int hotY);
    /// <summary>SDL 커서 지정·해제 함수의 ABI.</summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void CursorAction(nint cursor);
    /// <summary>SDL 현재 커서 조회 함수의 ABI.</summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate nint GetCursor();

    /// <summary>MonoGame와 같은 SDL 라이브러리의 추가 참조.</summary>
    private readonly nint _library;
    /// <summary>상태별 생성된 SDL 커서.</summary>
    private readonly Dictionary<GameCursor, nint> _handles = [];
    /// <summary>커서를 지정·해제·확인할 함수.</summary>
    private readonly CursorAction _set;
    private readonly CursorAction _free;
    private readonly GetCursor _get;
    /// <summary>종료 때 복구할 시스템 커서 (소유하지 않는다).</summary>
    private readonly nint _previous;
    /// <summary>네이티브 자원을 이미 해제했는지. 중복 종료에도 핸들을 다시 해제하지 않는다.</summary>
    private bool _disposed;
    /// <summary>현재 지정한 상태.</summary>
    public GameCursor Kind { get; private set; }
    /// <summary>SDL이 실제로 이 커서를 사용 중인지 (UI 검사에서 네이티브 연결도 확인한다).</summary>
    public bool Active => !_disposed && _handles.TryGetValue(Kind, out nint handle) && _get() == handle;

    /// <summary>게임 데이터의 원본 커서를 모두 읽어 SDL 핸들을 만든다.</summary>
    public OriginalCursor(GameResources resources)
    {
        string name = OperatingSystem.IsWindows() ? "SDL2.dll" : OperatingSystem.IsMacOS() ? "libSDL2-2.0.0.dylib" : "libSDL2-2.0.so.0";
        string bundled = Path.Combine(AppContext.BaseDirectory, "runtimes", RuntimeInformation.RuntimeIdentifier, "native", name);
        _library = File.Exists(bundled) ? NativeLibrary.Load(bundled)
            : NativeLibrary.Load(name, typeof(Mouse).Assembly, null);
        _set = Marshal.GetDelegateForFunctionPointer<CursorAction>(NativeLibrary.GetExport(_library, "SDL_SetCursor"));
        _free = Marshal.GetDelegateForFunctionPointer<CursorAction>(NativeLibrary.GetExport(_library, "SDL_FreeCursor"));
        _get = Marshal.GetDelegateForFunctionPointer<GetCursor>(NativeLibrary.GetExport(_library, "SDL_GetCursor"));
        var create = Marshal.GetDelegateForFunctionPointer<CreateCursor>(NativeLibrary.GetExport(_library, "SDL_CreateCursor"));
        _previous = _get();
        try
        {
            // 확인된 다섯 리소스를 한 번씩 만들고 이후 상태 전환에는 핸들만 바꾼다.
            foreach ((GameCursor kind, int id) in Resources)
            {
                byte[] bytes = resources.Files.TryReadAllBytes($"cursors/RT_CURSOR_{id}.bin")
                    ?? throw new FileNotFoundException($"원본 커서 데이터가 없습니다: {id}");
                CursorBitmap bitmap = CursorBitmap.Parse(bytes);
                nint handle = create(bitmap.Data, bitmap.Mask, bitmap.Width, bitmap.Height, bitmap.HotX, bitmap.HotY);
                if (handle == 0) throw new IOException($"SDL 커서 생성에 실패했습니다: {id}");
                _handles.Add(kind, handle);
            }
            Set(GameCursor.Arrow);
        }
        catch
        {
            Dispose();
            throw;
        }
    }

    /// <summary>마우스 상태가 바뀐 갱신 끝에 커서를 바꾼다. SDL이 창 재진입 때 복구한 상태도 맞춘다.</summary>
    public void Set(GameCursor kind)
    {
        Kind = kind;
        if (_get() != _handles[kind]) _set(_handles[kind]);
    }

    /// <summary>시스템 커서를 복구한 뒤 소유한 커서와 라이브러리 참조를 해제한다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _set(_previous);
        // 생성에 성공한 모든 네이티브 커서를 해제한다.
        foreach (nint handle in _handles.Values) _free(handle);
        _handles.Clear();
        NativeLibrary.Free(_library);
    }
}
